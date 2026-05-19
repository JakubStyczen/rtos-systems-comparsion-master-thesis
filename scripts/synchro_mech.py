import os
import json
import argparse
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np

# ==========================================
# 1. CLI ARGUMENT PARSING
# ==========================================
parser = argparse.ArgumentParser(description="Analyze Synchronization Latency for RTOS and GPOS.")
parser.add_argument('-o', '--output', type=str, default='results', 
                    help='Target directory for output files (default: results)')
parser.add_argument('-f', '--filter', action='store_true', 
                    help='Enable artifact filtering (outliers removal)')
parser.add_argument('-l', '--limit', type=int, default=10000, 
                    help='Cut-off limit for filtering in microseconds (default: 10000)')
parser.add_argument('-c', '--conf', type=str, default='config.json', 
                    help='Path to the configuration file (default: config.json)')
parser.add_argument('--ymax', type=float, default=None, 
                    help='Maximum value for Y-axis (default: auto)')

args = parser.parse_args()

FILTER_OUTLIERS = args.filter
FILTER_LIMIT_US = args.limit
OUTPUT_DIR_NAME = args.output
CONFIG_FILE_PATH = args.conf
Y_MAX = args.ymax

# ==========================================
# 2. PATH CONFIGURATION
# ==========================================
# Handle both absolute and relative paths for config
if not os.path.isabs(CONFIG_FILE_PATH):
    BASE_DIR = os.path.dirname(os.path.abspath(__file__))
    config_file = os.path.join(BASE_DIR, CONFIG_FILE_PATH)
else:
    BASE_DIR = os.path.dirname(CONFIG_FILE_PATH)
    config_file = CONFIG_FILE_PATH

RESULTS_DIR = os.path.join(BASE_DIR, OUTPUT_DIR_NAME)
os.makedirs(RESULTS_DIR, exist_ok=True)

print(f"--- STARTING SYNCHRONIZATION ANALYSIS ---")
print(f"Output Directory: {RESULTS_DIR}")
print(f"Config File: {config_file}")
print(f"Filtering: {'ENABLED (Limit: ' + str(FILTER_LIMIT_US) + ' us)' if FILTER_OUTLIERS else 'DISABLED'}\n")

if not os.path.exists(config_file):
    raise FileNotFoundError(f"Configuration file not found: {config_file}")

with open(config_file, 'r', encoding='utf-8') as f:
    config = json.load(f)

sync_mechanism = config.get("target_sync_mechanism", "Sync Mechanism")
dataframes = []

# ==========================================
# 3. LATENCY CALCULATION & DATA CLEANING
# ==========================================
for os_name, os_data in config["systems"].items():
    divisor = os_data["divisor"]
    file_name = os_data.get("file")
    
    if not file_name:
        print(f" -> SKIPPED: No file specified for {os_name}")
        continue
        
    file_path = os.path.join(BASE_DIR, file_name)
    
    if os.path.exists(file_path):
        # Robust data loading
        df_temp = pd.read_csv(file_path, sep=';', header=None, names=['t1', 't2'], skip_blank_lines=True)
        
        # Force numeric types and remove garbage (e.g., UART errors, empty lines)
        df_temp['t1'] = pd.to_numeric(df_temp['t1'], errors='coerce')
        df_temp['t2'] = pd.to_numeric(df_temp['t2'], errors='coerce')
        df_temp = df_temp.dropna(subset=['t1', 't2']).copy()
        
        # Calculate Delta T (t2 - t1)
        df_temp['delta_raw'] = df_temp['t2'] - df_temp['t1']
        
        # Filter out hardware timer overflows (t2 < t1)
        overflow_count = len(df_temp[df_temp['delta_raw'] < 0])
        df_temp = df_temp[df_temp['delta_raw'] >= 0].copy()
        if overflow_count > 0:
            print(f"    [!] Warning: Ignored {overflow_count} timer overflow samples for {os_name}.")
            
        # Convert to microseconds
        df_temp['Latency_us'] = df_temp['delta_raw'] / divisor
        
        # Save raw processed data (before upper limit filtering)
        save_df = df_temp[['Latency_us']].copy()
        safe_mech_name = sync_mechanism.replace(" ", "_").lower()
        out_file = f"processed_{os_name.lower()}_{safe_mech_name}.csv"
        save_df.to_csv(os.path.join(RESULTS_DIR, out_file), index=False, sep=';')
        
        # Apply artifact filtering for stats and plots
        if FILTER_OUTLIERS:
            count_before = len(df_temp)
            df_temp = df_temp[df_temp['Latency_us'] <= FILTER_LIMIT_US].copy()
            removed = count_before - len(df_temp)
            if removed > 0:
                print(f"    -> Filtered out {removed} outliers (> {FILTER_LIMIT_US} us)")

        df_temp['OS'] = os_name
        dataframes.append(df_temp[['Latency_us', 'OS']])
        print(f" -> Successfully processed: {os_name}")
    else:
        print(f" -> SKIPPED (File not found): {file_path}")

