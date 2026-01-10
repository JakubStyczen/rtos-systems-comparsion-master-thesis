import sys

if len(sys.argv) != 2:
    print(f"Użycie: {sys.argv[0]} <plik.csv>")
    sys.exit(1)

filename = sys.argv[1]

results = []

with open(filename, "r") as f:
    for line in f:
        line = line.strip()
        if not line:
            continue

        col1, col2 = line.split(";")
        col1 = float(col1)
        col2 = float(col2)

        # divider = 10_000_000
        divider = 1000
        # divider = 168

        result = (col2 - col1) / divider
        results.append(result)

# Wyświetlenie wszystkich wyników
for r in results:
    print(r)

# Średnia
if results:
    average = sum(results) / len(results)
    print("Średnia:", average)
else:
    print("Brak danych do obliczenia średniej")
