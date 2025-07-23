# Heap Fragmentation Analyzer 📈

This project provides a hands-on analysis of memory fragmentation in C++ applications. It includes two versions of a C++ program—one for **Windows** and one for **Linux**—that simulate a memory-intensive workload and measure both internal and external heap fragmentation over time. The results are saved to a `.csv` file, which can then be visualized using the included Python script.

-----

## What is Heap Fragmentation?

When a program runs, it constantly requests and releases blocks of memory from the operating system's heap. Over time, this process can lead to inefficiencies where memory is wasted. This waste is called fragmentation, and it comes in two main types:

### внутрішня фрагментація

**Internal fragmentation** occurs when the memory allocator provides a block of memory that is larger than the size requested by the application. The unused space *inside* that allocated block is wasted because it cannot be used by any other allocation.

  * **Example:** You ask for 50 bytes, but the allocator gives you a 64-byte block (due to alignment or internal data structures). The extra 14 bytes are internal fragmentation.
  * **How We Measure It:** We calculate the sum of the *actual* block sizes given by the OS (`HeapSize` on Windows, `malloc_usable_size` on Linux) and subtract the sum of the sizes our program originally *requested*.
    $Internal Fragmentation = \\sum(ActualBlockSizes) - \\sum(RequestedSizes)$

### External Fragmentation

**External fragmentation** occurs when there is plenty of free memory available on the heap, but it is broken up into so many small, non-contiguous blocks ("holes") that it cannot satisfy a large memory request.

  * **Example:** The heap has 1 MB of total free space, but it's scattered in 1 KB chunks. A request for a single 50 KB block will fail.
  * **How We Measure It:** We look at the total free memory versus the single largest available block. We calculate a ratio to represent this:
    $External Fragmentation Ratio = 1 - \\frac{LargestFreeBlock}{TotalFreeSpace}$
    A ratio close to 1.0 indicates high external fragmentation.

-----

## How the Program Works

The project simulates a workload over 100 "timesteps." In each step, it allocates several new, small blocks of memory and frees one random, older block. This process mimics the churn of a real application and gradually causes fragmentation.

The data collection is platform-specific, leveraging low-level OS and C library features.

### Windows Version 🪟

The Windows analyzer uses the **Win32 Heap API** for fine-grained control and analysis.

  * **Allocation:** Uses `HeapAlloc()` and `HeapFree()` for memory operations.
  * **Measurement:**
      * `HeapSize()` is used to get the actual size of an allocated block for measuring internal fragmentation.
      * `HeapWalk()` is the key function used to iterate through every single block in the heap (both busy and free) to measure external fragmentation.
  * **Key Technique:** We use `HeapSetInformation()` to **disable the Low-Fragmentation Heap (LFH)**. By default, the LFH manages small allocations in bulk and hides their true status from `HeapWalk`. Disabling it is crucial for an accurate measurement.

### Linux Version 🐧

The Linux analyzer uses non-standard extensions from the **GNU C Library (glibc)**, as standard POSIX C++ does not provide heap inspection tools.

  * **Allocation:** Uses the standard `malloc()` and `free()`.
  * **Measurement:**
      * `malloc_usable_size()` is the `glibc` equivalent of `HeapSize` and is used to measure internal fragmentation.
      * `malloc_info()` is used to get heap statistics. This function returns an XML string summarizing the heap's state. We parse this XML to find the sizes of all free chunks to measure external fragmentation.
  * **Key Technique:** We use `mallopt()` to tune the allocator's behavior (specifically, by disabling `mmap` for large allocations) to make fragmentation patterns more consistent and measurable from the main heap break.

-----

## Plotting the Results with Python

The C++ programs generate a `heap_fragmentation_stats.csv` file with the collected data. The provided Python script, `plotter.py`, reads this file and generates a visual report.

  * **Tools:** The script uses `pandas` to easily read and manage the CSV data and `matplotlib` to create the plots.
  * **Output:** It produces a PNG image (`custom_fragmentation_analysis.png`) containing graphs that visualize how fragmentation and memory usage change over the course of the simulation.

-----

## How to Run the Project

Follow these steps to compile the analyzers, run the simulation, and generate the plots.

### Prerequisites

  * A C++ compiler (`g++` on Linux, or MSVC on Windows).
  * Python 3.
  * `pip` for installing Python packages.

### Step 1: Install Python Libraries

Open your terminal or command prompt and install the necessary libraries.

```bash
pip install pandas matplotlib
```

### Step 2: Compile and Run the C++ Analyzer

Choose the version for your operating system.

**For Linux:**
Save the Linux version of the code as `linux_heap.cpp` and run these commands:

```bash
# Compile the program
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release   

cmake --build build --config Release

# Run the analyzer to generate the data file
./build/Release/main_linux
```

**For Windows:**
Save the Windows version of the code as `windows_heap.cpp` and compile it with your preferred compiler (e.g., in Visual Studio or with `cl.exe`). Then, run the resulting `.exe` file.

```powershell
# Run the analyzer to generate the data file
.\build\Release\main_windows.exe
```

This step will create the `heap_fragmentation_stats.csv` file.

### Step 3: Run the Python Plotter

Make sure you are in the same directory as the `.csv` file. Save the Python script as `plotter.py` and run it:

```bash
python plotter.py
```

This will read the data and create the `custom_fragmentation_analysis.png` image file containing your graphs.