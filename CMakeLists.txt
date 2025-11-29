cmake_minimum_required(VERSION 3.14)
project(MyProject LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optionally enable testing
include(CTest)

# Add subdirectory with tests
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()