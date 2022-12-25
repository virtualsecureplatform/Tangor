#include <tfheppwrapper.hpp>

namespace Tangor{

TFHEpp::EvalKey ek;

void HomNOTWrap(void *buffers[], void *cl_arg)
{
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,ina;
for(int i = 0; i<= P::n; i++) ina[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
TFHEpp::HomNOT<P>(res, ina);
for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
}

static struct starpu_perfmodel homnot_perf_model =
{
    .type = STARPU_HISTORY_BASED,
    .symbol = "HomNOT_perf_model"
};

struct starpu_codelet clHomNOT =
{
    .where = STARPU_CPU,
    .cpu_funcs = { HomNOTWrap },
    .cpu_funcs_name = { "HomNOT" },
    .nbuffers = 2,
    .modes = { STARPU_W, STARPU_R},
    .model = &homnot_perf_model
};

void HomMUXWrap(void *buffers[], void *cl_arg)
{
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,ins,ina,inb;
for(int i = 0; i<= P::n; i++) ins[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
for(int i = 0; i<= P::n; i++) ina[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[2]))[i];
for(int i = 0; i<= P::n; i++) inb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[3]))[i];
TFHEpp::HomMUX<P>(res,ins,ina,inb,ek);
for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
}

static struct starpu_perfmodel hommux_perf_model =
{
    .type = STARPU_HISTORY_BASED,
    .symbol = "HomMUX_perf_model"
};

struct starpu_codelet clHomMUX =
{
    .where = STARPU_CPU,
    .cpu_funcs = { HomMUXWrap },
    .cpu_funcs_name = { "HomMUX" },
    .nbuffers = 4,
    .modes = { STARPU_W, STARPU_R, STARPU_R, STARPU_R },
    .model = &hommux_perf_model
};

void HomNMUXWrap(void *buffers[], void *cl_arg)
{
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,ins,ina,inb;
for(int i = 0; i<= P::n; i++) ins[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
for(int i = 0; i<= P::n; i++) ina[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[2]))[i];
for(int i = 0; i<= P::n; i++) inb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[3]))[i];
TFHEpp::HomNMUX<P>(res,ins,ina,inb,ek);
for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
}

struct starpu_codelet clHomNMUX =
{
    .where = STARPU_CPU,
    .cpu_funcs = { HomNMUXWrap },
    .cpu_funcs_name = { "HomNMUX" },
    .nbuffers = 4,
    .modes = { STARPU_W, STARPU_R, STARPU_R, STARPU_R },
    .model = &hommux_perf_model
};

}