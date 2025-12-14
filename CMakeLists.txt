cmake_minimum_required(VERSION 3.14)
project(Stimulus VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Optionally enable testing
include(CTest)

# Add subdirectory with tests
if(BUILD_TESTING AND (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR))
    add_subdirectory(tests)
endif()

add_library(Stimulus INTERFACE)
add_library(Stimulus::Stimulus ALIAS Stimulus)

target_include_directories(Stimulus INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_options(Stimulus INTERFACE
    $<$<CXX_COMPILER_ID:GNU>:-std=c++26>
    $<$<CXX_COMPILER_ID:Clang>:-std=c++2c>
    $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(TARGETS Stimulus
    EXPORT StimulusTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT StimulusTargets
    FILE StimulusTargets.cmake
    NAMESPACE Stimulus::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Stimulus
)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/StimulusConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/StimulusConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Stimulus
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/StimulusConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/StimulusConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/StimulusConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Stimulus
)