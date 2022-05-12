#!/bin/bash
set -e
CLANG=$LLVM_INSTALL_PATH/bin/clang++
module load gcc9

DIR=$(pwd)
### Build Emergent compiler
echo "***** CLEANUP *****"
make clean

echo "***** COMPILE *****"
make emergent

echo "***** TEST *****"

cd tests/game_of_life/
rm -rf ./*.out
pwd
$DIR/bin/emergent ./game_of_life.emg
$CLANG ./game_of_life.cpp -o game_of_life
./game_of_life example.txt conway 4 example.txt

cd ../../

cd tests/rule_thirty/
rm -rf ./*.out
pwd
$DIR/bin/emergent ./rule_thirty.emg
$CLANG ./rule_thirty.cpp -o rule_thirty

cd ../../

cd tests/wireworld/
rm -rf ./*.out
pwd
$DIR/bin/emergent ./wireworld.emg
$CLANG ./wireworld.cpp -o wireworld

cd ../../

cd tests/waves/
rm -rf ./*.out
pwd
$DIR/bin/emergent ./waves.emg
$CLANG ./waves.cpp -o waves

echo "***** TESTS PASSED *****"
