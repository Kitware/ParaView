#ifndef ZBESJ_H
#define ZBESJ_H
/*
// Header conversion file for zbesj - (Complex bessel functions)
*/

#include "TransF77.h"

void F77NAME(zbesj) (const double *, const double *, const double *,
         const int *, const int *, double *, double *,
         int *, int *);

void zbesj (const double *x, const double *y, const double ord,
      const int  Kode, const int n, double *ReJ,
      double *ImJ, int*  nz, int*  ierr){
  const int N = n;
  int K = Kode;
  double  order = ord;

  F77NAME(zbesj) (x, y, &order, &K, &N, ReJ, ImJ, nz,ierr);
  if(ierr[0]) fprintf(stderr,"Error %d in zbesj\n",ierr[0]);
}

#endif
