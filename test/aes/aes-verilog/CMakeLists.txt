project(aes-test CXX)
cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-march=native -O2 -g")


find_package(verilator)
add_subdirectory(test/cpp)
# add_subdirectory(test/sbox)
# add_subdirectory(test/mixcolumns)
# add_subdirectory(test/shiftrows)