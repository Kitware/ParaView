/*
 * Macro definitions for FORTRAN Linpack routines
 * ------------------------------------------------------------------------ */

#ifndef  LINPACK
#define  LINPACK

#ifndef VECLIB_H
#include <veclib.h>
#endif /* ifndef VECLIB_H */

#if UNDERSCORE             /* Fortran routines need underscores */
#
void dppco_(double *a, int *n, double *rcond, double *z, int *info);
#define dppco(a,n,rcond,z,info)\
  (_vlib_ireg[0]=n, dppco_(a, _vlib_ireg, &rcond, z, &info))
#
void dppsl_(double *a, int *n, double *b);
#define dppsl(a,n,b)\
  (_vlib_ireg[0]=n, dppsl_(a, _vlib_ireg, b))
#
void dppfa_(double *ap, int *n, int *info);
#define dppfa(ap,n,info)\
  (_vlib_ireg[0]=n, dppfa_(ap, _vlib_ireg, &info))
#
void dpbco_(double *abd, int *lda, int *n, int *m, double *rcond,
           double *z, int *info);
#define dpbco(abd,lda,n,m,rcond,z,info)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,_vlib_ireg[2]=m,\
   dpbco_(abd,_vlib_ireg,_vlib_ireg+1,_vlib_ireg+2, &rcond, z, &info))
#
void dpbsl_(double *abd, int *lda, int *n, int *m, double *b);
#define dpbsl(abd,lda,n,m,b)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,_vlib_ireg[2]=m,\
   dpbsl_(abd,_vlib_ireg,_vlib_ireg+1,_vlib_ireg+2,b))
#
void dpbfa_(double *abd, int *lda, int *n, int *m, int *info);
#define dpbfa(abd,lda,n,m,info)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,_vlib_ireg[2]=m,\
   dpbfa_(abd,_vlib_ireg,_vlib_ireg+1,_vlib_ireg+2,&info))
#
void dgeco_(double *a, int *n, int *m, int *ipvt, double *rcond, double *b);
#define dgeco(a,n,m,ipvt,rcond,b)\
  (_vlib_ireg[0]=n,_vlib_ireg[1]=m,\
   dgeco_(a,_vlib_ireg,_vlib_ireg+1,ipvt,&rcond, b))
#
void dgesl_(double *a, int *lda, int *n, int *ipvt, double *b, int *job);
#define dgesl(a,lda,n,ipvt,b,job)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,_vlib_ireg[2]=job,\
   dgesl_(a,_vlib_ireg,_vlib_ireg+1,ipvt,b,_vlib_ireg+2))
#
void dgefa_(double *a, int *lda, int *n, int *ipvt, int *info);
#define dgefa(a,lda,n,ipvt,info)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,\
   dgefa_(a,_vlib_ireg,_vlib_ireg+1,ipvt,&info))
#
#else                      /* Fortran routines need no underscores */
#
#if defined(__hpux) || defined(_AIX) || defined(__blrts__) || defined(__bg__)
#
void dppco(double *a, int *n, double *rcond, double *z, int *info);
#define dppco(a,n,rcond,z,info)\
  (_vlib_ireg[0]=n, dppco(a, _vlib_ireg, &rcond, z, &info))
#
void dppsl(double *a, int *n, double *b);
#define dppsl(a,n,b)\
  (_vlib_ireg[0]=n, dppsl(a, _vlib_ireg, b))
#
void dppfa(double *ap, int *n, int *info);
#define dppfa(ap,n,info)\
  (_vlib_ireg[0]=n, dppfa(ap, _vlib_ireg, &info))
#
void dpbco(double *abd, int *lda, int *n, int *m, double *rcond,
           double *z, int *info);
#define dpbco(abd,lda,n,m,rcond,z,info)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,_vlib_ireg[2]=m,\
   dpbco(abd,_vlib_ireg,_vlib_ireg+1,_vlib_ireg+2, &rcond, z, &info))
#
void dpbsl(double *abd, int *lda, int *n, int *m, double *b);
#define dpbsl(abd,lda,n,m,b)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,_vlib_ireg[2]=m,\
   dpbsl(abd,_vlib_ireg,_vlib_ireg+1,_vlib_ireg+2,b))
