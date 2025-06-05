#!/usr/bin/python3

#Usage: ./plot_cumul_qtimes.py file1 file2 file3 ...

import matplotlib.pyplot as plt
import numpy as np
import sys
import cycler

custom_colors = ['#377eb8', '#ff7f00', '#e41a1c', '#f781bf', '#a65628', 
                 '#4daf4a', '#984ea3', '#999999', '#dede00', '#377eb8']
plt.rcParams["axes.prop_cycle"] = cycler.cycler(color=custom_colors)

def read_qtimes(filename):
    times = []
    with open(filename, 'r') as file:
        for line in file:
            parts = line.split()
            if len(parts) == 2:  # Ensure there is a time value
                try:
                    times.append(float(parts[1]))
                except ValueError:
                    continue  # Skip lines with non-numeric values
            elif len(parts)==3:
                try:
                    times.append(float(parts[2]))
                except ValueError:
                    continue  # Skip lines with non-numeric values
    return times


plt.figure(figsize=(8, 8))

for filename in sys.argv[1:]:
    times = read_qtimes(filename)
    times.sort()
    y_values = np.arange(1, len(times) + 1)
    plt.plot(times, y_values, linestyle='-', label=filename)

# Configure plot settings
plt.xlabel('Time (seconds)')
plt.ylabel('Solved Instances')
plt.grid(True, linestyle='--', alpha=0.6)
plt.legend()
plt.show()
