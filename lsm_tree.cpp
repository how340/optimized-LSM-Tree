#include "lsm_tree.h"

LSM_Tree::LSM_Tree(size_t bits_ratio, size_t level_ratio, size_t buffer_size,
                   int mode)
    : bloom_bits_per_entry(bits_ratio),
      level_ratio(level_ratio),
      buffer_size(buffer_size),
      mode(mode) {
  in_mem = new BufferLevel(buffer_size);
  root = new Level_Node{0, level_ratio};
}

LSM_Tree::~LSM_Tree() {
  // TODO: for sure there is memory leak. need to properly delete.
  delete in_mem;  // Free the dynamically allocated in-memory buffer
                  // Consider implementing or calling a method here to clean up
                  // other dynamically allocated resources, if any probably want
                  // to delete the tree structure from root recursively.
}
/**
 * LSM_Tree
 *
 * @param  {KEY_t} key   :
 * @param  {VALUE_t} val :
 */
void LSM_Tree::put(KEY_t key, VALUE_t val) {
  int insert_result;
  std::vector<Entry_t> buffer;
  Level_Node *cur = root;

  insert_result = in_mem->insert(key, val);

  if (insert_result == -1) {
    buffer = LSM_Tree::merge(cur);
    Run merged_run = create_run(buffer);
    cur->run_storage.push_back(merged_run);

    // clear buffer and insert again.
    in_mem->clear_buffer();
    in_mem->insert(key, val);
  }
}

/**
 * LSM_Tree
 *
 * @param  {KEY_t} key                 :
 * @return {std::unique_ptr<Entry_t>}  :
 */
std::unique_ptr<Entry_t> LSM_Tree::get(KEY_t key) {
  std::unique_ptr<Entry_t> in_mem_result = in_mem->get(key);

  if (in_mem_result) {  // check memory buffer first.
    if (in_mem_result->del) {
      std::cout << "not found (deleted)" << std::endl;
      return nullptr;
    }
    std::cout << "found in memory: " << in_mem_result->val << std::endl;
    return in_mem_result;
  } else {
    // search in secondary storage.
    std::unique_ptr<Entry_t> entry = nullptr;
    Level_Node *cur = root;

    while (cur) {
      std::cout << "searching_level: " << cur->level << std::endl;

      // start search from the back of the run storage. The latest run contains
      // the most updated data.
      for (auto rit = cur->run_storage.rbegin(); rit != cur->run_storage.rend();
           ++rit) {
        if (rit->search_bloom(key)) {
          int starting_point = rit->search_fence(key);
          std::cout << "starting at: " << starting_point << std::endl;
          if (starting_point != -1) {
            entry =
                rit->disk_search(starting_point, SAVE_MEMORY_PAGE_SIZE, key);
            if (entry) {
              if (entry->del) {
                std::cout << "not found (deleted)" << std::endl;
                return nullptr;
              }
              std::cout << entry->val << std::endl;
              return entry;
            }
          }
        }
      }
      cur = cur->next_level;
    }
  }
  std::cout << "not found" << std::endl;
  return nullptr;
}

/**
 * LSM_Tree
 *  do range from top to bottom, and back to front. Keep a linked list of the results for later traverse. 
 * After searching in each run, create a vector of all values
 * within range. When consolidating the ranges, keep a hashmap, and go from top to bottom. 
 * @param  {KEY_t} lower           :
 * @param  {KEY_t} upper           :
 * @return {std::vector<Entry_t>}  :
 */
std::vector<Entry_t> LSM_Tree::range(KEY_t lower, KEY_t upper) {
  // Linked list to keep track of search results.
  struct Node {
    std::vector<Entry_t> data;
    Node *next = nullptr;
  };

  Node *ret_root = new Node;
  std::vector<Entry_t> ret;  // returned vector

  ret_root->data = in_mem->get_range(lower, upper);
  ret_root->next = new Node;
  Node *ret_cur = ret_root;

  Level_Node *cur = root;

  while (cur) {
    // iterate each level from back to front.
    for (auto rit = cur->run_storage.rbegin(); rit != cur->run_storage.rend();
         ++rit) {
      std::cout << "reading level: " << cur->level << std::endl; 
      ret_cur = ret_cur->next;
      ret_cur->data = rit->range_disk_search(lower, upper);
    }
    cur = cur->next_level; 
  }
  std::unordered_map<KEY_t, Entry_t> hash_mp;
  ret_cur = ret_root;  // reset current pointer;

  while (ret_cur) {
    for (Entry_t entry : ret_cur->data) {
      // Only update if we have not seen this key
      //  any repeating key down the tree are older values.
      if (hash_mp.find(entry.key) == hash_mp.end()) {
        hash_mp[entry.key] = entry;
      }
    }
    ret_cur = ret_cur->next;
  }
  for (const auto &pair : hash_mp) {
    ret.push_back(pair.second);
  }

  // need to delete the content of the linked list.
  ret_cur = ret_root; 
  while(ret_cur){
    Node* tmp = ret_cur;
    ret_cur = ret_cur->next; 
    delete tmp; 
  }

  std::cout << "Range search is complete" << std::endl; 
  return ret;
}
/**
 * LSM_Tree
 *
 * @param  {KEY_t} key :
 */
