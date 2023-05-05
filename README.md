# Nisse
Ball Larus edge profiling with loop optimization

# Build
```bash
LLVM_INSTALL_DIR=<path/to/llvm/build>
cmake -DLLVM_INSTALL_DIR=$LLVM_INSTALL_DIR -G "Unix Makefiles" -B build/ .
cd build
make
```

# Runing the pass
```bash
sh ball_profiler.sh <path/to/file.c>
```
Will create in that file's folder :
 * `file.ll` an IR file with the passes mem2reg,instnamer,break-crit-edges ran
 * `file.profiled.ll` an IR file with the ball pass ran after the previous ones
 * `file` an executable compiled from `file.profiled.ll`
and will run `file`

# Currently done
Ball Larus edge profiling pass