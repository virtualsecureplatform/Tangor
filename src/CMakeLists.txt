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
    target_link_options(tangor PRIVATE
        "-Wl,-rpath,${TANGOR_BUNDLED_STARPU_PREFIX}/lib"
        "-Wl,-rpath,${TANGOR_CUDA_LIBRARY_DIR}")
elseif (STARPU_FOUND)
    target_include_directories (tangor PUBLIC ${STARPU_INCLUDE_DIRS})
    target_compile_options     (tangor PRIVATE ${STARPU_CFLAGS_OTHER})
    target_link_directories    (tangor PRIVATE ${STARPU_LIBRARY_DIRS})
    target_link_options        (tangor PRIVATE ${STARPU_LDFLAGS_OTHER})
    target_link_libraries      (tangor PRIVATE ${STARPU_LIBRARIES})
else (STARPU_FOUND)
    message(FATAL_ERROR "StarPU not found")
endif()
