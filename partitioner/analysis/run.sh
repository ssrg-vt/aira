#!/bin/bash

# Usage:
# ./run.sh PARALLEL_FUNCTIONS foo.c bar.c ...
#
# The file 'PARALLEL_FUNCTIONS' should contain a list of the names of functions
# that can be executed in parallel, one per line.

# This is not a system install, but your own compiled version of LLVM with the
# ptrack support built in.
LLVM_INSTALL_PATH="$HOME/Build/LLVM/llvm.install"
DEBUG=0

CLANG="$LLVM_INSTALL_PATH/bin/clang"
OPT="$LLVM_INSTALL_PATH/bin/opt"
LLVM_DIS="$LLVM_INSTALL_PATH/bin/llvm-dis"
LLVM_PASS="$LLVM_INSTALL_PATH/lib/LLVMPTrack.so"
STUBS="library/ptrack_stubs.so"
BIN=./bin
#STATS="-debug-only=ptrack -stats"

function die {
  echo "ERROR: $*"
  exit 1
}

function check_exists {
  [ ! -e "$1" ] && die "'$1' does not exist."
}
check_exists "$CLANG"
check_exists "$OPT"
check_exists "$LLVM_PASS"
check_exists "$STUBS"

INPUT_BC=""
OUTPUT_BC=""

# We want the code to be maximally optimised before execution so that our
# memory pattern profile is accurate, however we also want every source
# function to exist.  Disabling inlining will have a knock-on affect on other
# optimisations, but leaving it turned on will result in inaccurate
# partitioning decisions.
CFLAGS="-O3 -fno-inline"

# Compile each input file.  Note that we optimise *before* annotating memory
# access to try and get a better picture of the true access patterns.
PARALLEL="$1" ; shift
for f in "$@" ; do
  echo -e "\tCC\t$f"
  bc="$(basename ${f%.c}).bc" # TODO handle .cc, .cpp
  $CLANG $CFLAGS -emit-llvm -c "$f" -o "$bc" || die "Couldn't compile '$f'."
  INPUT_BC+=" $bc"
done

# Apply our annotation pass to each file.
for bc in $INPUT_BC ; do
  echo -e "\tOPT\t$bc"
  bc2="${bc%.bc}_annotated.bc"
  $OPT -load "$LLVM_PASS" -ptrack $STATS "$bc" -o "$bc2" \
    || die "Couldn't annotate '$bc'."
  OUTPUT_BC+=" $bc2"
done

# If debugging then convert the .bc files to .ll (i.e. readable text).
if [ $DEBUG -eq 1 ] ; then
  for bc in $OUTPUT_BC ; do
    $LLVM_DIS $bc
  done
fi

# Link together all the files.
echo -e "\tLD\t*.bc"
$CLANG -lm -march=native -O3 $OUTPUT_BC "$STUBS" -o $BIN || die "Couldn't link."

# Run the program.
echo -e "\tRUN"
PTRACK_PARALLEL="$PARALLEL" $BIN || die "Couldn't run program."

# If 'dot' is installed then render the graph.
which dot &> /dev/null \
  && echo -e "\tGRAPH" \
  && dot -Tpdf graph.dot > graph.pdf \
  || echo -e "\tNO GRAPH"

# Clean-up all created files.
echo -e "\tCLEAN"
rm -f $INPUT_BC $OUTPUT_BC $BIN graph.dot
