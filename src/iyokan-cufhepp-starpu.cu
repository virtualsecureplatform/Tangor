#include "int128_make_signed_fix.hpp"
#include "iyokan-starpu-codelets.hpp"

#include <cuda_runtime.h>
#include <starpu.h>
#include <starpu_cuda.h>

#include <include/cufhe_gpu.cuh>

#include <cstdlib>
#include <iostream>

namespace Tangor {
namespace {

using IksP = TFHEpp::lvl10param;
using BrP = TFHEpp::lvl01param;
using GateP = TFHEpp::lvl0param;

bool cufheppInitialized = false;

int currentCudaDevice()
{
    const int workerId = starpu_worker_get_id();
    const int deviceId = starpu_worker_get_devid(workerId);
    return deviceId < 0 ? 0 : deviceId;
}

GateP::T* gatePointer(void* buffer)
{
    return reinterpret_cast<GateP::T*>(STARPU_VECTOR_GET_PTR(buffer));
}

template <auto Bootstrap>
void runBinaryBootstrap(void* buffers[], void*)
{
    Bootstrap(gatePointer(buffers[0]), gatePointer(buffers[1]),
              gatePointer(buffers[2]), starpu_cuda_get_local_stream(),
              currentCudaDevice());
}

}  // namespace

bool initializeIyokanCufhepp(const TFHEpp::EvalKey& evalKey)
{
    if (cufheppInitialized) return true;

    const unsigned cudaWorkers = starpu_cuda_worker_get_count();
    if (cudaWorkers == 0) return false;

    int deviceCount = 0;
    const cudaError_t status = cudaGetDeviceCount(&deviceCount);
    if (status != cudaSuccess || deviceCount == 0) {
        std::cerr << "Tangor KVSP CUDA offload disabled: no CUDA device is "
                     "available"
                  << std::endl;
        return false;
    }

    cufhe::SetGPUNum(deviceCount);
    cufhe::Initialize(evalKey);
    cufheppInitialized = true;
    std::atexit(cleanupIyokanCufhepp);
    return true;
}

void cleanupIyokanCufhepp()
{
    if (!cufheppInitialized) return;
    cufhe::CleanUp();
    cufheppInitialized = false;
}

void iyokanCufheppHomAND(void* buffers[], void* clArg)
{
    runBinaryBootstrap<
        cufhe::AndBootstrap<BrP, TFHEpp::lvl1param::μ, IksP>>(buffers, clArg);
}

void iyokanCufheppHomNAND(void* buffers[], void* clArg)
{
    runBinaryBootstrap<
        cufhe::NandBootstrap<BrP, TFHEpp::lvl1param::μ, IksP>>(buffers, clArg);
}

void iyokanCufheppHomANDNOT(void* buffers[], void* clArg)
{
    runBinaryBootstrap<
        cufhe::AndYNBootstrap<BrP, TFHEpp::lvl1param::μ, IksP>>(buffers, clArg);
}

void iyokanCufheppHomOR(void* buffers[], void* clArg)
{
    runBinaryBootstrap<
        cufhe::OrBootstrap<BrP, TFHEpp::lvl1param::μ, IksP>>(buffers, clArg);
}

void iyokanCufheppHomNOR(void* buffers[], void* clArg)
{
    runBinaryBootstrap<
        cufhe::NorBootstrap<BrP, TFHEpp::lvl1param::μ, IksP>>(buffers, clArg);
}

void iyokanCufheppHomORNOT(void* buffers[], void* clArg)
{
    runBinaryBootstrap<
        cufhe::OrYNBootstrap<BrP, TFHEpp::lvl1param::μ, IksP>>(buffers, clArg);
}

void iyokanCufheppHomXOR(void* buffers[], void* clArg)
{
    runBinaryBootstrap<
        cufhe::XorBootstrap<BrP, TFHEpp::lvl1param::μ, IksP>>(buffers, clArg);
}

void iyokanCufheppHomXNOR(void* buffers[], void* clArg)
{
    runBinaryBootstrap<
        cufhe::XnorBootstrap<BrP, TFHEpp::lvl1param::μ, IksP>>(buffers, clArg);
}

void iyokanCufheppHomMUX(void* buffers[], void*)
{
    cufhe::MuxBootstrap<BrP, TFHEpp::lvl1param::μ, IksP>(
        gatePointer(buffers[0]), gatePointer(buffers[1]),
        gatePointer(buffers[2]), gatePointer(buffers[3]),
        starpu_cuda_get_local_stream(), currentCudaDevice());
}

void iyokanCufheppCMUXFFT(void* buffers[], void*)
{
    auto* output = reinterpret_cast<TFHEpp::lvl1param::T*>(
        STARPU_VARIABLE_GET_PTR(buffers[0]));
    const auto* select = reinterpret_cast<const double*>(
        STARPU_VARIABLE_GET_PTR(buffers[1]));
    auto* whenTrue = reinterpret_cast<TFHEpp::lvl1param::T*>(
        STARPU_VARIABLE_GET_PTR(buffers[2]));
    auto* whenFalse = reinterpret_cast<TFHEpp::lvl1param::T*>(
        STARPU_VARIABLE_GET_PTR(buffers[3]));
    cufhe::CMUXTFHEppFFTkernel(
        output, select, whenTrue, whenFalse, starpu_cuda_get_local_stream(),
        currentCudaDevice());
}

void iyokanCufheppCMUXFFTInPlaceTrue(void* buffers[], void*)
{
    auto* output = reinterpret_cast<TFHEpp::lvl1param::T*>(
        STARPU_VARIABLE_GET_PTR(buffers[0]));
    const auto* select = reinterpret_cast<const double*>(
        STARPU_VARIABLE_GET_PTR(buffers[1]));
    auto* whenFalse = reinterpret_cast<TFHEpp::lvl1param::T*>(
        STARPU_VARIABLE_GET_PTR(buffers[2]));
    cufhe::CMUXTFHEppFFTkernel(
        output, select, output, whenFalse, starpu_cuda_get_local_stream(),
        currentCudaDevice());
}

void iyokanCufheppCMUXFFTInPlaceFalse(void* buffers[], void*)
{
    auto* output = reinterpret_cast<TFHEpp::lvl1param::T*>(
        STARPU_VARIABLE_GET_PTR(buffers[0]));
    const auto* select = reinterpret_cast<const double*>(
        STARPU_VARIABLE_GET_PTR(buffers[1]));
    auto* whenTrue = reinterpret_cast<TFHEpp::lvl1param::T*>(
        STARPU_VARIABLE_GET_PTR(buffers[2]));
    cufhe::CMUXTFHEppFFTkernel(
        output, select, whenTrue, output, starpu_cuda_get_local_stream(),
        currentCudaDevice());
}

}  // namespace Tangor
