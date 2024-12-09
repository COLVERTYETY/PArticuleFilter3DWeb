import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# Load the uploaded CSV file to analyze the data
file_path = '../Particule_convergence.csv'
data = pd.read_csv(file_path)

# Define binning parameters
bin_size = 250  # Group every 250 N values
data['N_binned'] = (data['N'] // bin_size) * bin_size  # Bin N into ranges
grouped_binned = data.groupby('N_binned')['distance']

# Recalculate statistics for binned data
summary_stats_binned = grouped_binned.agg(['last', 'mean', 'min', 'max', 'std', 'median'])
summary_stats_binned['iqr'] = grouped_binned.quantile(0.75) - grouped_binned.quantile(0.25)

# Smooth out the 'last' values to be resilient to noise using a rolling mean
window_size = 3  # Define the window size for smoothing
summary_stats_binned['last_smoothed'] = summary_stats_binned['last'].rolling(window=window_size, center=True).mean()

# Remove NaN values for polynomial fitting
valid_data = summary_stats_binned.dropna(subset=['last_smoothed'])

# Fit a 2nd degree polynomial to the smoothed final distances
z = np.polyfit(valid_data.index, valid_data['last_smoothed'], 2)
p = np.poly1d(z)

# Compute the derivative of the polynomial (gradient)
dp = np.polyder(p)

# Find the N of least returns by solving for the minimum gradient
x_vals = np.linspace(valid_data.index.min(), valid_data.index.max(), 1000)
gradient_vals = dp(x_vals)
least_returns_N = x_vals[np.argmin(np.abs(gradient_vals))]

# Prepare the figure
fig, axes = plt.subplots(2, 1, figsize=(14, 12), gridspec_kw={'height_ratios': [3, 1]})

# Top plot: Evolution of distance across time for all N values
unique_N = data['N'].unique()
for n in unique_N:
    subset = data[data['N'] == n]
    axes[0].plot(
        range(len(subset)),
        subset['distance'],
        alpha=0.7,
        linewidth=1,
    )

# Customize top plot
axes[0].set_title('Evolution of Distance Across Time Steps for Different N Values')
axes[0].set_xlabel('Time Steps')
axes[0].set_ylabel('Distance (log scale)')
axes[0].set_yscale('log')
axes[0].grid()

# Bottom plot: Histogram of final distances by binned N with added statistics
bars = axes[1].bar(
    summary_stats_binned.index,
    summary_stats_binned['last'],
    alpha=0.7,
    color='skyblue',
    edgecolor='black',
    label='Final Distance',
    width=bin_size,
)

# Add error bars for variability (standard deviation)
axes[1].errorbar(
    summary_stats_binned.index,
    summary_stats_binned['last'],
    yerr=summary_stats_binned['std'],
    fmt='o',
    color='black',
    label='Std Dev',
    capsize=5,
)

# Add trend line for binned final distances
axes[1].plot(valid_data.index, p(valid_data.index), color='orange', label='Trend Line p2')

# Highlight the N of least returns
axes[1].axvline(
    x=least_returns_N,
    color='red',
    linestyle='--',
    label=f'N of Least Returns = {int(least_returns_N)}',
)

# Customize bottom plot
axes[1].set_title('Histogram of Final Distances by Binned N with Statistical Overlays')
axes[1].set_xlabel(f'N (Number of Particles, Binned by {bin_size})')
axes[1].set_ylabel('Final Distance')
axes[1].grid(axis='y', linestyle='--', alpha=0.6)
axes[1].set_ylim(0, 1)
axes[1].legend()

# Adjust layout and save the figure
plt.tight_layout()
plt.savefig('distance_evolution.png')

# Output the result
print(f"N of Least Returns (from polynomial fit): {int(least_returns_N)}")
