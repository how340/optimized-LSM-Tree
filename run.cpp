#include "run.h"

#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>

// the class access the files that represents a run.
Run::Run(std::string file_name, BloomFilter *bloom_filter,
         std::vector<KEY_t> *fence) {
  file_location = file_name;
  bloom = bloom_filter;
  fence_pointers = fence;
}

Run::~Run(void) {}

bool Run::search_bloom(KEY_t key) { return bloom->is_set(key); }

std::string Run::get_file_location() { return file_location; }

int Run::search_fence(KEY_t key) {
  int starting_point;

  // TODO: I can probably do this with binary search as well.
  for (int i = 0; i < fence_pointers->size(); ++i) {
    // special case for matching at the last fence post
    if (i == fence_pointers->size() - 1) {
      if (key == fence_pointers->at(i)) {
        starting_point = i - 1;
        return starting_point;
      } else {
        return -1;
      }
    }

    if (key >= fence_pointers->at(i) && key < fence_pointers->at(i + 1)) {
      starting_point = i;
      return starting_point;
    }
  }

  return -1;  // return -1 if we hit a false positive with bloom filter.
}

// read certain bytes from the binary file storages.
// This decode is not right!!!!!! NEED TO fix here.
std::unique_ptr<Entry_t> Run::disk_search(int starting_point,
                                          size_t bytes_to_read, KEY_t key) {
  // std::lock_guard<std::mutex> guard(Run_mutex);

  auto entry = std::make_unique<Entry_t>();
  size_t read_size;

  std::ifstream file(file_location, std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file for reading");
  }

  // get file size
  file.seekg(0, std::ios::end);
  size_t fileSize = file.tellg();

  // modify read_size base on how much more we can read.
  if ((starting_point + 1) * LOAD_MEMORY_PAGE_SIZE > fileSize) {
    read_size = fileSize - starting_point * LOAD_MEMORY_PAGE_SIZE;
  } else {
    read_size = LOAD_MEMORY_PAGE_SIZE;
  }

  // read in del flags: starting * page_size -> begining of page
  file.seekg(starting_point * LOAD_MEMORY_PAGE_SIZE + read_size - BOOL_BYTE_CNT,
             std::ios::beg);
  uint64_t result;
  file.read(reinterpret_cast<char *>(&result), BOOL_BYTE_CNT);
  std::bitset<64> del_flag_bitset(result);
  file.seekg(0, std::ios::beg);  // reset file stream

  // shift file stream pointer to starting_point base on fence pointer result.
  file.seekg(starting_point * LOAD_MEMORY_PAGE_SIZE, std::ios::beg);

  int idx = 0;
  while (read_size >
         BOOL_BYTE_CNT) {  // change into binary search if i have time.
    file.read(reinterpret_cast<char *>(&entry->key), sizeof(entry->key));
    file.read(reinterpret_cast<char *>(&entry->val), sizeof(entry->val));

    if (key == entry->key) {
      entry->del = del_flag_bitset[63 - idx];
      file.close();

      return entry;
    }

    read_size = read_size - sizeof(entry->key) - sizeof(entry->val);
    idx++;
  }

  file.close();
  return nullptr;  // return null if we couldn't find the result.
}

