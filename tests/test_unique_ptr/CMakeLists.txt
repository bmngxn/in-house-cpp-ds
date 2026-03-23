# 1. Name your executable and tell it which .cpp file to compile
add_executable(test_unique_ptr test_unique_ptr.cpp)

# 2. Link GoogleTest to your executable
# 'gtest_main' provides the main() function automatically!
target_link_libraries(test_unique_ptr PRIVATE gtest gtest_main)

# 3. Tell CMake's testing tool (CTest) to find and run these tests
include(GoogleTest)
gtest_discover_tests(test_unique_ptr)