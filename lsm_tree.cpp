#include "lsm_tree.h"

LSM_Tree::LSM_Tree(float bits_ratio,
                   size_t level_ratio,
                   size_t buffer_size,
                   int mode,
                   size_t threads)
    : bloom_bits_per_entry(bits_ratio),
      level_ratio(level_ratio),
      buffer_size(buffer_size),
      mode(mode),
      pool(threads) {
  in_mem = new BufferLevel(buffer_size);
  root = new Level_Node{0, level_ratio};
  if (mode == 1) {
    level_root = new Leveling_Node;

    level_root->level = lazy_cut_off;
    level_root->leveled_run =
        new Level_Run(pool, 10, lazy_cut_off, level_ratio, buffer_size);
  }
  num_of_threads = threads;
}

LSM_Tree::~LSM_Tree() {
  // TODO: for sure there is memory leak. need to properly delete.
  delete in_mem;  // Free the dynamically allocated in-memory buffer
                  // Consider implementing or calling a method here to clean up
                  // other dynamically allocated resources, if any probably want
                  // to delete the tree structure from root recursively.

  // std::cout << "LSM fail";
  // Level_Node* temp;
  // while (root){
  //     temp = root;
  //     root = root->next_level;
  //     delete temp;
  // }
  // root = nullptr;
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
  Level_Node* cur = root;

  insert_result = in_mem->insert(key, val);

  if (insert_result == -1) {
    merge_policy();

    // clear buffer and insert again.
    in_mem->clear_buffer();
    in_mem->insert(key, val);
  }

  total++;
  if (total % 100000 == 0) {
    std::cout << "inserted: " << total << std::endl;
  }
}

// overload for loading memory on boot.
void LSM_Tree::put(Entry_t entry) {
  int insert_result;
  std::vector<Entry_t> buffer;
  Level_Node* cur = root;

  insert_result = in_mem->insert(entry);
}

/**
 * LSM_Tree
 *
 * @param  {KEY_t} key                 :
 * @return {std::unique_ptr<Entry_t>}  :
 */
std::unique_ptr<Entry_t> LSM_Tree::get(KEY_t key) {
  std::unique_ptr<Entry_t> in_mem_result = in_mem->get(key);
  Level_Node* cur = root; 
  if (in_mem_result) {  // check memory buffer first.
    return in_mem_result;
  }

  while (cur) {
    // std::cout << "searching_level: " << cur->level << std::endl;

    std::vector<std::future<std::unique_ptr<Entry_t>>> futures;
    // start search from the back of the run storage. The latest run contains
    // the most updated data.
    int cnt = 0;
    for (auto rit = cur->run_storage.rbegin(); rit != cur->run_storage.rend();
         ++rit) {
      futures.push_back(pool.enqueue(
          [=]() -> std::unique_ptr<Entry> { return process_run(rit, key); }));

      // sync up after each batch of threads finish task to prevent unecessary
      // future searches.
      if ((cnt + 1) % num_of_threads == 0) {
        for (auto& fut : futures) {
          auto result = fut.get();
          if (result) {
            return result;
          }
        }
        futures.clear();
      }
    }
    // check futures for any remaining queued results.
    for (auto& fut : futures) {
      auto result = fut.get();
      if (result) {
        return result;
      }
    }
    futures.clear();

    cur = cur->next_level;
  }

  if (mode == 1) {
    // search in secondary storage.
    Leveling_Node* level_cur = level_root;
    std::cout << "running this " << std::endl;

    // go down the leveling levels. This will not be triggered if mode is 0.
    while (level_cur) {
      std::unique_ptr<Entry> entry = level_cur->leveled_run->get(key);
      if (entry) {
        return entry;
      }
      level_cur = level_cur ->next_level; 
    }
  }

  return nullptr;
}

std::unique_ptr<Entry_t> LSM_Tree::process_run(
    typename std::vector<Run>::reverse_iterator rit,
    const KEY_t& key) {
  std::unique_ptr<Entry_t> entry = nullptr;

  if (rit->search_bloom(key)) {
    int starting_point = rit->search_fence(key);
    if (starting_point != -1) {
      entry = rit->disk_search(starting_point, SAVE_MEMORY_PAGE_SIZE, key);
      if (entry) {
        if (entry->del) {
          return nullptr;
        }
        return entry;
      }
    }
  }

  return nullptr;
}

