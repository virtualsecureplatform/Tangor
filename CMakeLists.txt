cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_FLAGS "-march=native -O3 -g")
project(tangor CXX C)
set(CMAKE_CXX_STANDARD 20)
option(USE_HOGE "Use HOGE for 2-input gates" OFF)
option(TANGOR_BUILD_TESTS "Build Tangor test clients" ON)
set(TANGOR_CUFHEPP_SOURCE_DIR "${PROJECT_SOURCE_DIR}/thirdparties/cuFHEpp" CACHE PATH "cuFHEpp checkout that provides TFHEpp")
set(TANGOR_TFHEPP_SOURCE_DIR "" CACHE PATH "Optional TFHEpp checkout override; defaults to cuFHEpp's nested TFHEpp")

if(USE_HOGE)
    add_compile_definitions(USE_HOGE)
    add_subdirectory(thirdparties/xcl2)
endif()

# For OpenMP
find_package(OpenMP)
if(OpenMP_FOUND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

set(ENABLE_TEST OFF CACHE BOOL "Build TFHEpp tests" FORCE)
set(ENABLE_BENCHMARK OFF CACHE BOOL "Build TFHEpp benchmarks" FORCE)
set(ENABLE_TUTORIAL OFF CACHE BOOL "Build TFHEpp tutorial" FORCE)
set(USE_SPQLIOS_INTL ON CACHE BOOL "Use interleaved-format SPQLIOS")
set(USE_AVX512 OFF CACHE BOOL "Use AVX512 ver. SPQLIOS")
if(TANGOR_TFHEPP_SOURCE_DIR)
    set(TANGOR_TFHEPP_EFFECTIVE_SOURCE_DIR "${TANGOR_TFHEPP_SOURCE_DIR}")
    add_subdirectory("${TANGOR_TFHEPP_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/tfhepp")
else()
    set(TANGOR_TFHEPP_EFFECTIVE_SOURCE_DIR "${TANGOR_CUFHEPP_SOURCE_DIR}/thirdparties/TFHEpp")
    if(NOT EXISTS "${TANGOR_CUFHEPP_SOURCE_DIR}/thirdparties/CMakeLists.txt"
            OR NOT EXISTS "${TANGOR_TFHEPP_EFFECTIVE_SOURCE_DIR}/CMakeLists.txt")
        message(FATAL_ERROR
            "cuFHEpp/TFHEpp submodules are missing. Run "
            "`git submodule update --init --recursive thirdparties/cuFHEpp` "
            "or set TANGOR_TFHEPP_SOURCE_DIR.")
    endif()
    add_subdirectory("${TANGOR_CUFHEPP_SOURCE_DIR}/thirdparties" "${CMAKE_BINARY_DIR}/cufhepp-thirdparties")
endif()
if(TARGET spqlios-intl)
    target_include_directories(spqlios-intl PRIVATE "${TANGOR_TFHEPP_EFFECTIVE_SOURCE_DIR}/include")
endif()
if(TARGET spqlios-stockham)
    target_include_directories(spqlios-stockham PRIVATE "${TANGOR_TFHEPP_EFFECTIVE_SOURCE_DIR}/include")
endif()
set(TANGOR_TFHEPP_COMPILE_DEFINITIONS ${TFHEpp_DEFINITIONS})
foreach(def IN ITEMS
    USE_BLAKE3
    USE_RANDEN
    USE_80BIT_SECURITY
    USE_COMPRESS
    USE_CGGI19
    USE_CONCRETE
    USE_TFHE_RS
    USE_TERNARY
    USE_TERNARY_CMUX
    USE_KEY_BUNDLE
    USE_FFTW3
    USE_MKL
    USE_SPQLIOX_AARCH64
    USE_CONCRETE_FFT
    USE_SPQLIOS_INTL
    USE_SPQLIOS_STOCKHAM
    USE_SPQLIOS_ARITHMETIC
    USE_HEXL
    USE_AVX512)
    if(DEFINED ${def} AND ${${def}})
        list(APPEND TANGOR_TFHEPP_COMPILE_DEFINITIONS ${def})
    endif()
endforeach()
if((DEFINED USE_MKL AND USE_MKL)
        OR (DEFINED USE_CONCRETE_FFT AND USE_CONCRETE_FFT)
        OR (DEFINED USE_SPQLIOS_INTL AND USE_SPQLIOS_INTL))
    list(APPEND TANGOR_TFHEPP_COMPILE_DEFINITIONS USE_INTERLEAVED_FORMAT)
endif()
list(REMOVE_ITEM TANGOR_TFHEPP_COMPILE_DEFINITIONS "")
list(REMOVE_DUPLICATES TANGOR_TFHEPP_COMPILE_DEFINITIONS)
add_compile_definitions(${TANGOR_TFHEPP_COMPILE_DEFINITIONS})
add_subdirectory(src)

if(TANGOR_BUILD_TESTS)
    add_subdirectory(test)
endif()
