add_executable(shiftrows cpptest.cpp)

file(GLOB encrypt_sources RELATIVE "${PROJECT_SOURCE_DIR}/encrypt" "*.v")
verilate(shiftrows SOURCES ShiftRows.v VERILATOR_ARGS -Wno-UNOPTFLAT -Wno-LITENDIAN -Wno-IMPLICIT)
verilate(shiftrows SOURCES inverseShiftRows.v VERILATOR_ARGS -Wno-UNOPTFLAT -Wno-LITENDIAN -Wno-IMPLICIT)