#include "klee/klee.h"

#ifndef PAF_HEADER_LENGTH
#define PAF_HEADER_LENGTH 2048
#endif
#ifndef SF_MAX_CHANNELS
#define SF_MAX_CHANNELS 1024
#endif

void __locus_klee_harness_dim_2(void) {
    long long filelength;
    int channels;
    int marker, version;

    klee_make_symbolic(&filelength, sizeof(filelength), "filelength");
    klee_make_symbolic(&channels, sizeof(channels), "channels");
    klee_make_symbolic(&marker, sizeof(marker), "marker");
    klee_make_symbolic(&version, sizeof(version), "version");

    /* Predicate: !(filelength >= PAF_HEADER_LENGTH); assume it does not hold */
    klee_assume(!(!(filelength >= PAF_HEADER_LENGTH)));

    /* Path constraints between predicate and canary */
    klee_assume(version == 0);

    /* Canary should be unreachable */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
