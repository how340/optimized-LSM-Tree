#!/bin/bash

# # # Generate dataset for the experiement - I am reusing the same dataset here to reduce experiment time
./workloads/generator --puts 33554432 --seed $RANDOM > ./workloads/workload_double_check.txt


# load dataset into tree. 

(echo "0.0001 10 131072 0 8 10"  # Send initialization parameters
echo ""  
cat workloads/workload_double_check.txt
# echo "l workloads/load_1_0.dat " 
echo "q" ) | ./program 

# python3 basic_get_generate.py workloads/workload_1.txt

# for rep in {1..3}
# do 
#     (
#         cat get_$rep.txt
#         echo "q"
#     ) | ./program 
# done 


for file in ./*; do
  # Check if the file has a .dat or .txt extension
  if [[ "$file" == *.dat ]] || [[ "$file" == *.txt ]]; then
    # If it does, delete the file
    rm "$file"
    #echo "Deleted: $file"
  fi
done

# client_cnt=4
# python3 basic_load_command_generate.py $client_cnt

# ./server &
# SERVER_PID=$!
# # Start multiple clients in the background

# start_time=$(date +"%Y-%m-%d %H:%M:%S.%3N")

# for (( i =0; i<client_cnt; i++ ))
# do
#     ./client "client_input$i.txt" &
# done

# wait

# end_time=$(date +"%Y-%m-%d %H:%M:%S.%3N")
# start_seconds=$(date -d "$start_time" +%s.%3N)
# end_seconds=$(date -d "$end_time" +%s.%3N)
# duration=$(echo "$end_seconds - $start_seconds" | bc)
# echo "$duration ms"