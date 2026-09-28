#!/bin/bash
set -eu
FUZZER=$1
BUG=${2,,}
MAGMA=$PWD/magma
WORKDIR=$PWD/workdir
TARGETS=($(basename -a $MAGMA/targets/*_${BUG}_*))
mkdir -p $WORKDIR

cat > $WORKDIR/captainrc <<RC
WORKDIR=$WORKDIR
REPEAT=$4
TIMEOUT=$3
POLL=1
CACHE_ON_DISK=1
FUZZERS=($FUZZER)
${FUZZER}_TARGETS=(${TARGETS[*]})
RC
MAGMA=$MAGMA exec $MAGMA/tools/captain/run.sh $WORKDIR/captainrc
