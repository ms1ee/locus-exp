#!/bin/bash
set -eu
export FUZZER=$1
BUG=${2,,}
export MAGMA=$PWD/magma

for TARGET in $(basename -a $MAGMA/targets/*_${BUG}_*); do
    TARGET=$TARGET $MAGMA/tools/captain/build.sh
done
