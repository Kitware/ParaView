/*

   VECLIB -- Vector Processing Library

   This is a collection of functions for numerical and other types of opera-
   tions on arrays.  It mimics libraries supplied for typical vector machines
   and allows programers to concentrate on higher-level operations rather than
   writing loops.

   The library is divided into several sections.

   BLAS 1-3     These are the Basic Linear Algebra Subroutines, originally
                defined by Dongerra et al.  These are intended to be FORTRAN
          subroutines.  Macros for linkage from a C program are included
          in <veclib.h>, but the conventions on matrix storage, etc. all
          follow FORTRAN conventions.  Most computer manufacturers pro-
          vide optimized versions of the BLAS routines, so their librar-
                ies should be linked first.

    Math

    Triads

    Memory

    Misc

 * ------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include "vecerr.h"

#ifdef _CRAY
_fcd _f1, _f2, _f3, _f4;
#endif

static char *vecerr_msg[] = {
  "no message",
  "memory allocation failed"
};


void vecerr (char *s, int errno)
{
  fprintf(stderr, "veclib error in %s: %s\n", s, vecerr_msg[errno-100]);
  if (errno > vFATAL) exit(errno);
  return;
}
