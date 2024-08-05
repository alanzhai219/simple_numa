#include <iostream>
#include <vector>
#include <cerrno>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <numa.h>
#include <numaif.h>

int main() {
    // Check if the system supports NUMA
    if (numa_available() == -1) {
        std::cerr << "NUMA is not supported on this system." << std::endl;
        return 1;
    }

    // Allocate some memory
    size_t pageSize = getpagesize();
    size_t numPages = 10;
    size_t allocSize = numPages * pageSize;

    char* buffer = (char*)mmap(nullptr, allocSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (buffer == MAP_FAILED) {
        std::cerr << "Memory allocation failed: " << strerror(errno) << std::endl;
        return 1;
    }

    // Touch the pages to ensure they are allocated
    for (size_t i = 0; i < allocSize; i += pageSize) {
        buffer[i] = 0;
    }

    // Prepare the arguments for move_pages
    std::vector<void*> pages(numPages);
    std::vector<int> nodes(numPages, 0);  // Target node 0 for all pages
    std::vector<int> status(numPages);

    for (size_t i = 0; i < numPages; ++i) {
        pages[i] = buffer + i * pageSize;
    }

    // Move the pages
    int result = move_pages(0, numPages, pages.data(), nodes.data(), status.data(), MPOL_MF_MOVE);
    if (result == -1) {
        std::cerr << "move_pages failed: " << strerror(errno) << std::endl;
        munmap(buffer, allocSize);
        return 1;
    }

    // Check the status of each page
    for (size_t i = 0; i < numPages; ++i) {
        if (status[i] < 0) {
            std::cerr << "Page " << i << " could not be moved: " << strerror(-status[i]) << std::endl;
        } else {
            std::cout << "Page " << i << " moved to node " << status[i] << std::endl;
        }
    }

    // Clean up
    munmap(buffer, allocSize);
    return 0;
}

