#!/usr/bin/env bash
mkdir -p build
cd build
rm -rf *
cmake -DSERVER:BOOL=ON .. # build on server
make -j16
cd ..

sudo mkdir -p /mnt/pmem1/lbl
sudo rm -rf /mnt/pmem1/lbl/*

# # benchmark
# sudo ./build/benchmark_epli
# sudo gdb --args ./build/benchmark_epli

# # microbench
# chmod +x ./tests/run_microbench_epli.sh
# sudo ./tests/run_microbench_epli.sh

# microbench workload
chmod +x ./tests/run_microbench_workload.sh
sudo ./tests/run_microbench_workload.sh

# # microbench scalability
# chmod +x ./tests/run_microbench_scalability.sh
# sudo ./tests/run_microbench_scalability.sh

# # # microbench dram usage
# chmod +x ./tests/run_microbench_dram_usage.sh
# sudo ./tests/run_microbench_dram_usage.sh

# # # microbench recovery
# chmod +x ./tests/run_microbench_recovery.sh
# sudo ./tests/run_microbench_recovery.sh

# # multi_bench
# chmod +x ./tests/run_multibench.sh
# sudo ./tests/run_multibench.sh