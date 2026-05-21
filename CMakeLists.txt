cmake_minimum_required(VERSION 3.18)
set(CMAKE_CXX_FLAGS "-march=native -O3 -g")
project(tangor CXX C)
set(CMAKE_CXX_STANDARD 20)
option(USE_HOGE "Use HOGE for 2-input gates" OFF)
option(USE_CUFHEPP "Use cuFHEpp CUDA gate kernels through StarPU CUDA workers" OFF)
option(TANGOR_USE_BUNDLED_STARPU "Build and use bundled StarPU instead of pkg-config StarPU" ${USE_CUFHEPP})
option(TANGOR_BUILD_TESTS "Build Tangor test clients" ON)
set(TANGOR_CUFHEPP_SOURCE_DIR "${PROJECT_SOURCE_DIR}/thirdparties/cuFHEpp" CACHE PATH "cuFHEpp checkout that provides TFHEpp")
set(TANGOR_TFHEPP_SOURCE_DIR "" CACHE PATH "Optional TFHEpp checkout override; defaults to cuFHEpp's nested TFHEpp")

find_package(PkgConfig REQUIRED)
set(TANGOR_STARPU_HAS_CUDA OFF)

if(TANGOR_USE_BUNDLED_STARPU)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparties/starpu/autogen.sh")
        message(FATAL_ERROR
            "Bundled StarPU is missing. Run "
            "`git submodule update --init thirdparties/starpu` "
            "or configure with -DTANGOR_USE_BUNDLED_STARPU=OFF.")
    endif()

    include(ExternalProject)
    find_package(CUDAToolkit REQUIRED)
    find_package(Threads REQUIRED)
    foreach(tangor_starpu_tool IN ITEMS autoconf automake autoreconf bash libtool libtoolize make)
        string(TOUPPER "${tangor_starpu_tool}" tangor_starpu_tool_upper)
        find_program(TANGOR_${tangor_starpu_tool_upper}_EXECUTABLE "${tangor_starpu_tool}")
        if(NOT TANGOR_${tangor_starpu_tool_upper}_EXECUTABLE)
            message(FATAL_ERROR
                "Bundled StarPU requires `${tangor_starpu_tool}` to bootstrap. "
                "Install StarPU autotools dependencies or configure with "
                "-DTANGOR_USE_BUNDLED_STARPU=OFF.")
        endif()
    endforeach()

    get_filename_component(TANGOR_CUDA_BIN_DIR "${CUDAToolkit_NVCC_EXECUTABLE}" DIRECTORY)
    get_filename_component(TANGOR_CUDA_ROOT_DIR "${TANGOR_CUDA_BIN_DIR}" DIRECTORY)
    list(GET CUDAToolkit_INCLUDE_DIRS 0 TANGOR_CUDA_INCLUDE_DIR)
    set(TANGOR_CUDA_LIBRARY_DIR "${CUDAToolkit_LIBRARY_DIR}")

    set(TANGOR_STARPU_HAS_CUDA ON)
    set(TANGOR_BUNDLED_STARPU_PREFIX "${CMAKE_BINARY_DIR}/starpu-install")
    set(TANGOR_BUNDLED_STARPU_WORK_DIR "${CMAKE_BINARY_DIR}/starpu-build")
    set(TANGOR_BUNDLED_STARPU_LIBRARY "${TANGOR_BUNDLED_STARPU_PREFIX}/lib/libstarpu-1.4.so")
    set(TANGOR_BUNDLED_STARPU_INCLUDE_DIR "${TANGOR_BUNDLED_STARPU_PREFIX}/include/starpu/1.4")
    file(MAKE_DIRECTORY "${TANGOR_BUNDLED_STARPU_INCLUDE_DIR}" "${TANGOR_BUNDLED_STARPU_PREFIX}/lib")

    set(TANGOR_BUNDLED_STARPU_ENV
        "PATH=${TANGOR_CUDA_BIN_DIR}:$ENV{PATH}"
        "LD_LIBRARY_PATH=${TANGOR_CUDA_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH}"
        "NVCC=${CUDAToolkit_NVCC_EXECUTABLE}"
        "NVCCFLAGS=-arch=sm_80 -Wno-deprecated-gpu-targets")

    ExternalProject_Add(tangor_starpu_external
        SOURCE_DIR "${PROJECT_SOURCE_DIR}/thirdparties/starpu"
        BINARY_DIR "${TANGOR_BUNDLED_STARPU_WORK_DIR}/build"
        INSTALL_DIR "${TANGOR_BUNDLED_STARPU_PREFIX}"
        CONFIGURE_COMMAND
            ${CMAKE_COMMAND} -E env ${TANGOR_BUNDLED_STARPU_ENV}
            ${TANGOR_BASH_EXECUTABLE} -c
            "rm -rf '${TANGOR_BUNDLED_STARPU_WORK_DIR}/src' && \
             cp -a '<SOURCE_DIR>' '${TANGOR_BUNDLED_STARPU_WORK_DIR}/src' && \
             (git -C '<SOURCE_DIR>' log -n 1 --pretty='%H%d' 2>/dev/null || echo unknown) > '${TANGOR_BUNDLED_STARPU_WORK_DIR}/src/STARPU-REVISION' && \
             rm -rf '${TANGOR_BUNDLED_STARPU_WORK_DIR}/src/.git' && \
             cd '${TANGOR_BUNDLED_STARPU_WORK_DIR}/src' && ./autogen.sh && \
             mkdir -p '<BINARY_DIR>' && cd '<BINARY_DIR>' && \
             '${TANGOR_BUNDLED_STARPU_WORK_DIR}/src/configure' \
                --prefix='<INSTALL_DIR>' \
                --libdir='<INSTALL_DIR>/lib' \
                --enable-cuda \
                --with-cuda-dir='${TANGOR_CUDA_ROOT_DIR}' \
                --with-cuda-include-dir='${TANGOR_CUDA_INCLUDE_DIR}' \
                --with-cuda-lib-dir='${TANGOR_CUDA_LIBRARY_DIR}' \
                --with-cublas-dir='${TANGOR_CUDA_ROOT_DIR}' \
                --with-cublas-include-dir='${TANGOR_CUDA_INCLUDE_DIR}' \
                --with-cublas-lib-dir='${TANGOR_CUDA_LIBRARY_DIR}' \
                --disable-opencl \
                --disable-mpi \
                --disable-fortran \
                --disable-build-tests \
                --disable-build-examples \
                --disable-build-doc \
                --disable-starpupy \
                --disable-starpufft"
        BUILD_COMMAND ${CMAKE_COMMAND} -E env ${TANGOR_BUNDLED_STARPU_ENV} ${TANGOR_MAKE_EXECUTABLE} -j2
        INSTALL_COMMAND ${CMAKE_COMMAND} -E env ${TANGOR_BUNDLED_STARPU_ENV} ${TANGOR_MAKE_EXECUTABLE} install
        BUILD_BYPRODUCTS "${TANGOR_BUNDLED_STARPU_LIBRARY}")

    add_library(tangor_starpu::starpu SHARED IMPORTED GLOBAL)
    set_target_properties(tangor_starpu::starpu PROPERTIES
        IMPORTED_LOCATION "${TANGOR_BUNDLED_STARPU_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${TANGOR_BUNDLED_STARPU_INCLUDE_DIR}"
        INTERFACE_LINK_LIBRARIES "Threads::Threads;CUDA::cudart;CUDA::cublas")
    add_dependencies(tangor_starpu::starpu tangor_starpu_external)
