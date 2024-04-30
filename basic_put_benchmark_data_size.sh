#!/bin/bash

# Define the variables
bits_per_entry=0.0001
level_ratios=('2' '6' '10')
buffer_sizes=('2**17' '2**20' '2**23')
modes=0
threads=('8')
leveling_partitions=0

# Base directory for output files
output_dir="/experiment_results"
mkdir -p $output_dir

# Output file location
output_file="/experiment_results/basic_put_benchmark_short.txt"
# Ensure the output file is empty before starting
> "$output_file"

# Performance data file
perf_data_file="/experiment_results/perf_data.txt"
iostat_data_file="/experiment_results/iostat_data.txt"
vmstat_data_file="/experiment_results/vmstat_data.txt"

# Ensure performance data files are empty before starting
> "$perf_data_file"
> "$iostat_data_file"
> "$vmstat_data_file"

for j in {1..3}
do 
    # Generate dataset for the experiment
    for ((i = 0; i <= 15; i++))
    do
        ./workloads/generator --puts $((2**(i+15))) --seed $RANDOM > /workloads/workload_${i}.txt
    done

    for level in "${level_ratios[@]}"
    do 
        for buffer in "${buffer_sizes[@]}"
        do 
            for thread in "${threads[@]}"
            do 
                for i in {0..5}
                do 
                    echo "$((level)) $((buffer)) $((thread)) workload_$i trial_$j" >> "$output_file"
                    echo "$((level)) $((buffer)) $((thread)) workload_$i trial_$j" >> "$perf_data_file"
                    echo "$((level)) $((buffer)) $((thread)) workload_$i trial_$j" >> "$iostat_data_file"
                    echo "$((level)) $((buffer)) $((thread)) workload_$i trial_$j" >> "$vmstat_data_file"

                    # Start monitoring in background
                    iostat -dx 2 > "$iostat_data_file" &
                    iostat_pid=$!
                    vmstat 1 > "$vmstat_data_file" &
                    vmstat_pid=$!

                    # Monitor with perf
                    (
                        echo "$bits_per_entry $((level)) $((buffer)) $modes $((thread)) $leveling_partitions"   
                        echo ""                 # Mimic pressing "Enter" to finalize initialization
                        cat ../workloads/workload_$i.txt  # Send the workload data
                        echo q
                    ) | /mnt/c/Users/Bruce/Desktop/git/WSL2-Linux-Kernel/tools/perf/perf stat -e cache-misses,cache-references -x, -o "$perf_data_file" ./program >> "$output_file"

                    # Stop monitoring
                    kill $iostat_pid
                    kill $vmstat_pid

                    echo "" >> "$output_file"
                    echo "" >> "$perf_data_file"
                    echo "" >> "$iostat_data_file"
                    echo "" >> "$vmstat_data_file"

                done
            done
        done
    done
done
