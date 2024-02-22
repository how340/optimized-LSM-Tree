// This class implements the basic componenet in each level of the LSM Tree. 
// Each tier of the lsm tree will have 1 or more level. 
#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include <math.h>
#include <vector>
#include <map>
#include <iostream>
#include <bits/stdc++.h>

#include "key_value.h"
#include "bloom.h"
// The buffer level uses the CPP map library, which is implemented as black red tree. 

class BufferLevel {
    std::map<KEY_t, VALUE_t> mp;
    int max_size; //memory limit allocated at the buffer level
    int current_size = 0 ; 

    // some status of a disk operation for utility
    enum status {Success, Underflow, NotFound, BufferFull};

public: 
    // constructor
    BufferLevel(int size) : max_size(size){
        max_size = size;
    };

    // TODO: we don't need a destructor here right? 

    status insert(KEY_t key, VALUE_t val){
        if (current_size >= max_size){
            std::cout << "the buffer is currently full" << std::endl; 
            return BufferFull; 
        } 

        mp[key] = val; // this replaces old value in case of duplicate key
        current_size ++; 

        return Success;
    };

    // TODO: discuss with TA on how to implement this the best. 
    bool del(KEY_t key){
        // thoughts: if the key is found in the memory level, we can just delete. 
        //           else, we can keep a record of it in memory, and then deal with it during merge operations (either delete or propogate down the level)
        return false;
    }

    // search for a key and return value
    // TODO: this actually needs more work. 
    auto get(KEY_t key){
        if (mp.find(key) != mp.end()){
            std::cout << "found the element :" << mp[key] << std::endl; 
        } else {
            std::cout << "Did not find the element" << std::endl; 
        };
        return mp.find(key);
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

    void create_bloom_filter(BloomFilter& bloom){
        //TODO:determine needed size for bloom_filter. we will need to implement the MONKEY approach. 

        std::vector<std::pair<KEY_t, VALUE_t>> vec; // this operation is conducted many times. 
        copy(mp.begin(), mp.end(), back_inserter(vec)); 

        for (int i = 0; i < mp.size(); i++){
            bloom.set(vec[i].first);
        }
    };

    // push the buffer layer into a memory file. Probably needs to be a binary file for it to work. 
    void save_to_memory(std::string filename){
        //serialize the tree first. 
        std::vector<std::pair<KEY_t, VALUE_t>> vec; 
        copy(mp.begin(), mp.end(), back_inserter(vec)); 

        // open file_stream and convert vector side by side into a file. 
        std::ofstream out(filename);
        if (!out.is_open()) {
            throw std::runtime_error("Unable to open file for writing");
        }

        for(int i = 0; i < vec.size(); i++){
            out << vec[i].first << vec[i].second ;
        }

        mp.clear();// clear the buffer after pushing the file to save. 
    }
   
   // function returns the current size of the buffer level memory. 
   int size(void){
        return current_size;
   }

};

#endif