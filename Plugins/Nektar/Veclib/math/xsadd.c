/*
 * scalar plus a vector
 */
#ifdef __blrts__
void dsadd (int n, double alpha, double *x, int incx, double *y, int incy)
{
  int index_y =0, index_x = 0;
  while (n--) {
    y[index_y] =  alpha + x[index_x];
    index_y += incy;
    index_x += incx;
  }
  return;
}
#else
void dsadd (int n, double alpha, double *x, int incx, double *y, int incy)
{
  while (n--) {
    *y = alpha + *x;
    x += incx;
    y += incy;
  }
  return;
}
#endif