/**
 * LSM_Tree
 *  do range from top to bottom, and back to front. Keep a linked list of the
 * results for later traverse. After searching in each run, create a vector of
 * all values within range. When consolidating the ranges, keep a hashmap, and
 * go from top to bottom.
 * @param  {KEY_t} lower           :
 * @param  {KEY_t} upper           :
 * @return {std::vector<Entry_t>}  :
 */
std::vector<Entry_t> LSM_Tree::range(KEY_t lower, KEY_t upper) {
  // for storing All of the available sub-buffers created.

  std::vector<std::future<std::unordered_map<KEY_t, Entry_t>>> futures;
  std::vector<Entry_t> buffer =
      in_mem->get_range(lower, upper);  // returned vector

  std::unordered_map<KEY_t, Entry_t> hash_mp;
  std::vector<Entry_t> ret;
  // add buffer to hashmap first.
  for (auto& entry : buffer) {
    hash_mp[entry.key] = entry;
  }

  Level_Node* cur = root;

  while (cur) {
    // iterate each level from back to front.
    for (auto rit = cur->run_storage.rbegin(); rit != cur->run_storage.rend();
         ++rit) {
      // ret_cur = ret_cur->next;
      futures.push_back(
          pool.enqueue([=]() -> std::unordered_map<KEY_t, Entry_t> {
            std::vector<Entry_t> temp_vec =
                rit->range_disk_search(lower, upper);
            std::unordered_map<KEY_t, Entry_t> temp_mp;
            for (auto& entry : temp_vec) {
              auto it = temp_mp.find(entry.key);
              if (it == temp_mp.end()) {
                temp_mp[entry.key] = entry;
              }
            }
            return temp_mp;
          }));
    }
    cur = cur->next_level;
  }
  for (auto& fut : futures) {
    std::unordered_map<KEY_t, Entry_t> results = fut.get();
    // the merge method disgards repeating key from the merged map.
    hash_mp.merge(results);
  }
  Leveling_Node* level_cur = level_root;
  while (level_cur) {
    std::unordered_map<KEY_t, Entry_t> tmp =
        level_cur->leveled_run->range_search(lower, upper);
    hash_mp.merge(tmp);
  }

  futures.clear();
  for (const auto& pair : hash_mp) {
    ret.push_back(pair.second);
  }

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
  Level_Node* cur = root;

  del_result = in_mem->del(key);

  if (del_result == -1)  // buffer is full.
  {
    merge_policy();
    // clear buffer and del again.
    in_mem->clear_buffer();
    in_mem->del(key);
  }
}

/**
 * LSM_Tree
 * This function manages how to use the two available merge policies depending
 * on operating mode.
 * @param  {LSM_Tree::Level_Node*} cur :
 * @return {std::vector<Entry_t>}      :
 */
