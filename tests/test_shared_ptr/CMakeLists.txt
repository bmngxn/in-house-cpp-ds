add_executable(test_shared_ptr test_shared_ptr.cpp)

target_link_libraries(test_shared_ptr PRIVATE gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(test_shared_ptr)
