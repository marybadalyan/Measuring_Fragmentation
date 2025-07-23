#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <fstream> // Required for file I/O
#include <windows.h>
#include <heapapi.h>
#include <winnt.h>

// Data structure to hold all the metrics we collect at a single point in time.
struct HeapStats {
    int timeStep;
    size_t totalUserRequested;      // Sum of sizes your app asked for.
    size_t totalHeapCommitted;      // Sum of actual sizes allocated by the heap manager.
    size_t internalFragmentation;   // The difference between committed and requested.
    size_t totalFreeOnHeap;         // Total free memory, in many small blocks.
    size_t biggestFreeBlock;        // The largest single contiguous free block.
    double externalFragmentationRatio; // A calculated metric (1 - biggest/total).
};

// Global handle to the heap we will create and manage.
HANDLE heap;

/**
 * @brief Walks the specified heap to calculate total free memory and the largest
 * contiguous free block. This is the core of measuring EXTERNAL fragmentation.
 * @param heapHandle A handle to the heap to inspect.
 * @return A pair containing {totalFreeMemory, largestFreeBlock}.
 */
std::pair<SIZE_T, SIZE_T> GetHeapInfo(HANDLE heapHandle) {
    if (!HeapLock(heapHandle)) {
        std::cerr << "Failed to lock heap." << std::endl;
        return {0, 0};
    }

    PROCESS_HEAP_ENTRY entry;
    entry.lpData = nullptr;
    SIZE_T totalFree = 0;
    SIZE_T biggestFree = 0;

    while (HeapWalk(heapHandle, &entry)) {
        if (!(entry.wFlags & PROCESS_HEAP_ENTRY_BUSY)) { // Check if the block is free
            totalFree += entry.cbData;
            if (entry.cbData > biggestFree) {
                biggestFree = entry.cbData;
            }
        }
    }

    DWORD lastError = GetLastError();
    if (lastError != ERROR_NO_MORE_ITEMS) {
        std::cerr << "HeapWalk failed with error: " << lastError << std::endl;
    }

    HeapUnlock(heapHandle);
    return {totalFree, biggestFree};
}

int main() {
    // 1. Create a private, growable heap.
    heap = GetProcessHeap(); 
    if (!heap) {
        std::cerr << "HeapCreate failed: " << GetLastError() << std::endl;
        return 1;
    }

    // 2. CRITICAL STEP: Disable the Low-Fragmentation Heap (LFH) to observe classic fragmentation.
    ULONG heapInfo = 2;
    HeapSetInformation(heap, HeapCompatibilityInformation, &heapInfo, sizeof(heapInfo));
    
    // Seed the random number generator.
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // --- Main Simulation Loop ---
    std::cout << "Running memory simulation for 100 timesteps..." << std::endl;

    std::vector<HeapStats> statsOverTime;
    std::vector<void*> allocatedBlocks; // Keep track of all currently allocated blocks.
    std::vector<size_t> requestedSizes;  // Keep track of the size we requested for each block.

    for (int t = 0; t < 100; ++t) { // Run for 100 timesteps
        // Step A: Perform Memory Operations to simulate a workload.
        for (int i = 0; i < 10; ++i) {
            size_t size = 512 + (rand() % 1024);
            void* block = HeapAlloc(heap, 0, size);
            if (block) {
                allocatedBlocks.push_back(block);
                requestedSizes.push_back(size);
            }
        }

        if (allocatedBlocks.size() > 20) {
            int blockToFreeIdx = rand() % allocatedBlocks.size();
            HeapFree(heap, 0, allocatedBlocks[blockToFreeIdx]);
            allocatedBlocks.erase(allocatedBlocks.begin() + blockToFreeIdx);
            requestedSizes.erase(requestedSizes.begin() + blockToFreeIdx);
        }

        // Step B: Collect Data for this Timestep.
        HeapStats currentStats;
        currentStats.timeStep = t;
        currentStats.totalUserRequested = 0;
        currentStats.totalHeapCommitted = 0;
        for (size_t i = 0; i < allocatedBlocks.size(); ++i) {
            currentStats.totalUserRequested += requestedSizes[i];
            currentStats.totalHeapCommitted += HeapSize(heap, 0, allocatedBlocks[i]);
        }
        currentStats.internalFragmentation = currentStats.totalHeapCommitted - currentStats.totalUserRequested;

        auto [totalFree, biggestFree] = GetHeapInfo(heap);
        currentStats.totalFreeOnHeap = totalFree;
        currentStats.biggestFreeBlock = biggestFree;
        currentStats.externalFragmentationRatio = (totalFree > 0) ? (1.0 - (double)biggestFree / totalFree) : 0.0;

        statsOverTime.push_back(currentStats);
    }

    // --- Output Results to CSV File ---
    std::cout << "Simulation Complete. Writing data to heap_fragmentation_stats.csv..." << std::endl;
    std::ofstream csvFile("heap_fragmentation_stats.csv");
    
    if (!csvFile.is_open()) {
        std::cerr << "Error: Could not open file for writing." << std::endl;
    } else {
        // Write the header row for the CSV file.
        csvFile << "Time,InternalFrag_Bytes,ExternalFrag_Ratio,TotalFree_Bytes,BiggestBlock_Bytes,TotalUserRequested\n";
        
        // Write the data for each timestep.
        for (const auto& s : statsOverTime) {
            csvFile << s.timeStep << ","
                    << s.internalFragmentation << ","
                    << s.externalFragmentationRatio << ","
                    << s.totalFreeOnHeap << ","
                    << s.biggestFreeBlock << ","
                    << s.totalUserRequested << "\n"; // Added new data point
        }
        csvFile.close();
        std::cout << "Successfully wrote data to file." << std::endl;
    }

    // --- Final Cleanup ---
    std::cout << "\nCleaning up remaining allocated blocks..." << std::endl;
    for (void* block : allocatedBlocks) {
        HeapFree(heap, 0, block);
    }
    allocatedBlocks.clear();
    requestedSizes.clear();

    HeapDestroy(heap);
    std::cout << "Done.\n";
    return 0;
}
