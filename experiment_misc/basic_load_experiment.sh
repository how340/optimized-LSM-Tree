#!/bin/bash



./server &
SERVER_PID=$!
# Give the server a moment to start up
sleep 2  

start_time=$(date +"%Y-%m-%d %H:%M:%S.%3N")

for (( j =0; i<client_cnt; j++ ))
do
   ./client "client_input$j.txt" &
done

end_time=$(date +"%Y-%m-%d %H:%M:%S.%3N")
start_seconds=$(date -d "$start_time" +%s.%3N)
end_seconds=$(date -d "$end_time" +%s.%3N)
duration=$(echo "$end_seconds - $start_seconds" | bc)
echo "$duration ms" >> temp
