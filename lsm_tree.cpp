#include "lsm_tree.h"

LSM_Tree::LSM_Tree(float bits_ratio, size_t level_ratio, size_t buffer_size, int mode, size_t threads)
    : bloom_bits_per_entry(bits_ratio), level_ratio(level_ratio), buffer_size(buffer_size), mode(mode), pool(threads)
{
    in_mem = new BufferLevel(buffer_size);
    root = new Level_Node{0, level_ratio};
    num_of_threads = threads;
}

LSM_Tree::~LSM_Tree()
{
    // TODO: for sure there is memory leak. need to properly delete.
    delete in_mem; // Free the dynamically allocated in-memory buffer
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
void LSM_Tree::put(KEY_t key, VALUE_t val)
{
    auto start = std::chrono::high_resolution_clock::now();
    int insert_result;
    std::vector<Entry_t> buffer;
    Level_Node *cur = root;

    insert_result = in_mem->insert(key, val);

    if (insert_result == -1)
    {
        auto start2 = std::chrono::high_resolution_clock::now();
        buffer = LSM_Tree::merge(cur);
        auto stop2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> merge_duration = stop2 - start2;
        merge_accumulated_time += merge_duration;

        Run merged_run = create_run(buffer, cur->level);
        cur->run_storage.push_back(merged_run);

        // clear buffer and insert again.
        in_mem->clear_buffer();
        in_mem->insert(key, val);
    }
    // Stop measuring time
    auto stop = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = stop - start;

    accumulated_time += duration;
}

// overload for loading memory on boot.
void LSM_Tree::put(Entry_t entry)
{
    int insert_result;
    std::vector<Entry_t> buffer;
    Level_Node *cur = root;

    insert_result = in_mem->insert(entry);
}

/**
 * LSM_Tree
 *
 * @param  {KEY_t} key                 :
 * @return {std::unique_ptr<Entry_t>}  :
 */
std::unique_ptr<Entry_t> LSM_Tree::get(KEY_t key)
{
    auto start0 = std::chrono::high_resolution_clock::now();
    std::unique_ptr<Entry_t> in_mem_result = in_mem->get(key);

    if (in_mem_result)
    { // check memory buffer first.
        if (in_mem_result->del)
        {
            std::cout << "not found (deleted)" << std::endl;
            return nullptr;
        }
        std::cout << "found in memory: " << in_mem_result->val << std::endl;
        return in_mem_result;
    }

    std::vector<std::future<std::unique_ptr<Entry_t>>> futures;

    // search in secondary storage.
    Level_Node *cur = root;

    // Define the lambda function to search each run in a level.
    auto processRun = [](auto rit, const auto &key) -> std::unique_ptr<Entry_t> {
        std::unique_ptr<Entry_t> entry = nullptr;

        if (rit->search_bloom(key))
        {
            int starting_point = rit->search_fence(key);

            if (starting_point != -1)
            {
                auto entry = rit->disk_search(starting_point, SAVE_MEMORY_PAGE_SIZE, key);
                if (entry)
                {
                    if (entry->del)
                    {
                        return nullptr;
                    }
                    return entry;
                }
            }
        }
        return nullptr;
    };

    while (cur)
    {
        // std::cout << "searching_level: " << cur->level << std::endl;

        std::vector<std::future<std::unique_ptr<Entry_t>>> futures;
        // start search from the back of the run storage. The latest run contains
        // the most updated data.
        int cnt = 0;
        for (auto rit = cur->run_storage.rbegin(); rit != cur->run_storage.rend(); ++rit)
        {
            futures.push_back(pool.enqueue([=]() -> std::unique_ptr<Entry> { return processRun(rit, key); }));

            // sync up after each batch of threads finish task to prevent unecessary future searches.
            if ((cnt + 1) % num_of_threads == 0)
            {
                for (auto &fut : futures)
                {
                    auto result = fut.get();
                    if (result)
                    {
                        return result;
                    }
                }
                futures.clear();
            }
        }
        // check futures for any remaining queued results.
        for (auto &fut : futures)
        {
            auto result = fut.get();
            if (result)
            {
                return result;
            }
        }
        futures.clear();

        cur = cur->next_level;
    }

    auto stop0 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = stop0 - start0;
    get_accumulated_time += duration;

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
std::vector<Entry_t> LSM_Tree::range(KEY_t lower, KEY_t upper)
{
    // for storing All of the available sub-buffers created.

    std::vector<std::future<std::unordered_map<KEY_t, Entry_t>>> futures;
    std::vector<Entry_t> buffer = in_mem->get_range(lower, upper); // returned vector

    std::unordered_map<KEY_t, Entry_t> hash_mp;
    std::vector<Entry_t> ret;
    // add buffer to hashmap first.
    for (auto &entry : buffer)
    {
        hash_mp[entry.key] = entry;
    }

    Level_Node *cur = root;

    while (cur)
    {
        // iterate each level from back to front.
        for (auto rit = cur->run_storage.rbegin(); rit != cur->run_storage.rend(); ++rit)
        {
            // ret_cur = ret_cur->next;
            futures.push_back(pool.enqueue([=]() -> std::unordered_map<KEY_t, Entry_t> {
                std::vector<Entry_t> temp_vec = rit->range_disk_search(lower, upper);
                std::unordered_map<KEY_t, Entry_t> temp_mp;
                for (auto &entry : temp_vec)
                {
                    auto it = temp_mp.find(entry.key);
                    if (it == temp_mp.end())
                    {
                        temp_mp[entry.key] = entry;
                    }
                }
                return temp_mp;
            }));
        }
        cur = cur->next_level;
    }
    for (auto &fut : futures)
    {
        std::unordered_map<KEY_t, Entry_t> results = fut.get();
        // the merge method disgards repeating key from the merged map.
        hash_mp.merge(results);
    }

    futures.clear();
    for (const auto &pair : hash_mp)
    {
        ret.push_back(pair.second);
    }

    return ret;
}
/**
 * LSM_Tree
 *
 * @param  {KEY_t} key :
 */
void LSM_Tree::del(KEY_t key)
{
    int del_result;
    std::vector<Entry_t> buffer;
    Level_Node *cur = root;

    del_result = in_mem->del(key);

    if (del_result == -1) // buffer is full.
    {
        buffer = LSM_Tree::merge(cur);
        std::cout << cur->level << std::endl;
        Run merged_run = create_run(buffer, cur->level);
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
std::vector<Entry_t> LSM_Tree::merge(LSM_Tree::Level_Node *&cur)
{
    // Insert a new run into storage.
    std::vector<Entry_t> buffer = in_mem->flush_buffer(); // temp buffer for merging.

    // for storing All of the available sub-buffers created.
    std::vector<std::future<std::unordered_map<KEY_t, Entry_t>>> futures;
    std::unordered_set<size_t> level_to_delete;

    while (cur && cur->run_storage.size() == cur->max_num_of_runs - 1)
    {
        if (!cur->next_level)
        { // next level doesn't exist.
            cur->next_level = new Level_Node(cur->level + 1, cur->max_num_of_runs);
        }

        // naive full tiering merge policy.
        for (auto rit = cur->run_storage.rbegin(); rit != cur->run_storage.rend(); ++rit)
        {
            futures.push_back(pool.enqueue([=]() -> std::unordered_map<KEY_t, Entry_t> {
                std::vector<Entry_t> temp_vec = load_full_file(rit->get_file_location(), rit->return_fence());
                std::unordered_map<KEY_t, Entry_t> temp_mp;
                for (auto &entry : temp_vec)
                {
                    auto it = temp_mp.find(entry.key);
                    if (it == temp_mp.end())
                    { // any entries that come later are older, so we can just ignore.
                      // the caveat of this approach is that the deletes will always sink
                      // to the bottom of the tree. Additional code is needed to address this.
                        temp_mp[entry.key] = entry;
                    }
                }
                return temp_mp;
            }));
        }
        level_to_delete.insert(cur->level);
        cur = cur->next_level;
    }

    // reconstruct a full buffer and resolve updates/ deletes
    std::unordered_map<KEY_t, Entry_t> hash_mp;
    std::vector<Entry_t> ret;
    // add buffer to hashmap first.
    for (auto &entry : buffer)
    {
        hash_mp[entry.key] = entry;
    }

    auto start0 = std::chrono::high_resolution_clock::now();

    for (auto &fut : futures)
    {
        std::unordered_map<KEY_t, Entry_t> results = fut.get();
        // the merge method disgards repeating key from the merged map.
        hash_mp.merge(results);
    }

    futures.clear();

    // convert back to vector and sort. - this is about 10% of the cost. So not too bad.
    for (const auto &pair : hash_mp)
    {
        ret.push_back(pair.second);
    }
    std::sort(ret.begin(), ret.end());

    // delete outdated files. - also really small part of cost.
    Level_Node *del_cur = root;
    while (level_to_delete.size() > 0)
    {
        if (level_to_delete.find(del_cur->level) != level_to_delete.end())
        {
            for (int i = 0; i < del_cur->run_storage.size(); i++)
            {
                std::filesystem::path fileToDelete(del_cur->run_storage[i].get_file_location());
                std::filesystem::remove(fileToDelete);
            }
            del_cur->run_storage.clear();
            level_to_delete.erase(del_cur->level);
        }
        del_cur = del_cur->next_level;
    }
    auto stop0 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> update_duration = stop0 - start0;
    merge_update_accumulated_time += update_duration;

    return ret;
}

// this function will save the in-memory data and maintain file structure for
// data persistence.
void LSM_Tree::exit_save()
{
    // save in memory component to file
    //  write data structure to file.
    //  memory get its own designated file. named LSM_memory.dat. Don't need meta
    //  data. secondary memory is maintained by saving another meta data file for
    //  the file structure.
    exit_save_memory();
    level_meta_save();
}

// create a Run and associated file for a given vector of entries.
Run LSM_Tree::create_run(std::vector<Entry_t> buffer, int current_level)
{
    std::string file_name = generateRandomString(6);
    float bloom_bits;
    float cur_FPR; 
    /*Base on MONKEY, total_bits = -entries*ln(FPR)/(ln(2)^2)*/
    if (mode == 1)     // optimized version is activated by mode.
    {          
        cur_FPR = bloom_bits_per_entry * pow(level_ratio, current_level);
        bloom_bits = ceil(-(log(cur_FPR) / (pow(log(2), 2)))); // take ceiling to be safe
    }
    else
    {
        bloom_bits = bloom_bits_per_entry * buffer.size();
    }

    BloomFilter *bloom = new BloomFilter(bloom_bits);
    std::vector<KEY_t> *fence = new std::vector<KEY_t>;

    LSM_Tree::create_bloom_filter(bloom, buffer);
    LSM_Tree::save_to_memory(file_name, fence, buffer);

    Run run(file_name, bloom, fence);
    return run;
}

void LSM_Tree::create_bloom_filter(BloomFilter *bloom, const std::vector<Entry_t> &vec)
{
    for (int i = 0; i < vec.size(); i++)
    {
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
void LSM_Tree::save_to_memory(std::string filename, std::vector<KEY_t> *fence_pointer, std::vector<Entry_t> &vec)
{
    // two pointers to keep track of memory and fence_pointer traversal.
    std::vector<int> bool_bits;
    int memory_cnt = 0, fence_pointer_index = 0;
    std::ofstream out(filename, std::ios::binary);

    if (!out.is_open())
    {
        throw std::runtime_error("Unable to open file for writing");
    }

    for (const auto &entry : vec)
    { // go through all entries.
        if (entry.del)
        {
            bool_bits.push_back(1);
        }
        else
        {
            bool_bits.push_back(0);
        }

        if (memory_cnt == 0)
        {
            fence_pointer->push_back(entry.key);
            fence_pointer_index++;
        }
        out.write(reinterpret_cast<const char *>(&entry.key), sizeof(entry.key));
        out.write(reinterpret_cast<const char *>(&entry.val), sizeof(entry.val));

        memory_cnt = sizeof(entry.key) + sizeof(entry.val) + memory_cnt;

        // reached page maximum. Append the bool_bits to the back of the page.
        if (memory_cnt == SAVE_MEMORY_PAGE_SIZE)
        {
            memory_cnt = 0;
            uint64_t result = 0;

            if (bool_bits.size() != 64)
            {
                throw std::runtime_error("Vector must contain exactly 64 elements.");
            }

            for (size_t i = 0; i < 64; ++i)
            {
                if (bool_bits[i] != 0 && bool_bits[i] != 1)
                {
                    for (int value : bool_bits) // for debugging.
                    {
                        std::cout << value << std::endl;
                    }
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

    if (bool_bits.size() > 0)
    {
        while (bool_bits.size() < 64) // if i were to alter page size, need to use a constant for this.
        {
            int padding = 0;
            bool_bits.push_back(padding); // pad with zeros
        }

        uint64_t result = 0;
        for (size_t i = 0; i < 64; ++i)
        {
            if (bool_bits[i] != 0 && bool_bits[i] != 1)
            {
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

/**
 * LSM_Tree
 *
 * TODO: There is pretty big overlap between exit_save_memory() and
 * save_memory(). Explore simplification.
 */
void LSM_Tree::exit_save_memory()
{
    std::vector<int> bool_bits;
    int memory_cnt = 0;
    std::string filename = "lsm_tree_memory.dat";
    std::vector<Entry_t> buffer = in_mem->flush_buffer();

    std::ofstream out(filename, std::ios::binary);

    if (!out.is_open())
    {
        throw std::runtime_error("Unable to open file for writing on exit save");
    }

    for (const auto &entry : buffer)
    { // go through all entries.
        bool_bits.push_back(entry.del);

        out.write(reinterpret_cast<const char *>(&entry.key), sizeof(entry.key));
        out.write(reinterpret_cast<const char *>(&entry.val), sizeof(entry.val));

        memory_cnt = sizeof(entry.key) + sizeof(entry.val) + memory_cnt;

        if (memory_cnt == SAVE_MEMORY_PAGE_SIZE)
        {
            memory_cnt = 0;
            uint64_t result = 0;

            if (bool_bits.size() != 64)
            {
                throw std::runtime_error("Vector must contain exactly 64 elements.");
            }

            for (size_t i = 0; i < 64; ++i)
            {
                if (bool_bits[i] != 0 && bool_bits[i] != 1)
                {
                    throw std::runtime_error("Vector must contain only 0s and 1s.");
                }
                // Shift the bit to the correct position and set it in 'result'
                result |= static_cast<uint64_t>(bool_bits[i]) << (63 - i);
            }
            out.write(reinterpret_cast<const char *>(&result), sizeof(result));
            bool_bits.clear();
        }
    }

    if (bool_bits.size() > 0)
    {
        while (bool_bits.size() < 64)
        {
            int padding = 0;
            bool_bits.push_back(padding); // pad with zeros
        }
        uint64_t result = 0;
        for (size_t i = 0; i < 64; ++i)
        {
            if (bool_bits[i] != 0 && bool_bits[i] != 1)
            {
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

void LSM_Tree::level_meta_save()
{
    // storing each of the secondary storage related data in different files.
    // bloom and fence pointer binary size are stored as txt in the meta data
    // file. easier to load data on boot.
    std::string meta_data = "lsm_tree_level_meta.txt";
    std::ofstream meta(meta_data);

    if (!meta.is_open())
    {
        std::cerr << "Failed to open the files." << std::endl;
    }

    Level_Node *cur = root;

    // write meta data to the LSM tree first.
    meta << bloom_bits_per_entry << " " << level_ratio << " " << buffer_size << " " << mode << " " << num_of_threads
         << "\n";

    while (cur)
    {
        // for each level: write current level, level limit, run count, then
        // specific data for each run.
        meta << cur->level << " " << cur->max_num_of_runs << " " << cur->run_storage.size() << "\n";

        // for each run: write filename, bloom size and fence size.
        for (int i = 0; i < cur->run_storage.size(); i++)
        {
            // std::cout << cur->run_storage[i].get_file_location() << ", ";

            // write bloom filter to binary file of things could be wrong.
            BloomFilter temp_bloom = cur->run_storage[i].return_bloom();
            boost::dynamic_bitset<> temp_bitarray = temp_bloom.return_bitarray();

            std::string bloom_filename = "bloom_" + cur->run_storage[i].get_file_location();
            std::ofstream bloom(bloom_filename);
            bloom << temp_bitarray; // Write the bitset to the file
            bloom.close();

            // Write fence pointers to binary file
            std::vector<KEY_t> temp_fence = cur->run_storage[i].return_fence();

            std::string fence_filename = "fence_" + cur->run_storage[i].get_file_location();
            std::ofstream fence(fence_filename);
            fence.write(reinterpret_cast<const char *>(&temp_fence[0]), temp_fence.size() * sizeof(KEY_t));
            fence.close();

            meta << cur->run_storage[i].get_file_location() << std::endl;
        }

        cur = cur->next_level;
    }

    meta.close();
}

// helper function for generating a file_name for on-disk storage file name.
std::string LSM_Tree::generateRandomString(size_t length)
{
    const std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string randomString;
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 generator(static_cast<unsigned int>(seed));
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);

    for (size_t i = 0; i < length; ++i)
    {
        randomString += chars[distribution(generator)];
    }

    return "lsm_tree_" + randomString + ".dat";
}

void LSM_Tree::print_statistics()
{
    // PUT operation
    std::cout << "PUT time: " << accumulated_time.count() * 1000 << "ms" << std::endl;
    std::cout << "Merge time: " << merge_accumulated_time.count() * 1000 << "ms" << std::endl;
    std::cout << "Merge update time: " << merge_update_accumulated_time.count() * 1000 << "ms" << std::endl;
    std::cout << "Merge delete time: " << merge_del_accumulated_time.count() * 1000 << "ms" << std::endl;

    // GET operation
    std::cout << "GET time: " << get_accumulated_time.count() * 1000 << "ms" << std::endl;
    std::cout << "GET disk time: " << get_disk_accumulated_time.count() * 1000 << "ms" << std::endl;
    // Range operation

    // delete is essentially the same as get.
}

void LSM_Tree::load_memory()
{
    std::string memory_data = "lsm_tree_memory.dat";
    std::ifstream memory(memory_data, std::ios::binary);
    if (!memory.is_open())
    {
        std::cerr << "Failed to open the memory file." << std::endl;
    }
    // load in buffer from last instance. Need to be aware of the boolbits at the end.
    Entry_t entry;

    // get file size
    memory.seekg(0, std::ios::end);
    size_t fileSize = memory.tellg();
    int total_page = fileSize / LOAD_MEMORY_PAGE_SIZE + 1;
    int cur_page = 0;
    int read_size;
    // modify read_size base on how much more we can read.
    while (cur_page < total_page)
    {
        if ((cur_page + 1) * LOAD_MEMORY_PAGE_SIZE > fileSize)
        {
            read_size = fileSize - cur_page * LOAD_MEMORY_PAGE_SIZE;
        }
        else
        {
            read_size = LOAD_MEMORY_PAGE_SIZE;
        }

        // read in del flags: starting * page_size -> begining of page
        memory.seekg(cur_page * LOAD_MEMORY_PAGE_SIZE + read_size - BOOL_BYTE_CNT, std::ios::beg);
        uint64_t result;
        memory.read(reinterpret_cast<char *>(&result), BOOL_BYTE_CNT);
        std::bitset<64> del_flag_bitset(result);
        memory.seekg(0, std::ios::beg); // reset file stream

        // shift file stream pointer to starting_point base on fence pointer result.
        memory.seekg(cur_page * LOAD_MEMORY_PAGE_SIZE, std::ios::beg);

        int idx = 0;
        while (read_size > BOOL_BYTE_CNT)
        { // change into binary search if i have time.
            memory.read(reinterpret_cast<char *>(&entry.key), sizeof(entry.key));
            memory.read(reinterpret_cast<char *>(&entry.val), sizeof(entry.val));

            entry.del = del_flag_bitset[63 - idx];
            put(entry);
            read_size = read_size - sizeof(entry.key) - sizeof(entry.val);
            idx++;
        }
        cur_page++;
    }

    memory.close();
}

void LSM_Tree::reconstruct_file_structure(std::ifstream &meta)
{
    std::string line;
    Level_Node *cur = root;

    while (std::getline(meta, line))
    {
        std::cout << line << std::endl;
        std::istringstream iss(line);

        if (std::isdigit(line[0]))
        {
            std::string level, max_run, run_cnt;
            if (!(iss >> level >> max_run >> run_cnt))
            {
                break; // issue here.
            };

            if (level != "0")
            {
                cur->next_level = new LSM_Tree::Level_Node(cur->level + 1, cur->max_num_of_runs);
                cur = cur->next_level;
            }
        }
        else
        {
            std::string filename;
            iss >> filename;

            std::ifstream bloom("bloom_" + filename), fence("fence_" + filename, std::ios::binary);

            boost::dynamic_bitset<> input_bitarray;
            bloom >> input_bitarray;

            BloomFilter *bloom_filter = new BloomFilter(input_bitarray);

            std::vector<KEY_t> *fence_pointer = new std::vector<KEY_t>;
            KEY_t key;
            while (fence.read(reinterpret_cast<char *>(&key), sizeof(KEY_t)))
            {
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

std::vector<Entry_t> LSM_Tree::load_full_file(std::string file_location, std::vector<KEY_t> fence_pointers)
{
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
    for (int i = 0; i < fence_pointers.size(); i++)
    {

        if (i * LOAD_MEMORY_PAGE_SIZE > fileSize)
        {
            read_size = fileSize - (i - 1) * LOAD_MEMORY_PAGE_SIZE;
        }
        else
        {
            read_size = LOAD_MEMORY_PAGE_SIZE;
        }

        // read in del flags: starting * page_size -> begining of page
        uint64_t result;
        file.seekg(i * LOAD_MEMORY_PAGE_SIZE + read_size - BOOL_BYTE_CNT, std::ios::beg);
        file.read(reinterpret_cast<char *>(&result), BOOL_BYTE_CNT);
        std::bitset<64> del_flag_bitset(result);
        file.seekg(0, std::ios::beg);

        file.seekg(i * LOAD_MEMORY_PAGE_SIZE, std::ios::beg);

        int idx = 0;
        while (read_size > BOOL_BYTE_CNT)
        {
            file.read(reinterpret_cast<char *>(&entry.key), sizeof(entry.key));
            file.read(reinterpret_cast<char *>(&entry.val), sizeof(entry.val));

            entry.del = del_flag_bitset[63 - idx];
            buffer.push_back(entry);

            read_size = read_size - sizeof(entry.key) - sizeof(entry.val);
            idx++;
        }
    }
    file.close();
    return buffer;
}

void LSM_Tree::print()
{
    Level_Node *cur = root;

    while (cur)
    {
        std::cout << cur->level << ": " << std::endl;
        for (int i = 0; i < cur->run_storage.size(); i++)
        {
            std::cout << cur->run_storage[i].get_file_location() << " ";
        }
        std::cout << " -> end level." << std::endl;

        cur = cur->next_level;
    }
}