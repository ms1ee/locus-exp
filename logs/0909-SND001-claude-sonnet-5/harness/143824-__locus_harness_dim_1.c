#include "klee/klee.h"

/*
** Minimal standalone definitions mirroring the real constants used in
** src/paf.c so that the harness does not depend on the full libsndfile
** header/build machinery.
*/
#define SFM_READ   0x10
#define SFM_WRITE  0x20
#define SFM_RDWR   0x30

#define SF_MAX_CHANNELS 1024

typedef long long sf_count_t;

typedef struct
{
    int mode ;
} SF_FILE_INFO ;

typedef struct
{
    SF_FILE_INFO file ;
    sf_count_t   filelength ;
} SF_PRIVATE ;

typedef struct
{   int channels ;
} PAF_FMT ;

#define MAGMA_OR(a, b) ((a) || (b))

void __locus_klee_harness_dim_1(void)
{
    SF_PRIVATE psf_s ;
    SF_PRIVATE *psf = &psf_s ;

    PAF_FMT paf_fmt ;

    int mode ;
    sf_count_t filelength ;
    int channels ;

    klee_make_symbolic(&mode, sizeof(mode), "mode") ;
    klee_make_symbolic(&filelength, sizeof(filelength), "filelength") ;
    klee_make_symbolic(&channels, sizeof(channels), "channels") ;

    psf->file.mode   = mode ;
    psf->filelength  = filelength ;
    paf_fmt.channels = channels ;

    /* Constrain mode to the plausible set of values so we don't waste
    ** paths on modes that couldn't occur in practice, while still
    ** covering READ / WRITE / RDWR. */
    klee_assume(mode == SFM_READ || mode == SFM_WRITE || mode == SFM_RDWR) ;

    /* Predicate at paf.c:115 :
    **   if (! (psf->file.mode == SFM_READ ||
    **          (psf->file.mode == SFM_RDWR && psf->filelength > 0)))
    **       _exit (1) ;
    **
    ** Constrain inputs to the region where the predicate would NOT kill
    ** the execution, i.e. negate the predicate condition so we reach the
    ** code path that eventually leads to paf_read_header() and the
    ** canary check on paf_fmt.channels.
    */
    klee_assume(!(!(psf->file.mode == SFM_READ ||
                    (psf->file.mode == SFM_RDWR && psf->filelength > 0)))) ;

    /* paf_fmt.channels is read directly from the (attacker-controlled)
    ** file header via psf_binheader_readf() and is not otherwise
    ** constrained between the predicate and the canary, so it remains
    ** fully symbolic here. */

    /* Canary at paf.c:239 :
    **   MAGMA_OR(paf_fmt.channels < 1, paf_fmt.channels > SF_MAX_CHANNELS)
    ** If the predicate were a sound guard against reaching the canary
    ** with an out-of-range channel count, this assertion should never
    ** be violated under the constraints established above.
    */
    klee_assert(!(MAGMA_OR(paf_fmt.channels < 1, paf_fmt.channels > SF_MAX_CHANNELS))) ;
}
