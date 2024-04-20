#include "level_run.h"

// Level_Run::Level_Run(ThreadPool& pool) : pool(pool) {
//     root = new Node;

//     // for testing
//     Node* cur = root;

//     cur->lower = 5;
//     cur->upper = 10;
//     cur->next = new Node;
//     cur = cur->next;

//     cur->lower = 12;
//     cur->upper = 15;
//     cur->next = new Node;
//     cur = cur->next;

//     cur->lower = 20;
//     cur->upper = 25;
//     cur->next = new Node;
//     cur = cur->next;
// }

void Level_Run::insert_block(std::vector<Entry_t> buffer)
{
    KEY_t left = buffer[0].key;
    KEY_t right = buffer.back().key;

    // Find the two bounds
    Node *prev = nullptr, *cur = root;
    Node *front = nullptr, *back = nullptr; // for tracking insert location

    std::cout << "leak here 0" << std::endl;
    while (cur)
    {
        if (cur->lower > left && !front)
        { // find left post
            if (prev && prev->lower <= left)
            {
                front = prev;
            }
            else
            {
                front = root; // edge case for left smaller than all of the level.
            }
        }
        if (cur->lower > right && !back)
        { // find right post
            if (prev && prev->lower <= right)
            {
                back = cur;
                std::cout << "came here" << std::endl;
            }
            else
            {
                back = root; // edge case for right smaller than all of levels.
            }
        }
        prev = cur;
        cur = cur->next;
    }
    // check if the last node can be the front node
    if (prev->upper && front == nullptr){
        front = prev; 
    }

    std::cout << "leak here 1" << std::endl;
    // find overlapping blocks of files -> then read them in. -- multi-thread here.
    std::vector<std::future<std::unordered_map<KEY_t, Entry_t>>> futures;
    cur = front;
    while (cur != back)
    {
        futures.push_back(pool.enqueue([=]() -> std::unordered_map<KEY_t, Entry_t> {
            std::vector<Entry_t> temp_vec = load_full_file(cur->file_location, cur->fence_pointers);
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
        cur = cur->next;
    }
    std::cout << "leak here 2" << std::endl;
    // merge the new and old blocks.
    std::unordered_map<KEY_t, Entry_t> hash_mp;
    for (auto &entry : buffer)
    {
        hash_mp[entry.key] = entry;
    }
    for (auto &fut : futures)
    {
        std::unordered_map<KEY_t, Entry_t> results = fut.get();
        hash_mp.merge(results);
    }

    futures.clear();
    std::vector<Entry_t> ret;
    // convert back to vectors and sort. -- is there a more efficient way?
    for (const auto &pair : hash_mp)
    {
        ret.push_back(pair.second);
    }
    std::sort(ret.begin(), ret.end());
    std::cout << "leak here 3" << std::endl;
    Node *start_node = save_to_memory(ret); // create the underlying representation here.

    // store back onto disk format. use front and back to find correct insert location.
    Node *insert_start = nullptr, *start_node_chain_end= nullptr, *to_delete= nullptr, *insert_cur= nullptr;
    std::cout << "leak here 4" << std::endl;
    std::cout << front << " " << back << std::endl;
    if (front == back)
    {
        if (front == root)
        {
            if (root->is_empty)
            { // we have a empty level. Just swap and delete.
                delete root;
                root = start_node;
            }
            // insert everything in front of the linked list.
            start_node->next = root;
            root = start_node;
        }
        else if (front == nullptr)
        {
            if (root->is_empty)
            { // this is also the case when you have an empty level
                root = start_node;
            }
            else
            {
                // insert everything into the back of linked list.'
                insert_cur = root;
                while (insert_cur)
                {
                    insert_cur = insert_cur->next;
                }
                insert_cur->next = start_node;
            }
        }
        else
        {
            std::cout << "Error in finding overlapping regions" << std::endl;
        }
    }
    else
    {
        if (root == front) // need to replace root node with new node.
        {
            to_delete = root;
            insert_cur = root;
            root = start_node;
        }
        else
        {
            std::cout << "going here" << std::endl;
            insert_start = root;
            while (insert_start->next != front)
            { // go to node before front.
                insert_start = insert_start->next;
            }
            insert_start->next = start_node;
            to_delete = front; 
            insert_cur = front;
        }

        while (insert_cur->next != back) // find the last node that will be merged
        {
            insert_cur = insert_cur->next;
        }
        insert_cur->next = nullptr; // cut off the link

        start_node_chain_end = start_node; // find end of inserted linked list.
        while (start_node_chain_end->next != nullptr)
        {
            start_node_chain_end = start_node_chain_end->next;
        }
        if (back != nullptr){
            std::cout << back->lower << std::endl;
            start_node_chain_end->next = back; // relink the list
        }
    }
    std::cout << "leak here 5" << std::endl;
    // delete unnecessary files for house keeping

    while (to_delete)
    {
        Node *tmp = to_delete->next;
        delete to_delete;
        to_delete = tmp;
    }
}

std::vector<Entry_t> Level_Run::load_full_file(std::string file_name, std::vector<KEY_t> &fence_pointers)
{
    // read-in the the oldest run at the level.
    std::ifstream file(file_name, std::ios::binary);
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

// return the first node in the chain that needs to be inserted.
Level_Run::Node *Level_Run::save_to_memory(std::vector<Entry_t> vec)
{
    int block_entry_cnt = SAVE_MEMORY_PAGE_SIZE / 16 * 20;
    int block_cnt = vec.size() / block_entry_cnt + 1;

    int vector_partitions_l = 0;
    int vector_partitions_r = block_entry_cnt + 1;

    Entry_t entry;

    std::vector<std::future<Node *>> futures;

    while (block_cnt > 0)
    {
        if (vector_partitions_r > vec.size())
        { // keep everything inbound.
            vector_partitions_r = vec.size();
        }
        std::vector<Entry_t> block(vec.begin() + vector_partitions_l, vec.begin() + vector_partitions_r);

        futures.push_back(pool.enqueue([=]() -> Node * {
            Node *temp_nd = process_block(block);

            return temp_nd;
        }));
        // update loop info.
        vector_partitions_l = vector_partitions_r;
        vector_partitions_r = vector_partitions_r + block_entry_cnt + 1;
        block_cnt--;
    }

    Node *chain_start = futures[0].get();
    Node *chain_cur = chain_start;
    for (int i = 1; i < futures.size(); i++)
    {
        Node *tmp = futures[i].get();
        chain_cur->next = tmp;
        chain_cur = chain_cur->next;
    }
    return chain_start;
}

Level_Run::Node *Level_Run::process_block(std::vector<Entry_t> block)
{
    // set up file output structure.
    std::vector<int> bool_bits;
    int memory_cnt = 0;
    std::string filename = generate_file_name(6);
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open())
    {
        throw std::runtime_error("Unable to open file for writing");
    }
    // fence pointer and bloom filter
    BloomFilter *bloom = new BloomFilter(bits_per_entry);
    std::vector<KEY_t> fence_pointers;

    for (const auto &entry : block)
    { // go through all entries.
        bool_bits.push_back(entry.del ? 1 : 0);
        bloom->set(entry.key);

        if (memory_cnt == 0)
        {
            fence_pointers.push_back(entry.key);
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

    fence_pointers.push_back(block.back().key);

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
    // construct node* for return
    Node *node = new Node;
    node->lower = block.begin()->key;
    node->upper = block.back().key;
    node->file_location = filename;
    node->bloom = bloom;
    node->fence_pointers = fence_pointers;
    node->is_empty = false;
    node->next = nullptr;

    return node;
}

void Level_Run::print()
{
    Node *cur = root;

    while (cur)
    {
        std::cout <<  "node filename " << cur->file_location << std::endl;
        std::cout << "file range " << cur->lower << " -> " << cur->upper << std::endl;
        cur = cur->next;
    }
}

// helper function for generating a file_name for on-disk storage file name.
std::string Level_Run::generate_file_name(size_t length)
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

    return "lsm_tree_leveling" + randomString + ".dat";
}
