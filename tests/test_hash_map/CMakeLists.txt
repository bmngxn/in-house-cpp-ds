add_executable(test_hash_map test_hash_map.cpp)

target_link_libraries(test_hash_map PRIVATE gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(test_hash_map)
