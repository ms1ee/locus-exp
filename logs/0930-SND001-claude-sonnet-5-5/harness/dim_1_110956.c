#include "klee/klee.h"

#define PAF_HEADER_LENGTH 2048
#ifndef SF_MAX_CHANNELS
#define SF_MAX_CHANNELS 1024
#endif

void __locus_klee_harness_dim_1(void) {
    /* Predicate input */
    long long filelength;
    klee_make_symbolic(&filelength, sizeof(filelength), "filelength");

    /* Header fields parsed from the file (paf_read_header) */
    int version, channels;
    klee_make_symbolic(&version, sizeof(version), "version");
    klee_make_symbolic(&channels, sizeof(channels), "channels");

    /* Predicate condition negated per harness convention */
    klee_assume(!(filelength < PAF_HEADER_LENGTH));

    /* Path constraint between predicate and canary: version must be 0
       (otherwise paf_read_header exits/returns before the canary) */
    klee_assume(version == 0);

    /* Canary should be unreachable */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
