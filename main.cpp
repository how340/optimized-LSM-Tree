#include "buffer_level.h"
#include "run.h"
#include "lsm_tree.h"
#include <filesystem> 
#include <iostream>
#include "sys.h"
    
namespace fs = std::filesystem;


void command_loop(LSM_Tree& tree) {
    char command;
    KEY_t key_a, key_b;
    VALUE_t val;


    while (std::cin >> command) {
        switch (command) {
        case 'p':
            std::cin >> key_a >> val;

            if (val < MIN_VAL || val > MAX_VAL) {
                die("Could not insert value " + std::to_string(val) + ": out of range.");
            } else {
                tree.put(key_a, val);
            }

            break;
        case 'g':
            std::cin >> key_a;
            tree.get(key_a);
            break;


        default:
            die("Invalid command.");
        }
    }
}


int main(int argc, char *argv[]){
    int opt; 
    size_t buffer_size, test_size; 

    if (argc > 1) {
        // Loop through all arguments (skipping argv[0], which is the program name)
        buffer_size = std::stoi(argv[1]);
        test_size = std::stoi(argv[2]);

        std::cout << "buffer size: " << buffer_size << std::endl; 
        std::cout << "test size: " << test_size << std::endl; 
    } else {
        std::cout << "No command-line arguments provided." << std::endl;
    }

    int search_key = 756;
    

    // std::cout << "\n basic LSM tree benchmark \n" << std::endl;

    LSM_Tree lsm_tree(10, 3, buffer_size, 1);
    command_loop(lsm_tree);
    // std::cout << lsm_tree.root->level << std::endl;

    // // start time
    // auto start = std::chrono::high_resolution_clock::now();

    // for( int i = 0; i < test_size; i++){
    //     if (i == search_key){
    //         lsm_tree.put(i, 42069);
    //     } else {
    //         lsm_tree.put(i, 42);
    //     }   
    // }

    // // Record the end time
    // auto end = std::chrono::high_resolution_clock::now();
    
    // // Calculate the duration in milliseconds
    // auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // // Output the duration
    // std::cout << lsm_tree.get(search_key)->val<< std::endl;


    // // Output the duration
    // std::cout << "Program executed in " << duration.count() << " milliseconds." << std::endl;

    // // Printing content of saved file
    // // std::vector<Entry_t> buffer;// read files in questions into buffer. 

    // // std::ifstream file("lsm_tree_i0yw97.dat", std::ios::binary);
    
    // // if (!file.is_open()) throw std::runtime_error("Unable to open file for writing");

    // // Entry_t entry;
    // // while (file.read(reinterpret_cast<char*>(&entry), sizeof(entry))) {
    // //     std::cout << entry.key << ":" << entry.val << ", ";
    // //     buffer.push_back(entry);
    // // }
    // // file.close(); 

    fs::path path_to_directory{"./"};

    // Iterate over the directory
    for (const auto& entry : fs::directory_iterator(path_to_directory)) {
        // Check if the file extension is .dat
        if (entry.path().extension() == ".dat") {
            // If it is, delete the file
            fs::remove(entry.path());
            std::cout << "Deleted: " << entry.path() << std::endl;
        }
    }

    return 0;
};