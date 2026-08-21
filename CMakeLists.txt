cmake_minimum_required(VERSION 3.18)

# Accept the cache variables used by KVSP's existing Iyokan build rules.  This
# lets callers replace the source directory without changing their Makefile or
# deployment scripts.
set(IYOKAN_MARCH "native" CACHE STRING
    "Target -march value accepted for Iyokan/KVSP compatibility")
option(IYOKAN_ENABLE_CUDA
       "Enable the CUDA-compatible Iyokan evaluator path" OFF)
option(IYOKAN_80BIT_SECURITY
       "Use Iyokan's 80-bit TFHE security parameter" OFF)
if(IYOKAN_ENABLE_CUDA)
    set(USE_CUFHEPP ON CACHE BOOL
        "Use cuFHEpp CUDA gate kernels through StarPU CUDA workers" FORCE)
    set(TANGOR_USE_BUNDLED_STARPU ON CACHE BOOL
        "Build and use bundled StarPU instead of pkg-config StarPU" FORCE)
    # Iyokan disables cuFHEpp key bundles for CUDA evaluation. Its RAM
    # bootstrap path requires the regular BSK archive format, and retaining
    # key bundles makes Iyokan-generated evaluation keys unreadable here.
    set(USE_KEY_BUNDLE OFF CACHE BOOL
        "Use regular cuFHEpp bootstrapping-key archives for Iyokan compatibility" FORCE)
endif()
if(IYOKAN_80BIT_SECURITY)
    set(USE_80BIT_SECURITY ON CACHE BOOL "Use TFHEpp 80-bit security" FORCE)
endif()

set(CMAKE_CXX_FLAGS "-march=${IYOKAN_MARCH} -O3 -g")
# TFHEpp reads this normal variable while Tangor adds it as a subdirectory.
# Match Iyokan's forwarding behaviour so KVSP's AVX2 build is genuinely
# portable rather than silently reverting to the host architecture.
set(TFHEPP_MARCH "${IYOKAN_MARCH}")
project(tangor CXX C)
set(CMAKE_CXX_STANDARD 20)
option(USE_HOGE "Use HOGE for 2-input gates" OFF)
option(USE_CUFHEPP "Use cuFHEpp CUDA gate kernels through StarPU CUDA workers" OFF)
option(TANGOR_USE_BUNDLED_STARPU "Build and use bundled StarPU instead of pkg-config StarPU" ON)
option(TANGOR_BUILD_TESTS "Build Tangor test clients" ON)
option(TANGOR_BUILD_KVSP_COMPAT "Build Iyokan command-line compatibility binaries for KVSP" ON)
option(TANGOR_FETCH_IYOKAN_COMPAT
       "Fetch the pinned Iyokan frontend when a local compatibility source is unavailable" ON)
option(TANGOR_KVSP_STARPU_GATE_OFFLOAD "Schedule KVSP homomorphic gates with StarPU" ON)
option(TANGOR_STARPU_PROFILE
       "Enable StarPU worker/task profiling and FxT tracing by default" OFF)
if(TANGOR_STARPU_PROFILE)
    add_compile_definitions(TANGOR_STARPU_PROFILE_DEFAULT=1)
endif()
set(TANGOR_CUFHEPP_SOURCE_DIR "${PROJECT_SOURCE_DIR}/thirdparties/cuFHEpp" CACHE PATH "cuFHEpp checkout that provides TFHEpp")
set(TANGOR_TFHEPP_SOURCE_DIR "" CACHE PATH "Optional TFHEpp checkout override; defaults to cuFHEpp's nested TFHEpp")
set(TANGOR_BUNDLED_STARPU_MAX_CPUS "256" CACHE STRING "Bundled StarPU CPU-worker capacity; also sizes STARPU_NMAXWORKERS for many CUDA streams")
set(TANGOR_BUNDLED_STARPU_MAX_COMBINED_WORKERS "1" CACHE STRING "Bundled StarPU combined-worker capacity; Tangor does not use parallel StarPU tasks")

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
set(TANGOR_STARPU_HAS_CUDA OFF)

