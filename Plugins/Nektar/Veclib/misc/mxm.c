/*
 * mxm() - Matrix Multiply (double precision)
 *
 * The following function computes the matrix product C = A * B.  The matrices
 * are assumed to be in row-major order and must occupy consecutive memory lo-
 * cations.  The input quantities are as follows:
 *
 *     mxm(A,nra,B,nca,C,ncb)
 *
 *     A    ... double* ... source vector one
 *     nra  ... int     ... number of rows in A
 *     B    ... double* ... source operand two
 *     nca  ... int     ... number of columns in A
 *     C    ... double* ... result vector
 *     ncb  ... int     ... number of columns in B
 *
 * A more general matrix multiply with arbitrary skips is given in mxma().
 */


#if !defined(mxm)

void mxm (double *A, int *Inra, double* B, int *Inca,  double *C, int *Incb)
{
  register double *a = A,
                  *b = B,
                  *c = C;
  register double sum;
  register i, j, k;
  int nra = *Inra;
  int nca = *Inca;
  int ncb = *Incb;

#if 1
  for (i = 0; i < nra; i++) {
    for (j = 0; j < ncb; j++) {

      b   = B + j;                  /* Next column of B    */
      sum = 0.;                     /* Clear sum           */

      for(k = 0; k < nca; k++) {    /* ------------------- */
  sum += a[k] * (*b);         /* Inner product loop  */
  b   += ncb;                 /* ------------------- */
      }
      *c++   = sum;                 /* Store and increment */
    }
    a += nca;                       /* Next row of A       */
  }
#else
  for (i = 0; i < nra; i++)
    for (j = 0; j < ncb; j++) {
      C[i*(ncb)+j] = 0.;
      for(k = 0; k < nca; k++)
  C[i*(ncb)+j] += A[i*(nca)+k] * B[k*(ncb)+j];
    }
#endif
  return;                           /*      All Done       */
}

#endif
