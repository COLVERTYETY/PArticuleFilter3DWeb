import matplotlib.pyplot as plt
import pandas as pd

# Load the CSV file
file_path = '../runtime.csv'
data = pd.read_csv(file_path)

# Plot the data
plt.figure(figsize=(10, 6))
plt.plot(data['N'], data['Runtime(ms)'], marker='o')
plt.xlabel('N (log scale)')
plt.ylabel('Runtime (ms)')
plt.xscale('log')
plt.title('Evolution of Runtime According to N')
plt.grid(True)

# Save the figure
plt.savefig('runtime_evolution.png')
# plt.show()
