add_executable(sbox cpptest.cpp)

file(GLOB encrypt_sources RELATIVE "${PROJECT_SOURCE_DIR}/encrypt" "*.v")
verilate(sbox SOURCES SubBytes.v VERILATOR_ARGS -Wno-UNOPTFLAT -Wno-LITENDIAN -Wno-IMPLICIT)
verilate(sbox SOURCES inverseSubBytes.v VERILATOR_ARGS -Wno-UNOPTFLAT -Wno-LITENDIAN -Wno-IMPLICIT)