#pragma once
#include "tangorparam.hpp"
#include <starpu.h>

namespace Tangor{

extern struct starpu_codelet clHomNOT;
extern struct starpu_codelet clHomMUX;
extern struct starpu_codelet clHomNMUX;

extern TFHEpp::EvalKey ek;

template <class P, int casign, int cbsign, typename P::T offset>
void HomGateWrap(void *buffers[], void *cl_arg)
{
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,ina,inb;
for(int i = 0; i<= P::n; i++) ina[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
for(int i = 0; i<= P::n; i++) inb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[2]))[i];
TFHEpp::HomGate<P,casign,cbsign,offset>(res,ina,inb,ek);
for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
}
}