#pragma once
#ifndef LSM_TREE_H
#define LSM_TREE_H

#include "buffer_level.h"
#include "key_value.h"

class LSM_Tree{

    BufferLevel::BufferLevel(1000);//arbituary level for now. 
    int bits_per_entry; 


public: 
    LSM_Tree(int bits_ratio): bits_per_entry(bits_ratio); 

    ~LSM_Tree():

    

}

#endif 