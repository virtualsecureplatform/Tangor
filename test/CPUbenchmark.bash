#!/bin/bash
numtest=10
for circuit in "c17" "c432" "c499" "c880" "c1355" "c1908" "c2670" "c3540" "c5315" "c7552"; do
    cd $circuit
    for ((i=0; i < $numtest; i++)); do
        STARPU_PERF_MODEL_HOMOGENEOUS_CPU=0 STARPU_HOSTNAME=nishino-cpu STARPU_SCHED=dmda bash test.bash $1
    done
    mv CPU_runtime.txt ../../../benchmark/CPU_${circuit}.txt
    cd ..
done
