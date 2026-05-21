# Tangor
PoC of StarPU based Iyokan

## CPU build

Tangor builds the CPU path by default. The HOGE/FPGA path remains opt-in via
`-DUSE_HOGE=ON`.

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

Tangor uses `thirdparties/cuFHEpp` for the TFHEpp checkout at
`thirdparties/cuFHEpp/thirdparties/TFHEpp`. It does not build cuFHEpp's CUDA
library for the CPU path.

To enable cuFHEpp CUDA gate kernels through StarPU CUDA workers:

```sh
git submodule update --init --recursive
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DUSE_CUFHEPP=ON
cmake --build build -j
```

When `USE_CUFHEPP=ON`, Tangor builds the bundled `thirdparties/starpu` with
CUDA support by default, then builds cuFHEpp's `cufhe_gpu` target and adds CUDA
implementations to the StarPU gate codelets. CPU implementations remain
available as StarPU fallbacks.

StarPU uses one CUDA stream per CUDA worker. Set `STARPU_NWORKER_PER_CUDA` at
runtime to control streams per GPU. For A100-style one-block cuFHEpp kernels,
matching the worker count to the SM count is a useful experiment:

```sh
ulimit -s unlimited
STARPU_NCPU=0 STARPU_NCUDA=2 STARPU_NWORKER_PER_CUDA=108 bash test.bash
```

The bundled StarPU build reserves enough worker slots for two 108-SM GPUs by
default. For larger systems, raise `TANGOR_BUNDLED_STARPU_MAX_CPUS` at CMake
configure time. Keep `STARPU_CUDA_THREAD_PER_WORKER` unset unless you
specifically want one CPU-side StarPU driver thread per CUDA stream; Tangor's
CUDA codelets are asynchronous, so that setting is not needed for stream
concurrency.

The bundled StarPU path needs CUDA plus StarPU's autotools bootstrap
dependencies. On Ubuntu, install at least:

```sh
sudo apt-get install autoconf automake libtool libtool-bin make
```

To use a system StarPU instead, configure with `-DTANGOR_USE_BUNDLED_STARPU=OFF`.
If that StarPU was built without CUDA support, CMake prints a warning and builds
the CPU codelets only.

Set `TANGOR_WRITE_STARPU_BOUND_LP=1` at runtime to emit `minimum_runtime.lp`.

To use a different cuFHEpp checkout:

```sh
cmake -S . -B build -DTANGOR_CUFHEPP_SOURCE_DIR=/path/to/cuFHEpp
```

To override only TFHEpp:

```sh
cmake -S . -B build -DTANGOR_TFHEPP_SOURCE_DIR=/path/to/TFHEpp
```
