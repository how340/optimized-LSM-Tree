// g++ -std=c++17 -I /Users/hongkaiwang/opt/boost_1_67_0 -g lsm_tree.cpp
// level_run.cpp bloom.cpp server.cpp run.cpp -w -o server
#include <string>
#include "lib/httplib.h"

#include <filesystem>
#include <iostream>
#include <mutex>
#include <sstream>

#include "buffer_level.h"
#include "lsm_tree.h"
#include "run.h"
#include "task_queue.h"

namespace fs = std::filesystem;

std::mutex tree_mutex;  // Global mutex for LSM_Tree access

std::string http_command_process(LSM_Tree* tree, std::string input) {
  // create file stream for handling the command.
  std::stringstream ss(input);
  std::string command;
  KEY_t key_a, key_b;
  VALUE_t val;
  std::string return_msg;

  ss >> command;

  if (command == "q") {
    tree->exit_save();
    std::cout << "shutting down..." << std::endl;
    return_msg = "shutdown";
    return return_msg;
  }

  if (command == "cq") {
    std::cout << "shutting down client" << std::endl;
    return_msg = "shutdown-c";
    return return_msg;
  }

  try {
    if (command == "p") {  // put
      ss >> key_a >> val;
      tree->put(key_a, val);
      return_msg = "Key-value set!";
    } else if (command == "g") {  // get
      ss >> key_a;
      auto value = tree->get(key_a);
      if (value) {
        std::stringstream oss;
        oss << *value << " found!";
        return_msg = oss.str();
      } else {
        return_msg = "Key not found!";
      }
    } else if (command == "r") {  // range
      ss >> key_a >> key_b;

      if (key_b < key_a) {  // make sure key a is less than key b.
        KEY_t tmp = key_a;
        key_a = key_b;
        key_b = key_a;
      }

      auto range_values = tree->range(key_a, key_b);
      std::stringstream oss;  // in case i want to return this later.
      for (const auto& entry : range_values) {
        oss << entry.key << ":" << entry.val << std::endl;
      }
      return_msg = oss.str();
    } else if (command == "d") {  // delete
      ss >> key_a;
      tree->del(key_a);
      return_msg = "Key deleted";

    } else if (command == "l") {  // load
      std::string file_path;

      ss >> file_path;
      std::ifstream file(file_path, std::ios::binary);

      if (!file) {
        throw std::runtime_error("Cannot open file: " + file_path);
      }

      file.seekg(0, std::ios::end);
      size_t file_size = file.tellg();
      file.seekg(0, std::ios::beg);

      std::vector<char> file_data(file_size);
      if (!file.read(file_data.data(), file_size)) {
        std::cerr << "Error reading file.\n";
      }
      // read in the run's information.
      int idx = 0;
      while (file_size > 0) {
        Entry_t entry;
        std::memcpy(&entry.key, &file_data[idx], sizeof(KEY_t));
        idx += sizeof(KEY_t);
        std::memcpy(&entry.val, &file_data[idx], sizeof(VALUE_t));
        idx += sizeof(VALUE_t);

        tree->put(entry.key, entry.val);
        file_size = file_size - (sizeof(VALUE_t) + sizeof(KEY_t));
      }

      file.close();
      return_msg = "file loaded";
    } else if (command == "s") {  // print structure
      return_msg = tree->print();
    }
  } catch (const std::exception& e) {
    return_msg = "Invalid command. Try again!";
    return return_msg;
  };

  return return_msg;
}
void taskProcessor(LSM_Tree* tree,
                   MyTaskQueue& taskQueue,
                   std::map<std::string, std::string>& results) {
  std::mutex results_mutex;
  while (true) {
    Task task;
    if (taskQueue.pop(task)) {
      std::cout << "Processing task with data: " << task.data << std::endl;
      auto start = std::chrono::high_resolution_clock::now();
      std::string ret = http_command_process(tree, task.data);
      results_mutex.lock();
      results[task.id] = ret;
      results_mutex.unlock();

      auto end = std::chrono::high_resolution_clock::now();
      auto duration =
          std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
      std::cout << duration.count() << " milliseconds." << std::endl;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(
          100));  // Sleep briefly if no task is available
    }
  }
}
std::string generate_unique_id() {
  unsigned long long counter;
  std::string base;
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  auto millis =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  base = std::to_string(millis);

  std::stringstream ss;
  ss << base << "-" << std::setw(5) << std::setfill('0') << counter++;

  return ss.str();
}
int main() {
  using namespace httplib;

  /********************************************************
   *                  code for running the server.
   ********************************************************/
  Server svr;
  // Global task queue
  MyTaskQueue taskQueue;
  std::map<std::string, std::string> results;
  std::mutex results_mutex;

  // Add in load data meta data files.
  LSM_Tree* lsm_tree = new LSM_Tree(0.0001, 10, 10000, 0, 8, 0);

  // POST endpoint
  svr.Post("/post", [&](const Request& req, Response& res) {
    std::string command = req.body;  // Command is received in the body

    if (req.body.empty()) {
      res.set_content("Received empty data", "text/plain");
    } else {
      if (command == "q") {
        res.set_content("shutdown", "text/plain");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        svr.stop();  // Stop the server
      }
      std::string task_id = generate_unique_id();
      Task task;
      task.id = task_id;
      task.data = command;

      taskQueue.push(task);
      taskQueue.printAndRestore();

      results_mutex.lock();
      results[task_id] = "Processing";
      results_mutex.unlock();

      res.set_content(task_id, "text/plain");
    }
  });

  svr.Get("/status", [&](const Request& req, Response& res) {
    auto id = req.get_param_value("id");
    std::lock_guard<std::mutex> lock(results_mutex);
    if (results.find(id) != results.end()) {
      res.set_content(results[id], "text/plain");
      results.erase(
          id);  // remove the info from results to keep memory overhead low.
    } else {
      res.set_content("Task ID not found", "text/plain");
    }
  });

  // Start the task processor thread
  std::thread worker(
      [&]() { taskProcessor(lsm_tree, std::ref(taskQueue), results); });
  worker.detach();

  svr.listen("127.0.0.1", 8080);

  delete lsm_tree;

  return 0;
}

