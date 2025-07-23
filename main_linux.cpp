#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <string>
#include <sstream>
#include <regex> // Required for parsing malloc_info output

// Include malloc.h to define mallopt
#include <malloc.h>


// Data structure to hold all the metrics we collect at a single point in time.
// This structure is identical to the Windows version.
struct HeapStats {
    int timeStep;
    size_t totalUserRequested;
    size_t totalHeapCommitted;
    size_t internalFragmentation;
    size_t totalFreeOnHeap;
    size_t biggestFreeBlock;
    double externalFragmentationRatio;
};

/**
 * @brief Parses the XML output of malloc_info to get heap statistics.
 * This is the Linux equivalent of the GetHeapInfo function that used HeapWalk.
 * @return A pair containing {totalFreeMemory, largestFreeBlock}.
 */
// Corrected function to accurately parse free chunks
std::pair<size_t, size_t> getHeapInfo() {
    std::vector<char> buffer(65536, 0); // 64KB buffer

    FILE* stream = fmemopen(buffer.data(), buffer.size(), "w");
    if (!stream) {
        perror("fmemopen failed");
        return {0, 0};
    }

    malloc_info(0, stream);
    fclose(stream);

    std::string info(buffer.data());
    size_t totalFree = 0;
    size_t biggestFree = 0;

    try {
        // First, find the main heap's free block section.
        // We look for the content between <free> and </free>.
        size_t free_start = info.find("<free>");
        size_t free_end = info.find("</free>");

        if (free_start != std::string::npos && free_end != std::string::npos) {
            std::string free_section = info.substr(free_start, free_end - free_start);

            // Now, search for chunks only within the free section.
            std::regex chunk_regex("<chunk size=\"(\\d+)\">");
            auto chunks_begin = std::sregex_iterator(free_section.begin(), free_section.end(), chunk_regex);
            auto chunks_end = std::sregex_iterator();

            for (std::sregex_iterator i = chunks_begin; i != chunks_end; ++i) {
                std::smatch match = *i;
                size_t chunkSize = std::stoul(match[1].str());
                totalFree += chunkSize;
                if (chunkSize > biggestFree) {
                    biggestFree = chunkSize;
                }
            }
        }
    } catch (const std::regex_error& e) {
        std::cerr << "Regex error while parsing malloc_info: " << e.what() << std::endl;
        return {0, 0};
    }

    return {totalFree, biggestFree};
}

int main() {
    // --- Linux-specific Allocator Tuning ---
    // This is the conceptual equivalent of disabling the LFH to make fragmentation
    // more visible. We are telling malloc not to use mmap for large allocations,
    // forcing it to use the main heap break (sbrk), which tends to fragment more.
    mallopt(M_MMAP_MAX, 0);

    // Seed the random number generator.
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // --- Main Simulation Loop (identical to the Windows version) ---
    std::cout << "Running memory simulation for 100 timesteps..." << std::endl;

    std::vector<HeapStats> statsOverTime;
    std::vector<void*> allocatedBlocks;
    std::vector<size_t> requestedSizes;

    for (int t = 0; t < 100; ++t) {
        // Step A: Perform Memory Operations
        for (int i = 0; i < 10; ++i) {
            size_t size = 512 + (rand() % 1024);
            void* block = malloc(size); // Use malloc instead of HeapAlloc
            if (block) {
                allocatedBlocks.push_back(block);
                requestedSizes.push_back(size);
            }
        }

        if (allocatedBlocks.size() > 20) {
            int blockToFreeIdx = rand() % allocatedBlocks.size();
            free(allocatedBlocks[blockToFreeIdx]); // Use free instead of HeapFree
            allocatedBlocks.erase(allocatedBlocks.begin() + blockToFreeIdx);
            requestedSizes.erase(requestedSizes.begin() + blockToFreeIdx);
        }

        // Step B: Collect Data for this Timestep
        HeapStats currentStats;
        currentStats.timeStep = t;
        currentStats.totalUserRequested = 0;
        currentStats.totalHeapCommitted = 0;
        for (size_t i = 0; i < allocatedBlocks.size(); ++i) {
            currentStats.totalUserRequested += requestedSizes[i];
            // Use malloc_usable_size instead of HeapSize
            currentStats.totalHeapCommitted += malloc_usable_size(allocatedBlocks[i]);
        }
        currentStats.internalFragmentation = currentStats.totalHeapCommitted - currentStats.totalUserRequested;

        // Use our new Linux-specific function
        auto [totalFree, biggestFree] = getHeapInfo();
        currentStats.totalFreeOnHeap = totalFree;
        currentStats.biggestFreeBlock = biggestFree;
        currentStats.externalFragmentationRatio = (totalFree > 0) ? (1.0 - (double)biggestFree / totalFree) : 0.0;

        statsOverTime.push_back(currentStats);
    }

    // --- Output Results to CSV File (identical to the Windows version) ---
    std::cout << "Simulation Complete. Writing data to heap_fragmentation_stats_linux.csv..." << std::endl;
    std::ofstream csvFile("heap_fragmentation_stats_linux.csv");
    
    if (!csvFile.is_open()) {
        std::cerr << "Error: Could not open file for writing." << std::endl;
    } else {
        csvFile << "Time,InternalFrag_Bytes,ExternalFrag_Ratio,TotalFree_Bytes,BiggestBlock_Bytes,TotalUserRequested\n";
        for (const auto& s : statsOverTime) {
            csvFile << s.timeStep << ","
                    << s.internalFragmentation << ","
                    << s.externalFragmentationRatio << ","
                    << s.totalFreeOnHeap << ","
                    << s.biggestFreeBlock << ","
                    << s.totalUserRequested << "\n";
        }
        csvFile.close();
        std::cout << "Successfully wrote data to file." << std::endl;
    }

    // --- Final Cleanup ---
    std::cout << "\nCleaning up remaining allocated blocks..." << std::endl;
    for (void* block : allocatedBlocks) {
        free(block);
    }
    
    std::cout << "Done.\n";
    return 0;
}
