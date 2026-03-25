# 1. Name your executable and tell it which .cpp file to compile
add_executable(test_lru_cache test_lru_cache.cpp)
add_executable(test_concurrent_lru_cache test_concurrent_lru_cache.cpp)

# 2. Link GoogleTest to your executable
target_link_libraries(test_lru_cache PRIVATE gtest gtest_main)
target_link_libraries(test_concurrent_lru_cache PRIVATE gtest gtest_main)

# 3. Tell CMake's testing tool (CTest) to find and run these tests
include(GoogleTest)
gtest_discover_tests(test_lru_cache)
gtest_discover_tests(test_concurrent_lru_cache)
