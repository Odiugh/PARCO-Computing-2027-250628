#!/bin/python

import argparse
import os
import subprocess
import sys
import json

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
    global N, T, wordfile, debug_level, version, seed, num_threads
    global compiler, compile_options

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
        metavar="FILE",
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

    parser.add_argument(
        "-J", "--json",
        type=str,
        metavar="FILE",
        help="Load arguments from a JSON file (command line options override it)."
    )

    args = parser.parse_args()

    cfg = load_json(args.json) if args.json else {}

    def pick(cli, key, current):
        return cli if cli is not None else cfg.get(key, current)

    N           = pick(args.size,    "size",    N)
    T           = pick(args.steps,   "steps",   T)
    wordfile    = pick(args.preset,  "preset",  wordfile)
    version     = pick(args.version, "version", version)
    debug_level = pick(args.debug,   "debug",   debug_level)
    seed        = pick(args.seed,    "seed",    seed)
    num_threads = pick(args.threads, "threads", num_threads)

    # JSON-only options
    compiler        = cfg.get("compiler", compiler)
    compile_options = cfg.get("compile_options", compile_options)
    
    return args

JSON_KEYS = {
    "size", "steps", "version", "preset", "debug", "threads", "seed",
    "compiler", "compile_options"
}

def load_json(path):
    try:
        with open(path) as f:
            cfg = json.load(f)
    except (OSError, json.JSONDecodeError) as e:
        sys.exit(f"Cannot read {path}: {e}")

    if not isinstance(cfg, dict):
        sys.exit(f"{path}: top level must be a JSON object")

    unknown = set(cfg) - JSON_KEYS
    if unknown:
        sys.exit(f"{path}: unknown keys {sorted(unknown)}; allowed: {sorted(JSON_KEYS)}")

    return cfg

if __name__ == "__main__":
    main()