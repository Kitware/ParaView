#ifndef  H_LAPACK
#define  H_LAPACK

#include "TransF77.h"

extern "C" {
  // Matrix factorisation and solves
  void F77NAME(dsptrf) (const char& uplo, const int& n,
      double* ap, int *ipiv, int& info);
  void F77NAME(dsptrs) (const char& uplo, const int& n,
      const int& nrhs, const double* ap,
      const int  *ipiv, double* b,
      const int& ldb, int& info);
  void F77NAME(dpptrf) (const char& uplo, const int& n,
      double* ap, int& info);
  void F77NAME(dpptrs) (const char& uplo, const int& n,
      const int& nrhs, const double* ap,
      double* b, const int& ldb, int& info);
  void F77NAME(dpbtrf) (const char& uplo, const int& n, const int& kd,
      double* ab, const int& ldab, int& info);
  void F77NAME(dpptri) (const char& uplo, const int& n,
                        double* ap, int& info);
  void F77NAME(dpbtrs) (const char& uplo, const int& n,
      const int& kd, const int& nrhs,
      const double* ab, const int& ldab,
      double* b, const int& ldb, int& info);
  void F77NAME(dgbtrf) (const int& m, const int& n, const int& kl,
      const int& ku, double* a, const int& lda,
      int* ipiv, int& info);
  void F77NAME(dgbtrs) (const char& trans, const int& n, const int& kl,
      const int &ku, const int& nrhs,  const double* a,
      const int& lda, int* ipiv, double* b,
      const int& ldb, int& info);
  void F77NAME(dgetrf) (const int& m, const int& n, double* a,
      const int& lda, int* ipiv, int& info);
  void F77NAME(dgetrs) (const char& trans, const int& n, const int& nrhs,
      const double* a,   const int& lda, int* ipiv,
      double* b, const int& ldb, int& info);
  void F77NAME(dgetri) (const int& n, double *a, const int& lda,
      const int *ipiv, double *wk,  const int& lwk,
      int& info);
  void F77NAME(dsterf) (const int& n, double *d, double *e, int& info);
  void F77NAME(dgeev)  (const char& uplo, const char& lrev, const int& n,
      double* a, const int& lda, double* wr, double* wi,
      double* rev,  const int& ldr,
      double* lev,  const int& ldv,
      double* work, const int& lwork, int& info);

  void F77NAME(dspev)  (const char& jobz, const char& uplo, const int& n,
      double* ap, double* w, double* z, const int& ldz,
      double* work, int& info);
  void F77NAME(dsbev)  (const char& jobz, const char& uplo, const int& kl,
      const int& ku,  double* ap, const int& lda,
      double* w, double* z, const int& ldz,
      double* work, int& info);
}

namespace lapack {

  /// factor a real packed-symmetric matrix using Bunch-Kaufman pivoting.
  static void dsptrf (const char& uplo, const int& n,
         double* ap, int *ipiv, int& info){
    F77NAME(dsptrf) (uplo,n,ap,ipiv,info);
  }
  /// Solve a real  symmetric matrix problem using Bunch-Kaufman pivoting.
  static void dsptrs (const char& uplo, const int& n, const int& nrhs,
         const double* ap, const int  *ipiv, double* b,
         const int& ldb, int& info){
    F77NAME(dsptrs) (uplo,n,nrhs,ap,ipiv,b,ldb,info);
  }

