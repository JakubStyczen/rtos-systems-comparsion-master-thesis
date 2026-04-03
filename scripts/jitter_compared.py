# import os
# import json
# import pandas as pd
# import seaborn as sns
# import matplotlib.pyplot as plt
# import numpy as np

# # ==========================================
# # KONFIGURACJA ANALIZY
# # ==========================================
# FILTER_OUTLIERS = True       # Flaga odfiltrowania artefaktów (np. overflow licznika)
# FILTER_LIMIT_US = 100_000    # Próg odcięcia: 100 000 us (100 ms)

# BASE_DIR = os.path.dirname(os.path.abspath(__file__))
# RESULTS_DIR = os.path.join(BASE_DIR, 'results')
# os.makedirs(RESULTS_DIR, exist_ok=True)

# with open(os.path.join(BASE_DIR, 'config.json'), 'r', encoding='utf-8') as f:
#     config = json.load(f)

# target_time_us = config["target_time_us"]
# dataframes = []

# # 

# # ==========================================
# # PRZETWARZANIE DANYCH
# # ==========================================
# for os_name, os_data in config["systems"].items():
#     divisor = os_data["divisor"]
    
#     for load_type, file_name in os_data["files"].items():
#         file_path = os.path.join(BASE_DIR, file_name)
        
#         if os.path.exists(file_path):
#             # Wczytanie surowych danych (skip_blank_lines dla pewności)
#             df_temp = pd.read_csv(file_path, sep=';', header=None, names=['czas_ms', 'timestamp'], skip_blank_lines=True)
            
#             # ==========================================
#             # CZYSZCZENIE DANYCH (Odsiewanie pustych linii i błędów UART)
#             # ==========================================
#             # 1. Wymuszamy typ numeryczny. Jeśli w linii był jakiś tekst/śmieć, zamieni się na NaN.
#             df_temp['timestamp'] = pd.to_numeric(df_temp['timestamp'], errors='coerce')
            
#             # 2. Usuwamy wszystkie wiersze, które nie mają poprawnego znacznika czasu (czyli puste lub zepsute linie)
#             df_temp = df_temp.dropna(subset=['timestamp']).copy()
            
#             # ==========================================
#             # OBLICZENIA FIZYCZNE
#             # ==========================================
#             # Dopiero teraz, mając czysty, ciągły strumień liczb, liczymy różnicę
#             df_temp['delta_raw'] = df_temp['timestamp'].diff()
            
#             # Usuwamy pierwszy wiersz (bo nie ma poprzednika do obliczenia delty)
#             df_temp = df_temp.dropna(subset=['delta_raw']).copy()
            
#             # Przeliczenie na mikrosekundy i Jitter
#             df_temp['measured_time_us'] = df_temp['delta_raw'] / divisor
#             df_temp['Jitter_us'] = (df_temp['measured_time_us'] - target_time_us).abs()
            
#             # ZAPIS DANYCH REALNYCH (bez filtra) do folderu results
#             save_df = df_temp[['measured_time_us', 'Jitter_us']].copy()
#             out_file = f"processed_{os_name.lower()}_{load_type.lower()}.csv"
#             save_df.to_csv(os.path.join(RESULTS_DIR, out_file), index=False, sep=';')
            
#             # FILTRACJA DLA WYKRESÓW I STATYSTYK (jeśli flaga aktywna)
#             if FILTER_OUTLIERS:
#                 count_before = len(df_temp)
#                 df_temp = df_temp[df_temp['Jitter_us'] <= FILTER_LIMIT_US].copy()
#                 removed = count_before - len(df_temp)
#                 if removed > 0:
#                     print(f" -> {os_name} ({load_type}): Usunięto {removed} artefaktów (> {FILTER_LIMIT_US} us)")

#             df_temp['OS'] = os_name
#             df_temp['Load'] = load_type
#             dataframes.append(df_temp[['Jitter_us', 'OS', 'Load']])
#             print(f" -> Przetworzono {os_name} {load_type}")
#         else:
#             print(f" -> POMINIĘTO: Nie znaleziono {file_path}")

