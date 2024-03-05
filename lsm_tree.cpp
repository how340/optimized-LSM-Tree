#include "lsm_tree.h"

LSM_Tree::LSM_Tree(size_t bits_ratio, size_t level_ratio)
    : bloom_bits_per_entry(bits_ratio), level_ratio(level_ratio)
{
    root = new Run_tree_node{1, level_ratio};


}

LSM_Tree::~LSM_Tree() {
        delete in_mem; // Free the dynamically allocated in-memory buffer
        // Consider implementing or calling a method here to clean up other dynamically allocated resources, if any
}

int LSM_Tree::buffer_insert(KEY_t key, VALUE_t val)
{
    int insert_result; 
    insert_result = in_mem->insert(key, val);

    if (insert_result == -1){//buffer size is full.
        //Need to initiate push down of information into a disk-stored run. 
        
        return insert_result; 
    } 

    return insert_result;
}
