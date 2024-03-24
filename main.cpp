#include "buffer_level.h"
#include "run.h"
#include "lsm_tree.h"
#include <filesystem> 
#include <iostream>
#include "sys.h"
    
namespace fs = std::filesystem;

/* 
List of TODO elements: 
1.implement data persistence - create API to handle saving normal files, reading in saved files, and handling exiting applications. 
2. Implement the two other query operators (range, delete). I need to figure out what to save delete as. (add an additional bit?)
2. Implement a more advanced merge policy to improve performance. 
3. Improve the two optimization proposed in the paper. 
4. Implement more optimization by myself. 
5. multi-threaded support
6. better file I/O approach. 

*/

void command_loop(LSM_Tree& tree) {
    char command;
    KEY_t key_a, key_b;
    VALUE_t val;

    std::chrono::milliseconds totalDuration(0);

    while (std::cin >> command) {
        if (command == 'q') {  // Assuming 'q' is the command to quit
            tree.exit_save();
            break;  // Exit the loop
        }
        
        switch (command) {
            case 'p': {
                std::cin >> key_a >> val;
                if (val < MIN_VAL || val > MAX_VAL) {
                    die("Could not insert value " + std::to_string(val) + ": out of range.");
                } else {
                    tree.put(key_a, val);
                }
                break;
            }
            case 'g': {
                std::cin >> key_a;
                
                auto timer_s = std::chrono::high_resolution_clock::now();

                tree.get(key_a);

                auto timer_e = std::chrono::high_resolution_clock::now();
                auto timer_duration = std::chrono::duration_cast<std::chrono::milliseconds>(timer_e - timer_s);
                totalDuration += timer_duration;

                std::cout << "total lookup took: " << totalDuration.count() << " milliseconds." << std::endl;

                break;
            }
            default:
                die("Invalid command.");
        }
    }
}

// load data on start up.
void meta_load_save(){
    std::string meta_data = "lsm_tree_level_meta.txt";
    std::string fence_data = "lsm_tree_fence.dat";
    std::string bloom_data = "lsm_tree_bloom.dat";

    std::ifstream meta(meta_data);
    std::ifstream fence(fence_data, std::ios::binary);
    std::ifstream bloom(bloom_data, std::ios::binary);

    if (!meta.is_open() || !fence.is_open() || !bloom.is_open()) {
        std::cerr << "Failed to open the files." << std::endl;
    }

    std::string word;
    int lsm_meta_count = 0;

    // read in the lsm tree meta data (first 4 words.)
    while (lsm_meta_count < 4 && meta >> word) {
        std::cout << "Word " << (wordCount + 1) << ": " << word << std::endl;
        lsm_meta_count++;
    }

    int a; 
    int b; 
    int c; 
    
    while (meta >> word) { // This reads up to the next space or newline

        if 
        std::cout << "Read word: " << word << std::endl;
    }
    meta.close(); 
    fence.close(); 
    bloom.close();

};


void memory_load_save(){


};





int main(int argc, char *argv[]){
    int opt; 
    size_t buffer_size, test_size; 

    // if (argc > 1) {
    //     // Loop through all arguments (skipping argv[0], which is the program name)
    //     buffer_size = std::stoi(argv[1]);

    //     std::cout << "buffer size: " << buffer_size << std::endl; 
    // } else {
    //     std::cout << "No command-line arguments provided." << std::endl;
    // }


    // std::cout << "\n basic LSM tree benchmark \n" << std::endl;

    std::string meta_data_path = "lsm_tree_level_meta.txt";
    std::string fence_data_path = "lsm_tree_fence.dat";
    std::string bloom_data_path = "lsm_tree_bloom.dat";
    std::string memory_data_path = "lsm_tree_memory.dat";

    if (fs::exists(meta_data_path) && fs::exists(fence_data_path) && fs::exists(bloom_data_path) && fs::exists(memory_data_path)) {
        std::cout << "All save files are present. Loading data..." << std::endl;
        meta_load_save();
    } else {
        std::cout << "Some data storage files are missing. Database will overwrite all past data." << std::endl;
    }





    LSM_Tree lsm_tree(10, 3, 100000, 1); // 1 mil integer buffer size

    // start time
    auto start = std::chrono::high_resolution_clock::now();

    command_loop(lsm_tree);

    // Record the end time
    auto end = std::chrono::high_resolution_clock::now();
    
    // Calculate the duration in milliseconds
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Output the duration TODO: there is bug in searching. This was giving search errors. 
    // std::cout << lsm_tree.get(search_key)->val<< std::endl;


    // Output the duration
    std::cout << "Program executed in " << duration.count() << " milliseconds." << std::endl;


    /* ------------------------------------
    delete files for house keeping. 
    ------------------------------------ */ 
    
    // fs::path path_to_directory{"./"};

    // // Iterate over the directory
    // for (const auto& entry : fs::directory_iterator(path_to_directory)) {
    //     // Check if the file extension is .dat
    //     if (entry.path().extension() == ".dat") {
    //         // If it is, delete the file
    //         fs::remove(entry.path());
    //         std::cout << "Deleted: " << entry.path() << std::endl;
    //     }
    // }

    return 0;
};