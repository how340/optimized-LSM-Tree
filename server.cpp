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

// namespace fs = std::filesystem;

// void command_loop(LSM_Tree *tree)
// {
//     char command;
//     KEY_t key_a, key_b;
//     VALUE_t val;

//     std::chrono::milliseconds totalDuration(0);

//     while (std::cin >> command)
//     {
//         if (command == 'q')
//         { // Assuming 'q' is the command to quit
//             tree->exit_save();
//             break; // Exit the loop
//         }

//         switch (command)
//         {
//         case 'p': { // put
//             std::cin >> key_a >> val;
//             if (val < MIN_VAL || val > MAX_VAL)
//             {
//                 die("Could not insert value " + std::to_string(val) + ": out of range.");
//             }
//             else
//             {
//                 tree->put(key_a, val);
//             }
//             break;
//         }
//         case 'g': { // get
//             std::cin >> key_a;

//             auto timer_s = std::chrono::high_resolution_clock::now();

//             tree->get(key_a);

//             auto timer_e = std::chrono::high_resolution_clock::now();
//             auto timer_duration = std::chrono::duration_cast<std::chrono::milliseconds>(timer_e - timer_s);
//             totalDuration += timer_duration;

//             std::cout << "total lookup took: " << totalDuration.count() << " milliseconds." << std::endl;

//             break;
//         }
//         case 'r': { // range
//             std::cin >> key_a >> key_b;
//             // TODO: add check to make sure key_a is less than key_b.
//             std::vector<Entry_t> ret;

//             if (key_a < MIN_VAL || key_a > MAX_VAL)
//             {
//                 die("Could not insert value " + std::to_string(key_a) + ": out of range.");
//             }
//             else if (key_b < MIN_VAL || key_b > MAX_VAL)
//             {
//                 die("Could not insert value " + std::to_string(key_b) + ": out of range.");
//             }
//             else
//             {
//                 ret = tree->range(key_a, key_b);
//             }

//             for (Entry_t entry : ret)
//             {
//                 if (!entry.del)
//                 {
//                     std::cout << entry.key << ":" << entry.val << std::endl;
//                 }
//                 else
//                 {
//                     std::cout << "Not found (deleted)" << std::endl;
//                 }
//             }
//             break;
//         }
//         case 'd': { // delete
//             std::cin >> key_a;
//             tree->del(key_a);
//             break;
//         }
//         case 'l': { // load from binary file.
//             std::string file_path;
//             std::cin >> file_path;

//             std::ifstream file(file_path, std::ios::binary);
//             if (!file)
//             {
//                 std::cerr << "Cannot open file: " << file_path << std::endl;
//             }

//             while (file.read(reinterpret_cast<char *>(&key_a), sizeof(key_a)) &&
//                    file.read(reinterpret_cast<char *>(&val), sizeof(val)))
//             {
//                 tree->put(key_a, val);
//             }

//             file.close();
//             break;
//         }
//         case 's': { // print current LSM tree view
//             tree->print();
//             break;
//         }
//         default:
//             die("Invalid command.");
//             break;
//         }
//     }
// }

// int main()
// {
//     httplib::Server svr;

//     svr.Post("/api/data", [](const httplib::Request &req, httplib::Response &res) {
//         std::cout << "Received data: " << req.body << std::endl;
//         res.set_content(req.body, "text/plain");
//     });

//     LSM_Tree *lsm_tree = new LSM_Tree(10, 3, 100000, 1);

//     command_loop(lsm_tree);

//     svr.listen("localhost", 8080);
//     return 0;
// }

void http_command_processor(LSM_Tree* tree) {
    httplib::Server svr;

    svr.Post("/command", [tree](const httplib::Request& req, httplib::Response& res) {
        std::istringstream iss(req.body);
        std::string command;
        KEY_t key_a, key_b;
        VALUE_t val;

        iss >> command;

        if (command == "q") {
            tree->exit_save();
            res.set_content("Exiting and saving.", "text/plain");
            return;
        }

        try {
            if (command == "p") { // put
                iss >> key_a >> val;
                tree->put(key_a, val);
                res.set_content("Put operation successful.", "text/plain");
            } else if (command == "g") { // get
                iss >> key_a;
                auto value = tree->get(key_a);
                res.set_content("Got value: " + std::to_string(value->val), "text/plain");
            } else if (command == "r") { // range
                iss >> key_a >> key_b;
                auto range_values = tree->range(key_a, key_b);
                std::stringstream ss;
                for (const auto& entry : range_values) {
                    ss << entry.key << ":" << entry.val << " ";
                }
                res.set_content("Range values: " + ss.str(), "text/plain");
            } else if (command == "d") { // delete
                iss >> key_a;
                tree->del(key_a);
                res.set_content("Delete operation successful.", "text/plain");
            } else if (command == "l") { // load
                std::string file_path;
                iss >> file_path;
                std::ifstream file(file_path, std::ios::binary);
                if (!file) {
                    throw std::runtime_error("Cannot open file: " + file_path);
                }
                while (file.read(reinterpret_cast<char *>(&key_a), sizeof(key_a)) &&
                       file.read(reinterpret_cast<char *>(&val), sizeof(val))) {
                    tree->put(key_a, val);
                }
                file.close();
                res.set_content("Data loaded from file.", "text/plain");
            } else if (command == "s") { // print structure
                tree->print();
                res.set_content("Printed tree structure.", "text/plain");
            } else {
                throw std::runtime_error("Invalid command.");
            }
        } catch (const std::exception& e) {
            res.status = 400; // Bad Request
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
            return;
        }
    });

    svr.listen("localhost", 1234);
}

int main() {
    LSM_Tree tree(10, 3, 100000, 1);

    http_command_processor(&tree);
    return 0;
}
