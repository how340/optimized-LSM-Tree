// This class implements the basic componenet in each level of the LSM Tree. 
// Each tier of the lsm tree will have 1 or more level. 
#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include <math.h>
#include <vector>

#include "key_value.h"
#include "bloom.h"
// will use a tree implementation for each run. 

// Each level will be implemented as a tree with n nodes, where the n = level
class Buffer {
    int size;
    KEY_t range[2] = {0, 0}; // think if this will lead to unexpected behvavior. 

public: 
    // constructor
    Buffer(int size) : size(size){}
    // TODO: do we need a destructor here? 

    // insert should check current size, and return bool to indicate if the run is filled. 
    bool insert(KEY_t key, VALUE_t val){}

    // search for a key and return value
    VALUE_t* get(KEY_t key){}

    // get current range in buffer 
    KEY_t* get_range(void){}

    // clean content in the buffer. 
    void clear_buffer(void){}

};


#endif