if(TANGOR_USE_BUNDLED_STARPU)
    if(NOT EXISTS "${PROJECT_SOURCE_DIR}/thirdparties/starpu/autogen.sh")
        message(FATAL_ERROR
            "Bundled StarPU is missing. Run "
            "`git submodule update --init thirdparties/starpu` "
            "or configure with -DTANGOR_USE_BUNDLED_STARPU=OFF.")
    endif()

    include(ExternalProject)
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

    if(USE_CUFHEPP)
        find_package(CUDAToolkit REQUIRED)
        get_filename_component(TANGOR_CUDA_BIN_DIR "${CUDAToolkit_NVCC_EXECUTABLE}" DIRECTORY)
        get_filename_component(TANGOR_CUDA_ROOT_DIR "${TANGOR_CUDA_BIN_DIR}" DIRECTORY)
        list(GET CUDAToolkit_INCLUDE_DIRS 0 TANGOR_CUDA_INCLUDE_DIR)
        set(TANGOR_CUDA_LIBRARY_DIR "${CUDAToolkit_LIBRARY_DIR}")
        set(TANGOR_STARPU_HAS_CUDA ON)
        set(TANGOR_BUNDLED_STARPU_ENV
            "PATH=${TANGOR_CUDA_BIN_DIR}:$ENV{PATH}"
            "LD_LIBRARY_PATH=${TANGOR_CUDA_LIBRARY_DIR}:$ENV{LD_LIBRARY_PATH}"
            "NVCC=${CUDAToolkit_NVCC_EXECUTABLE}"
            "NVCCFLAGS=-arch=sm_80 -Wno-deprecated-gpu-targets")
        set(TANGOR_BUNDLED_STARPU_CUDA_CONFIGURE_ARGS
            "--enable-cuda"
            "--with-cuda-dir='${TANGOR_CUDA_ROOT_DIR}'"
            "--with-cuda-include-dir='${TANGOR_CUDA_INCLUDE_DIR}'"
            "--with-cuda-lib-dir='${TANGOR_CUDA_LIBRARY_DIR}'"
            "--with-cublas-dir='${TANGOR_CUDA_ROOT_DIR}'"
            "--with-cublas-include-dir='${TANGOR_CUDA_INCLUDE_DIR}'"
            "--with-cublas-lib-dir='${TANGOR_CUDA_LIBRARY_DIR}'")
    else()
        set(TANGOR_BUNDLED_STARPU_ENV "PATH=$ENV{PATH}")
        set(TANGOR_BUNDLED_STARPU_CUDA_CONFIGURE_ARGS "--disable-cuda")
    endif()
    if(TANGOR_STARPU_PROFILE)
        # Explicitly enable FxT rather than relying on StarPU's optional
        # auto-detection. This makes the profiling CMake switch fail clearly
        # when the required fxt development package is unavailable.
        pkg_check_modules(FXT REQUIRED fxt)
        set(TANGOR_BUNDLED_STARPU_PROFILE_CONFIGURE_ARGS "--enable-fxt")
    else()
        set(TANGOR_BUNDLED_STARPU_PROFILE_CONFIGURE_ARGS "--disable-fxt")
    endif()
    set(TANGOR_BUNDLED_STARPU_PREFIX "${CMAKE_BINARY_DIR}/starpu-install")
    set(TANGOR_BUNDLED_STARPU_WORK_DIR "${CMAKE_BINARY_DIR}/starpu-build")
    set(TANGOR_BUNDLED_STARPU_LIBRARY "${TANGOR_BUNDLED_STARPU_PREFIX}/lib/libstarpu-1.4.so")
    set(TANGOR_BUNDLED_STARPU_INCLUDE_DIR "${TANGOR_BUNDLED_STARPU_PREFIX}/include/starpu/1.4")
    file(MAKE_DIRECTORY "${TANGOR_BUNDLED_STARPU_INCLUDE_DIR}" "${TANGOR_BUNDLED_STARPU_PREFIX}/lib")

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
                ${TANGOR_BUNDLED_STARPU_CUDA_CONFIGURE_ARGS} \
                ${TANGOR_BUNDLED_STARPU_PROFILE_CONFIGURE_ARGS} \
                --enable-maxcpus='${TANGOR_BUNDLED_STARPU_MAX_CPUS}' \
                --enable-maxcombinedworkers='${TANGOR_BUNDLED_STARPU_MAX_COMBINED_WORKERS}' \
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
        INTERFACE_INCLUDE_DIRECTORIES "${TANGOR_BUNDLED_STARPU_INCLUDE_DIR}")
    if(TANGOR_STARPU_HAS_CUDA)
        set_property(TARGET tangor_starpu::starpu APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES "Threads::Threads;CUDA::cudart;CUDA::cublas")
    else()
        set_property(TARGET tangor_starpu::starpu APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES "Threads::Threads")
    endif()
    add_dependencies(tangor_starpu::starpu tangor_starpu_external)
