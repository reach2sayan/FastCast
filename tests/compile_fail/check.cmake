# Runs one compile-failure test.
#
# Invoked by ctest as
#   cmake -DBUILD_DIR=<dir> -DTARGET=<target> -DCONFIG=<cfg> -DEXPECT=<regex> -P check.cmake
#
# The test passes only if building TARGET fails *and* the compiler output
# matches EXPECT. A build that fails for any other reason (missing header,
# broken toolchain, ...) is reported as a test failure, not a pass.

foreach (var BUILD_DIR TARGET EXPECT)
    if (NOT DEFINED ${var})
        message(FATAL_ERROR "check.cmake: -D${var}=... is required")
    endif ()
endforeach ()

set(build_cmd "${CMAKE_COMMAND}" --build "${BUILD_DIR}" --target "${TARGET}")
if (DEFINED CONFIG AND NOT CONFIG STREQUAL "")
    list(APPEND build_cmd --config "${CONFIG}")
endif ()

execute_process(
        COMMAND ${build_cmd}
        RESULT_VARIABLE rc
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err)
set(log "${out}\n${err}")

if (rc EQUAL 0)
    message(FATAL_ERROR
            "compile_fail.${TARGET}: expected the build to FAIL, but it succeeded.\n${log}")
endif ()

if (NOT log MATCHES "${EXPECT}")
    message(FATAL_ERROR
            "compile_fail.${TARGET}: the build failed, but the output does not match "
            "the expected diagnostic /${EXPECT}/:\n${log}")
endif ()

message(STATUS "compile_fail.${TARGET}: failed to compile as expected (/${EXPECT}/)")
