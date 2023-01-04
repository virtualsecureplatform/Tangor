#include <verilated.h>
#include <verilated_fst_c.h>
#include <VSubBytes.h>
#include <VinverseSubBytes.h>

#include <array>
#include <cstdint>
#include <iostream>


int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	VSubBytes *dut = new VSubBytes();
	VinverseSubBytes *dutinv = new VinverseSubBytes();
	constexpr uint8_t data = 42; 
	dut->num = data;
	dut->eval();
	std::cout<<"Enc done"<<std::endl;
	dutinv->num = dut->SubByte;
	dutinv->eval();
	assert(dutinv->SubByte == data);
}
