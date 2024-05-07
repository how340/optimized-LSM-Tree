#!/bin/bash

# Define the variables
bits_per_entry=0.0001
level_ratios=('10')
buffer_sizes=('2**17' '2**20' '2**23')
modes=0
threads=('8')
leveling_partitions=0

# Base directory for output files
output_dir="experiment_results"
mkdir -p $output_dir

# Output file location
output_file="experiment_results/basic_range_benchmark_gau.txt"
# Ensure the output file is empty before starting
> "$output_file"

# python3 basic_range_generate.py

# #uniform distribution data
# for level in "${level_ratios[@]}"
# do 
#     for buffer in "${buffer_sizes[@]}"
#     do 
#         for thread in "${threads[@]}"
#         do 
#             for i in {0..10}
#             do  
#                 echo "$((level)) $((buffer)) $((thread)) uni_workload_$i" >> "$output_file"

#                 (
#                     echo "$bits_per_entry $((level)) $((buffer)) $modes $((thread)) $leveling_partitions"   
#                     echo ""               
#                     cat workloads/uni_workload_$i.txt 
#                     echo q
#                 ) | ./program >> "$output_file"

#                 python3 range_query_generator.py 
#                 for rep in {1..3}
#                 do 
#                     (
#                         cat range_get_$rep.txt
#                         echo "q"
#                     ) | ./program >> "$output_file"
#                 done 

#                 for file in ./*; do
#                 # Check if the file has a .dat or .txt extension
#                 if [[ "$file" == *.dat ]] || [[ "$file" == *.txt ]]; then
#                     # If it does, delete the file
#                     rm "$file"
#                     echo "Deleted: $file"
#                 fi
#                 done
#                 echo "" >> "$output_file"
#             done
            
#         done
#     done
# done

echo "------------------------------------------------------------"
echo "************************Gaussian****************************"
echo "------------------------------------------------------------"


#Gaussian distribution data
for level in "${level_ratios[@]}"
do 
    for buffer in "${buffer_sizes[@]}"
    do 
        for thread in "${threads[@]}"
        do 
            for i in {0..10}
            do 
                echo "$((level)) $((buffer)) $((thread)) gau_workload_$i" >> "$output_file"
                 # Monitor with perf
                    (
                        echo "$bits_per_entry $((level)) $((buffer)) $modes $((thread)) $leveling_partitions"   
                        echo ""                 # Mimic pressing "Enter" to finalize initialization
                        cat workloads/gau_workload_$i.txt  # Send the workload data
                        echo q
                    ) | ./program >> "$output_file"

                python3 range_query_generator.py 
                for rep in {1..3}
                do 
                    (
                        cat range_get_$rep.txt
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

