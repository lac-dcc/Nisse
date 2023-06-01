#!/usr/bin/env bash

# Example:
# ./mem_profiler.sh ../../tests/simple_array.c

if [ $# -lt 1 ]
then
    echo Syntax: regions_ex file.c
    exit 1
fi

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
DR_NAME="$(dirname $1)"
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

CLANG_FLAGS="-Xclang -disable-O0-optnone -std=c99 -c -S -emit-llvm"

# Generating the bytecode in SSA form:
#
$LLVM_CLANG $CLANG_FLAGS ../$FL_NAME -o $LL_NAME
if [[ $ret_code -ne 0 ]]; then
  echo "Compilation failed"
  cd -
  echo $FL_NAME >> err.txt
  exit $ret_code
fi
$LLVM_OPT -S -passes="mem2reg,instnamer" $LL_NAME -o $LL_NAME

# Running the pass:
#
$LLVM_OPT -S -load-pass-plugin $MY_LLVM_LIB -passes="ball" -stats \
    $LL_NAME -o $PF_NAME

# Print the instrumented CFG
#
$LLVM_OPT -S -passes="dot-cfg" -stats \
    $PF_NAME -o $PF_NAME

# Compile the newly instrumented program, and link it against the profiler.
# We are passing -no_pie to disable address space layout randomization:
#
$LLVM_CLANG -Wall -std=c99 $PF_NAME $PROFILER_IMPL -o $BS_NAME
ret_code=$?
if [[ $ret_code -ne 0 ]]; then
  echo "Compilation failed"
  cd -
  echo $FL_NAME >> err.txt
  exit $ret_code
fi

# Run the instrumented binary:
#
./$BS_NAME 0

# Prepare the result folders
#
mkdir graphs profiles compiled partial_profiles dot

# Propagation Flags to use:
#
if [ $# -eq 2 ]
then
  PROP_FLAG="-s"
else
  PROP_FLAG=""
fi

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
for i in .*.dot; do
  mv $i dot/
done

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