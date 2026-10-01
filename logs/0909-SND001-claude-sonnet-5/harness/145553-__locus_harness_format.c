#include "klee/klee.h"

/*
** ---------------------------------------------------------------------------
** Locus KLEE harness for predicate at src/sndfile.c:3119
**
**      if (SF_CONTAINER (psf->sf.format) != SF_FORMAT_PAF)
**          _exit (1) ;
**
** This predicate (inserted upstream in psf_open_file()) only checks that the
** container part of the requested/auto-detected sf.format equals
** SF_FORMAT_PAF before continuing on to paf_open() -> paf_read_header().
** It says nothing about the number of channels stored in the on-disk PAF
** header (paf_fmt.channels), which is read straight from file data and is
** fully attacker controlled.
**
** The canary at src/paf.c:206 checks:
**
**      MAGMA_OR (paf_fmt.channels < 1, paf_fmt.channels > SF_MAX_CHANNELS)
**
** i.e. it flags when the channel count read from the file header is out of
** the valid [1, SF_MAX_CHANNELS] range.
**
** Since the format-container predicate does not constrain paf_fmt.channels
** in any way, we expect KLEE to find a counterexample where the predicate
** does *not* fire (container == SF_FORMAT_PAF, so we proceed to
** paf_read_header) yet the canary condition is still true (bad channel
** count) -- demonstrating the predicate is unsound as a guard for this
** canary.
** ---------------------------------------------------------------------------
*/

/* Minimal, self contained model of the relevant libsndfile constants. */
#define SF_FORMAT_TYPEMASK   0x0FFF0000
#define SF_FORMAT_SUBMASK    0x0000FFFF
#define SF_FORMAT_PAF        0x00050000
#define SF_MAX_CHANNELS      1024

#define SF_CONTAINER(x)   ((x) & SF_FORMAT_TYPEMASK)
#define SF_CODEC(x)       ((x) & SF_FORMAT_SUBMASK)

void __locus_klee_harness_format(void) {
    int format;     /* psf->sf.format, as seen at the predicate site */
    int channels;   /* paf_fmt.channels, read later from the PAF header */

    klee_make_symbolic(&format, sizeof(format), "format");
    klee_make_symbolic(&channels, sizeof(channels), "channels");

    /* Predicate site: if this were true, execution would _exit(1) before
    ** ever reaching paf_open()/paf_read_header(). We constrain to the case
    ** where the predicate does NOT kill execution, i.e. the container is
    ** SF_FORMAT_PAF and control flow continues on to the PAF-specific
    ** code path that eventually reaches the canary. */
    klee_assume(!(SF_CONTAINER (format) != SF_FORMAT_PAF));

    /* paf_fmt.channels is populated purely from bytes read out of the file
    ** header in paf_read_header() (via psf_binheader_readf), independent of
    ** the format/container value checked by the predicate above. Model
    ** this by leaving `channels` fully unconstrained. */

    /* Canary condition from paf.c:206 should be unreachable if the
    ** predicate soundly guards it. */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
