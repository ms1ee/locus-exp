#include "klee/klee.h"

#define SFM_READ  0x10
#define SFM_WRITE 0x20
#define SFM_RDWR  0x30
#define SF_MAX_CHANNELS 1024

void __locus_klee_harness_dim_1(void) {
    int mode;
    long long filelength;
    int channels;   /* paf_fmt.channels, only assigned by paf_read_header */
    int marker_ok, version;

    klee_make_symbolic(&mode, sizeof(mode), "mode");
    klee_make_symbolic(&filelength, sizeof(filelength), "filelength");
    klee_make_symbolic(&channels, sizeof(channels), "channels");
    klee_make_symbolic(&marker_ok, sizeof(marker_ok), "marker_ok");
    klee_make_symbolic(&version, sizeof(version), "version");

    /* Inputs the predicate kills: the code does
       if (!(mode == SFM_READ || mode == SFM_RDWR)) _exit(1);
       so the kill set is where its condition is true. */
    klee_assume(!(mode == SFM_READ || mode == SFM_RDWR));

    /* Data flow between predicate and canary: paf_read_header (the only place
       paf_fmt.channels is set from the file) is only called for
       READ, or RDWR with filelength > 0. Otherwise the canary is never reached. */
    int reached = 0;
    if (mode == SFM_READ || (mode == SFM_RDWR && filelength > 0)) {
        /* header parsing constraints on the path to the canary */
        if (filelength >= 0x800 && marker_ok && version == 0)
            reached = 1;
    }

    int canary = (channels < 1 || channels > SF_MAX_CHANNELS);

    /* Canary must be unreachable / not triggered on killed inputs */
    klee_assert(!(reached && canary));
}
