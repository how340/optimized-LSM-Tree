#pragma once
#ifndef LEVEL_RUN_H
#define LEVEL_RUN_H

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>

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

        // Constructor
        Node(const KEY_t &lower_bound, const KEY_t &upper_bound, const std::string &fileLoc = "",
             Node *nextNode = nullptr)
            : lower(lower_bound), upper(upper_bound), file_location(fileLoc), next(nextNode)
        {
            // Initialize fence_pointers if needed
            // Initialize BloomFilter if it needs specific parameters
        }

        // Default constructor if needed, can initialize with default KEY_t values if KEY_t is default-constructible
        Node() : lower(), upper()
        {
        } // Ensures lower and upper are default-initialized
    };

    Node *root;

  public:
    Level_Run(ThreadPool &pool) : pool(pool)
    {
        root = new Node(1, 3);

        // for testing
        Node *cur = root;

        cur->next = new Node(5, 10);
        cur = cur->next;

        cur->next = new Node(12, 15);
        cur = cur->next;

        cur->next = new Node(20, 25);

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