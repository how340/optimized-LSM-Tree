#pragma once
#ifndef LSM_TREE_H
#define LSM_TREE_H

#include "buffer_level.h"
#include "key_value.h"
/* Functions that the LSM tree level should have. 

1. Merge runs within levels based on merge policy. 

2. conduct basic query functionalities. 

3. maintain fence pointer, bloom filters, and file structures for LSM tree

4. be an intermediate between the buffer level and the on-disk level. 

5. 


*/


     // This will be changed to a key and some type of pointer that can point to specific location in the file system. 
    /* fence pointer can work in the follow manner: 
        WE keep an vector of size equal to the number of pages per run. 
        At each page i, the value at position i-1 in the vector denotes the beginning key value on that page. 
        The size of the whole vector needs to be i + 1, with the last vector position holding the maximum key value in the current run.
    */




class LSM_Tree{

    BufferLevel::BufferLevel(1000);//arbituary level for now. 
    int bits_per_entry; 


public: 
    LSM_Tree(int bits_ratio): bits_per_entry(bits_ratio); 

    ~LSM_Tree():

    

}

#endif 