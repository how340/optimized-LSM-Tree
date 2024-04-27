import random

# Function to read values from the file
def read_values(filename):
    with open(filename, 'r') as file:
        values = set(line.strip().split(' ')[2] for line in file if line.strip())
    return values

# Function to generate new unique values that are not in the existing values
def generate_new_values(existing_values, num_new_values):
    new_values = set()
    while len(new_values) < num_new_values:
        new_value = f"new_value_{random.randint(1, 100000)}"  # Change this based on how you want to generate values
        if new_value not in existing_values:
            new_values.add(new_value)
    return new_values

# Function to write the final values to a new file
def write_values(filename, values):
    with open(filename, 'w') as file:
        for value in values:
            file.write(value + '\n')

# Main function to process the files
def process_files(input_filename, output_filename, num_old_values, num_new_values):
    # Read the existing values
    existing_values = read_values(input_filename)
    
    # Select a subset of existing values randomly
    selected_old_values = set(random.sample(existing_values, num_old_values))
    
    # Generate new values
    new_values = generate_new_values(existing_values, num_new_values)
    
    # Combine old and new values
    all_values = selected_old_values.union(new_values)
    
    # Write to a new file
    write_values(output_filename, all_values)

# Parameters
input_filename = 'input.txt'
output_filename = 'output.txt'
num_old_values = 7000
num_new_values = 3000

# Execute the process
process_files(input_filename, output_filename, num_old_values, num_new_values)
