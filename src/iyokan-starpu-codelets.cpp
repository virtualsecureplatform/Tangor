#include "iyokan-starpu-codelets.hpp"

#include <array>
#include <atomic>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include <starpu.h>

namespace Tangor {
struct IyokanStarpuTask::State {
    std::atomic_bool finished{false};
    starpu_data_handle_t output{};
};

namespace {

using P = TFHEpp::lvl0param;

const TFHEpp::EvalKey* currentEvalKey = nullptr;
std::mutex backendMutex;
std::once_flag starpuInitOnce;
int starpuInitStatus = -1;

std::mutex persistentHandlesMutex;
std::unordered_map<P::T*, starpu_data_handle_t> persistentHandles;
thread_local bool captureActive = false;
thread_local std::shared_ptr<IyokanStarpuTask> capturedTask;

starpu_data_handle_t persistentHandleFor(IyokanTLWE& value)
{
    auto* data = value.data();
    std::lock_guard<std::mutex> lock(persistentHandlesMutex);
    const auto it = persistentHandles.find(data);
    if (it != persistentHandles.end())
        return it->second;

    starpu_data_handle_t handle{};
    starpu_vector_data_register(&handle, STARPU_MAIN_RAM,
                                reinterpret_cast<uintptr_t>(data),
                                value.size(), sizeof(P::T));
    persistentHandles.emplace(data, handle);
    return handle;
}

void taskCompleted(void* arg)
{
    auto* state = static_cast<IyokanStarpuTask::State*>(arg);
    const int status = starpu_data_acquire_cb(
        state->output, STARPU_R,
        [](void* completedArg) {
            auto* completed = static_cast<IyokanStarpuTask::State*>(completedArg);
            starpu_data_release(completed->output);
            completed->finished.store(true, std::memory_order_release);
        },
        state);
    if (status != 0)
        throw std::runtime_error("Tangor could not queue a StarPU host copy");
}

void captureTask(std::shared_ptr<IyokanStarpuTask> task)
{
    if (!captureActive)
        return;
    if (capturedTask)
        throw std::runtime_error("Tangor captured more than one Iyokan gate");
    capturedTask = std::move(task);
}

void ensureStarpuRuntime()
{
    std::call_once(starpuInitOnce, [] {
        starpuInitStatus = starpu_init(nullptr);
        if (starpuInitStatus == 0)
            std::atexit([] { starpu_shutdown(); });
    });
    if (starpuInitStatus != 0)
        throw std::runtime_error("Tangor could not initialize StarPU");
}

void ensureStarpu(const TFHEpp::EvalKey& evalKey)
{
    ensureStarpuRuntime();
    std::lock_guard<std::mutex> lock(backendMutex);
    if (currentEvalKey == nullptr)
        currentEvalKey = &evalKey;
    else if (currentEvalKey != &evalKey)
        throw std::runtime_error(
            "Tangor's KVSP StarPU backend cannot evaluate two keys in one process");
#ifdef USE_CUFHEPP
    initializeIyokanCufhepp(evalKey);
#endif
}

template <int caSign, int cbSign,
          std::make_signed_t<typename P::T> offset>
void binaryCodelet(void* buffers[], void*)
{
    IyokanTLWE output, left, right;
    const auto* leftData = reinterpret_cast<const P::T*>(
        STARPU_VECTOR_GET_PTR(buffers[1]));
    const auto* rightData = reinterpret_cast<const P::T*>(
        STARPU_VECTOR_GET_PTR(buffers[2]));
    for (size_t i = 0; i < output.size(); ++i) {
        left[i] = leftData[i];
        right[i] = rightData[i];
    }
    TFHEpp::HomGate<TFHEpp::lvl01param, TFHEpp::lvl1param::μ,
                    TFHEpp::lvl10param, caSign, cbSign, offset>(
        output, left, right, *currentEvalKey);
    auto* outputData = reinterpret_cast<P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]));
    for (size_t i = 0; i < output.size(); ++i)
        outputData[i] = output[i];
}

void notCodelet(void* buffers[], void*)
{
    auto* output = reinterpret_cast<P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]));
    const auto* input = reinterpret_cast<const P::T*>(
        STARPU_VECTOR_GET_PTR(buffers[1]));
    for (size_t i = 0; i < IyokanTLWE{}.size(); ++i)
        output[i] = -input[i];
}

