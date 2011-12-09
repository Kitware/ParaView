/**************************************************************************/
//                                                                        //
//   Author:    T.Warburton                                               //
//   Design:    T.Warburton && S.Sherwin                                  //
//   Date  :    12/4/96                                                   //
//                                                                        //
//   Copyright notice:  This code shall not be replicated or used without //
//                      the permission of the author.                     //
//                                                                        //
/**************************************************************************/

#include <math.h>
#include <veclib.h>
#include "hotel.h"

/* local declarations */
static void reshuffle(Element_List **,int);

static double  Alpha_CNAB[][3] = {
  {1., 0., 0.},
  {1., 0., 0.},
  {1., 0., 0.}};

static double  Beta_CNAB[][3] = {
  {1.0 ,       0.0    ,  0.0      },
  {3.0/2.0,   -1.0/2.0,  0.0      },
  {23.0/12.0, -4.0/3.0,  5.0/12.0 }};


static double  Beta_AM[][3] = {
  {1.0 ,       0.0    ,  0.0      },
  {1.0/2.0,    1.0/2.0,  0.0      },
  {5.0,        8.0,     -1.0      }};

double         Gamma_CNAB[3] = { 1., 1., 1.};

static double Alpha_Int[3] = { 1.0, 0.0, 0.0};
static double  Beta_Int[3] = { 1.0, 0.0, 0.0};
static double Gamma_Int    = 1.0;

void   getalpha(double *a){ dcopy(3,Alpha_Int,1,a,1); }
void   getbeta(double *b) { dcopy(3,Beta_Int,1,b,1); }
double getgamma(void)         { return Gamma_Int; }

void set_order_CNAB(int Je){
  dcopy(3, Alpha_CNAB[Je-1], 1, Alpha_Int, 1);
  dcopy(3,  Beta_CNAB[Je-1], 1,  Beta_Int, 1);
  Gamma_Int =  Gamma_CNAB[Je-1];
}

void set_order_CNAM(int Je){
  dcopy(3, Alpha_CNAB[Je-1], 1, Alpha_Int, 1);
  dcopy(3,  Beta_AM[Je-1], 1,  Beta_Int, 1);
}

/* This routine integrates the transformed values */
void integrate_CNAB(int Je, double dt, Element_List *U, Element_List *Uf[]){
  register  int i;
  int       nq = U->hjtot*U->nz;

  dsmul(nq, Beta_Int[Je-1]*dt, Uf[Je-1]->base_hj, 1, Uf[Je-1]->base_hj, 1);

  for(i = 0; i < Je-1; ++i)
    daxpy(nq, Beta_Int[i]*dt, Uf[i]->base_hj,1, Uf[Je-1]->base_hj, 1);

  dvadd(nq, Uf[Je-1]->base_hj,1, U->base_hj, 1, U->base_hj, 1);

  reshuffle(Uf,Je);
}

static void reshuffle(Element_List *U[],int N){
  int i;
  Element_List *t = U[N-1];
  for(i = N-1; i; --i) U[i] = U[i-1];
  U[0] = t;
}

static void reshuffle(double **u,int N){
  int i;
  double *t = u[N-1];
  for(i = N-1; i; --i) u[i] = u[i-1];
  u[0] = t;
}

/* This routine integrates the physical values */
void Integrate_CNAB(int Je, double dt, Element_List *U, Element_List *Uf,
        double **u, double **uf){
  register  int i;
  int       nq;
  double    theta = dparam("THETA");

  nq = U->htot*U->nz;
  dcopy(nq, Uf->base_h, 1, uf[0], 1);

  /* multiply u^n by theta factor */
  dscal(nq, 1.0/(1-theta),U->base_h,1); // ideally should put outside library
  for(i = 0; i < Je; ++i)
    daxpy(nq, Beta_Int[i]*dt,  uf[i], 1, U->base_h,1);

  reshuffle(uf,Je);
}

