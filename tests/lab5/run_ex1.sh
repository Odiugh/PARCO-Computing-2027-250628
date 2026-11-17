#!/bin/bash
# Exercise 1: run with 1, 2, 4, 8 threads, first with close and then with spread binding.
# Results are also saved in ex1_results.txt
# On a small machine use a smaller matrix:  N=4096 ./run_ex1.sh

N=${N:-16384}
export OMP_PLACES=cores

for BIND in close spread; do
    for P in 1 2 4 8 16 32 64; do
        OMP_PROC_BIND=$BIND ./ex1_matvec $N $P
        echo
    done
done | tee ex1_results.txt
