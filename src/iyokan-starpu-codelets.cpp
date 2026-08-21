#include "iyokan-starpu-codelets.hpp"

#include <array>
#include <atomic>
#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <unordered_map>

#include <starpu.h>

namespace Tangor {
struct IyokanStarpuTask::State {
    std::atomic_bool finished{false};
    std::atomic_int error{0};
    starpu_data_handle_t output{};
};

namespace {

using P = TFHEpp::lvl0param;

const TFHEpp::EvalKey* currentEvalKey = nullptr;
std::mutex backendMutex;
std::once_flag starpuInitOnce;
int starpuInitStatus = -1;
bool starpuRunning = false;
unsigned configuredCpuWorkers = 0;
unsigned configuredCudaDevices = 0;

std::mutex persistentHandlesMutex;
std::unordered_map<P::T*, starpu_data_handle_t> persistentHandles;
std::unordered_map<void*, starpu_data_handle_t> persistentVariableHandles;
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

starpu_data_handle_t persistentVariableHandleFor(void* value, size_t size)
{
    std::lock_guard<std::mutex> lock(persistentHandlesMutex);
    const auto it = persistentVariableHandles.find(value);
    if (it != persistentVariableHandles.end())
        return it->second;

    starpu_data_handle_t handle{};
    starpu_variable_data_register(&handle, STARPU_MAIN_RAM,
                                  reinterpret_cast<uintptr_t>(value), size);
    persistentVariableHandles.emplace(value, handle);
    return handle;
}

void taskCompleted(void* arg)
{
    auto* stateRef =
        static_cast<std::shared_ptr<IyokanStarpuTask::State>*>(arg);
    (*stateRef)->finished.store(true, std::memory_order_release);
    delete stateRef;
}

void captureTask(std::shared_ptr<IyokanStarpuTask> task)
{
    if (!captureActive)
        return;
    // A frontend task may expand into multiple StarPU primitives (for
    // example, a ROM/RAM CMUX tree). The last submitted task depends on all
    // values that feed its output, so it is the completion token propagated
    // back to Iyokan's ready queue.
    capturedTask = std::move(task);
}

void ensureStarpuRuntime()
{
    std::call_once(starpuInitOnce, [] {
#ifdef TANGOR_STARPU_PROFILE_DEFAULT
        // A profiling build is deliberately opt-in at CMake configuration
        // time. Respect environment variables so callers can redirect output
        // or selectively disable individual diagnostics without rebuilding.
        setenv("STARPU_PROFILING", "1", 0);
        setenv("STARPU_WORKER_STATS", "1", 0);
        setenv("STARPU_BUS_STATS", "1", 0);
        setenv("STARPU_PROFILING_TASK", "1", 0);
        setenv("STARPU_FXT_TRACE", "1", 0);
#endif
        if (configuredCudaDevices > 0)
            setenv("STARPU_NWORKER_PER_CUDA", "32", 0);

        starpu_conf config{};
        starpu_conf_init(&config);
        if (configuredCpuWorkers > 0)
            config.ncpus = static_cast<int>(configuredCpuWorkers);
        config.ncuda = static_cast<int>(configuredCudaDevices);
        if (std::getenv("STARPU_SCHED") == nullptr)
            config.sched_policy_name = "dmdas";

        starpuInitStatus = starpu_init(&config);
        if (starpuInitStatus == 0) {
            starpuRunning = true;
            std::atexit(shutdownIyokanStarpu);
        }
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

void cmuxFFTInPlaceTrueCodelet(void* buffers[], void*)
{
    auto& output = *reinterpret_cast<IyokanTRLWE*>(
        STARPU_VARIABLE_GET_PTR(buffers[0]));
    const auto& select = *reinterpret_cast<const IyokanTRGSWFFT*>(
        STARPU_VARIABLE_GET_PTR(buffers[1]));
    const auto& whenFalse = *reinterpret_cast<const IyokanTRLWE*>(
        STARPU_VARIABLE_GET_PTR(buffers[2]));
    const IyokanTRLWE whenTrue = output;
    TFHEpp::CMUXFFT<TFHEpp::lvl1param>(output, select, whenTrue, whenFalse);
}

void cmuxFFTInPlaceFalseCodelet(void* buffers[], void*)
{
    auto& output = *reinterpret_cast<IyokanTRLWE*>(
        STARPU_VARIABLE_GET_PTR(buffers[0]));
    const auto& select = *reinterpret_cast<const IyokanTRGSWFFT*>(
        STARPU_VARIABLE_GET_PTR(buffers[1]));
    const auto& whenTrue = *reinterpret_cast<const IyokanTRLWE*>(
        STARPU_VARIABLE_GET_PTR(buffers[2]));
    const IyokanTRLWE whenFalse = output;
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
    auto* model = new starpu_perfmodel{};
    model->type = STARPU_HISTORY_BASED;
    model->symbol = name;
    codelet.model = model;
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
starpu_codelet cmuxFFTInPlaceTrueStarpuCodelet = makeCodelet(
    cmuxFFTInPlaceTrueCodelet, "IyokanCMUXFFTInPlaceTrue",
    {STARPU_RW, STARPU_R, STARPU_R});
starpu_codelet cmuxFFTInPlaceFalseStarpuCodelet = makeCodelet(
    cmuxFFTInPlaceFalseCodelet, "IyokanCMUXFFTInPlaceFalse",
    {STARPU_RW, STARPU_R, STARPU_R});
starpu_codelet muxWoSEStarpuCodelet = makeCodelet(
    muxWoSECodelet, "IyokanMUXwoSE", {STARPU_W, STARPU_R, STARPU_R, STARPU_R});

#ifdef USE_CUFHEPP
struct IyokanRamCudaCodeletInitializer {
    IyokanRamCudaCodeletInitializer()
    {
        const char* const enableCudaCmux =
            std::getenv("TANGOR_EXPERIMENTAL_CUDA_CMUX");
        if (enableCudaCmux == nullptr || enableCudaCmux[0] != '1' ||
            enableCudaCmux[1] != '\0')
            return;

        addCudaImplementation(cmuxFFTStarpuCodelet, iyokanCufheppCMUXFFT);
        addCudaImplementation(cmuxFFTInPlaceTrueStarpuCodelet,
                              iyokanCufheppCMUXFFTInPlaceTrue);
        addCudaImplementation(cmuxFFTInPlaceFalseStarpuCodelet,
                              iyokanCufheppCMUXFFTInPlaceFalse);
    }
};

IyokanRamCudaCodeletInitializer iyokanRamCudaCodeletInitializer;
#endif

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
std::shared_ptr<IyokanStarpuTask> submitAsync(
    starpu_codelet& codelet, std::array<IyokanTLWE*, Count> values)
{
    std::array<starpu_data_handle_t, Count> handles{};
    for (size_t i = 0; i < Count; ++i)
        handles[i] = persistentHandleFor(*values[i]);

    auto state = std::make_shared<IyokanStarpuTask::State>();
    state->output = handles[0];
    auto* callbackState =
        new std::shared_ptr<IyokanStarpuTask::State>(state);
    int status = 0;
    if constexpr (Count == 2)
        status = starpu_task_insert(
            &codelet, STARPU_W, handles[0], STARPU_R, handles[1],
            STARPU_CALLBACK_WITH_ARG_NFREE, taskCompleted, callbackState, 0);
    else if constexpr (Count == 3)
        status = starpu_task_insert(
            &codelet, STARPU_W, handles[0], STARPU_R, handles[1], STARPU_R,
            handles[2], STARPU_CALLBACK_WITH_ARG_NFREE, taskCompleted,
            callbackState, 0);
    else
        status = starpu_task_insert(
            &codelet, STARPU_W, handles[0], STARPU_R, handles[1], STARPU_R,
            handles[2], STARPU_R, handles[3], STARPU_CALLBACK_WITH_ARG_NFREE,
            taskCompleted, callbackState, 0);
    if (status != 0) {
        delete callbackState;
        throw std::runtime_error("Tangor could not submit an asynchronous StarPU gate");
    }
    return std::shared_ptr<IyokanStarpuTask>(
        new IyokanStarpuTask(std::move(state)));
}

template <size_t Count>
std::shared_ptr<IyokanStarpuTask> submitVariableAsync(
    starpu_codelet& codelet, const std::array<void*, Count>& values,
    const std::array<size_t, Count>& sizes,
    const std::array<enum starpu_data_access_mode, Count>& modes)
{
    std::array<starpu_data_handle_t, Count> handles{};
    for (size_t i = 0; i < handles.size(); ++i) {
        handles[i] = persistentVariableHandleFor(values[i], sizes[i]);
    }

    auto state = std::make_shared<IyokanStarpuTask::State>();
    state->output = handles[0];
    auto* callbackState =
        new std::shared_ptr<IyokanStarpuTask::State>(state);
    int status = 0;
    if constexpr (Count == 3) {
        status = starpu_task_insert(
            &codelet, modes[0], handles[0], modes[1], handles[1], modes[2],
            handles[2], STARPU_CALLBACK_WITH_ARG_NFREE, taskCompleted,
            callbackState, 0);
    }
    else {
        status = starpu_task_insert(
            &codelet, modes[0], handles[0], modes[1], handles[1], modes[2],
            handles[2], modes[3], handles[3],
            STARPU_CALLBACK_WITH_ARG_NFREE, taskCompleted, callbackState, 0);
    }
    if (status != 0) {
        delete callbackState;
        throw std::runtime_error("Tangor could not submit a StarPU RAM task");
    }
    return std::shared_ptr<IyokanStarpuTask>(
        new IyokanStarpuTask(std::move(state)));
}

std::shared_ptr<IyokanStarpuTask> submitMuxWoSEAsync(
    IyokanTRLWE& output, const IyokanTLWE& select,
    const IyokanTLWE& whenTrue, const IyokanTLWE& whenFalse)
{
    const starpu_data_handle_t outputHandle =
        persistentVariableHandleFor(&output, sizeof(output));
    const std::array<starpu_data_handle_t, 3> inputHandles{
        persistentHandleFor(const_cast<IyokanTLWE&>(select)),
        persistentHandleFor(const_cast<IyokanTLWE&>(whenTrue)),
        persistentHandleFor(const_cast<IyokanTLWE&>(whenFalse))};

    auto state = std::make_shared<IyokanStarpuTask::State>();
    state->output = outputHandle;
    auto* callbackState =
        new std::shared_ptr<IyokanStarpuTask::State>(state);
    const int status = starpu_task_insert(
        &muxWoSEStarpuCodelet, STARPU_W, outputHandle, STARPU_R,
        inputHandles[0], STARPU_R, inputHandles[1], STARPU_R, inputHandles[2],
        STARPU_CALLBACK_WITH_ARG_NFREE, taskCompleted, callbackState, 0);
    if (status != 0) {
        delete callbackState;
        throw std::runtime_error("Tangor could not submit a StarPU RAM MUX task");
    }
    return std::shared_ptr<IyokanStarpuTask>(
        new IyokanStarpuTask(std::move(state)));
}

void waitForTask(const std::shared_ptr<IyokanStarpuTask>& task)
{
    while (!task->isFinished())
        std::this_thread::yield();
    task->synchronizeOutput();
}

void markVariableHostWrite(void* value)
{
    starpu_data_handle_t handle{};
    {
        std::lock_guard<std::mutex> lock(persistentHandlesMutex);
        const auto it = persistentVariableHandles.find(value);
        if (it == persistentVariableHandles.end())
            return;
        handle = it->second;
    }
    if (starpu_data_acquire(handle, STARPU_W) != 0)
        throw std::runtime_error("Tangor could not mark a CPU variable write");
    starpu_data_release(handle);
}

}  // namespace

void configureIyokanStarpu(unsigned cpuWorkers, unsigned cudaDevices)
{
    std::lock_guard<std::mutex> lock(backendMutex);
    if (starpuRunning || starpuInitStatus != -1)
        throw std::runtime_error(
            "Tangor's StarPU worker configuration is already active");
    configuredCpuWorkers = cpuWorkers;
    configuredCudaDevices = cudaDevices;
}

void prepareIyokanStarpu(const TFHEpp::EvalKey& evalKey)
{
    ensureStarpu(evalKey);
}

void shutdownIyokanStarpu()
{
    std::lock_guard<std::mutex> backendLock(backendMutex);
    if (!starpuRunning)
        return;

    starpu_task_wait_for_all();
    {
        std::lock_guard<std::mutex> handlesLock(persistentHandlesMutex);
        for (const auto& [_, handle] : persistentHandles)
            starpu_data_unregister_no_coherency(handle);
        for (const auto& [_, handle] : persistentVariableHandles)
            starpu_data_unregister_no_coherency(handle);
        persistentHandles.clear();
        persistentVariableHandles.clear();
    }
#ifdef USE_CUFHEPP
    cleanupIyokanCufhepp();
#endif
    currentEvalKey = nullptr;
    starpu_shutdown();
    starpuRunning = false;
}

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
    int error = state_->error.load(std::memory_order_acquire);
    if (error == 0) {
        error = starpu_data_acquire(state_->output, STARPU_R);
        if (error == 0)
            starpu_data_release(state_->output);
        else
            state_->error.store(error, std::memory_order_release);
    }
    if (error != 0)
        throw std::runtime_error("Tangor could not synchronize a StarPU output");
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

void synchronizeIyokanStarpuCapture()
{
    if (!captureActive)
        throw std::runtime_error(
            "Tangor synchronized an inactive Iyokan StarPU capture");
    if (capturedTask)
        waitForTask(capturedTask);
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

void markIyokanTRLWEHostWrite(IyokanTRLWE& value)
{
    markVariableHostWrite(&value);
}

void markIyokanTRGSWFFTHostWrite(IyokanTRGSWFFT& value)
{
    markVariableHostWrite(&value);
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
    waitForTask(submitAsync(codeletFor(gate),
                            std::array{&output,
                                       const_cast<IyokanTLWE*>(&left),
                                       const_cast<IyokanTLWE*>(&right)}));
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
    waitForTask(submitAsync(
        notStarpuCodelet,
        std::array{&output, const_cast<IyokanTLWE*>(&input)}));
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
    waitForTask(submitAsync(
        muxStarpuCodelet,
        std::array{&output, const_cast<IyokanTLWE*>(&select),
                   const_cast<IyokanTLWE*>(&whenTrue),
                   const_cast<IyokanTLWE*>(&whenFalse)}));
}

void runIyokanStarpuCMUXFFT(IyokanTRLWE& output,
                            const IyokanTRGSWFFT& select,
                            const IyokanTRLWE& whenTrue,
                            const IyokanTRLWE& whenFalse)
{
    ensureStarpuRuntime();
    std::shared_ptr<IyokanStarpuTask> task;
    if (&output == &whenTrue) {
        task = submitVariableAsync<3>(
            cmuxFFTInPlaceTrueStarpuCodelet,
            {&output, const_cast<IyokanTRGSWFFT*>(&select),
             const_cast<IyokanTRLWE*>(&whenFalse)},
            {sizeof(IyokanTRLWE), sizeof(IyokanTRGSWFFT), sizeof(IyokanTRLWE)},
            {STARPU_RW, STARPU_R, STARPU_R});
    }
    else if (&output == &whenFalse) {
        task = submitVariableAsync<3>(
            cmuxFFTInPlaceFalseStarpuCodelet,
            {&output, const_cast<IyokanTRGSWFFT*>(&select),
             const_cast<IyokanTRLWE*>(&whenTrue)},
            {sizeof(IyokanTRLWE), sizeof(IyokanTRGSWFFT), sizeof(IyokanTRLWE)},
            {STARPU_RW, STARPU_R, STARPU_R});
    }
    else {
        task = submitVariableAsync<4>(
            cmuxFFTStarpuCodelet,
            {&output, const_cast<IyokanTRGSWFFT*>(&select),
             const_cast<IyokanTRLWE*>(&whenTrue),
             const_cast<IyokanTRLWE*>(&whenFalse)},
            {sizeof(IyokanTRLWE), sizeof(IyokanTRGSWFFT), sizeof(IyokanTRLWE),
             sizeof(IyokanTRLWE)},
            {STARPU_W, STARPU_R, STARPU_R, STARPU_R});
    }
    if (captureActive)
        captureTask(task);
    else
        waitForTask(task);
}

void runIyokanStarpuMuxWoSE(IyokanTRLWE& output, const IyokanTLWE& select,
                            const IyokanTLWE& whenTrue,
                            const IyokanTLWE& whenFalse,
                            const TFHEpp::EvalKey& evalKey)
{
    ensureStarpu(evalKey);
    auto task = submitMuxWoSEAsync(output, select, whenTrue, whenFalse);
    if (captureActive)
        captureTask(task);
    else
        waitForTask(task);
}

}  // namespace Tangor
