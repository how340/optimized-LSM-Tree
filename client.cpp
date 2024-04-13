#include "lib/httplib.h"
#include <iostream>
#include <string>

int main() {
    httplib::Client cli("http://localhost:1234");
    
    while (true) {
        std::string input;
        std::cout << "Enter command (q to quit): ";
        std::getline(std::cin, input);
        
        if (input == "q") {
            break;  // Exit the loop and end the program
        }

        if (input.empty()) {
            continue;  // Skip empty input
        }

        auto res = cli.Post("/command", input, "text/plain");
        if (res) {
            std::cout << "Response from server: " << res->body << std::endl;
        } else {
            std::cout << "Failed to connect or other error occurred." << std::endl;
        }
    }

    return 0;
}
