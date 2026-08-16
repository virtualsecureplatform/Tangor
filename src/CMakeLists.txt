if(TANGOR_USE_BUNDLED_STARPU OR STARPU_FOUND)
set(tangor_sources tangor.cpp yosys-json-parser.cpp starpu_build_graph.cpp wrappedcodelets.cpp)

if(USE_HOGE)
set(tangor_sources ${tangor_sources} hogewrapper.cpp)
endif()

if(TANGOR_USE_CUFHEPP_STARPU)
set(tangor_sources ${tangor_sources} cufhepp_starpu.cu)
endif()

add_executable(tangor
  ${tangor_sources}
  )

target_link_libraries(tangor PRIVATE tfhe++)
if(USE_HOGE)
find_package(OpenCL REQUIRED)
target_link_libraries(tangor PRIVATE xcl2 rt)
endif()

if(TANGOR_USE_CUFHEPP_STARPU)
find_package(CUDAToolkit REQUIRED)
target_link_libraries(tangor PRIVATE cufhe_gpu CUDA::cudart)
target_compile_definitions(tangor PRIVATE USE_CUFHEPP)
endif()


target_include_directories(tangor
  PRIVATE ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparties/json/single_include
  ${PROJECT_SOURCE_DIR}/thirdparties/xcl2
  )

#For StarPU
if(TANGOR_USE_BUNDLED_STARPU)
    add_dependencies(tangor tangor_starpu_external)
    target_link_libraries(tangor PRIVATE tangor_starpu::starpu)
    target_link_options(tangor PRIVATE "-Wl,-rpath,${TANGOR_BUNDLED_STARPU_PREFIX}/lib")
    if(TANGOR_CUDA_LIBRARY_DIR)
        target_link_options(tangor PRIVATE "-Wl,-rpath,${TANGOR_CUDA_LIBRARY_DIR}")
    endif()
elseif (STARPU_FOUND)
    target_include_directories (tangor PUBLIC ${STARPU_INCLUDE_DIRS})
    target_compile_options     (tangor PRIVATE ${STARPU_CFLAGS_OTHER})
    target_link_directories    (tangor PRIVATE ${STARPU_LIBRARY_DIRS})
    target_link_options        (tangor PRIVATE ${STARPU_LDFLAGS_OTHER})
    target_link_libraries      (tangor PRIVATE ${STARPU_LIBRARIES})
else (STARPU_FOUND)
    message(FATAL_ERROR "StarPU not found")
endif()
endif()

