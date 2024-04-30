import numpy as np
import sys
# Set the number of samples

def generate_set(num):
    num_samples = 2**((i*2)+15)
    # Generate uniformly distributed integers
    uniform_data = np.random.randint(-2147483648, 2147483647, size=num_samples, dtype=np.int32)

    # Generate Gaussian-distributed numbers (mean=0, std=1)
    # Note: The scale (std dev) can be adjusted if a wider range is desired
    gaussian_data = np.random.normal(loc=0, scale=800000000, size=num_samples)
   
    # Convert the Gaussian data to 32-bit integers
    gaussian_data = np.clip(gaussian_data, -2147483648, 2147483647)
    gaussian_data = gaussian_data.astype(np.int32)

    write_values(f"workloads/uni_workload_{num}.txt", uniform_data)
    write_values(f"workloads/gau_workload_{num}.txt", gaussian_data)

# Output the results
def write_values(filename, values):
    with open(filename, 'w') as file:
        for value in values:
            file.write("g " + str(value) + '\n')

for i in range(0,5):
    generate_set(i)
