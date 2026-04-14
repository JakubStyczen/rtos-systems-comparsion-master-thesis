import os
import json
import argparse
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LogNorm

# ==========================================
# 1. CLI ARGUMENTS
# ==========================================
parser = argparse.ArgumentParser(description="Analyze Jitter Series with Outlier Filtering.")
parser.add_argument('-o', '--output', type=str, default='results_series', 
                    help='Output directory (default: results_series)')
parser.add_argument('-c', '--conf', type=str, default='config.json', 
                    help='Config file path (default: config.json)')
parser.add_argument('-f', '--filter', action='store_true', 
                    help='Enable outlier filtering')
parser.add_argument('-l', '--limit', type=int, default=10000, 
                    help='Jitter cut-off limit in microseconds (default: 10000)')

args = parser.parse_args()

FILTER_OUTLIERS = args.filter
FILTER_LIMIT_US = args.limit
RESULTS_DIR = args.output
CONFIG_FILE = args.conf

# Setup output directory
os.makedirs(RESULTS_DIR, exist_ok=True)

# ==========================================
# 2. UTILS & PARSING
# ==========================================
def parse_target_time(val):
    """Converts '100us' or '1ms' strings to float microseconds."""
    val = str(val).strip().lower()
    if 'ms' in val:
        return float(val.replace('ms', '')) * 1000.0
    elif 'us' in val:
        return float(val.replace('us', ''))
    return np.nan

# Load config
with open(CONFIG_FILE, 'r', encoding='utf-8') as f:
    config = json.load(f)

all_data = []

# ==========================================
# 3. DATA PROCESSING
# ==========================================
print(f"--- Processing Series Data (Filter: {FILTER_OUTLIERS}, Limit: {FILTER_LIMIT_US} us) ---")

for os_name, os_data in config["systems"].items():
    divisor = os_data["divisor"]
    group = os_data["group"]
    
    for load_type, file_name in os_data.get("files", {}).items():
        file_path = file_name
        
        if os.path.exists(file_path):
            # Wczytywanie i czyszczenie znaków '='
            df = pd.read_csv(file_path, sep=';', header=None, names=['target_str', 'timestamp'], skip_blank_lines=True)
            is_sep = df['target_str'].str.contains('=', na=False)
            df = df[~is_sep].copy()
            
            # Parsowanie do liczb
            df['Target_us'] = df['target_str'].apply(parse_target_time)
            df['timestamp'] = pd.to_numeric(df['timestamp'], errors='coerce')
            df = df.dropna(subset=['Target_us', 'timestamp']).copy()
            
            # --- KLUCZOWA ZMIANA: Ciągły DIFF() ---
            # Odejmowanie wartości idzie teraz przez cały plik. 
            # Dzięki temu pomiar 100us z nowego cyklu odejmie od siebie czas z 10000us z poprzedniego!
            df['delta_raw'] = df.groupby('Target_us')['timestamp'].diff()
            df = df.dropna(subset=['delta_raw']).copy()
            
            # --- IMPLEMENTACJA TWOJEGO "START TIMERA" ---
            # Dynamicznie sprawdzamy ile jest unikalnych timerów (6 dla RTOS, 5 dla Windows/Linux)
            # Odrzucamy pierwsze N pomiarów, traktując je jako kalibrację T0 (warm-up)
            unique_targets_count = df['Target_us'].nunique()
            if len(df) > unique_targets_count:
                df = df.iloc[(unique_targets_count - 1):].copy()
            
            # Obliczenia fizyczne i Jitter
            df['measured_time_us'] = df['delta_raw'] / divisor
            df['Jitter_us'] = (df['measured_time_us'] - df['Target_us']).abs()
            
            # ZAPIS DANYCH SUROWYCH (Przed filtrem)
            raw_save_path = os.path.join(RESULTS_DIR, f"raw_{os_name.lower()}_{load_type.lower()}.csv")
            df[['Target_us', 'measured_time_us', 'Jitter_us']].to_csv(raw_save_path, index=False, sep=';')

            # FILTRACJA (Zetnie nam artefakty na granicach serii)
            if FILTER_OUTLIERS:
                before = len(df)
                df = df[df['Jitter_us'] <= FILTER_LIMIT_US].copy()
                removed = before - len(df)
                if removed > 0:
                    print(f" -> {os_name} ({load_type}): Odrzucono {removed} wartości skrajnych (> {FILTER_LIMIT_US} us)")

            df['OS'] = os_name
            df['Group'] = group
            df['Load'] = load_type
            all_data.append(df[['Jitter_us', 'Target_us', 'OS', 'Group', 'Load']])
            print(f" -> Załadowano {os_name} [{load_type}] ({len(df)} pomiarów)")
        else:
            print(f" -> Brak pliku: {file_path}")

df_final = pd.concat(all_data, ignore_index=True)

# ==========================================
# EKSPORT STATYSTYK DO OPTYMALIZACJI WYKRESÓW
# ==========================================
print("\n=== GENEROWANIE DANYCH DIAGNOSTYCZNYCH DLA AI ===")

# Funkcje pomocnicze dla percentyli
def q1(x): return x.quantile(0.25)
def q3(x): return x.quantile(0.75)
def p999(x): return x.quantile(0.999)

# Agregacja kluczowych miar dla wygładzania KDE i osi Y
ai_stats = df_final.groupby(['OS', 'Load'])['Jitter_us'].agg(
    Min='min',
    Q1=q1,
    Mediana='median',
    Q3=q3,
    P99_9=p999,
    Max='max',
    Std='std'
).round(2)

# Obliczenie rozstępu międzykwartylowego (IQR)
ai_stats['IQR'] = (ai_stats['Q3'] - ai_stats['Q1']).round(2)