// TODO: Need to implement range_disk_search after updating the file formatting.
// TODO: this function can definitely be reformatted. Probably implement a
// function called page search.
std::vector<Entry_t> Run::range_disk_search(KEY_t lower, KEY_t upper) {
  std::vector<Entry_t> ret;
  std::ifstream file(file_location, std::ios::binary);
  size_t read_size;
  auto entry = std::make_unique<Entry_t>();

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file for reading");
  }

  // get file size
  file.seekg(0, std::ios::end);
  size_t fileSize = file.tellg();

  // find starting positions.
  int starting_left = search_fence(lower);
  int starting_right = search_fence(upper);

  std::cout << "reading file: " << file_location
            << ". On fence location: " << starting_left << "->"
            << starting_right << std::endl;
  if (starting_left == -1 && starting_right == -1) {
    return ret;  // out of bound in this run.
  } else if (starting_right == -1) {
    std::cout << "accessing left to end" << std::endl;
    for (int i = starting_left; i < fence_pointers->size(); i++) {
      // modify read_size base on how much more we can read.
      if ((i + 1) * LOAD_MEMORY_PAGE_SIZE > fileSize) {
        read_size = fileSize - i * LOAD_MEMORY_PAGE_SIZE;
      } else {
        read_size = LOAD_MEMORY_PAGE_SIZE;
      }

      // read in del flags: starting * page_size -> begining of page
      file.seekg(i * LOAD_MEMORY_PAGE_SIZE + read_size - BOOL_BYTE_CNT,
                 std::ios::beg);
      uint64_t result;
      file.read(reinterpret_cast<char *>(&result), BOOL_BYTE_CNT);
      std::bitset<64> del_flag_bitset(result);
      file.seekg(0, std::ios::beg);

      file.seekg(i * LOAD_MEMORY_PAGE_SIZE, std::ios::beg);

      int idx = 0;
      while (read_size >
             BOOL_BYTE_CNT) {  // change into binary search if i have time.
        file.read(reinterpret_cast<char *>(&entry->key), sizeof(entry->key));
        file.read(reinterpret_cast<char *>(&entry->val), sizeof(entry->val));

        if (lower <= entry->key && upper >= entry->key) {
          entry->del = del_flag_bitset[63 - idx];
          ret.push_back(*entry);
        }

        read_size = read_size - sizeof(entry->key) - sizeof(entry->val);
        idx++;
      }
    }

  } else if (starting_left == -1) {
    std::cout << "accessing beginning to right" << std::endl;
    for (int i = 0; i <= starting_right; i++) {
      // modify read_size base on how much more we can read.
      if ((i + 1) * LOAD_MEMORY_PAGE_SIZE > fileSize) {
        read_size = fileSize - i * LOAD_MEMORY_PAGE_SIZE;
      } else {
        read_size = LOAD_MEMORY_PAGE_SIZE;
      }

      // read in del flags: starting * page_size -> begining of page
      file.seekg(i * LOAD_MEMORY_PAGE_SIZE + read_size - BOOL_BYTE_CNT,
                 std::ios::beg);
      uint64_t result;
      file.read(reinterpret_cast<char *>(&result), BOOL_BYTE_CNT);
      std::bitset<64> del_flag_bitset(result);
      file.seekg(0, std::ios::beg);

      file.seekg(i * LOAD_MEMORY_PAGE_SIZE, std::ios::beg);

      int idx = 0;
      while (read_size >
             BOOL_BYTE_CNT) {  // change into binary search if i have time.
        file.read(reinterpret_cast<char *>(&entry->key), sizeof(entry->key));
        file.read(reinterpret_cast<char *>(&entry->val), sizeof(entry->val));

        if (lower <= entry->key && upper >= entry->key) {
          entry->del = del_flag_bitset[63 - idx];
          ret.push_back(*entry);
        }

        read_size = read_size - sizeof(entry->key) - sizeof(entry->val);
        idx++;
      }
    }
  } else {
    std::cout << "accessing left to right" << std::endl;
    for (int i = starting_left; i <= starting_right; i++) {
      // modify read_size base on how much more we can read.
      if ((i + 1) * LOAD_MEMORY_PAGE_SIZE > fileSize) {
        read_size = fileSize - i * LOAD_MEMORY_PAGE_SIZE;
      } else {
        read_size = LOAD_MEMORY_PAGE_SIZE;
      }

      // read in del flags: starting * page_size -> begining of page
      file.seekg(i * LOAD_MEMORY_PAGE_SIZE + read_size - BOOL_BYTE_CNT,
                 std::ios::beg);
      uint64_t result;
      file.read(reinterpret_cast<char *>(&result), BOOL_BYTE_CNT);
      std::bitset<64> del_flag_bitset(result);
      file.seekg(0, std::ios::beg);

      file.seekg(i * LOAD_MEMORY_PAGE_SIZE, std::ios::beg);

      int idx = 0;
      while (read_size >
             BOOL_BYTE_CNT) {  // change into binary search if i have time.
        file.read(reinterpret_cast<char *>(&entry->key), sizeof(entry->key));
        file.read(reinterpret_cast<char *>(&entry->val), sizeof(entry->val));

        if (lower <= entry->key && upper >= entry->key) {
          entry->del = del_flag_bitset[63 - idx];
          ret.push_back(*entry);
        }

        read_size = read_size - sizeof(entry->key) - sizeof(entry->val);
        idx++;
      }
    }
  }
  file.close();
  return ret;
}

std::vector<KEY_t> Run::return_fence() { return *fence_pointers; }

BloomFilter Run::return_bloom() { return *bloom; }