// This class implements the basic componenet in each level of the LSM Tree. 
// Each tier of the lsm tree will have 1 or more level. 
#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include <math.h>
#include <vector>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <bits/stdc++.h>

#include "key_value.h"
#include "bloom.h"

// The buffer level uses the CPP map library, which is implemented as black red tree.

class BufferLevel {
    // two vectors in parallel to keep track of key values pairs using the same index. 
    std::vector<Entry_t> store;

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
            return -1; 
        } 
        
        Entry_t entry{key, val, false};
        store.push_back(entry);
        current_size ++; 

        return 0;
    };

    // Deletion is done as write with an additional flag. 
    int del(KEY_t key){
        if (current_size >= max_size){
            return -1; 
        } 

        Entry_t entry{key, 0, true};
        store.push_back(entry);
        current_size ++; 

        return 0;
    }

    // search for a key and return value. Update and deletion are done during compaction.
    std::unique_ptr<Entry_t> get(KEY_t key){
        Entry_t entry; 

        // core intuitation - the last item is the latest value. We report the latest no matter what. 
        for (int i=0; i < store.size(); i++){
            if (store[i].key == key){
                Entry_t entry = store[i];
            } 
        }

        return std::make_unique<Entry_t>(entry);
    }

    // Get a range of values stored in the tree.
    std::vector<Entry_t> get_range(KEY_t lower, KEY_t upper){
        std::unordered_map<KEY_t, VALUE_t> hash_mp; 
        std::vector<Entry_t> ret; 

        for (int i=0; i < store.size(); i++){
            Entry_t temp_entry = store[i]; 
            if (temp_entry.key >= lower && temp_entry.key <= upper){
                if (temp_entry.del){
                    hash_mp.erase(temp_entry.key);
                } else {
                    hash_mp[temp_entry.key] = temp_entry.val; 
                }
            }
        }
        for (const auto& pair : hash_mp) {
                ret.push_back(Entry_t{pair.first, pair.second, false});
            }
        return ret; 
    }

    // clean content in the buffer. Used when pushing to next level. 
    void clear_buffer(void){
        store.clear();
        current_size = 0;
    };

    // function returns the current size of the buffer level memory. 
    int size(void){
        return current_size;
    }

    std::vector<Entry_t> return_store(){
        return store; 
    }


};

    

#endif