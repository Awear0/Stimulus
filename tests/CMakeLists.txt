# tests/CMakeLists.txt

enable_testing()

# Look for an installed GTest package
find_package(GTest REQUIRED)

add_executable(tests
    test_basic_connect_emit.cpp
    test_connection.cpp
    test_connect_once.cpp
    test_map.cpp
    test_transform.cpp
    test_map_transform.cpp
    test_forward_signal.cpp
    test_all_different.cpp
    test_execution_policy.cpp
    test_partial_call.cpp
    test_partial_connection.cpp
    test_filter.cpp
    test_pipe_operator.cpp
    test_chain.cpp
    test_guard.cpp
)

target_link_libraries(tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)