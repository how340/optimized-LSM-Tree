#include "key_value.h"
#include "level_run.h"

int main(int argc, char *argv[])
{
    ThreadPool pool(4);
    Level_Run test_run(pool);


    std::vector<Entry_t> buffer;
    Entry_t entry;
    for (int i = 0; i < 1000; i ++){
        entry.key = i; 
        entry.val = i * 100; 
        buffer.push_back(entry);
    }

    test_run.insert_block(buffer);

    test_run.print();
}