if not dataframes:
    raise ValueError("No data processed. Please check file paths in your config file.")

df_all = pd.concat(dataframes, ignore_index=True)
df_all['OS'] = pd.Categorical(df_all['OS'], categories=list(config["systems"].keys()), ordered=True)

# Znajdź najmniejszą wartość > 0 w danych (minimum 0.001)
min_nonzero_value = max(0.001, df_all[df_all['Latency_us'] > 0]['Latency_us'].min())
if pd.isna(min_nonzero_value):
    min_nonzero_value = 0.001  # Fallback jeśli wszystkie wartości to zera

print(f"Found minimum non-zero latency value: {min_nonzero_value:.6f} us")

# Zamieniamy idealne zera na najmniejszą zliczoną próbkę czasu
df_all['Latency_us'] = df_all['Latency_us'].replace(0.0, min_nonzero_value)

# ==========================================
# 4. SUMMARY STATISTICS
# ==========================================
stats = df_all.groupby('OS')['Latency_us'].agg(
    Min='min', Median='median', Mean='mean',
    P90=lambda x: x.quantile(0.90), P95=lambda x: x.quantile(0.95), 
    P99=lambda x: x.quantile(0.99), P99_9=lambda x: x.quantile(0.999), 
    Max='max', Std='std'
).round(3)

stats.to_csv(os.path.join(RESULTS_DIR, 'summary_statistics.csv'))
print("\n=== SUMMARY STATISTICS ===")
print(stats)

# ==========================================
# 5. VISUALIZATION
# ==========================================
print("\nGenerating plots...")
sns.set_theme(style="whitegrid")
plt.rcParams['figure.dpi'] = 300

# Plot 1: Violin Plot
# plt.figure(figsize=(10, 6))
# ax = sns.violinplot(data=df_all, x="OS", y="Latency_us", inner="quart", palette="muted")
# ax.set_yscale("log")
# title_violin = f"{sync_mechanism} Latency Distribution"
# if FILTER_OUTLIERS: title_violin += f" (Filter: < {FILTER_LIMIT_US} us)"
# plt.title(title_violin, fontsize=14, pad=15)
# plt.ylabel("Latency [us] (Log Scale)", fontsize=12)
# plt.xlabel("Operating System", fontsize=12)
# plt.tight_layout()
# plt.savefig(os.path.join(RESULTS_DIR, 'plot_violin.png'))
# plt.close()

# Plot 2: Empirical CDF
plt.figure(figsize=(10, 6))
sns.ecdfplot(data=df_all, x="Latency_us", hue="OS", log_scale=True, linewidth=2.5)
plt.title(f"Empirical CDF - {sync_mechanism}", fontsize=14, pad=15)
plt.xlabel("Latency [us] (Log Scale)", fontsize=12)
plt.ylabel("Cumulative Probability", fontsize=12)
plt.grid(True, which="both", ls="--", alpha=0.5)
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, 'plot_ecdf.png'))
plt.close()

# Plot 3: Step Histogram
# --- POPRAWKA: Rozszerzamy granice koszyków, aby zamknąć wykres ---
plt.figure(figsize=(10, 6))
min_val = max(0.001, df_all['Latency_us'].min())
max_val = max(1.0, df_all['Latency_us'].max())

# Mnożymy i dzielimy przez 2 (lub inny margines), aby stworzyć "puste" koszyki na bokach
# Dzięki temu linia schodkowa naturalnie spadnie do zera (osi X)
lower_bound = min_val * 0.5
upper_bound = max_val * 2.0

log_bins = np.logspace(np.log10(lower_bound), np.log10(upper_bound), 100)

ax = sns.histplot(data=df_all, x="Latency_us", hue="OS", 
                  element="step", fill=False, bins=log_bins, linewidth=2)
ax.set_xscale("log")
ax.set_yscale("log")
plt.title(f"Latency Histogram - {sync_mechanism} (Log-Log Scale)", fontsize=14, pad=15)
plt.xlabel("Latency [us]", fontsize=12)
plt.ylabel("Count", fontsize=12)
plt.grid(True, which="both", ls="--", alpha=0.5)
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, 'plot_histogram.png'))
plt.close()

print(f"\nSuccess! All results and plots have been saved to: {RESULTS_DIR}")