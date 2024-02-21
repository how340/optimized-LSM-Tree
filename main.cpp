#include "buffer.h"
#include <iostream>

int main(){
    Buffer test(10);

    test.insert(10, 20);
    test.insert(1, 20);
    test.insert(2, 20);
    test.insert(3, 20);
    test.insert(4, 20);
    test.insert(5, 20);
    test.insert(6, 25);
    test.insert(7, 20);
    test.insert(8, 20);
    test.insert(9, 20);
    

    test.get(6);

    test.clear_buffer();

    test.get(1);

    test.insert(1, 20);
    test.insert(1, 10);
     
    std::cout << *(test.get(1)) << std::endl;

    for (int i = 0; i < 15; i++){
        test.insert(i, 20);
        std::cout << i << std::endl;
    }

    test.clear_buffer();

    test.insert(1, 2);
    test.insert(2, 4);
    test.insert(3, 6);
    test.insert(4, 8);
    test.insert(5, 10);

    std::vector<VALUE_t> ret = test.get_range(2, 4);
    for (const auto& value : ret) {
        std::cout << value << '\n';
    }
    return 0;
};