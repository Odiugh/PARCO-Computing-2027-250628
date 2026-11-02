#!/bin/python

import argparse
import os
import sys

compile_options = [
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-march=native",
    "-O3"
]

version = "serial"

N = 100

T = 100

def main():
    args = parse_args()

def parse_args():
    parser = argparse.ArgumentParser(
        description="Test differet implementations of Conway's Game of Life"
    )

    # Optional arguments
    parser.add_argument(
        "-N",
        type=int,
        help="World size."
    )
    
    # Changed short flag for height to -H to prevent collision with -h (--help)
    parser.add_argument(
        "-T",
        type=int,
        help="Simulated steps"
    )
    
    parser.add_argument(
        "-V", "--version",
        type=str,
        help="Game implemetantion (serial, omp, mpi)."
    )
    
    # Corrected spelling of directory in help text
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

    args = parser.parse_args()

    # Set options to those specified
    
    return args

if __name__ == "__main__":
    main()