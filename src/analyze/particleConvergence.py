import matplotlib.pyplot as plt
import pandas as pd

# Load the uploaded CSV file to analyze the data
file_path = '../Particule_convergence.csv'
data = pd.read_csv(file_path)


# Group the data by N and plot the distance over time for each N
unique_N = data['N'].unique()
plt.figure(figsize=(10, 6))
# Determine the best, average, and worst N values according to final distance
best_N = data.groupby('N')['distance'].last().idxmin()
worst_N = data.groupby('N')['distance'].last().idxmax()

# Calculate the average, Q1, and Q3 distances
average_distance = data.groupby('N')['distance'].last().mean()
Q1_distance = data.groupby('N')['distance'].last().quantile(0.25)
Q3_distance = data.groupby('N')['distance'].last().quantile(0.75)

# Find the closest N values to the average, Q1, and Q3 distances
average_N = (data.groupby('N')['distance'].last() - average_distance).abs().idxmin()
Q1_N = (data.groupby('N')['distance'].last() - Q1_distance).abs().idxmin()
Q3_N = (data.groupby('N')['distance'].last() - Q3_distance).abs().idxmin()



# Plot for each unique value of N
for n in unique_N:
    subset = data[data['N'] == n]
    plt.plot(range(len(subset)), subset['distance'], label=f'N={n}')
    if n == best_N:
        plt.text(len(subset) - 1, subset['distance'].iloc[-1], f'N={n} (best)', fontsize=8)
    elif n == average_N:
        plt.text(len(subset) - 1, subset['distance'].iloc[-1], f'N={n} (average)', fontsize=8)
    elif n == worst_N:
        plt.text(len(subset) - 1, subset['distance'].iloc[-1], f'N={n} (worst)', fontsize=8)
    elif n == Q1_N:
        plt.text(len(subset) - 1, subset['distance'].iloc[-1], f'N={n} (Q1)', fontsize=8)
    elif n == Q3_N:
        plt.text(len(subset) - 1, subset['distance'].iloc[-1], f'N={n} (Q3)', fontsize=8)

# Customize the plot
plt.title('Evolution of Distance Across Time Steps for Different N Values')
plt.xlabel('Time Steps')
plt.ylabel('Distance (log scale)')
plt.yscale('log')
# plt.legend()
plt.grid()
plt.tight_layout()

# Show the plot
# plt.show()
plt.savefig('distance_evolution.png')
