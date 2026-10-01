#include "klee/klee.h"

#ifndef SF_MAX_CHANNELS
#define SF_MAX_CHANNELS 1024
#endif

void __locus_klee_harness_dim_4(void) {
    int version;
    int channels;
    klee_make_symbolic(&version, sizeof(version), "paf_fmt.version");
    klee_make_symbolic(&channels, sizeof(channels), "paf_fmt.channels");

    /* Predicate kills when !(version == 0); constrain to negation: version == 0 */
    klee_assume(!(!(version == 0)));

    /* Canary should be unreachable */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
