/*
 * Vector Library
 *
 * RCS Information
 * -------------------------
 * $Author: ssherw $
 * $Date: 2006/05/13 09:15:23 $
 * $Revision: 1.3 $
 * $Source: /homedir/cvs/Nektar/include/veclib.h,v $
 * ------------------------------------------------------------------------- */

#ifndef VECLIB_H   /* BEGIN VECLIB.H DECLARATIONS */
#define VECLIB_H

/* ======================================================================== */

#ifndef BLAS
#define BLAS   3                  /* Full translation for all levels of BLAS */
#endif

#if defined (__hpux) || defined (_CRAY)
#define UNDERSCORE 0              /* Fortran routines need no underscore */
#else
#define UNDERSCORE 1              /* Fortran routines need an underscore */
#endif

#define MATRIX_BLAS    0          /* BLAS translations for mxm/mxv routines */

#if defined(_CRAY) || defined (__hpux) || defined (__sgi)
#if MATRIX_BLAS                   /* No SCILIB calls if BLAS translated     */
#define SCILIB         0          /* extra SCILIB fortran mxm/mxv routines  */
#else                             /* present on Cray PVPs, SGI and Convex   */
#define SCILIB         0          /* platforms only.                        */
#endif
#endif

#define CPUTIME        1          /* dclock definition for CRAYs            */

/*
 * On Cray PVP platforms set this to 0 as the hardware gather/scatter is
 * supposed to be faster
 */

#define GATHER_SCATTER 0          /* Use Gather/Scatter optimized routines */

/* ======================================================================== */

