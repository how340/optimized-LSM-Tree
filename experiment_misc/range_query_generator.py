import numpy as np
import sys
import random
# Set the number of samples

# Function to write the final values to a new file
def write_values(filename, values):
    with open(filename, 'w') as file:
        for value in values:
            file.write("r " + str(value[0]) + " " + str(value[1]) +'\n')

# Main function to process the files
def process_files(output_filename):
    # Read the existing values
    all_values = generate_uni_range(500)
    # Write to a new file
    write_values(output_filename, all_values)

def generate_uni_range(cnt):
    ret = [] 
    r = 100000000
    
    uniform_data = np.random.randint(-2147483647, 2147483647, size=cnt, dtype=np.int32)
    
    for i in uniform_data: 
        if i - r < -2147483647: 
            left = -2147483647
        else: 
            left = i - r
        
        if i + r > 2147483647:
            right = 2147483647
        else: 
            right = i + r
        
        ret.append((left, right))
    return ret 


output_filename = 'range_get_1.txt'
output_filename1 = 'range_get_2.txt'
output_filename2 = 'range_get_3.txt'

process_files(output_filename)
process_files(output_filename1)
process_files(output_filename2)