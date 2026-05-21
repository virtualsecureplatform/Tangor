#include <cuda_runtime.h>
#include <starpu.h>
#include <starpu_cuda.h>

#include <include/bootstrap_gpu.cuh>
#include <include/cufhe_gpu.cuh>

#include <iostream>

namespace Tangor {
namespace {

using IksP = TFHEpp::lvl10param;
using BrP = TFHEpp::lvl01param;
using P = TFHEpp::lvl1param;

bool cufhepp_initialized = false;

int current_cuda_device()
{
    const int worker_id = starpu_worker_get_id();
    const int device_id = starpu_worker_get_devid(worker_id);
    return device_id < 0 ? 0 : device_id;
}

typename P::T *tlwe_ptr(void *buffer)
{
    return reinterpret_cast<typename P::T *>(STARPU_VECTOR_GET_PTR(buffer));
}

const typename P::T *const_tlwe_ptr(void *buffer)
{
    return reinterpret_cast<const typename P::T *>(STARPU_VECTOR_GET_PTR(buffer));
}

template <typename Bootstrap>
void hom2_wrap(void *buffers[], void *cl_arg, Bootstrap bootstrap)
{
    (void)cl_arg;
    bootstrap(tlwe_ptr(buffers[0]), const_tlwe_ptr(buffers[1]),
              const_tlwe_ptr(buffers[2]), starpu_cuda_get_local_stream(),
              current_cuda_device());
}

}  // namespace

bool CufheppInitialize(const TFHEpp::EvalKey& ek)
{
    const unsigned cuda_workers = starpu_cuda_worker_get_count();
    if (cuda_workers == 0) return false;

    int device_count = 0;
    const cudaError_t status = cudaGetDeviceCount(&device_count);
    if (status != cudaSuccess || device_count == 0) {
        std::cerr << "cuFHEpp disabled: no CUDA device is available"
                  << std::endl;
        return false;
    }

    cufhe::SetGPUNum(device_count);
    cufhe::Initialize(ek);
    cufhepp_initialized = true;
    return true;
}

void CufheppCleanUp()
{
    if (!cufhepp_initialized) return;
    cufhe::CleanUp();
    cufhepp_initialized = false;
}

void CufheppHomNANDWrap(void *buffers[], void *cl_arg)
{
    hom2_wrap(buffers, cl_arg, cufhe::NandBootstrap<IksP, BrP, P::μ>);
}

void CufheppHomNORWrap(void *buffers[], void *cl_arg)
{
    hom2_wrap(buffers, cl_arg, cufhe::NorBootstrap<IksP, BrP, P::μ>);
}

void CufheppHomXNORWrap(void *buffers[], void *cl_arg)
{
    hom2_wrap(buffers, cl_arg, cufhe::XnorBootstrap<IksP, BrP, P::μ>);
}

void CufheppHomANDWrap(void *buffers[], void *cl_arg)
{
    hom2_wrap(buffers, cl_arg, cufhe::AndBootstrap<IksP, BrP, P::μ>);
}

void CufheppHomORWrap(void *buffers[], void *cl_arg)
{
    hom2_wrap(buffers, cl_arg, cufhe::OrBootstrap<IksP, BrP, P::μ>);
}

void CufheppHomXORWrap(void *buffers[], void *cl_arg)
{
    hom2_wrap(buffers, cl_arg, cufhe::XorBootstrap<IksP, BrP, P::μ>);
}

void CufheppHomANDNYWrap(void *buffers[], void *cl_arg)
{
    hom2_wrap(buffers, cl_arg, cufhe::AndNYBootstrap<IksP, BrP, P::μ>);
}

void CufheppHomANDYNWrap(void *buffers[], void *cl_arg)
{
    hom2_wrap(buffers, cl_arg, cufhe::AndYNBootstrap<IksP, BrP, P::μ>);
}

void CufheppHomORNYWrap(void *buffers[], void *cl_arg)
{
    hom2_wrap(buffers, cl_arg, cufhe::OrNYBootstrap<IksP, BrP, P::μ>);
}

void CufheppHomORYNWrap(void *buffers[], void *cl_arg)
{
    hom2_wrap(buffers, cl_arg, cufhe::OrYNBootstrap<IksP, BrP, P::μ>);
}

void CufheppHomNOTWrap(void *buffers[], void *cl_arg)
{
    (void)cl_arg;
    cufhe::NotBootstrap<P>(tlwe_ptr(buffers[0]), const_tlwe_ptr(buffers[1]),
                           starpu_cuda_get_local_stream(),
                           current_cuda_device());
}

void CufheppHomMUXWrap(void *buffers[], void *cl_arg)
{
    (void)cl_arg;
    cufhe::MuxBootstrap<IksP, BrP, P::μ>(
        tlwe_ptr(buffers[0]), const_tlwe_ptr(buffers[1]),
        const_tlwe_ptr(buffers[2]), const_tlwe_ptr(buffers[3]),
        starpu_cuda_get_local_stream(), current_cuda_device());
}

void CufheppHomNMUXWrap(void *buffers[], void *cl_arg)
{
    (void)cl_arg;
    cufhe::NMuxBootstrap<IksP, BrP, P::μ>(
        tlwe_ptr(buffers[0]), const_tlwe_ptr(buffers[1]),
        const_tlwe_ptr(buffers[2]), const_tlwe_ptr(buffers[3]),
        starpu_cuda_get_local_stream(), current_cuda_device());
}

}  // namespace Tangor
