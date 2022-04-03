#!/bin/bash
set -e
export LLVM_INSTALL_PATH=/modules/cs325/llvm-12.0.1
export PATH=$LLVM_INSTALL_PATH/bin:$PATH
export LD_LIBRARY_PATH=$LLVM_INSTALL_PATH/lib:$LD_LIBRARY_PATH
CLANG=$LLVM_INSTALL_PATH/bin/clang++
module load gcc9

DIR=$(pwd)
### Build Emergent compiler
echo "***** CLEANUP *****"
make clean

echo "***** COMPILE *****"
make emergent

echo "***** TEST *****"

function validate {
  $1 > perf_out
  echo $1
  grep "Result" perf_out;grep "PASSED" perf_out
  rc=$?; if [[ $rc != 0 ]]; then echo "***** TEST FAILED *****";exit $rc; fi;rm perf_out
}

cd tests/game_of_life/
pwd
rm -rf output.ll game_of_life
$DIR/bin/emergent ./game_of_life.emg
$CLANG driver.cpp output.ll -o game_of_life
validate "./game_of_life"

echo "***** ALL TESTS PASSED *****"
