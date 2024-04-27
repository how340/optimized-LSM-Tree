#!/bin/bash
(
    #std::cin >> bits_per_entry >> level_ratio >> buffer_size >> mode >> threads >> leveling_partition;
    echo "0.0001 10 1000 0 1 0"  # Send initialization parameters
    echo ""                 # Mimic pressing "Enter" to finalize initialization
    cat workloads/workload_0.txt  # Send the workload data
    echo q
) | ./program