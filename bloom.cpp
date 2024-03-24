#include "bloom.h"
#include <iostream>
// Selection of hash functions inspired by https://llimllib.github.io/bloomfilter-tutorial/

// Hash-1: FNV-1 
// Inspired by http://isthe.com/chongo/tech/comp/fnv/#FNV-1a
uint32_t BloomFilter::hash_1(KEY_t k) const {
    // FNV prime and offset(hash) is dependent on bit length. 
    const int32_t Prime = 0x01000193; 
    const int32_t Offset  = 0x811C9DC5; 
    int32_t hash = Offset; 

    for (int i = 0; i < 4; ++i) {
        uint8_t byte = (k >> (i * 8)) & 0xFF; // Extract byte from the integer
        hash ^= byte;                            // XOR the bottom with the current byte
        hash *= Prime;                       // Multiply by the FNV prime
    }

    return hash % bitarray.size();
}

// Hash-2: murmur3
// Modified from https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp
uint32_t BloomFilter::hash_2(KEY_t k) const {
    int32_t key = k;
    uint32_t seed = 42; //arbiturary seed 
    uint32_t hash = MurmurHash3_x86_32(&key, sizeof(key), seed);

    return hash % bitarray.size();
}

// Hash-3: xxhash 
// Acquired from: http://cyan4973.github.io/xxHash/
uint32_t BloomFilter::hash_3(KEY_t k) const {
    int32_t key = k;
    uint32_t seed = 42; //arbiturary seed 
    uint32_t hash = XXHash32::hash(&key, sizeof(key), seed);

    return hash % bitarray.size();
}

// Public Functions
// This funciton manipulates the bits for the bitarray
void BloomFilter::set(KEY_t key) {
    bitarray.set(hash_1(key));
    bitarray.set(hash_2(key));
    bitarray.set(hash_3(key));
};

// This funciton Check the bits for the bitarray
bool BloomFilter::is_set(KEY_t key) const {
    return (bitarray.test(hash_1(key))
         && bitarray.test(hash_2(key))
         && bitarray.test(hash_3(key)));
};

boost::dynamic_bitset<> BloomFilter::return_bitarray(){
    return bitarray;
}; 
