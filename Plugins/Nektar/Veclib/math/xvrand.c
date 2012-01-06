/*
 * Random number generation
 */

#include <stdio.h>
#include <math.h>
#include <assert.h>

#ifndef NULL
#define NULL (0L)
#endif
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static int iseed;

/* Prototypes */

void    InitializeRandom  (int);
double  Random            (double, double);
double  GetGaussian       (double);
double  GetLorentz        (double);
void    Get2Gaussians     (double*, double*, double);
double  ran1              (int*);

/* ---------------------------------------------------------------------- */

void dvrand (int n, double *x, const int incx)
{
  while (n--) {
    *x = Random (0.,1.);
    x += incx;
  }
  return;
}

double drand(void) { return Random (0.,1.); }

/* ---------------------------------------------------------------------- */

double Random(double low, double high)
{
  double d_r = ran1(&iseed);

  d_r = d_r * (high-low) + low;
  assert(d_r>=low );
  assert(d_r<=high);

  return d_r;
}

double GetGaussian(double sigma)
{
  double theta = Random(0.0,2*M_PI);
  double x     = Random(0.0,1.0);
  double r     = sqrt( -2.0*sigma*sigma*log(x) );

  return r * cos(theta);
}

double GetLorentz(double width)
{
  double x = Random(-M_PI/2,M_PI/2);

  return width * tan(x);
}

void Get2Gaussians(double *g1, double *g2, double sigma)
{
  double theta = Random(0.0,2*M_PI);
  double     x = Random(0.0,1.0);
  double     r = sqrt( -2.0*sigma*sigma*log(x));

  assert(g1!=NULL);
  assert(g2!=NULL);
  *g1 = r*cos(theta);
  *g2 = r*sin(theta);
}

void InitializeRandom(int flag)
{
  if ( flag < 0 )
    iseed = time(NULL);
  else
    iseed = flag;

  (void) ran1(&iseed);
}

#define M1  259200
#define IA1 7141
#define IC1 54773
#define RM1 (1.0/M1)
#define M2  134456
#define IA2 8121
#define IC2 28411
#define RM2 (1.0/M2)
#define M3  243000
#define IA3 4561
#define IC3 51349

double ran1(int *idum)
{
  static long ix1,ix2,ix3;
  static double r[98];
  double temp;
  static int iff=0;
  int j;

  if (*idum < 0 || iff == 0) {
    iff=1;
    ix1=(IC1-(*idum)) % M1;
    ix1=(IA1*ix1+IC1) % M1;
    ix2=ix1 % M2;
    ix1=(IA1*ix1+IC1) % M1;
    ix3=ix1 % M3;
    for (j=1;j<=97;j++) {
      ix1=(IA1*ix1+IC1) % M1;
      ix2=(IA2*ix2+IC2) % M2;
      r[j]=(ix1+ix2*RM2)*RM1;
    }
    *idum=1;
  }
  ix1=(IA1*ix1+IC1) % M1;
  ix2=(IA2*ix2+IC2) % M2;
  ix3=(IA3*ix3+IC3) % M3;
  j  =1 + ((97*ix3)/M3);
  temp=r[j];
  r[j]=(ix1+ix2*RM2)*RM1;
  return temp;
}

#undef M1
#undef IA1
#undef IC1
#undef RM1
#undef M2
#undef IA2
#undef IC2
#undef RM2
#undef M3
#undef IA3
#undef IC3
