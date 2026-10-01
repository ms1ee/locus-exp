#include "klee/klee.h"

#define MK(a,b,c,d) ((int)((unsigned)(a) | ((unsigned)(b) << 8) | ((unsigned)(c) << 16) | ((unsigned)(d) << 24)))
#define PAF_MARKER MK(' ', 'p', 'a', 'f')
#define FAP_MARKER MK('f', 'a', 'p', ' ')
#define SF_MAX_CHANNELS 1024

void __locus_klee_harness_dim_3(void) {
    int marker;
    int version, channels;
    klee_make_symbolic(&marker, sizeof(marker), "marker");
    klee_make_symbolic(&version, sizeof(version), "version");
    klee_make_symbolic(&channels, sizeof(channels), "channels");

    /* Predicate would kill: predicate condition is false, i.e. marker is valid */
    klee_assume(!(!(marker == PAF_MARKER || marker == FAP_MARKER)));

    /* Path constraint between predicate and canary: version must be 0 */
    klee_assume(version == 0);

    /* Canary should be unreachable */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
