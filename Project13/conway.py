#!/bin/python

import argparse
import os
import subprocess
import sys

##### GLOBAL VARIABLES #####

compiler = "gcc"

compile_options = [
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-std=c23",
    "-march=native",
    "-O3",
    "-fopenmp"
]

version = "serial"

debug_level = 0;

N = 100
T = 100
wordfile = "RANDOM"
num_threads = 1
seed = 42

##### MAIN #####

def main():
    args = parse_args()

    cmd = [
        compiler,
        *compile_options,
        f"-DN={N}",
        f"-DT={T}",
        f"-DPRESET=\"{wordfile}\"",
        f"-DDEBUG={debug_level}",
        f"-DRANDSEED={seed}",
        f"-DNUM_THREADS={num_threads}"
    ]

    match version:
        case "serial": cmd.append("source/serial.c")
        case "omp": cmd.append("source/omp.c")

    print(cmd)

    subprocess.run(cmd, check=True)
    result = subprocess.run(["./a.out"], capture_output=True, check=True, text=True)
    os.remove("a.out")

    print(f"Time from py: {result.stdout}")
    


##### FUNCTIONS #####

def parse_args():
    global N
    global T
    global wordfile
    global debug_level
    global version
    global seed
    global num_threads

    parser = argparse.ArgumentParser(
        description="Test differet implementations of Conway's Game of Life"
    )

    # Optional arguments
    parser.add_argument(
        "-N", "--size",
        type=int,
        help="World size."
    )
    
    parser.add_argument(
        "-T", "--steps",
        type=int,
        help="Simulated steps"
    )
    
    parser.add_argument(
        "-V", "--version",
        type=str,
        help="Game implemetantion (serial, omp, mpi)."
    )
    
    parser.add_argument(
        "-P", "--preset",
        type=str,
        help="Preset file (RANDOM if not specified)."
    )

    parser.add_argument(
        "-D", "--debug",
        type=int,
        help="Debug level (0 if not specified, 2+ prints the game)."
    )

    parser.add_argument(
            "-C", "--threads",
            type=int,
            help="Number of Threads (default is 1)"
        )

    parser.add_argument(
            "-R", "--seed",
            type=int,
            help="Random seed"
        )

    args = parser.parse_args()

    # Set options to those specified
    if args.size    : N = args.size
    if args.steps   : T = args.steps
    if args.preset  : wordfile = args.preset
    if args.version : version = args.version
    if args.debug   : debug_level = args.debug
    if args.seed    : seed = args.seed
    if args.threads : num_threads = args.threads
    
    return args

if __name__ == "__main__":
    main()