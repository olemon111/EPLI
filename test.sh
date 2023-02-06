#!/usr/bin/env bash
mkdir -p build
cd build
rm -rf *
cmake -DSERVER:BOOL=ON .. # build on server
make -j16
cd ..

sudo mkdir -p /mnt/pmem1/lbl
sudo rm -rf /mnt/pmem1/lbl/*

# # microbench
chmod +x ./tests/run_microbench_epli.sh
sudo ./tests/run_microbench_epli.sh

# # benchmark for epl-Tree
# sudo ./build/benchmark_epli
# sudo gdb --args ./build/benchmark_epli