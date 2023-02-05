#!/usr/bin/env bash
mkdir -p build
cd build
rm -rf *
cmake -DSERVER:BOOL=ON .. # build on server
make -j16
cd ..
chmod +x ./tests/run_microbench_epl.sh