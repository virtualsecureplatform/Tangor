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
extern TFHEpp::TLWE<TFHEpp::lvl1param> buftlwea,buftlweb,bufres;
extern std::unique_ptr<cl::Buffer> buffer_res,buffer_ina,buffer_inb;
extern std::array<cl_mem_ext_ptr_t, iksknumbus> ikskBufs;
extern std::array<cl_mem_ext_ptr_t, bknumbus> bknttBufs;

extern std::unique_ptr<cl::Context> context;
extern std::unique_ptr<cl::CommandQueue> q;
extern std::unique_ptr<cl::Kernel> kernel;

extern std::array<std::shared_ptr<cl::Buffer>, bknumbus> buffer_bkntts;
extern std::array<std::shared_ptr<cl::Buffer>, iksknumbus> buffer_iksks;

template<uint16_t scaleaindex, uint16_t scalebindex, uint16_t offsetindex>
void HOGEwrap(void *buffers[], void *cl_arg){
    cl_int err;

    using P = TFHEpp::lvl1param;
    for(int i = 0; i<= P::n; i++) buftlwea[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
    for(int i = 0; i<= P::n; i++) buftlweb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[2]))[i];

    // Setting the kernel Arguments
    OCL_CHECK(err, err = (*kernel).setArg(0, scaleaindex)); // scalea
    OCL_CHECK(err, err = (*kernel).setArg(1, scalebindex)); // scaleb
    OCL_CHECK(err, err = (*kernel).setArg(2, offsetindex)); // offset

    OCL_CHECK(err, err = q->enqueueMigrateMemObjects({*buffer_ina,*buffer_inb}, 0 /* 0 means from host*/));
    OCL_CHECK(err, err = q->enqueueTask(*kernel));
    OCL_CHECK(err, err = q->enqueueMigrateMemObjects({*buffer_res}, CL_MIGRATE_MEM_OBJECT_HOST));
    q->finish();
    for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = bufres[i];
}

template<bool negate>
void HOGEMUXwrap(void *buffers[], void *cl_arg){
    cl_int err;

    using P = TFHEpp::lvl1param;
    for(int i = 0; i<= P::n; i++) buftlwea[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i]; //s
    for(int i = 0; i<= P::n; i++) buftlweb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[2]))[i]; //c1

    // Setting the kernel Arguments
    OCL_CHECK(err, err = (*kernel).setArg(0, static_cast<uint16_t>(0))); // scalea
    OCL_CHECK(err, err = (*kernel).setArg(1, static_cast<uint16_t>(0))); // scaleb
    OCL_CHECK(err, err = (*kernel).setArg(2,  static_cast<uint16_t>(2))); // offset

    OCL_CHECK(err, err = q->enqueueMigrateMemObjects({*buffer_ina,*buffer_inb}, 0 /* 0 means from host*/));
    OCL_CHECK(err, err = q->enqueueTask(*kernel));
    OCL_CHECK(err, err = q->enqueueMigrateMemObjects({*buffer_res}, CL_MIGRATE_MEM_OBJECT_HOST));
    q->finish();
    for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = bufres[i];
    for(int i = 0; i<= P::n; i++) buftlweb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[3]))[i]; //c0
    OCL_CHECK(err, err = (*kernel).setArg(0,  static_cast<uint16_t>(2))); // scalea
    OCL_CHECK(err, err = q->enqueueMigrateMemObjects({*buffer_inb}, 0 /* 0 means from host*/));
    OCL_CHECK(err, err = q->enqueueTask(*kernel));
    OCL_CHECK(err, err = q->enqueueMigrateMemObjects({*buffer_res}, CL_MIGRATE_MEM_OBJECT_HOST));
    q->finish();
    if constexpr(negate)
    for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = -(reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] - bufres[i]);
    else
    for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] += bufres[i];
    reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[P::n] += negate?-P::μ:P::μ;
}

}
