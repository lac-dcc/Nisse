# Nisse
This repository contains a suite of LLVM passes that implement [Ball Larus Optimal Edge Profiling](https://dl.acm.org/doi/10.1145/183432.183527).
To reduce the overhead of Ball-Larus instrumentation, we avoid inserting counters on loops with [well-founded](https://en.wikipedia.org/wiki/Well-founded_relation) induction variables.

# Build
Nisse is implemented on top of LLVM.
The code can be built [out of the LLVM tree](https://github.com/banach-space/llvm-tutor#helloworld-your-first-pass) as follows:

```bash
LLVM_INSTALL_DIR=<path/to/llvm/build>
cmake -DLLVM_INSTALL_DIR=$LLVM_INSTALL_DIR -G "Unix Makefiles" -B build/ .
cd build
make
```

# Running
We provide a bash script to run our passes, which can be invoked as follows:

```bash
sh nisse_profiler.sh <path/to/file.c>
```

This basic command will create, in the current folder, the following files:

 * `file.ll` an IR file modified by the following LLVM passes: mem2reg, instnamer, and break-crit-edges.
 * `file.profiled.ll` an IR file instrumented with Ball-Larus counters.
 * `file` an executable file compiled from `file.profiled.ll`.

# Printing Debugging Data
To print edges, spanning tree and the complement of the spanning tree, just
append an extra argument to the `niss_profiler.sh` script, as follows:

```bash
sh nisse_profiler.sh <path/to/file.c> <sth>
```
