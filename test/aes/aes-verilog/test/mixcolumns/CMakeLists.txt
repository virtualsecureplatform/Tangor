add_executable(mixcolumns cpptest.cpp)

file(GLOB encrypt_sources RELATIVE "${PROJECT_SOURCE_DIR}/encrypt" "*.v")
verilate(mixcolumns SOURCES MixColumns.v VERILATOR_ARGS -Wno-UNOPTFLAT -Wno-LITENDIAN -Wno-IMPLICIT)
verilate(mixcolumns SOURCES inverseMixColumns.v VERILATOR_ARGS -Wno-UNOPTFLAT -Wno-LITENDIAN -Wno-IMPLICIT)