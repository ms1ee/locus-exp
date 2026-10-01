#include "klee/klee.h"

/* Mirrors SF_MAX_CHANNELS as used in libsndfile's sndfile.h */
#define SF_MAX_CHANNELS 1024

typedef struct
{	int version ;
	int endianness ;
	int samplerate ;
	int format ;
	int channels ;
	int source ;
} PAF_FMT ;

void __locus_klee_harness_value_range(void)
{
	PAF_FMT paf_fmt;
	klee_make_symbolic(&paf_fmt.version, sizeof(paf_fmt.version), "version");
	klee_make_symbolic(&paf_fmt.channels, sizeof(paf_fmt.channels), "channels");

	/* The removed predicate asserted: paf_fmt.version must equal 0.
	 * Constrain to the case where the predicate would have killed
	 * execution, i.e. version != 0. */
	klee_assume(!(paf_fmt.version == 0));

	/* Between the predicate and the canary, there is no code that
	 * validates or otherwise constrains paf_fmt.channels based on
	 * paf_fmt.version, so an invalid channel count can still reach
	 * the canary check even though the (removed) version predicate
	 * would have killed this execution. */
	klee_assert(!(paf_fmt.channels < 1 || paf_fmt.channels > SF_MAX_CHANNELS));
}
