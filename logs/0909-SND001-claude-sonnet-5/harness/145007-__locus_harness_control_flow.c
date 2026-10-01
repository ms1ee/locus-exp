#include "klee/klee.h"

/*
 * Constants mirrored from include/sndfile.h.in and src/common.h / src/paf.c
 * so that this harness does not need to pull in the full libsndfile type
 * system (SF_PRIVATE, PAF_FMT, etc.) in order to reason about the
 * control-flow / value-range path between the predicate at src/paf.c:120
 * and the canary at src/paf.c:206 (well, the MAGMA_OR canary logged at
 * src/paf.c:233, guarded originally by paf_fmt.version != 0 check at
 * src/paf.c:206).
 */

#define SFM_READ            0x10
#define SFM_WRITE           0x20
#define SFM_RDWR            0x30

#define PAF_HEADER_LENGTH   2048

/* Arbitrary distinct "magic marker" values standing in for the real
 * PAF_MARKER / FAP_MARKER constants -- their concrete numeric value is
 * irrelevant to the control flow, only whether the read marker equals
 * one of the two valid values. */
#define PAF_MARKER_VAL      1
#define FAP_MARKER_VAL      2

#define SF_MAX_CHANNELS     1024

void __locus_klee_harness_control_flow(void) {
    int mode;          /* psf->file.mode */
    long filelength;   /* psf->filelength */
    int marker;        /* local `marker` in paf_read_header */
    int version;        /* paf_fmt.version */
    int channels;       /* paf_fmt.channels */

    klee_make_symbolic(&mode, sizeof(mode), "mode");
    klee_make_symbolic(&filelength, sizeof(filelength), "filelength");
    klee_make_symbolic(&marker, sizeof(marker), "marker");
    klee_make_symbolic(&version, sizeof(version), "version");
    klee_make_symbolic(&channels, sizeof(channels), "channels");

    /* Predicate (src/paf.c:120) would kill execution unless mode is one of
     * the three valid values. Constrain to the negation of the predicate
     * condition, i.e. the case where the predicate does NOT fire and
     * execution continues. */
    klee_assume(!(mode != SFM_READ && mode != SFM_WRITE && mode != SFM_RDWR));

    /* To actually reach paf_read_header() (src/paf.c:123), and hence the
     * canary inside it, the mode must be SFM_READ, or SFM_RDWR with a
     * non-empty existing file. */
    klee_assume(mode == SFM_READ || (mode == SFM_RDWR && filelength > 0));

    /* paf_read_header() returns immediately (no canary reached) if the
     * file is shorter than the fixed header length (src/paf.c:178-179). */
    klee_assume(filelength >= PAF_HEADER_LENGTH);

    /* paf_read_header() exits immediately if the marker read from the
     * header does not match one of the two valid PAF signatures
     * (src/paf.c:188-189). */
    klee_assume(marker == PAF_MARKER_VAL || marker == FAP_MARKER_VAL);

    /* paf_read_header() exits immediately unless the header version field
     * is exactly 0 (src/paf.c:206-207, the version-dimension predicate
     * right before the canary). */
    klee_assume(version == 0);

    /* channels is read straight from the (attacker-controlled) file header
     * and is NOT constrained in any way by psf->file.mode. The predicate
     * under test only restricts `mode`, so it provides no protection
     * against an out-of-range channel count reaching the canary check. */

    /* Canary (src/paf.c:206 / 233): channels must be within [1, SF_MAX_CHANNELS]. */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
