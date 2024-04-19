#include "key_value.h"
#include "level_run.h"

int main(int argc, char *argv[])
{
    ThreadPool pool(4);
    Level_Run test_run(pool);

    test_run.print();

    std::vector<Entry_t> buffer;

    Entry_t entry_1;
    entry_1.key = 5;
    entry_1.val = 7;

    Entry_t entry_2;
    entry_2.key =30;
    entry_1.val = 17;

    buffer.push_back(entry_1);
    buffer.push_back(entry_2);

    test_run.insert_block(buffer);

    test_run.print();
}
