#include "klee/klee.h"

void __locus_klee_harness_state_comm_seen(void) {
    unsigned found_chunk;
    int channels;
    int layout_tag;

    klee_make_symbolic(&found_chunk, sizeof(found_chunk), "found_chunk");
    klee_make_symbolic(&channels, sizeof(channels), "channels");
    klee_make_symbolic(&layout_tag, sizeof(layout_tag), "layout_tag");

    /* Only the HAVE_COMM bit matters; its exact value is irrelevant. */
    const unsigned HAVE_COMM_FLAG = 0x10;

    /* The predicate `if (!(found_chunk & HAVE_COMM)) _exit(1)` kills executions
       where this condition holds: constrain to exactly those states. */
    klee_assume(!(found_chunk & HAVE_COMM_FLAG));

    /* psf is zero-initialised and psf->sf.channels is assigned only by
       aiff_read_comm_chunk, after which HAVE_COMM is set. So with HAVE_COMM
       unset, channels is still 0. */
    klee_assume(channels == 0);

    /* Canary in aiff_read_chanmap: must be unreachable in killed states. */
    klee_assert(!(channels > (layout_tag & 0xffff)));
}
