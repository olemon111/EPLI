#!/bin/bash
BUILDDIR=$(dirname "$0")/../build
Loadname="ycsb"

function Run() {
    dbname=$1
    loadnum=$2 
    opnum=$3
    scansize=$4
    thread=$5
    workloadtype=$6
    dataset=$7

    Loadname="${dataset}"

    # microbench_workload
    if [ $dataset == "ycsb" ]; then
        test_read_write $dbname $loadnum $opnum $scansize $thread $workloadtype 3 # YCSB
    else
        if [ $dataset == "llt" ]; then
            test_read_write $dbname $loadnum $opnum $scansize $thread $workloadtype 4 # LLT
        else
            if [ $dataset == "ltd" ]; then
                test_read_write $dbname $loadnum $opnum $scansize $thread $workloadtype 5 # LTD
            else
                test_read_write $dbname $loadnum $opnum $scansize $thread $workloadtype 6 # LGN
            fi
        fi
    fi
}

function test_read_write() {
    dbname=$1
    loadnum=$2 
    opnum=$3
    scansize=$4
    thread=$5
    workloadtype=$6
    loadstype=$7

    dataset=$Loadname

    # Read and Write
    rm -f /mnt/pmem1/lbl/*
    Loadname="${Loadname}-${workloadtype}"
    date | tee output/workload/${dataset}/${dbname}-${Loadname}.txt
    # gdb --args \
    numactl --cpubind=1 --membind=1 ${BUILDDIR}/microbench_workload --dbname ${dbname} --load-size ${loadnum} \
    --put-size ${opnum} --get-size ${opnum} \
    --loadstype ${loadstype} --workloadtype ${workloadtype} -t $thread | tee -a output/workload/${dataset}/${dbname}-${Loadname}.txt
    # rm -f /mnt/pmem1/lbl/*

    echo numactl --cpubind=1 --membind=1 "${BUILDDIR}/microbench_workload --dbname ${dbname} --load-size ${loadnum} "\
    "--put-size ${opnum} --get-size ${opnum} --loadstype ${loadstype} --workloadtype ${workloadtype} -t $thread"
}

function run_all() {
    dbs="epli apex lbtree fastfair"
    for dbname in $dbs; do
        echo "Run: " $dbname
        Run $dbname $1 $2 $3 $4 $5 $6
        sleep 60
    done
}

function main() {
    dbname="epli"
    loadnum=2000000
    opnum=10000000
    scansize=0
    thread=1
    workloadtype="r"
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
        workloadtype=$6
    fi
    if [ $# -ge 7 ]; then
        dataset=$7
    fi

    if [ $dbname == "all" ]; then
        run_all $loadnum $opnum $scansize $thread $workloadtype $dataset
    else
        echo "Run $dbname $loadnum $opnum $scansize $thread $workloadtype $dataset"
        Run $dbname $loadnum $opnum $scansize $thread $workloadtype $dataset
    fi
}

# # YCSB
# main all 400000000 10000000 0 1 rw ycsb
# main all 400000000 10000000 0 1 rhwh ycsb
# # LLT
# main all 400000000 10000000 0 1 rw llt
# main all 400000000 10000000 0 1 rhwh llt
# # LTD
# main all 320000000 10000000 0 1 rw ltd
# main all 320000000 10000000 0 1 rhwh ltd
# # LGN
# main all 170000000 10000000 0 1 rw lgn
# main all 170000000 10000000 0 1 rhwh lgn
# # remain 10million for insert

main epli 400000000 10000000 0 1 rw llt
# main epli 400000000 10000000 0 1 rhwh llt
sleep 30
main apex 400000000 10000000 0 1 rw llt