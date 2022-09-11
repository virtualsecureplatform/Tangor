set(tangor_sources tangor.cpp yosys-json-parser.cpp starpu_build_graph.cpp wrappedcodelets.cpp tfheppwrapper.cpp)

if(USE_HOGE)
set(tangor_sources ${tangor_sources} hogewrapper.cpp)
endif()

add_executable(tangor
  ${tangor_sources}
  )

target_link_libraries(tangor PRIVATE tfhe++)
if(USE_HOGE)
target_link_libraries(tangor PRIVATE xcl2)
endif()


target_include_directories(tangor
  PUBLIC ${PROJECT_SOURCE_DIR}/include
  ${PROJECT_SOURCE_DIR}/thirdparties/json/single_include
  ${PROJECT_SOURCE_DIR}/thirdparties/xcl2
  )

#For StarPU
find_package(PkgConfig)
pkg_check_modules(STARPU REQUIRED starpu-1.3)
if (STARPU_FOUND)
    target_include_directories (tangor PUBLIC ${STARPU_INCLUDE_DIRS})
    target_link_directories    (tangor PRIVATE ${STARPU_STATIC_LIBRARY_DIRS})
    target_link_libraries      (tangor PRIVATE ${STARPU_STATIC_LIBRARIES})
else (STARPU_FOUND)
    message(FATAL_ERROR "StarPU not found")
endif()