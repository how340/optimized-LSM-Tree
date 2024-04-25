#!/bin/bash

(
    #std::cin >> bits_per_entry >> level_ratio >> buffer_size >> mode >> threads >> leveling_partition;
    echo "0.0001 3 10000 1 1 100"  # Send initialization parameters
    echo ""                 # Mimic pressing "Enter" to finalize initialization
    cat workloads/workload.txt  # Send the workload data
    echo q
) | ./program
