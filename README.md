# Nisse
This repository contains a suite of LLVM passes that implement [Knuth, D.E., Stevenson, F.R. Optimal measurement points for program frequency counts.](https://doi.org/10.1007/BF01951942).
To reduce the overhead of KS instrumentation, we avoid inserting counters on loops with [well-founded](https://en.wikipedia.org/wiki/Well-founded_relation) induction variables.

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
bash nisse_profiler.sh <path/to/file.c>
```

It also requires to configure the file `config.sh`.

LLVM_INSTALL_DIR is the path to the build directory of llvm.
SOURCE_DIR is the path to this folder.

This basic command will create a folder `file.c.profiling`.
In that folder are 5 subfolders:
* `compiled` contains 3 files:
  *  `file.ll` an IR file modified by the following LLVM passes: mem2reg, and instnamer.
  *  `file.profiled.ll` an IR file instrumented with KS counters.
  *  `file` an executable file compiled from `file.profiled.ll`.
* `profiles` contains the complete profile for each function. By default, if a function is called multiple times, the profile will contain the total execution. Adding an an extra argument to the `nisse_profiler.sh` script generates a separate profile for each execution instead.
* `partial_profiles` contains the profile data obtained by the instrumentation for each function.
* `graphs` contains the vertices, edges, spanning tree and instrumented edges of each function's CFG.
* `dot` contains a `dot` file with the representation of each function's CFG.

Functions with no branches are not instrumented (since their execution is always linear).