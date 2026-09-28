#include "klee/klee.h"

/* ------------------------------------------------------------------
 * Locus harness for src/paf.c
 *
 * Predicate (removed, originally at src/paf.c:176):
 *     marker must equal PAF_MARKER or FAP_MARKER, otherwise the
 *     original code path claimed execution would be killed
 *     (early return / exit) before ever reaching the canary.
 *
 * Canary (src/paf.c:206):
 *     MAGMA_OR(paf_fmt.channels < 1, paf_fmt.channels > SF_MAX_CHANNELS)
 *
 * This harness models the marker check and the channels field that
 * flow between the predicate site and the canary site.  We assume
 * the negation of the predicate condition (i.e. we force the case
 * that the predicate claims is impossible: marker is *not* one of
 * the two valid magic values) and then check whether the canary's
 * invalid-channel condition can still be violated (i.e. whether the
 * "impossible" invalid-channels state is actually reachable), which
 * would demonstrate the predicate is unsound.
 * ------------------------------------------------------------------ */

#define MAKE_MARKER(a, b, c, d) \
	((((a) & 0xFF) << 24) | (((b) & 0xFF) << 16) | (((c) & 0xFF) << 8) | ((d) & 0xFF))

#define FAP_MARKER	(MAKE_MARKER ('f', 'a', 'p', ' '))
#define PAF_MARKER	(MAKE_MARKER (' ', 'p', 'a', 'f'))

#define SF_MAX_CHANNELS 1024

void __locus_klee_harness_format_magic(void)
{
	int marker;
	int channels;

	klee_make_symbolic(&marker, sizeof(marker), "marker");
	klee_make_symbolic(&channels, sizeof(channels), "channels");

	/* Predicate condition (as originally intended): the marker equals
	 * PAF_MARKER or FAP_MARKER.  The predicate "kills" execution when
	 * this condition is FALSE (marker invalid).  We assume the negation
	 * of the predicate condition, i.e. we force the marker to be
	 * invalid -- the case the predicate claims can never reach the
	 * canary. */
	klee_assume(!(marker == PAF_MARKER || marker == FAP_MARKER));

	/* Canary condition: channels out of valid range. */
	klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
