#pragma once
#ifndef LSM_TREE_H
#define LSM_TREE_H

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>

#include "buffer_level.h"
#include "key_value.h"
#include "run.h"

/* Functions that the LSM tree level should have.

1. Merge runs within levels based on merge policy.

2. conduct basic query functionalities.

3. maintain fence pointer, bloom filters, and file structures for LSM tree

4. be an intermediate between the buffer level and the on-disk level.

5.


*/

// This will be changed to a key and some type of pointer that can point to
// specific location in the file system.
/* fence pointer can work in the follow manner:
    WE keep an vector of size equal to the number of pages per run.
    At each page i, the value at position i-1 in the vector denotes the
   beginning key value on that page. The size of the whole vector needs to be i
   + 1, with the last vector position holding the maximum key value in the
   current run.
*/

class LSM_Tree {
  int buffer_size;
  BufferLevel* in_mem;  // Think about destructor here.
  int bloom_bits_per_entry;
  int level_ratio;
  // const int SAVE_MEMORY_PAGE_SIZE = 512;
  int mode;  // determine whether we run the baseline LSM implementation or
             // optimized version. 0 means optimized version, 1 is un-optimized

 public:
  LSM_Tree(size_t bits_ratio, size_t level_ratio, size_t buffer_size, int mode);
  ~LSM_Tree();

  struct Level_node {
    size_t level;
    size_t max_num_of_runs;
    Level_node* next_level;  // point to the next_run_tree_node
    std::vector<Run> run_storage;

    // Default constructor
    Level_node(size_t lvl, size_t numRuns, Level_node* nextLvl = nullptr)
        : level(lvl), max_num_of_runs(numRuns), next_level(nextLvl){};
  };

  typedef struct Level_node Level_Node;

  Level_Node* root;

  void put(KEY_t key, VALUE_t val);
  std::unique_ptr<Entry_t> get(KEY_t key);
  std::vector<Entry_t> range(KEY_t lower, KEY_t upper);
  void del(KEY_t key);
  std::vector<Entry_t> merge(Level_Node* cur);

  Run create_run(std::vector<Entry_t>);
  void create_bloom_filter(BloomFilter* bloom, const std::vector<Entry_t>& vec);
  void save_to_memory(std::string filename, std::vector<KEY_t>* fence_pointer,
                      std::vector<Entry_t>& vec);

  // saving files on quit command
  void exit_save_memory();
  void level_meta_save();
  void exit_save();

  // helper functions
  void print();
  std::string generateRandomString(size_t length);
};

#endif