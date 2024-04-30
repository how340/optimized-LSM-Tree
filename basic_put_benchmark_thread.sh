#!/bin/bash

# Define the variables
bits_per_entry=0.0001
level_ratios=('2' '10')
buffer_sizes=('2**19')
modes=0
threads=('1' '4' '8')
leveling_partitions=0

# Base directory for output files
output_dir="experiment_results"
mkdir -p "$output_dir"

# Output file location
output_file="$output_dir/basic_put_benchmark_thread.txt"
# Ensure the output file is empty before starting
> "$output_file"

# Performance data files
iostat_data_file="$output_dir/iostat_data_thread.txt"
vmstat_data_file="$output_dir/vmstat_data_thread.txt"

# Ensure performance data files are empty before starting
> "$iostat_data_file"
> "$vmstat_data_file"

for j in {1..3}
do 

    ./workloads/generator --puts $((2**24)) --seed $RANDOM > workloads/workload_thread.txt


    for level in "${level_ratios[@]}"
    do 
        for buffer in "${buffer_sizes[@]}"
        do 
            for thread in "${threads[@]}"
            do 
    
                echo "$((level)) $((buffer)) $((thread)) trial_$j" >> "$output_file"
                echo "$((level)) $((buffer)) $((thread))  trial_$j" >> "$iostat_data_file"
                echo "$((level)) $((buffer)) $((thread))  trial_$j" >> "$vmstat_data_file"

                # Start monitoring in background
                iostat -dx 2 >> "$iostat_data_file" &
                iostat_pid=$!
                vmstat 2 >> "$vmstat_data_file" &
                vmstat_pid=$!

                # (
                #     echo "$bits_per_entry $((level)) $((buffer)) $modes $((thread)) $leveling_partitions"   
                #     # echo "0.0001 10 1000 0 1 0"  # Send initialization parameters
                #     echo ""                 # Mimic pressing "Enter" to finalize initialization
                #     cat /workloads/workload_thread.txt # Send the workload data
                #     echo q
                # ) | ./program >> "$output_file"

                # echo "" >> "$output_file"
                
                # Monitor with perf
                (
                    echo "$bits_per_entry $((level)) $((buffer)) $modes $((thread)) $leveling_partitions"   
                    echo ""                 # Mimic pressing "Enter" to finalize initialization
                    cat workloads/workload_thread.txt  # Send the workload data
                    echo q
                ) | ./program >> "$output_file"

                # Stop monitoring
                kill $iostat_pid
                kill $vmstat_pid

                echo "" >> "$output_file"

                for file in ./*; do
                # Check if the file has a .dat or .txt extension
                if [[ "$file" == *.dat ]] || [[ "$file" == *.txt ]]; then
                    # If it does, delete the file
                    rm "$file"
                    echo "Deleted: $file"
                fi
                done
                echo "" >> "$output_file"
            done
        done
    done
done
