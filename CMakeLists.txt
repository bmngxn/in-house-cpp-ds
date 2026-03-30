cmake_minimum_required(VERSION 3.14)
project(CppDataStructures CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 1. Dependencies ---

# Force GoogleTest to use the shared C++ runtime library (fixes MTd vs MDd mismatch)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add GoogleTest
add_subdirectory(ext/googletest)

# Add Google Benchmark (and force it not to build its own tests)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable benchmark tests" FORCE)
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable benchmark install" FORCE)
add_subdirectory(ext/benchmark)

# --- 2. Project Setup ---

# Make your 'include' folder globally accessible
include_directories(include)

# Enable CTest
enable_testing()

# Add your test directories
add_subdirectory(tests/test_unique_ptr)
add_subdirectory(tests/test_lru_cache)
add_subdirectory(tests/test_shared_ptr)
add_subdirectory(tests/test_spsc_queue)
add_subdirectory(tests/test_hash_map)