void LSM_Tree::del(KEY_t key) {
  int del_result;
  std::vector<Entry_t> buffer;
  Level_Node *cur = root;

  del_result = in_mem->del(key);

  if (del_result == -1)  // buffer is full.
  {
    buffer = LSM_Tree::merge(cur);
    Run merged_run = create_run(buffer);
    cur->run_storage.push_back(merged_run);
    // clear buffer and del again.
    in_mem->clear_buffer();
    in_mem->del(key);
  }
}

// merge policy for combining different
/**
 * LSM_Tree
 *
 * @param  {LSM_Tree::Level_Node*} cur :
 * @return {std::vector<Entry_t>}      :
 */
std::vector<Entry_t> LSM_Tree::merge(LSM_Tree::Level_Node *cur) {
  std::cout << "merging" << std::endl;
  // Insert a new run into storage.
  std::vector<Entry_t> buffer =
      in_mem->flush_buffer();  // temp buffer for merging.

  while (cur && cur->run_storage.size() == cur->max_num_of_runs) {
    if (!cur->next_level) {  // next level doesn't exist.
      cur->next_level = new Level_Node(cur->level + 1, cur->max_num_of_runs);
    }

    // naive full tiering merge policy.
    for (int i = 0; i < cur->run_storage.size(); i++) {
      std::ifstream in(cur->run_storage[i].get_file_location(),
                       std::ios::binary);
      if (!in.is_open())
        throw std::runtime_error("Unable to open file for writing");

      Entry_t entry;
      while (in.read(reinterpret_cast<char *>(&entry), sizeof(entry))) {
        buffer.push_back(entry);
      }

      in.close();
    }

    // delete current level info and files.
    for (int i = 0; i < cur->run_storage.size(); i++) {
      std::filesystem::path fileToDelete(
          cur->run_storage[i].get_file_location());
      std::filesystem::remove(fileToDelete);
    }
    cur->run_storage.clear();

    cur = cur->next_level;
  }

  // resolve updates and deletes
  std::unordered_map<KEY_t, Entry_t> hash_mp;
  std::vector<Entry_t> ret;

  for (int i = 0; i < buffer.size(); i++) {
    Entry_t temp_entry = buffer[i];
    if (temp_entry.del) {
      // delete item exists on same level, remove the item.
      if (hash_mp.find(temp_entry.key) != hash_mp.end()) {
        hash_mp.erase(temp_entry.key);
      } else {  // delete item not exists on same level. proprogate.
        hash_mp[temp_entry.key] = temp_entry;
      }
    } else {  // this operation will always use a later key/val to update output
              // value.
      hash_mp[temp_entry.key] = temp_entry;
    }
  }
  for (const auto &pair : hash_mp) {
    ret.push_back(pair.second);
  }

  std::sort(ret.begin(), ret.end());

  // TODO: I think I should pass by reference here. This buffer could be pretty
  // big.
  return ret;
}

// this function will save the in-memory data and maintain file structure for
// data persistence.
void LSM_Tree::exit_save() {
  // save in memory component to file
  //  write data structure to file.
  //  memory get its own designated file. named LSM_memory.dat. Don't need meta
  //  data. secondary memory is maintained by saving another meta data file for
  //  the file structure.
  exit_save_memory();
  level_meta_save();
}

// create a Run and associated file for a given vector of entries.
Run LSM_Tree::create_run(std::vector<Entry_t> buffer) {
  // create new bloom, new fence pointer, create new run in next level.
  std::string file_name = generateRandomString(6);

  BloomFilter *bloom = new BloomFilter(bloom_bits_per_entry * buffer.size());
  std::vector<KEY_t> *fence = new std::vector<KEY_t>;

  LSM_Tree::create_bloom_filter(bloom, buffer);
  LSM_Tree::save_to_memory(file_name, fence, buffer);

  Run run(file_name, bloom, fence);
  return run;
}