void muxCodelet(void* buffers[], void*)
{
    IyokanTLWE output, select, whenTrue, whenFalse;
    const std::array<void*, 3> inputs{buffers[1], buffers[2], buffers[3]};
    const auto copyInput = [](IyokanTLWE& destination, void* source) {
        const auto* values = reinterpret_cast<const P::T*>(
            STARPU_VECTOR_GET_PTR(source));
        for (size_t i = 0; i < destination.size(); ++i)
            destination[i] = values[i];
    };
    copyInput(select, inputs[0]);
    copyInput(whenTrue, inputs[1]);
    copyInput(whenFalse, inputs[2]);
    TFHEpp::HomMUX<P>(output, select, whenTrue, whenFalse, *currentEvalKey);
    auto* values = reinterpret_cast<P::T*>(STARPU_VECTOR_GET_PTR(buffers[0]));
    for (size_t i = 0; i < output.size(); ++i)
        values[i] = output[i];
}

void cmuxFFTCodelet(void* buffers[], void*)
{
    auto& output = *reinterpret_cast<IyokanTRLWE*>(
        STARPU_VARIABLE_GET_PTR(buffers[0]));
    const auto& select = *reinterpret_cast<const IyokanTRGSWFFT*>(
        STARPU_VARIABLE_GET_PTR(buffers[1]));
    const auto& whenTrue = *reinterpret_cast<const IyokanTRLWE*>(
        STARPU_VARIABLE_GET_PTR(buffers[2]));
    const auto& whenFalse = *reinterpret_cast<const IyokanTRLWE*>(
        STARPU_VARIABLE_GET_PTR(buffers[3]));
    TFHEpp::CMUXFFT<TFHEpp::lvl1param>(output, select, whenTrue, whenFalse);
}

void muxWoSECodelet(void* buffers[], void*)
{
    auto& output = *reinterpret_cast<IyokanTRLWE*>(
        STARPU_VARIABLE_GET_PTR(buffers[0]));
    const auto& select = *reinterpret_cast<const IyokanTLWE*>(
        STARPU_VARIABLE_GET_PTR(buffers[1]));
    const auto& whenTrue = *reinterpret_cast<const IyokanTLWE*>(
        STARPU_VARIABLE_GET_PTR(buffers[2]));
    const auto& whenFalse = *reinterpret_cast<const IyokanTLWE*>(
        STARPU_VARIABLE_GET_PTR(buffers[3]));
    TFHEpp::HomMUXwoSE<TFHEpp::lvl01param>(output, select, whenTrue,
                                             whenFalse, *currentEvalKey);
}

template <class Function>
starpu_codelet makeCodelet(Function function, const char* name,
                           std::initializer_list<enum starpu_data_access_mode> modes)
{
    starpu_codelet codelet{};
    codelet.where = STARPU_CPU;
    codelet.cpu_funcs[0] = function;
    codelet.cpu_funcs_name[0] = name;
    codelet.nbuffers = modes.size();
    size_t index = 0;
    for (const auto mode : modes)
        codelet.modes[index++] = mode;
    return codelet;
}

#ifdef USE_CUFHEPP
void addCudaImplementation(starpu_codelet& codelet,
                           starpu_cuda_func_t cudaFunction)
{
    codelet.where = STARPU_CPU | STARPU_CUDA;
    codelet.cuda_funcs[0] = cudaFunction;
    codelet.cuda_flags[0] = STARPU_CUDA_ASYNC;
}
#endif

#define TANGOR_BINARY_CODELET(name, caSign, cbSign, offset)                     \
    starpu_codelet name = makeCodelet(                                         \
        binaryCodelet<caSign, cbSign, offset>, #name,                           \
        {STARPU_W, STARPU_R, STARPU_R})

TANGOR_BINARY_CODELET(andCodelet, 1, 1, -P::μ);
TANGOR_BINARY_CODELET(nandCodelet, -1, -1, P::μ);
TANGOR_BINARY_CODELET(andNotCodelet, 1, -1, -P::μ);
TANGOR_BINARY_CODELET(orCodelet, 1, 1, P::μ);
TANGOR_BINARY_CODELET(norCodelet, -1, -1, -P::μ);
TANGOR_BINARY_CODELET(orNotCodelet, 1, -1, P::μ);
TANGOR_BINARY_CODELET(xorCodelet, 2, 2, 2 * P::μ);
TANGOR_BINARY_CODELET(xnorCodelet, -2, -2, -2 * P::μ);

