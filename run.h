#pragma once
#ifndef RUN_H
#define RUN_H

#include <vector>

#include "key_value.h"
#include "bloom.h"

#define FILE_PATTERN "/store/lsm-XXXXXX"

class Run {
    BloomFilter bloom;
    std::vector<KEY_t> fence_pointers;
    KEY_t max_key;
    Entry_t *mapping;
    size_t mapping_length;
    int mapping_fd;
    long file_size() {return max_size * sizeof(Entry_t);}
public:
    long size, max_size;
    std::string tmp_file;
    Run(long, float);
    ~Run(void);
    Entry_t * map_read(size_t, off_t);
    Entry_t * map_read(void);

};
#endif 