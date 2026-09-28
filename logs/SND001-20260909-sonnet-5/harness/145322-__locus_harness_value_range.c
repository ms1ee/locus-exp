#include "klee/klee.h"

/* SF_MAX_CHANNELS as defined in libsndfile's common.h */
#define SF_MAX_CHANNELS 1024

void __locus_klee_harness_value_range(void) {
    int version;
    int channels;

    klee_make_symbolic(&version, sizeof(version), "version");
    klee_make_symbolic(&channels, sizeof(channels), "channels");

    /* Constrain: predicate would kill (negation of predicate condition) */
    klee_assume(!(version != 0));

    /* Assert: canary should be unreachable */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
