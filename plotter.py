import sys
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
import numpy as np

# --- Parse data ------------------------------------------------------------
hits, misses = [], []
for line in sys.stdin:
    parts = line.strip().split(",")
    if len(parts) < 3:
        continue
    try:
        time = int(parts[2])
    except ValueError:
        continue
    if parts[1] == "hit":
        hits.append(time)
    elif parts[1] == "miss":
        misses.append(time)

if not hits or not misses:
    print("No hit/miss data found — check cache_hits output.")
    sys.exit(1)

# --- Compute stats ---------------------------------------------------------
med_hit = np.median(hits)
med_miss = np.median(misses)
threshold = int((med_hit + med_miss) / 2)

print(f"\n[Stats]")
print(f"Median hit latency : {med_hit:.1f} cycles")
print(f"Median miss latency: {med_miss:.1f} cycles")
print(f"Suggested CACHE_THRESHOLD = {threshold} cycles\n")

# --- Plot configuration ----------------------------------------------------
plt.figure(figsize=(7,4))

bins = np.linspace(0, 400, 80)  # same range for both

plt.hist(hits, bins=bins, color="#1f77b4", alpha=0.9, label="hit")
plt.hist(misses, bins=bins, color="#ff7f0e", alpha=0.9, label="miss")

plt.xlabel("Access time (cycles)")
plt.ylabel("Frequency")
plt.title("Cache hit vs miss timing histogram")
plt.legend()
plt.grid(axis='y', linestyle='--', alpha=0.6)
plt.tight_layout()
plt.savefig("hist.png", dpi=120)

print("Histogram saved as hist.png.")
