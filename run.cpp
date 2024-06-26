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
Run::Run(std::string file_name,
         BloomFilter* bloom_filter,
         std::vector<KEY_t>* fence) {
  file_location = file_name;
  bloom = bloom_filter;
  fence_pointers = fence;
}

Run::Run() {
  delete bloom;
  bloom = nullptr;
  delete fence_pointers;
  fence_pointers = nullptr;
}

bool Run::search_bloom(KEY_t key) {
  return bloom->is_set(key);
}

std::string Run::get_file_location() {
  return file_location;
}

int Run::search_fence(KEY_t key) {
  int starting_point;

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
std::unique_ptr<Entry_t> Run::disk_search(int starting_point,
                                          size_t bytes_to_read,
                                          KEY_t key) {
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
  file.read(reinterpret_cast<char*>(&result), BOOL_BYTE_CNT);
  std::bitset<64> del_flag_bitset(result);
  file.seekg(0, std::ios::beg);  // reset file stream

  // shift file stream pointer to starting_point base on fence pointer result.
  file.seekg(starting_point * LOAD_MEMORY_PAGE_SIZE, std::ios::beg);

  int idx = 0;
  while (read_size >
         BOOL_BYTE_CNT) {  // change into binary search if i have time.
    file.read(reinterpret_cast<char*>(&entry->key), sizeof(entry->key));
    file.read(reinterpret_cast<char*>(&entry->val), sizeof(entry->val));
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

// function called page search.
std::vector<Entry_t> Run::range_disk_search(KEY_t lower, KEY_t upper) {
  std::vector<Entry_t> ret;
  std::ifstream file(file_location, std::ios::binary);
  size_t read_size;
  auto entry = std::make_unique<Entry_t>();

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file for reading");
  }
  file.seekg(0, std::ios::end);
  size_t file_size = file.tellg();
  file.seekg(0, std::ios::beg);
  // find starting positions on which pages to look at.
  int starting_left = search_fence(lower);
  int starting_right = search_fence(upper);
  if (starting_left == -1 && starting_right == -1) {
    if (lower < fence_pointers->at(0) && upper > fence_pointers->back()) {
      starting_left = 0;
      starting_right = fence_pointers->size() - 1;
    } else {
      return ret;
    }
  } else if (starting_right == -1) {
    starting_right = fence_pointers->back();
  } else if (starting_left == -1) {
    starting_left = 0;
  }
  std::vector<char> file_data(file_size);

  if (!file.read(file_data.data(), file_size)) {
    std::cerr << "Error reading file.\n";
  }

  // read in the run's information.
  for (int i = starting_left; i < starting_right; i++) {
    size_t start_index = i * LOAD_MEMORY_PAGE_SIZE;
    if (start_index >= file_size)
      break;  // Ensure we do not start beyond the file size

    size_t end_index = (i + 1) * LOAD_MEMORY_PAGE_SIZE;
    read_size = std::min(end_index, file_size) -
                start_index;  // Ensures read_size is within bounds

    // read in del flags: starting * page_size -> beginning of page
    uint64_t result;
    if (read_size >= BOOL_BYTE_CNT) {
      std::memcpy(&result, &file_data[start_index + read_size - BOOL_BYTE_CNT],
                  sizeof(result));
    } else {
      result = 0;  // Not enough data to read BOOL_BYTE_CNT bytes
    }
    std::bitset<64> del_flag_bitset(result);

    // Need to figure out how to part this in one go.

    int idx = (i - 1) * LOAD_MEMORY_PAGE_SIZE;
    int cnt = 0;
    while (idx < read_size - BOOL_BYTE_CNT) {
      Entry_t entry;
      std::memcpy(&entry.key, &file_data[idx], sizeof(KEY_t));
      idx += sizeof(KEY_t);
      std::memcpy(&entry.val, &file_data[idx], sizeof(VALUE_t));
      idx += sizeof(KEY_t);
      entry.del = del_flag_bitset[63 - cnt];
      cnt++;
      if (!entry.del && entry.key >= lower && entry.key <= upper) {
        ret.push_back(entry);
      }
    }
  }

  file.close();
  return ret;
}

std::vector<KEY_t> Run::return_fence() {
  return *fence_pointers;
}

BloomFilter Run::return_bloom() {
  return *bloom;
}