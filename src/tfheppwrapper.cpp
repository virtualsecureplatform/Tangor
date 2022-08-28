#include <tfheppwrapper.hpp>

namespace Tangor{

void HomNOTWrap(void *buffers[], void *cl_arg)
{
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,ina;
for(int i = 0; i<= P::n; i++) ina[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
TFHEpp::HomNOT<P>(res, ina);
for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
}

struct starpu_codelet clHomNOT =
{
    .cpu_funcs = { HomNOTWrap },
    .cpu_funcs_name = { "HomNOT" },
    .nbuffers = 2,
    .modes = { STARPU_W, STARPU_R}
};

template <class P, int casign, int cbsign, typename P::T offset>
void HomGateWrap(void *buffers[], void *cl_arg)
{
const TFHEpp::EvalKey* const ek = static_cast<TFHEpp::EvalKey*>(cl_arg);
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,ina,inb;
for(int i = 0; i<= P::n; i++) ina[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
for(int i = 0; i<= P::n; i++) inb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[2]))[i];
TFHEpp::HomGate<P,casign,cbsign,offset>(res,ina,inb,*ek);
for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
}

struct starpu_codelet clHomNAND =
{
    .cpu_funcs = { HomGateWrap<P, -1, -1, P::μ> },
    .cpu_funcs_name = { "HomNAND" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomNOR =
{
    .cpu_funcs = { HomGateWrap<P, -1, -1, -P::μ> },
    .cpu_funcs_name = { "HomNOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomXNOR =
{
    .cpu_funcs = { HomGateWrap<P, -2, -2, -2*P::μ> },
    .cpu_funcs_name = { "HomXNOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomAND =
{
    .cpu_funcs = { HomGateWrap<P, 1, 1, -P::μ> },
    .cpu_funcs_name = { "HomAND" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomOR =
{
    .cpu_funcs = { HomGateWrap<P, 1, 1, P::μ> },
    .cpu_funcs_name = { "HomOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomXOR =
{
    .cpu_funcs = { HomGateWrap<P, 2, 2, 2*P::μ> },
    .cpu_funcs_name = { "HomXOR" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomANDNY =
{
    .cpu_funcs = { HomGateWrap<P, -1, 1, -P::μ> },
    .cpu_funcs_name = { "HomANDNY" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomANDYN =
{
    .cpu_funcs = { HomGateWrap<P, 1, -1, -P::μ> },
    .cpu_funcs_name = { "HomANDYN" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomORNY =
{
    .cpu_funcs = { HomGateWrap<P, -1, 1, P::μ> },
    .cpu_funcs_name = { "HomORNY" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

struct starpu_codelet clHomORYN =
{
    .cpu_funcs = { HomGateWrap<P, 1, -1, P::μ> },
    .cpu_funcs_name = { "HomORYN" },
    .nbuffers = 3,
    .modes = { STARPU_W, STARPU_R, STARPU_R }
};

void HomMUXWrap(void *buffers[], void *cl_arg)
{
const TFHEpp::EvalKey* const ek = static_cast<TFHEpp::EvalKey*>(cl_arg);
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,ins,ina,inb;
for(int i = 0; i<= P::n; i++) ins[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
for(int i = 0; i<= P::n; i++) ina[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[2]))[i];
for(int i = 0; i<= P::n; i++) inb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[3]))[i];
TFHEpp::HomMUX<P>(res,ins,ina,inb,*ek);
for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
}

struct starpu_codelet clHomMUX =
{
    .cpu_funcs = { HomMUXWrap },
    .cpu_funcs_name = { "HomMUX" },
    .nbuffers = 4,
    .modes = { STARPU_W, STARPU_R, STARPU_R, STARPU_R }
};

void HomNMUXWrap(void *buffers[], void *cl_arg)
{
const TFHEpp::EvalKey* const ek = static_cast<TFHEpp::EvalKey*>(cl_arg);
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,ins,ina,inb;
for(int i = 0; i<= P::n; i++) ins[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
for(int i = 0; i<= P::n; i++) ina[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[2]))[i];
for(int i = 0; i<= P::n; i++) inb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[3]))[i];
TFHEpp::HomNMUX<P>(res,ins,ina,inb,*ek);
for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
}

struct starpu_codelet clHomNMUX =
{
    .cpu_funcs = { HomNMUXWrap },
    .cpu_funcs_name = { "HomNMUX" },
    .nbuffers = 4,
    .modes = { STARPU_W, STARPU_R, STARPU_R, STARPU_R }
};

}