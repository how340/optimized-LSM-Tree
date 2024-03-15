#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream> 
#include <iostream>
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

std::string Run::get_file_location()
{
    return file_location;
}

int Run::search_fence(KEY_t key){   
    int starting_point; 

    for(int i = 0; i < fence_pointers->size() ;++i){
        if (key > fence_pointers->at(i) && key < fence_pointers->at(i+1)){ 
            starting_point = i;
            return starting_point;
        }
    }

    return -1; // return -1 if we hit a false positive with bloom filter. 
}


// read certain bytes from the binary file storages. 
// Make sure this approach doesn't cause memory leak. As long as the return variable can go out of scope, this wil be safe. 
// TODO: consider the case where the node could be delete flag, will need to go in to check. 
std::unique_ptr<Entry_t> Run::disk_search(int starting_point, size_t bytes_to_read, KEY_t key) {
    //std::lock_guard<std::mutex> guard(Run_mutex);

    auto entry = std::make_unique<Entry_t>();

    size_t read_size = bytes_to_read; 

    std::ifstream file(file_location, std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for reading");
    }

    // shift file stream pointer to starting_point base on fence pointer result. 
    file.seekg(starting_point * 512, std::ios::beg);// *512 cuz 512 bytes per page. 

    while(read_size > 0){
        file.read(reinterpret_cast<char*>(&entry->key), sizeof(entry->key));
        file.read(reinterpret_cast<char*>(&entry->val), sizeof(entry->val));

        if (key == entry->key){
            file.close();
            return entry;
        }
        read_size--;
    }
    
    file.close();
    return nullptr;// return null if we couldn't find the result. 
}


// TODO: Might just come back to this later. Probably not needed for midway check. 
// std::vector<Entry_t> Run::range_disk_search(KEY_t left, KEY_t right){
//     std::vector<Entry_t> search_result; 

//     int left_post = -1;
//     int right_post = -1;

//     if(Run::search_bloom(left)){
//         left_post = Run::search_fence(left);
//     }
//     if(Run::search_bloom(right)){
//         right_post = Run::search_fence(right);
//     }

//     if(left_post == -1 && right_post == -1){// no value in the disk.
//         return search_result;
//     }else if (left_post == -1){
//         left_post = 0;
//     }else if (right_post == -1){
//         right_post = fence->size() - 1;
//     }

//     // search through the valid pages for here. 

//     return search_result;
// }