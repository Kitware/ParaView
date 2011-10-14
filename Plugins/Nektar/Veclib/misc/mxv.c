/*
 * mxv() - Matrix - Vector Multiply (double precision)
 *
 * This following function computes the matrix-vector product C = A * B.
 * The matrix A is assumed to be stored in row-major order and must oc-
 * cupy consectutive memory locations.
 *
 *      mxv(A,nra,B,nca,C)
 *
 *      A   ... double* ... matrix factor (input)
 *      nra ... int     ... number of rows in A
 *      B   ... double* ... vector factor (input)
 *      nca ... int     ... number of columns in A
 *      C   ... double* ... vector product (output)
 *
 * A more general matrix-vector multiply with arbitrary skips is given in
 * mxva().
 */

#if !defined(mxv)

void mxv(double* A, int *Inra, double* B, int *Inca, double *C){
  register double *a = A,
                  *c = C;
  register double  sum;
  register int     i, j;
  int nra = *Inra;
  int nca = *Inca;

  for(i = 0; i < nra; i++) {
    sum  = 0.;
    for (j = 0; j < nca; j++) sum += (*a++) * B[j];
    *c++ = sum;
  }

  return;
}

#endif
