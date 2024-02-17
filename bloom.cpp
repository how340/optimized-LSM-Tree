#include "bloom.h"

// Selection of hash functions inspired by https://llimllib.github.io/bloomfilter-tutorial/

// FNV-1 hash. 
// inspired by http://isthe.com/chongo/tech/comp/fnv/#FNV-1a
uint64_t BloomFilter::hash_1(KEY k) const {
    // FNV prime and offset(hash) is dependent on bit length. 
    const uint32_t Prime = 0x01000193; 
    const uint32_t hash  = 0x811C9DC5; 
  
     for (int i = 0; i < 4; ++i) {
        uint8_t byte = (k >> (i * 8)) & 0xFF; // Extract byte from the integer
        hash ^= byte;                            // XOR the bottom with the current byte
        hash *= FNV_prime;                       // Multiply by the FNV prime
    }

    return hash % bitarray.size();
}

// murmur3 hash
uint64_t BloomFilter::hash_2(KEY k) const {

    int32_t key = k;
    uint32_t seed = 42; //arbiturary seed 
    uint32_t hash = MurmurHash3_x86_32(&key, sizeof(key), seed);

    return hash % bitarray.size();
}

// xxhash hash
uint64_t BloomFilter::hash_3(KEY k) const {

    int32_t key = k;
    uint32_t seed = 42; //arbiturary seed 
    uint32_t result2 = XXHash32::hash(&key, sizeof(key), seed);

    return 0;
}

void BloomFilter::set(KEY key) {
    bitarray.set(hash_1(key));
    bitarray.set(hash_2(key));
    bitarray.set(hash_3(key));
}

bool BloomFilter::is_set(KEY key) const {
    return (bitarray.test(hash_1(key))
         && bitarray.test(hash_2(key))
         && bitarray.test(hash_3(key)));
}

// murmurhash3 modified from https://github.com/aappleby/smhasher/blob/master/src/MurmurHash3.cpp

uint32_t MurmurHash3_x86_32(const void* key, int len, uint32_t seed) {
    const uint8_t* data = (const uint8_t*)key;
    const int nblocks = len / 4;
    uint32_t h1 = seed;
    const uint32_t c1 = 0xcc9e2d51;
    const uint32_t c2 = 0x1b873593;

    // body
    const uint32_t* blocks = (const uint32_t*)(data + nblocks * 4);
    for (int i = -nblocks; i; i++) {
        uint32_t k1 = blocks[i];

        k1 *= c1;
        k1 = (k1 << 15) | (k1 >> (32 - 15));
        k1 *= c2;

        h1 ^= k1;
        h1 = (h1 << 13) | (h1 >> (32 - 13));
        h1 = h1 * 5 + 0xe6546b64;
    }

    // tail
    const uint8_t* tail = (const uint8_t*)(data + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3) {
        case 3:
            k1 ^= tail[2] << 16;
        case 2:
            k1 ^= tail[1] << 8;
        case 1:
            k1 ^= tail[0];
            k1 *= c1;
            k1 = (k1 << 15) | (k1 >> (32 - 15));
            k1 *= c2;
            h1 ^= k1;
    }

    // finalization
    h1 ^= len;

    // fmix
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    return h1;
};
