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

    Loadname="${dataset}"

    # microbench_epli
    if [ $rw == "r" ]; then
        test_read $dbname $loadnum $opnum $scansize $thread $reverse
    else
        if [ $rw == "w" ]; then
            test_write $dbname $loadnum $opnum $scansize $thread $reverse
        else
            if [ $dataset == "ycsb" ]; then
                test_read_write $dbname $loadnum $opnum $scansize $thread $reverse 3 # YCSB
            else
                if [ $dataset == "llt" ]; then
                    test_read_write $dbname $loadnum $opnum $scansize $thread $reverse 4 # LLT
                else
                    if [ $dataset == "ltd" ]; then
                        test_read_write $dbname $loadnum $opnum $scansize $thread $reverse 5 # LTD
                    else
                        test_read_write $dbname $loadnum $opnum $scansize $thread $reverse 6 # LGN
                    fi
                fi
            fi
        fi
    fi
}

function test_write() {
    dbname=$1
    loadnum=$2 
    opnum=$3
    scansize=$4
    thread=$5
    reverse=$6

    # Write
    # rm -f /mnt/pmem1/lbl/*
    Loadname="${Loadname}-w"
    date | tee output/${dbname}-${Loadname}.txt
    # gdb --args \
    numactl --cpubind=1 --membind=1 ${BUILDDIR}/microbench_epli --dbname ${dbname} --load-size ${loadnum} \
    --put-size ${opnum} --get-size 0 \
    --loadstype 3 --reverse ${reverse} -t $thread | tee -a output/${dbname}-${Loadname}.txt
    # rm -f /mnt/pmem1/lbl/*

    echo numactl --cpubind=1 --membind=1 "${BUILDDIR}/microbench_epli --dbname ${dbname} --load-size ${loadnum} "\
    "--put-size ${opnum} --get-size 0 --loadstype 3 --reverse ${reverse} -t $thread"
}

function test_read() {
    dbname=$1
    loadnum=$2 
    opnum=$3
    scansize=$4
    thread=$5
    reverse=$6

    # Read
    # rm -f /mnt/pmem1/lbl/*
    Loadname="${Loadname}-r"
    date | tee output/${dbname}-${Loadname}.txt
    # gdb --args \
    numactl --cpubind=1 --membind=1 ${BUILDDIR}/microbench_epli --dbname ${dbname} --load-size ${loadnum} \
    --put-size 0 --get-size ${opnum} \
    --loadstype 3 --reverse ${reverse} -t $thread | tee -a output/${dbname}-${Loadname}.txt
    # rm -f /mnt/pmem1/lbl/*

    echo numactl --cpubind=1 --membind=1 "${BUILDDIR}/microbench_epli --dbname ${dbname} --load-size ${loadnum} "\
    "--put-size 0 --get-size ${opnum} --loadstype 3 --reverse ${reverse} -t $thread"
}

function test_read_write() {
    dbname=$1
    loadnum=$2 
    opnum=$3
    scansize=$4
    thread=$5
    reverse=$6
    loadstype=$7

    # Read and Write
    # rm -f /mnt/pmem1/lbl/*
    Loadname="${Loadname}-rw"
    date | tee output/${dbname}-${Loadname}.txt
    # gdb --args \
    numactl --cpubind=1 --membind=1 ${BUILDDIR}/microbench_epli --dbname ${dbname} --load-size ${loadnum} \
    --put-size ${opnum} --get-size ${opnum} \
    --loadstype ${loadstype} --reverse ${reverse} -t $thread | tee -a output/${dbname}-${Loadname}.txt
    # rm -f /mnt/pmem1/lbl/*

    echo numactl --cpubind=1 --membind=1 "${BUILDDIR}/microbench_epli --dbname ${dbname} --load-size ${loadnum} "\
    "--put-size ${opnum} --get-size ${opnum} --loadstype ${loadstype} --reverse ${reverse} -t $thread"
}

function run_all() {
    dbs="epli"
    # dbs="fastfair"
    # dbs="lbtree fastfair"
    for dbname in $dbs; do
    
        echo "Run: " $dbname
        Run $dbname $1 $2 $3 $4 $5 $6 $7
        sleep 100
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

# main apex 400000000 10000000 0 1 0 r ycsb
# main epli 400000000 10000000 0 1 0 w ycsb
# main epli 400000000 10000000 0 1 0 a llt
# main lbtree 400000000 10000000 0 1 0 r ycsb
# main apex 400000000 10000000 0 1 0 a
# main fastfair 400000000 10000000 0 1 0 a ycsb

# # Test Operation
main all 400000000 10000000 0 1 0 o llt