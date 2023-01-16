cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_FLAGS "-march=native -O3 -g")
project(tangor CXX)
set(CMAKE_CXX_STANDARD 20)
option(USE_HOGE "Use HOGE for 2-input gates" OFF)

if(USE_HOGE)
    add_compile_definitions(USE_HOGE)
    add_subdirectory(thirdparties/xcl2)
endif()

# For OpenMP
find_package(OpenMP)
if(OpenMP_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

add_subdirectory(thirdparties/cuFHE/thirdparties/TFHEpp)
add_subdirectory(src)

add_subdirectory(test)
