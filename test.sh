#!/bin/bash

# # Normal update
# ./workloads/generator --puts 200000 --seed $RANDOM > ./workloads/workload_demo.txt

# (echo "0.0001 10 131072 0 8 0"  #
# echo ""  
# cat workloads/workload_demo.txt
# echo "q" ) | ./program 

# # skewed update
# ./workloads/generator --puts 5000 --seed 42 > ./workloads/workload_demo_small.txt

# (echo "0.0001 10 131072 0 8 0"  #
# echo ""  
# for i in {1..40}
# do 
#   cat workloads/workload_demo_small.txt
# done 
# echo "q" ) | ./program

# # normal read
./workloads/generator --puts 200000 --gets 2000 --gets-misses-ratio 0 --seed $RANDOM > ./workloads/workload_demo.txt

(echo "0.0001 10 131072 0 8 0"  #
echo ""  
cat workloads/workload_demo.txt
echo "q" ) | ./program 

# skewed read
# ./workloads/generator --puts 200000 --gets 2000 --gets-skewness 0.5 --gets-miss 0 --seed $RANDOM > ./workloads/workload_demo.txt

# (echo "0.0001 10 131072 0 8 0"  #
# echo ""  
# cat workloads/workload_demo.txt
# echo "s"
# echo "q" ) | ./program 

# for file in ./*; do
#   # Check if the file has a .dat or .txt extension
#   if [[ "$file" == *.dat ]] || [[ "$file" == *.txt ]]; then
#     # If it does, delete the file
#     rm "$file"
#     #echo "Deleted: $file"
#   fi
# done