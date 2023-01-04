#include <verilated.h>
#include <verilated_fst_c.h>
#include <VMixColumns.h>
#include <VinverseMixColumns.h>

#include <array>
#include <cstdint>
#include <iostream>


int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	VMixColumns *dut = new VMixColumns();
	VinverseMixColumns *dutinv = new VinverseMixColumns();
	constexpr std::array<uint32_t,4> data = {0x3243f6a8, 0x885a308d, 0x313198a2, 0xe0370734}; 
	for(int i = 0; i < 4; i++) dut->x[i] = data[i];
	dut->eval();
	std::cout<<"Enc done"<<std::endl;
	for(int i = 0; i < 4; i++) dutinv->x[i] = dut->z[i];
	dutinv->eval();
	for(int i = 0; i < 4; i++) std::cout<<std::hex<<dutinv->z[i]<<std::endl;
	for(int i = 0; i < 4; i++) assert(dutinv->z[i] == data[i]);
}