#ifdef USE_CUFHEPP
struct IyokanCudaCodeletInitializer {
    IyokanCudaCodeletInitializer()
    {
        addCudaImplementation(andCodelet, iyokanCufheppHomAND);
        addCudaImplementation(nandCodelet, iyokanCufheppHomNAND);
        addCudaImplementation(andNotCodelet, iyokanCufheppHomANDNOT);
        addCudaImplementation(orCodelet, iyokanCufheppHomOR);
        addCudaImplementation(norCodelet, iyokanCufheppHomNOR);
        addCudaImplementation(orNotCodelet, iyokanCufheppHomORNOT);
        addCudaImplementation(xorCodelet, iyokanCufheppHomXOR);
        addCudaImplementation(xnorCodelet, iyokanCufheppHomXNOR);
    }
};

IyokanCudaCodeletInitializer iyokanCudaCodeletInitializer;
#endif

starpu_codelet notStarpuCodelet =
    makeCodelet(notCodelet, "IyokanNOT", {STARPU_W, STARPU_R});
starpu_codelet muxStarpuCodelet = makeCodelet(
    muxCodelet, "IyokanMUX", {STARPU_W, STARPU_R, STARPU_R, STARPU_R});
#ifdef USE_CUFHEPP
struct IyokanMuxCudaCodeletInitializer {
    IyokanMuxCudaCodeletInitializer()
    {
        addCudaImplementation(muxStarpuCodelet, iyokanCufheppHomMUX);
    }
};

IyokanMuxCudaCodeletInitializer iyokanMuxCudaCodeletInitializer;
#endif
starpu_codelet cmuxFFTStarpuCodelet = makeCodelet(
    cmuxFFTCodelet, "IyokanCMUXFFT", {STARPU_W, STARPU_R, STARPU_R, STARPU_R});
starpu_codelet muxWoSEStarpuCodelet = makeCodelet(
    muxWoSECodelet, "IyokanMUXwoSE", {STARPU_W, STARPU_R, STARPU_R, STARPU_R});

starpu_codelet& codeletFor(IyokanBinaryGate gate)
{
    switch (gate) {
    case IyokanBinaryGate::AND: return andCodelet;
    case IyokanBinaryGate::NAND: return nandCodelet;
    case IyokanBinaryGate::ANDNOT: return andNotCodelet;
    case IyokanBinaryGate::OR: return orCodelet;
    case IyokanBinaryGate::NOR: return norCodelet;
    case IyokanBinaryGate::ORNOT: return orNotCodelet;
    case IyokanBinaryGate::XOR: return xorCodelet;
    case IyokanBinaryGate::XNOR: return xnorCodelet;
    }
    throw std::logic_error("unknown Iyokan binary gate");
}

template <size_t Count>
void submitAndWait(starpu_codelet& codelet, std::array<IyokanTLWE*, Count> values)
{
    std::array<starpu_data_handle_t, Count> handles{};
    for (size_t i = 0; i < Count; ++i) {
        starpu_vector_data_register(&handles[i], STARPU_MAIN_RAM,
                                    reinterpret_cast<uintptr_t>(values[i]->data()),
                                    values[i]->size(), sizeof(P::T));
    }
    int status = 0;
    if constexpr (Count == 2)
        status = starpu_task_insert(&codelet, STARPU_W, handles[0], STARPU_R,
                                    handles[1], 0);
    else if constexpr (Count == 3)
        status = starpu_task_insert(&codelet, STARPU_W, handles[0], STARPU_R,
                                    handles[1], STARPU_R, handles[2], 0);
    else
        status = starpu_task_insert(&codelet, STARPU_W, handles[0], STARPU_R,
                                    handles[1], STARPU_R, handles[2], STARPU_R,
                                    handles[3], 0);
    if (status != 0)
        throw std::runtime_error("Tangor could not submit a StarPU gate task");
    starpu_task_wait_for_all();
    for (auto handle : handles)
        starpu_data_unregister(handle);
}

