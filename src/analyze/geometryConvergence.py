import pandas as pd
import matplotlib.pyplot as plt

# Load the CSV file
new_file_path = '../Geometry_convergence.csv'  # Adjust path as needed
new_point_cloud_data = pd.read_csv(new_file_path)

# Filter the data for 3D plot and variance subplots
new_filtered_data = new_point_cloud_data[['id', 'x', 'y', 'z']]
variance_data = new_point_cloud_data[['id', 'varx', 'vary', 'varz', 'distance']]

# Create a figure for the 3D plot and variance subplots
fig = plt.figure(figsize=(15, 10))

# Create the 3D plot
ax_3d = fig.add_subplot(221, projection='3d')
for point_id in new_filtered_data['id'].unique():
    point_data = new_filtered_data[new_filtered_data['id'] == point_id]
    
    # Plot the trajectory
    ax_3d.plot(point_data['x'], point_data['y'], point_data['z'], label=f'ID {point_id}')
    
    # Mark the starting point
    ax_3d.scatter(point_data.iloc[0]['x'], point_data.iloc[0]['y'], point_data.iloc[0]['z'], 
                  color='green', marker='o', s=100, label=f'Start {point_id}')
    
    # Mark the ending point
    ax_3d.scatter(point_data.iloc[-1]['x'], point_data.iloc[-1]['y'], point_data.iloc[-1]['z'], 
                  color='red', marker='*', s=150, label=f'End {point_id}')

ax_3d.set_xlabel('X Coordinate')
ax_3d.set_ylabel('Y Coordinate')
ax_3d.set_zlabel('Z Coordinate')
ax_3d.set_title('Evolution of Points in 3D Space')
ax_3d.legend(loc='upper left', bbox_to_anchor=(1.05, 1), fontsize='small')

# Create subplots for variance evolution
ax_varx = fig.add_subplot(222)
ax_vary = fig.add_subplot(223)
ax_varz = fig.add_subplot(224)

for point_id in variance_data['id'].unique():
    point_data = variance_data[variance_data['id'] == point_id]
    time_steps = range(len(point_data))  # Assuming sequential time steps
    
    ax_varx.plot(point_data['distance'], point_data['varx'], label=f'ID {point_id}')
    ax_varx.set_title('VarX Evolution')
    # ax_varx.set_yscale('log')
    # ax_varx.set_xscale('log')
    #  add start and end points
    ax_varx.scatter(point_data.iloc[-1]['distance'], point_data.iloc[-1]['varx'], color='red', marker='*', s=100, label=f'End {point_id}')
    # ax_varx.scatter(point_data.iloc[-1]['distance'], point_data.iloc[-1]['varx'], color='red', marker='*', s=150, label=f'End {point_id}')
    ax_vary.plot(point_data['distance'], point_data['vary'], label=f'ID {point_id}')
    ax_vary.set_title('VarY Evolution')
    # ax_vary.set_yscale('log')
    # ax_vary.set_xscale('log')
    ax_vary.scatter(point_data.iloc[-1]['distance'], point_data.iloc[-1]['vary'], color='red', marker='*', s=100, label=f'End {point_id}')
    ax_varz.plot(point_data['distance'], point_data['varz'], label=f'ID {point_id}')
    ax_varz.set_title('VarZ Evolution')
    # ax_varz.set_yscale('log')
    # ax_varz.set_xscale('log')
    ax_varz.scatter(point_data.iloc[-1]['distance'], point_data.iloc[-1]['varz'], color='red', marker='*', s=100, label=f'End {point_id}')

ax_varx.set_title('VarX Evolution')
ax_varx.set_xlabel('Distance')
ax_varx.set_ylabel('VarX')
ax_varx.legend(fontsize='small')

ax_vary.set_title('VarY Evolution')
ax_vary.set_xlabel('Distance')
ax_vary.set_ylabel('VarY')
ax_vary.legend(fontsize='small')

ax_varz.set_title('VarZ Evolution')
ax_varz.set_xlabel('Distance')
ax_varz.set_ylabel('VarZ')
ax_varz.legend(fontsize='small')

# Adjust layout and show the plots
plt.tight_layout()
# plt.show()
plt.savefig('geometry_convergence.png')
