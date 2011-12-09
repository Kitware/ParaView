#ifndef COMPLEX_H   /* BEGIN complex.h DECLARATIONS */
#define COMPLEX_H

/*  Definition of complex data types */

typedef struct zcmplx {           /* Type definitions for complex numbers */
  double r;
  double i;
} zcomplex;

typedef struct ccmplx {          /* Note change in name from cmplx to avoid */
  float r;                       /* conflicts with essl.h/blas.h defs      */
  float i;
} complex;

#endif
