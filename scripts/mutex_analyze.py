import sys
import os
import statistics
import csv
import matplotlib.pyplot as plt
import numpy as np

# ======== Weryfikacja argumentów ========
if len(sys.argv) != 2:
    print(f"Użycie: {sys.argv[0]} <plik.csv>")
    sys.exit(1)

filename = sys.argv[1]
results = []

# divider = 168  # normalizacja czasu, możesz zmienić

# divider = 10_000_000
divider = 1_000_000 # linux divider for ms?
# divider = 10_000 # us for windows

# ======== Wczytanie i przetworzenie danych ========
with open(filename, "r") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue
        try:
            col1, col2 = line.split(";")
            col1 = float(col1)
            col2 = float(col2)
            result = (col2 - col1) / divider
            results.append(result)
        except ValueError:
            print(f"Pominięto linię: {line}")

if not results:
    print("Brak danych do analizy")
    sys.exit(1)

# ======== Funkcja do percentyli ========
def percentile(data, p):
    data_sorted = sorted(data)
    k = (len(data_sorted) - 1) * (p / 100)
    f = int(k)
    c = min(f + 1, len(data_sorted) - 1)
    return data_sorted[f] + (data_sorted[c] - data_sorted[f]) * (k - f)

# ======== Obliczenie statystyk ========
min_val = min(results)
max_val = max(results)
median_val = statistics.median(results)
mean_val = statistics.mean(results)
std_val = statistics.stdev(results) if len(results) > 1 else 0
p90 = percentile(results, 90)
p95 = percentile(results, 95)
p99 = percentile(results, 99)

# ======== Wyświetlenie statystyk ========
print(f"Liczba próbek: {len(results)}")
print(f"Min: {min_val:.6f}")
print(f"Max: {max_val:.6f}")
print(f"Mediana: {median_val:.6f}")
print(f"Średnia: {mean_val:.6f}")
print(f"Odchylenie standardowe: {std_val:.6f}")
print(f"P90: {p90:.6f}")
print(f"P95: {p95:.6f}")
print(f"P99: {p99:.6f}")

# ======== Przygotowanie folderu wynikowego ========
base_name = os.path.splitext(os.path.basename(filename))[0]
output_folder = "wyniki"
os.makedirs(output_folder, exist_ok=True)

# ======== Zapis przetworzonych czasów ========
processed_file = os.path.join(output_folder, f"{base_name}_processed.csv")
with open(processed_file, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["Czas"])
    for r in results:
        writer.writerow([r])
print(f"Wyniki czasów zapisane do: {processed_file}")

# ======== Zapis statystyk ========
stats_file = os.path.join(output_folder, f"{base_name}_stats.csv")
with open(stats_file, "w", newline="") as f:
    writer = csv.writer(f)
    writer.writerow(["Statystyka", "Wartość"])
    writer.writerow(["Liczba próbek", len(results)])
    writer.writerow(["Min", min_val])
    writer.writerow(["Max", max_val])
    writer.writerow(["Mediana", median_val])
    writer.writerow(["Średnia", mean_val])
    writer.writerow(["Odchylenie standardowe", std_val])
    writer.writerow(["P90", p90])
    writer.writerow(["P95", p95])
    writer.writerow(["P99", p99])
print(f"Statystyki zapisane do: {stats_file}")

# ======== Generowanie wykresów ========
fig, axes = plt.subplots(2, 2, figsize=(14, 10))
fig.suptitle(f'Analiza Mutex - {base_name}', fontsize=16, fontweight='bold')

# Wykres 1: Histogram
ax = axes[0, 0]
ax.hist(results, bins=50, color='steelblue', alpha=0.7, edgecolor='black')
ax.axvline(mean_val, color='red', linestyle='--', linewidth=2, label=f'Średnia: {mean_val:.6f}')
ax.axvline(median_val, color='green', linestyle='--', linewidth=2, label=f'Mediana: {median_val:.6f}')
ax.set_xlabel('Czas (ms)')
ax.set_ylabel('Liczba zdarzeń')
ax.set_title('Histogram rozkładu czasów')
ax.legend()
ax.grid(alpha=0.3)

# Wykres 2: Wykres liniowy (chronologiczny)
ax = axes[0, 1]
ax.plot(results, linewidth=0.5, color='steelblue', alpha=0.7)
ax.axhline(mean_val, color='red', linestyle='--', linewidth=1, label=f'Średnia: {mean_val:.6f}')
ax.axhline(median_val, color='green', linestyle='--', linewidth=1, label=f'Mediana: {median_val:.6f}')
ax.set_xlabel('Numer próbki')
ax.set_ylabel('Czas (ms)')
ax.set_title('Czasowy przebieg wartości')
ax.legend()
ax.grid(alpha=0.3)


plt.tight_layout()

# Zapis wykresu
plot_file = os.path.join(output_folder, f"{base_name}_plot.png")
plt.savefig(plot_file, dpi=300, bbox_inches='tight')
print(f"Wykres zapisany do: {plot_file}")
plt.show()