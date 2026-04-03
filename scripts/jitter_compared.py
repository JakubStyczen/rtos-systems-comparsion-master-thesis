import os
import json
import argparse
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import numpy as np

# ==========================================
# 1. COMMAND LINE ARGUMENTS HANDLING
# ==========================================
parser = argparse.ArgumentParser(description="Jitter analysis for RTOS and GPOS systems.")
parser.add_argument('-o', '--output', type=str, default='results', 
                    help='Name of the output folder for results (default: "results")')
parser.add_argument('-f', '--filter', action='store_true', 
                    help='Enable artifact filtration (outliers)')
parser.add_argument('-l', '--limit', type=int, default=10000, 
                    help='Filtration threshold in microseconds (default: 10000)')
parser.add_argument('-c', '--conf', type=str, default='config.json', 
                    help='Path to the configuration file (default: config.json)')

args = parser.parse_args()

FILTER_OUTLIERS = args.filter
FILTER_LIMIT_US = args.limit
OUTPUT_DIR_NAME = args.output
CONFIG_FILE = args.conf

# ==========================================
# 2. PATH CONFIGURATION
# ==========================================
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
RESULTS_DIR = os.path.join(BASE_DIR, OUTPUT_DIR_NAME)
os.makedirs(RESULTS_DIR, exist_ok=True)

print(f"--- STARTING ANALYSIS ---")
print(f"Output directory: {RESULTS_DIR}")
print(f"Filtration: {'ENABLED (Limit: ' + str(FILTER_LIMIT_US) + ' us)' if FILTER_OUTLIERS else 'DISABLED'}\n")

config_file = os.path.join(BASE_DIR, CONFIG_FILE)
if not os.path.exists(config_file):
    raise FileNotFoundError(f"Configuration file not found at: {config_file}")

with open(config_file, 'r', encoding='utf-8') as f:
    config = json.load(f)

target_time_us = config.get("target_time_us", 1000)
dataframes = []

# ==========================================
# 3. JITTER CALCULATION AND CLEANING
# ==========================================
for os_name, os_data in config["systems"].items():
    divisor = os_data["divisor"]
    
    for load_type, file_name in os_data.get("files", {}).items():
        file_path = os.path.join(BASE_DIR, file_name)
        
        if os.path.exists(file_path):
            # Robust data loading
            df_temp = pd.read_csv(file_path, sep=';', header=None, names=['time_ms', 'timestamp'], skip_blank_lines=True)
            
            # Enforce numeric types and remove garbage (e.g., empty lines, UART errors)
            df_temp['timestamp'] = pd.to_numeric(df_temp['timestamp'], errors='coerce')
            df_temp = df_temp.dropna(subset=['timestamp']).copy()
            
            # Physical calculations
            df_temp['delta_raw'] = df_temp['timestamp'].diff()
            df_temp = df_temp.dropna(subset=['delta_raw']).copy()
            df_temp['measured_time_us'] = df_temp['delta_raw'] / divisor
            df_temp['Jitter_us'] = (df_temp['measured_time_us'] - target_time_us).abs()
            
            # Save unfiltered, processed data
            save_df = df_temp[['measured_time_us', 'Jitter_us']].copy()
            out_file = f"processed_{os_name.lower()}_{load_type.lower()}.csv"
            save_df.to_csv(os.path.join(RESULTS_DIR, out_file), index=False, sep=';')
            
            # Apply filter for statistical analysis and plots
            if FILTER_OUTLIERS:
                count_before = len(df_temp)
                df_temp = df_temp[df_temp['Jitter_us'] <= FILTER_LIMIT_US].copy()
                removed = count_before - len(df_temp)
                if removed > 0:
                    print(f" -> {os_name} ({load_type}): Removed {removed} artifacts (> {FILTER_LIMIT_US} us)")

            df_temp['OS'] = os_name
            df_temp['Load'] = load_type
            dataframes.append(df_temp[['Jitter_us', 'OS', 'Load']])
            print(f" -> Successfully processed: {os_name} [{load_type}]")
        else:
            print(f" -> SKIPPED (File not found): {file_path}")

if not dataframes:
    raise ValueError("No data was processed. Please check paths in config.json.")

df_all = pd.concat(dataframes, ignore_index=True)
df_all['OS'] = pd.Categorical(df_all['OS'], categories=list(config["systems"].keys()), ordered=True)

# ==========================================
# 4. SUMMARY STATISTICS
# ==========================================
stats = df_all.groupby(['OS', 'Load'])['Jitter_us'].agg(
    Min='min', Median='median', Mean='mean',
    P90=lambda x: x.quantile(0.90), P95=lambda x: x.quantile(0.95), 
    P99=lambda x: x.quantile(0.99), P99_9=lambda x: x.quantile(0.999), 
    Max='max', Std='std'
).round(2)

stats.to_csv(os.path.join(RESULTS_DIR, 'summary_statistics.csv'))
print("\n=== SUMMARY STATISTICS ===")
print(stats)

# ==========================================
# 5. VISUALIZATION
# ==========================================
print("\nGenerating plots...")
sns.set_theme(style="whitegrid")
plt.rcParams['figure.dpi'] = 300

# Violin Plot (Summary)
plt.figure(figsize=(12, 6))
ax = sns.violinplot(data=df_all, x="OS", y="Jitter_us", hue="Load", split=True, inner="quart")
ax.set_yscale("log")
title_violin = f"T={target_time_us}us Jitter Distribution: Idle vs Stress"
if FILTER_OUTLIERS: 
    title_violin += f" (Filter: < {FILTER_LIMIT_US} us)"
plt.title(title_violin)
plt.ylabel("Jitter [us] (Log Scale)")
plt.xlabel("Operating System")
plt.tight_layout()
plt.savefig(os.path.join(RESULTS_DIR, 'violin_comparison.png'))
plt.close()

# Loop through load states for ECDF and Histograms
load_states = df_all['Load'].unique()

for load_type in load_states:
    df_load = df_all[df_all['Load'] == load_type]
    if df_load.empty: 
        continue
    
    # ECDF Plot
    plt.figure(figsize=(10, 6))
    sns.ecdfplot(data=df_load, x="Jitter_us", hue="OS", log_scale=True, linewidth=2.5)
    plt.title(f"T={target_time_us}us Empirical Cumulative Distribution Function (ECDF) - State: {load_type}")
    plt.xlabel("Jitter [us] (Log Scale)")
    plt.ylabel("Cumulative Probability")
    plt.grid(True, which="both", ls="--", alpha=0.5)
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, f'ecdf_{load_type.lower()}.png'))
    plt.close()

    # Histogram Plot
    plt.figure(figsize=(10, 6))
    min_val = max(0.1, df_load['Jitter_us'].min())
    max_val = max(1.0, df_load['Jitter_us'].max())
    log_bins = np.logspace(np.log10(min_val), np.log10(max_val), 100)
    
    ax = sns.histplot(data=df_load, x="Jitter_us", hue="OS", 
                      element="step", fill=False, bins=log_bins, linewidth=2)
    ax.set_xscale("log")
    ax.set_yscale("log")
    plt.title(f"Jitter Histogram - State: {load_type} (Log-Log Scale)")
    plt.xlabel("Jitter [us] (Log Scale)")
    plt.ylabel("Count (Log Scale)")
    plt.grid(True, which="both", ls="--", alpha=0.5)
    plt.tight_layout()
    plt.savefig(os.path.join(RESULTS_DIR, f'histogram_{load_type.lower()}.png'))
    plt.close()

print(f"\nAnalysis completed successfully! Results saved in: {RESULTS_DIR}")