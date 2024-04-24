#!/bin/bash

# Define the variables
bits_per_entry=(5 10 20)
level_ratios=(2 3 5 8 10)
buffer_sizes=(32768 131072 2097152 16777216)
modes=(0)
threads=(1 2 4 8)
leveling_partitions=(0)

# Base directory for output files
output_dir="./experiment_results"
mkdir -p $output_dir

# Output file location
output_file="./experiment_results/basic_get_benchmark.txt"
# Ensure the output file is empty before starting
> "$output_file"

# # Generate dataset for the experiement
# for ((i=15; i<=30; i++))
# do
#     ./workloads/generator --puts $((2**i)) --seed $RANDOM >> ./workloads/workload_$((i-15)).txt
# done


# Loop through each variable
for bits in "${bits_per_entry[@]}"; do 
    for level_ratio in "${level_ratios[@]}"; do
        for buffer_size in "${buffer_sizes[@]}"; do
            for mode in "${modes[@]}"; do
                for thread in "${threads[@]}"; do
                    for leveling_partition in "${leveling_partitions[@]}"; do

                        echo "Experiment: Level Ratio=$level_ratio, Buffer Size=$buffer_size, Mode=$mode, Threads=$thread, Leveling Partition=$leveling_partition" >> "$output_file"

                        # Repeat each experiment 5 times
                        for repeat in {1..5}; do
                            echo "Repeat $repeat" >> "$output_file"
                            
                            # Loop through workload files from workload_0.txt to workload_10.txt
                            for i in {0..15}; do
        
                                echo "Running workload_${i}.txt" >> "$output_file"

                                # Header to separate the results
                                echo "Start of result -> Bits_per_entry: $bits Level Ratio: $level_ratio, Buffer Size: $buffer_size, Mode: $mode, Threads: $thread, Leveling Partition: $leveling_partition, Workload: workload_${i}.txt, Repeat: $repeat" >> "$output_file"

                                # Send the initial values and the contents of the workload file to the program
                                # Then, send 'q' to indicate the end of input
                                (
                                    #std::cin >> bits_per_entry >> level_ratio >> buffer_size >> mode >> threads >> leveling_partition;
                                    echo "$bits $level_ratio $buffer_size $mode $thread $leveling_partition"  # Send initialization parameters
                                    echo ""                 # Mimic pressing "Enter" to finalize initialization
                                    cat workloads/workload_${i}.txt  # Send the workload data
                                    echo q
                                ) | ./program >> "$output_file"

                                echo "" >> "$output_file"
                            done
                        done

                        echo "-----------" >> "$output_file"
                    done
                done
            done
        done
    done
done
echo "All experiments completed."