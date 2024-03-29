// This class implements the basic componenet in each level of the LSM Tree. 
// Each tier of the lsm tree will have 1 or more level. 
#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include <math.h>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <bits/stdc++.h>

#include "key_value.h"
#include "bloom.h"

// The buffer level uses the CPP map library, which is implemented as black red tree.

class BufferLevel {
    std::map<KEY_t, VALUE_t> mp;
    int max_size; //memory limit allocated at the buffer level
    int current_size = 0 ; 
    const int MEMORY_PAGE_SIZE = 512;

public: 
    // constructor
    BufferLevel(int size) : max_size(size){
        max_size = size;
    };

    // TODO: we don't need a destructor here right? 

    int insert(KEY_t key, VALUE_t val){
        if (current_size >= max_size){
            // std::cout << "the buffer is currently full" << std::endl; 
            return -1; 
        } 

        mp[key] = val; // this replaces old value in case of duplicate key
        current_size ++; 

        return 0;
    };

    // TODO: discuss with TA on how to implement this the best. 
    bool del(KEY_t key){
        // thoughts: if the key is found in the memory level, we can just delete. 
        //           else, we can keep a record of it in memory, and then deal with it during merge operations (either delete or propogate down the level)
        return false;
    }

    // search for a key and return value
    // TODO: this actually needs more work. 
    // The object will return an iterator, the higher level function need to consider this for searching. 
    std::unique_ptr<Entry_t> get(KEY_t key){
        auto it = mp.find(key);
        if (it != mp.end()) {
            // std::cout << "Found the element: " << it->second << std::endl;
            // Assuming Entry_t can be constructed or assigned from the type of `it->second`
            return std::make_unique<Entry_t>(Entry_t{it->first, it->second});
        }
        return nullptr;
    }

    // Get a range of values stored in the tree.
    std::vector<VALUE_t> get_range(KEY_t lower, KEY_t upper){
        // use map iterators to acheive this. 
        auto start = mp.lower_bound(lower);
        auto end = mp.upper_bound(upper);

        std::vector<VALUE_t> ret;

        for (auto it = start; it != end; ++it){
            std::cout << it -> first << " ==> " << it->second << std::endl;
            ret.push_back(it->second);
        };

        return ret; 
    }

    // clean content in the buffer. Used when pushing to next level. 
    void clear_buffer(void){
        mp.clear();
        current_size = 0;
    };

    // this function converts the underlying tree structure to a vector for push down operation when the buffer is full. 
    std::vector<Entry_t> convert_tree_to_vector(){
        std::vector<Entry_t> vec; 
        for(std::map<KEY_t, VALUE_t>::iterator iter = mp.begin();iter != mp.end(); ++iter){
            Entry_t entry; 
            entry.key = iter->first;
            entry.val = iter->second;
            vec.push_back(entry);
        }   
        return vec; 
    }

    // -----------------------------------------
    // the two functions related to fence pointer and bloom filter will probably be moved to the lsm-tree level later.
    /*
    the bloomfilter will be handled by the LSM tree class, as the lsm tree will need to handle the relationships 
    between level and runs. 
    
    */
    void create_bloom_filter(BloomFilter* bloom, const std::vector<Entry_t>& vec){
        for (int i = 0; i < mp.size(); i++){
            bloom->set(vec[i].key);
        }
    };

    // writes content of buffer to file on disk. 
    // passing by reference needs to be worked on a bit here. Don't know how this came to be. 
    void save_to_memory(std::string filename,  std::vector<KEY_t>* fence_pointer, std::vector<Entry_t>& vec){
        // two pointers to keep track of memory and fence_pointer traversal. 
        int memory_cnt = 0, fence_pointer_index = 0;
        std::ofstream out(filename, std::ios::binary);

        std::cout << "incoming vector size is : " << vec.size() << std::endl;
        if (!out.is_open()) {
            throw std::runtime_error("Unable to open file for writing");
        }

        for (const auto& entries : vec){// go through all entries.
            if (memory_cnt == 0){
                fence_pointer->push_back(entries.key);
                fence_pointer_index++; 
            }
            
            out.write(reinterpret_cast<const char*>(&entries.key), sizeof(entries.key));
            out.write(reinterpret_cast<const char*>(&entries.val), sizeof(entries.val));
            
            memory_cnt = sizeof(entries.key) + sizeof(entries.val) + memory_cnt; 
            
            if (memory_cnt == MEMORY_PAGE_SIZE)  memory_cnt = 0; // reached page maximum
        }  
        // Lazy approach right now, the last fence pointer is just the maximum key for now. 
        fence_pointer->push_back(vec.back().key);

        out.close(); 
        std::cout << "Successfully written to file : " << filename << std::endl; 
    }

    // function returns the current size of the buffer level memory. 
    int size(void){
        return current_size;
    }

};

    

#endif