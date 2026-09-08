# Consumer project used to verify `find_package(FastCast)` against an
# installed tree. Configure it with
#   cmake -S tests/package -B build-pkg -DCMAKE_PREFIX_PATH=<install prefix>
cmake_minimum_required(VERSION 3.14)
project(FastCastPackageTest LANGUAGES CXX)

find_package(FastCast 1.1 REQUIRED CONFIG)

add_executable(package_test main.cpp)
target_link_libraries(package_test PRIVATE FastCast::FastCast)

enable_testing()
add_test(NAME package_test COMMAND package_test)
