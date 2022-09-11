#pragma once
#include <xcl2.hpp>
#include <tfhe++.hpp>

#define PC_NAME(n) n | XCL_MEM_TOPOLOGY

namespace Tangor{

void HOGEinit(const std::string binaryFile, const TFHEpp::EvalKey& ek);

template<uint16_t scaleaindex, uint16_t scalebindex, uint16_t offsetindex>
void HOGEwrap(void *buffers[], void *cl_arg){
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
OCL_CHECK(err, err = (kernel_bootstrapping).setArg(0, scaleaindex)); // scalea
OCL_CHECK(err, err = (kernel_bootstrapping).setArg(1, scalebindex)); // scaleb
OCL_CHECK(err, err = (kernel_bootstrapping).setArg(2, offsetindex)); // offset
OCL_CHECK(err, err = (kernel_bootstrapping).setArg(3, buffer_res));
OCL_CHECK(err, err = (kernel_bootstrapping).setArg(4, buffer_ina));
OCL_CHECK(err, err = (kernel_bootstrapping).setArg(5, buffer_inb));

OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_ina,buffer_inb}, 0 /* 0 means from host*/));
cl_event event;
OCL_CHECK(err, err = q.enqueueTask(kernel_bootstrapping,0,NULL,&event));
OCL_CHECK(err, err = q.enqueueMigrateMemObjects({buffer_res}, CL_MIGRATE_MEM_OBJECT_HOST));
q.finish();
for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
clReleaseEvent(event);
}
}