# df_all = pd.concat(dataframes, ignore_index=True)
# df_all['OS'] = pd.Categorical(df_all['OS'], categories=list(config["systems"].keys()), ordered=True)

# # ==========================================
# # STATYSTYKI ZBIORCZE
# # ==========================================
# stats = df_all.groupby(['OS', 'Load'])['Jitter_us'].agg(
#     Min='min', Mediana='median', Srednia='mean',
#     P95=lambda x: x.quantile(0.95), P99=lambda x: x.quantile(0.99),
#     P99_9=lambda x: x.quantile(0.999), Max='max', Std='std'
# ).round(2)

# stats.to_csv(os.path.join(RESULTS_DIR, 'summary_statistics.csv'))
# print("\nStatystyki po filtracji:")
# print(stats)

# # ==========================================
# # WIZUALIZACJA
# # ==========================================
# print("\nGenerowanie wykresów...")
# sns.set_theme(style="whitegrid")
# plt.rcParams['figure.dpi'] = 300

# # 1. Wykres Skrzypcowy (Split Violin) - to zostawiamy razem, bo to najlepszy sposób na pokazanie różnic
# plt.figure(figsize=(12, 6))
# ax = sns.violinplot(data=df_all, x="OS", y="Jitter_us", hue="Load", split=True, inner="quart")
# ax.set_yscale("log")
# plt.title(f"Rozkład Jittera: Idle vs Stress (Filtr: < {FILTER_LIMIT_US} us)")
# plt.ylabel("Jitter [us] (Skala Logarytmiczna)")
# plt.xlabel("System Operacyjny")
# plt.tight_layout()
# plt.savefig(os.path.join(RESULTS_DIR, 'violin_comparison.png'))
# plt.close()

# # Pętla generująca osobne wykresy ECDF i Histogramy dla Idle oraz Stress
# stany_obciazenia = df_all['Load'].unique()

# for load_type in stany_obciazenia:
#     print(f" -> Rysowanie wykresów dla stanu: {load_type}...")
#     df_load = df_all[df_all['Load'] == load_type]
    
#     # 2. ECDF dla konkretnego obciążenia
#     plt.figure(figsize=(10, 6))
#     sns.ecdfplot(data=df_load, x="Jitter_us", hue="OS", log_scale=True, linewidth=2.5)
#     plt.title(f"Dystrybuanta Empiryczna (ECDF) - Stan: {load_type}")
#     plt.xlabel("Jitter [us] (Skala Logarytmiczna)")
#     plt.ylabel("Prawdopodobieństwo skumulowane")
#     plt.grid(True, which="both", ls="--", alpha=0.5)
#     plt.tight_layout()
#     nazwa_ecdf = f'ecdf_{load_type.lower()}.png'
#     plt.savefig(os.path.join(RESULTS_DIR, nazwa_ecdf))
#     plt.close()

#     # 3. Histogramy Krokodylowe (Step) dla konkretnego obciążenia
#     plt.figure(figsize=(10, 6))
    
#     # Bezpieczne wyliczenie logarytmicznych przedziałów (binów)
#     min_val = max(0.1, df_load['Jitter_us'].min()) # Zabezpieczenie przed log(0)
#     max_val = df_load['Jitter_us'].max()
#     log_bins = np.logspace(np.log10(min_val), np.log10(max_val), 100)
    
#     ax = sns.histplot(data=df_load, x="Jitter_us", hue="OS", 
#                       element="step", fill=False, bins=log_bins, linewidth=2)
#     ax.set_xscale("log")
#     ax.set_yscale("log")
#     plt.title(f"Histogram Jittera - Stan: {load_type} (Skala Log-Log)")
#     plt.xlabel("Jitter [us] (Skala Logarytmiczna)")
#     plt.ylabel("Liczba zliczeń (Skala Logarytmiczna)")
#     plt.grid(True, which="both", ls="--", alpha=0.5)
#     plt.tight_layout()
#     nazwa_hist = f'histogram_{load_type.lower()}.png'
#     plt.savefig(os.path.join(RESULTS_DIR, nazwa_hist))
#     plt.close()

