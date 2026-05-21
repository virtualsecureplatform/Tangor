#include <wrappedcodelets.hpp>

namespace Tangor{

TFHEpp::EvalKey ek;

#ifdef USE_CUFHEPP
#define TANGOR_CODELET_WHERE (STARPU_CPU | STARPU_CUDA)
#define TANGOR_CUDA_IMPL(func) \
    .cuda_funcs = { func }, \
    .cuda_flags = { STARPU_CUDA_ASYNC },
#else
#define TANGOR_CODELET_WHERE STARPU_CPU
#define TANGOR_CUDA_IMPL(func)
#endif

static struct starpu_perfmodel homgate_perf_model =
{
    .type = STARPU_HISTORY_BASED,
    .symbol = "HomGate_perf_model"
};

struct starpu_codelet clHomNAND =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = {  HomGateWrap<P, -1, -1, P::μ>
                },
    TANGOR_CUDA_IMPL(CufheppHomNANDWrap)
    .cpu_funcs_name = { "HomNAND" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomNOR =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = {  HomGateWrap<P, -1, -1, -P::μ>
    },
    TANGOR_CUDA_IMPL(CufheppHomNORWrap)
    .cpu_funcs_name = { "HomNOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomXNOR =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = {  HomGateWrap<P, -2, -2, -2*P::μ>
     },
    TANGOR_CUDA_IMPL(CufheppHomXNORWrap)
    .cpu_funcs_name = { "HomXNOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomAND =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = {  HomGateWrap<P, 1, 1, -P::μ>
    },
    TANGOR_CUDA_IMPL(CufheppHomANDWrap)
    .cpu_funcs_name = { "HomAND" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomOR =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = {  HomGateWrap<P, 1, 1, P::μ>
    },
    TANGOR_CUDA_IMPL(CufheppHomORWrap)
    .cpu_funcs_name = { "HomOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomXOR =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = {  HomGateWrap<P, 2, 2, 2*P::μ>
     },
    TANGOR_CUDA_IMPL(CufheppHomXORWrap)
    .cpu_funcs_name = { "HomXOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomANDNY =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = {  HomGateWrap<P, -1, 1, -P::μ>
    },
    TANGOR_CUDA_IMPL(CufheppHomANDNYWrap)
    .cpu_funcs_name = { "HomANDNY" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomANDYN =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = {  HomGateWrap<P, 1, -1, -P::μ>
                },
    TANGOR_CUDA_IMPL(CufheppHomANDYNWrap)
    .cpu_funcs_name = { "HomANDYN" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomORNY =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = {  HomGateWrap<P, -1, 1, P::μ>
                },
    TANGOR_CUDA_IMPL(CufheppHomORNYWrap)
    .cpu_funcs_name = { "HomORNY" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

struct starpu_codelet clHomORYN =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = {  HomGateWrap<P, 1, -1, P::μ>},
    TANGOR_CUDA_IMPL(CufheppHomORYNWrap)
    .cpu_funcs_name = { "HomORYN" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R },
    .model = &homgate_perf_model
};

void HomNOTWrap(void *buffers[], void *cl_arg)
{
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,ina;
for(int i = 0; i<= P::k * P::n; i++) ina[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
TFHEpp::HomNOT<P>(res, ina);
for(int i = 0; i<= P::k * P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
}

static struct starpu_perfmodel homnot_perf_model =
{
    .type = STARPU_HISTORY_BASED,
    .symbol = "HomNOT_perf_model"
};

struct starpu_codelet clHomNOT =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = { HomNOTWrap },
    TANGOR_CUDA_IMPL(CufheppHomNOTWrap)
    .cpu_funcs_name = { "HomNOT" },
    .nbuffers = 2,
    .modes = { STARPU_W, STARPU_R},
    .model = &homnot_perf_model
};

void HomMUXWrap(void *buffers[], void *cl_arg)
{
#ifdef USE_HOGE
if(starpu_worker_get_id()==0){
	HOGEMUXwrap<false>(buffers, cl_arg);
	return;
}
#endif
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,ins,ina,inb;
for(int i = 0; i<= P::k * P::n; i++) ins[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
for(int i = 0; i<= P::k * P::n; i++) ina[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[2]))[i];
for(int i = 0; i<= P::k * P::n; i++) inb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[3]))[i];
TFHEpp::HomMUX<P>(res,ins,ina,inb,ek);
for(int i = 0; i<= P::k * P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
}

static struct starpu_perfmodel hommux_perf_model =
{
    .type = STARPU_HISTORY_BASED,
    .symbol = "HomMUX_perf_model"
};

struct starpu_codelet clHomMUX =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = { HomMUXWrap },
    TANGOR_CUDA_IMPL(CufheppHomMUXWrap)
    .cpu_funcs_name = { "HomMUX" },
    .nbuffers = 4,
    .modes = { STARPU_W, STARPU_R, STARPU_R, STARPU_R },
    .model = &hommux_perf_model
};

void HomNMUXWrap(void *buffers[], void *cl_arg)
{
#ifdef USE_HOGE
if(starpu_worker_get_id()==0){
	HOGEMUXwrap<true>(buffers, cl_arg);
	return;
}
#endif
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,ins,ina,inb;
for(int i = 0; i<= P::k * P::n; i++) ins[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
for(int i = 0; i<= P::k * P::n; i++) ina[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[2]))[i];
for(int i = 0; i<= P::k * P::n; i++) inb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[3]))[i];
TFHEpp::HomNMUX<P>(res,ins,ina,inb,ek);
for(int i = 0; i<= P::k * P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
}

struct starpu_codelet clHomNMUX =
{
    .where = TANGOR_CODELET_WHERE,
    .cpu_funcs = { HomNMUXWrap },
    TANGOR_CUDA_IMPL(CufheppHomNMUXWrap)
    .cpu_funcs_name = { "HomNMUX" },
    .nbuffers = 4,
    .modes = { STARPU_W, STARPU_R, STARPU_R, STARPU_R },
    .model = &hommux_perf_model
};

#undef TANGOR_CUDA_IMPL
#undef TANGOR_CODELET_WHERE

}
