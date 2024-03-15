#pragma once
#ifndef RUN_H
#define RUN_H

#include <vector>

#include "key_value.h"
#include "bloom.h"

class Run {
    // The two search assistant elements are stored at the LSM level. Thus, these elements just need to be pointers. 
    BloomFilter* bloom;
    std::vector<KEY_t>* fence_pointers;
    std::string file_location; // storage location of the stored binary file

public:
    Run(std::string file_name, BloomFilter* bloom, std::vector<KEY_t>* fence);
    ~Run();

    int search_fence(KEY_t key);
    bool search_bloom(KEY_t key);
    std::string get_file_location();

    //search for single value
    std::unique_ptr<Entry_t> disk_search(int starting_point, size_t bytes_to_read, KEY_t key);
    //std::vector<Entry_t> range_disk_search(); Implement later after having LSM tree running. 
};


#endif 