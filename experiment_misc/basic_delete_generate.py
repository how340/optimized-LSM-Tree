import numpy as np
import sys
import random
# Set the number of samples

# Function to write the final values to a new file
def write_values(filename, values):
    with open(filename, 'w') as file:
        for value in values:
            file.write("d " + str(value) + '\n')

# Main function to process the files
def process_files(output_filename):
    # Read the existing values
    all_values = generate_uni_range(10000)
    # Write to a new file
    write_values(output_filename, all_values)

def generate_uni_range(cnt):
    uniform_data = np.random.randint(-2147483647, 2147483647, size=cnt, dtype=np.int32)
    
    return uniform_data 


output_filename = 'delete_1.txt'
output_filename1 = 'delete_2.txt'
output_filename2 = 'delete_3.txt'

process_files(output_filename)
process_files(output_filename1)
process_files(output_filename2)