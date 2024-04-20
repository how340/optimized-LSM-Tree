#include "key_value.h"
#include "level_run.h"

int main(int argc, char *argv[])
{
    ThreadPool pool(4);
    Level_Run test_run(pool);

    std::vector<Entry_t> buffer;
    Entry_t entry;
    for (int i = 0; i < 1000; i++)
    {
        entry.key = i;
        entry.val = i * 100;
        buffer.push_back(entry);
    }

    test_run.insert_block(buffer);

    test_run.print();

    buffer.clear();

    for (int i = 700; i < 1500; i++)
    {
        entry.key = i;
        entry.val = i * 50;
        buffer.push_back(entry);
    }

    test_run.insert_block(buffer);
    buffer.clear(); 

    test_run.print();

    for (int i = 1600; i < 1800; i++)
    {
        entry.key = i;
        entry.val = i * 70;
        buffer.push_back(entry);
    }

    test_run.insert_block(buffer);
    buffer.clear(); 
    
    test_run.print();

    for (int i = -100; i < 300; i++)
    {
        entry.key = i;
        entry.val = i * 70;
        buffer.push_back(entry);
    }

    test_run.insert_block(buffer);
    buffer.clear(); 
    
    test_run.print();

    for (int i = -500; i < -200; i++)
    {
        entry.key = i;
        entry.val = i * 70;
        buffer.push_back(entry);
    }

    test_run.insert_block(buffer);
    buffer.clear(); 
    
    test_run.print();

    for (int i = -600; i < 2000; i++)
    {
        entry.key = i;
        entry.val = i * 3;
        buffer.push_back(entry);
    }

    test_run.insert_block(buffer);
    buffer.clear(); 
    
    test_run.print();

    int size = test_run.return_size(); 
    std::cout << "current size: " <<size << std::endl;

    std::cout << *(test_run.get(100)) << std::endl; 

    std::cout << *(test_run.get(-200)) << std::endl; 

    if(test_run.get(2000)){
        std::cout << " found ?"<< std::endl; 
    } else {
        std::cout << "Not found"<< std::endl; 
    }
    
    std::vector<Entry_t> range;

    range = test_run.range_search(100, 300);
    for (Entry_t& entry : range){
        std::cout << entry << std::endl; 
    }
    // test_run.flush(); 
    // test_run.print(); 
    // size = test_run.return_size();
    // std::cout << "current size: " <<size << std::endl;
}
