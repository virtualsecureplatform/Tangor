#include <verilated.h>
#include <verilated_fst_c.h>
#include <Vaes_1cc.h>
#include <Vinverse_aes_1cc.h>

#include <array>
#include <cstdint>
#include <iostream>
#include <random>
#include <AES.h>

void clock(Vaes_1cc *dut, VerilatedFstC* tfp){
  static uint time_counter = 0;
  dut->eval();
  tfp->dump(1000*time_counter);
  time_counter++;
  dut->clock = !dut->clock;
  dut->eval();
  tfp->dump(1000*time_counter);
  time_counter++;
  dut->clock = !dut->clock;
}

void clock(Vinverse_aes_1cc *dut, VerilatedFstC* tfp){
  static uint time_counter = 0;
  dut->eval();
  tfp->dump(1000*time_counter);
  time_counter++;
  dut->clock = !dut->clock;
  dut->eval();
  tfp->dump(1000*time_counter);
  time_counter++;
  dut->clock = !dut->clock;
}


int main(int argc, char** argv) {
	Verilated::commandArgs(argc, argv);
	Vaes_1cc *dut = new Vaes_1cc();
	Vinverse_aes_1cc *dutinv = new Vinverse_aes_1cc();
	Verilated::traceEverOn(true);
	VerilatedFstC* tfp = new VerilatedFstC;
	dut->trace(tfp, 100);  // Trace 100 levels of hierarchy
	tfp->open("simx.fst");
	constexpr int Nk = 4;
	std::random_device seed_gen;
    std::default_random_engine engine(seed_gen());
	std::uniform_int_distribution<uint32_t> U32dist(0, UINT32_MAX);
	std::array<uint32_t,4>input_text;
	std::array<uint32_t,Nk>key;
	std::array<uint32_t,4>cipher = {};
	// e0370734313198a2885a308d3243f6a8
	// constexpr std::array<uint32_t,4>input_text = {0x3243f6a8, 0x885a308d, 0x313198a2, 0xe0370734};
	// 0c0d0e0f08090a0b0405060700010203
	// constexpr std::array<uint32_t,Nk>key = {0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f};
	// constexpr std::array<uint32_t,4>cipher = {0xb60dbb17, 0xafdecfe8, 0xa980ba86, 0x739fb585};
	for(int i = 0; i < 4; i++) input_text[i] = U32dist(engine);
	for(int i = 0; i < Nk; i++) key[i] = U32dist(engine);

	AES aes(AESKeyLength::AES_128);
	unsigned char charplain[32];
	for(int i = 0; i < 4; i++) for(int j = 0; j < 4; j++) charplain[i*4+j] = input_text[i]>>(8*j);
	unsigned char charkey[32];
	for(int i = 0; i < Nk; i++) for(int j = 0; j < 4; j++) charkey[i*4+j] = key[i]>>(8*j);
	auto c = aes.EncryptECB(charplain, 128, charkey);
	// for(int i = 0; i < 4; i++) for(int j = 0; j < 4; j++) assert(c[i*4+j] = cipher[i]>>(8*j));
	for(int i = 0; i < 4; i++) for(int j = 0; j < 4; j++) cipher[i] += c[i*4+j] << (8*j);

	dut->clock = 0;
	dut->reset = 0;
	for(int i = 0; i < 4; i++) dut->e_input[i] = input_text[i];
	for(int i = 0; i < Nk; i++) dut->g_input[i] = key[i];
	clock(dut,tfp);
	for(int i = 0; i < 4; i++) std::cout<<std::hex<<dut->o[i]<<std::endl;
	for(int i = 0; i < 4; i++) assert(dut->o[i] == cipher[i]);
	std::cout<<"Enc done"<<std::endl;
	for(int i = 0; i < 4; i++)dutinv->e_input[i] = dut->o[i];
	for(int i = 0; i < Nk; i++) dutinv->g_input[i] = key[i];
	clock(dutinv,tfp);
	dut->final();
	dutinv->final();
    tfp->close();
	for(int i = 0; i < 4; i++) std::cout<<std::hex<<dutinv->o[i]<<std::endl;
	for(int i = 0; i < 4; i++)assert(dutinv->o[i] == input_text[i]);

}
