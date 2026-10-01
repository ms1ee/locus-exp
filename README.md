# locus-exp

This repo runs [Locus](https://github.com/cirrus-uchicago/Locus) (ICSE '26) on a [Magma](https://github.com/HexHive/magma) bug, then
fuzzes the result with [SelectFuzz](https://github.com/cuhk-seclab/SelectFuzz) (S&P '23), with and
without the predicates Locus inserts.

## Environment

- Ubuntu 22.04.
- Python 3.13.5
- cmake 4.4.3
- KLEE 3.1 (LLVM 13) + [gllvm](https://github.com/SRI-CSL/gllvm) (`gclang`, `get-bc`), for `--bitcode-build`

cmake 4 dropped `cmake_minimum_required(VERSION <3.5)`, and libsndfile declares `3.1..3.18`.\
So we resolved with `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`.

Bugs so far: `SND001`, `SND005` (libsndfile).
```
git clone --recurse-submodules <this repo> locus-exp && cd locus-exp
cd locus && uv sync && cd ..
echo core | sudo tee /proc/sys/kernel/core_pattern
```
## 1. Obtain predicates

```
./locus_SND001.sh
```

`locus_SND001.sh` verbatim; following the Locus usage.

```bash
#!/bin/bash
set -eu
LOCUS=$PWD/locus
MAGMA=$PWD/magma
BUG=SND001
MODEL=claude-sonnet-5-5
OUT=$PWD/logs/$(date +%m%d)-$BUG-$MODEL
DIR=/tmp/locus-SND001
rm -rf $DIR
mkdir -p $OUT

export PATH=/usr/lib/llvm-13/bin:$PATH
export KLEE_INCLUDE=$HOME/opt/klee-3.1/include

# 1. Set up target with canary
git clone https://github.com/libsndfile/libsndfile.git $DIR/repo
cd $DIR/repo && git checkout 86c9f9eb
git apply $MAGMA/targets/libsndfile/patches/bugs/SND001.patch

# 2. Build with compile_commands.json
cmake -B build -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
make -C build -j$(nproc)
ln -s build/compile_commands.json .

# 3. Build LLVM bitcode for KLEE
CC=gclang cmake -B build-bc -DCMAKE_POLICY_VERSION_MINIMUM=3.5 \
  -DCMAKE_C_FLAGS="-g -O0 -Xclang -disable-O0-optnone" \
  -DBUILD_PROGRAMS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF \
  -DENABLE_EXTERNAL_LIBS=OFF -DENABLE_MPEG=OFF
make -C build-bc -j$(nproc)
echo /__locus_klee.bc >> .git/info/exclude
git add -A && git commit -m "setup"

# 4. Run Locus
export ANTHROPIC_API_KEY="<YOUR_KEY>"
cd $LOCUS
uv run locus \
  --repo $DIR/repo \
  --binary-build "make -C build -j$(nproc)" \
  --bitcode-build "make -C build-bc -j$(nproc) sndfile && get-bc -b -o build-bc/libsndfile.bc build-bc/libsndfile.a && clang -c -emit-llvm -g -O0 -Xclang -disable-O0-optnone -I$KLEE_INCLUDE -Iinclude -Ibuild-bc/include -Isrc -Ibuild-bc/src -include build-bc/src/config.h -include src/common.h '-DMAGMA_OR(a,b)=((a)||(b))' '-DMAGMA_AND(a,b)=((a)&&(b))' __locus_harness_*.c -o build-bc/harness.bc && llvm-link build-bc/libsndfile.bc build-bc/harness.bc -o __locus_klee.bc" \
  --model anthropic:$MODEL \
  --artifacts-dir $OUT \
  --min-iterations 3 --max-iterations 5 "$@" 2>&1 | tee $OUT/$BUG.log
git -C $DIR/repo diff > $OUT/$BUG.patch
mkdir -p $OUT/history
mv $DIR/repo/__locus_history*.json $OUT/history/
```

Locus edits the source tree in place and never emits a patch.
Magma clones the source afresh inside the container, so that tree cannot be handed over. \
We therefore record the predicates with `git diff`, and stage 2 applies that patch.

## 2. Build binary

`magma/` is our fork of Magma ([ms1ee/magma](https://github.com/ms1ee/magma), branch `locus-exp`). \
`magma/fuzzers/selectfuzz/` adds SelectFuzz as a Magma fuzzer.

Only `selectfuzz` is supported for now.

```
# Usage: ./build.sh <fuzzer> <bug>
./build.sh selectfuzz SND001
```

`build.sh` verbatim; it builds one Docker image per target.

```bash
#!/bin/bash
set -eu
export FUZZER=$1
BUG=${2,,}
export MAGMA=$PWD/magma

for TARGET in $(basename -a $MAGMA/targets/*_${BUG}_*); do
    TARGET=$TARGET $MAGMA/tools/captain/build.sh
done
```

## 3. Fuzz

```
# Usage: ./fuzz.sh <fuzzer> <bug> <timeout> <repeat>
./fuzz.sh selectfuzz SND001 24h 10
```

`fuzz.sh` verbatim; it writes the captainrc and hands it to Magma's captain.

```bash
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
```