/* This routine integrates the physical values */
void integrate_CNAB(int Je, double dt, Element_List *U, Element_List *Uf,
        double **u, double **uf){
  register  int i;
  int       nq;
  double    theta = dparam("THETA");

  nq = U->hjtot*U->nz;
  dcopy(nq, Uf->base_hj, 1, uf[0], 1);

  /* multiply u^n by theta factor */
  dscal(nq, 1.0/(1-theta),U->base_hj,1); // ideally should put outside library
  for(i = 0; i < Je; ++i)
    daxpy(nq, Beta_Int[i]*dt,  uf[i], 1, U->base_hj,1);

  reshuffle(uf,Je);
}

/* This routine integrates the physical values */
void Integrate_AB(int Je, double dt, Element_List *U, Element_List *Uf,
        double **u, double **uf){
  register  int i;
  int       nq;


  nq = U->htot*U->nz;
  dcopy(nq, Uf->base_h, 1, uf[0], 1);

  for(i = 0; i < Je; ++i)
    daxpy(nq, Beta_Int[i]*dt,  uf[i], 1, U->base_h,1);

  reshuffle(uf,Je);
}

/* This routine integrates the physical values */
void Integrate_AM(int Je, double dt, Element_List *U, Element_List *Uf,
        double **u, double **uf){
  register  int i;
  int       nq;
  double    theta = dparam("THETA");

  nq = U->htot*U->nz;
  dcopy(nq, Uf->base_h, 1, uf[0], 1);

  for(i = 0; i < Je; ++i)
    daxpy(nq, Beta_Int[i]*dt,  uf[i], 1, U->base_h,1);

  reshuffle(uf,Je);
}

/* This routine integrates the physical values */
void Integrate_CNAM(int Je, double dt, Element_List *U, Element_List *Uf,
        double **u, double **uf){
  register  int i;
  int       nq;
  double    theta = dparam("THETA");

  nq = U->htot*U->nz;
  dcopy(nq, Uf->base_h, 1, uf[0], 1);

  /* multiply u^n by theta factor */
  dscal(nq, 1.0/(1-theta),U->base_h,1);
  for(i = 0; i < Je; ++i)
    daxpy(nq, Beta_Int[i]*dt,  uf[i], 1, U->base_h,1);

  reshuffle(uf,Je);
}

static double Alpha_SS[][3] = {
  { 1.0,      0.0,     0.0},
  { 2.0, -1.0/2.0,     0.0},
  { 3.0, -3.0/2.0, 1.0/3.0}};
#if 0
static double  Beta_SS[][3] = {
  { 1.0,  0.0, 0.0},
  { 2.0, -1.0, 0.0},
  { 3.0, -3.0, 1.0}};
#else
static double  Beta_SS[][3] = {
  { 1.0,  0.0, 0.0},
  { 2.0, -1.0, 0.0},
  { 5.0/2.0, -2.0, 1.0/2.0}};
#endif

double Gamma_SS[3] = { 1., 3./2., 11./6.};

double getgamma(int i){ return Gamma_SS[i-1];}

static int tmp_order = 1;

void set_order(int Je){
  tmp_order = Je;
  dcopy(3, Alpha_SS[Je-1], 1, Alpha_Int, 1);
  dcopy(3,  Beta_SS[Je-1], 1,  Beta_Int, 1);
  Gamma_Int =  Gamma_SS[Je-1];
}

/* This routine integrates the physical values with a stiffly stable scheme */
void Integrate_SS(int Je, double dt, Element_List *U, Element_List *Uf,
                         double **u,      double **uf){
  register  int i;
  int       nq;

  nq = U->htot*U->nz;
  dcopy(nq,  U->base_h, 1,  u[0], 1);
  dcopy(nq, Uf->base_h, 1, uf[0], 1);

  dsmul(nq, Alpha_Int[Je-1], u[Je-1], 1,U->base_h, 1);

  for(i = 0; i < Je-1; ++i)
    daxpy(nq, Alpha_Int[i],    u[i], 1, U->base_h, 1);

  for(i = 0; i < Je; ++i)
    daxpy(nq, Beta_Int[i]*dt, uf[i], 1, U->base_h, 1);

  reshuffle( u, Je);
  reshuffle(uf, Je);
}
