#pragma once
#ifndef LEVEL_RUN_H
#define LEVEL_RUN_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <filesystem>

#include "bloom.h"
#include "key_value.h"
#include "lib/ThreadPool.h"

class Level_Run
{
    ThreadPool &pool;

    const int LOAD_MEMORY_PAGE_SIZE = SAVE_MEMORY_PAGE_SIZE / 64 + SAVE_MEMORY_PAGE_SIZE;
    const int BOOL_BYTE_CNT = SAVE_MEMORY_PAGE_SIZE / 64;
    const int BLOCK_SIZE = LOAD_MEMORY_PAGE_SIZE * 20; // this is quite arbituary for now.
    float bits_per_entry = 10; // need to change the LSM tree code to store this as a constant. 


    struct Node
    {
        BloomFilter* bloom;
        std::vector<KEY_t> fence_pointers;
        std::string file_location;

        // these are stored as the rough fence posts of this node/
        KEY_t lower;
        KEY_t upper;

        Node *next = nullptr;
        bool is_empty = true;

        ~Node() {
        delete bloom;  // Delete the BloomFilter object if it exists.
        bloom = nullptr; // Prevent dangling pointer.

        std::filesystem::path fileToDelete(file_location);
        std::filesystem::remove(fileToDelete);
    }

    };

    Node *root;

  public:
    Level_Run(ThreadPool &pool) : pool(pool)
    {
        root = new Node;
    }
    // ~Level_Run();

    // used to insert blocks of data into the existing leveling levels.
    void insert_block(std::vector<Entry_t>);

    std::vector<Entry_t> load_full_file(std::string, std::vector<KEY_t>&); // just need the index in the storage.

    Node* save_to_memory(std::vector<Entry_t>);
    Node* process_block(std::vector<Entry_t>);

    void print(); 
    std::string generate_file_name(size_t length);
};

#endif