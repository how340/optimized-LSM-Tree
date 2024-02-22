#include "buffer_level.h"
#include "run.h"
#include <iostream>

int main(){
    BufferLevel test(10);

    test.insert(10, 20);
    test.insert(1, 20);
    test.insert(2, 20);
    test.insert(3, 20);
    test.insert(4, 20);
    test.insert(5, 20);
    test.insert(6, 25);
    test.insert(7, 20);
    test.insert(8, 20);
    test.insert(9, 20);
    
    test.get(6);

    test.clear_buffer();

    test.get(1);

    test.insert(1, 20);
    test.insert(1, 10);
    
    std::cout << "testing the outcome of value not found. " << std::endl;
    std::cout << (test.get(1) -> first) << std::endl;

    std::cout << (test.get(20) -> second) << std::endl;
    for (int i = 0; i < 15; i++){
        test.insert(i, 20);
    }

    test.clear_buffer();

    test.insert(1, 2);
    test.insert(2, 4);
    test.insert(3, 6);
    test.insert(4, 8);
    test.insert(5, 10);

    std::vector<VALUE_t> ret = test.get_range(2, 4);
    for (const auto& value : ret) {
        std::cout << value << '\n';
    }

    test.save_to_memory("fillname.dat");

    // test bloomfilter creation. 
    std::cout << "testing bloom filter" << std::endl;

    test.insert(1, 2);
    test.insert(2, 4);
    test.insert(3, 6);
    test.insert(4, 8);
    test.insert(5, 10);

    long length = test.size();
    int bloom_ratio = 10;

    BloomFilter bloom(length * bloom_ratio);

    test.create_bloom_filter(bloom);

    std::cout << "is the key 3 set: "<< bloom.is_set(3) << std::endl;


    // testing run concepts
    std::cout << "testing run concepts" << std::endl;

    Run test_run(100, 10.0);

    std::cout << "temp file is  "<< test_run.tmp_file  << std::endl;

    std::ifstream read_file("fillname.dat", std::ios::in)


    return 0;
};