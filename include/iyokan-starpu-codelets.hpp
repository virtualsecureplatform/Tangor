#pragma once

#include <memory>

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

// A non-blocking StarPU submission used by the KVSP task frontend.  The
// frontend polls this object before propagating a circuit value, at which
// point synchronizeOutput() makes the result available to ordinary CPU-only
// RAM/ROM tasks as well.
class IyokanStarpuTask {
public:
    struct State;

    explicit IyokanStarpuTask(std::shared_ptr<State> state);
    bool isFinished() const;
    void synchronizeOutput() const;

private:
    std::shared_ptr<State> state_;

    friend void beginIyokanStarpuCapture();
    friend std::shared_ptr<IyokanStarpuTask> endIyokanStarpuCapture();
    friend void runIyokanStarpuBinaryGate(IyokanBinaryGate, IyokanTLWE&,
                                          const IyokanTLWE&,
                                          const IyokanTLWE&,
                                          const TFHEpp::EvalKey&);
    friend void runIyokanStarpuNot(IyokanTLWE&, const IyokanTLWE&);
    friend void runIyokanStarpuMux(IyokanTLWE&, const IyokanTLWE&,
                                   const IyokanTLWE&, const IyokanTLWE&,
                                   const TFHEpp::EvalKey&);
};

// These delimit the synchronous Iyokan task body.  When active, ordinary
// gate calls enqueue one StarPU task and return immediately; outside this
// scope the public gate API retains its synchronous behaviour.
void beginIyokanStarpuCapture();
std::shared_ptr<IyokanStarpuTask> endIyokanStarpuCapture();

// Tell the persistent-handle registry that a CPU-only Iyokan task has
// overwritten a level-0 value.
void markIyokanTLWEHostWrite(IyokanTLWE& value);

#ifdef USE_CUFHEPP
// cuFHEpp support for the level-0 gate representation used by Iyokan/KVSP.
// These are separate from Tangor's legacy level-1 CUDA wrappers.
bool initializeIyokanCufhepp(const TFHEpp::EvalKey& evalKey);
void cleanupIyokanCufhepp();

void iyokanCufheppHomAND(void* buffers[], void* clArg);
void iyokanCufheppHomNAND(void* buffers[], void* clArg);
void iyokanCufheppHomANDNOT(void* buffers[], void* clArg);
void iyokanCufheppHomOR(void* buffers[], void* clArg);
void iyokanCufheppHomNOR(void* buffers[], void* clArg);
void iyokanCufheppHomORNOT(void* buffers[], void* clArg);
void iyokanCufheppHomXOR(void* buffers[], void* clArg);
void iyokanCufheppHomXNOR(void* buffers[], void* clArg);
void iyokanCufheppHomMUX(void* buffers[], void* clArg);
#endif

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
