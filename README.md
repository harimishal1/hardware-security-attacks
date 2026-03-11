# Hardware Side-Channel Attacks

Implementations of classical microarchitectural side-channel attacks, 
written from scratch as part of the Hardware Security course at 
Vrije Universiteit Amsterdam (VUSec).

## Attacks Implemented

### Flush+Reload
A cache-based side-channel attack exploiting shared memory and the 
CLFLUSH instruction. By flushing a cache line and measuring reload 
latency with RDTSC, an attacker can determine whether a victim process 
accessed a memory location during the flush-reload window — enabling 
covert channels and cryptographic key extraction.

### Meltdown
Exploits the transient execution window created by out-of-order 
processors to read kernel memory from user space before an exception 
is raised and the pipeline is flushed. Demonstrates how speculative 
execution can bypass hardware privilege boundaries.

### Cache Timing Baseline (cache_hits.c)
Measurement harness for establishing L1/L2/DRAM timing distributions 
using RDTSC, used to calibrate hit/miss thresholds for the 
Flush+Reload attack.

## Visualisation
`plotter.py` generates timing distribution plots to visualise 
covert-channel signal-to-noise ratio and cache hit/miss separation.

## Build
```bash
gcc -O0 -o flush_reload flush_reload.c
gcc -O0 -o meltdown meltdown.c
gcc -O0 -o cache_hits cache_hits.c
```

> Note: Meltdown requires an unpatched kernel or a system with 
> KPTI disabled. Tested on x86-64 Linux.

## Dependencies
- x86-64 Linux
- Python 3 + matplotlib (`pip install matplotlib`) for plotter

## References
- Lipp et al., *Meltdown: Reading Kernel Memory from User Space*, USENIX Security 2018
- Yarom & Falkner, *FLUSH+RELOAD: A High Resolution, Low Noise L3 Cache Side-Channel Attack*, USENIX Security 2014
