/**************************************************************************/
//                                                                        //
//   Author:    S.Sherwin                                                 //
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

/* generate the eignvalues of a packed symmetric or banded  matrix */
void EigenMatrix(double **a, int n, int bwidth)
{
  int info,i;
  double *work = dvector(0,3*n-1);
  double *ev   = dvector(0,n-1),dum;

  if(n>2*bwidth)
    dsbev('N','L',n,bwidth-1,*a,bwidth,ev,&dum,1,work,info);
  else
    dspev('N','L',n,*a,ev,&dum,1,work,info);

  if(info) error_msg(EigenMatrix -- Info not zero);

  for(i = 0; i < n; ++i)
    printf("%#12.6le\n",ev[i]);

  printf("\n %d eigenvalues\n",n);
  dvabs(n,ev,1,ev,1);
  fprintf(stdout,"max eig %lf min eig %lf L2 condition # %lf\n",
    ev[idmax(n,ev,1)],ev[idmin(n,ev,1)]
      ,ev[idmax(n,ev,1)]/ev[idmin(n,ev,1)]);

  if(ev[idmin(n,ev,1)] < 1e-14){
    ev[idmin(n,ev,1)] += ev[idmax(n,ev,1)];
    fprintf(stdout,"max eig %lf 2nd min eig %lf L2 condition # %lf\n",
      ev[idmax(n,ev,1)],ev[idmin(n,ev,1)]
      ,ev[idmax(n,ev,1)]/ev[idmin(n,ev,1)]);
  }

  free(work); free(ev);

}


double FullEigenMatrix(double **a, int n, int lda, int  trip )
{
  register int i;
  int   info= 0,lwork = 3*n;
  double *work = dvector(0,3*n-1);
  double *er   = dvector(0,n-1),dum;
  double *ei   = dvector(0,n-1),dum1;

  dgeev('N','N',n,*a,lda,er,ei,&dum,1,&dum1,1,work,lwork,info);

  if(info) error_msg("FullEigenMatrix-dgeev--Info is not zero\n");

  if(trip){
#if defined  (__blrts__) || defined (__bg__)
    fprintf(stdout,"Max real/imag eigen: %lf %lf \n",
            er[idamax(n,er,1)-1],ei[idamax(n,ei,1)-1]);
#else
    fprintf(stdout,"Max real/imag eigen: %lf %lf \n",
      er[idamax(n,er,1)],ei[idamax(n,ei,1)]);
#endif
  }
  else{
    printf("VARIABLES = Re,Im\n");
    printf("ZONE F=POINT, I=%d\n", n);
    for(i = 0; i < n;++i) printf("%#16.12lg %#16.12lg \n",er[i],ei[i]);

    fprintf(stderr,"\n %d eigenvalues\n",n);
    dvmul(n,er,1,er,1,er,1);
    dvmul(n,ei,1,ei,1,ei,1);
    dvadd(n,er,1,ei,1,er,1);
    dvsqrt(n,er,1,er,1);
    fprintf(stderr,"max eig %lf min eig %lf L2 condition # %lf\n",
      er[idmax(n,er,1)],er[idmin(n,er,1)]
      ,er[idmax(n,er,1)]/er[idmin(n,er,1)]);

    if(er[idmin(n,er,1)] < 1e-14){
      er[idmin(n,er,1)] += er[idmax(n,er,1)];
      fprintf(stderr,"max eig %lf 2nd min eig %lf L2 condition # %lf\n",
        er[idmax(n,er,1)],er[idmin(n,er,1)]
        ,er[idmax(n,er,1)]/er[idmin(n,er,1)]);
    }
  }
#if defined  (__blrts__) || defined (__bg__)
  double x = er[idamax(n,er,1)-1];
#else
  double x = er[idamax(n,er,1)];
#endif
  free(er);free(ei);free(work);
  return x;
}


double FullEigenVec(double **a, double *v, int n, int lda, int  trip )
{
  register int i;
  int   info= 0,lwork = 4*n;
  int j;

  double *work = dvector(0,lwork-1);
  double *er   = dvector(0,n-1),dum;
  double *ei   = dvector(0,n-1);
  double *evecs= dvector(0,n*n-1);
  double  minev = 1e6;
  int revmin = 0;
  double TOL = 1e-8;

  dgeev('N','V',n,*a,lda,er,ei,&dum,1,evecs,n,work,lwork,info);

  if(info) error_msg("FullEigenMatrix-dgeev--Info is not zero\n");

  // find smallest -ve real eigenvalue..

  for(i=0;i<n;++i)
    if( fabs(ei[i]) < TOL){
      fprintf(stderr, "%lf **************************\n", er[i]);
      for(j=0;j<n;++j)
  fprintf(stderr, "%12.8lf ", evecs[i*n+j]);
      fprintf(stderr, "\n");


      if( er[i] < minev){

  minev = er[i];
  revmin = i;
      }
    }

  fprintf(stdout,"Min +ve real e/valu: %lf\n",
    er[revmin]);

  //  dcopy(n, evecs+evmax*n, 1, v, 1);
  dcopy(n, evecs+revmin*n, 1, v, 1);

  free(er);free(ei);free(work);   free(evecs);

  return er[revmin];
}
