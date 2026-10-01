#include "klee/klee.h"

#define SF_MAX_CHANNELS 1024
#define MK(a,b,c,d) ((int)((unsigned)(a) | ((unsigned)(b) << 8) | ((unsigned)(c) << 16) | ((unsigned)(d) << 24)))
#define FAP_MARKER MK('f','a','p',' ')
#define PAF_MARKER MK(' ','p','a','f')

void __locus_klee_harness_dim_2(void) {
    int marker;
    int version, endianness, samplerate, format, channels, source;
    klee_make_symbolic(&marker, sizeof(marker), "marker");
    klee_make_symbolic(&version, sizeof(version), "version");
    klee_make_symbolic(&endianness, sizeof(endianness), "endianness");
    klee_make_symbolic(&samplerate, sizeof(samplerate), "samplerate");
    klee_make_symbolic(&format, sizeof(format), "format");
    klee_make_symbolic(&channels, sizeof(channels), "channels");
    klee_make_symbolic(&source, sizeof(source), "source");

    /* Predicate would kill: negation of predicate condition */
    klee_assume(!(marker != PAF_MARKER && marker != FAP_MARKER));

    /* Path constraint between predicate and canary: version must be zero */
    klee_assume(version == 0);

    /* Canary should be unreachable */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
