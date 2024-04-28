#!/bin/bash

# # Generate dataset for the experiement - I am reusing the same dataset here to reduce experiment time
# for ((i = 0; i <= 5; i++))
# do
#     ./workloads/generator --puts $((2**((i*2)+15))) --seed $RANDOM > ./workloads/workload_${i}.txt
# done

# load dataset into tree. 
(
    echo "0.0001 6 8388608 0 8 0 "  # Send initialization parameters
    echo ""                 # Mimic pressing "Enter" to finalize initialization
    cat workloads/workload_4.txt  # Send the workload data
    echo "q" 
) | ./program 

# python3 basic_get_generate.py workloads/workload_5.txt

# for i in {1..3}
# do 
    
#     (
#         cat get_$i.txt
#         echo "q"
#     ) | ./program >> temp

#     echo "" >> temp
# done 

# for file in ./*; do
#   # Check if the file has a .dat or .txt extension
#   if [[ "$file" == *.dat ]] || [[ "$file" == *.txt ]]; then
#     # If it does, delete the file
#     rm "$file"
#     echo "Deleted: $file"
#   fi
# done