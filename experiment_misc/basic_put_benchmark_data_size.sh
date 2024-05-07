#!/bin/bash

# Define the variables
bits_per_entry=0.0001
level_ratios=('2' '6' '10')
buffer_sizes=('2**17' '2**20' '2**23')
modes=0
threads=('8')
leveling_partitions=100

# Base directory for output files
output_dir="experiment_results_clean"
mkdir -p $output_dir
postix="_compare_base_15_25_workload"
# Output file location
output_file="experiment_results_clean/basic_put_benchmark$postix.txt"
# Ensure the output file is empty before starting
> "$output_file"

# Performance data file
iostat_data_file="experiment_results_clean/iostat$postix.txt"
vmstat_data_file="experiment_results_clean/vmstat$postix.txt"

# Ensure performance data files are empty before starting
> "$iostat_data_file"
> "$vmstat_data_file"

for j in {1..3}
do 
    # Generate dataset for the experiment
    for ((i = 0; i <= 10; i++))
    do
        ./workloads/generator --puts $((2**(${i}+15))) --seed $RANDOM > workloads/workload_${i}.txt
    done

    for level in "${level_ratios[@]}"
    do 
        for buffer in "${buffer_sizes[@]}"
        do 
            for thread in "${threads[@]}"
            do 
                for i in {0..10}
                do 
                    echo "$((level)) $((buffer)) $((thread)) workload_$i trial_$j $((leveling_partitions))" >> "$output_file"
                    echo "$((level)) $((buffer)) $((thread)) workload_$i trial_$j $((leveling_partitions))" >> "$iostat_data_file"
                    echo "$((level)) $((buffer)) $((thread)) workload_$i trial_$j $((leveling_partitions))" >> "$vmstat_data_file"

                    # Start monitoring in background
                    iostat -dx 1 >> "$iostat_data_file" &
                    iostat_pid=$!
                    vmstat 1 >> "$vmstat_data_file" &
                    vmstat_pid=$!

                    # Monitor with perf
                    (
                        echo "$bits_per_entry $((level)) $((buffer)) $modes $((thread)) $leveling_partitions"   
                        echo ""                 # Mimic pressing "Enter" to finalize initialization
                        cat workloads/workload_$i.txt  # Send the workload data
                        echo q
                    ) | ./program >> "$output_file"

                    # Stop monitoring
                    kill $iostat_pid
                    kill $vmstat_pid

                    echo "" >> "$output_file"
                    echo "" >> "$iostat_data_file"
                    echo "" >> "$vmstat_data_file"

                    
                    for file in ./*; do
                    # Check if the file has a .dat or .txt extension
                    if [[ "$file" == *.dat ]] || [[ "$file" == *.txt ]]; then
                        # If it does, delete the file
                        rm "$file"
                        echo "Deleted: $file"
                    fi
                    done
                done
            done
        done
    done
done
