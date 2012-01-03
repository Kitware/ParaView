/*
 * Random number generation
 *
 * The following set of routines provides several types of random number
 * generation.  The only routines provided as part of VECLIB generate a
 * set of numbers distributed in a Gaussian fashion between (0,1).
 *
 * Not particularly fast.
 */

#include <math.h>
#include <time.h>

/* CALLABLE */

double drand   (void);
void   dvrand  (int, double*, int);
void   raninit (int);

/* INTERNAL */

static double  ran0  (double, double);
static double  ran1  (int*);
static int     iseed;

/* ------------------   CALLABLE FUNCTIONS  ------------------ */

double drand(void)
{
  return Random(0.0, 1.0);
}

double dvrand(int n, double *x, const int incx)
{
  while (n--) {
    *x = ran0(0.0, 1.0);
    x += incx;
  }
  return;
}

void raninit(int flag)
{
  if ( flag < 0 )
    iseed = time(NULL);
  else
    iseed = flag;

  (void) ran1(&iseed);
}

/* ------------------  I N T E R N A L  ------------------ */

static double ran0(double low, double high)
{
  return ran1(&iseed) * (high-low) + low;
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
  static long   ix1,ix2,ix3;
  static double r[98];
  double temp;
  int    j;
  static int iff=0;

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
  ix1 =(IA1*ix1+IC1) % M1;
  ix2 =(IA2*ix2+IC2) % M2;
  ix3 =(IA3*ix3+IC3) % M3;
  j   =1 + ((97*ix3)/M3);
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
