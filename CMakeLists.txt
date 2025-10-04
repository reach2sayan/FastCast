cmake_minimum_required(VERSION 3.8)
project(FastCast)

include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    if (POLICY CMP0141)
        cmake_policy(SET CMP0141 NEW)
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,
                                                $<CXX_COMPILER_ID:MSVC>>,
                                                $<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,
                                                $<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
    endif ()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3")
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
else ()
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-Wall -Wextra -pedantic -stdlib=libc++)
    elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-Wall -Wextra -pedantic)
    endif ()
endif ()


set(BOOST_INCLUDE_LIBRARIES test) # enabled libraries
set(BOOST_ENABLE_CMAKE ON)
FetchContent_Declare(
        Boost
        GIT_REPOSITORY "https://github.com/boostorg/boost.git"
        GIT_TAG "boost-1.86.0"
        GIT_SHALLOW ON
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        OVERRIDE_FIND_PACKAGE TRUE
        EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(Boost)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)
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

add_executable(measure benchmark.cpp utilities.hpp)
target_link_libraries(measure PRIVATE benchmark::benchmark FastCast)

add_executable(tests tests.cpp)
target_link_libraries(tests PRIVATE FastCast Boost::unit_test_framework)
target_include_directories(tests PRIVATE ${Boost_INCLUDE_DIRS})

add_test(NAME tests COMMAND tests)
enable_testing()
