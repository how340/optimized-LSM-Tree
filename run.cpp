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
      std::cout << "last post";
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
  file.seekg(0, std::ios::beg);// reset file stream

  // shift file stream pointer to starting_point base on fence pointer result.
  file.seekg(starting_point * LOAD_MEMORY_PAGE_SIZE, std::ios::beg);

  int idx = 0;
  while (read_size > BOOL_BYTE_CNT) {
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
std::vector<Entry_t> Run::range_disk_search() { return std::vector<Entry_t>(); }

std::vector<KEY_t> Run::return_fence() { return *fence_pointers; }

BloomFilter Run::return_bloom() { return *bloom; }