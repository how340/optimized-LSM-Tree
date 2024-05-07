import random
import sys

# Function to read values from the file
def read_values(filename):
    dict = set(); 
    with open(filename, 'r') as file:
        for line in file: 
            values = set(int(line.strip().split(' ')[1]) for line in file if line.strip())

    return values

# Function to generate new unique values that are not in the existing values
def generate_new_values(existing_values, num_new_values):
    new_values = []
    while len(new_values) < num_new_values:
        new_value = random.randint(-2147483648, 2147483648)  # Change this based on how you want to generate values
        if new_value not in existing_values:
            new_values.append(new_value)
    return new_values

# Function to write the final values to a new file
def write_values(filename, values):
    with open(filename, 'w') as file:
        for value in values:
            file.write("g " + str(value) + '\n')

# Main function to process the files
def process_files(input_filename, output_filename, num_old_values, num_new_values):
    # Read the existing values
    existing_values = read_values(input_filename)



    # Select a subset of existing values randomly
    selected_old_values = random.sample(sorted(existing_values), num_old_values)
    
    # Generate new values
    new_values = generate_new_values(existing_values, num_new_values)
    
    # Combine old and new values
    all_values = selected_old_values + new_values
    
    # Write to a new file
    write_values(output_filename, all_values)



# Parameters
input_filename = sys.argv[1]

num_old_values = 900
num_new_values = 100


output_filename = 'get_1.txt'
output_filename1 = 'get_2.txt'
output_filename2 = 'get_3.txt'
    
# Execute the process
process_files(input_filename,output_filename, num_old_values, num_new_values)
process_files(input_filename,output_filename1, num_old_values, num_new_values)
process_files(input_filename,output_filename2, num_old_values, num_new_values)
