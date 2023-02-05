#!/usr/bin/env bash
mkdir -p build
cd build
rm -rf *
cmake -DSERVER:BOOL=ON .. # build on server
make -j16
cd ..
# # microbench
# chmod +x ./tests/run_microbench_epl.sh
# sudo ./tests/run_microbench_epl.sh

# # benchmark for epl-Tree
sudo mkdir -p /mnt/pmem1/lbl
sudo rm -rf /mnt/pmem1/lbl/*
# sudo gdb --args ./build/benchmark_epl
sudo ./build/benchmark_epl
