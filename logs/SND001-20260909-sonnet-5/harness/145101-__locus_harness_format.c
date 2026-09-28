#include "klee/klee.h"

#define SF_MAX_CHANNELS   1024
#define PAF_MARKER        1
#define FAP_MARKER        2

void __locus_klee_harness_format(void) {
    int marker;
    int channels;

    klee_make_symbolic(&marker, sizeof(marker), "marker");
    klee_make_symbolic(&channels, sizeof(channels), "channels");

    /* Constrain: predicate would kill the run (i.e. negate the predicate
     * condition so that the header marker is one of the two valid PAF
     * signatures and execution continues past the predicate). */
    klee_assume(!(marker != PAF_MARKER && marker != FAP_MARKER));

    /* channels is read from the file header after the marker check and is
     * otherwise unconstrained on this path (it is independent of marker). */

    /* Assert: canary should be unreachable, i.e. channels should always be
     * within [1, SF_MAX_CHANNELS] once the predicate has been satisfied. */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
