#!/bin/bash
# Define the variables
bits_per_entry=0.0001
level_ratios=('2' '6' '10')
buffer_sizes=('2**17' '2**20' '2**23')
modes=0
threads=('1' '4' '8')
leveling_partitions=0

# Base directory for output files
output_dir="./experiment_results"
mkdir -p $output_dir

# Output file location
output_file="./experiment_results/basic_delete_benchmark.txt"
# Ensure the output file is empty before starting
#> "$output_file"
# Performance data file
iostat_data_file="experiment_results/iostat_delete.txt"
vmstat_data_file="experiment_results/vmstat_delete.txt"

# Ensure performance data files are empty before starting
> "$iostat_data_file"
> "$vmstat_data_file"


# Generate dataset for the experiement - I am reusing the same dataset here to reduce experiment time
for ((i = 0; i <= 5; i++))
do
    ./workloads/generator --puts $((2**((i*2)+15))) --seed $RANDOM > ./workloads/workload_${i}.txt
done

for level in "${level_ratios[@]}"
do 
    for buffer in "${buffer_sizes[@]}"
    do 
        for thread in "${threads[@]}"
        do 
            for i in {0..5}
            do 
                echo "$((level)) $((buffer)) $((thread)) workload_$i" >> "$output_file"
                echo "$((level)) $((buffer)) $((thread)) workload_$i trial_$j" >> "$iostat_data_file"
                echo "$((level)) $((buffer)) $((thread)) workload_$i trial_$j" >> "$vmstat_data_file"

                (
                    echo "$bits_per_entry $((level)) $((buffer)) $modes $((thread)) $leveling_partitions"   
                    echo ""                 # Mimic pressing "Enter" to finalize initialization
                    cat ./workloads/workload_$i.txt  # Send the workload data
                    echo "q" 
                ) | ./program 
                
                python3 basic_delete_generate.py
                
                # Start monitoring in background
                iostat -dx 1 > "$iostat_data_file" &
                iostat_pid=$!
                vmstat 1 > "$vmstat_data_file" &
                vmstat_pid=$!

                for rep in {1..3}
                do 
                    time (
                        cat delete_$rep.txt
                        echo "q"
                    ) | ./program >> "$output_file"
                done 

                for file in ./*; do
                # Check if the file has a .dat or .txt extension
                if [[ "$file" == *.dat ]] || [[ "$file" == *.txt ]]; then
                    # If it does, delete the file
                    rm "$file"
                    echo "Deleted: $file"
                fi
                done
                
                echo "" >> "$output_file"
                echo "" >> "$iostat_data_file"
                echo "" >> "$vmstat_data_file"
            done
            
        done
    done
done