// void http_command_processor(LSM_Tree* tree, std::atomic<bool>&
// server_running) {
//   httplib::Server svr;

//   svr.Post("/command", [tree, &server_running](const httplib::Request& req,
//                                                httplib::Response& res) {
//     std::istringstream iss(req.body);
//     std::string command;
//     KEY_t key_a, key_b;
//     VALUE_t val;

//     iss >> command;

//     if (command == "q") {
//       tree->exit_save();
//       res.set_content("shutdown", "text/plain");  // Special shutdown message
//       server_running = false;
//       return;
//     }

//     try {
//       if (command == "p") {  // put
//         iss >> key_a >> val;
//         if (val < MIN_VAL || val > MAX_VAL) {
//           die("Could not insert value " + std::to_string(val) +
//               ": out of range.");
//         } else if (key_a < MIN_KEY || key_a > MAX_KEY) {
//           die("Could not insert value " + std::to_string(val) +
//               ": out of range.");
//         } else {
//           tree->put(key_a, val);
//         }
//         res.set_content("Put operation successful.", "text/plain");
//       } else if (command == "g") {  // get
//         iss >> key_a;
//         if (key_a < MIN_KEY || key_a > MAX_KEY) {
//           die("Could not get value " + std::to_string(key_a) +
//               ": out of range.");
//         }
//         auto value = tree->get(key_a);

//         if (!value) {  // value did not return
//           res.set_content("Value not found", "text/plain");
//         } else if (value->del) {
//           res.set_content("Value not found (recently deleted)",
//           "text/plain");
//         } else {
//           res.set_content("Got value: " + std::to_string(value->val),
//                           "text/plain");
//         }
//       } else if (command == "r") {  // range
//         iss >> key_a >> key_b;
//         auto range_values = tree->range(key_a, key_b);
//         std::stringstream ss;
//         if (range_values.size() == 0) {
//           res.set_content("No value within range found", "text/plain");
//         } else {
//           for (const auto& entry : range_values) {
//             ss << entry.key << ":" << entry.val << " ";
//           }
//           res.set_content("Range values: " + ss.str(), "text/plain");
//         }
//       } else if (command == "d") {  // delete
//         iss >> key_a;
//         tree->del(key_a);
//         res.set_content("Delete operation successful.", "text/plain");
//       } else if (command == "l") {  // load
//         std::string file_path;
//         iss >> file_path;
//         std::ifstream file(file_path, std::ios::binary);

