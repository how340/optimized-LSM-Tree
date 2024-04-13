// g++ -std=c++11 -o server server.cpp -pthread
#include "httplib.h"
#include <iostream>

int main() {
    httplib::Server svr;

    svr.Post("/api/data", [](const httplib::Request& req, httplib::Response& res) {
        std::cout << "Received data: " << req.body << std::endl;
        res.set_content(req.body, "text/plain");
    });

    svr.listen("localhost", 8080);
    return 0;
}
