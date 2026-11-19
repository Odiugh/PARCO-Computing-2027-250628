#!/usr/bin/env python3

import argparse
import glob
import json
import os
import re
import statistics
import subprocess
import sys

##### GLOBAL VARIABLES #####

SOURCE_DIR = "source"
EXECUTABLE = "a.out"
TARGETS = ("best", "average", "median")
JSON_KEYS = {
    "size", "steps", "versions", "preset", "debug", "threads", "seed",
    "target", "runs",
    "compiler", "compile_options",   # JSON only, no CLI flag
}

# Compiler and version
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

versions = ["serial"]

# Constants
debug_level = 0;
N = 100
T = 100
worldfile = "RANDOM"
num_threads = 1
seed = 42

# Benchmarking variables
target = ["best", "average", "median"]
runs = 1


##### MAIN #####

def main():
    args = parse_args()

    results = {}   # version -> {metric: seconds}
    for v in versions:
        print(f"\n=== {v} ===")
        compile_version(v)
        results[v] = measure(v)

    print_summary(results)
    


##### FUNCTIONS #####


def source_path(name):
    return os.path.join(SOURCE_DIR, f"{name}.c")

def compile_version(name):
    cmd = [
        compiler,
        *compile_options,
        f"-DN={N}",
        f"-DT={T}",
        f"-DPRESET=\"{worldfile}\"",
        f"-DDEBUG={debug_level}",
        f"-DRANDSEED={seed}",
        f"-DNUM_THREADS={num_threads}",
        source_path(name),
        "-o", EXECUTABLE,
    ]

    print(cmd)
    subprocess.run(cmd, check=True)

def measure(name):
    times = []

    try:
        for i in range(runs):
            result = subprocess.run(
                [f"./{EXECUTABLE}"], capture_output=True, check=True, text=True
            )

            # Print stdout to check DEBUG>=1 output
            if debug_level > 0:
                print(result.stdout)

            # The time is the last line printed (the world is printed before it in debug mode)
            lines = [l for l in result.stdout.splitlines() if l.strip()]
            try:
                times.append(float(lines[-1]))
            except (IndexError, ValueError):
                sys.exit(f"{name}, run {i + 1}: cannot read a time from the program output")

            print(f"Run {i + 1}/{runs}: {times[-1]:.6f} s")

    finally:
        if os.path.exists(EXECUTABLE):
            os.remove(EXECUTABLE)

    return {
        "best":    min(times),
        "average": statistics.mean(times),
        "median":  statistics.median(times),
    }

def print_summary(results):
    baseline = versions[0]
    width = max(len(v) for v in versions + ["version"])

    print(f"\n##### SUMMARY (N={N}, T={T}, threads={num_threads}, runs={runs}) #####")
    for name in target:
        print(f"\n{name}  (speedup vs {baseline})")
        print(f"{'version':<{width}}  {'time (s)':>12}  {'speedup':>8}")
        base_time = results[baseline][name]
        for v in versions:
            t = results[v][name]
            speedup = f"{base_time / t:.2f}x" if t > 0 else "n/a"
            print(f"{v:<{width}}  {t:>12.6f}  {speedup:>8}")

def check_versions(names):
    available = sorted(
        os.path.basename(p)[:-2] for p in glob.glob(os.path.join(SOURCE_DIR, "*.c"))
    )

    for name in names:
        # The name goes into a file path, so only allow plain names (no "../", no "/")
        if not isinstance(name, str) or not re.fullmatch(r"\w+", name):
            sys.exit(f"Invalid version {name!r}: use a plain name like 'serial'")

        if not os.path.isfile(source_path(name)):
            sys.exit(f"Version '{name}': {source_path(name)} not found. "
                     f"Available: {', '.join(available) or 'none'}")

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

    # "versions": a list of version names (a single string is accepted too)
    if "versions" in cfg:
        v = cfg["versions"]
        if isinstance(v, str):
            v = [v]
        if (not isinstance(v, list) or not v
                or any(not isinstance(x, str) for x in v)):
            sys.exit(f"{path}: 'versions' must be a non-empty list of version names")
        cfg["versions"] = v

    # "target": a list of metrics (a single string is accepted too)
    if "target" in cfg:
        t = cfg["target"]
        if isinstance(t, str):
            t = [t]
        if (not isinstance(t, list) or not t
                or any(x not in TARGETS for x in t)):
            sys.exit(f"{path}: 'target' must be a non-empty list of {list(TARGETS)}")
        cfg["target"] = list(dict.fromkeys(t))   # drop duplicates, keep order

    # "runs": positive integer (bool is an int subclass in Python, so exclude it)
    if "runs" in cfg:
        r = cfg["runs"]
        if isinstance(r, bool) or not isinstance(r, int) or r < 1:
            sys.exit(f"{path}: 'runs' must be an integer >= 1")

    return cfg

def parse_args():
    global N, T, worldfile, debug_level, versions, seed, num_threads
    global target, runs
    global compiler, compile_options

    parser = argparse.ArgumentParser(
        description="Test different implementations of Conway's Game of Life"
    )

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
        "-V", "--versions",
        nargs="+",
        metavar="VERSION",
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

    parser.add_argument(
        "-n", "--runs",
        type=int,
        help="Number of timed runs (default is 1)."
    )

    parser.add_argument("-B", "--target",
        nargs="+",
        choices=TARGETS, metavar="METRIC",
        help=f"Metrics to report, any of: {', '.join(TARGETS)} (default is all)."
    )

    args = parser.parse_args()

    if args.runs is not None and args.runs < 1:
        parser.error("--runs must be >= 1")

    cfg = load_json(args.json) if args.json else {}

    # Load arguments from json
    def pick(cli, key, current):
        return cli if cli is not None else cfg.get(key, current)

    # Override json args with cli args
    N           = pick(args.size,    "size",    N)
    T           = pick(args.steps,   "steps",   T)
    worldfile   = pick(args.preset,  "preset",  worldfile)
    versions    = pick(args.versions, "versions", versions)
    debug_level = pick(args.debug,   "debug",   debug_level)
    seed        = pick(args.seed,    "seed",    seed)
    num_threads = pick(args.threads, "threads", num_threads)
    runs        = pick(args.runs,    "runs",    runs)
    target      = list(dict.fromkeys(pick(args.target, "target", target)))

    # JSON-only options
    compiler        = cfg.get("compiler", compiler)
    compile_options = cfg.get("compile_options", compile_options)

    versions = list(dict.fromkeys(versions))
    check_versions(versions)
    
    return args


if __name__ == "__main__":
    main()