void LSM_Tree::merge_policy() {
  std::vector<Entry_t> buffer = in_mem->flush_buffer();
  std::unordered_map<KEY_t, Entry_t> merge_map;
  // put in_mem content into the buffer.
  for (auto& entry : buffer) {
    merge_map[entry.key] = entry;
  }
  /************************************************************
   *                Unoptimized mode
   *************************************************************/
  if (mode == 0) {
    Level_Node* cur = root;
    std::set<int> level_to_delete;

    // flush down levels iteratively.
    while (cur && cur->run_storage.size() == cur->max_num_of_runs - 1) {
      std::unordered_map<KEY_t, Entry_t> tmp = merge(cur);
      merge_map.merge(tmp);

      if (!cur->next_level) {  // add a tiered level if we don't have a
                               // following level.
        cur->next_level = new Level_Node(cur->level + 1, cur->max_num_of_runs);
        total_levels++;
      }

      // flag current level for delete.
      level_to_delete.emplace(cur->level);
      cur = cur->next_level;
    }

    // convert hash_mp back to vector form.
    std::vector<Entry_t> merge_buffer;
    for (const auto& pair : merge_map) {
      merge_buffer.push_back(pair.second);
    }
    std::sort(merge_buffer.begin(), merge_buffer.end());
    // push into the new level.
    Run merged_run = create_run(merge_buffer, cur->level);
    cur->run_storage.push_back(merged_run);
    // remove merged level's in-memory representation and disk files.
    Level_Node* del_cur = root;
    while (level_to_delete.size() > 0) {
      if (level_to_delete.find(del_cur->level) != level_to_delete.end()) {
        for (int i = 0; i < del_cur->run_storage.size(); i++) {
          std::filesystem::path fileToDelete(
              del_cur->run_storage[i].get_file_location());
          std::filesystem::remove(fileToDelete);
        }
        del_cur->run_storage.clear();
        level_to_delete.erase(del_cur->level);
      }
      del_cur = del_cur->next_level;
    }
  } else if (mode == 1) {
    /************************************************************
     *                Optimized mode
     *************************************************************/
    Level_Node* cur = root;
    Leveling_Node* level_cur = level_root;
    std::set<int> level_to_delete;
    int max_level =
        0;  // this is counter for whether we want to use leveling levels.

    // flush down levels iteratively.
    while (cur && cur->run_storage.size() == cur->max_num_of_runs - 1) {
      std::unordered_map<KEY_t, Entry_t> tmp = merge(cur);
      merge_map.merge(tmp);

      // We add levels as usual, but we won't add new level if we are just below
      // the cut off.
      if (!cur->next_level && cur->level != lazy_cut_off - 1) {
        cur->next_level = new Level_Node(cur->level + 1, cur->max_num_of_runs);
        total_levels++;
      }

      // flag current level for delete.
      level_to_delete.emplace(cur->level);
      cur = cur->next_level;
      max_level++;
    }

    // cur is always nullptr here.
    if (max_level == lazy_cut_off) {
      std::cout << "going here" << std::endl;
      while (level_cur &&
             level_cur->leveled_run->return_size() >
                 level_cur->leveled_run->return_max_size() * 2 / 3 - 1) {
        // create new level if next level doesn't exist.
        if (!level_cur->next_level) {
          level_cur->next_level = new Leveling_Node;
          level_cur->next_level->level = level_cur->level + 1;
          level_cur->next_level->leveled_run =
              new Level_Run(pool, 10, lazy_cut_off, level_ratio, buffer_size);
        }
        std::cout << "segment 1 " << std::endl;
        std::unordered_map<KEY_t, Entry_t> tmp = partial_merge(level_cur);
        merge_map.merge(tmp);

        level_cur = level_cur->next_level;
      }
    }
    std::cout << "segment 2 " << std::endl;
    // convert hash_mp back to vector form.
    std::vector<Entry_t> merge_buffer;
    for (const auto& pair : merge_map) {
      merge_buffer.push_back(pair.second);
    }
    std::sort(merge_buffer.begin(), merge_buffer.end());

    // push the merged vector into different levels depend on whether the
    // leveling level is needed.
    if (max_level == lazy_cut_off) {
      level_cur->leveled_run->insert_block(merge_buffer);
    } else {
      Run merged_run = create_run(merge_buffer, cur->level);
      cur->run_storage.push_back(merged_run);
    }
    std::cout << "segment 3" << std::endl;
    // Delete tiered node after we are done.
    Level_Node* del_cur = root;
    while (level_to_delete.size() > 0) {
      if (level_to_delete.find(del_cur->level) != level_to_delete.end()) {
        for (int i = 0; i < del_cur->run_storage.size(); i++) {
          std::filesystem::path fileToDelete(
              del_cur->run_storage[i].get_file_location());
          std::filesystem::remove(fileToDelete);
        }
        del_cur->run_storage.clear();
        level_to_delete.erase(del_cur->level);
      }
      del_cur = del_cur->next_level;
    }

  } else {
    std::cout << "wrong mode input" << std::endl;
  }
}
/**
 * LSM_Tree merge - used for tiered levels.
 *  The function will collect the information in a level and bring it into
 * memory for flushing down to the next tiered level.
 * @param  {LSM_Tree::Level_Node*} cur :
 * @return {std::vector<Entry_t>}      :
 */
