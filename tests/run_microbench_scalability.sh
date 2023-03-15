#!/bin/bash
BUILDDIR=$(dirname "$0")/../build
Loadname="ycsb"

function Run() {
    dbname=$1
    loadnum=$2 
    opnum=$3
    scansize=$4
    thread=$5
    reverse=$6
    rw=$7
    dataset=$8

    Loadname=${dataset}

    # microbench_epli
    if [ $dataset == "ycsb" ]; then
        test_scalability $dbname $loadnum $opnum $scansize $thread $reverse 3 # YCSB
    else
        if [ $dataset == "llt" ]; then
            test_scalability $dbname $loadnum $opnum $scansize $thread $reverse 4 # LLT
        else
            if [ $dataset == "ltd" ]; then
                test_scalability $dbname $loadnum $opnum $scansize $thread $reverse 5 # LTD
            else
                test_scalability $dbname $loadnum $opnum $scansize $thread $reverse 6 # LGN
            fi
        fi
    fi
}


function test_scalability() {
    dbname=$1
    loadnum=$2 
    opnum=$3
    scansize=$4
    thread=$5
    reverse=$6
    loadstype=$7

    # Read and Write
    rm -f /mnt/pmem1/lbl/*
    Loadname="${Loadname}"
    date | tee output/scalability/${dbname}-${Loadname}-${thread}.txt
    # gdb --args \
    numactl --cpubind=1 --membind=1 ${BUILDDIR}/microbench_epli --dbname ${dbname} --load-size ${loadnum} \
    --put-size ${opnum} --get-size ${opnum} \
    --loadstype ${loadstype} --reverse ${reverse} -t $thread | tee -a output/scalability/${dbname}-${Loadname}-${thread}.txt
    # rm -f /mnt/pmem1/lbl/*

    echo numactl --cpubind=1 --membind=1 "${BUILDDIR}/microbench_epli --dbname ${dbname} --load-size ${loadnum} "\
    "--put-size ${opnum} --get-size ${opnum} --loadstype ${loadstype} --reverse ${reverse} -t $thread"
}

function run_all() {
    dbs="epli apex lbtree fastfair"
    for dbname in $dbs; do
        echo "Run: " $dbname
        Run $dbname $1 $2 $3 $4 $5 $6 $7
        sleep 60
    done
}

function main() {
    dbname="epli"
    loadnum=2000000
    opnum=10000000
    scansize=0
    thread=1
    reverse=0
    rw="r"
    dataset="ycsb"

    if [ $# -ge 1 ]; then
        dbname=$1
    fi
    if [ $# -ge 2 ]; then
        loadnum=$2
    fi
    if [ $# -ge 3 ]; then
        opnum=$3
    fi
    if [ $# -ge 4 ]; then
        scansize=$4
    fi
    if [ $# -ge 5 ]; then
        thread=$5
    fi
    if [ $# -ge 6 ]; then
        reverse=$6
    fi
    if [ $# -ge 7 ]; then
        rw=$7
    fi
    if [ $# -ge 8 ]; then
        dataset=$8
    fi

    if [ $dbname == "all" ]; then
        run_all $loadnum $opnum $scansize $thread $reverse $rw $dataset
    else
        echo "Run $dbname $loadnum $opnum $scansize $thread $reverse $rw $dataset"
        Run $dbname $loadnum $opnum $scansize $thread $reverse $rw $dataset
    fi
}

# # Test Scalability
# # open scalability in microbench_epli.cc first
# main fastfair 400000000 0 0 1 0 r llt
# main epli 400000000 0 0 1 0 r ycsb
# main epli 400000000 0 0 1 0 r llt
# main apex 400000000 0 0 1 0 r llt
# main apex 400000000 0 0 1 0 r ycsb
# main all 400000000 0 0 1 0 r ycsb
# main epli 330000000 0 0 1 0 r ltd

# # multi thread
for thread in {3..16}
do
    main apex 400000000 10000000 0 $thread 0 r llt
    sleep 100
done