# print(f"\nWszystkie wyniki i odseparowane wykresy zapisano w folderze: {RESULTS_DIR}")

import os
import json
import argparse
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np

# ==========================================
# 1. OBSŁUGA ARGUMENTÓW Z TERMINALA
# ==========================================
parser = argparse.ArgumentParser(description="Analiza fluktuacji czasowych (Jitter) dla systemów RTOS i GPOS.")
parser.add_argument('-o', '--output', type=str, default='results', 
                    help='Nazwa folderu docelowego na wyniki (domyślnie: "results")')
parser.add_argument('-f', '--filter', action='store_true', 
                    help='Włącza filtrację artefaktów (wartości odstających)')
parser.add_argument('-l', '--limit', type=int, default=10000, 
                    help='Próg odcięcia dla filtru w mikrosekundach (domyślnie: 10000)')
parser.add_argument('-c', '--conf', type=str, default='config.json', 
                    help='Ścieżka do pliku konfiguracyjnego (domyślnie: config.json)')

args = parser.parse_args()

FILTER_OUTLIERS = args.filter
FILTER_LIMIT_US = args.limit
OUTPUT_DIR_NAME = args.output
CONFIG_FILE = args.conf

# ==========================================
# 2. KONFIGURACJA ŚCIEŻEK
# ==========================================
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR = os.path.join(BASE_DIR, OUTPUT_DIR_NAME)
os.makedirs(RESULTS_DIR, exist_ok=True)

print(f"--- ROZPOCZĘCIE ANALIZY ---")
print(f"Folder wynikowy: {RESULTS_DIR}")
print(f"Filtracja: {'WŁĄCZONA (Limit: ' + str(FILTER_LIMIT_US) + ' us)' if FILTER_OUTLIERS else 'WYŁĄCZONA'}\n")

config_file = os.path.join(BASE_DIR, CONFIG_FILE)
if not os.path.exists(config_file):
    raise FileNotFoundError(f"Brak pliku konfiguracyjnego w: {config_file}")

with open(config_file, 'r', encoding='utf-8') as f:
    config = json.load(f)

target_time_us = config.get("target_time_us", 1000)
dataframes = []

# ==========================================
# 3. WYZNACZANIE JITTERA I CZYSZCZENIE
# ==========================================
for os_name, os_data in config["systems"].items():
    divisor = os_data["divisor"]
    
    for load_type, file_name in os_data.get("files", {}).items():
        file_path = os.path.join(BASE_DIR, file_name)
        
        if os.path.exists(file_path):
            # Odporne wczytanie danych
            df_temp = pd.read_csv(file_path, sep=';', header=None, names=['czas_ms', 'timestamp'], skip_blank_lines=True)
            
            # Wymuszenie liczb i usunięcie śmieci (np. pustych linii, błędów UART)
            df_temp['timestamp'] = pd.to_numeric(df_temp['timestamp'], errors='coerce')
            df_temp = df_temp.dropna(subset=['timestamp']).copy()
            
            # Obliczenia fizyczne
            df_temp['delta_raw'] = df_temp['timestamp'].diff()
            df_temp = df_temp.dropna(subset=['delta_raw']).copy()
            df_temp['measured_time_us'] = df_temp['delta_raw'] / divisor
            df_temp['Jitter_us'] = (df_temp['measured_time_us'] - target_time_us).abs()
            
            # Zapis niefiltrowanych, przetworzonych danych
            save_df = df_temp[['measured_time_us', 'Jitter_us']].copy()
            out_file = f"processed_{os_name.lower()}_{load_type.lower()}.csv"
            save_df.to_csv(os.path.join(RESULTS_DIR, out_file), index=False, sep=';')
            
            # Aplikacja filtru do analizy statystycznej i wykresów
            if FILTER_OUTLIERS:
                count_before = len(df_temp)
                df_temp = df_temp[df_temp['Jitter_us'] <= FILTER_LIMIT_US].copy()
                removed = count_before - len(df_temp)
                if removed > 0:
                    print(f" -> {os_name} ({load_type}): Usunięto {removed} artefaktów (> {FILTER_LIMIT_US} us)")

            df_temp['OS'] = os_name
            df_temp['Load'] = load_type
            dataframes.append(df_temp[['Jitter_us', 'OS', 'Load']])
            print(f" -> Pomyślnie przetworzono: {os_name} [{load_type}]")
        else:
            print(f" -> POMINIĘTO (Brak pliku): {file_path}")