std::unordered_map<KEY_t, Entry_t> LSM_Tree::merge(LSM_Tree::Level_Node*& cur) {
  // for storing All of the available sub-buffers created.
  std::vector<std::future<std::unordered_map<KEY_t, Entry_t>>> futures;
  std::unordered_set<size_t> level_to_delete;

  // naive full tiering merge policy.
  for (auto rit = cur->run_storage.rbegin(); rit != cur->run_storage.rend();
       ++rit) {
    futures.push_back(pool.enqueue([=]() -> std::unordered_map<KEY_t, Entry_t> {
      std::vector<Entry_t> temp_vec =
          load_full_file(rit->get_file_location(), rit->return_fence());
      std::unordered_map<KEY_t, Entry_t> temp_mp;
      for (auto& entry : temp_vec) {
        auto it = temp_mp.find(entry.key);
        if (it ==
            temp_mp.end()) {  // any entries that come later are older, so we
                              // can just ignore. the caveat of this approach
                              // is that the deletes will always sink to the
                              // bottom of the tree. Additional code is needed
                              // to address this.
          temp_mp[entry.key] = entry;
        }
      }
      return temp_mp;
    }));
  }

  // reconstruct a full buffer and resolve updates/ deletes
  std::unordered_map<KEY_t, Entry_t> hash_mp;

  auto start0 = std::chrono::high_resolution_clock::now();

  for (auto& fut : futures) {
    std::unordered_map<KEY_t, Entry_t> results = fut.get();
    // the merge method disgards repeating key from the merged map.
    hash_mp.merge(results);
  }

  futures.clear();

  auto stop0 = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> update_duration = stop0 - start0;
  merge_update_accumulated_time +=
      update_duration;  // TODO: add mutex for tracking.

  return hash_mp;
}

