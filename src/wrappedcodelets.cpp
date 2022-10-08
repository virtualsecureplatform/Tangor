#include <wrappedcodelets.hpp>

namespace Tangor{

constexpr int num_cputasks = 2;

void init_heteroprio(unsigned ctx){
  // CPU uses num_cputasks buckets and visits them in the natural order
  starpu_heteroprio_set_nb_prios(ctx, STARPU_CPU_IDX, num_cputasks);
  // It uses direct mapping idx => idx
  for(unsigned idx = 0; idx < num_cputasks; ++idx){
    starpu_heteroprio_set_mapping(ctx, STARPU_CPU_IDX, idx, idx);
    // If there is no CUDA worker we must tell that CPU is faster
    starpu_heteroprio_set_faster_arch(ctx, STARPU_CPU_IDX, idx);
  }
  
  #ifdef USE_HOGE
// CUDA is enabled and uses 1 buckets
starpu_heteroprio_set_nb_prios(ctx, STARPU_OPENCL_IDX, 1);
// CUDA will first look at bucket 1
starpu_heteroprio_set_mapping(ctx, STARPU_OPENCL_IDX, 0, 1);
// For bucket 1 CUDA is the fastest
starpu_heteroprio_set_faster_arch(ctx, STARPU_OPENCL_IDX, 1);
// And CPU is 3 times slower
starpu_heteroprio_set_arch_slow_factor(ctx, STARPU_CPU_IDX, 1, 3.0f);
  #endif
}

struct starpu_codelet clHomNAND =
{
    // .where = STARPU_CPU | STARPU_OPENCL,
    // .cpu_funcs = { HomGateWrap<P, -1, -1, P::μ> },
	#ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<2,2,0>},
	#endif
    // .cpu_funcs_name = { "HomNAND" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomNOR =
{
    // .where = STARPU_CPU | STARPU_OPENCL,
    // .cpu_funcs = { HomGateWrap<P, -1, -1, -P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<2,2,2>},
	#endif
    // .cpu_funcs_name = { "HomNOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomXNOR =
{
    // .where = STARPU_CPU | STARPU_OPENCL,
    // .cpu_funcs = { HomGateWrap<P, -2, -2, -2*P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<3,3,3>},
	#endif
    // .cpu_funcs_name = { "HomXNOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomAND =
{
    // .where = STARPU_CPU | STARPU_OPENCL,
    // .cpu_funcs = { HomGateWrap<P, 1, 1, -P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<0,0,2>},
	#endif
    // .cpu_funcs_name = { "HomAND" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomOR =
{
    // .where = STARPU_CPU | STARPU_OPENCL,
    // .cpu_funcs = { HomGateWrap<P, 1, 1, P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<0,0,0>},
	#endif
    // .cpu_funcs_name = { "HomOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomXOR =
{
    // .where = STARPU_CPU | STARPU_OPENCL,
    // .cpu_funcs = { HomGateWrap<P, 2, 2, 2*P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<1,1,1>},
	#endif
    // .cpu_funcs_name = { "HomXOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomANDNY =
{
    // .where = STARPU_CPU | STARPU_OPENCL,
    // .cpu_funcs = { HomGateWrap<P, -1, 1, -P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<2,0,2>},
	#endif
    // .cpu_funcs_name = { "HomANDNY" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomANDYN =
{
    // .where = STARPU_CPU | STARPU_OPENCL,
    // .cpu_funcs = { HomGateWrap<P, 1, -1, -P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<0,2,2>},
	#endif
    // .cpu_funcs_name = { "HomANDYN" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomORNY =
{
    // .where = STARPU_CPU | STARPU_OPENCL,
    // .cpu_funcs = { HomGateWrap<P, -1, 1, P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<2,0,0>},
	#endif
    // .cpu_funcs_name = { "HomORNY" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomORYN =
{
    // .where = STARPU_CPU | STARPU_OPENCL,
    // .cpu_funcs = { HomGateWrap<P, 1, -1, P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<0,2,0>},
	#endif
    // .cpu_funcs_name = { "HomORYN" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

}