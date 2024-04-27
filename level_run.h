#pragma once
#ifndef LEVEL_RUN_H
#define LEVEL_RUN_H

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include <mutex> 

#include "bloom.h"
#include "key_value.h"
#include "lib/ThreadPool.h"

class Level_Run {
  ThreadPool& pool;

  float bits_per_entry;  // need to change the LSM tree code to store this as a
                         // constant.

  int current_size = 0;
  int max_size;  // number of blocks allowed in each level.
  int current_level;
  int level_ratio;
  int buffer_size;

  float leveling_flush_ratio = 0.9;

  struct Node {
    BloomFilter* bloom;
    std::vector<KEY_t> fence_pointers;
    std::string file_location;

    // these are stored as the rough fence posts of this node/
    KEY_t lower;
    KEY_t upper;

    Node* next = nullptr;
    bool is_empty = true;

    ~Node() {
      delete bloom;     // Delete the BloomFilter object if it exists.
      bloom = nullptr;  // Prevent dangling pointer.

      std::filesystem::path fileToDelete(file_location);
      std::filesystem::remove(fileToDelete);
    }
  };

  Node* root;

 public:
  Level_Run(ThreadPool& pool,
            int max_size,
            int level,
            int ratio,
            int buffer,
            int bits_per_entry)
      : pool(pool),
        max_size(max_size),
        current_level(level),
        level_ratio(ratio),
        buffer_size(buffer),
        bits_per_entry(bits_per_entry) {
    root = new Node;
  }
  // ~Level_Run();

  // used to insert blocks of data into the existing leveling levels.
  void insert_block(std::vector<Entry_t>&);

  // methods to load level and insert into table.
  std::vector<Entry_t> load_full_file(
      std::string,
      std::vector<KEY_t>&);  // just need the index in the storage.
  Node* save_to_memory(std::vector<Entry_t>&);
  Node* process_block(const std::vector<Entry_t>&, int, int);

  // Find some blocks to push down for merging
  std::unordered_map<KEY_t, Entry> flush();

  // searching in this level; TODO: add the two functions.
  std::unique_ptr<Entry_t> get(KEY_t key);
  std::unordered_map<KEY_t, Entry_t> range_search(KEY_t lower, KEY_t upper);

  std::unique_ptr<Entry_t> disk_search(KEY_t, std::string, int);
  int search_fence(KEY_t key, std::vector<KEY_t>&);
  std::unordered_map<KEY_t, Entry_t> range_block_search(KEY_t, KEY_t, Node*);
  // helper functions
  void print();
  std::string generate_file_name(size_t length);
  int return_size();
  int return_max_size();
};

#endif