template <size_t Count>
std::shared_ptr<IyokanStarpuTask> submitAsync(
    starpu_codelet& codelet, std::array<IyokanTLWE*, Count> values)
{
    std::array<starpu_data_handle_t, Count> handles{};
    for (size_t i = 0; i < Count; ++i)
        handles[i] = persistentHandleFor(*values[i]);

    auto state = std::make_shared<IyokanStarpuTask::State>();
    state->output = handles[0];
    int status = 0;
    if constexpr (Count == 2)
        status = starpu_task_insert(
            &codelet, STARPU_W, handles[0], STARPU_R, handles[1],
            STARPU_CALLBACK_WITH_ARG_NFREE, taskCompleted, state.get(), 0);
    else if constexpr (Count == 3)
        status = starpu_task_insert(
            &codelet, STARPU_W, handles[0], STARPU_R, handles[1], STARPU_R,
            handles[2], STARPU_CALLBACK_WITH_ARG_NFREE, taskCompleted,
            state.get(), 0);
    else
        status = starpu_task_insert(
            &codelet, STARPU_W, handles[0], STARPU_R, handles[1], STARPU_R,
            handles[2], STARPU_R, handles[3], STARPU_CALLBACK_WITH_ARG_NFREE,
            taskCompleted, state.get(), 0);
    if (status != 0)
        throw std::runtime_error("Tangor could not submit an asynchronous StarPU gate");
    return std::shared_ptr<IyokanStarpuTask>(
        new IyokanStarpuTask(std::move(state)));
}

void submitVariableAndWait(starpu_codelet& codelet,
                           const std::array<void*, 4>& values,
                           const std::array<size_t, 4>& sizes)
{
    std::array<starpu_data_handle_t, 4> handles{};
    for (size_t i = 0; i < handles.size(); ++i) {
        starpu_variable_data_register(&handles[i], STARPU_MAIN_RAM,
                                      reinterpret_cast<uintptr_t>(values[i]),
                                      sizes[i]);
    }
    const int status = starpu_task_insert(
        &codelet, STARPU_W, handles[0], STARPU_R, handles[1], STARPU_R,
        handles[2], STARPU_R, handles[3], 0);
    if (status != 0)
        throw std::runtime_error("Tangor could not submit a StarPU RAM task");
    starpu_task_wait_for_all();
    for (auto handle : handles)
        starpu_data_unregister(handle);
}

}  // namespace

IyokanStarpuTask::IyokanStarpuTask(std::shared_ptr<State> state)
    : state_(std::move(state))
{
}

bool IyokanStarpuTask::isFinished() const
{
    return state_->finished.load(std::memory_order_acquire);
}

void IyokanStarpuTask::synchronizeOutput() const
{
    // Completion is published only after taskCompleted's asynchronous host
    // acquisition has made this value coherent in main memory.
}

void beginIyokanStarpuCapture()
{
    if (captureActive)
        throw std::runtime_error("Tangor nested an Iyokan StarPU capture");
    captureActive = true;
    capturedTask.reset();
}

std::shared_ptr<IyokanStarpuTask> endIyokanStarpuCapture()
{
    if (!captureActive)
        throw std::runtime_error("Tangor ended an inactive Iyokan StarPU capture");
    captureActive = false;
    return std::move(capturedTask);
}

void markIyokanTLWEHostWrite(IyokanTLWE& value)
{
    starpu_data_handle_t handle{};
    {
        std::lock_guard<std::mutex> lock(persistentHandlesMutex);
        const auto it = persistentHandles.find(value.data());
        if (it == persistentHandles.end())
            return;
        handle = it->second;
    }
    if (starpu_data_acquire(handle, STARPU_W) != 0)
        throw std::runtime_error("Tangor could not mark a CPU TLWE write");
    starpu_data_release(handle);
}

void runIyokanStarpuBinaryGate(IyokanBinaryGate gate, IyokanTLWE& output,
                               const IyokanTLWE& left, const IyokanTLWE& right,
                               const TFHEpp::EvalKey& evalKey)
{
    ensureStarpu(evalKey);
    if (captureActive) {
        captureTask(submitAsync(codeletFor(gate),
                                std::array{&output,
                                           const_cast<IyokanTLWE*>(&left),
                                           const_cast<IyokanTLWE*>(&right)}));
        return;
    }
    submitAndWait(codeletFor(gate),
                  std::array{&output, const_cast<IyokanTLWE*>(&left),
                             const_cast<IyokanTLWE*>(&right)});
}

