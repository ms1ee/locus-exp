#include "klee/klee.h"

#ifndef SF_MAX_CHANNELS
#define SF_MAX_CHANNELS 1024
#endif

void __locus_klee_harness_channels(void) {
    int channels;
    klee_make_symbolic(&channels, sizeof(channels), "channels");

    /* Predicate would kill: negation of (channels >= 1 && channels <= SF_MAX_CHANNELS) */
    klee_assume(!(channels >= 1 && channels <= SF_MAX_CHANNELS));

    /* Canary should be unreachable if predicate is sound */
    klee_assert(!((channels < 1) || (channels > SF_MAX_CHANNELS)));
}