# Opcjonalnie: Przesunięcie kolumn dla lepszej czytelności
ai_stats = ai_stats[['Min', 'Q1', 'Mediana', 'Q3', 'IQR', 'P99_9', 'Max', 'Std']]

print("\nSkopiuj poniższy blok (tabelę Markdown) i wklej go w czacie:")
print("-" * 60)
print(ai_stats.to_markdown())
print("-" * 60)
print("\n")


# ==========================================
# 4. VISUALIZATION (INDIVIDUAL PER OS)
# ==========================================
print("\nGenerating final visualizations...")
sns.set_theme(style="whitegrid")
plt.rcParams['figure.dpi'] = 300

# Iterate directly over unique operating systems
for os_name in df_final['OS'].unique():
    df_os = df_final[df_final['OS'] == os_name].copy()
    if df_os.empty: continue
    
    print(f" -> Plotting charts for OS: {os_name}...")
    
    # Sort X-axis so timers appear in ascending order
    df_os['Target_us'] = pd.Categorical(df_os['Target_us'], 
                                         categories=sorted(df_os['Target_us'].unique()), 
                                         ordered=True)

# ==========================================
    # PLOT 1: DISTRIBUTION (BOXPLOT + STRIPPLOT FOR ALL OS)
    # ==========================================
    plt.figure(figsize=(10, 6))
    
    df_plot = df_os.copy()
    # Log scale fix: replace 0.0 with 0.1 to avoid math domain errors
    df_plot['Jitter_us'] = df_plot['Jitter_us'].replace(0.0, 0.1)
    
    # 1. Boxplot - główne "pudełka" pokazujące medianę i rozstęp kwartylowy (IQR)
    # Wyłączamy domyślne rysowanie outlierów (fliersize=0), bo zrobimy to lepiej punktami
    ax = sns.boxplot(
        data=df_plot, x="Target_us", y="Jitter_us", hue="Load",
        palette={"Idle": "#4C72B0", "Stress": "#C44E52"},
        fliersize=0, boxprops={'alpha': 0.6}, linewidth=1.5
    )
    
    # 2. Stripplot - chmura rzeczywistych pomiarów
    # Używamy alpha=0.15, żeby gęste skupiska punktów ładnie się cieniowały,
    # a pojedyncze outliery były nadal widoczne.
    sns.stripplot(
        data=df_plot, x="Target_us", y="Jitter_us", hue="Load",
        dodge=True, alpha=0.15, size=2, color=".2", linewidth=0, ax=ax, jitter=True
    )
    
    # Naprawa legendy (Stripplot domyślnie podwaja pozycje w legendzie)
    handles, labels = ax.get_legend_handles_labels()
    plt.legend(handles[0:2], labels[0:2], title="Load", bbox_to_anchor=(1.02, 0.5), loc='center left')
    
    plt.title(f"Jitter Distribution (Boxplot & Points) - {os_name}", fontsize=14, pad=15)
    
    ax.set_yscale("log")
    ax.set_ylim(bottom=0.05) # Utrzymuje dolną krawędź na czystym poziomie
    
    plt.xlabel("Target Timer [us]", fontsize=12)
    plt.ylabel("Jitter [us] (Log Scale)", fontsize=12)
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, f'distribution_{os_name.lower()}.png'))
    plt.close()

    # # ==========================================
    # # PLOT 2: WORST-CASE EXECUTION TIME (WCET) TREND
    # # ==========================================
    # plt.figure(figsize=(10, 6))
    # df_max = df_os.groupby(['Load', 'Target_us'])['Jitter_us'].max().reset_index()
    
    # sns.pointplot(data=df_max, x="Target_us", y="Jitter_us", hue="Load", 
    #               markers=["o", "s"], linestyles=["-", "--"], dodge=True)
    
    # plt.yscale("log")
    # plt.title(f"Max Jitter (WCET) Trend - {os_name}", fontsize=14, pad=15)
    # plt.xlabel("Target Timer [us]", fontsize=12)
    # plt.ylabel("Max Jitter [us] (Log Scale)", fontsize=12)
    # plt.grid(True, which="both", ls="--", alpha=0.5)
    # plt.legend(title="Load")
    # plt.tight_layout()
    # plt.savefig(os.path.join(RESULTS_DIR, f'trend_wcet_{os_name.lower()}.png'))
    # plt.close()

    # # ==========================================
    # # PLOT 3: 99.9th PERCENTILE HEATMAP
    # # ==========================================
    # plt.figure(figsize=(10, 4))
    # df_p99 = df_os.groupby(['Target_us', 'Load'])['Jitter_us'].quantile(0.999).reset_index()
    
    # # Pivot matrix: Columns = Timers, Rows = Idle/Stress
    # pivot = df_p99.pivot(index="Load", columns="Target_us", values="Jitter_us")
    
    # sns.heatmap(pivot, annot=True, fmt=".1f", cmap="YlOrRd", norm=LogNorm(), 
    #             cbar_kws={'label': 'Jitter P99.9 [us]'})
    
    # plt.title(f"99.9th Percentile Jitter - {os_name}", fontsize=14, pad=15)
    # plt.xlabel("Target Timer [us]", fontsize=12)
    # plt.ylabel("System State", fontsize=12)
    
    # # Ensure y-axis labels are readable (horizontal)
    # plt.yticks(rotation=0) 
    # plt.tight_layout()
    # plt.savefig(os.path.join(RESULTS_DIR, f'heatmap_{os_name.lower()}.png'))
    # plt.close()

print(f"\nAll visualizations (per OS) saved to: {RESULTS_DIR}")