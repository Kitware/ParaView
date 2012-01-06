/* ------------------------------------------------------------------------- *
 * Architecture-dependent Routines                                           *
 *                                                                           *
 * This section re-defines some of the library routines based on the machine *
 * architecture and compiler system.                                         *
 * ------------------------------------------------------------------------- */

#if defined(_AIX)
#
#ifndef VECLIB_H
#include <veclib.h>
#endif
#
#if GATHER_SCATTER
#
void   dgthr(int*, double *, double *, int *);
# define dgathr(n,x,y,z)  \
 (_vlib_ireg[0] = n, dgthr (_vlib_ireg,x,z,y))
void   dsctr(int*, double *, int *, double *);
# define dscatr(n,x,y,z)  \
 (_vlib_ireg[0] = n, dsctr (_vlib_ireg,x,y,z))
#
#endif
#
#endif
