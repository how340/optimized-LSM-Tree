#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>

#include "run.h"

std::mutex Run_mutex;

// the class access the files that represents a run.
Run::Run(long max_size, float bf_bits_per_entry) :
         max_size(max_size),
         bloom(max_size * bf_bits_per_entry)
{
    size = 0;
    fence_pointers.reserve(max_size / getpagesize());

    // this approach is inspired by ChatGPT
    char tmp_fn_template[] = "./store/runXXXXXX"; 
    mapping_fd = mkstemp(tmp_fn_template); 

    // Check if mkstemp was successful
    assert(mapping_fd != -1);

    // Store the name of the temporary file
    tmp_file = std::string(tmp_fn_template);

    mapping = nullptr;
}

Run::~Run(void) {
    assert(mapping == nullptr);
    remove(tmp_file.c_str());
}


// place holder for interacting with file. 
Entry_t * Run::map_read(size_t len, off_t offset) {
    std::lock_guard<std::mutex> guard(Run_mutex);

}

Entry_t * Run::map_read(void) {
    map_read(max_size * sizeof(Entry_t), 0);
    return mapping;
}
