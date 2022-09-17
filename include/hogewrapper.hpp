#pragma once
#include <xcl2.hpp>
#include <tfhe++.hpp>
#include <starpu.h>

#define PC_NAME(n) n | XCL_MEM_TOPOLOGY

namespace Tangor{

void HOGEinit(const std::string& binaryFile, const TFHEpp::EvalKey& ek);

	constexpr uint buswidthlb = 9;
	constexpr uint buswords = 1U<<(buswidthlb-6);
	constexpr uint iksknumbus = 10;
	constexpr uint totaliksknumbus = 40;
	constexpr uint bknumbus = 8;
	constexpr uint cyclebit = 5;
	constexpr uint numcycle = 1<<cyclebit;
	constexpr uint wordsinbus = (1U<<buswidthlb)/std::numeric_limits<typename TFHEpp::lvl0param::T>::digits;

extern cl_mem_ext_ptr_t inaBuf, inbBuf, resBuf;
extern std::array<cl_mem_ext_ptr_t, iksknumbus> ikskBufs;
extern std::array<cl_mem_ext_ptr_t, bknumbus> bknttBufs;

extern cl::Context context;
extern cl::CommandQueue q;
extern cl::Kernel kernel;

extern std::array<std::shared_ptr<cl::Buffer>, bknumbus> buffer_bkntts;
extern std::array<std::shared_ptr<cl::Buffer>, iksknumbus> buffer_iksks;

template<uint16_t scaleaindex, uint16_t scalebindex, uint16_t offsetindex>
void HOGEwrap(void *buffers[], void *cl_arg){
cl_int err;
using P = TFHEpp::lvl1param;
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,tlwea,tlweb;
for(int i = 0; i<= P::n; i++) tlwea[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
for(int i = 0; i<= P::n; i++) tlweb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[2]))[i];

resBuf.obj = res.data();
resBuf.param = 0;
resBuf.flags = PC_NAME(0);

inaBuf.obj = tlwea.data();
inaBuf.param = 0;
inaBuf.flags = PC_NAME(1);

inbBuf.obj = tlweb.data();
inbBuf.param = 0;
inbBuf.flags = PC_NAME(2);

OCL_CHECK(err, cl::Buffer buffer_ina(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                        sizeof(tlwea), &inaBuf, &err));
OCL_CHECK(err, cl::Buffer buffer_inb(context, CL_MEM_READ_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                        sizeof(tlweb), &inbBuf, &err));
OCL_CHECK(err, cl::Buffer buffer_res(context, CL_MEM_WRITE_ONLY | CL_MEM_EXT_PTR_XILINX | CL_MEM_USE_HOST_PTR,
                                        sizeof(res), &resBuf, &err));

// Setting the kernel Arguments
OCL_CHECK(err, err = (kernel).setArg(0, scaleaindex)); // scalea
OCL_CHECK(err, err = (kernel).setArg(1, scalebindex)); // scaleb
OCL_CHECK(err, err = (kernel).setArg(2, offsetindex)); // offset
OCL_CHECK(err, err = (kernel).setArg(3, buffer_res));
OCL_CHECK(err, err = (kernel).setArg(4, buffer_ina));
OCL_CHECK(err, err = (kernel).setArg(5, buffer_inb));

OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_ina,buffer_inb}, 0 /* 0 means from host*/));
cl_event event;
cl::Event eventpp(event);
OCL_CHECK(err, err = q.enqueueTask(kernel,NULL,&eventpp));
OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_res}, CL_MIGRATE_MEM_OBJECT_HOST));
q.finish();
for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
clReleaseEvent(event);
}
}