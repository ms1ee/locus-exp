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
