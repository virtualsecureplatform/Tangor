#include <wrappedcodelets.hpp>

namespace Tangor{

struct starpu_codelet clHomNAND =
{
    .cpu_funcs = { HomGateWrap<P, -1, -1, P::μ> },
	#ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<2,2,0>},
	#endif
    .cpu_funcs_name = { "HomNAND" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomNOR =
{
    .cpu_funcs = { HomGateWrap<P, -1, -1, -P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<2,2,2>},
	#endif
    .cpu_funcs_name = { "HomNOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomXNOR =
{
    .cpu_funcs = { HomGateWrap<P, -2, -2, -2*P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<3,3,3>},
	#endif
    .cpu_funcs_name = { "HomXNOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomAND =
{
    .cpu_funcs = { HomGateWrap<P, 1, 1, -P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<0,0,2>},
	#endif
    .cpu_funcs_name = { "HomAND" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomOR =
{
    .cpu_funcs = { HomGateWrap<P, 1, 1, P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<0,0,0>},
	#endif
    .cpu_funcs_name = { "HomOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomXOR =
{
    .cpu_funcs = { HomGateWrap<P, 2, 2, 2*P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<1,1,1>},
	#endif
    .cpu_funcs_name = { "HomXOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomANDNY =
{
    .cpu_funcs = { HomGateWrap<P, -1, 1, -P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<2,0,2>},
	#endif
    .cpu_funcs_name = { "HomANDNY" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomANDYN =
{
    .cpu_funcs = { HomGateWrap<P, 1, -1, -P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<0,2,2>},
	#endif
    .cpu_funcs_name = { "HomANDYN" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomORNY =
{
    .cpu_funcs = { HomGateWrap<P, -1, 1, P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<2,0,0>},
	#endif
    .cpu_funcs_name = { "HomORNY" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomORYN =
{
    .cpu_funcs = { HomGateWrap<P, 1, -1, P::μ> },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<0,2,0>},
	#endif
    .cpu_funcs_name = { "HomORYN" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

}