if(TANGOR_BUILD_KVSP_COMPAT)
    # Iyokan embeds the compact mux-RAM netlists so that KVSP blueprints do
    # not depend on a runtime data directory. Keep the generated objects in
    # Tangor's build tree rather than writing into the source checkout.
    foreach(tangor_mux_ram_spec IN ITEMS
            "mux-ram-8-8-8.min.json;mux-ram-8-8-8.o"
            "mux-ram-8-16-16.min.json;mux-ram-8-16-16.o"
            "mux-ram-9-16-16.min.json;mux-ram-9-16-16.o")
        list(GET tangor_mux_ram_spec 0 tangor_mux_ram_json)
        list(GET tangor_mux_ram_spec 1 tangor_mux_ram_object)
        add_custom_command(
            OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${tangor_mux_ram_object}"
            # objcopy derives exported _binary_* symbols from its input name.
            # Run in the source directory so these retain Iyokan's stable
            # symbol names instead of embedding an absolute build path.
            COMMAND ${CMAKE_COMMAND} -E chdir
                "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}"
                ${CMAKE_OBJCOPY} -I binary -O elf64-x86-64 -B i386
                "${tangor_mux_ram_json}"
                "${CMAKE_CURRENT_BINARY_DIR}/${tangor_mux_ram_object}"
            DEPENDS "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/${tangor_mux_ram_json}")
        list(APPEND tangor_iyokan_compat_objects
            "${CMAKE_CURRENT_BINARY_DIR}/${tangor_mux_ram_object}")
    endforeach()

    add_executable(iyokan
        "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/main.cpp"
        "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/iyokan.cpp"
        "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/iyokan_plain.cpp"
        "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/iyokan_tfhepp.cpp"
        "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/error.cpp"
        ${tangor_iyokan_compat_objects})
    target_compile_options(iyokan PRIVATE -Wall -Wextra -Wno-sign-compare)
    target_link_libraries(iyokan PRIVATE
        tfhe++
        Threads::Threads
        OpenMP::OpenMP_CXX
        Backward::Backward
        spdlog::spdlog)
    target_include_directories(iyokan PRIVATE
        "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}"
        "${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/picojson"
        "${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/ThreadPool"
        "${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/CLI11/include"
        "${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/toml11/include"
        "${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/fmt/include"
        "${TANGOR_CUFHEPP_SOURCE_DIR}/thirdparties/TFHEpp/thirdparties/cereal/include")
    target_compile_definitions(iyokan PRIVATE
        ${TANGOR_TFHEPP_COMPILE_DEFINITIONS}
        IYOKAN_GIT_REVISION="tangor-compat")
    if(TANGOR_USE_CUFHEPP_STARPU)
        target_sources(iyokan PRIVATE
            "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/iyokan_cufhe.cpp"
            iyokan-cufhepp-starpu.cu)
        target_link_libraries(iyokan PRIVATE cufhe_gpu CUDA::cudart)
        # cuFHEpp's cufhe_gpu target is normally configured at a project root
        # that also exposes this directory.  Tangor nests it, so preserve the
        # include path Iyokan's CUDA frontend expects for cufhe_gpu.cuh.
        target_include_directories(iyokan PRIVATE
            "${TANGOR_CUFHEPP_SOURCE_DIR}/include")
        target_compile_definitions(iyokan PRIVATE IYOKAN_CUDA_ENABLED)
    endif()
    set_target_properties(iyokan PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

    # When StarPU is present, redirect the ordinary level-0 TFHE gates used by
    # the KVSP frontend into Tangor's codelets. The frontend and RAM machinery
    # remain shared while the gate execution path migrates independently.
    if(TANGOR_KVSP_STARPU_GATE_OFFLOAD AND
            (TANGOR_USE_BUNDLED_STARPU OR STARPU_FOUND))
        target_sources(iyokan PRIVATE iyokan-starpu-codelets.cpp)
        target_include_directories(iyokan PRIVATE "${PROJECT_SOURCE_DIR}/include")
        set_source_files_properties(
            "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/iyokan_tfhepp.cpp"
            PROPERTIES COMPILE_OPTIONS
            "-include;${PROJECT_SOURCE_DIR}/include/iyokan-starpu-offload.hpp")
        if(TANGOR_USE_BUNDLED_STARPU)
            add_dependencies(iyokan tangor_starpu_external)
            target_link_libraries(iyokan PRIVATE tangor_starpu::starpu)
            target_link_options(iyokan PRIVATE
                "-Wl,-rpath,${TANGOR_BUNDLED_STARPU_PREFIX}/lib")
            if(TANGOR_CUDA_LIBRARY_DIR)
                target_link_options(iyokan PRIVATE
                    "-Wl,-rpath,${TANGOR_CUDA_LIBRARY_DIR}")
            endif()
        else()
            target_include_directories(iyokan PRIVATE ${STARPU_INCLUDE_DIRS})
            target_compile_options(iyokan PRIVATE ${STARPU_CFLAGS_OTHER})
            target_link_directories(iyokan PRIVATE ${STARPU_LIBRARY_DIRS})
            target_link_options(iyokan PRIVATE ${STARPU_LDFLAGS_OTHER})
            target_link_libraries(iyokan PRIVATE ${STARPU_LIBRARIES})
        endif()
    endif()

    add_executable(iyokan-packet
        "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/iyokan-packet.cpp"
        "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}/error.cpp")
    target_compile_options(iyokan-packet PRIVATE -Wall -Wextra -Wno-sign-compare)
    target_link_libraries(iyokan-packet PRIVATE
        tfhe++
        Threads::Threads
        OpenMP::OpenMP_CXX
        Backward::Backward
        spdlog::spdlog)
    target_include_directories(iyokan-packet PRIVATE
        "${TANGOR_IYOKAN_COMPAT_SOURCE_DIR}"
        "${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/CLI11/include"
        "${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/toml11/include"
        "${TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR}/fmt/include"
        "${TANGOR_CUFHEPP_SOURCE_DIR}/thirdparties/TFHEpp/thirdparties/cereal/include")
    target_compile_definitions(iyokan-packet PRIVATE
        ${TANGOR_TFHEPP_COMPILE_DEFINITIONS})
    set_target_properties(iyokan-packet PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
