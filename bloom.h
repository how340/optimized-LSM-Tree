#include <boost/dynamic_bitset.hpp>
#include "lib/xxhash32.h"
#include "lib/murmur3.h"

typedef int32_t KEY;

class BloomFilter {
    // the underlying representation of this should be a bit array. 
    boost::dynamic_bitset<> bitarray;

    // hash functions for bloom filter.
    uint32_t hash_1(KEY) const;
    uint32_t hash_2(KEY) const;
    uint32_t hash_3(KEY) const;

public:
    // constructor
    BloomFilter(long length) : bitarray(length) {}

    // set bit in bitarray
    void set(KEY);

    // check bit in bitarray
    bool is_set(KEY) const;

};
