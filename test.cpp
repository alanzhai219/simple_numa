#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <numaif.h>
#include <errno.h>
#include <sys/mman.h>
#include <vector>

#include <cstdint> // for uintptr_t
#include <cstddef> // for size_t

void* align_pointer(void* data, size_t pagesize) {
    return reinterpret_cast<void*>(
        reinterpret_cast<uintptr_t>(data) & ~(pagesize - 1));
}


void check_memory_node(void *addr, size_t length) {
    const size_t page_size = getpagesize();
    const size_t page_nums = length / page_size;

    std::vector<int> nodes(page_nums, -1);
    std::vector<void*> pages(page_nums);

    int flags = 0;
    // int nodes[length / getpagesize()];
    // void *pages[length / getpagesize()];

    // Initialize the pages array with the address
    for (size_t i = 0; i < page_nums; i++) {
        pages[i] = addr + i * page_size;
    }

    // Use move_pages to get the location of memory pages
    int result = move_pages(0, page_nums, pages.data(), NULL, nodes.data(), flags);
    
    if (result == -1) {
        perror("move_pages");
        return;
    }

    // Print the node information for each page
    for (size_t i = 0; i < page_nums; i++) {
        printf("Page %zu is on node %d\n", i, nodes[i]);
    }
}

void move_memory_to_node(void *addr, size_t length, int target_node) {
    const size_t page_size = getpagesize();
    const size_t page_nums = length / page_size / 2;

    // prepare the arguments
    std::vector<void*> pages(page_nums);
    std::vector<int> nodes(page_nums, target_node);
    std::vector<int> status(page_nums);
    // void *pages[page_nums];
    // int nodes[page_nums];

    // Initialize the pages array with the address and nodes array with target node
    for (size_t i = 0; i < page_nums; i++) {
        pages[i] = addr + i * page_size;
    }

    // Use move_pages to move the memory pages to the target node
    int result = move_pages(0, page_nums, pages.data(), nodes.data(), status.data(), MPOL_MF_MOVE);
    
    if (result == -1) {
        perror("move_pages");
        return;
    }

    printf("Moved memory to node %d\n", target_node);
}

void bind_memory_to_node(void* addr, size_t length, int target_node) {
    const size_t page_size = getpagesize();
    const size_t page_nums = (length + page_size - 1) / page_size / 2;
    void* pages = align_pointer(addr, page_size);
    unsigned long mask = 0;
    unsigned flag = 0;
    if (target_node < 0) {
        mask = -1;
        flag = 0;
    } else {
        mask = 1ul << target_node;
        flag = MPOL_MF_MOVE | MPOL_MF_STRICT;
    }

    auto rc = mbind(pages,
                    page_size * page_nums,
                    MPOL_BIND,
                    &mask,
                    sizeof(mask)*8,
                    flag);
    if (rc < 0) {
        perror("mbind failed.");
        exit(1);
    }
}

int main() {
    const size_t page_nums = 10;
    size_t page_size = getpagesize();
    size_t length = page_nums * page_size; // 10 pages of memory
    void *memory = malloc(length);
    // char* memory = (char*)mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (memory == NULL) {
        perror("malloc");
        return 1;
    }

    // Initialize memory to ensure it's actually allocated
    memset(memory, 0, length);
    // for (size_t i = 0; i < page_nums; ++i) {
    //     memory[i] = 0;
    // }
    // check the location
    check_memory_node(memory, length);

    // Move the memory to a different NUMA node (e.g., node 1)
    int target_node = 1; // Replace with your desired target node
    // move_memory_to_node(memory, length, target_node);
    bind_memory_to_node(memory, length, target_node);

    // check the location
    check_memory_node(memory, length);

    free(memory);
    // munmap(memory, length);
    return 0;
}

