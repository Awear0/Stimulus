# tests/CMakeLists.txt

enable_testing()

# Look for an installed GTest package
find_package(GTest REQUIRED)

add_executable(tests
    test_basic_connect_emit.cpp
    test_connection.cpp
)

target_link_libraries(tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(tests)