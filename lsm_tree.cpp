#include "lsm_tree.h"

LSM_Tree::LSM_Tree(size_t bits_ratio, size_t level_ratio)
    : bloom_bits_per_entry(bits_ratio), level_ratio(level_ratio)
{
    root = new Level_Node{0, level_ratio};


}

LSM_Tree::~LSM_Tree() {
        delete in_mem; // Free the dynamically allocated in-memory buffer
        // Consider implementing or calling a method here to clean up other dynamically allocated resources, if any
        // probably want to delete the tree structure from root recursively. 
}

//TODO: consider adding overwrite function? Add delete later as well. 
int LSM_Tree::buffer_insert(KEY_t key, VALUE_t val)
{
    int insert_result; 
    insert_result = in_mem->insert(key, val);

    // TODO: can rewrite this to check if the level is going to be full, and just doesn't wirte the file. 
    if (insert_result == -1){//buffer size is full.
        Level_Node* cur = root; 

        // Insert a new run into storage. 
        std::vector<Entry_t> vec = in_mem->convert_tree_to_vector();
        BloomFilter* bloom = new BloomFilter(size*bloom_bits_per_entry);
        std::vector<KEY_t>* fence_pointer = new std::vector<KEY_t>;

        std::string file_name = "lsm_tree_" + generateRandomString(6) + ".dat";

        in_mem->save_to_memory(file_name, fence_pointer, vec);
        in_mem->create_bloom_filter(bloom, vec);
        Run run(file_name, bloom, fence_pointer);

        cur->run_storage.push_back(run); 
        
        // TODO: I will need to allow for different choices of merge policy. Do naive (full_buffer) tiering for now. 
        while (cur && cur->run_storage.size() == cur->max_num_of_runs){
            std::cout << "\nreached current level max\n" << std::endl;

            if (!cur->next_level){// next level doesn't exist. 
                cur->next_level = new Level_Node(cur->level + 1, cur->max_num_of_runs);
            }

            // TODO: at some point, the size of all of the files will exceed the memory. 
            // two main ways of doing this, either do external sorting, or do the partial compaction with the following level. 
            // naive full tiering merge policy. 
            std::vector<Entry_t> buffer;// read files in questions into buffer. 

            for (int i = 0; i < cur->run_storage.size(); i++){
                std::ifstream in(cur->run_storage[i].get_file_location(), std::ios::binary);

                if (!in.is_open()) throw std::runtime_error("Unable to open file for writing");
                while(!in.eof()){
                    auto entry = std::make_unique<Entry_t>();
                    in.read(reinterpret_cast<char*>(&entry->key), sizeof(entry->key));
                    in.read(reinterpret_cast<char*>(&entry->val), sizeof(entry->val));
                    buffer.push_back(*entry);
                }
                in.close(); 
            }

            // re-sort, create new bloom, new fence pointer, create new run in next level. 
            std::sort(buffer.begin(), buffer.end());
            std::string merged_file_name = "lsm_tree_" + generateRandomString(6) + ".dat";

            BloomFilter* merged_bloom = new BloomFilter(size*bloom_bits_per_entry);
            std::vector<KEY_t>* merged_fence_pointer = new std::vector<KEY_t>;

            LSM_Tree::save_to_memory(merged_file_name, merged_fence_pointer, buffer);
            LSM_Tree::create_bloom_filter(merged_bloom, buffer);

            Run merged_run(merged_file_name, merged_bloom, merged_fence_pointer);// find a way to generate dynamic run file name.
            // delete current level info and the actual files. 

            for (int i = 0; i < cur->run_storage.size(); i++){
                std::filesystem::path fileToDelete(cur->run_storage[i].get_file_location());

                std::filesystem::remove(fileToDelete);
            }
            cur->run_storage.clear();

            cur = cur->next_level;

            cur->run_storage.push_back(merged_run); // push to the new level. 
            //DEBUG messages. Need to fix either reading the file or the write method.
            // std::cout << "Buffer has the size of: " << buffer.size() << std::endl;
            // for (int i = 0; i < buffer.size(); i++){
            //     std::cout << buffer[i].key << " : " << buffer[i].val << ", "; 
            // }
            
        }
        
        // clear buffer and insert again. 
        in_mem->clear_buffer(); 
        in_mem->insert(key, val);

        return insert_result;
    } 

    return insert_result;
}

