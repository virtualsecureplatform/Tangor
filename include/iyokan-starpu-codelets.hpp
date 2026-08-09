#pragma once

#include <tfhe++.hpp>

namespace Tangor {

// The KVSP/Iyokan frontend represents every ordinary circuit wire as a
// level-0 TLWE. Tangor's original experiment instead used level-1 wires, so
// keep this API separate from the legacy codelets.
using IyokanTLWE = TFHEpp::TLWE<TFHEpp::lvl0param>;
using IyokanTRLWE = TFHEpp::TRLWE<TFHEpp::lvl1param>;
using IyokanTRGSWFFT = TFHEpp::TRGSWFFT<TFHEpp::lvl1param>;

enum class IyokanBinaryGate {
    AND,
    NAND,
    ANDNOT,
    OR,
    NOR,
    ORNOT,
    XOR,
    XNOR,
};

void runIyokanStarpuBinaryGate(IyokanBinaryGate gate, IyokanTLWE& output,
                               const IyokanTLWE& left,
                               const IyokanTLWE& right,
                               const TFHEpp::EvalKey& evalKey);
void runIyokanStarpuNot(IyokanTLWE& output, const IyokanTLWE& input);
void runIyokanStarpuMux(IyokanTLWE& output, const IyokanTLWE& select,
                        const IyokanTLWE& whenTrue,
                        const IyokanTLWE& whenFalse,
                        const TFHEpp::EvalKey& evalKey);

// RAM and ROM networks select packed TRLWE words with CMUXFFT.  These are
// separate from the level-0 gate path above because their operands live at
// TFHEpp level 1.
void runIyokanStarpuCMUXFFT(IyokanTRLWE& output,
                            const IyokanTRGSWFFT& select,
                            const IyokanTRLWE& whenTrue,
                            const IyokanTRLWE& whenFalse);
void runIyokanStarpuMuxWoSE(IyokanTRLWE& output, const IyokanTLWE& select,
                            const IyokanTLWE& whenTrue,
                            const IyokanTLWE& whenFalse,
                            const TFHEpp::EvalKey& evalKey);

}  // namespace Tangor
