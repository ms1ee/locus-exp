#include "klee/klee.h"

#ifndef SF_MAX_CHANNELS
#define SF_MAX_CHANNELS 1024
#endif

void __locus_klee_harness_channels(void) {
    int channels;
    klee_make_symbolic(&channels, sizeof(channels), "channels");

    /* Predicate would kill: !(channels < 1 || channels > SF_MAX_CHANNELS) is false */
    klee_assume(!(!(channels < 1 || channels > SF_MAX_CHANNELS)));

    /* Canary must be unreachable (channels is unmodified between canary and predicate) */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
