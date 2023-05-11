#!/usr/bin/env bash

# Example:
# ./mem_profiler.sh ../../tests/simple_array.c

if [ $# -lt 1 ]
then
    echo Syntax: regions_ex file.c
    exit 1
else
  # LLVM tools:
  #
  source ./config.sh

  LLVM_OPT=$LLVM_INSTALL_DIR/bin/opt
  LLVM_CLANG=$LLVM_INSTALL_DIR/bin/clang
  PROFILER_IMPL=$SOURCE_DIR/lib/prof.c
  MY_LLVM_LIB=$BUILD_DIR/lib/libNisse.so
  PROP_BIN=$BUILD_DIR/bin/propagation

  # Move to folder where the file is:
  #
  DR_NAME="$PWD/$(dirname $1)"
  cd $DR_NAME

  # File names:
  #
  FL_NAME=$(basename $1)
  BS_NAME=$(basename $FL_NAME .c)
  PROF_DIR=$FL_NAME.profiling
  LL_NAME="$BS_NAME.ll"
  PF_NAME="$BS_NAME.profiled.ll"

  
  # Create the profiling folder:
  #
  if [ -d "$PROF_DIR" ]; then
    rm -r "$PROF_DIR"
  fi
  mkdir $PROF_DIR
  cd $PROF_DIR

  # Pass to use:
  if [ $# -eq 2 ]
  then
    PASS="print<nisse>"
    PROP_FLAG="-d -s"
  else
    PASS="nisse"
    PROP_FLAG="-s"
  fi

  # Generating the bytecode in SSA form:
  #
  $LLVM_CLANG -Xclang -disable-O0-optnone -c -S -emit-llvm ../$FL_NAME -o $LL_NAME
  $LLVM_OPT -S -passes="mem2reg,instnamer,break-crit-edges" $LL_NAME -o $LL_NAME

  # Running the pass:
  #
  $LLVM_OPT -S -load-pass-plugin $MY_LLVM_LIB -passes=$PASS -stats \
     $LL_NAME -o $PF_NAME

  # Compile the newly instrumented program, and link it against the profiler.
  # We are passing -no_pie to disable address space layout randomization:
  #
  $LLVM_CLANG -Wall $PF_NAME $PROFILER_IMPL -o $BS_NAME

  # Run the instrumented binary:
  #
  ./$BS_NAME

  # Prepare the result folders
  #
  mkdir graphs profiles compiled partial_profiles

  # Propagate the weights for each function:
  #
  for i in *.prof; do
    PROF_NAME="$i.full"
    $PROP_BIN $i $PROP_FLAG -o $PROF_NAME
    mv $i partial_profiles/
    mv $PROF_NAME profiles/
  done

  # Move the files to apropriate folders
  #
  for i in *.graph; do
    mv $i graphs/
  done

  for i in *.ll; do
    mv $i compiled/
  done

  mv $BS_NAME compiled/

  # Go back to the folder where you were before:
  #
  cd -
fi
