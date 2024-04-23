# pragma once
#ifndef BLOOM_H
#define BLOOM_H

#include <boost/dynamic_bitset.hpp>
#include "lib/xxhash32.h"
#include "lib/murmur3.h"

#include "key_value.h"

class BloomFilter {
    // the underlying representation of this should be a bit array. 
    boost::dynamic_bitset<> bitarray;

    // hash functions for bloom filter.
    uint32_t hash_1(KEY_t) const;
    uint32_t hash_2(KEY_t) const;
    uint32_t hash_3(KEY_t) const;

public:
    // constructor
    BloomFilter(long length) : bitarray(length) {}

    BloomFilter(boost::dynamic_bitset<> input_bitarray): bitarray(input_bitarray) {}

    // set bit in bitarray
    void set(KEY_t);

    // check bit in bitarray
    bool is_set(KEY_t) const;

    // return copy of bitarray; 
    boost::dynamic_bitset<> return_bitarray(); 

    int return_bitarray_size(); 
};

#endif