#include "klee/klee.h"

void __locus_klee_harness_state_channels(void) {
    int channels;
    int layout_tag;
    klee_make_symbolic(&channels, sizeof(channels), "channels");
    klee_make_symbolic(&layout_tag, sizeof(layout_tag), "layout_tag");

    /* States the predicate kills: channels < 2 */
    klee_assume(channels < 2);

    /* Reaching the canary requires a layout with a non-NULL channel_map,
       i.e. a tag whose low 16 bits (channel count) are >= 1. */
    klee_assume((layout_tag & 0xffff) >= 1);

    /* Canary must be unreachable for killed states */
    klee_assert(!(channels > (layout_tag & 0xffff)));
}
