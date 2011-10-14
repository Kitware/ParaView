/*
 * mxva() - Matrix - Vector Multiply w/skips (double precision)
 *
 * This following function computes the matrix-vector product C = A * B.
 *
 *      mxva(A,iac,iar,B,ib,C,ic,nra,nca)
 *
 *      A   ... double* ... matrix factor (input)
 *      iac ... int     ... increment in A between column elements
 *      iar ... int     ... increment in A between row elements
 *      B   ... double* ... vector factor (input)
 *      ib  ... int     ... increment in B between consectuve elements
 *      C   ... double* ... vector product (output)
 *      ic  ... int     ... increment in C between consectuve elements
 *      nra ... int     ... number of rows in A
 *      nca ... int     ... number of columns in A
 *
 */

#if !defined(mxva)

void mxva(double* A, int *Iiac, int *Iiar, double* B,
    int *Iib, double *C, int *Iic, int *Inra, int *Inca){

  register double *a, *b,
                  *c = C;
  register double  sum;
  register int     i, j;
  int iac = *Iiac;
  int iar = *Iiar;
  int ib  = *Iib;
  int ic  = *Iic;
  int nra = *Inra;
  int nca = *Inca;

#if 0
  int choice = 0;
  if (iar == 1) choice += 10;
  else if (iar == nra) choice += 20;
  if (iac == 1) choice += 100;
  else if (iac == nca) choice += 200;
  if (ib == 1) choice += 1;
  if (ic == 1) choice += 2;

  switch (choice) {
  case 213: /* c = A*b */
    /* if (iar == 1 && iac == nca && ib == 1 && ic == 1) */
    for (i = 0; i < nra; ++i) {
      C[i] = 0.;
      for (j = 0; j < nca; ++j)
  C[i] += A[i*nca+j]*B[j];
    }
    break;
  case 123: /* c = A^T*b */
    /* if (iar == nra && iac == 1 && ib == 1 && ic == 1) */
    for (i = 0; i < nra; ++i)
      C[i] = 0.;
    for (j = 0; j < nca; ++j)
      for (i = 0; i < nra; ++i)
  C[i] += A[j*nra+i]*B[j];
    break;
  case 121: /* c = A^T*b */
    /* if (iar == nra && iac == 1 && ib == 1 && ic != 1) */
    for (i = 0; i < nra; ++i)
      C[i*ic] = 0.;
    for (j = 0; j < nca; ++j)
      for (i = 0; i < nra; ++i)
  C[i*ic] += A[j*nra+i]*B[j];
    break;
  default:
    for(i = 0; i < nra; ++i) {
      sum = 0.;
      a   = A;
      A  += iac;
      b   = B;
      for(j = 0; j < nca; ++j) {
  sum += (*a) * (*b);
  a   += iar;
  b   += ib;
      }

      *c  = sum;
      c += ic;
    }
    break;
  }
#else
  for(i = 0; i < nra; ++i) {
    sum = 0.;
    a   = A;
    A  += iac;
    b   = B;
    for(j = 0; j < nca; ++j) {
      sum += (*a) * (*b);
      a   += iar;
      b   += ib;
    }

    *c  = sum;
     c += ic;
  }
#endif
  return;
}

#endif
