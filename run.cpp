#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream> 

#include "run.h"

// the class access the files that represents a run.
Run::Run(std::string file_name, BloomFilter* bloom_filter, std::vector<KEY_t>* fence){
    file_location = file_name; 
    bloom = bloom_filter;
    fence_pointers = fence; 
}

Run::~Run(void)
{
}

bool Run::search_bloom(KEY_t key){
    return bloom->is_set(key);
}

int Run::search_fence(KEY_t key){   
    int starting_point; 

    for(int i = 0; i < fence_pointers->size() ; i++){

        if (key > fence_pointers->at(i) && key < fence_pointers->at(i)){ 
            starting_point = i;
            return starting_point;
        }
    }

    return -1; // return -1 if we hit a false positive with bloom filter. 
}


// read certain bytes from the binary file storages.  
VALUE_t Run::disk_search(int starting_point, size_t bytes_to_read, KEY_t key) {
    //std::lock_guard<std::mutex> guard(Run_mutex);

    Entry_t kvp;
    size_t read_size = bytes_to_read; 

    std::ifstream file(file_location, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for reading");
    }

    // shift file stream pointer to starting_point base on fence pointer result. 
    file.seekg(starting_point * 64, std::ios::beg);// *64 cuz each pair is 2*32 bits. 

    while(read_size > 0){
        file.read(reinterpret_cast<char*>(&kvp.key), sizeof(kvp.key));
        file.read(reinterpret_cast<char*>(&kvp.val), sizeof(kvp.val));

        if (key == kvp.key){
            file.close();
            return kvp.val;
        }

        read_size--;
    }
    
    // how to handle the case when 
    file.close();
    return kvp.val;
}
