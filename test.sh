#!/bin/bash

# # Generate dataset for the experiement - I am reusing the same dataset here to reduce experiment time
# for ((i = 0; i <= 5; i++))
# do
#     ./workloads/generator --puts $((2**((i*2)+15))) --seed $RANDOM > ./workloads/workload_${i}.txt
# done

# load dataset into tree. 
# (
#     echo "0.0001 6 8388608 0 8 0 "  # Send initialization parameters
#     echo ""                 # Mimic pressing "Enter" to finalize initialization
#     cat workloads/workload_4.txt  # Send the workload data
#     echo "q" 
# ) | ./program 


echo "0.0001 6 8388608 0 8 0 "   >> temp
                    
# Start monitoring in background
iostat -dx 2 > iostat_data_file &
iostat_pid=$!
vmstat 1 > vmstat_data_file &
vmstat_pid=$!
cachegrind_file="cachegrind_output.txt"


 # Run the program with Valgrind Cachegrind
valgrind --tool=cachegrind --log-file="$cachegrind_file" --cachegrind-out-file="cachegrind.out" ./program "0.0001 6 8388608 0 8 0" < workloads/workload_2.txt

# Append Cachegrind output to a summary file
echo "Cachegrind output for workload_$i trial_$j" >> "$cachegrind_file"
cat "cachegrind.out" >> "$cachegrind_file"
echo "" >> "$cachegrind_file"


# Stop monitoring
kill $iostat_pid
kill $vmstat_pid

echo "" >> temp


# for file in ./*; do
#   # Check if the file has a .dat or .txt extension
#   if [[ "$file" == *.dat ]] || [[ "$file" == *.txt ]]; then
#     # If it does, delete the file
#     rm "$file"
#     echo "Deleted: $file"
#   fi
# done