cmake_minimum_required(VERSION 3.14)

# ---------------------------------------------------------------------------
# Version: the macros in fastcast.hpp are the single source of truth.
# ---------------------------------------------------------------------------
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/fastcast.hpp" _fastcast_header)
foreach (part MAJOR MINOR PATCH)
    if (NOT _fastcast_header MATCHES "#define FASTCAST_VERSION_${part} ([0-9]+)")
        message(FATAL_ERROR "Could not read FASTCAST_VERSION_${part} from fastcast.hpp")
    endif ()
    set(FASTCAST_VERSION_${part} "${CMAKE_MATCH_1}")
endforeach ()
unset(_fastcast_header)

project(FastCast
        VERSION ${FASTCAST_VERSION_MAJOR}.${FASTCAST_VERSION_MINOR}.${FASTCAST_VERSION_PATCH}
        DESCRIPTION "A cached, header-only drop-in replacement for dynamic_cast"
        HOMEPAGE_URL "https://github.com/reach2sayan/FastCast"
        LANGUAGES CXX)

# ---------------------------------------------------------------------------
# Options
# ---------------------------------------------------------------------------
if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(FASTCAST_IS_TOPLEVEL ON)
else ()
    set(FASTCAST_IS_TOPLEVEL OFF)
endif ()

option(FASTCAST_BUILD_TESTS "Build the unit tests" ${FASTCAST_IS_TOPLEVEL})
option(FASTCAST_BUILD_EXAMPLES "Build the examples" ${FASTCAST_IS_TOPLEVEL})
option(FASTCAST_BUILD_BENCHMARKS "Build the benchmarks (fetches Google Benchmark)" ${FASTCAST_IS_TOPLEVEL})
option(FASTCAST_INSTALL "Generate install rules" ${FASTCAST_IS_TOPLEVEL})
option(FASTCAST_WARNINGS_AS_ERRORS "Treat warnings as errors in tests/examples/benchmarks" OFF)
option(FASTCAST_ENABLE_SANITIZERS "Build tests/examples with AddressSanitizer + UndefinedBehaviorSanitizer" OFF)

