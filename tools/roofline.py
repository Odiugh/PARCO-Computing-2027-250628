#!/bin/python

import multiprocessing
import psutil
import subprocess
import re
import sys

def get_simd_width():
    try:
        # Read CPU info from the kernel
        cpu_info = subprocess.check_output("lscpu", shell=True).decode()
        if "avx512" in cpu_info.lower():
            return 512
        elif "avx" in cpu_info.lower():
            return 256
        elif "sse" in cpu_info.lower() or "neon" in cpu_info.lower():
            return 128
    except Exception:
        pass
    return 64

# ----- PEAK FLOPS -----

cpu_count = multiprocessing.cpu_count() # cores

cpu_max_freq = psutil.cpu_freq().max / 10**3 # GHz

simd_width = get_simd_width()/32 # simd bit width in 32 bit floats

fma_units = 2

peak_gflops = cpu_max_freq * cpu_count * simd_width * fma_units

# ----- Bandwidth Peak ----- 

output = subprocess.check_output(
    ["lshw", "-C", "memory"], stderr=subprocess.DEVNULL
).decode()

clock_matches = re.findall(r"clock:\s*(\d+)\s*MHz", output, re.IGNORECASE)

if clock_matches:
    speeds = [int(m) for m in clock_matches if int(m) > 400]
    if speeds:
        speed_mts = max(speeds)
        channels = len(speeds)

        print(speed_mts)
        print(channels)




print(peak_gflops)
