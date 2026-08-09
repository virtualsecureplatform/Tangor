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

## KVSP compatibility

With the default `-DTANGOR_BUILD_KVSP_COMPAT=ON`, the build produces
`build/bin/iyokan` and `build/bin/iyokan-packet` in addition to Tangor's
native executable when StarPU is available. They implement the CLI and cereal packet/archive format
expected by KVSP, including `plain`, `tfhe`, snapshots, key generation, and
packet conversion. Point KVSP at them without changing the KVSP command line:

```sh
KVSP_IYOKAN_PATH="$PWD/build/bin/iyokan" \
KVSP_IYOKAN_PACKET_PATH="$PWD/build/bin/iyokan-packet" \
  /path/to/kvsp
```

The compatibility targets share the checked-out Iyokan frontend sources while
linking to Tangor's selected TFHEpp build. This keeps packets, evaluation keys,
and snapshots byte-compatible during the scheduler migration. Set
`-DTANGOR_BUILD_KVSP_COMPAT=OFF` for a standalone Tangor-only build.
If that checkout is not adjacent to Tangor, CMake fetches the pinned Iyokan
frontend (including its submodules) by default; use
`-DTANGOR_FETCH_IYOKAN_COMPAT=OFF` plus
`TANGOR_IYOKAN_COMPAT_SOURCE_DIR` and
`TANGOR_IYOKAN_COMPAT_THIRDPARTY_DIR` for an offline source mirror.

When StarPU is available, `TANGOR_KVSP_STARPU_GATE_OFFLOAD=ON` (the default)
routes the frontend's level-0 AND/NAND/ANDNOT/OR/NOR/ORNOT/XOR/XNOR/NOT/MUX
operations through Tangor StarPU codelets. Disable it to compare the frontend
against its original CPU gate path. The same dispatch layer covers the packed
level-1 `CMUXFFT` ROM/RAM selector and RAM's `HomMUXwoSE` write selection.

Tangor also accepts KVSP's existing Iyokan CMake cache variables, so its
AVX2/AVX-512 build recipes can use Tangor as the source directory without
renaming flags:

```sh
cmake -S /path/to/Tangor -B build/Iyokan-avx2 \
  -DCMAKE_BUILD_TYPE=Release \
  -DIYOKAN_ENABLE_CUDA=OFF \
  -DIYOKAN_MARCH=x86-64-v3 \
  -DUSE_AVX512=OFF
cmake --build build/Iyokan-avx2 --target iyokan iyokan-packet
```

`IYOKAN_80BIT_SECURITY` and `IYOKAN_ENABLE_CUDA` are accepted as well; the
latter maps to Tangor's cuFHEpp/StarPU CUDA configuration.

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