// helper function for generating a file_name for on-disk storage file name. 
std::string LSM_Tree::generateRandomString(size_t length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::string randomString;
    auto seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::mt19937 generator(static_cast<unsigned int>(seed));
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);

    for(size_t i = 0; i < length; ++i) {
        randomString += chars[distribution(generator)];
    }

    return randomString;
}

// These were originally from the buffer class. I should move them here. This makes more sense here. 
std::unique_ptr<Entry_t> LSM_Tree::search_value(KEY_t key){

    std::unique_ptr<Entry_t> in_mem_result = in_mem->get(key);

    if (in_mem_result){ //check memory buffer first. 
        return in_mem_result; 
    } 
    else {
        // do in-memory search
        Level_Node* cur = root; 
        std::unique_ptr<Entry_t> entry = nullptr;
        std::cout << "searching_level: " << cur->level <<std::endl; 
        for(size_t i = 0; i < cur->run_storage.size(); ++i) {
            
            if(cur->run_storage[i].search_bloom(key)){
                int starting_point = cur->run_storage[i].search_fence(key);

                if (starting_point != -1){
                    entry = cur->run_storage[i].disk_search(starting_point, MEMORY_PAGE_SIZE, key);
                    if (entry){return entry;} 
                }
            };
        }

        // continue to do things recursively in the next levels. TODO: write tests here.  
        while(cur->next_level){
            cur = cur->next_level; 
            std::cout << "searching_level: " << cur->level <<std::endl; 
            for(size_t i = 0; i < cur->run_storage.size(); ++i) {
                if(cur->run_storage[i].search_bloom(key)){
                    int starting_point = cur->run_storage[i].search_fence(key);

                    if (starting_point != -1){
                        entry = cur->run_storage[i].disk_search(starting_point, MEMORY_PAGE_SIZE, key);
                    if (entry){return entry;} 
                    }
                }
            }   
                
        }
    }
    return nullptr;
}


void LSM_Tree::create_bloom_filter(BloomFilter* bloom, const std::vector<Entry_t>& vec){
        for (int i = 0; i < vec.size(); i++){
            bloom->set(vec[i].key);
        }
    };

// writes content of buffer to file on disk. 
// passing by reference needs to be worked on a bit here. Don't know how this came to be. 
void LSM_Tree::save_to_memory(std::string filename,  std::vector<KEY_t>* fence_pointer, std::vector<Entry_t>& vec){
    // two pointers to keep track of memory and fence_pointer traversal. 
    int memory_cnt = 0, fence_pointer_index = 0;
    std::ofstream out(filename, std::ios::binary);

    if (!out.is_open()) {
        throw std::runtime_error("Unable to open file for writing");
    }

    std::cout << "current vector has size: " << vec.size() << std::endl;
    for (const auto& entries : vec){// go through all entries.
        if (memory_cnt == 0){
            fence_pointer->push_back(entries.key);
            fence_pointer_index++; 
        }
        
        out.write(reinterpret_cast<const char*>(&entries.key), sizeof(entries.key));
        out.write(reinterpret_cast<const char*>(&entries.val), sizeof(entries.val));
        
        memory_cnt = sizeof(entries.key) + sizeof(entries.val) + memory_cnt; 
        
        if (memory_cnt == MEMORY_PAGE_SIZE)  memory_cnt = 0; // reached page maximum
    }  
    // Lazy approach right now, the last fence pointer is just the maximum key for now. 
    fence_pointer->push_back(vec.back().key);

    out.close(); 
    std::cout << "Successfully written to file : " << filename << std::endl; 
}