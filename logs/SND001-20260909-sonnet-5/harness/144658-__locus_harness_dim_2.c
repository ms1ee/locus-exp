#include "klee/klee.h"

/* Relevant constants from libsndfile (src/sndfile.h / src/common.h) */
#define SFM_READ         0x10
#define SFM_WRITE        0x20
#define SFM_RDWR         0x30

#define PAF_HEADER_LENGTH  2048
#define SF_MAX_CHANNELS    1024

typedef long long sf_count_t_local;

void __locus_klee_harness_dim_2(void)
{
    /* Symbolic inputs relevant to the predicate at paf.c:118
    ** and the canary at paf.c:206 (channel-count check inside
    ** paf_read_header()).
    */
    int file_mode;
    sf_count_t_local filelength;
    int channels;

    klee_make_symbolic(&file_mode, sizeof(file_mode), "file_mode");
    klee_make_symbolic(&filelength, sizeof(filelength), "filelength");
    klee_make_symbolic(&channels, sizeof(channels), "channels");

    /* Restrict file_mode to the values it can legitimately take so the
    ** predicate's disjuncts are meaningful (READ / WRITE / RDWR). */
    klee_assume(file_mode == SFM_READ || file_mode == SFM_WRITE || file_mode == SFM_RDWR);

    /* Predicate condition (paf.c:118):
    **   (psf->file.mode == SFM_READ ||
    **    (psf->file.mode == SFM_RDWR && psf->filelength > 0))
    **   && psf->filelength < PAF_HEADER_LENGTH
    **
    ** Constrain inputs to the region where the predicate would kill
    ** execution (i.e. where it is *not* satisfied), representing the
    ** case in which paf_read_header() is reached but bails out early
    ** at the "if (psf->filelength < PAF_HEADER_LENGTH) return
    ** SFE_PAF_SHORT_HEADER;" check before ever inspecting paf_fmt.channels.
    */
    klee_assume(!(
        (file_mode == SFM_READ ||
         (file_mode == SFM_RDWR && filelength > 0))
        && filelength < PAF_HEADER_LENGTH
    ));

    /* Canary condition (paf.c:206):
    **   MAGMA_OR(paf_fmt.channels < 1, paf_fmt.channels > SF_MAX_CHANNELS)
    **
    ** If the predicate is sound, then under the assumption above the
    ** canary (bad channel count) should never be observed/violated.
    */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