# The library itself only needs C++11. When FastCast is the top-level project
# and nothing else was requested, build the tests with the newest standard so
# the constexpr paths get exercised too. Override with -DCMAKE_CXX_STANDARD=11..23.
if (NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif ()
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

# ---------------------------------------------------------------------------
# The library (header-only)
# ---------------------------------------------------------------------------
add_library(FastCast INTERFACE)
add_library(FastCast::FastCast ALIAS FastCast)
target_include_directories(FastCast INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
target_compile_features(FastCast INTERFACE cxx_std_11)

# ---------------------------------------------------------------------------
# Developer flags shared by tests, examples and benchmarks (not exported)
# ---------------------------------------------------------------------------
add_library(fastcast_dev_flags INTERFACE)
if (MSVC)
    target_compile_options(fastcast_dev_flags INTERFACE /W4 /permissive- /EHsc)
    if (FASTCAST_WARNINGS_AS_ERRORS)
        target_compile_options(fastcast_dev_flags INTERFACE /WX)
    endif ()
else ()
    target_compile_options(fastcast_dev_flags INTERFACE
            -Wall -Wextra -pedantic -Wstrict-aliasing=1 -Wshadow -Wconversion
            -Wsign-conversion -Wold-style-cast -Wnon-virtual-dtor)
    if (FASTCAST_WARNINGS_AS_ERRORS)
        target_compile_options(fastcast_dev_flags INTERFACE -Werror)
    endif ()
endif ()

if (FASTCAST_ENABLE_SANITIZERS)
    if (MSVC)
        target_compile_options(fastcast_dev_flags INTERFACE /fsanitize=address)
    else ()
        target_compile_options(fastcast_dev_flags INTERFACE
                -fsanitize=address,undefined -fno-omit-frame-pointer
                -fno-sanitize-recover=all)
        target_link_options(fastcast_dev_flags INTERFACE
                -fsanitize=address,undefined)
    endif ()
endif ()

# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------
if (FASTCAST_BUILD_TESTS)
    enable_testing()
    find_package(Threads REQUIRED)

    # Catch2 v2 is vendored as a single header (tests/catch.hpp), so no
    # external dependency needs to be fetched or built.
    add_executable(tests tests/tests.cpp)
    target_link_libraries(tests PRIVATE
            FastCast fastcast_dev_flags Threads::Threads)
    # The vendored Catch2 header is not ours to keep warning-clean.
    if (NOT MSVC)
        target_compile_options(tests PRIVATE -Wno-conversion -Wno-sign-conversion)
    endif ()
    add_test(NAME tests COMMAND tests)
    set_tests_properties(tests PROPERTIES LABELS unit)

    # Compile-failure tests: each source in tests/compile_fail must *fail* to
    # compile, and the diagnostic must match the given regular expression.
    set(_cf_dir "${CMAKE_CURRENT_SOURCE_DIR}/tests/compile_fail")
    function(fastcast_add_compile_fail_test name expect)
        add_library(compile_fail_${name} OBJECT EXCLUDE_FROM_ALL "${_cf_dir}/${name}.cpp")
        target_link_libraries(compile_fail_${name} PRIVATE FastCast)
        add_test(NAME compile_fail.${name}
                COMMAND "${CMAKE_COMMAND}"
                "-DBUILD_DIR=${CMAKE_BINARY_DIR}"
                "-DTARGET=compile_fail_${name}"
                "-DCONFIG=$<CONFIG>"
                "-DEXPECT=${expect}"
                -P "${_cf_dir}/check.cmake")
        # They invoke the build tool on this tree, so never run them in parallel
        # with each other.
        set_tests_properties(compile_fail.${name} PROPERTIES
                RUN_SERIAL TRUE LABELS compile_fail)
    endfunction()
    fastcast_add_compile_fail_test(cast_away_const "cannot cast away const")
    fastcast_add_compile_fail_test(cast_away_const_ref "cannot cast away const")
    fastcast_add_compile_fail_test(non_polymorphic "not polymorphic")
    fastcast_add_compile_fail_test(non_pointer_target "no matching")
endif ()

# ---------------------------------------------------------------------------
# Examples (also run under ctest so the README snippets stay honest)
# ---------------------------------------------------------------------------
if (FASTCAST_BUILD_EXAMPLES)
    enable_testing()
    add_executable(example_basic examples/basic.cpp)
    target_link_libraries(example_basic PRIVATE FastCast fastcast_dev_flags)
    add_test(NAME example_basic COMMAND example_basic)
    set_tests_properties(example_basic PROPERTIES LABELS example)
endif ()

# ---------------------------------------------------------------------------
# Benchmarks
# ---------------------------------------------------------------------------
if (FASTCAST_BUILD_BENCHMARKS)
    include(FetchContent)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
    set(_benchmark_extra_args)
    if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.28)
        set(_benchmark_extra_args EXCLUDE_FROM_ALL)
    endif ()
    FetchContent_Declare(
            benchmark
            GIT_REPOSITORY https://github.com/google/benchmark.git
            GIT_TAG v1.9.1
            GIT_SHALLOW ON
            ${_benchmark_extra_args})
    FetchContent_MakeAvailable(benchmark)

    add_executable(measure benchmark/benchmark.cpp)
    # Google Benchmark's headers need C++14; this raises the standard for the
    # benchmark target only when the project-wide one is older.
    target_compile_features(measure PRIVATE cxx_std_14)
    target_link_libraries(measure PRIVATE
            FastCast fastcast_dev_flags benchmark::benchmark)
endif ()

# ---------------------------------------------------------------------------
# Install / package config: `find_package(FastCast)` -> FastCast::FastCast
# ---------------------------------------------------------------------------
if (FASTCAST_INSTALL)
    include(CMakePackageConfigHelpers)
    set(_cfg_dir "${CMAKE_INSTALL_LIBDIR}/cmake/FastCast")

    install(FILES fastcast.hpp DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
    install(TARGETS FastCast EXPORT FastCastTargets)
    install(EXPORT FastCastTargets
            NAMESPACE FastCast::
            DESTINATION "${_cfg_dir}")

    configure_package_config_file(
            cmake/FastCastConfig.cmake.in
            "${CMAKE_CURRENT_BINARY_DIR}/FastCastConfig.cmake"
            INSTALL_DESTINATION "${_cfg_dir}")
    write_basic_package_version_file(
            "${CMAKE_CURRENT_BINARY_DIR}/FastCastConfigVersion.cmake"
            COMPATIBILITY SameMajorVersion
            ARCH_INDEPENDENT)
    install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/FastCastConfig.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/FastCastConfigVersion.cmake"
            DESTINATION "${_cfg_dir}")
    install(FILES LICENSE.txt README.md CHANGELOG.md
            DESTINATION "${CMAKE_INSTALL_DOCDIR}")

    # `cpack` (or `cmake --build build --target package`) produces an
    # architecture-independent FastCast-<version>.tar.gz / .zip containing the
    # installed tree above. The release workflow attaches these to GitHub
    # Releases; see README.md "Releasing".
    if (FASTCAST_IS_TOPLEVEL)
        set(CPACK_GENERATOR "TGZ;ZIP")
        set(CPACK_PACKAGE_NAME "FastCast")
        set(CPACK_PACKAGE_VENDOR "reach2sayan")
        set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
        set(CPACK_PACKAGE_FILE_NAME "FastCast-${PROJECT_VERSION}")
        set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")
        set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
        set(CPACK_VERBATIM_VARIABLES ON)
        set(CPACK_SOURCE_GENERATOR "TGZ")
        set(CPACK_SOURCE_PACKAGE_FILE_NAME "FastCast-${PROJECT_VERSION}-src")
        set(CPACK_SOURCE_IGNORE_FILES
                "/\\.git/" "/\\.idea/" "/build.*/" "/cmake-build-.*/" "/out/" "\\.DS_Store$")
        include(CPack)
    endif ()
endif ()
