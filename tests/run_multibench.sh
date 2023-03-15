#!/bin/bash
BUILDDIR=$(dirname "$0")/../build
Loadname="llt"
function Run() {
    dbname=$1
    loadnum=$2
    opnum=$3
    scansize=$4
    thread=$5
    workloadtype=$6

    if [ $workloadtype == "r" ]; then
        test_read $dbname $loadnum $opnum $scansize $thread
    else
        test_write $dbname $loadnum $opnum $scansize $thread
    fi
}

function test_read() {
    dbname=$1
    loadnum=$2
    opnum=$3
    scansize=$4
    thread=$5

    rm -f /mnt/pmem1/lbl/*
    date | tee output/multi_thread/multi-${dbname}-${Loadname}-th${thread}-${workloadtype}.txt
    # gdb --args \
    numactl --cpubind=1 --membind=1 ${BUILDDIR}/multi_bench --dbname ${dbname} \
    --loadstype 4 --load-size ${loadnum} --put-size 0 --get-size ${opnum} --workloadtype ${workloadtype} \
        -t $thread | tee -a output/multi_thread/multi-${dbname}-${Loadname}-th${thread}-${workloadtype}.txt

    echo numactl --cpubind=1 --membind=1 ${BUILDDIR}/multi_bench --dbname ${dbname} \
    --loadstype 4 --load-size ${loadnum} --put-size 0 --get-size ${opnum} --workloadtype ${workloadtype} \
        -t $thread | tee -a output/multi_thread/multi-${dbname}-${Loadname}-th${thread}-${workloadtype}.txt
}

function test_write() {
    dbname=$1
    loadnum=$2
    opnum=$3
    scansize=$4
    thread=$5

    rm -f /mnt/pmem1/lbl/*
    date | tee output/multi_thread/multi-${dbname}-${Loadname}-th${thread}-${workloadtype}.txt
    # gdb --args \
    numactl --cpubind=1 --membind=1 ${BUILDDIR}/multi_bench --dbname ${dbname} \
    --loadstype 4 --load-size ${loadnum} --put-size ${opnum} --get-size ${opnum} --workloadtype ${workloadtype} \
        -t $thread | tee -a output/multi_thread/multi-${dbname}-${Loadname}-th${thread}-${workloadtype}.txt

    echo numactl --cpubind=1 --membind=1 ${BUILDDIR}/multi_bench --dbname ${dbname} \
    --loadstype 4 --load-size ${loadnum} --put-size ${opnum} --get-size ${opnum} --workloadtype ${workloadtype} \
        -t $thread | tee -a output/multi_thread/multi-${dbname}-${Loadname}-th${thread}-${workloadtype}.txt

    # numactl --cpubind=1 --membind=1 ${BUILDDIR}/multi_bench --dbname ${dbname} \
    # --loadstype 4 --load-size ${loadnum} --put-size ${opnum} --get-size 0 --workloadtype ${workloadtype} \
    #     -t $thread | tee -a output/multi_thread/multi-${dbname}-${Loadname}-th${thread}-${workloadtype}.txt

    # echo numactl --cpubind=1 --membind=1 ${BUILDDIR}/multi_bench --dbname ${dbname} \
    # --loadstype 4 --load-size ${loadnum} --put-size ${opnum} --get-size 0 --workloadtype ${workloadtype} \
    #     -t $thread | tee -a output/multi_thread/multi-${dbname}-${Loadname}-th${thread}-${workloadtype}.txt
}

function Run_all() {
    dbs="apex"
    # dbs="apex epli lbtree fastfair"
    for dbname in $dbs; do
        echo "Run: " $dbname
        Run $dbname $1 $2 $3 $4 $5
        sleep 100
    done
}

# # read
dbname="all"
loadnum=400000000
opnum=10000000
scansize=0
workloadtype="r"

dbname="epli"


for thread in 16
# for thread in {2..16}
# for thread in  1, 2, 4, 8, 16
do
    # if [ $dbname == "all" ]; then
        # Run_all $loadnum $opnum $scansize $thread $workloadtype
    # else
    Run $dbname $loadnum $opnum $scansize $thread $workloadtype
    # fi
    sleep 100
done