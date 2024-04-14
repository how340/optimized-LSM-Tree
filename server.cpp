// g++ -std=c++11 -o server server.cpp -pthread
#include "lib/httplib.h"
#include <string>

#include <filesystem>
#include <iostream>
#include <sstream>

#include "buffer_level.h"
#include "lsm_tree.h"
#include "run.h"
#include "sys.h"

namespace fs = std::filesystem;

void http_command_processor(LSM_Tree *tree)
{
    httplib::Server svr;

    svr.Post("/command", [&svr,tree](const httplib::Request &req, httplib::Response &res) {
        std::istringstream iss(req.body);
        std::string command;
        KEY_t key_a, key_b;
        VALUE_t val;

        iss >> command;

        if (command == "q") {
            tree->exit_save();
            res.set_content("shutdown", "text/plain");  // Special shutdown message
            svr.stop();
            return;
        }

        try
        {
            if (command == "p")
            { // put
                iss >> key_a >> val;
                if (val < MIN_VAL || val > MAX_VAL)
                {
                    die("Could not insert value " + std::to_string(val) + ": out of range.");
                }
                else if (key_a < MIN_KEY || key_a > MAX_KEY)
                {
                    die("Could not insert value " + std::to_string(val) + ": out of range.");
                }
                else
                {
                    tree->put(key_a, val);
                }
                tree->put(key_a, val);
                res.set_content("Put operation successful.", "text/plain");
            }
            else if (command == "g")
            { // get
                iss >> key_a;
                if (key_a < MIN_KEY || key_a > MAX_KEY)
                {
                    die("Could not insert value " + std::to_string(val) + ": out of range.");
                }
                auto value = tree->get(key_a);

                if (!value)
                { // value did not return
                    res.set_content("Value not found", "text/plain");
                }
                else if (value->del)
                {
                    res.set_content("Value not found (recently deleted)", "text/plain");
                }
                else
                {
                    res.set_content("Got value: " + std::to_string(value->val), "text/plain");
                }
            }
            else if (command == "r")
            { // range
                iss >> key_a >> key_b;
                auto range_values = tree->range(key_a, key_b);
                std::stringstream ss;
                if (range_values.size() == 0)
                {
                    res.set_content("No value within range found", "text/plain");
                }
                else
                {
                    for (const auto &entry : range_values)
                    {
                        ss << entry.key << ":" << entry.val << " ";
                    }
                    res.set_content("Range values: " + ss.str(), "text/plain");
                }
            }
            else if (command == "d")
            { // delete
                iss >> key_a;
                tree->del(key_a);
                res.set_content("Delete operation successful.", "text/plain");
            }
            else if (command == "l")
            { // load
                std::string file_path;
                iss >> file_path;
                std::ifstream file(file_path, std::ios::binary);
                if (!file)
                {
                    throw std::runtime_error("Cannot open file: " + file_path);
                }
                while (file.read(reinterpret_cast<char *>(&key_a), sizeof(key_a)) &&
                       file.read(reinterpret_cast<char *>(&val), sizeof(val)))
                {
                    tree->put(key_a, val);
                }
                file.close();
                res.set_content("Data loaded from file.", "text/plain");
            }
            else if (command == "s")
            { // print structure
                tree->print();
                res.set_content("Printed tree structure.", "text/plain");
            }
            else
            {
                throw std::runtime_error("Invalid command.");
            }
        }
        catch (const std::exception &e)
        {
            res.status = 400; // Bad Request
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
            return;
        }
    });

    svr.listen("localhost", 1234);
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
    int bits_per_entry, level_ratio, buffer_size, mode;
    std::string a, b, c, d;
    if (std::getline(meta, line))
    {
        std::istringstream iss(line);
        if (!(iss >> a >> b >> c >> d))
        {
            std::cerr << "error loading lsm isntance meta data" << std::endl;
        };
        bits_per_entry = std::stoi(a);
        level_ratio = std::stoi(b);
        buffer_size = std::stoi(c);
        mode = std::stoi(d);
    }
    else
    {
        std::cout << "Error reading LSM tree instance meta data" << std::endl;
    }

    LSM_Tree *lsm_tree = new LSM_Tree(bits_per_entry, level_ratio, buffer_size, mode);

    lsm_tree->load_memory();
    lsm_tree->reconstruct_file_structure(meta);

    meta.close();
    return lsm_tree;
};

int main()
{
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
        std::cout << "Some data storage files are missing. Database will overwrite "
                     "all past data."
                  << std::endl;
        lsm_tree = new LSM_Tree(10, 3, 100000,
                                1); // 1 mil integer buffer size. MAKING THIS # for testing haha.
    }

    http_command_processor(lsm_tree);

    delete lsm_tree; 

    return 0;
}
