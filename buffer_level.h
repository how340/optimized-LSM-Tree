// This class implements the basic componenet in each level of the LSM Tree.
// Each tier of the lsm tree will have 1 or more level.
#pragma once
#ifndef BUFFER_H
#define BUFFER_H

#include <bits/stdc++.h>
#include <math.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

#include "bloom.h"
#include "key_value.h"

// The buffer level uses the CPP map library, which is implemented as black red
// tree.

class BufferLevel {
  std::vector<Entry_t> store;

  int max_size;  // memory limit allocated at the buffer level
  int current_size = 0;

 public:
  // constructor
  BufferLevel(int size) : max_size(size) { max_size = size; };

  int insert(KEY_t key, VALUE_t val) {
    if (current_size >= max_size) {
      return -1;
    }

    Entry_t entry{key, val, false};
    store.push_back(entry);
    current_size++;

    return 0;
  };

  //overload for easy loading memory
  int insert(Entry_t entry) {
    if (current_size >= max_size) {
      return -1;
    }

    store.push_back(entry);
    current_size++;

    return 0;
  };

  // Deletion is done as write with an additional flag.
  int del(KEY_t key) {
    if (current_size >= max_size) {
      return -1;
    }

    Entry_t entry{key, 0, true};
    store.push_back(entry);
    current_size++;

    return 0;
  }

  // search for a key and return value. Update and deletion are done during
  // compaction.
  std::unique_ptr<Entry_t> get(KEY_t key) {
    // core intuitation - the last item is the latest value. We report the
    // latest no matter what.
    for (auto rit = store.rbegin(); rit != store.rend(); ++rit) {
      if (rit->key == key) {
        return std::make_unique<Entry_t>(*rit);
      }
    }

    return nullptr;
  }

  // Get a range of values stored in the tree.
  std::vector<Entry_t> get_range(KEY_t lower, KEY_t upper) {
    std::unordered_map<KEY_t, Entry_t> hash_mp;
    std::vector<Entry_t> ret;

    for (int i = 0; i < store.size(); i++) {
      Entry_t temp_entry = store[i];
      if (temp_entry.key >= lower && temp_entry.key <= upper) {
        if (temp_entry.del) {
          // delete item exists on same level, remove the item.
          if (hash_mp.find(temp_entry.key) != hash_mp.end()) {
            hash_mp.erase(temp_entry.key);
          } else {  // delete item not exists on same level. proprogate.
            hash_mp[temp_entry.key] = temp_entry;
          }
        } else {  // this operation will always use a later key/val to update
                  // output value.
          hash_mp[temp_entry.key] = temp_entry;
        }
      }
    }
    for (const auto &pair : hash_mp) {
      ret.push_back(pair.second);
    }
    
    return ret;
  }

  // clean content in the buffer. Used when pushing to next level.
  void clear_buffer(void) {
    store.clear();
    current_size = 0;
  };

  // function returns the current size of the buffer level memory.
  int size(void) { return current_size; }

  /*
  flush_buffer() - The function flushes the content into a vector format.

  During the flush process, updates and deletes that can be detected in the
  buffer will be resolved.

  The algo is essentially the same as range, without testing value range.
  */
  std::vector<Entry_t> flush_buffer() {
    std::unordered_map<KEY_t, Entry_t> hash_mp;
    std::vector<Entry_t> ret;

    for (int i = 0; i < store.size(); i++) {
      Entry_t temp_entry = store[i];

      if (temp_entry.del) {
        // delete item exists on same level, remove the item.
        if (hash_mp.find(temp_entry.key) != hash_mp.end()) {
          hash_mp.erase(temp_entry.key);
        } else {  // delete item not exists on same level. proprogate.
          hash_mp[temp_entry.key] = temp_entry;
        }
      } else {  // this operation will always use a later key/val to update
                // output value.
        hash_mp[temp_entry.key] = temp_entry;
      }
    }
    for (const auto &pair : hash_mp) {
      ret.push_back(pair.second);
    }
    std::sort(ret.begin(), ret.end());
    return ret;
  }
};

#endif