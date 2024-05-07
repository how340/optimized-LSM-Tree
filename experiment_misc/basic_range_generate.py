import numpy as np
import sys
# Set the number of samples

def generate_set(num):
    num_samples = 2**(num+15)
    # Generate uniformly distributed integers
    uniform_key = np.random.randint(-2147483647, 2147483647, size=num_samples, dtype=np.int32)
    uniform_data = np.random.randint(-2147483647, 2147483647, size=num_samples, dtype=np.int32)

    # non-uniform key generation
    gaussian_key = np.random.normal(loc=0, scale=800000000, size=num_samples)

    gaussian_key = np.clip(gaussian_key, -2147483647, 2147483647)
    gaussian_key = gaussian_key.astype(np.int32)

    write_values(f"workloads/uni_workload_{num}.txt", zip(uniform_key, uniform_data))
    write_values(f"workloads/gau_workload_{num}.txt", zip(gaussian_key, uniform_data))

# Output the results
def write_values(filename, values):
    with open(filename, 'w') as file:
        for key, val in values:
            file.write("p " + str(key) + " " + str(val) + '\n')

for i in range(0,11):
    generate_set(i)

