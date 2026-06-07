cmake_minimum_required(VERSION 3.14)
project(FastCast CXX)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif ()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(FASTCAST_IS_TOPLEVEL ON)
else ()
    set(FASTCAST_IS_TOPLEVEL OFF)
endif ()
option(FASTCAST_BUILD_TESTS "Build the unit tests" ${FASTCAST_IS_TOPLEVEL})
option(FASTCAST_BUILD_BENCHMARKS "Build the benchmarks" ${FASTCAST_IS_TOPLEVEL})

add_library(FastCast INTERFACE)
add_library(FastCast::FastCast ALIAS FastCast)
target_include_directories(FastCast INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_compile_features(FastCast INTERFACE cxx_std_20)

add_library(fastcast_warnings INTERFACE)
if (MSVC)
    target_compile_options(fastcast_warnings INTERFACE /W3)
else ()
    target_compile_options(fastcast_warnings INTERFACE
            -Wall -Wextra -pedantic -Wstrict-aliasing=1)
endif ()

if (FASTCAST_BUILD_TESTS)
    enable_testing()

    # Catch2 v2 is vendored as a single header (tests/catch.hpp), so no
    # external dependency needs to be fetched or built.
    add_executable(tests tests/tests.cpp)
    target_link_libraries(tests PRIVATE
            FastCast fastcast_warnings)

    add_test(NAME tests COMMAND tests)
endif ()

if (FASTCAST_BUILD_BENCHMARKS)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
            benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG v1.9.1
            GIT_SHALLOW ON
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE
            EXCLUDE_FROM_ALL)
    FetchContent_MakeAvailable(benchmark)

    add_executable(measure benchmark/benchmark.cpp)
    target_link_libraries(measure PRIVATE
            FastCast fastcast_warnings benchmark::benchmark)
endif ()