#ifdef __cplusplus
extern "C"
{
#endif /* ifdef __cplusplus */

/* ----------------- SECTION 1: Mathematical Primatives -------------------- */

double drand  (void);

void   ifill  (int n, int    a, int     *x, int incx);
void   dfill  (int n, double a, double  *x, int incx);

void   dsadd  (int n, double a, double  *x, int incx, double  *y, int incy);
void   dsmul  (int n, double a, double  *x, int incx, double  *y, int incy);
void   dsdiv  (int n, double a, double  *x, int incx, double  *y, int incy);

void   izero  (int n, int     *x, int incx);
void   dzero  (int n, double  *x, int incx);
void   sneg   (int n, float   *x, int incx);
void   dneg   (int n, double  *x, int incx);
void   dvrand (int n, double  *x, int incx);

void   dvneg  (int n, double  *x, int incx, double  *y, int incy);
void   dvrecp (int n, double  *x, int incx, double  *y, int incy);
void   dvabs  (int n, double  *x, int incx, double  *y, int incy);
void   dvlg10 (int n, double  *x, int incx, double  *y, int incy);
void   dvcos  (int n, double  *x, int incx, double  *y, int incy);
void   dvsin  (int n, double  *x, int incx, double  *y, int incy);
void   dvsqrt (int n, double  *x, int incx, double  *y, int incy);
void   dvexp  (int n, double  *x, int incx, double  *y, int incy);

void   dvamax (int n, double  *x, int incx, double  *y, int incy,
                double  *z, int incz);
void   dvdiv  (int n, double  *x, int incx, double  *y, int incy,
                double  *z, int incz);
void   dvadd  (int n, double  *x, int incx, double  *y, int incy,
                double  *z, int incz);
void   dvsub  (int n, double  *x, int incx, double  *y, int incy,
                double  *z, int incz);
void   dvmul  (int n, double  *x, int incx, double  *y, int incy,
                double  *z, int incz);
void   dvmul_inc1(int n, double *x, double *y, double *z);


/* -------------------- SECTION 2: Triad Operations ------------------------ */

void   dsvtvm (int n, double a, double *x, int incx, double *y, int incy,
                          double *z, int incz);
void   dsvtvp (int n, double a, double *x, int incx, double *y, int incy,
                          double *z, int incz);
void   dsvvmt (int n, double a, double *x, int incx, double *y, int incy,
                          double *z, int incz);
void   dsvvpt (int n, double a, double *x, int incx, double *y, int incy,
                          double *z, int incz);
void   dsvvtm (int n, double a, double *x, int incx, double *y, int incy,
                          double *z, int incz);
void   dsvvtp (int n, double a, double *x, int incx, double *y, int incy,
                          double *z, int incz);
void   dsvtsp (int n, double a, double b, double *x, int incx,
                                    double *y, int incy);

void   dvvtvp (int n, double *w, int incw, double *x, int incx,
                double *y, int incy, double *z, int incz);
void   dvvtvm (int n, double *w, int incw, double *x, int incx,
                double *y, int incy, double *z, int incz);
void   dvvpvt (int n, double *w, int incw, double *x, int incx,
                double *y, int incy, double *z, int incz);
void   dvvmvt (int n, double *w, int incw, double *x, int incx,
                double *y, int incy, double *z, int incz);
void   dvvvtm (int n, double *w, int incw, double *x, int incx,
                double *y, int incy, double *z, int incz);

/* ------------------ SECTION 3: Relational Primitives --------------------- */

void   iseq   (int n, int    *a, int    *x, int incx, int *y, int incy);
void   dsle   (int n, double *a, double *x, int incx, int *y, int incy);
void   dslt   (int n, double *a, double *x, int incx, int *y, int incy);
void   dsne   (int n, double *a, double *x, int incx, int *y, int incy);

/* ------------------- SECTION 4: Reduction Functions ---------------------- */

int    isum   (int n, int    *x, int incx);
double dsum   (int n, double *x, int incx);
double dvpoly (int n, double *x, int incx, int m, double *c, int incc,
                double *y, int incy);
int    idmax  (int n, double *x, int incx);
int    ismax  (int n, float  *x, int incx);
int    iimax  (int n, int    *x, int incx);
int    idmin  (int n, double *x, int incx);
int    ismin  (int n, float  *x, int incx);
int    icount (int n, int    *x, int incx);
int    ifirst (int n, int    *x, int incx);

/* ------------------ SECTION 5: Conversion Primitives --------------------- */

void   dvfloa (int n, int      *x, int incx, double *y, int incy);
void   svfloa (int n, int      *x, int incx, float  *y, int incy);
void   vsngl  (int n, double   *x, int incx, float  *y, int incy);

void   dbrev  (int n, double *x, int incx, double *y, int incy);
void   sbrev  (int n, float  *x, int incx, float  *y, int incy);
void   ibrev  (int n, int    *x, int incx, int    *y, int incy);

/* ----------------- SECTION 6: Miscellaneous Functions -------------------- */

void   dscatr (int n, double  *x, int *y, double  *z);
void   sscatr (int n, float   *x, int *y, float   *z);
void   dgathr (int n, double  *x, int *y, double  *z);
void   sgathr (int n, float   *x, int *y, float   *z);
void   dramp  (int n, double  *a, double *b, double *x, int incx);
void   iramp  (int n, int     *a, int    *b, int    *x, int incx);
void   dcndst (int n, double  *x, int incx, int *y, int incy,
                double  *z, int incz);
void   dmask  (int n, double  *w, int incw, double *x, int incx,
                int     *y, int incy, double *z, int incz);
double dpoly  (int n, double xp, double *x, double *y);
void   polint (double *x, double *y, int n, double xp, double *v, double *err);
void   spline (int n, double yp1, double ypn, double *x, double *y,double *y2);

double splint (int n, double x, double *xp, double *yp, double *ypp);
double dclock (void);

void   realft (int n, double *f, int dir);
void   fftdf  (double*,int*,int*,int*,int*,int*,double*,int*,int*,int*,int*);
void   zbesj_ (double *, double *, double *, int *, int *,double *,
         double *, int *, int *);

#if defined (_CRAY)
#define d1mach_ D1MACH
#define i1mach_ I1MACH
#endif

double d1mach_(long *i);
long i1mach_(long *i);

/* --------------- SECTION 7: Memory Management Functions ------------------ */

int      *ivector     (int rmin, int rmax);
float    *svector     (int rmin, int rmax);
double   *dvector     (int rmin, int rmax);

int     **imatrix     (int rmin, int rmax, int cmin, int cmax);
float   **smatrix     (int rmin, int rmax, int cmin, int cmax);
double  **dmatrix     (int rmin, int rmax, int cmin, int cmax);

void     free_dvector (double *v, int rmin);
void     free_svector (float  *v, int rmin);
void     free_ivector (int    *v, int rmin);

void     free_dmatrix (double **mat, int rmin, int cmin);
void     free_smatrix (float  **mat, int rmin, int cmin);
void     free_imatrix (int    **mat, int rmin, int cmin);

void     vecerr       (char *sec, int msgid);

/* ------------------------------------------------------------------------- *
 *                   BLAS: Basic Linear Algebra Subroutines                  *
 * ------------------------------------------------------------------------- */


#include <Blas.h>
using namespace blas;

/* ------------------------------------------------------------------------- *
 *       non-BLAS convention vector copies for negative increments           *
 * ------------------------------------------------------------------------- */

void     icopy        (int n, int *x, int incx, int *y, int incy);
void     mscopy       (int n, float *x, int incx, float *y, int incy);
void     mdcopy       (int n, double *x, int incx, double *y, int incy);


#include <Lapack.h>
using namespace lapack;


/* -------------------extra matrix multiply routines ----------------------- */

#if !SCILIB      /* If no Fortran versions then C prototypes
                    of the mxm/mxv calls */
#if !MATRIX_BLAS /* These two are translated otherwise */
void   mxm    (const double *a, const int& nra, const double *b,
         const int& nca, double *c, const int& ncb);
void   mxv    (const double *a, const int& nra, const double *b,
         const int& nca, double *ab);
#endif
void   mxma   (const double *a, const int& iac, const int& iar,
         const double *b, const int& ibc, const int& ibr,
         double *c, const int& icc, const int& icr,
         const int& nra, const int& nca, const int& ncb);
void   mxva   (const double *a, const int& iac, const int& iar,
         const double *b, const int& ib,  double *c, const int& ic,
         const int& nra,  const int& nca);
#endif

#if MATRIX_BLAS                      /* Translate mxm/mxv to BLAS */
#
#ifdef _CRAY                         /* CRAYs */
#
#
  void  mxm    (const double *a, const int& nra, const double *b,
    const int& nca, double *c, const int& ncb){

    char   trans='N';
    double oned=1.0,zero=0.0;

    SGEMM(_cptofcd(trans,1),_cptofcd(trans,1),ncb,
    nra,nca,&one, b,ncb,a,nca,&zero,c,ncb);
  }

  void   mxv    (const double *a, const int& nra, const double *b,
     const int& nca, double *ab);
  char trans='T';
  double oned = =1.0, zero =0.0;
  int one = 1;

  SGEMV(_cptofcd(trans,1),nra,nca,&oned,a,nra,b,&one,&zero,c,&one);
}
#else
#
# define  mxm(a,nra,b,nca,c,ncb)\
  (F77NAME(dgemm)('N','N',ncb,nra,nca,1.0,b,ncb,a,nca,0.0,c,ncb))
#
# define  mxv(a,nra,b,nca,c)\
  (f77NAME(dgemv) ('T',nra,nca,1.0,a,nra,b,1,0.0,c,1))
#
#
#endif
#
#endif


#ifndef  tempVector
#define  tempVector(v,n)      double *v = dvector(0,n)
#define  freeVector(v)        free (v)
#endif

#ifdef __cplusplus
}
#endif /* ifdef __cplusplus */

#endif           /* END OF VECLIB.H DECLARATIONS */
