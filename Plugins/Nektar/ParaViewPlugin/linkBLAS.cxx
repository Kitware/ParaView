#include "../include/Blas.h"

int main()
{
  int n = 5;
  double da = 2;
  double dx[5] = {0, 1, 2, 3, 4};
  int incx = 1;

  blas::dscal(n, da, dx, incx);

  return 0;
}