//         if (!file) {
//           throw std::runtime_error("Cannot open file: " + file_path);
//         }

//         file.seekg(0, std::ios::end);
//         size_t file_size = file.tellg();
//         file.seekg(0, std::ios::beg);

//         std::vector<char> file_data(file_size);
//         if (!file.read(file_data.data(), file_size)) {
//           std::cerr << "Error reading file.\n";
//         }
//         // read in the run's information.
//         int idx = 0;
//         while (file_size > 0) {
//           Entry_t entry;
//           std::memcpy(&entry.key, &file_data[idx], sizeof(KEY_t));
//           idx += sizeof(KEY_t);
//           std::memcpy(&entry.val, &file_data[idx], sizeof(VALUE_t));
//           idx += sizeof(VALUE_t);

//           tree->put(entry.key, entry.val);
//           file_size = file_size - (sizeof(VALUE_t) + sizeof(KEY_t));
//         }

//         file.close();
//         res.set_content("Data loaded from file.", "text/plain");
//       } else if (command == "s") {  // print structure
//         tree->print();
//         res.set_content("Printed tree structure.", "text/plain");
//       } else {
//         throw std::runtime_error("Invalid command.");
//       }
//     } catch (const std::exception& e) {
//       res.status = 400;  // Bad Request
//       res.set_content(std::string("Error: ") + e.what(), "text/plain");
//       return;
//     };
//   });

//     svr.listen("localhost", 1234);
// }

// load data on start up.
// // load data on start up.

// LSM_Tree* meta_load_save() {
//   std::string meta_data = "lsm_tree_level_meta.txt";
//   std::ifstream meta(meta_data);

//   if (!meta.is_open()) {
//     std::cerr << "Failed to open the meta file." << std::endl;
//   }

//   std::string line;

//   // read in the lsm tree meta data (in first line)
//   float bits_per_entry;
//   int level_ratio, buffer_size, mode, threads, partition;
//   std::string a, b, c, d, e, f;
//   if (std::getline(meta, line)) {
//     std::istringstream iss(line);
//     if (!(iss >> a >> b >> c >> d >> e >> f)) {
//       std::cerr << "error loading lsm isntance meta data" << std::endl;
//     };
//     bits_per_entry = std::stof(a);
//     level_ratio = std::stoi(b);
//     buffer_size = std::stoi(c);
//     mode = std::stoi(d);
//     threads = std::stoi(e);
//     partition = std::stoi(f);
//   } else {
//     std::cout << "Error reading LSM tree instance meta data" << std::endl;
//   }

//   LSM_Tree* lsm_tree = new LSM_Tree(bits_per_entry, level_ratio, buffer_size,
//                                     mode, threads, partition);

//   lsm_tree->load_memory();
//   lsm_tree->reconstruct_file_structure(meta);
//   // std::cout << "done with meta read" << std::endl;
//   meta.close();
//   return lsm_tree;
// };

// int main() {
//   std::string meta_data_path = "lsm_tree_level_meta.txt";
//   std::string memory_data_path = "lsm_tree_memory.dat";

//   LSM_Tree* lsm_tree;

//   if (fs::exists(meta_data_path) && fs::exists(memory_data_path)) {
//     std::cout << "All save files are present. Loading data..." << std::endl;
//     lsm_tree = meta_load_save();
//   } else {
//     std::cout << "Some data storage files are missing. Database will
//     overwrite "
//                  "all past data."
//               << std::endl;

//     lsm_tree = new LSM_Tree(
//         0.0001, 10, 100000, 0, 8,
//         0);  // 1 mil integer buffer size. MAKING THIS # for testing haha.
//   }
//   std::atomic<bool> server_running(true);
//   http_command_processor(lsm_tree, server_running);

//   delete lsm_tree;

//   return 0;
// }