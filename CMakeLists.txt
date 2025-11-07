cmake_minimum_required(VERSION 3.16)
project(hashbrowns VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compilation options based on build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Propagate build type as a define for embedding in JSON meta
add_definitions(-DHASHBROWNS_BUILD_TYPE="$<CONFIG>")

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -Wall -Wextra -Wpedantic -fsanitize=address,undefined")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -march=native -flto")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g -DNDEBUG")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /Wall")
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /DNDEBUG")
endif()

# Enable position independent code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include directories
include_directories(src)

# Core library
set(CORE_SOURCES
    src/core/memory_manager.cpp
    src/core/timer.cpp
)

set(CORE_HEADERS
    src/core/data_structure.h
    src/core/memory_manager.h
    src/core/timer.h
)

add_library(hashbrowns_core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(hashbrowns_core PUBLIC src)

# Data structures library (will be populated with implementations)
set(STRUCTURES_SOURCES
    src/structures/dynamic_array.cpp
    src/structures/linked_list.cpp
    src/structures/hash_map.cpp
)

set(STRUCTURES_HEADERS
    src/structures/dynamic_array.h
    src/structures/linked_list.h
    src/structures/hash_map.h
)

# Create structures library now that we have sources
add_library(hashbrowns_structures STATIC ${STRUCTURES_SOURCES} ${STRUCTURES_HEADERS})
target_link_libraries(hashbrowns_structures PUBLIC hashbrowns_core)
target_include_directories(hashbrowns_structures PUBLIC src)

# Benchmark library
set(BENCHMARK_SOURCES
    src/benchmark/benchmark_suite.cpp
    src/benchmark/regression_tester.cpp
)

set(BENCHMARK_HEADERS
    src/benchmark/benchmark_suite.h
    src/benchmark/stats_analyzer.h
    src/benchmark/regression_tester.h
)

# Only create benchmark library if we have sources
if(BENCHMARK_SOURCES)
    add_library(hashbrowns_benchmark STATIC ${BENCHMARK_SOURCES} ${BENCHMARK_HEADERS})
    target_link_libraries(hashbrowns_benchmark PUBLIC hashbrowns_core)
    if(TARGET hashbrowns_structures)
        target_link_libraries(hashbrowns_benchmark PUBLIC hashbrowns_structures)
    endif()
    target_include_directories(hashbrowns_benchmark PUBLIC src)
endif()

# Regression check CLI tool (optional helper)
add_executable(regression_check tools/regression_check.cpp)
target_link_libraries(regression_check PRIVATE hashbrowns_benchmark)
target_include_directories(regression_check PRIVATE src)

# Main executable
set(MAIN_SOURCES
    src/main.cpp
)

# Create main executable
add_executable(hashbrowns ${MAIN_SOURCES})
target_link_libraries(hashbrowns PRIVATE hashbrowns_core hashbrowns_structures)
if(TARGET hashbrowns_benchmark)
    target_link_libraries(hashbrowns PRIVATE hashbrowns_benchmark)
endif()

# Enable testing
enable_testing()

# Find or build testing framework (using a simple approach for now)
add_library(simple_test INTERFACE)
target_include_directories(simple_test INTERFACE tests)

# Unit tests
file(GLOB TEST_SOURCES tests/unit/*.cpp)
if(TEST_SOURCES)
    add_executable(unit_tests ${TEST_SOURCES})
    target_link_libraries(unit_tests PRIVATE hashbrowns_core hashbrowns_structures simple_test)
    if(TARGET hashbrowns_benchmark)
        target_link_libraries(unit_tests PRIVATE hashbrowns_benchmark)
    endif()
    # Ensure tests directory is available as an include root for cross-platform builds
    target_include_directories(unit_tests PRIVATE tests)
    add_test(NAME UnitTests COMMAND unit_tests)
endif()

# Integration tests  
file(GLOB_RECURSE INTEGRATION_TEST_SOURCES tests/integration/*.cpp)
if(INTEGRATION_TEST_SOURCES)
    add_executable(integration_tests ${INTEGRATION_TEST_SOURCES})
    target_link_libraries(integration_tests PRIVATE hashbrowns_benchmark simple_test)
    add_test(NAME IntegrationTests COMMAND integration_tests)
endif()

# Performance tests
file(GLOB_RECURSE PERF_TEST_SOURCES tests/performance/*.cpp)
if(PERF_TEST_SOURCES)
    add_executable(performance_tests ${PERF_TEST_SOURCES})
    target_link_libraries(performance_tests PRIVATE hashbrowns_benchmark simple_test)
    add_test(NAME PerformanceTests COMMAND performance_tests)
endif()

# Custom targets for development workflow

# Format code using clang-format
find_program(CLANG_FORMAT_EXE NAMES clang-format)
if(CLANG_FORMAT_EXE)
    file(GLOB_RECURSE ALL_SOURCE_FILES 
         src/*.cpp src/*.h 
         tests/*.cpp tests/*.h)
    add_custom_target(format
        COMMAND ${CLANG_FORMAT_EXE} -i ${ALL_SOURCE_FILES}
        COMMENT "Formatting source code with clang-format"
    )
endif()

# Static analysis with clang-tidy
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXE})
endif()

# Documentation generation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

# Benchmark runner target (only if main executable exists)
if(TARGET hashbrowns)
    add_custom_target(benchmark
        COMMAND $<TARGET_FILE:hashbrowns> --size 10000 --runs 10 --output benchmark_results.csv
        DEPENDS hashbrowns
        COMMENT "Running performance benchmarks"
    )
endif()

# Memory check with Valgrind (Linux/macOS)
find_program(VALGRIND_EXE NAMES valgrind)
if(VALGRIND_EXE)
    add_custom_target(memcheck
        COMMAND ${VALGRIND_EXE} --tool=memcheck --leak-check=full --show-leak-kinds=all 
                --track-origins=yes --verbose $<TARGET_FILE:unit_tests>
        DEPENDS unit_tests
        COMMENT "Running memory check with Valgrind"
    )
endif()

# Packaging configuration
set(CPACK_PACKAGE_NAME "hashbrowns")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-performance C++ data structure benchmarking suite")
set(CPACK_PACKAGE_VENDOR "hashbrowns project")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)

# Installation rules
if(TARGET hashbrowns)
    install(TARGETS hashbrowns
            RUNTIME DESTINATION bin)
endif()

install(TARGETS hashbrowns_core hashbrowns_structures
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin)

if(TARGET hashbrowns_benchmark)
    install(TARGETS hashbrowns_benchmark
            ARCHIVE DESTINATION lib
            LIBRARY DESTINATION lib
            RUNTIME DESTINATION bin)
endif()

install(DIRECTORY src/
        DESTINATION include/hashbrowns
        FILES_MATCHING PATTERN "*.h")

# Development helpers
add_custom_target(clean-all
    COMMAND ${CMAKE_BUILD_TOOL} clean
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND ${CMAKE_COMMAND} -E remove ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMENT "Deep clean of build directory"
)

# Print configuration summary
message(STATUS "")
message(STATUS "========== Build Configuration ==========")
message(STATUS "Project: ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "System: ${CMAKE_SYSTEM_NAME} ${CMAKE_SYSTEM_VERSION}")
message(STATUS "Architecture: ${CMAKE_SYSTEM_PROCESSOR}")
message(STATUS "Install prefix: ${CMAKE_INSTALL_PREFIX}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
else()
    message(STATUS "Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
endif()
message(STATUS "=========================================")
message(STATUS "")