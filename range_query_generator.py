import numpy as np
import sys
import random
# Set the number of samples

# Function to read values from the file
def read_values(filename):
    dict = set(); 
    with open(filename, 'r') as file:
        for line in file: 
            values = set(int(line.strip().split(' ')[1]) for line in file if line.strip())

    return values

# Function to write the final values to a new file
def write_values(filename, values):
    with open(filename, 'w') as file:
        for value in values:
            file.write("g " + str(value) + '\n')

# Main function to process the files
def process_files(input_filename, output_filename):
    # Read the existing values
    existing_values = read_values(input_filename)

    
    # Combine old and new values
    all_values = selected_old_values + new_values
    
    # Write to a new file
    write_values(output_filename, all_values)



# Parameters
input_filename = sys.argv[1]