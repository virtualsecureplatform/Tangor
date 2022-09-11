#pragma once
#include<tfheppwrapper.hpp>
#ifdef USE_HOGE
#include<hogewrapper.hpp>
#endif

namespace Tangor{

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

}