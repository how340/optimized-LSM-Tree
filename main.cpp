// g++ -g  main.cpp bloom.cpp run.cpp lsm_tree.cpp sys.cpp -o program
#include <filesystem>
#include <iostream>
#include <sstream>

#include "buffer_level.h"
#include "lsm_tree.h"
#include "run.h"
#include "sys.h"
#include "level_run.h"

namespace fs = std::filesystem;

void command_loop(LSM_Tree *tree)
{
    char command;
    KEY_t key_a, key_b;
    VALUE_t val;

    std::chrono::milliseconds totalDuration(0);

    while (std::cin >> command)
    {
        if (command == 'q')
        { // Assuming 'q' is the command to quit
            tree->exit_save();
            tree->print_statistics();
            break; // Exit the loop
        }

        switch (command)
        {
        case 'p': { // put
            std::cin >> key_a >> val;
            if (val < MIN_VAL || val > MAX_VAL)
            {
                std::cout << "Could not insert value " << std::to_string(val) << ": out of range." << std::endl;
            }
            else
            {
                tree->put(key_a, val);
            }
            break;
        }
        case 'g': { // get
            std::cin >> key_a;
            if (key_a < MIN_VAL || key_a > MAX_VAL)
            {
                std::cout << "Could not search value " << std::to_string(key_a) << ": out of range." << std::endl;
            }
            std::unique_ptr<Entry> entry = tree->get(key_a);
            if (entry && !entry->del){
                std::cout << *entry << std::endl;
            } else {
                std::cout << "Not found" << std::endl; 
            }
            break;
        }
        case 'r': { // range
            std::cin >> key_a >> key_b;
            // TODO: add check to make sure key_a is less than key_b.
            std::vector<Entry_t> ret;

            if (key_a < MIN_VAL || key_a > MAX_VAL)
            {
                std::cout << "Could not search value " << std::to_string(key_a) << ": out of range." << std::endl;
            }
            else if (key_b < MIN_VAL || key_b > MAX_VAL)
            {
                std::cout << "Could not search value " << std::to_string(key_b) << ": out of range." << std::endl;
            }
            else
            {
                ret = tree->range(key_a, key_b);
            }

            // if (ret.size() > 0)
            // {
            //     std::cout << "Range found" << std::endl;
            // }

            for (Entry_t entry : ret)
            {
                if (!entry.del)
                {
                    std::cout << entry.key << ":" << entry.val << std::endl;
                }
                else
                {
                    std::cout << "Not found (deleted)" << std::endl;
                }
            }
            break;
        }
        case 'd': { // delete
            std::cin >> key_a;
            tree->del(key_a);
            break;
        }
        case 'l': { // load from binary file.
            std::string file_path;
            std::cin >> file_path;

            std::ifstream file(file_path, std::ios::binary);
            if (!file)
            {
                std::cerr << "Cannot open file: " << file_path << std::endl;
            }

            while (file.read(reinterpret_cast<char *>(&key_a), sizeof(key_a)) &&
                   file.read(reinterpret_cast<char *>(&val), sizeof(val)))
            {
                tree->put(key_a, val);
            }

            file.close();
            break;
        }
        case 's': { // print current LSM tree view
            tree->print();
            break;
        }
        default:
            die("Invalid command.");
            break;
        }
    }
}

// load data on start up.
LSM_Tree *meta_load_save()
{
    std::string meta_data = "lsm_tree_level_meta.txt";
    std::ifstream meta(meta_data);

    if (!meta.is_open())
    {
        std::cerr << "Failed to open the meta file." << std::endl;
    }

    std::string line;

    // read in the lsm tree meta data (in first line)
    int bits_per_entry, level_ratio, buffer_size, mode, threads;
    std::string a, b, c, d, e;
    if (std::getline(meta, line))
    {
        std::istringstream iss(line);
        if (!(iss >> a >> b >> c >> d >> e))
        {
            std::cerr << "error loading lsm isntance meta data" << std::endl;
        };
        bits_per_entry = std::stoi(a);
        level_ratio = std::stoi(b);
        buffer_size = std::stoi(c);
        mode = std::stoi(d);
        threads = std::stoi(e);
    }
    else
    {
        std::cout << "Error reading LSM tree instance meta data" << std::endl;
    }

    LSM_Tree *lsm_tree = new LSM_Tree(bits_per_entry, level_ratio, buffer_size, mode, threads);

    lsm_tree->load_memory();
    lsm_tree->reconstruct_file_structure(meta);

    meta.close();
    return lsm_tree;
};

int main(int argc, char *argv[])
{
    int opt;
    size_t buffer_size, test_size;

    // if (argc > 1) {
    //     // Loop through all arguments (skipping argv[0], which is the program
    //     name) buffer_size = std::stoi(argv[1]);

    //     std::cout << "buffer size: " << buffer_size << std::endl;
    // } else {
    //     std::cout << "No command-line arguments provided." << std::endl;
    // }

    // std::cout << "\n basic LSM tree benchmark \n" << std::endl;
    std::string meta_data_path = "lsm_tree_level_meta.txt";
    std::string memory_data_path = "lsm_tree_memory.dat";

    LSM_Tree *lsm_tree;

    if (fs::exists(meta_data_path) && fs::exists(memory_data_path))
    {
        std::cout << "All save files are present. Loading data..." << std::endl;
        lsm_tree = meta_load_save();
    }
    else
    {
        float bits_per_entry;
        int level_ratio, buffer_size, mode, threads;
        // std::cin >> bits_per_entry >> level_ratio >> buffer_size >> mode >> threads;

        bits_per_entry = 0.00001;// there is a floating point exception when this value becomes larger than 0.001. 
        level_ratio = 3;
        buffer_size = 100000;
        mode = 1;
        threads = 1;

        lsm_tree = new LSM_Tree(bits_per_entry, level_ratio, buffer_size, mode,
                                threads); // 1 mil integer buffer size. MAKING THIS # for testing haha.
    }

    /*
        Testing without command loop.
    */

    // start time
    command_loop(lsm_tree);

    /* ------------------------------------
    delete files for house keeping.
    ------------------------------------ */

    fs::path path_to_directory{"./"};

    // Iterate over the directory
    for (const auto &entry : fs::directory_iterator(path_to_directory))
    {
        // Check if the file extension is .dat
        if (entry.path().extension() == ".dat" || entry.path().extension() == ".txt")
        {
            // If it is, delete the file
            fs::remove(entry.path());
        }
    }

    return 0;
};