// implement a lazy merge approach for the optimized tree
std::unordered_map<KEY_t, Entry_t> LSM_Tree::partial_merge(
    LSM_Tree::Leveling_Node*& cur) {
  std::unordered_map<KEY_t, Entry_t> ret = cur->leveled_run->flush();

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
Run LSM_Tree::create_run(std::vector<Entry_t> buffer, int current_level) {
  std::string file_name = generateRandomString(6);
  float bloom_bits;
  float cur_FPR;
  /*Base on MONKEY, total_bits = -entries*ln(FPR)/(ln(2)^2)*/
  if (mode == 1)  // optimized version is activated by mode.
  {
    cur_FPR = bloom_bits_per_entry * pow(level_ratio, current_level);
    bloom_bits =
        ceil(-(log(cur_FPR) / (pow(log(2), 2))));  // take ceiling to be safe
  } else {
    bloom_bits = bloom_bits_per_entry * buffer.size();
  }

  BloomFilter* bloom = new BloomFilter(bloom_bits);
  std::vector<KEY_t>* fence = new std::vector<KEY_t>;

  LSM_Tree::create_bloom_filter(bloom, buffer);
  LSM_Tree::save_to_memory(file_name, fence, buffer);

  Run run(file_name, bloom, fence);
  run.set_current_level(current_level);
  return run;
}

void LSM_Tree::create_bloom_filter(BloomFilter* bloom,
                                   const std::vector<Entry_t>& vec) {
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
                              std::vector<KEY_t>* fence_pointer,
                              std::vector<Entry_t>& vec) {
  // two pointers to keep track of memory and fence_pointer traversal.
  std::vector<int> bool_bits;
  int memory_cnt = 0, fence_pointer_index = 0;
  std::ofstream out(filename, std::ios::binary);

  if (!out.is_open()) {
    throw std::runtime_error("Unable to open file for writing");
  }

  for (const auto& entry : vec) {  // go through all entries.
    if (entry.del) {
      bool_bits.push_back(1);
    } else {
      bool_bits.push_back(0);
    }

    if (memory_cnt == 0) {
      fence_pointer->push_back(entry.key);
      fence_pointer_index++;
    }
    out.write(reinterpret_cast<const char*>(&entry.key), sizeof(entry.key));
    out.write(reinterpret_cast<const char*>(&entry.val), sizeof(entry.val));

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
          for (int value : bool_bits)  // for debugging.
          {
            std::cout << value << std::endl;
          }
          throw std::runtime_error("Vector must contain only 0s and 1s.");
        }
        // Shift the bit to the correct position and set it in 'result'
        result |= static_cast<uint64_t>(bool_bits[i]) << (63 - i);
      }
      out.write(reinterpret_cast<const char*>(&result), sizeof(result));
      bool_bits.clear();
    }
  }

  fence_pointer->push_back(vec.back().key);

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
    out.write(reinterpret_cast<const char*>(&result), sizeof(result));
    bool_bits.clear();
  }

  out.close();
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
    throw std::runtime_error("Unable to open file for writing on exit save");
  }

  for (const auto& entry : buffer) {  // go through all entries.
    bool_bits.push_back(entry.del);

    out.write(reinterpret_cast<const char*>(&entry.key), sizeof(entry.key));
    out.write(reinterpret_cast<const char*>(&entry.val), sizeof(entry.val));

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
      out.write(reinterpret_cast<const char*>(&result), sizeof(result));
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
    out.write(reinterpret_cast<const char*>(&result), sizeof(result));
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

  Level_Node* cur = root;

  // write meta data to the LSM tree first.
  meta << bloom_bits_per_entry << " " << level_ratio << " " << buffer_size
       << " " << mode << " " << num_of_threads << "\n";

  while (cur) {
    // for each level: write current level, level limit, run count, then
    // specific data for each run.
    meta << cur->level << " " << cur->max_num_of_runs << " "
         << cur->run_storage.size() << "\n";

    // for each run: write filename, bloom size and fence size.
    for (int i = 0; i < cur->run_storage.size(); i++) {
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
      fence.write(reinterpret_cast<const char*>(&temp_fence[0]),
                  temp_fence.size() * sizeof(KEY_t));
      fence.close();

      meta << cur->run_storage[i].get_file_location() << std::endl;
    }

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

void LSM_Tree::print_statistics() {
  // PUT operation
  std::cout << "PUT time: " << accumulated_time.count() * 1000 << "ms"
            << std::endl;
  std::cout << "Merge time: " << merge_accumulated_time.count() * 1000 << "ms"
            << std::endl;
  std::cout << "Merge update time: "
            << merge_update_accumulated_time.count() * 1000 << "ms"
            << std::endl;
  std::cout << "Merge delete time: "
            << merge_del_accumulated_time.count() * 1000 << "ms" << std::endl;

  // GET operation
  std::cout << "GET time: " << get_accumulated_time.count() * 1000 << "ms"
            << std::endl;
  std::cout << "GET disk time: " << get_disk_accumulated_time.count() * 1000
            << "ms" << std::endl;
  std::cout << "GET FP hit: " << FP_hits << std::endl;
  // Range operation

  // delete is essentially the same as get.
}

void LSM_Tree::load_memory() {
  std::string memory_data = "lsm_tree_memory.dat";
  std::ifstream memory(memory_data, std::ios::binary);
  if (!memory.is_open()) {
    std::cerr << "Failed to open the memory file." << std::endl;
  }
  Entry_t entry;

  // get file size
  memory.seekg(0, std::ios::end);
  size_t fileSize = memory.tellg();
  int total_page = fileSize / LOAD_MEMORY_PAGE_SIZE + 1;
  int cur_page = 0;
  int read_size;
  // modify read_size base on how much more we can read.
  while (cur_page < total_page) {
    if ((cur_page + 1) * LOAD_MEMORY_PAGE_SIZE > fileSize) {
      read_size = fileSize - cur_page * LOAD_MEMORY_PAGE_SIZE;
    } else {
      read_size = LOAD_MEMORY_PAGE_SIZE;
    }

    // read in del flags: starting * page_size -> begining of page
    memory.seekg(cur_page * LOAD_MEMORY_PAGE_SIZE + read_size - BOOL_BYTE_CNT,
                 std::ios::beg);
    uint64_t result;
    memory.read(reinterpret_cast<char*>(&result), BOOL_BYTE_CNT);
    std::bitset<64> del_flag_bitset(result);
    memory.seekg(0, std::ios::beg);  // reset file stream

    // shift file stream pointer to starting_point base on fence pointer result.
    memory.seekg(cur_page * LOAD_MEMORY_PAGE_SIZE, std::ios::beg);

    int idx = 0;
    while (read_size >
           BOOL_BYTE_CNT) {  // change into binary search if i have time.
      memory.read(reinterpret_cast<char*>(&entry.key), sizeof(entry.key));
      memory.read(reinterpret_cast<char*>(&entry.val), sizeof(entry.val));

      entry.del = del_flag_bitset[63 - idx];
      put(entry);
      read_size = read_size - sizeof(entry.key) - sizeof(entry.val);
      idx++;
    }
    cur_page++;
  }

  memory.close();
}

void LSM_Tree::reconstruct_file_structure(std::ifstream& meta) {
  std::string line;
  Level_Node* cur = root;

  while (std::getline(meta, line)) {
    std::cout << line << std::endl;
    std::istringstream iss(line);

    if (std::isdigit(line[0])) {
      std::string level, max_run, run_cnt;
      if (!(iss >> level >> max_run >> run_cnt)) {
        break;  // issue here.
      };

      if (level != "0") {
        cur->next_level =
            new LSM_Tree::Level_Node(cur->level + 1, cur->max_num_of_runs);
        cur = cur->next_level;
      }
    } else {
      std::string filename;
      iss >> filename;

      std::ifstream bloom("bloom_" + filename),
          fence("fence_" + filename, std::ios::binary);

      boost::dynamic_bitset<> input_bitarray;
      bloom >> input_bitarray;

      BloomFilter* bloom_filter = new BloomFilter(input_bitarray);

      std::vector<KEY_t>* fence_pointer = new std::vector<KEY_t>;
      KEY_t key;
      while (fence.read(reinterpret_cast<char*>(&key), sizeof(KEY_t))) {
        fence_pointer->push_back(key);
      }

      Run run(filename, bloom_filter, fence_pointer);
      cur->run_storage.push_back(run);
      fence.close();
      bloom.close();
    }
  }
  std::cout << "meta load complete!" << std::endl;
}

std::vector<Entry_t> LSM_Tree::load_full_file(
    std::string file_location,
    std::vector<KEY_t> fence_pointers) {
  // read-in the the oldest run at the level.
  std::ifstream file(file_location, std::ios::binary);
  if (!file.is_open())
    throw std::runtime_error("Unable to open file for loading");

  Entry_t entry;
  std::vector<Entry_t> buffer;

  file.seekg(0, std::ios::end);
  size_t fileSize = file.tellg();
  size_t read_size;
  // read in the run's information.
  for (int i = 0; i < fence_pointers.size(); i++) {
    if (i * LOAD_MEMORY_PAGE_SIZE > fileSize) {
      read_size = fileSize - (i - 1) * LOAD_MEMORY_PAGE_SIZE;
    } else {
      read_size = LOAD_MEMORY_PAGE_SIZE;
    }

    // read in del flags: starting * page_size -> begining of page
    uint64_t result;
    file.seekg(i * LOAD_MEMORY_PAGE_SIZE + read_size - BOOL_BYTE_CNT,
               std::ios::beg);
    file.read(reinterpret_cast<char*>(&result), BOOL_BYTE_CNT);
    std::bitset<64> del_flag_bitset(result);
    file.seekg(0, std::ios::beg);

    file.seekg(i * LOAD_MEMORY_PAGE_SIZE, std::ios::beg);

    int idx = 0;
    while (read_size > BOOL_BYTE_CNT) {
      file.read(reinterpret_cast<char*>(&entry.key), sizeof(entry.key));
      file.read(reinterpret_cast<char*>(&entry.val), sizeof(entry.val));

      entry.del = del_flag_bitset[63 - idx];
      buffer.push_back(entry);

      read_size = read_size - sizeof(entry.key) - sizeof(entry.val);
      idx++;
    }
  }
  file.close();
  return buffer;
}

void LSM_Tree::print() {
  Level_Node* cur = root;

  while (cur) {
    std::cout << cur->level << ": " << std::endl;
    for (int i = 0; i < cur->run_storage.size(); i++) {
      std::cout << cur->run_storage[i].get_file_location() << " "
                << cur->run_storage[i].return_current_level() << std::endl;
    }
    std::cout << " -> end level." << std::endl;

    cur = cur->next_level;
  }

  Leveling_Node* level_cur = level_root;
  std::cout << "Leveling levels:" << std::endl;
  while (level_cur) {
    std::cout << level_cur->level << ": " << std::endl;
    auto level_node_cur = level_cur->leveled_run;
    level_node_cur->print();
    std::cout << " -> end level." << std::endl;

    level_cur = level_cur->next_level;
  }
}