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


    // test bloomfilter creation. 
    std::cout << "testing bloom filter" << std::endl;

    test.insert(1, 2);
    test.insert(2, 4);
    test.insert(3, 6);
    test.insert(4, 8);
    test.insert(5, 10);

    long length = test.size();
    int bloom_ratio = 10;


    std::vector<Entry_t> vec = test.convert_tree_to_vector();

    BloomFilter bloom(length * bloom_ratio);

    test.create_bloom_filter(bloom, vec);

    std::cout << "is the key 3 set: "<< bloom.is_set(3) << std::endl;

    // Test buffer saving to SSTable. 
    // switching to a much larger buffer.
    std::cout << "testing writing SSTable files" << std::endl;

    int buffer_size = 2000; 
    BufferLevel test_large(buffer_size);
    
    int search_key = 756;
    for( int i = 0; i < buffer_size; i++){
        if (i == search_key){
            test_large.insert(i, 42069);
        } else {
            test_large.insert(i, 42);
        }
        
    }

    std::vector<Entry_t> vec_large = test_large.convert_tree_to_vector();
    std::vector<KEY_t> fence_pointer;

    test_large.save_to_memory("fillname.dat", &fence_pointer, vec_large);


    //print the fence pointers
    std::cout << "reading from fence pointers: " << std::endl;
    for (int i = 0; i < fence_pointer.size(); ++i) {
        std::cout << fence_pointer[i];
        if (i < fence_pointer.size() - 1) {
            std::cout << ", "; // Print a comma between elements, but not after the last element
        }
    }
    std::cout << std::endl; // Print a newline at the end

    // test reading out the binary file. 
    std::cout << "testing reading SSTable files" << std::endl;

    std::vector<Entry_t> keyValuePairs;
    std::ifstream file("fillname.dat", std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file for reading");
    }

    Entry_t kvp;
    // Move the following to LSM tree file -----------------------------------------------------
    // this will be essentially how we use fence pointers to interact with the file structure.
    // use fence post to look for the search key directly in file. 
    // TODO: also some edge cases. Implement test later. 
    for(int i = 0; i < fence_pointer.size() ; i++){
        std::cout << fence_pointer[i] << std::endl;
        if (search_key > fence_pointer[i] && search_key < fence_pointer[i+1]){
            file.seekg(i * 512, std::ios::beg);
            size_t numBytesToRead = 512; // Specify how many bytes you want to read
            std::vector<Entry_t> buffer(numBytesToRead);

            while(numBytesToRead > 0){
                file.read(reinterpret_cast<char*>(&kvp.key), sizeof(kvp.key));
                file.read(reinterpret_cast<char*>(&kvp.val), sizeof(kvp.val));
                keyValuePairs.push_back(kvp);
                numBytesToRead--;
            }       
        }
    }

    file.close();

    Run run1("fillname.dat", &bloom, &fence_pointer);

    // add test for the run functionalities. 


    return 0;
};