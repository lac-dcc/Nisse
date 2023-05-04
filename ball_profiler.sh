#!/bin/zsh

# Example:
# ./mem_profiler.sh ../../tests/simple_array.c

if [ $# -lt 1 ]
then
    echo Syntax: regions_ex file.c
    exit 1
else
  # LLVM tools:
  #
  PROG_HOME=~/Stage
  LLVM_INSTALL_DIR=$PROG_HOME/llvm-project/build
  LLVM_OPT=$LLVM_INSTALL_DIR/bin/opt
  LLVM_CLANG=$LLVM_INSTALL_DIR/bin/clang
  MY_LLVM_LIB=$PROG_HOME/Nisse/build/lib/libBall.so
  PROFILER_IMPL=$PROG_HOME/Nisse/lib/prof.c

  # Move to folder where the file is:
  #
  DR_NAME="$PWD/$(dirname $1)"
  cd $DR_NAME

  # File names:
  #
  FL_NAME=$(basename $1)
  BS_NAME=$(basename $FL_NAME .c)
  LL_NAME="$BS_NAME.ll"
  PF_NAME="$BS_NAME.profiled.ll"

  # Generating the bytecode in SSA form:
  #
  $LLVM_CLANG -Xclang -disable-O0-optnone -c -S -emit-llvm $FL_NAME -o $LL_NAME
  $LLVM_OPT -S -passes="mem2reg,instnamer,break-crit-edges" $LL_NAME -o $LL_NAME

  # Running the pass:
  #
  $LLVM_OPT -S -load-pass-plugin $MY_LLVM_LIB -passes=ball -stats \
     $LL_NAME -o $PF_NAME

  # Compile the newly instrumented program, and link it against the profiler.
  # We are passing -no_pie to disable address space layout randomization:
  #
  $LLVM_CLANG -Wl $PF_NAME $PROFILER_IMPL -o $BS_NAME

  # Run the instrumented binary:
  #
  ./$BS_NAME

  # Go back to the folder where you were before:
  #
  cd -
fi
