// g++ -o client client.cpp -pthread
#include <chrono>
#include <iostream>
#include <string>
#include "lib/httplib.h"

int main() {
  httplib::Client cli("127.0.0.1", 8080);

  while (true) {
    std::string input;
    std::cout << "Enter command (q to quit): ";
    std::getline(std::cin, input);

    if (input.empty()) {
      continue;  // Skip empty input
    }

    auto res = cli.Post("/post", input, "text/plain");
    if (res) {
      if (input == "q") {
        break;
      }

      auto start = std::chrono::high_resolution_clock::now();
      std::cout << "Response from server: " << res->body << std::endl;

      std::this_thread::sleep_for(std::chrono::milliseconds(200));
      std::string task_id = res->body;
      // periodically check for results
      while (true) {
        std::string url = "/status?id=" + task_id;
        auto get_res = cli.Get(url.c_str());

        if (get_res->body == "Processing" ||
            get_res->body == "Task ID not found") {
          std::cout << "waiting for server.." << std::endl;
          std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        } else if (get_res->body == "shutdown" ||
                   get_res->body == "shutdown-c") {
          std::cout << "Shutting down client..." << std::endl;
          return 0;  // Exit the client loop
        } else {
          std::cout << get_res->body << std::endl;

          auto end = std::chrono::high_resolution_clock::now();
          auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
              end - start);
          std::cout << duration.count() << " milliseconds." << std::endl;
          break;
        }
      }
    } else {
      std::cout << "Failed to connect or other error occurred.\n";
    }
  }
}

/* Purely for running the load experiement */
// #include <chrono>   // This header is required for std::chrono
// #include <fstream>  // Include for file operations
// #include <iostream>
// #include <string>
// #include <thread>  // This header is required for std::this_thread
// #include "lib/httplib.h"

// int main(int argc, char* argv[]) {
//   httplib::Client cli("127.0.0.1", 8080);
//   std::ifstream file(argv[1]);  // Open the file passed as the first argument

//   if (!file.is_open()) {
//     std::cerr << "Failed to open file." << std::endl;
//     return 1;
//   }

//   std::string input;
//   while (getline(file, input)) {  // Read each line from the file
//     if (input.empty()) {
//       continue;  // Skip empty input
//     }
//     if (input == "cq"){
//       break;
//     }

//     auto res = cli.Post("/post", input, "text/plain");
//     if (input == "q"){
//       break;
//     }
//     if (res) {
//       std::cout << "Response from server: " << res->body << std::endl;

//       std::this_thread::sleep_for(std::chrono::milliseconds(200));
//       std::string task_id = res->body;
//       // periodically check for results
//       while (true) {
//         std::string url = "/status?id=" + task_id;
//         auto get_res = cli.Get(url.c_str());

//         if (get_res->body == "Processing") {
//           std::cout << "waiting for server.." << std::endl;
//           std::this_thread::sleep_for(std::chrono::milliseconds(200));
//         } else if (get_res->body == "shutdown" ||
//                    get_res->body == "shutdown-c") {
//           std::cout << "Shutting down client..." << std::endl;
//           return 0;  // Exit the client loop
//         } else if (get_res->body == "Task ID not found") {
//           std::this_thread::sleep_for(std::chrono::milliseconds(200));
//         } else {
//           std::cout << get_res->body << std::endl;
//           break;
//         }
//       }
//     } else {
//       std::cout << "Failed to connect or other error occurred.\n";
//     }
//   }
//   file.close();  // Close the file
// }
