#ifndef H_VMATH
#define H_VMATH

#include <string.h>
#include <Blas.h>

namespace vmath {


  /***************** Math routines from veclib.h ***************/

  /// Fill a vector with a constant value
  template<class T>  void fill( int n, const T alpha,  T *x, const int incx ){
    while( n-- ) {
      *x = alpha;
      x += incx;
    }
  }


  /// Multiply vector z = x*y
  template<class T>  void vmul( int n, const T *x, const int incx, T*y,
              const int incy,  T*z, const int incz){
    while( n-- ) {
      *z = (*x) * (*y);
      x += incx;
      y += incy;
      z += incz;
    }
  }

  /// Add vector z = x+y
  template<class T>  void vadd( int n, const T *x, const int incx, const T *y,
              const int incy,  T*z, const int incz){
    while( n-- ) {
      *z = (*x) + (*y);
      x += incx;
      y += incy;
      z += incz;
    }
  }

  /// Add vector y = alpha + x
  template<class T>  void sadd( int n, const T alpha, const T *x,
        const int incx, T *y, const int incy){
    while( n-- ) {
      *y = alpha + (*x);
      x += incx;
      y += incy;
    }
  }

  /// Subtract vector z = x-y
  template<class T>  void vsub( int n, const T *x, const int incx, T*y,
              const int incy,  T*z, const int incz){
    while( n-- ) {
      *z = (*x) - (*y);
      x += incx;
      y += incy;
      z += incz;
    }
  }

  /// Zero vector
  template<class T>  void zero(int n, T *x, const int incx){
    if(incx == 1)
      memset(x,'\0', n*sizeof(T));
    else{
      T zero = 0;
      while(n--){
  *x = zero;
  x+=incx;
      }
    }
  }

  /// Negate x = -x
  template<class T>  void neg( int n, T *x, const int incx){
    while( n-- ) {
      *x = -(*x);
      x += incx;
    }
  }


  /// sqrt y = sqrt(x)
  template<class T> void vsqrt(int n, const T *x, const int incx,
             T *y, const int incy){
    while (n--) {
      *y  = sqrt( *x );
      x  += incx;
      y  += incy;
    }
  }



  /************ Misc routine from Veclib (and extras)  ************/

  /// Gather vector z[i] = x[y[i]]
  template<class T>  void gathr(int n, const T *x, const int *y,
              T *z){
    while (n--) *z++ = *(x + *y++);
    return;
  }

  /// Scatter vector z[y[i]] = x[i]
  template<class T>  void scatr(int n, const T *x, const int *y,
              T *z){
    while (n--) *(z + *(y++)) = *(x++);
  }


  /// Assemble z[y[i]] += x[i] - z should be zero'd first
  template<class T>  void assmb(int n, const T *x, const int *y,
              T *z){
    while (n--) *(z + *(y++)) += *(x++);
  }


  /************* Reduction routines from Veclib *****************/

  /// Subtract return sum(x)
  template<class T>  T vsum( int n, const T *x, const int incx){

    T sum = 0;

    while( n-- ) {
      sum += (*x);
      x += incx;
    }

    return sum;
  }


  /// Return the index of the maximum element in x
  template<class T>  int imax( int n, const T *x, const int incx){

    int    i, indx = ( n > 0 ) ? 0 : -1;
    T      xmax = *x;

    for (i = 0; i < n; i++) {
      if (*x > xmax) {
  xmax = *x;
  indx = i;
      }
      x += incx;
    }
    return indx;
  }

  /// Return the maximum element in x -- called vmax to avoid conflict with max
  template<class T>  T vmax( int n, const T *x, const int incx){

    T  xmax = *x;

    while( n-- ){
      if (*x > xmax) {
  xmax = *x;
      }
      x += incx;
    }
    return xmax;
  }


  /// Return the index of the minimum element in x
  template<class T>  int imin( int n, const T *x, const int incx){

    int    i, indx = ( n > 0 ) ? 0 : -1;
    T      xmin = *x;

    for(i = 0;i < n;i++) {
      if( *x < xmin ) {
  xmin = *x;
  indx = i;
      }
      x += incx;
    }
    return indx;
  }


  /// Return the minimum element in x - called vmin to avoid conflict with min
  template<class T>  T vmin( int n, const T *x, const int incx){

    T    xmin = *x;

    while( n-- ){
      if (*x < xmin) {
  xmin = *x;
      }
      x += incx;
    }
    return xmin;
  }

  /********** Memory routines from Veclib.h ***********************/

  // copy one double vector to another - This is just a wrapper around blas
  static void vcopy(int n, const double *x, int incx, double *y,
        int const incy){
    blas::dcopy(n,x,incx,y,incy);
  }

  // copy one int vector to another
  static void vcopy(int n, const int  *x, const int incx, int *y,
        int const incy){
    if( incx ==1 && incy == 1)
      memcpy(y,x,n*sizeof(int));
    else
      while( n-- ) {
  *y = *x;
  x += incx;
  y += incy;
      }
  }

}
#endif
