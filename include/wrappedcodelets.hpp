#pragma once
#ifdef USE_HOGE
#include<hogewrapper.hpp>
#endif
#include "tangorparam.hpp"
#include <starpu.h>

namespace Tangor{

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

extern struct starpu_codelet clHomNAND;
extern struct starpu_codelet clHomNOR;
extern struct starpu_codelet clHomXNOR;
extern struct starpu_codelet clHomAND;
extern struct starpu_codelet clHomOR;
extern struct starpu_codelet clHomXOR;
extern struct starpu_codelet clHomANDNY;
extern struct starpu_codelet clHomANDYN;
extern struct starpu_codelet clHomORNY;
extern struct starpu_codelet clHomORYN;

extern struct starpu_codelet clHomNOT;

extern struct starpu_codelet clHomMUX;
extern struct starpu_codelet clHomNMUX;
}