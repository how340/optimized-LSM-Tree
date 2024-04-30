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
output_file="./experiment_results/basic_range_benchmark_pc.txt"
# Ensure the output file is empty before starting
> "$output_file"

python3 basic_range_generate.py

for level in "${level_ratios[@]}"
do 
    for buffer in "${buffer_sizes[@]}"
    do 
        for thread in "${threads[@]}"
        do 
            for i in {0..5}
            do 
                echo "$((level)) $((buffer)) $((thread)) uni_workload_$i" >> "$output_file"
                (
                    echo "$bits_per_entry $((level)) $((buffer)) $modes $((thread)) $leveling_partitions"   
                    echo ""               
                    cat ./workloads/uni_workload_$i.txt  
                    echo "q" 
                ) | ./program 
                

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