else()
    pkg_check_modules(STARPU REQUIRED starpu-1.4)
    foreach(starpu_include_dir IN LISTS STARPU_INCLUDE_DIRS)
        if(EXISTS "${starpu_include_dir}/starpu_config.h")
            file(STRINGS "${starpu_include_dir}/starpu_config.h" starpu_cuda_define
                REGEX "^[ \t]*#[ \t]*define[ \t]+STARPU_USE_CUDA")
            if(starpu_cuda_define)
                set(TANGOR_STARPU_HAS_CUDA ON)
            endif()
        endif()
    endforeach()
endif()

set(TANGOR_USE_CUFHEPP_STARPU OFF)
if(USE_CUFHEPP)
    if(TANGOR_STARPU_HAS_CUDA)
        set(TANGOR_USE_CUFHEPP_STARPU ON)
    else()
        message(WARNING
            "USE_CUFHEPP was requested, but the installed StarPU was built "
            "without CUDA support. Tangor will build the CPU StarPU codelets "
            "only; use a CUDA-enabled StarPU build to enable cuFHEpp codelets.")
    endif()
endif()

if(TANGOR_USE_CUFHEPP_STARPU)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 20)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    set(CMAKE_CUDA_ARCHITECTURES "80" CACHE STRING "CUDA architectures for cuFHEpp")
    add_compile_definitions(USE_CUFHEPP)
endif()

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
if(NOT TANGOR_USE_CUFHEPP_STARPU)
    set(USE_SPQLIOS_INTL ON CACHE BOOL "Use interleaved-format SPQLIOS")
endif()
set(USE_AVX512 OFF CACHE BOOL "Use AVX512 ver. SPQLIOS")
if(TANGOR_TFHEPP_SOURCE_DIR)
    if(TANGOR_USE_CUFHEPP_STARPU)
        message(FATAL_ERROR "USE_CUFHEPP requires cuFHEpp's nested TFHEpp; unset TANGOR_TFHEPP_SOURCE_DIR.")
    endif()
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
    if(TANGOR_USE_CUFHEPP_STARPU)
        add_subdirectory("${TANGOR_CUFHEPP_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/cufhepp")
    else()
        add_subdirectory("${TANGOR_CUFHEPP_SOURCE_DIR}/thirdparties" "${CMAKE_BINARY_DIR}/cufhepp-thirdparties")
    endif()
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
    USE_AVX512
    USE_FFT
    USE_GPU_FFT)
    if(DEFINED ${def})
        if(${${def}})
            list(APPEND TANGOR_TFHEPP_COMPILE_DEFINITIONS ${def})
        endif()
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
