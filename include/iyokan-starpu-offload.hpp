#pragma once

// This header is force-included only for Tangor's KVSP-compatible evaluator.
// Iyokan must install this compatibility shim before TFHEpp instantiates the
// lvl3 __int128 types.
#include "int128_make_signed_fix.hpp"

// Include TFHEpp before defining the substitution macros so TFHEpp's own gate
// declarations retain their original names.
#include <tfhe++.hpp>

#include <type_traits>

#include "iyokan-starpu-codelets.hpp"

// Iyokan's ordinary TaskAsync wrapper waits for the entire gate call inside
// one host thread.  The Tangor compatibility target replaces it with a
// StarPU-aware variant defined in iyokan_tfhepp.hpp.
#define TANGOR_KVSP_STARPU_ASYNC 1

namespace TFHEpp {

template <class brP, typename brP::targetP::T μ, class iksP>
inline void TangorStarpuHomAND(TLWE<typename iksP::targetP>& output,
                               const TLWE<typename brP::domainP>& left,
                               const TLWE<typename brP::domainP>& right,
                               const EvalKey& evalKey)
{
    static_assert(std::is_same_v<brP, lvl01param> &&
                      std::is_same_v<iksP, lvl10param>,
                  "Tangor's KVSP StarPU backend supports Iyokan's lvl0 gate "
                  "configuration only");
    Tangor::runIyokanStarpuBinaryGate(Tangor::IyokanBinaryGate::AND, output,
                                      left, right, evalKey);
}

template <class brP, typename brP::targetP::T μ, class iksP>
inline void TangorStarpuHomNAND(TLWE<typename iksP::targetP>& output,
                                const TLWE<typename brP::domainP>& left,
                                const TLWE<typename brP::domainP>& right,
                                const EvalKey& evalKey)
{
    Tangor::runIyokanStarpuBinaryGate(Tangor::IyokanBinaryGate::NAND, output,
                                      left, right, evalKey);
}

template <class brP, typename brP::targetP::T μ, class iksP>
inline void TangorStarpuHomANDYN(TLWE<typename iksP::targetP>& output,
                                 const TLWE<typename brP::domainP>& left,
                                 const TLWE<typename brP::domainP>& right,
                                 const EvalKey& evalKey)
{
    Tangor::runIyokanStarpuBinaryGate(Tangor::IyokanBinaryGate::ANDNOT,
                                      output, left, right, evalKey);
}

template <class brP, typename brP::targetP::T μ, class iksP>
inline void TangorStarpuHomOR(TLWE<typename iksP::targetP>& output,
                              const TLWE<typename brP::domainP>& left,
                              const TLWE<typename brP::domainP>& right,
                              const EvalKey& evalKey)
{
    Tangor::runIyokanStarpuBinaryGate(Tangor::IyokanBinaryGate::OR, output,
                                      left, right, evalKey);
}

template <class brP, typename brP::targetP::T μ, class iksP>
inline void TangorStarpuHomNOR(TLWE<typename iksP::targetP>& output,
                               const TLWE<typename brP::domainP>& left,
                               const TLWE<typename brP::domainP>& right,
                               const EvalKey& evalKey)
{
    Tangor::runIyokanStarpuBinaryGate(Tangor::IyokanBinaryGate::NOR, output,
                                      left, right, evalKey);
}

template <class brP, typename brP::targetP::T μ, class iksP>
inline void TangorStarpuHomORYN(TLWE<typename iksP::targetP>& output,
                                const TLWE<typename brP::domainP>& left,
                                const TLWE<typename brP::domainP>& right,
                                const EvalKey& evalKey)
{
    Tangor::runIyokanStarpuBinaryGate(Tangor::IyokanBinaryGate::ORNOT,
                                      output, left, right, evalKey);
}

template <class brP, typename brP::targetP::T μ, class iksP>
inline void TangorStarpuHomXOR(TLWE<typename iksP::targetP>& output,
                               const TLWE<typename brP::domainP>& left,
                               const TLWE<typename brP::domainP>& right,
                               const EvalKey& evalKey)
{
    Tangor::runIyokanStarpuBinaryGate(Tangor::IyokanBinaryGate::XOR, output,
                                      left, right, evalKey);
}

template <class brP, typename brP::targetP::T μ, class iksP>
inline void TangorStarpuHomXNOR(TLWE<typename iksP::targetP>& output,
                                const TLWE<typename brP::domainP>& left,
                                const TLWE<typename brP::domainP>& right,
                                const EvalKey& evalKey)
{
    Tangor::runIyokanStarpuBinaryGate(Tangor::IyokanBinaryGate::XNOR, output,
                                      left, right, evalKey);
}

template <class P>
inline void TangorStarpuHomNOT(TLWE<P>& output, const TLWE<P>& input)
{
    static_assert(std::is_same_v<P, lvl0param>,
                  "Tangor's KVSP StarPU backend supports lvl0 NOT only");
    Tangor::runIyokanStarpuNot(output, input);
}

template <class P>
inline void TangorStarpuHomMUX(TLWE<P>& output, const TLWE<P>& select,
                               const TLWE<P>& whenTrue,
                               const TLWE<P>& whenFalse,
                               const EvalKey& evalKey)
{
    static_assert(std::is_same_v<P, lvl0param>,
                  "Tangor's KVSP StarPU backend supports lvl0 MUX only");
    Tangor::runIyokanStarpuMux(output, select, whenTrue, whenFalse, evalKey);
}

template <class P>
inline void TangorStarpuCMUXFFT(TRLWE<P>& output, const TRGSWFFT<P>& select,
                                const TRLWE<P>& whenTrue,
                                const TRLWE<P>& whenFalse)
{
    static_assert(std::is_same_v<P, lvl1param>,
                  "Tangor's KVSP StarPU backend supports lvl1 CMUXFFT only");
    Tangor::runIyokanStarpuCMUXFFT(output, select, whenTrue, whenFalse);
}

template <class brP, typename brP::targetP::T μ = brP::targetP::μ>
inline void TangorStarpuHomMUXwoSE(TRLWE<typename brP::targetP>& output,
                                   const TLWE<typename brP::domainP>& select,
                                   const TLWE<typename brP::domainP>& whenTrue,
                                   const TLWE<typename brP::domainP>& whenFalse,
                                   const EvalKey& evalKey)
{
    static_assert(std::is_same_v<brP, lvl01param>,
                  "Tangor's KVSP StarPU backend supports lvl01 MUXwoSE only");
    Tangor::runIyokanStarpuMuxWoSE(output, select, whenTrue, whenFalse,
                                   evalKey);
}

}  // namespace TFHEpp

#define HomAND TangorStarpuHomAND
#define HomNAND TangorStarpuHomNAND
#define HomANDYN TangorStarpuHomANDYN
#define HomOR TangorStarpuHomOR
#define HomNOR TangorStarpuHomNOR
#define HomORYN TangorStarpuHomORYN
#define HomXOR TangorStarpuHomXOR
#define HomXNOR TangorStarpuHomXNOR
#define HomNOT TangorStarpuHomNOT
#define HomMUX TangorStarpuHomMUX
#define CMUXFFT TangorStarpuCMUXFFT
#define HomMUXwoSE TangorStarpuHomMUXwoSE
