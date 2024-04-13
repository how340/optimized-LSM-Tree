// g++ -std=c++11 -o client client.cpp -pthread

#include "httplib.h"
#include <iostream>
#include <string>

int main() {
    httplib::Client cli("http://localhost:8080");

    std::string userInput;
    std::cout << "Enter a string to send to the server: ";
    std::getline(std::cin, userInput);  // Read a line of text from the user

    auto res = cli.Post("/api/data", userInput, "text/plain");
    if (res) {
        std::cout << "Response from server: " << res->body << std::endl;
    } else {
        std::cout << "Failed to connect or other error occurred." << std::endl;
    }

    return 0;
}
