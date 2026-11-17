#!/bin/bash
# Exercise 2: try different cutoffs with 1, 2, 4, 8 threads.
# Results are also saved in ex2_results.txt
# On a small machine use fewer edges:  M=5000000 ./run_ex2.sh

M=${M:-20000000}

for P in 1 2 4 8; do
    for C in 16 1024 65536 1000000 $M; do
        ./ex2_msort $M $P $C | head -3
        echo
    done
done | tee ex2_results.txt
