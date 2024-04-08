#include <filesystem>
#include <iostream>
#include <sstream>

#include "buffer_level.h"
#include "lsm_tree.h"
#include "run.h"
#include "sys.h"

namespace fs = std::filesystem;

/*
List of TODO elements:
1.implement data persistence - create API to handle saving normal files, reading
in saved files, and handling exiting applications.
2. Implement the two other query operators (range, delete). I need to figure out
what to save delete as. (add an additional bit?)
2. Implement a more advanced merge policy to improve performance.
3. Improve the two optimization proposed in the paper.
4. Implement more optimization by myself.
5. multi-threaded support
6. better file I/O approach.

*/

void command_loop(LSM_Tree* tree) {
  char command;
  KEY_t key_a, key_b;
  VALUE_t val;

  std::chrono::milliseconds totalDuration(0);

  while (std::cin >> command) {
    if (command == 'q') {  // Assuming 'q' is the command to quit
      tree->exit_save();
      break;  // Exit the loop
    }

    switch (command) {
      case 'p': {  // put
        std::cin >> key_a >> val;
        if (val < MIN_VAL || val > MAX_VAL) {
          die("Could not insert value " + std::to_string(val) +
              ": out of range.");
        } else {
          tree->put(key_a, val);
        }
        break;
      }
      case 'g': {  // get
        std::cin >> key_a;

        auto timer_s = std::chrono::high_resolution_clock::now();

        tree->get(key_a);

        auto timer_e = std::chrono::high_resolution_clock::now();
        auto timer_duration =
            std::chrono::duration_cast<std::chrono::milliseconds>(timer_e -
                                                                  timer_s);
        totalDuration += timer_duration;

        std::cout << "total lookup took: " << totalDuration.count()
                  << " milliseconds." << std::endl;

        break;
      }
      case 'r': {  // range
        std::cin >> key_a >> key_b;
        // TODO: add check to make sure key_a is less than key_b.
        std::vector<Entry_t> ret;

        if (key_a < MIN_VAL || key_a > MAX_VAL) {
          die("Could not insert value " + std::to_string(key_a) +
              ": out of range.");
        } else if (key_b < MIN_VAL || key_b > MAX_VAL) {
          die("Could not insert value " + std::to_string(key_b) +
              ": out of range.");
        } else {
          ret = tree->range(key_a, key_b);
        }

        for (Entry_t entry : ret) {
          if (!entry.del) {
            std::cout << entry.key << ":" << entry.val << std::endl;
          } else {
            std::cout << "Not found (deleted)" << std::endl;
          }
        }
        break;
      }
      case 'd': {  // delete
        std::cin >> key_a;
        tree->del(key_a);
        break;
      }
      // TODO: add the load operator.
      case 'x': {  // print current LSM tree view
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
LSM_Tree* meta_load_save() {
  std::string meta_data = "lsm_tree_level_meta.txt",
              memory_data = "lsm_tree_memory.dat";
  std::ifstream meta(meta_data), memory(memory_data, std::ios::binary);

  if (!meta.is_open() || !memory.is_open()) {
    std::cerr << "Failed to open the files." << std::endl;
  }

  std::string line;

  // read in the lsm tree meta data (in first line)
  int bit_per_entry, level_ratio, buffer_size, mode;
  std::string a, b, c, d;
  if (std::getline(meta, line)) {
    std::istringstream iss(line);
    if (!(iss >> a >> b >> c >> d)) {
      std::cerr << "error loading lsm isntance meta data" << std::endl;
    };
    bit_per_entry = std::stoi(a);
    level_ratio = std::stoi(b);
    buffer_size = std::stoi(c);
    mode = std::stoi(d);
  } else {
    std::cout << "Error reading LSM tree instance meta data" << std::endl;
  }

  LSM_Tree* lsm_tree =
      new LSM_Tree(bit_per_entry, level_ratio, buffer_size, mode);

  // load in buffer from last instance.
  Entry_t entry;
  while (memory.read(reinterpret_cast<char*>(&entry), sizeof(entry))) {
    lsm_tree->put(entry.key, entry.val);
  }
  // TODO: THERE IS A MEMORY BAD ALLOC BUG HERE!
  // reconstruct the file structures.
  LSM_Tree::Level_Node* cur = lsm_tree->root;
  while (std::getline(meta, line)) {
    std::cout << line << std::endl;
    std::istringstream iss(line);

    if (std::isdigit(line[0])) {
      std::string level, max_run, run_cnt;
      if (!(iss >> level >> max_run >> run_cnt)) {
        break;  // issue here.
      };

      if (level != "0") {
        cur->next_level =
            new LSM_Tree::Level_Node(cur->level + 1, cur->max_num_of_runs);
        cur = cur->next_level;
      }

    } else {
      std::string filename;
      iss >> filename;

      std::ifstream bloom("bloom_" + filename),
          fence("fence_" + filename, std::ios::binary);

      boost::dynamic_bitset<> input_bitarray;
      bloom >> input_bitarray;

      BloomFilter* bloom_filter = new BloomFilter(input_bitarray);

      std::vector<KEY_t>* fence_pointer = new std::vector<KEY_t>;
      KEY_t key;
      while (fence.read(reinterpret_cast<char*>(&key), sizeof(KEY_t))) {
        fence_pointer->push_back(key);
      }

      Run run(filename, bloom_filter, fence_pointer);
      cur->run_storage.push_back(run);
      fence.close();
      bloom.close();
    }
  }
  std::cout << "meta load complete!" << std::endl;
  meta.close();
  memory.close();

  return lsm_tree;
};

int main(int argc, char* argv[]) {
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

  LSM_Tree* lsm_tree;

  if (fs::exists(meta_data_path) && fs::exists(memory_data_path)) {
    std::cout << "All save files are present. Loading data..." << std::endl;
    lsm_tree = meta_load_save();
  } else {
    std::cout << "Some data storage files are missing. Database will overwrite "
                 "all past data."
              << std::endl;
    lsm_tree = new LSM_Tree(
        10, 3, 100000,
        1);  // 1 mil integer buffer size. MAKING THIS # for testing haha.
  }

  /*
      Testing without command loop.
  */

  // start time
  auto start = std::chrono::high_resolution_clock::now();

  command_loop(lsm_tree);

  // Record the end time
  auto end = std::chrono::high_resolution_clock::now();

  // Calculate the duration in milliseconds
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

  // Output the duration
  std::cout << "Program executed in " << duration.count() << " milliseconds."
            << std::endl;

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