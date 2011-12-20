/* ------------------------------------------------------------------------- *
 * Architecture-dependent Routines                                           *
 *                                                                           *
 * This section re-defines some of the library routines based on the machine *
 * architecture and compiler system.                                         *
 * ------------------------------------------------------------------------- */

#if defined(__sgi)
#
#ifndef VECLIB_H
#include <veclib.h>
#endif
#
#if GATHER_SCATTER
#
void   gather_(int*, double *, double *, int *);
# define dgathr(n,x,y,z)  \
 (_vlib_ireg[0] = n, gather_ (_vlib_ireg,z,x,y))
void scatter_(int*, double *, int *, double *);
# define dscatr(n,x,y,z)  \
 (_vlib_ireg[0] = n, scatter_ (_vlib_ireg,z,y,x))
#
#endif
#
#if SCILIB && !MATRIX_BLAS
#
void   mxm_   (double *a, int *nra, double *b, int *nca, double *c, int *ncb);
void   mxv_   (double *a, int *nra, double *b, int *nca, double *ab);
void   mxma_  (double *a, int *iac, int *iar, double *b, int *ibc, int *ibr,
         double *c, int *icc, int *icr, int *nra, int *nca, int *ncb);
void   mxva_  (double *a, int *iac, int *iar, double *b, int *ib,
         double *c, int *ic, int *nra, int *nca);
#
# define  mxm(a,nra,b,nca,c,ncb)\
  (_vlib_ireg[0]=ncb,_vlib_ireg[1]=nca,_vlib_ireg[2]=nra,\
   mxm_(b,_vlib_ireg,a,_vlib_ireg+1,c,_vlib_ireg+2))
#
# define  mxv(a,nra,b,nca,c)\
  (_vlib_ireg[0]=nra,_vlib_ireg[1]=nca,_vlib_ireg[2]=1,\
   mxva_(a,_vlib_ireg+1,_vlib_ireg+2,b,_vlib_ireg+2,c,_vlib_ireg+2,\
   _vlib_ireg,_vlib_ireg+1))
#
# define  mxma(a,iac,iar,b,ibc,ibr,c,icc,icr,nra,nca,ncb)\
  (_vlib_ireg[0]=iac,_vlib_ireg[1]=iar,_vlib_ireg[2]=ibc,_vlib_ireg[3]=ibr,\
   _vlib_ireg[4]=icc,_vlib_ireg[5]=icr,_vlib_ireg[6]=nra,_vlib_ireg[7]=nca,\
   _vlib_ireg[8]=ncb,\
   mxma_(a,_vlib_ireg,_vlib_ireg+1,b,_vlib_ireg+2,_vlib_ireg+3,\
   c,_vlib_ireg+4,_vlib_ireg+5,_vlib_ireg+6,_vlib_ireg+7,_vlib_ireg+8))
#
# define  mxva(a,iac,iar,b,ib,c,ic,nra,nca)\
  (_vlib_ireg[0]=iac,_vlib_ireg[1]=iar,_vlib_ireg[2]=ib,_vlib_ireg[3]=ic,\
   _vlib_ireg[4]=nra,_vlib_ireg[5]=nca,\
   mxva_(a,_vlib_ireg,_vlib_ireg+1,b,_vlib_ireg+2,c,_vlib_ireg+3,\
   _vlib_ireg+4,_vlib_ireg+5))
#
#endif
#
#endif
