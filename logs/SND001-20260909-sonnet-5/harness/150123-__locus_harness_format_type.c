#include "klee/klee.h"

/*
** Constants mirrored from src/sndfile.h / src/common.h so this harness is
** self-contained and does not need the full sfconfig.h build environment.
**
**   SF_FORMAT_PAF        0x00050000   (container type identifying PAF files)
**   SF_FORMAT_TYPEMASK   0xFFFF0000
**   SF_CONTAINER(x)      ((x) & SF_FORMAT_TYPEMASK)
**   SF_MAX_CHANNELS      1024
*/
#define SF_FORMAT_TYPEMASK  0xFFFF0000
#define SF_FORMAT_PAF       0x00050000
#define SF_CONTAINER(x)     ((x) & SF_FORMAT_TYPEMASK)
#define SF_MAX_CHANNELS     1024

/*
** Data flow being modelled (src/sndfile.c psf_open_file(), around line 3117,
** down to src/paf.c paf_read_header(), around line 206):
**
**   psf->sf.format is determined earlier in psf_open_file() (either supplied
**   by the caller for write mode, or computed via guess_file_type() /
**   format_from_extension() for read mode).
**
**   The predicate that used to live at sndfile.c:3117 was an inserted
**   guard of the form:
**
**       if (SF_CONTAINER (psf->sf.format) != SF_FORMAT_PAF)
**           _exit (...);   // "predicate fires" -> kill this path
**
**   i.e. it hypothesised that only paths where the container format equals
**   SF_FORMAT_PAF can be safe to continue exploring, on the theory that any
**   path that is *not* killed (SF_CONTAINER(format) == SF_FORMAT_PAF) is one
**   where the subsequent dispatch switch calls paf_open() -> paf_read_header()
**   and, more importantly, one that should never be able to make the PAF
**   channel-count canary in paf.c fire.
**
**   That hypothesis is unsound: allowing the format-dispatch to reach
**   paf_open()/paf_read_header() (SF_CONTAINER(format) == SF_FORMAT_PAF) says
**   nothing at all about the value of paf_fmt.channels, which is read
**   directly from the (attacker-controlled) file header a few lines later
**   and only range-checked *after* the removed predicate's location. So a
**   file with container format == SF_FORMAT_PAF but a bogus channel count
**   (e.g. 0) sails past the predicate's "safe" branch and still triggers the
**   canary at paf.c:206.
*/

void __locus_klee_harness_format_type(void) {
    int sf_format;   /* psf->sf.format at the predicate's location */
    int channels;    /* paf_fmt.channels, parsed later from the file header */

    klee_make_symbolic(&sf_format, sizeof(sf_format), "sf_format");
    klee_make_symbolic(&channels, sizeof(channels), "channels");

    /* Constrain: predicate would kill iff SF_CONTAINER(sf_format) != SF_FORMAT_PAF.
    ** We take the negation of that kill trigger, i.e. the branch the removed
    ** predicate believed was "safe" and allowed to continue: the container
    ** format really is PAF, so the dispatch switch calls paf_open() ->
    ** paf_read_header(). */
    klee_assume(!(SF_CONTAINER(sf_format) != SF_FORMAT_PAF));

    /* channels is whatever value paf_read_header() happens to read out of
    ** the (symbolic/attacker-controlled) file header; nothing constrains it
    ** at this point in the trace, matching the real code. */

    /* Assert: canary should be unreachable, i.e. channels should always be in
    ** range once we're on the "safe" branch of the (removed) predicate. */
    klee_assert(!(channels < 1 || channels > SF_MAX_CHANNELS));
}
