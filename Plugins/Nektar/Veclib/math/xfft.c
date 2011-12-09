/* ------------------------------------------------------------------------ *
 * Fourier Transform Routines                                               *
 *                                                                          *
 * The following routines are from "Numerical Recipies in C" and have been  *
 * slightly modified to conform with (ahem) industry standards for FFT's.   *
 * Both routines are decimation in time versions of the classic Cooley-     *
 * Tukey algorithm.  The calling conventions are as follows:                *
 *                                                                          *
 * Real to Complex    : realft(n, data, isign)                              *
 * Complex to Complex : four1 (n, data, isign)                              *
 *                                                                          *
 *     n      ...   number of elements (complex)                            *
 *     data   ...   complex array of data to transform                      *
 *     isign  ...   sign of the exponent in the Fourier integral            *
 *                     -1 for a Physical -> Fourier transform               *
 *                     +1 for a Fourier -> Physical transform               *
 *                                                                          *
 * NOTES                                                                    *
 * ---------------                                                          *
 * 1. realft() packs the real part of the last mode into the imaginary      *
 *    part of the first mode.                                               *
 *                                                                          *
 * 2. The routines below are all kinds of screwed up.  Don't try to follow  *
 *    conventions unless you want a headache.                               *
 *                                                                          *
 * ------------------------------------------------------------------------ */

#include <stdio.h>
#include <math.h>

void realft(int, double*, int);    /* INTERNAL TYPE DECLARATIONS */
void four1 (int, double*, int);

#ifndef  M_PI
#define  M_PI  3.14159265358979323846
#endif

/* ------------------------------------------------------------------------ *
 * realft() - FFT of a single real function                                 *
 *                                                                          *
 * ISIGN: +1 performs a transform from Fourier  -> Physcial space           *
 *        -1 performs a transform from Physical -> Fourier space            *
 *                                                                          *
 * The real component of the first mode and the real component of the last  *
 * mode are packed into the first two elements of data.  This convention is *
 * expected regardless of the direction of the transform.                   *
 *                                                                          *
 * ------------------------------------------------------------------------ */

void realft(int n, double *data, int isign)
{
  register int    i, i1, i2, i3, i4, n2p3;
  register double wr, wi, wpr, wpi, wtemp;
  register double h1r, h1i, h2r, h2i;
  double c1 = 0.5, c2, theta;

  data  -=  1;      /* realign the data vector */
  isign *= -1;      /* switch the sign         */

  switch (isign) {
  case  1:                        /* FORWARD TRANSFORM */
    c2    = -0.5;
    theta =  M_PI / (double) n;
    four1(n, data, isign);
    break;
  case -1:                        /* INVERSE TRANFORM */
    dneg(n-1, data + 4, 2);
    c2    =  0.5;
    theta = -M_PI / (double) n;
    break;
  default:
    printf("realft(): called with ISIGN = %d\n", isign);
    exit(-1);
    break;
  }

  wtemp = sin (c1 * theta);
  wpr   = -2. * wtemp * wtemp;
  wpi   = sin (theta);
  wr    = 1. + wpr;
  wi    = wpi;
  n2p3  = (n<<1) + 3;

  for(i = 2; i <= (n>>1); i++) {
    i4       = 1 + (i3 = n2p3 - (i2 = 1+(i1 = i+i-1)));

    h1r      =  c1 * (data[i1] + data[i3]);
    h1i      =  c1 * (data[i2] - data[i4]);
    h2r      = -c2 * (data[i2] + data[i4]);
    h2i      =  c2 * (data[i1] - data[i3]);

    data[i1] = h1r + wr * h2r - wi * h2i;
    data[i2] = h1i + wr * h2i + wi * h2r;
    data[i3] = h1r - wr * h2r + wi * h2i;
    data[i4] = -h1i + wr * h2i + wi * h2r;

    wr       = (wtemp = wr) * wpr - wi * wpi + wr;
    wi       = wi * wpr + wtemp * wpi + wi;
  }

  if(isign == 1) {
    double renorm = 1. / (double) ( n << 1 );

    data[1] = (h1r=data[1]) + data[2];   /* squeeze the first and last */
    data[2] = h1r - data[2];             /* data elements together     */
    dsmul(n<<1, renorm, data + 1, 1 , data + 1, 1);    /* renormalize                */
    dneg(n-1, data + 4, 2);
  }
  else {
    double renorm = 2.;

    data[1] = c1 * ((h1r=data[1]) + data[2]);
    data[2] = c1 * (h1r-data[2]);
    four1(n, data, isign);                        /* inverse transform */
    dsmul(n<<1, renorm, data + 1, 1, data + 1, 1);             /* renormalize       */
  }
  return;
}

/* ------------------------------------------------------------------------ *
 * four1() - Discrete Fourier Transform                                     *
 *                                                                          *
 * Replaces data[] by its discrete Fourier transform.  Based on the Cooley- *
 * Tukey (decimation in time) algorithm.                                    *
 *                                                                          *
 * ------------------------------------------------------------------------ */

void four1(int nn, double *data, int isign)
{
  int    n,mmax,m,j,istep,i;
  double wtemp, wr, wpr, wpi, wi, theta;
  double tempr,tempi;

  n = nn << 1;
  j = 1;

  /*
   * BIT REVERSEAL SECTION
   */

  for(i = 1;i < n; i += 2) {
    if (j > i) {
      tempr     = data[j];
      tempi     = data[j+1];
      data[j]   = data[i];
      data[j+1] = data[i+1];
      data[i]   = tempr;
      data[i+1] = tempi;
    }
    m = n >> 1;
    while(m >= 2 && j > m) {
      j  -= m;
      m >>= 1;
    }
    j += m;
  }

  /*
   * DANIELSON-LANCZOS SECTION
   */

  mmax = 2;
  while(n > mmax) {

    istep = mmax << 1;
    theta = M_PI * 2. / ( isign * mmax );
    wtemp = sin(0.5 * theta);
    wpr   = -2. * wtemp * wtemp;
    wpi   = sin( theta );
    wr    = 1.;
    wi    = 0.;


    /*            FFT INNER LOOP              *
     * -------------------------------------- */

    for(m = 1;m < mmax;m += 2) {
        for(i = m;i <= n;i += istep) {
            j          = i + mmax;
            tempr      = wr * data[j]   - wi * data[j+1];
            tempi      = wr * data[j+1] + wi * data[j];
            data[j]    = data[i]   - tempr;
            data[j+1]  = data[i+1] - tempi;
            data[i]   += tempr;
            data[i+1] += tempi;
  }                                        /* trigonometric recurrence */
        wr = (wtemp=wr) * wpr - wi * wpi + wr;
        wi = wi * wpr + wtemp * wpi + wi;
      }

    /* -------------------------------------- */

    mmax = istep;
  }
  return;
}
