#include "klee/klee.h"

#ifndef SF_MAX_CHANNELS
#define SF_MAX_CHANNELS 1024
#endif

void __locus_klee_harness_dim_2(void) {
    int version;
    int channels;
    klee_make_symbolic(&version, sizeof(version), "paf_fmt.version");
    klee_make_symbolic(&channels, sizeof(channels), "paf_fmt.channels");

    /* Constrain to inputs for which the predicate condition is false */
    klee_assume(!(version != 0));

    /* Canary must be unreachable */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
