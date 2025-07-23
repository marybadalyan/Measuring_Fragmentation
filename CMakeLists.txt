
cmake_minimum_required(VERSION 3.10)

# Set the project name
project(main)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add the executable
if(WIN32)
    add_executable(main_windows main_windows.cpp)
else()
    add_executable(main_linux main_linux.cpp)
endif()