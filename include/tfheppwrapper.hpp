#pragma once
#include "tangorparam.hpp"
#include <starpu.h>
#include <starpu_heteroprio.h>

namespace Tangor{

extern struct starpu_codelet clHomNOT;
extern struct starpu_codelet clHomMUX;
extern struct starpu_codelet clHomNMUX;

extern TFHEpp::EvalKey ek;

template <class P, int casign, int cbsign, typename P::T offset>
void HomGateWrap(void *buffers[], void *cl_arg)
{
#ifdef USE_HOGE
if(starpu_worker_get_id()==0){
	constexpr uint16_t scaleaindex = (casign < 0)?((casign==-2)?3:2):((casign==2)?1:0);
	constexpr uint16_t scalebindex = (cbsign < 0)?((cbsign==-2)?3:2):((cbsign==2)?1:0);
	constexpr uint16_t offsetindex = (static_cast<typename std::make_signed<typename P::T>::type>(offset) < 0)?((offset==-2*P::μ)?3:2):((offset==2*P::μ)?1:0);
	HOGEwrap<scaleaindex, scalebindex, offsetindex>(buffers, cl_arg);
	return;
}
#endif
/* CPU copy of the vector pointer */
TFHEpp::TLWE<P> res,ina,inb;
for(int i = 0; i<= P::n; i++) ina[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[1]))[i];
for(int i = 0; i<= P::n; i++) inb[i] = reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[2]))[i];
TFHEpp::HomGate<P,casign,cbsign,offset>(res,ina,inb,ek);
for(int i = 0; i<= P::n; i++) reinterpret_cast<typename P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]))[i] = res[i];
}
}