#include "klee/klee.h"

/*
 * These mirror the SFM_* file-mode constants and the PAF header length
 * constant used in src/paf.c.  The harness is self contained so it does
 * not need to pull in the full sndfile/common headers.
 */
#define SFM_READ            0x10
#define SFM_WRITE           0x20
#define SFM_RDWR            0x30

#define PAF_HEADER_LENGTH   2048
#define SF_MAX_CHANNELS     1024

/*
 * ---------------------------------------------------------------------
 * Predicate under test (now REMOVED because it was shown unsound):
 *
 *   if ( (psf->file.mode == SFM_READ ||
 *         (psf->file.mode == SFM_RDWR && psf->filelength > 0))
 *        && psf->filelength < PAF_HEADER_LENGTH )
 *       _exit (1) ;
 *
 * This predicate was hoisted from paf_read_header()'s short-header guard
 * ( `if (psf->filelength < PAF_HEADER_LENGTH) return SFE_PAF_SHORT_HEADER ;`
 * at src/paf.c:183-184 ) up into paf_open(), on the assumption that once
 * the mode/filelength combination is known to be "too short to contain a
 * PAF header", the canary further down in paf_read_header() (the
 * paf_fmt.channels range check, originally at src/paf.c:206, now at
 * src/paf.c:234/238) can never be reached.
 *
 * The predicate says nothing at all about paf_fmt.channels -- that value
 * is populated later from header bytes that are completely independent
 * of the mode/filelength check.  So even when the predicate "fires"
 * (i.e. the mode/filelength combination it flags as fatal holds), the
 * channels value feeding the canary is still totally unconstrained, and
 * a KLEE counterexample (mode=SFM_READ, filelength=0, channels=0) shows
 * the canary condition can still be triggered.  Hence the predicate is
 * unsound.
 * ---------------------------------------------------------------------
 */

void __locus_klee_harness_size(void)
{
    int mode;
    long filelength;
    int channels;

    klee_make_symbolic(&mode, sizeof(mode), "mode");
    klee_make_symbolic(&filelength, sizeof(filelength), "filelength");
    klee_make_symbolic(&channels, sizeof(channels), "channels");

    /* Restrict `mode` to the values SF_PRIVATE->file.mode can actually take. */
    klee_assume(mode == SFM_READ || mode == SFM_WRITE || mode == SFM_RDWR);

    /* Predicate (guard) condition, matching the removed code at src/paf.c:123. */
    int predicate_condition =
        (mode == SFM_READ || (mode == SFM_RDWR && filelength > 0))
        && (filelength < PAF_HEADER_LENGTH);

    /* Constrain inputs to the case where the predicate would have fired
     * (i.e. where the removed code would have called _exit(1), under the
     * hypothesis that this makes the canary unreachable). */
    klee_assume(!(!predicate_condition));

    /* paf_fmt.channels (canary input) is unconstrained by the predicate. */
    int canary_condition = (channels < 1) || (channels > SF_MAX_CHANNELS);

    /* If the predicate were sound, the canary could never be triggered
     * whenever the predicate fires.  KLEE can find channels == 0 (or any
     * value outside [1, SF_MAX_CHANNELS]) satisfying this, proving the
     * predicate unsound. */
    klee_assert(!(canary_condition));
}
