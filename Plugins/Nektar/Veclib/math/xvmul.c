/*
 *  Vector element-wise multiplication
 */

void dvmul(int n, double *x, int incx, double *y, int incy,
           double *z, int incz)
{
  while( n-- ) {
    *z = (*x) * (*y);
    x += incx;
    y += incy;
    z += incz;
  }
  return;
}



void dvmul_inc1(int n, double *x, double *y, double *z)
{

  int i,k,Istart_stop;
  Istart_stop = n/4;

  k = 0;
  for (i = 0; i < Istart_stop; i++,k = k+4){
      *(z+k)   = (*(x+k))    *  (*(y+k));
      *(z+k+1) = (*(x+k+1))  *  (*(y+k+1));
      *(z+k+2) = (*(x+k+2))  *  (*(y+k+2));
      *(z+k+3) = (*(x+k+3))  *  (*(y+k+3));
  }
  Istart_stop = n - n%4 ;
  for (i = Istart_stop; i < n; i++)
    *(z+i) = (*(x+i))  * (*(y+i)) ;


  return;
}

/*
void dvmul_inc1(int n, double *x, double *y, double *z)
{

  int i,k,Istart_stop;
  Istart_stop = n/2;

  k = 0;
  for (i = 0; i < Istart_stop; i++){
      *(z+k) = (*(x+k))  *  (*(y+k));
      *(z+k+1) = (*(x+k+1))  *  (*(y+k+1));
      k += 2;
  }
  Istart_stop = n - n%2 ;
  for (i = Istart_stop; i < n; i++)
    *(z+i) = (*(x+i))  * (*(y+i)) ;


  return;
}

*/
