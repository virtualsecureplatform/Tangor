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

The default TFHE configuration is Block Binary keys with Subset Keys
(`USE_BLOCK_BINARY=ON`, `USE_SUBSET_KEY=ON`). Regenerate secret/evaluation
keys, encrypted packets, and snapshots after upgrading, since artifacts made
with the former defaults are not compatible.

## KVSP compatibility

With the default `-DTANGOR_BUILD_KVSP_COMPAT=ON`, the build produces
`build/bin/iyokan` and `build/bin/iyokan-packet` in addition to Tangor's
native executable. They implement the CLI and cereal packet/archive format
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

### GPU execution (recommended)

The default KVSP GPU path uses Iyokan's native cuFHE frontend, built through
Tangor's compatibility target. It keeps the ordinary gate graph and the
GPU-compatible RAM/ROM work on the native multi-stream CUDA path, while the
requested CPU worker pool performs the frontend work. This is the released
high-performance configuration; pass `--enable-gpu` at runtime.

Tangor accepts KVSP's Iyokan CMake cache variables, so KVSP can use Tangor as
its source directory without renaming build flags:

```sh
git submodule update --init --recursive
cmake -S /path/to/Tangor -B build/Iyokan-avx2 \
  -DCMAKE_BUILD_TYPE=Release \
  -DIYOKAN_ENABLE_CUDA=ON \
  -DIYOKAN_MARCH=x86-64-v3 \
  -DUSE_AVX512=OFF
cmake --build build/Iyokan-avx2 --target iyokan iyokan-packet
```

To have KVSP build that compatibility target in its normal location:

```sh
make -C /path/to/kvsp ENABLE_CUDA=1 IYOKAN_SOURCE=/path/to/Tangor iyokan-avx2
```

Run with both CPU and GPU resources. For example, this uses 64 CPU workers and
two GPUs:

```sh
/path/to/kvsp/build/bin/iyokan-avx2 tfhe --enable-gpu --cpu 64 --num-gpu 2 \
  --evalkey eval.key -c 224 -o result.enc --snapshot result.snapshot \
  --blueprint /path/to/kvsp/build/share/kvsp/alexandrite.toml -i fib.enc
```

The evaluator cannot inspect encrypted termination itself. For KVSP's bundled
`fib(5)` input, a plaintext emulator establishes that 224 cycles are required.
Decrypt the resulting packet to confirm `f0 = true` and `x10 = 5`:

```sh
/path/to/kvsp/build/bin/kvsp dec --cpu alexandrite -k secret.key -i result.enc
```

On two A100 GPUs, the full 224-cycle Fibonacci workload was verified against
Iyokan with byte-identical decrypted packets; Tangor completed in 221.02 s and
Iyokan in 219.44 s in that sample.

### Experimental StarPU gate offload

`TANGOR_KVSP_STARPU_GATE_OFFLOAD=ON` enables the alternative StarPU codelet
backend. It is disabled by default because the native cuFHE GPU path above is
the faster complete KVSP implementation. This mode is useful for scheduler and
codelet experiments; run it without `--enable-gpu` and configure StarPU worker
counts explicitly, for example:

```sh
cmake -S /path/to/Tangor -B build/starpu \
  -DIYOKAN_ENABLE_CUDA=ON -DTANGOR_KVSP_STARPU_GATE_OFFLOAD=ON
STARPU_NCPU=1 STARPU_NCUDA=2 STARPU_NWORKER_PER_CUDA=32 STARPU_SCHED=eager \
  build/starpu/bin/iyokan tfhe --cpu 64 ...
```

To use a different cuFHEpp checkout:

```sh
cmake -S . -B build -DTANGOR_CUFHEPP_SOURCE_DIR=/path/to/cuFHEpp
```

To override only TFHEpp:

```sh
cmake -S . -B build -DTANGOR_TFHEPP_SOURCE_DIR=/path/to/TFHEpp
```
