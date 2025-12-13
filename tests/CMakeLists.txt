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
    test_threads.cpp
    test_exceptions.cpp
)

target_link_libraries(tests
    PRIVATE
        GTest::gtest
        GTest::gtest_main
)

# Enable maximum warnings and treat them as errors
if(MSVC)
    target_compile_options(tests PRIVATE
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
    target_compile_options(tests PRIVATE
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
        target_compile_options(tests PRIVATE
            -Wmisleading-indentation
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wuseless-cast
        )
    endif()
    
    # Clang-specific warnings
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        target_compile_options(tests PRIVATE
            -Wmost
            -Wextra-semi
        )
    endif()
endif()

target_compile_options(tests PRIVATE
        -fsanitize=thread)

target_link_options(tests PRIVATE
        -fsanitize=thread)

include(GoogleTest)
gtest_discover_tests(tests)