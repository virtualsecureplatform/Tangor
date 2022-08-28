#pragma once
#include <tfhe++.hpp>

namespace Tangor{
#ifdef USE_M12
using P = TFHEpp::lvlMparam;
#else
using P = TFHEpp::lvl1param;
#endif
}