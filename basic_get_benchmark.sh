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
output_file="./experiment_results/basic_get_benchmark_short_2.txt"
# Ensure the output file is empty before starting
#> "$output_file"


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
                (
                    echo "$bits_per_entry $((level)) $((buffer)) $modes $((thread)) $leveling_partitions"   
                    echo ""                 # Mimic pressing "Enter" to finalize initialization
                    cat ./workloads/workload_$i.txt  # Send the workload data
                    echo "q" 
                ) | ./program 
                
                python3 basic_get_generate.py workloads/workload_$i.txt

                for rep in {1..3}
                do 
                    (
                        cat get_$rep.txt
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
            done
            
        done
    done
done
