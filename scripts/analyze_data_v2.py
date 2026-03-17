import argparse
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict
from pathlib import Path
import os
import csv

mpl.rcParams['axes.formatter.useoffset'] = False
mpl.rcParams['axes.formatter.use_mathtext'] = False

def percentile(data, p):
    data_sorted = sorted(data)
    k = (len(data_sorted) - 1) * (p / 100)
    f = int(k)
    c = min(f + 1, len(data_sorted) - 1)
    return data_sorted[f] + (data_sorted[c] - data_sorted[f]) * (k - f)

def process_and_plot(file_path, generate_plots=True):
    timestamps = {}
    counter = 0
    key_labels = f"diff_{counter}"

    with open(file_path, 'r') as file:
        for line in file:
            line = line.strip()
            if "=" in line:
                counter += 1
                key_labels = f"diff_{counter}"
                timestamps[key_labels] = defaultdict(list)
            if line and "=" not in line:
                try:
                    label, value = line.split(';')
                    timestamps[key_labels][label].append(int(value))
                except ValueError:
                    print(f"Błąd parsowania wiersza: {line}")

    differences = {}
    for _, series in timestamps.items():
        for label, values in series.items():
            if len(values) > 1:
                differences[label] = [values[i + 1] - values[i] for i in range(len(values) - 1)]
            else:
                differences[label] = []

    # dividor = 168
    dividor = 10_000 # windows for ms
    processed_data = {}
    stats_data = {}

    for label, values in differences.items():
        print(label, len(values))
        scaled_values = [v / dividor for v in values]
        scaled_values = scaled_values[2:]  # pomijamy 2 pierwsze
        if not scaled_values:
            continue
        mean = np.mean(scaled_values)
        if "s" in label:
            mean = int(label[:-2])
        else:
            mean = int(label)
        threshold_up = 4 * mean
        threshold_down = 0.1 * mean
        filtered_values = [v for v in scaled_values if threshold_down <= v <= threshold_up]
        processed_data[label] = filtered_values

        # Oblicz statystyki
        if filtered_values:
            min_val = min(filtered_values)
            max_val = max(filtered_values)
            median_val = np.median(filtered_values)
            mean_val = np.mean(filtered_values)
            std_val = np.std(filtered_values, ddof=1) if len(filtered_values) > 1 else 0
            p90 = percentile(filtered_values, 90)
            p95 = percentile(filtered_values, 95)
            p99 = percentile(filtered_values, 99)
            stats_data[label] = {
                "Liczba próbek": len(filtered_values),
                "Min": min_val,
                "Max": max_val,
                "Mediana": median_val,
                "Średnia": mean_val,
                "Odchylenie standardowe": std_val,
                "P90": p90,
                "P95": p95,
                "P99": p99
            }

    if not processed_data:
        print("Brak danych do przetworzenia")
        return

    # ====== Przygotowanie folderu wynikowego ======
    base_name = Path(file_path).stem
    output_folder = "wyniki"
    os.makedirs(output_folder, exist_ok=True)

    # ====== Zapis przetworzonych próbek ======
    for label, values in processed_data.items():
        out_file = os.path.join(output_folder, f"{base_name}_{label}_processed.csv")
        with open(out_file, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["Czas"])
            for v in values:
                writer.writerow([v])
        print(f"Wyniki czasów dla {label} zapisane do: {out_file}")

    # ====== Zapis statystyk ======
    stats_file = os.path.join(output_folder, f"{base_name}_stats.csv")
    with open(stats_file, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["Label", "Statystyka", "Wartość"])
        for label, stats in stats_data.items():
            for stat_name, stat_val in stats.items():
                writer.writerow([label, stat_name, stat_val])
    print(f"Statystyki zapisane do: {stats_file}")

    # ====== Wykresy ======
    if generate_plots:
        nonempty_items = [(label, values) for label, values in processed_data.items()]
        if nonempty_items:
            # Szereg czasowy
            fig, axes = plt.subplots(len(nonempty_items), 1, figsize=(12, 3 * len(nonempty_items)))
            if len(nonempty_items) == 1:
                axes = [axes]
            for ax, (label, values) in zip(axes, nonempty_items):
                ax.plot(values, linestyle='-', label=label)
                ax.set_title(f'Zmiana wartości w czasie dla {label}')
                ax.set_xlabel('Indeks')
                ax.set_ylabel('Wartość (po skalowaniu)')
                ax.grid(True)
            plt.tight_layout()
            plt.savefig(os.path.join(output_folder, f'timeseries_{base_name}.png'))
            plt.close()

            # Histogram
            fig, axes = plt.subplots(len(nonempty_items), 1, figsize=(12, 3 * len(nonempty_items)))
            if len(nonempty_items) == 1:
                axes = [axes]
            for ax, (label, values) in zip(axes, nonempty_items):
                ax.hist(values, bins=100, alpha=0.7, color='blue', edgecolor='black')
                ax.set_title(f'Histogram wartości dla {label}')
                ax.set_xlabel('Wartość (po skalowaniu)')
                ax.set_ylabel('Częstość')
            plt.tight_layout()
            plt.savefig(os.path.join(output_folder, f'histograms_{base_name}.png'))
            plt.close()

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file_path", help="Ścieżka do pliku logu")
    parser.add_argument("--no-hist", action="store_true", help="Nie generuj wykresów i histogramów")
    args = parser.parse_args()
    process_and_plot(args.file_path, generate_plots=not args.no_hist)