if not dataframes:
    raise ValueError("Nie przetworzono żadnych danych. Sprawdź ścieżki w config.json.")

df_all = pd.concat(dataframes, ignore_index=True)
df_all['OS'] = pd.Categorical(df_all['OS'], categories=list(config["systems"].keys()), ordered=True)

# ==========================================
# 4. STATYSTYKI ZBIORCZE
# ==========================================
stats = df_all.groupby(['OS', 'Load'])['Jitter_us'].agg(
    Min='min', Mediana='median', Srednia='mean',
    P90=lambda x: x.quantile(0.90), P95=lambda x: x.quantile(0.95), 
    P99=lambda x: x.quantile(0.99), P99_9=lambda x: x.quantile(0.999), 
    Max='max', Std='std'
).round(2)

stats.to_csv(os.path.join(RESULTS_DIR, 'summary_statistics.csv'))
print("\n=== STATYSTYKI ZBIORCZE ===")
print(stats)

# ==========================================
# 5. WIZUALIZACJA
# ==========================================
print("\nGenerowanie wykresów...")
sns.set_theme(style="whitegrid")
plt.rcParams['figure.dpi'] = 300

# Wykres Skrzypcowy (zbiorczy)
plt.figure(figsize=(12, 6))
ax = sns.violinplot(data=df_all, x="OS", y="Jitter_us", hue="Load", split=True, inner="quart")
ax.set_yscale("log")
tytul_violin = "Rozkład Jittera: Idle vs Stress"
if FILTER_OUTLIERS: tytul_violin += f" (Filtr: < {FILTER_LIMIT_US} us)"
plt.title(tytul_violin)
plt.ylabel("Jitter [us] (Skala Logarytmiczna)")
plt.xlabel("System Operacyjny")
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, 'violin_comparison.png'))
plt.close()

# Pętla po stanach obciążenia dla ECDF i Histogramów
stany_obciazenia = df_all['Load'].unique()

for load_type in stany_obciazenia:
    df_load = df_all[df_all['Load'] == load_type]
    if df_load.empty: continue
    
    # ECDF
    plt.figure(figsize=(10, 6))
    sns.ecdfplot(data=df_load, x="Jitter_us", hue="OS", log_scale=True, linewidth=2.5)
    plt.title(f"Dystrybuanta Empiryczna (ECDF) - Stan: {load_type}")
    plt.xlabel("Jitter [us] (Skala Logarytmiczna)")
    plt.ylabel("Prawdopodobieństwo skumulowane")
    plt.grid(True, which="both", ls="--", alpha=0.5)
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, f'ecdf_{load_type.lower()}.png'))
    plt.close()

    # Histogram
    plt.figure(figsize=(10, 6))
    min_val = max(0.1, df_load['Jitter_us'].min())
    max_val = max(1.0, df_load['Jitter_us'].max())
    log_bins = np.logspace(np.log10(min_val), np.log10(max_val), 100)
    
    ax = sns.histplot(data=df_load, x="Jitter_us", hue="OS", 
                      element="step", fill=False, bins=log_bins, linewidth=2)
    ax.set_xscale("log")
    ax.set_yscale("log")
    plt.title(f"Histogram Jittera - Stan: {load_type} (Skala Log-Log)")
    plt.xlabel("Jitter [us] (Skala Logarytmiczna)")
    plt.ylabel("Liczba zliczeń (Skala Logarytmiczna)")
    plt.grid(True, which="both", ls="--", alpha=0.5)
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, f'histogram_{load_type.lower()}.png'))
    plt.close()

print(f"\nZakończono sukcesem! Zapisano w folderze: {RESULTS_DIR}")