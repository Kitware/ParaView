#ifndef H_BLAS
#define H_BLAS

#include "TransF77.h"

/** Translations for using Fortran version of blas */
namespace blas {


  extern "C" {
    /// -- BLAS Level 1:
    void   F77NAME(dcopy) (const int& n, const double *x, const int& incx,
         double *y, const int& incy);
    void   F77NAME(daxpy) (const int& n, const double& alpha, const double *x,
         const int& incx, const double *y, const int& incy);
    void   F77NAME(dswap) (const int& n, double *x, const int& incx,
         double *y, const int& incy);
    void   F77NAME(dscal) (const int& n, const double& alpha, double *x,
         const int& incx);
    void   F77NAME(drot)  (const int& n, double *x, const int& incx,
         double *y, const int& incy, const double& c,
         const double& s);
    double F77NAME(ddot)  (const int& n, const double *x,  const int& incx,
         const double *y, const int& incy);
    double F77NAME(dnrm2) (const int& n, const double *x, const int& incx);
    double F77NAME(dasum) (const int& n, const double *x, const int& incx);
    int    F77NAME(idamax)(const int& n, const double *x, const int& incx);

    /// -- BLAS level 2
    void F77NAME(dgemv) (const char& trans,  const int& m,
       const int& n,       const double& alpha,
       const double* a,    const int& lda,
       const double* x,    const int& incx,
       const double& beta, double* y, const int& incy);

    void F77NAME(dspmv) (const char& trans, const int& n,    const double& alpha,
       const double* a,   const double* x, const int& incx,
       const double& beta,      double* y, const int& incy);

    /// -- BLAS level 3:
    void F77NAME(dgemm) (const char& trans,   const char& transb,
       const int& m1,       const int& n,
       const int& k,        const double& alpha,
       const double* a,     const int& lda,
       const double* b,     const int& ldb,
       const double& beta,  double* c, const int& ldc);
  }

  /// - BLAS level 1: Copy \a x to \a y
  static void dcopy (const int& n, const double *x, const int& incx,
         double *y, const int& incy){
    F77NAME(dcopy)(n,x,incx,y,incy);}

  /// - BLAS level 1: y = alpha \a x plus \a y
  static void daxpy (const int& n, const double& alpha, const double *x,
         const int& incx,  const double *y, const int& incy){
    F77NAME(daxpy)(n,alpha,x,incx,y,incy);}
  /// - BLAS level 1: Swap \a x with  \a y
  static void dswap (const int& n,double *x, const int& incx,
         double *y, const int& incy){
    F77NAME(dswap)(n,x,incx,y,incy);}
  /// - BLAS level 1: x = alpha \a x
  static void dscal (const int& n, const double& alpha, double *x,
         const int& incx){
    F77NAME(dscal)(n,alpha,x,incx);}
  /// - BLAS level 1: Plane rotation by c = cos(theta), s = sin(theta)
  static void drot (const int& n,  double *x,  const int& incx,
        double *y, const int& incy, const double& c,
        const double& s){
    F77NAME(drot)(n,x,incx,y,incy,c,s);}
  /// - BLAS level 1: output =   \f$ x^T  y \f$
  static double ddot (const int& n, const double *x, const int& incx,
          const double *y, const int& incy){
    return F77NAME(ddot)(n,x,incx,y,incy);}
  /// - BLAS level 1: output = \f$ ||x||_2 \f$
  static double dnrm2 (const int& n, const double *x, const int& incx){
    return F77NAME(dnrm2)(n,x,incx);}
  /// - BLAS level 1: output = \f$ ||x||_1 \f$
  static double dasum (const int& n, const double *x, const int& incx){
    return F77NAME(dasum)(n,x,incx);}

  /// - BLAS level 1: output = 1st value where \f$ |x[i]| = max |x|_1 \f$
  /// Note it is modified to return a value between (0,n-1) as per
  /// the standard C convention
  static int idamax (const int& n, const double *x,  const int& incx){
    return F77NAME(idamax)(n,x,incx) -1;
  }
  /// - BLAS level 2: Matrix vector multiply y = A \e x where A[m x n]
  static void dgemv (const char& trans,   const int& m,    const int& n,
         const double& alpha, const double* a, const int& lda,
         const double* x,     const int& incx, const double& beta,
         double* y,     const int& incy) {
    F77NAME(dgemv) (trans,m,n,alpha,a,lda,x,incx,beta,y,incy);
  }

  static void dspmv (const char& trans,  const int& n,    const double& alpha,
         const double* a,    const double* x, const int& incx,
         const double& beta,       double* y, const int& incy){
    F77NAME(dspmv) (trans,n,alpha,a,x,incx,beta,y,incy);
  }


  /// - BLAS level 3: Matrix-matrix multiply C = A x B where A[m x n],
  ///   B[n x k], C[m x k]
  static void dgemm (const char& transa,  const char& transb, const int& m,
        const int& n,        const int& k,       const double& alpha,
        const double* a,     const int& lda,     const double* b,
        const int& ldb,      const double& beta,       double* c,
        const int& ldc) {
    F77NAME(dgemm) (transa,transb,m,n,k,alpha,a,lda,b,ldb,beta,c,ldc);
  }

  // Wrapper to mutliply two (row major) matrices together C = a*A*B + b*C
  static void Cdgemm(const int M, const int N, const int K, const double a,
        double *A, const int ldA, double * B, const int ldB,
        const double b, double *C, const int ldC){
    dgemm('N','N',N,M,K,a,B,ldB,A,ldA,b,C,ldC) ;
  }
}

#endif