#
void dpbfa(double *abd, int *lda, int *n, int *m, int *info);
#define dpbfa(abd,lda,n,m,info)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,_vlib_ireg[2]=m,\
   dpbfa(abd,_vlib_ireg,_vlib_ireg+1,_vlib_ireg+2,&info))
#
void dgeco(double *a, int *n, int *m, int *ipvt, double *rcond, double *b);
#define dgeco(a,n,m,ipvt,rcond,b)\
  (_vlib_ireg[0]=n,_vlib_ireg[1]=m,\
   dgeco(a,_vlib_ireg,_vlib_ireg+1,ipvt,&rcond, b))
#
void dgesl(double *a, int *lda, int *n, int *ipvt, double *b, int *job);
#define dgesl(a,lda,n,ipvt,b,job)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,_vlib_ireg[2]=job,\
   dgesl(a,_vlib_ireg,_vlib_ireg+1,ipvt,b,_vlib_ireg+2))
#
void dgefa(double *a, int *lda, int *n, int *ipvt, int *info);
#define dgefa(a,lda,n,ipvt,info)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,\
   dgefa(a,_vlib_ireg,_vlib_ireg+1,ipvt,&info))
#
#else                      /* Either Cray or error */
#
#ifdef _CRAY
#
void SPPCO(double *a, int *n, double *rcond, double *z, int *info);
#define dppco(a,n,rcond,z,info)\
  (_vlib_ireg[0]=n, SPPCO(a, _vlib_ireg, &rcond, z, &info))
#
void SPPSL(double *a, int *n, double *b);
#define dppsl(a,n,b)\
  (_vlib_ireg[0]=n, SPPSL(a, _vlib_ireg, b))
#
void SPPFA(double *ap, int *n, int *info);
#define dppfa(ap,n,info)\
  (_vlib_ireg[0]=n, SPPFA(ap, _vlib_ireg, &info))
#
void SPBCO(double *abd, int *lda, int *n, int *m, double *rcond,
           double *z, int *info);
#define dpbco(abd,lda,n,m,rcond,z,info)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,_vlib_ireg[2]=m,\
   SPBCO(abd,_vlib_ireg,_vlib_ireg+1,_vlib_ireg+2, &rcond, z, &info))
#
void SPBSL(double *abd, int *lda, int *n, int *m, double *b);
#define dpbsl(abd,lda,n,m,b)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,_vlib_ireg[2]=m,\
   SPBSL(abd,_vlib_ireg,_vlib_ireg+1,_vlib_ireg+2,b))
#
void SPBFA(double *abd, int *lda, int *n, int *m, int *info);
#define dpbfa(abd,lda,n,m,info)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,_vlib_ireg[2]=m,\
   SPBFA(abd,_vlib_ireg,_vlib_ireg+1,_vlib_ireg+2,&info))
#
void SGECO(double *a, int *n, int *m, int *ipvt, double *rcond, double *b);
#define dgeco(a,n,m,ipvt,rcond,b)\
  (_vlib_ireg[0]=n,_vlib_ireg[1]=m,\
   SGECO(a,_vlib_ireg,_vlib_ireg+1,ipvt,&rcond, b))
#
void SGESL(double *a, int *lda, int *n, int *ipvt, double *b, int *job);
#define dgesl(a,lda,n,ipvt,b,job)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,_vlib_ireg[2]=job,\
   SGESL(a,_vlib_ireg,_vlib_ireg+1,ipvt,b,_vlib_ireg+2))
#
void SGEFA(double *a, int *lda, int *n, int *ipvt, int *info);
#define dgefa(a,lda,n,ipvt,info)\
  (_vlib_ireg[0]=lda,_vlib_ireg[1]=n,\
   SGEFA(a,_vlib_ireg,_vlib_ireg+1,ipvt,&info))
#
#else
#error Architecture mixup for LINPACK
#endif /* ifdef _CRAY */
#
#endif /* else if defined(__hpux) || defined(_AIX) */
#
#endif /* else if UNDERSCORE */
#
#endif /* ifndef  LINPACK */