void LSM_Tree::create_bloom_filter(BloomFilter *bloom,
                                   const std::vector<Entry_t> &vec) {
  for (int i = 0; i < vec.size(); i++) {
    bloom->set(vec[i].key);
  }
};

/**
 * save_to_memory
 * The function stores an vector containing entries into a binary and modify the
 * fence pointers for the page.
 *
 * Special attention: The deletion status of each key/value pairs are packed
 * into the first few bits of the page. Current, each page is 512 bytes and can
 * hold 64 key/val pairs. To allow for the deletion feature. I am adding anthoer
 * 64 bits(8 bytes of data) into the end of a memory page, making each actual
 * memory page to be 520 bytes.
 * @param  {std::string} filename              : The file_location for the
 * stored binary file.
 * @param  {std::vector<KEY_t>*} fence_pointer : Pointer to a vector containing
 * the fence pointers
 * @param  {std::vector<Entry_t>} vec          : a vector containing entries.
 */
void LSM_Tree::save_to_memory(std::string filename,
                              std::vector<KEY_t> *fence_pointer,
                              std::vector<Entry_t> &vec) {
  // two pointers to keep track of memory and fence_pointer traversal.
  std::vector<int> bool_bits;
  int memory_cnt = 0, fence_pointer_index = 0;
  std::ofstream out(filename, std::ios::binary);

  if (!out.is_open()) {
    throw std::runtime_error("Unable to open file for writing");
  }

  for (const auto &entry : vec) {  // go through all entries.
    bool_bits.push_back(entry.del);

    if (memory_cnt == 0) {
      fence_pointer->push_back(entry.key);
      fence_pointer_index++;
    }
    out.write(reinterpret_cast<const char *>(&entry.key), sizeof(entry.key));
    out.write(reinterpret_cast<const char *>(&entry.val), sizeof(entry.val));

    memory_cnt = sizeof(entry.key) + sizeof(entry.val) + memory_cnt;

    // reached page maximum. Append the bool_bits to the back of the page.
    if (memory_cnt == SAVE_MEMORY_PAGE_SIZE) {
      memory_cnt = 0;
      uint64_t result = 0;

      if (bool_bits.size() != 64) {
        throw std::runtime_error("Vector must contain exactly 64 elements.");
      }

      for (size_t i = 0; i < 64; ++i) {
        if (bool_bits[i] != 0 && bool_bits[i] != 1) {
          throw std::runtime_error("Vector must contain only 0s and 1s.");
        }
        // Shift the bit to the correct position and set it in 'result'
        result |= static_cast<uint64_t>(bool_bits[i]) << (63 - i);
      }
      out.write(reinterpret_cast<const char *>(&result), sizeof(result));
      bool_bits.clear();
    }
  }

  fence_pointer->push_back(vec.back().key);

  if (bool_bits.size() > 0) {
    while (
        bool_bits.size() <
        64)  // if i were to alter page size, need to use a constant for this.
    {
      int padding = 0;
      bool_bits.push_back(padding);  // pad with zeros
    }

    uint64_t result = 0;
    for (size_t i = 0; i < 64; ++i) {
      if (bool_bits[i] != 0 && bool_bits[i] != 1) {
        throw std::runtime_error("Vector must contain only 0s and 1s.");
      }
      // Shift the bit to the correct position and set it in 'result'
      result |= static_cast<uint64_t>(bool_bits[i]) << (63 - i);
    }
    out.write(reinterpret_cast<const char *>(&result), sizeof(result));
    bool_bits.clear();
  }

  out.close();
  // std::cout << "Successfully written to file : " << filename << std::endl;
}

/**
 * LSM_Tree
 *
 * TODO: There is pretty big overlap between exit_save_memory() and
 * save_memory(). Explore simplification.
 */
