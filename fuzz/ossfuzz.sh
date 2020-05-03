#!/bin/sh
#
# entry point for oss-fuzz, so that fuzzers
# and build invocation can be changed without having
# to modify the oss-fuzz repo.
#
# invoke it from the git root.

# make sure to exit on problems
set -e
set -u
set -x

for prog in zip cmake ninja; do
    if ! which $prog >/dev/null; then
	echo please install $prog
	exit 1
    fi
done


# build the corpus (all inputs are json, the same corpus can be used for everyone)
fuzz/build_corpus.sh

mkdir -p build
cd build

cmake .. \
-GNinja \
-DCMAKE_BUILD_TYPE=Debug \
-DSIMDJSON_BUILD_STATIC=On \
-DENABLE_FUZZING=On \
-DSIMDJSON_COMPETITION=Off \
-DSIMDJSON_FUZZ_LINKMAIN=Off \
-DSIMDJSON_FUZZ_LDFLAGS=$LIB_FUZZING_ENGINE

cmake --build . --target all_fuzzers

cp fuzz/fuzz_* $OUT

# all corpora are equal, they all take json as input
for f in $(ls $OUT/fuzz* |grep -v '.zip$') ; do
   cp ../corpus.zip $OUT/$(basename $f).zip
done
