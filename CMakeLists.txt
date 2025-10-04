cmake_minimum_required(VERSION 3.8)
project(FastCast)
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BOOST_INCLUDE_LIBRARIES test) # enabled libraries
set(BOOST_ENABLE_CMAKE ON)
FetchContent_Declare(
        Boost
        GIT_REPOSITORY "https://github.com/boostorg/boost.git"
        GIT_TAG "boost-1.86.0"
        GIT_PROGRESS ON
        GIT_SHALLOW ON
        OVERRIDE_FIND_PACKAGE TRUE
        EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(Boost)
find_package(Boost REQUIRED Test)

set(BENCHMARK_ENABLE_GTEST_TESTS
        OFF CACHE BOOL
        "Disable GTest integration for Benchmark"
        FORCE)
FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.9.1
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(benchmark)

add_library(FastCast INTERFACE fastcast.hpp)

add_executable(measure benchmark.cpp
        utilities.hpp)
target_link_libraries(measure PRIVATE benchmark::benchmark FastCast)

add_executable(tests tests.cpp
        tests.cpp)
target_link_libraries(tests PRIVATE FastCast)
target_include_directories(tests PRIVATE ${Boost_INCLUDE_DIRS})

add_test(NAME tests COMMAND tests)
enable_testing()
