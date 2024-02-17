#include "bloom.h"
#include <iostream>
// Selection of hash functions inspired by https://llimllib.github.io/bloomfilter-tutorial/

// Hash-1: FNV-1 
// Inspired by http://isthe.com/chongo/tech/comp/fnv/#FNV-1a
uint32_t BloomFilter::hash_1(KEY k) const {
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
uint32_t BloomFilter::hash_2(KEY k) const {
    int32_t key = k;
    uint32_t seed = 42; //arbiturary seed 
    uint32_t hash = MurmurHash3_x86_32(&key, sizeof(key), seed);

    return hash % bitarray.size();
}

// Hash-3: xxhash 
// Acquired from: http://cyan4973.github.io/xxHash/
uint32_t BloomFilter::hash_3(KEY k) const {
    int32_t key = k;
    uint32_t seed = 42; //arbiturary seed 
    uint32_t hash = XXHash32::hash(&key, sizeof(key), seed);

    return hash % bitarray.size();
}

// Public Functions
// This funciton manipulates the bits for the bitarray
void BloomFilter::set(KEY key) {
    bitarray.set(hash_1(key));
    bitarray.set(hash_2(key));
    bitarray.set(hash_3(key));
};

// This funciton Check the bits for the bitarray
bool BloomFilter::is_set(KEY key) const {
    return (bitarray.test(hash_1(key))
         && bitarray.test(hash_2(key))
         && bitarray.test(hash_3(key)));
};

// brief test for bloom filter functionality. 
int main(){
    // in practice, the length of the bloom filter will be determined by each level. 
    BloomFilter bloom = BloomFilter(200);

    int32_t testing_array[20] = {
        461054704,
        442148408,
        930675244,
        501544069,
        -951908533,
        1971813547,
        1657796400,
        -323402993,
        919540504,
        2145060469,
        1095696046,
        794616133,
        -782711141,
        256623214,
        -614641161,
        63188288,
        -1026516159,
        1806756581,
        -72559114,
        226920238};

    for (int i = 0; i < 20; i ++){
        bloom.set(testing_array[i]);
    }

    int32_t matching_array[20] = {
        -951908533,
        -323402993,
        -1026516159,
        930675244,
        -614641161,
        -782711141,
        461054704,
        501544069,
        919540504,
        2145060469
        -231069846,
        -637889456,
        1935858525,
        790447499,
        -1708161621,
        342023357,
        -491969042,
        1463794994,
        -1773683749,
        -1815258757
    };

    for (int i = 0; i < 20; i ++){
        if(bloom.is_set(matching_array[i])){
            std::cout << "Interger" << matching_array[i] << "is in bloom filter" << std::endl; 
        } else {
            std::cout << "Interger" << matching_array[i] << "is not in bloom filter" << std::endl;
        }
    }

};