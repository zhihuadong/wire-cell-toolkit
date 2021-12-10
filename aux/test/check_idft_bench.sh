#!/bin/bash

# A do all script for IDFT benchmark with all known IDFTs
# Note, this will almost certainly fail if a systen does not have:
# 
# - wire-cell-toolkit built at least under build/
# - wire-cell-python built and in the environment
# - run this script in-place in the source
# - host has at exactly one GPU
# - GPU has enough memory
#
# even if fails, it documents what to run

tstdir="$(dirname $(realpath $BASH_SOURCE))"
auxdir="$(dirname $tstdir)"
topdir="$(dirname $auxdir)"
blddir="$topdir/build"
cib="$blddir/aux/check_idft_bench"

torchcfg="$tstdir/test_idft_pytorch.jsonnet"

set -x
wirecell-aux run-idft-bench -o idft-bench-fftw-cpu.json $cib
wirecell-aux run-idft-bench -o idft-bench-torch-cpu.json -p WireCellPytorch -t TorchDFT $cib
wirecell-aux run-idft-bench -o idft-bench-torch-gpu.json -p WireCellPytorch -t TorchDFT -c $torchcfg $cib

wirecell-aux plot-idft-bench -o idft-bench.pdf \
             idft-bench-fftw-cpu.json  \
             idft-bench-torch-cpu.json  \
             idft-bench-torch-gpu.json
