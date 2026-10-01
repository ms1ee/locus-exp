#include "klee/klee.h"

#ifndef SF_MAX_CHANNELS
#define SF_MAX_CHANNELS 1024
#endif

void __locus_klee_harness_dim_3(void) {
    int version, channels;
    klee_make_symbolic(&version, sizeof(version), "version");
    klee_make_symbolic(&channels, sizeof(channels), "channels");

    /* Predicate kills when version != 0; consider inputs where it would kill. */
    klee_assume(!(version != 0));

    /* Canary should be unreachable. */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
