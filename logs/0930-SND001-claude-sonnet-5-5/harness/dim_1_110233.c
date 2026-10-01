#include "klee/klee.h"

#ifndef PAF_HEADER_LENGTH
#define PAF_HEADER_LENGTH 2048
#endif
#ifndef SF_MAX_CHANNELS
#define SF_MAX_CHANNELS 1024
#endif

typedef long long sf_count_t_h;

void __locus_klee_harness_dim_1(void) {
    sf_count_t_h filelength;
    int marker, version, endianness, samplerate, format, channels, source;
    const int PAF_MARKER_V = (' ' << 24) | ('p' << 16) | ('a' << 8) | 'f';
    const int FAP_MARKER_V = ('f' << 24) | ('a' << 16) | ('p' << 8) | ' ';

    klee_make_symbolic(&filelength, sizeof(filelength), "filelength");
    klee_make_symbolic(&marker, sizeof(marker), "marker");
    klee_make_symbolic(&version, sizeof(version), "version");
    klee_make_symbolic(&endianness, sizeof(endianness), "endianness");
    klee_make_symbolic(&samplerate, sizeof(samplerate), "samplerate");
    klee_make_symbolic(&format, sizeof(format), "format");
    klee_make_symbolic(&channels, sizeof(channels), "channels");
    klee_make_symbolic(&source, sizeof(source), "source");

    /* Predicate would kill when filelength < PAF_HEADER_LENGTH; assume survival. */
    klee_assume(!(filelength < PAF_HEADER_LENGTH));

    /* Intermediate path conditions between predicate and canary. */
    klee_assume(marker == PAF_MARKER_V || marker == FAP_MARKER_V);
    klee_assume(version == 0);

    /* Canary should be unreachable. */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