else()
    pkg_check_modules(STARPU QUIET starpu-1.4)
    if(NOT STARPU_FOUND)
        if(TANGOR_BUILD_KVSP_COMPAT)
            message(WARNING
                "StarPU was not found; the `tangor` evaluator target will be "
                "unavailable, but the KVSP packet compatibility tool will be built.")
        else()
            message(FATAL_ERROR
                "StarPU was not found. Install starpu-1.4, configure with "
                "-DTANGOR_USE_BUNDLED_STARPU=ON, or enable "
                "-DTANGOR_BUILD_KVSP_COMPAT=ON to build packet tooling only.")
        endif()
    endif()
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
if(TANGOR_KVSP_STARPU_GATE_OFFLOAD AND
        NOT (TANGOR_USE_BUNDLED_STARPU OR STARPU_FOUND))
    message(FATAL_ERROR
        "Tangor's KVSP evaluator requires StarPU. Install starpu-1.4 or "
        "configure with -DTANGOR_USE_BUNDLED_STARPU=ON.")
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
# cuFHEpp consumes TFHEpp's split Fourier layout. Use that layout in CPU-only
# builds too, so one evaluation key and one encrypted ROM/RAM packet can move
# between Tangor CPU workers, CUDA workers, and Iyokan without conversion.
set(USE_SPQLIOS_INTL OFF CACHE BOOL
    "Use TFHEpp's split Fourier layout for CPU/CUDA archive compatibility" FORCE)
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
    USE_BLOCK_BINARY
    USE_CONCRETE
    USE_TFHE_RS
    USE_TERNARY
    USE_TERNARY_CMUX
    USE_KEY_BUNDLE
    USE_SUBSET_KEY
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

# KVSP communicates with its FHE backend through `iyokan` and
# `iyokan-packet`. Compile the reference packet/archive implementation with
# Tangor's TFHEpp so generated keys, packets, and snapshots retain the exact
# binary layout expected by KVSP.
if(TANGOR_BUILD_KVSP_COMPAT)
    set(TANGOR_IYOKAN_COMPAT_SOURCE_DIR "${PROJECT_SOURCE_DIR}/../Iyokan/src"
        CACHE PATH "Iyokan frontend source directory used by the KVSP compatibility target")
    set(TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR "${PROJECT_SOURCE_DIR}/../Iyokan/thirdparty"
        CACHE PATH "Iyokan frontend third-party directory used by the KVSP compatibility target")
    set(tangor_iyokan_compat_missing FALSE)
    foreach(tangor_compat_path IN ITEMS
            "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/iyokan-packet.cpp"
            "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/packet.hpp"
            "${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/CLI11/include/CLI/CLI.hpp"
            "${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/toml11/include/toml.hpp")
        if(NOT EXISTS "${tangor_compat_path}")
            set(tangor_iyokan_compat_missing TRUE)
        endif()
    endforeach()
    if(tangor_iyokan_compat_missing AND TANGOR_FETCH_IYOKAN_COMPAT)
        include(FetchContent)
        FetchContent_Declare(tangor_iyokan_frontend
            GIT_REPOSITORY https://github.com/virtualsecureplatform/Iyokan.git
            GIT_TAG 8c2d26e19bb91740abf478bd0204e3b8eb10a84f
            GIT_SHALLOW TRUE
            GIT_SUBMODULES_RECURSE TRUE)
        FetchContent_GetProperties(tangor_iyokan_frontend)
        if(NOT tangor_iyokan_frontend_POPULATED)
            FetchContent_Populate(tangor_iyokan_frontend)
        endif()
        set(TANGOR_IYOKAN_COMPAT_SOURCE_DIR
            "${tangor_iyokan_frontend_SOURCE_DIR}/src" CACHE PATH
            "Iyokan frontend source directory used by the KVSP compatibility target" FORCE)
        set(TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR
            "${tangor_iyokan_frontend_SOURCE_DIR}/thirdparty" CACHE PATH
            "Iyokan frontend third-party directory used by the KVSP compatibility target" FORCE)
        set(tangor_iyokan_compat_missing FALSE)
        foreach(tangor_compat_path IN ITEMS
                "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/iyokan-packet.cpp"
                "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/packet.hpp"
                "${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/CLI11/include/CLI/CLI.hpp"
                "${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/toml11/include/toml.hpp")
            if(NOT EXISTS "${tangor_compat_path}")
                set(tangor_iyokan_compat_missing TRUE)
            endif()
        endforeach()
    endif()
    if(tangor_iyokan_compat_missing)
        message(FATAL_ERROR
            "TANGOR_BUILD_KVSP_COMPAT could not find the Iyokan frontend. "
            "Set TANGOR_IYOKAN_COMPAT_SOURCE_DIR and "
            "TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR, enable "
            "TANGOR_FETCH_IYOKAN_COMPAT, or disable the compatibility target.")
    endif()
    add_subdirectory("${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/spdlog"
                     "${CMAKE_BINARY_DIR}/iyokan-compat-spdlog")
    add_subdirectory("${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/backward-cpp"
                     "${CMAKE_BINARY_DIR}/iyokan-compat-backward")
endif()
add_subdirectory(src)

if(TANGOR_BUILD_TESTS)
    add_subdirectory(test)
endif()
