project(EPLI)
cmake_minimum_required(VERSION 3.0)

set(MKL_LINK_DIRECTORY "/opt/intel/mkl/lib/intel64")
set(MKL_INCLUDE_DIRECTORY "/opt/intel/mkl/include")

# use `cmake -DSERVER:BOOL=ON ..` when running in server
option(SERVER "Run in server" ON)
option(USE_LIBPMEM "libpmem or libvmmalloc" ON)
option(BUF_SORT "Sort buffer in KVBufer" OFF)
option(STREAMING_LOAD "Use Non-temporal Load" OFF)
option(STREAMING_STORE "Use Non-temporal Store" OFF)
option(NO_LOCK "Don't use lock" OFF)
option(BRANGE "Multi-thread expanding" ON)
option(NO_ENTRY_BUF "BEntry without KVBuffer" ON)

# EPL-Tree Configuration
# use `make clean && make CXX_DEFINES="-DNAME=VALUE"` to override during compile
if(SERVER)
  set(COMMON_PMEM_FILE \"/mnt/pmem1/lbl/common-alloc\")
  set(PMEM_DIR \"/mnt/pmem1/lbl/\")
else()
  set(COMMON_PMEM_FILE \"/mnt/pmem1/lbl/common-alloc\")
  set(PMEM_DIR \"/mnt/pmem1/lbl/\")
endif(SERVER)

if(BRANGE)
  set(EXPAND_THREADS 4)
endif(BRANGE)

if(NO_ENTRY_BUF)
  set(BLEVEL_EXPAND_BUF_KEY 4)
else()
  set(BLEVEL_EXPAND_BUF_KEY 6)
endif(NO_ENTRY_BUF)

set(EXPANSION_FACTOR 2)
set(DEFAULT_SPAN 2)
set(PMEMKV_THRESHOLD 10000)
set(ENTRY_SIZE_FACTOR 1.2)

configure_file(
  "${PROJECT_SOURCE_DIR}/src/epltree_config.h.in"
  "${PROJECT_SOURCE_DIR}/src/epltree_config.h"
)

# -march=native used to detect CPU flags and use corrosponding clflush instruction
set(CMAKE_CXX_FLAGS "-march=native -Wall -Wno-reorder -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mrtm -msse4.1 -mavx2")

# Require C++17
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_STANDARD 17)

FIND_PACKAGE(OpenMP REQUIRED)

if(OPENMP_FOUND)
  message("OPENMP FOUND")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

add_definitions(-DNDEBUG -DNDEBUGGING)
add_compile_definitions(CORES_PER_SOCKET=16) # for clht

# MK for RMI
link_directories(${MKL_LINK_DIRECTORY})
include_directories(${MKL_INCLUDE_DIRECTORY})

# EPL-Tree shared library
include_directories(include)

set(EPLI_SRC
  src/epltree.h
  src/enviroment.cc
  src/clht_gc.cc
  src/clht_lb_res.cc
  src/measurements.cc
  src/ssmem.cc
)

add_library(epli SHARED ${EPLI_SRC})
target_link_libraries(epli pmem pmemobj mkl_rt pthread)

# benchmark_epl
add_executable(benchmark_epl tests/benchmark_epl.cc)
target_link_libraries(benchmark_epl epli)
add_test(benchmark_epl benchmark_epl)

# benchmark_epli
add_executable(benchmark_epli tests/benchmark_epli.cc)
target_link_libraries(benchmark_epli epli)
add_test(benchmark_epli benchmark_epli)

# Unit Test
enable_testing()

include_directories(src)

# microbench_epl
add_executable(microbench_epli tests/microbench_epli.cc)
target_link_libraries(microbench_epli epli)
add_test(microbench_epli microbench_epli)
