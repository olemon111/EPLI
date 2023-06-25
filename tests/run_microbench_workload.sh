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
    theta=$8

    Loadname="${dataset}"

    # microbench_workload
    if [ $dataset == "ycsb" ]; then
        test_read_write $dbname $loadnum $opnum $scansize $thread $workloadtype 3 $theta # YCSB
    else
        if [ $dataset == "llt" ]; then
            test_read_write $dbname $loadnum $opnum $scansize $thread $workloadtype 4 $theta # LLT
        else
            if [ $dataset == "ltd" ]; then
                test_read_write $dbname $loadnum $opnum $scansize $thread $workloadtype 5 $theta # LTD
            else
                test_read_write $dbname $loadnum $opnum $scansize $thread $workloadtype 6 $theta # LGN
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
    theta=$8

    dataset=$Loadname

    # Read and Write
    rm -f /mnt/pmem1/lbl/*
    Loadname="${Loadname}-${workloadtype}"
    date | tee output/workload/${dataset}/${dbname}-${Loadname}-${theta}.txt
    # gdb --args \
    numactl --cpubind=1 --membind=1 ${BUILDDIR}/microbench_workload --dbname ${dbname} --load-size ${loadnum} \
    --put-size ${opnum} --get-size ${opnum} \
    --loadstype ${loadstype} --workloadtype ${workloadtype} --theta ${theta} -t $thread | tee -a output/workload/${dataset}/${dbname}-${Loadname}-${theta}.txt
    # rm -f /mnt/pmem1/lbl/*

    echo numactl --cpubind=1 --membind=1 "${BUILDDIR}/microbench_workload --dbname ${dbname} --load-size ${loadnum} "\
    "--put-size ${opnum} --get-size ${opnum} --loadstype ${loadstype} --workloadtype ${workloadtype} --theta ${theta} -t $thread"
}

function run_all() {
    # dbs="epli"
    # dbs="apex"
    # dbs="lbtree fastfair"
    dbs="epli apex lbtree fastfair"
    for dbname in $dbs; do
        echo "Run: " $dbname
        Run $dbname $1 $2 $3 $4 $5 $6 $7
        sleep 10
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
    theta=0

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
    if [ $# -ge 8 ]; then
        theta=$8
    fi

    if [ $dbname == "all" ]; then
        run_all $loadnum $opnum $scansize $thread $workloadtype $dataset $theta
    else
        echo "Run $dbname $loadnum $opnum $scansize $thread $workloadtype $dataset $theta"
        Run $dbname $loadnum $opnum $scansize $thread $workloadtype $dataset $theta
    fi
}

# # # # # YCSB
# # z means zipfian
# # # main all 400000000 10000000 0 1 rw ycsb
# # # main all 400000000 10000000 0 1 rhwh ycsb
# main all 400000000 10000000 0 1 whz ycsb 0.8
# main all 400000000 10000000 0 1 whz ycsb 0.5
# # # # # # LLT
# # # # main all 400000000 10000000 0 1 rw llt
# # # # main all 400000000 10000000 0 1 rhwh llt
# main epli 400000000 10000000 0 1 whz llt 0.8
# sleep 60
# main epli 400000000 10000000 0 1 whz llt 0.5
# sleep 30
# main fastfair 400000000 10000000 0 1 whz llt 0.8
# sleep 30
# main all 400000000 10000000 0 1 whz llt 0.5
# # # # # # LTD
# # # # main all 230000000 10000000 0 1 rw ltd
# # # # main all 230000000 10000000 0 1 rhwh ltd
# main all 230000000 10000000 0 1 whz ltd 0.8
# main all 230000000 10000000 0 1 whz ltd 0.5
# # # # # # LGN
# # # # main all 130000000 10000000 0 1 rw lgn
# # # # main all 130000000 10000000 0 1 rhwh lgn
# main all 130000000 10000000 0 1 whz lgn 0.8
# main all 130000000 10000000 0 1 whz lgn 0.5
# # # # remain 10million for insert

# # EPLI only (open bitmap in writing test)
# # # YCSB
# main epli 400000000 10000000 0 1 rrh ycsb
# sleep 60
# main epli 400000000 10000000 0 1 wh ycsb
# sleep 60
# main epli 400000000 10000000 0 1 w ycsb
# sleep 60

# main utree 400000000 10000000 0 1 rrh llt
# main utree 400000000 10000000 0 1 wh llt

# main epli 400000000 10000000 0 1 rrh llt
# sleep 60
# main epli 400000000 10000000 0 1 wh llt
# sleep 60
# main epli 400000000 10000000 0 1 w llt
# sleep 60

# # # LTD
# main epli 230000000 10000000 0 1 rrh ltd
# sleep 60
# main epli 230000000 10000000 0 1 wh ltd
# sleep 30
# main epli 230000000 10000000 0 1 w ltd
# sleep 30

# # # LGN
# main epli 130000000 10000000 0 1 rrh lgn
# sleep 30
# main epli 130000000 10000000 0 1 wh lgn
# sleep 30
# main epli 130000000 10000000 0 1 w lgn

# # # Test Dynamic workload
# main epli 400000000 10000000 0 1 d llt 0.99

# # Test Skew workload
# main all 16000000 128000000 0 1 rz llt 0.99
# main all 16000000 128000000 0 1 rz llt 0.9                 
# main all 16000000 128000000 0 1 rz llt 0.8
# main all 16000000 128000000 0 1 rz llt 0.7
# main all 16000000 128000000 0 1 rz llt 0.6
# main all 16000000 128000000 0 1 r llt
