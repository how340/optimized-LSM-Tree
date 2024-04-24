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
#include <set>
#include <sstream>

#include "buffer_level.h"
#include "key_value.h"
#include "level_run.h"
#include "lib/ThreadPool.h"
#include "run.h"

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
  size_t num_of_threads;
  ThreadPool pool;
  BufferLevel* in_mem;  // Think about destructor here.

  long total; 
  int buffer_size;
  float bloom_bits_per_entry;
  int level_ratio;
  int lazy_cut_off = 2;
  int mode;  // determine whether we run the baseline LSM implementation or
             // optimized version. 0 means optimized version, 1 is un-optimized
  int leveling_partitions; 

  int total_levels = 1;
  /******************************************************
          Struct for storing LSM tree structure
  ******************************************************/
  struct Level_Node  // Constructs a level with the tiered strategy
  {
    size_t level;
    size_t max_num_of_runs;
    Level_Node* next_level;  // point to the next_run_tree_node
    std::vector<Run> run_storage;

    // Default constructor
    Level_Node(size_t lvl, size_t numRuns, Level_Node* nextLvl = nullptr)
        : level(lvl), max_num_of_runs(numRuns), next_level(nextLvl){};
  };

  struct Leveling_Node  // construct a level with the leveling strategy.
  {
    size_t level;
    Leveling_Node* next_level;  // point to the next_run_tree_node
    Level_Run* leveled_run;// stores the Level_run object. 
  };

  Level_Node* root;
  Leveling_Node* level_root = nullptr;

  /******************************************************
              Evaluation stat variables.
  ******************************************************/
  // evaluation stat variables. PUT operator
  std::chrono::duration<double> accumulated_time;
  std::chrono::duration<double> merge_accumulated_time;
  std::chrono::duration<double> merge_update_accumulated_time;
  std::chrono::duration<double> merge_del_accumulated_time;
  // GET
  std::chrono::duration<double> get_accumulated_time;
  std::chrono::duration<double> get_disk_accumulated_time;
  // Range
  std::chrono::duration<double> range_accumulated_time;

  // FPR calculation
  std::mutex fpHitsMutex;  // Mutex to protect FP_hits
  int FP_hits;

 public:
  LSM_Tree(float bits_ratio,
           size_t level_ratio,
           size_t buffer_size,
           int mode,
           size_t threads, 
           int partition);
  ~LSM_Tree();

  void put(KEY_t key, VALUE_t val);
  void put(Entry_t entry);  // overload for loading saved memory.

  std::unique_ptr<Entry_t> get(KEY_t key);
  std::unique_ptr<Entry_t> process_run(
      typename std::vector<Run>::reverse_iterator rit,
      const KEY_t& key);

  std::vector<Entry_t> range(KEY_t lower, KEY_t upper);
  void del(KEY_t key);

  // merge policies
  void merge_policy();
  std::unordered_map<KEY_t, Entry_t> merge(Level_Node*& cur);
  std::unordered_map<KEY_t, Entry_t> partial_merge(Leveling_Node*& cur);

  Run create_run(std::vector<Entry_t>, int);
  void create_bloom_filter(BloomFilter* bloom, const std::vector<Entry_t>& vec);
  void save_to_memory(std::string filename,
                      std::vector<KEY_t>* fence_pointer,
                      std::vector<Entry_t>& vec);

  // saving files on quit command
  void exit_save_memory();
  void level_meta_save();
  void exit_save();

  // Loading functions
  void load_memory();
  void reconstruct_file_structure(std::ifstream& meta);
  std::vector<Entry_t> load_full_file(std::string file_location,
                                      std::vector<KEY_t> fence_pointers);

  // helper functions
  void print();
  std::string generateRandomString(size_t length);
  void print_statistics();
};

#endif