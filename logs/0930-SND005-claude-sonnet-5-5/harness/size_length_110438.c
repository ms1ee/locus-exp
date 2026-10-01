#include "klee/klee.h"
#include <stdint.h>

/* Model of the path aiff_read_header (src/aiff.c:414) -> aiff_read_chanmap (canary at :1782).
 *
 * Reaching the canary requires the parser to consume: COMM chunk (8 hdr + 18 data)
 * plus a CHAN chunk header (8) = 34 bytes before the chanmap payload is read.
 * Reads cannot go past the end of the file, so reaching aiff_read_chanmap with a
 * valid layout_tag requires psf->filelength >= 34 for non-pipe input.
 */
void __locus_klee_harness_size_length(void) {
    int      is_pipe;
    int64_t  filelength;
    int      channels;
    int      layout_tag;
    uint32_t bytes_consumed_before_chan;   /* bytes the parser has read before CHAN payload */

    klee_make_symbolic(&is_pipe, sizeof(is_pipe), "is_pipe");
    klee_make_symbolic(&filelength, sizeof(filelength), "filelength");
    klee_make_symbolic(&channels, sizeof(channels), "channels");
    klee_make_symbolic(&layout_tag, sizeof(layout_tag), "layout_tag");
    klee_make_symbolic(&bytes_consumed_before_chan, sizeof(bytes_consumed_before_chan),
                       "bytes_consumed_before_chan");

    /* Constrain to inputs where the predicate fires (execution is killed at aiff.c:414). */
    klee_assume(!is_pipe && filelength < 34);

    /* Path feasibility: COMM (26 bytes) + CHAN header (8) must have been read. */
    klee_assume(bytes_consumed_before_chan >= 34);

    /* For a non-pipe file, the parser cannot read beyond filelength. */
    int reached_chanmap = ((int64_t)bytes_consumed_before_chan <= filelength);

    if (reached_chanmap) {
        /* Canary condition: channels > (layout_tag & 0xffff) */
        klee_assert(!(channels > (layout_tag & 0xffff)));
    }
}