void LSM_Tree::exit_save_memory() {
  std::vector<int> bool_bits;
  int memory_cnt = 0;
  std::string filename = "lsm_tree_memory.dat";
  std::vector<Entry_t> buffer = in_mem->flush_buffer();

  std::ofstream out(filename, std::ios::binary);

  if (!out.is_open()) {
    throw std::runtime_error("Unable to open file for writing");
  }

  for (const auto &entry : buffer) {  // go through all entries.
    bool_bits.push_back(entry.del);

    out.write(reinterpret_cast<const char *>(&entry.key), sizeof(entry.key));
    out.write(reinterpret_cast<const char *>(&entry.val), sizeof(entry.val));

    memory_cnt = sizeof(entry.key) + sizeof(entry.val) + memory_cnt;

    if (memory_cnt == SAVE_MEMORY_PAGE_SIZE) {
      memory_cnt = 0;
      uint64_t result = 0;

      if (bool_bits.size() != 64) {
        throw std::runtime_error("Vector must contain exactly 64 elements.");
      }

      for (size_t i = 0; i < 64; ++i) {
        if (bool_bits[i] != 0 && bool_bits[i] != 1) {
          throw std::runtime_error("Vector must contain only 0s and 1s.");
        }
        // Shift the bit to the correct position and set it in 'result'
        result |= static_cast<uint64_t>(bool_bits[i]) << (63 - i);
      }
      out.write(reinterpret_cast<const char *>(&result), sizeof(result));
      bool_bits.clear();
    }
  }

  if (bool_bits.size() > 0) {
    while (bool_bits.size() < 64) {
      int padding = 0;
      bool_bits.push_back(padding);  // pad with zeros
    }
    uint64_t result = 0;
    for (size_t i = 0; i < 64; ++i) {
      if (bool_bits[i] != 0 && bool_bits[i] != 1) {
        throw std::runtime_error("Vector must contain only 0s and 1s.");
      }
      // Shift the bit to the correct position and set it in 'result'
      result |= static_cast<uint64_t>(bool_bits[i]) << (63 - i);
    }
    out.write(reinterpret_cast<const char *>(&result), sizeof(result));
    bool_bits.clear();
  }

  out.close();
}

void LSM_Tree::level_meta_save() {
  // storing each of the secondary storage related data in different files.
  // bloom and fence pointer binary size are stored as txt in the meta data
  // file. easier to load data on boot.
  std::string meta_data = "lsm_tree_level_meta.txt";
  std::ofstream meta(meta_data);

  if (!meta.is_open()) {
    std::cerr << "Failed to open the files." << std::endl;
  }

  Level_Node *cur = root;

  // write meta data to the LSM tree first.
  meta << bloom_bits_per_entry << " " << level_ratio << " " << buffer_size
       << " " << mode << "\n";

  while (cur) {
    std::cout << cur->level << ": ";
    // for each level: write current level, level limit, run count, then
    // specific data for each run.
    meta << cur->level << " " << cur->max_num_of_runs << " "
         << cur->run_storage.size() << "\n";

    // for each run: write filename, bloom size and fence size.
    for (int i = 0; i < cur->run_storage.size(); i++) {
      std::cout << cur->run_storage[i].get_file_location() << ", ";

      // write bloom filter to binary file. TODO: debug this. maybe the spacing
      // of things could be wrong.
      BloomFilter temp_bloom = cur->run_storage[i].return_bloom();
      boost::dynamic_bitset<> temp_bitarray = temp_bloom.return_bitarray();

      std::string bloom_filename =
          "bloom_" + cur->run_storage[i].get_file_location();
      std::ofstream bloom(bloom_filename);
      bloom << temp_bitarray;  // Write the bitset to the file
      bloom.close();

      // Write fence pointers to binary file
      std::vector<KEY_t> temp_fence = cur->run_storage[i].return_fence();

      std::string fence_filename =
          "fence_" + cur->run_storage[i].get_file_location();
      std::ofstream fence(fence_filename);
      fence.write(reinterpret_cast<const char *>(&temp_fence[0]),
                  temp_fence.size() * sizeof(KEY_t));
      fence.close();

      meta << cur->run_storage[i].get_file_location() << std::endl;
    }

    std::cout << "\n----------------------------" << std::endl;
    cur = cur->next_level;
  }

  meta.close();
}

// helper function for generating a file_name for on-disk storage file name.
std::string LSM_Tree::generateRandomString(size_t length) {
  const std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
  std::string randomString;
  auto seed =
      std::chrono::high_resolution_clock::now().time_since_epoch().count();
  std::mt19937 generator(static_cast<unsigned int>(seed));
  std::uniform_int_distribution<> distribution(0, chars.size() - 1);

  for (size_t i = 0; i < length; ++i) {
    randomString += chars[distribution(generator)];
  }

  return "lsm_tree_" + randomString + ".dat";
}

void LSM_Tree::print() {
  Level_Node *cur = root;

  while (cur) {
    std::cout << cur->level << ": " << std::endl;
    for (int i = 0; i < cur->run_storage.size() - 1; i++) {
      std::cout << cur->run_storage[i].get_file_location() << " ";
    }
    std::cout
        << cur->run_storage[cur->run_storage.size() - 1].get_file_location()
        << std::endl;
    cur = cur->next_level;
  }
}