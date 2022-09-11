#include <wrappedcodelets.hpp>

namespace Tangor{

struct starpu_codelet clHomNAND =
{
    .cpu_funcs = { HomGateWrap<P, -1, -1, P::μ> },
    .cpu_funcs_name = { "HomNAND" },
	#ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<2,2,0>},
	#endif
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomNOR =
{
    .cpu_funcs = { HomGateWrap<P, -1, -1, -P::μ> },
    .cpu_funcs_name = { "HomNOR" },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<2,2,2>},
	#endif
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomXNOR =
{
    .cpu_funcs = { HomGateWrap<P, -2, -2, -2*P::μ> },
    .cpu_funcs_name = { "HomXNOR" },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<3,3,3>},
	#endif
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomAND =
{
    .cpu_funcs = { HomGateWrap<P, 1, 1, -P::μ> },
    .cpu_funcs_name = { "HomAND" },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<0,0,2>},
	#endif
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomOR =
{
    .cpu_funcs = { HomGateWrap<P, 1, 1, P::μ> },
    .cpu_funcs_name = { "HomOR" },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<0,0,0>},
	#endif
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomXOR =
{
    .cpu_funcs = { HomGateWrap<P, 2, 2, 2*P::μ> },
    .cpu_funcs_name = { "HomXOR" },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<1,1,1>},
	#endif
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomANDNY =
{
    .cpu_funcs = { HomGateWrap<P, -1, 1, -P::μ> },
    .cpu_funcs_name = { "HomANDNY" },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<2,0,2>},
	#endif
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomANDYN =
{
    .cpu_funcs = { HomGateWrap<P, 1, -1, -P::μ> },
    .cpu_funcs_name = { "HomANDYN" },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<0,2,2>},
	#endif
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomORNY =
{
    .cpu_funcs = { HomGateWrap<P, -1, 1, P::μ> },
    .cpu_funcs_name = { "HomORNY" },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<2,0,0>},
	#endif
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomORYN =
{
    .cpu_funcs = { HomGateWrap<P, 1, -1, P::μ> },
    .cpu_funcs_name = { "HomORYN" },
    #ifdef USE_HOGE
	.opencl_funcs = {HOGEwrap<0,2,0>},
	#endif
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

}