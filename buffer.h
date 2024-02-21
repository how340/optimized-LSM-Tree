// This class implements the basic componenet in each level of the LSM Tree. 
// Each tier of the lsm tree will have 1 or more level. 
#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include <math.h>
#include <vector>
#include <map>
#include <iostream>

#include "key_value.h"

// The buffer level uses the CPP map library, which is implemented as black red tree. 

class Buffer {
    std::map<KEY_t, VALUE_t> mp;
    int max_size; //memory limit allocated at the buffer level
    int current_size = 0 ; 

    // some status of a disk operation for utility
    enum status {InsertNotComplete, Success, Underflow, NotFound, BufferFull};

public: 
    // constructor
    Buffer(int size) : max_size(size){
        max_size = size;

    };
    // add destructor

    status insert(KEY_t key, VALUE_t val){
        // TODO: add in check size of the buffer later. 
        if (current_size >= max_size){
            std::cout << "the buffer is currently full" << std::endl; 
            return BufferFull; 
        } 

        mp[key] = val; // this replaces old value in case of duplicate key
        current_size ++; // use hard count for proxy of memory right now. 
        return Success;
    };


    // TODO: discuss with TA on how to implement this the best. 
    bool del(KEY_t key){
        return false;
    }

    // search for a key and return value
    VALUE_t* get(KEY_t key){
        if (mp.find(key) != mp.end()){
            std::cout << "found the element :" << mp[key] << std::endl; 
            return &mp[key];
        } else {
            std::cout << "Did not find the element" << std::endl; 
            static VALUE_t ret = -1;
            return &ret; 
        };
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

    // clean content in the buffer. Used when all information are flushed down next level. 
    void clear_buffer(void){
        mp.clear();
        current_size = 0;
    };

};

#endif