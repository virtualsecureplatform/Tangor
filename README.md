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

To use a different cuFHEpp checkout:

```sh
cmake -S . -B build -DTANGOR_CUFHEPP_SOURCE_DIR=/path/to/cuFHEpp
```

To override only TFHEpp:

```sh
cmake -S . -B build -DTANGOR_TFHEPP_SOURCE_DIR=/path/to/TFHEpp
```
