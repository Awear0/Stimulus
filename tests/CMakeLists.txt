# tests/CMakeLists.txt

enable_testing()

# Look for an installed GTest package
find_package(GTest REQUIRED)

set(TEST_SOURCES
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
    test_threads.cpp
    test_exceptions.cpp
)

# Enable maximum warnings and treat them as errors
if(MSVC)
    add_compile_options(
        /W4                     # Maximum warning level
        /WX                     # Treat warnings as errors
        /permissive-            # Strict C++ conformance
        /w14640                 # Thread-safe static initialization
        /w14826                 # Conversion warnings
        /w14905                 # Wide string literal cast
        /w14906                 # String literal cast
        /w14928                 # Illegal copy-initialization
    )
else()
    add_compile_options(
        -Wall                   # Standard warnings
        -Wextra                 # Extra warnings
        -Wpedantic              # Pedantic warnings
        -Werror                 # Treat warnings as errors
        -Wshadow                # Warn about shadowing
        -Wnon-virtual-dtor      # Warn about non-virtual destructors
        -Wold-style-cast        # Warn about C-style casts
        -Wcast-align            # Warn about pointer cast alignment
        -Wunused                # Warn about unused variables
        -Woverloaded-virtual    # Warn about overloaded virtual functions
        -Wconversion            # Warn about type conversions
        -Wsign-conversion       # Warn about sign conversions
        -Wnull-dereference      # Warn about null dereferences
        -Wdouble-promotion      # Warn about float to double promotion
        -Wformat=2              # Warn about format string issues
        -Wimplicit-fallthrough  # Warn about fallthrough in switch
    )
    
    # GCC-specific warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(
            -Wmisleading-indentation
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wuseless-cast
        )
    endif()
    
    # Clang-specific warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        add_compile_options(
            -Wmost
            -Wextra-semi
        )
    endif()
endif()

include(GoogleTest)

add_executable(tests
    ${TEST_SOURCES}
)

target_link_libraries(tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

add_executable(tests-thread-sanitizer
    ${TEST_SOURCES}
)

target_compile_options(tests-thread-sanitizer PRIVATE
    -fsanitize=thread)

target_link_options(tests-thread-sanitizer PRIVATE
    -fsanitize=thread)

target_link_libraries(tests-thread-sanitizer
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

add_executable(tests-address-sanitizer
    ${TEST_SOURCES}
)

target_compile_options(tests-address-sanitizer PRIVATE
    -fsanitize=address,undefined)

target_link_options(tests-address-sanitizer PRIVATE
    -fsanitize=address,undefined)

target_link_libraries(tests-address-sanitizer
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

gtest_discover_tests(tests)
gtest_discover_tests(tests-thread-sanitizer)
gtest_discover_tests(tests-address-sanitizer)