  /// Cholesky factor a real Positive Definite packed-symmetric matrix.
  static void dpptrf (const char& uplo, const int& n,
          double *ap, int& info) {
    F77NAME(dpptrf) (uplo,n,ap,info);
  }
   ///    DPPTRI  -  the  inverse  of a real symmetric positive definite matrix A
   ///    using the Cholesky factorization A = U**T*U or A = L*L**T  computed  by
   ///    DPPTRF
   static void dpptri (const char& uplo, const int& n,
                      double *ap, int& info) {
    F77NAME(dpptri) (uplo,n,ap,info);
  }
  /// Solve a real Positive defiinte symmetric matrix problem using
  /// Cholesky factorization.
  static void dpptrs (const char& uplo, const int& n, const int& nrhs,
         const double *ap, double *b, const int& ldb,
         int& info) {
    F77NAME(dpptrs) (uplo,n,nrhs,ap,b,ldb,info);
  }
  /// Cholesky factorize a real positive-definite banded-symmetric matrix
  static void dpbtrf (const char& uplo, const int& n, const int& kd,
         double *ab, const int& ldab, int& info) {
    F77NAME(dpbtrf) (uplo,n,kd,ab,ldab,info);
  }
  /// Solve a real, Positive definite banded symmetric matrix problem
  /// using Cholesky factorization.
  static void dpbtrs (const char& uplo, const int& n,
          const int& kd, const int& nrhs,
          const double *ab, const int& ldab,
          double *b, const int& ldb, int& info) {
    F77NAME(dpbtrs) (uplo,n,kd,nrhs,ab,ldab,b,ldb,info);
  }

  /// General banded matrix LU factorisation
  static void dgbtrf (const int& m, const int& n, const int& kl,
         const int& ku, double* a, const int& lda,
         int* ipiv, int& info){
    F77NAME(dgbtrf)(m,n,kl,ku,a,lda,ipiv,info);
  }
  /// Solve general banded matrix using LU factorisation
  static void dgbtrs (const char& trans, const int& n, const int& kl,
         const int &ku, const int& nrhs,  const double* a,
         const int& lda, int* ipiv, double* b,
         const int& ldb, int& info){
    F77NAME(dgbtrs)(trans,n,kl,ku,nrhs,a,lda,ipiv,b,ldb,info);
  }

  /// General matrix LU factorisation
  static void dgetrf (const int& m, const int& n, double *a,
          const int& lda, int *ipiv, int& info) {
    F77NAME(dgetrf) (m,n,a,lda,ipiv,info);
  }
  /// General matrix LU backsolve
  static void dgetrs (const char& trans, const int& n, const int& nrhs,
         const double* a, const int& lda, int* ipiv,
         double* b, const int& ldb, int& info) {
    F77NAME(dgetrs) (trans,n,nrhs,a,lda,ipiv,b,ldb,info);
  }

  /// Generate matrix inverse
  static void dgetri (const int& n, double *a, const int& lda,
         const int *ipiv, double *wk,  const int& lwk, int& info){
    F77NAME(dgetri) (n, a, lda, ipiv, wk, lwk,info);
  }


  //  -- Find eigenvalues of symmetric tridiagonal matrix
  static void dsterf(const int& n, double *d, double *e, int& info){
    F77NAME(dsterf)(n,d,e,info);
  }

  // -- Solve general real matrix eigenproblem.
  static void dgeev (const char& uplo, const char& lrev, const int& n,
        double* a, const int& lda, double* wr, double* wi,
        double* rev,  const int& ldr,
        double* lev,  const int& ldv,
        double* work, const int& lwork, int& info) {
    F77NAME(dgeev) (uplo, lrev, n, a, lda, wr, wi, rev,
       ldr, lev, ldv, work, lwork, info);
  }

  // -- Solve packed-symmetric real matrix eigenproblem.

  static void dspev (const char& jobz, const char& uplo, const int& n,
        double* ap, double* w, double* z, const int& ldz,
        double* work, int& info) {
    F77NAME(dspev) (jobz, uplo, n, ap, w, z, ldz, work, info);
  }

  // -- Solve packed-banded real matrix eigenproblem.

  static void dsbev (const char& jobz, const char& uplo, const int& kl,
         const int& ku,  double* ap, const int& lda,
         double* w, double* z, const int& ldz,
         double* work, int& info) {
    F77NAME(dsbev) (jobz, uplo, kl, ku, ap, lda, w, z, ldz, work, info);
  }

}


#endif
