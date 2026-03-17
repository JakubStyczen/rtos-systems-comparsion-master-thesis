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
    data = sorted(data)
    k = (len(data) - 1) * (p / 100)
    f = int(k)
    c = min(f + 1, len(data) - 1)
    return data[f] + (data[c] - data[f]) * (k - f)


def process_and_plot(file_path, generate_plots=True):

    timestamps = defaultdict(list)

    # ===== wczytanie danych =====
    with open(file_path, 'r') as file:
        for line in file:
            line = line.strip()
            if not line:
                continue

            try:
                label, value = line.split(';')
                timestamps[label].append(int(value))
            except ValueError:
                print(f"Błąd parsowania wiersza: {line}")

    # ===== różnice =====
    differences = {}

    for label, values in timestamps.items():
        if len(values) > 1:
            differences[label] = [
                values[i + 1] - values[i]
                for i in range(len(values) - 1)
            ]
        else:
            differences[label] = []

    dividor = 168
    processed_data = {}
    stats = {}

    for label, values in differences.items():

        scaled_values = [v / dividor for v in values]
        scaled_values = scaled_values[2:]

        if not scaled_values:
            continue

        mean = np.mean(scaled_values)

        threshold_up = 10 * int(label[:-2])
        threshold_down = 0.1 * int(label[:-2])

        filtered_values = [
            v for v in scaled_values
            if threshold_down <= v <= threshold_up
        ]

        processed_data[label] = filtered_values

        if filtered_values:

            stats[label] = {
                "count": len(filtered_values),
                "min": min(filtered_values),
                "max": max(filtered_values),
                "median": np.median(filtered_values),
                "mean": np.mean(filtered_values),
                "std": np.std(filtered_values, ddof=1) if len(filtered_values) > 1 else 0,
                "p90": percentile(filtered_values, 90),
                "p95": percentile(filtered_values, 95),
                "p99": percentile(filtered_values, 99),
            }

    if not processed_data:
        print("Brak danych po filtracji")
        return

    base_name = Path(file_path).stem
    out_dir = Path("wyniki")
    out_dir.mkdir(exist_ok=True)

    # ===== zapis próbek =====

    for label, values in processed_data.items():

        out_file = out_dir / f"{base_name}_{label}_samples.csv"

        with open(out_file, "w", newline="") as f:
            writer = csv.writer(f)
            writer.writerow(["value"])
            for v in values:
                writer.writerow([v])

        print(f"Zapis próbek: {out_file}")

    # ===== zapis statystyk =====

    stats_file = out_dir / f"{base_name}_stats.csv"

    with open(stats_file, "w", newline="") as f:

        writer = csv.writer(f)

        writer.writerow([
            "label",
            "count",
            "min",
            "max",
            "median",
            "mean",
            "std",
            "p90",
            "p95",
            "p99"
        ])

        for label, s in stats.items():
            writer.writerow([
                label,
                s["count"],
                s["min"],
                s["max"],
                s["median"],
                s["mean"],
                s["std"],
                s["p90"],
                s["p95"],
                s["p99"],
            ])

    print(f"Zapis statystyk: {stats_file}")

    if not generate_plots:
        return

    items = list(processed_data.items())

    # ===== wykres czasowy =====

    fig, axes = plt.subplots(len(items), 1, figsize=(12, 3 * len(items)))

    if len(items) == 1:
        axes = [axes]

    for ax, (label, values) in zip(axes, items):

        ax.plot(values)
        ax.set_title(label)
        ax.set_xlabel("index")
        ax.set_ylabel("time")
        ax.grid(True)

    plt.tight_layout()
    plot_file = out_dir / f"timeseries_{base_name}.png"
    plt.savefig(plot_file)
    plt.close()

    print(f"Zapis wykresu: {plot_file}")

    # ===== histogram =====

    fig, axes = plt.subplots(len(items), 1, figsize=(12, 3 * len(items)))

    if len(items) == 1:
        axes = [axes]

    for ax, (label, values) in zip(axes, items):

        ax.hist(values, bins=100)
        ax.set_title(label)
        ax.set_xlabel("value")
        ax.set_ylabel("count")

    plt.tight_layout()
    hist_file = out_dir / f"histograms_{base_name}.png"
    plt.savefig(hist_file)
    plt.close()

    print(f"Zapis histogramów: {hist_file}")


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("file_path")
    parser.add_argument("--no-plots", action="store_true")

    args = parser.parse_args()

    process_and_plot(args.file_path, generate_plots=not args.no_plots)