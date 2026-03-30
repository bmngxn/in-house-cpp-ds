add_executable(test_spsc_queue test_spsc_queue.cpp)

target_link_libraries(test_spsc_queue PRIVATE gtest gtest_main)

include(GoogleTest)
gtest_discover_tests(test_spsc_queue)