void runIyokanStarpuNot(IyokanTLWE& output, const IyokanTLWE& input)
{
    // NOT is linear and does not need an evaluation key, but it still uses the
    // same StarPU lifetime as the bootstrapped gates.
    ensureStarpuRuntime();
    if (captureActive) {
        captureTask(submitAsync(notStarpuCodelet,
                                std::array{&output,
                                           const_cast<IyokanTLWE*>(&input)}));
        return;
    }
    submitAndWait(notStarpuCodelet,
                  std::array{&output, const_cast<IyokanTLWE*>(&input)});
}

void runIyokanStarpuMux(IyokanTLWE& output, const IyokanTLWE& select,
                        const IyokanTLWE& whenTrue,
                        const IyokanTLWE& whenFalse,
                        const TFHEpp::EvalKey& evalKey)
{
    ensureStarpu(evalKey);
    if (captureActive) {
        captureTask(submitAsync(
            muxStarpuCodelet,
            std::array{&output, const_cast<IyokanTLWE*>(&select),
                       const_cast<IyokanTLWE*>(&whenTrue),
                       const_cast<IyokanTLWE*>(&whenFalse)}));
        return;
    }
    submitAndWait(muxStarpuCodelet,
                  std::array{&output, const_cast<IyokanTLWE*>(&select),
                             const_cast<IyokanTLWE*>(&whenTrue),
                             const_cast<IyokanTLWE*>(&whenFalse)});
}

void runIyokanStarpuCMUXFFT(IyokanTRLWE& output,
                            const IyokanTRGSWFFT& select,
                            const IyokanTRLWE& whenTrue,
                            const IyokanTRLWE& whenFalse)
{
    // The RAM selector updates its accumulator in place.  Registering that
    // same object as both the write buffer and a separate read buffer creates
    // two StarPU handles for one allocation, which is invalid and can crash
    // during RAM evaluation.  Keep this CPU-only selector operation local in
    // the aliased case; it has no CUDA implementation yet in any event.
    if (&output == &whenTrue || &output == &whenFalse) {
        TFHEpp::CMUXFFT<TFHEpp::lvl1param>(output, select, whenTrue,
                                           whenFalse);
        return;
    }
    ensureStarpuRuntime();
    submitVariableAndWait(
        cmuxFFTStarpuCodelet,
        {&output, const_cast<IyokanTRGSWFFT*>(&select),
         const_cast<IyokanTRLWE*>(&whenTrue),
         const_cast<IyokanTRLWE*>(&whenFalse)},
        {sizeof(IyokanTRLWE), sizeof(IyokanTRGSWFFT), sizeof(IyokanTRLWE),
         sizeof(IyokanTRLWE)});
}

void runIyokanStarpuMuxWoSE(IyokanTRLWE& output, const IyokanTLWE& select,
                            const IyokanTLWE& whenTrue,
                            const IyokanTLWE& whenFalse,
                            const TFHEpp::EvalKey& evalKey)
{
    ensureStarpu(evalKey);
    std::array<starpu_data_handle_t, 4> handles{};
    starpu_variable_data_register(&handles[0], STARPU_MAIN_RAM,
                                  reinterpret_cast<uintptr_t>(&output),
                                  sizeof(output));
    const std::array<const IyokanTLWE*, 3> inputs{&select, &whenTrue,
                                                   &whenFalse};
    for (size_t index = 0; index < inputs.size(); ++index) {
        starpu_variable_data_register(&handles[index + 1], STARPU_MAIN_RAM,
                                      reinterpret_cast<uintptr_t>(inputs[index]),
                                      sizeof(IyokanTLWE));
    }
    const int status = starpu_task_insert(
        &muxWoSEStarpuCodelet, STARPU_W, handles[0], STARPU_R, handles[1],
        STARPU_R, handles[2], STARPU_R, handles[3], 0);
    if (status != 0)
        throw std::runtime_error("Tangor could not submit a StarPU RAM task");
    starpu_task_wait_for_all();
    for (auto handle : handles)
        starpu_data_unregister(handle);
}

}  // namespace Tangor
