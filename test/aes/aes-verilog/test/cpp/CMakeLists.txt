add_executable(cpptest cpptest.cpp ${PROJECT_SOURCE_DIR}/thirdparties/AES/src/AES.cpp)
target_include_directories(cpptest
  PUBLIC ${PROJECT_SOURCE_DIR}/thirdparties/AES/src
  )

file(GLOB encrypt_sources RELATIVE "${PROJECT_SOURCE_DIR}/encrypt" "*.v")
verilate(cpptest SOURCES ${PROJECT_SOURCE_DIR}/encrypt/aes_1cc.v INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/encrypt THREADS 4 VERILATOR_ARGS -Wno-UNOPTFLAT -Wno-LITENDIAN -Wno-IMPLICIT TRACE_FST)
verilate(cpptest SOURCES ${PROJECT_SOURCE_DIR}/decrypt/inverse_aes_1cc.v INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/decrypt THREADS 4 VERILATOR_ARGS -Wno-UNOPTFLAT -Wno-LITENDIAN -Wno-IMPLICIT TRACE_FST)