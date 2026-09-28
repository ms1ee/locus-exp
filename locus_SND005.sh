#!/bin/bash
set -eu
LOCUS=$PWD/locus
MAGMA=$PWD/magma
OUT=$PWD/logs/$(date +%m%d)-SND005
DIR=/tmp/locus-SND005
rm -rf $DIR

# 1. Set up target with canary
git clone https://github.com/libsndfile/libsndfile.git $DIR/repo
cd $DIR/repo && git checkout 86c9f9eb
git apply $MAGMA/targets/libsndfile/patches/bugs/SND005.patch

# 2. Build with compile_commands.json
cmake -B ../build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make -C ../build -j$(nproc)
ln -s ../build/compile_commands.json .
git add -A && git commit -m "setup"

# 3. Run Locus
export ANTHROPIC_API_KEY="<YOUR_KEY>"
cd $LOCUS
uv run locus \
  --repo $DIR/repo \
  --binary-build "make -C ../build -j$(nproc)" \
  --model anthropic:claude-opus-5 \
  --min-iterations 1 --max-iterations 3 "$@" 2>&1 | tee $OUT.log
git -C $DIR/repo diff > $OUT.patch
