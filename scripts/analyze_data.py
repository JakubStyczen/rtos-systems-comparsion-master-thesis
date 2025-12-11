import argparse
import matplotlib as mpl
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict
from pathlib import Path

mpl.rcParams['axes.formatter.useoffset'] = False
mpl.rcParams['axes.formatter.use_mathtext'] = False


def process_and_plot(file_path):
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
            if line:
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

    processed_data = {}
    for label, values in differences.items():
        scaled_values = [v / 168 for v in values]
            
        scaled_values = scaled_values[2:]
        mean = np.mean(scaled_values)
        threshold_up = 1.5 * mean
        threshold_down = 0.1*mean
        filtered_values = [v for v in scaled_values if threshold_down <= v <= threshold_up]
        filtered_values = scaled_values
        processed_data[label] = filtered_values
        print(mean)

    nonempty_items = [(label, values) for label, values in processed_data.items() if values]
    if not nonempty_items:
        return

    log_name = Path(file_path).stem

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
    plt.savefig(f'timeseries_{log_name}.png')
    plt.close()

    fig, axes = plt.subplots(len(nonempty_items), 1, figsize=(12, 3 * len(nonempty_items)))
    if len(nonempty_items) == 1:
        axes = [axes]
    for ax, (label, values) in zip(axes, nonempty_items):
        ax.hist(values, bins=100, alpha=0.7, color='blue', edgecolor='black')
        ax.set_title(f'Histogram wartości dla {label}')
        ax.set_xlabel('Wartość (po skalowaniu)')
        ax.set_ylabel('Częstość')
    plt.tight_layout()
    plt.savefig(f'histograms_{log_name}.png')
    plt.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("file_path", help="Ścieżka do pliku logu")
    args = parser.parse_args()
    process_and_plot(args.file_path)