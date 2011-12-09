/**************************************************************************/
//                                                                        //
//   Author:    T.Warburton                                               //
//   Design:    T.Warburton && S.Sherwin                                  //
//   Date  :    12/4/96                                                   //
//                                                                        //
//   Copyright notice:  This code shall not be replicated or used without //
//                      the permission f the author.                     //
//                                                                        //
/**************************************************************************/

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <polylib.h>
#include "veclib.h"
#include "hotel.h"
#include "nekstruct.h"

#include <stdio.h>

using namespace polylib;

typedef struct dinfo {
  int   qa;         /* number of 'a' quad. points                           */
  double **Dap;      /* Differential matrix for a direction                  */
  double **Dapt;     /* Differential matrix for a direction (transposed)     */
  double **Dam;      /* Differential matrix for a direction                  */
  double **Damt;     /* Differential matrix for a direction (transposed)     */
  struct dinfo *next;
} Dinfo;

Dinfo *addD(int qa)
{
  int i,j;
  double *za,*w;
  Dinfo *D = (Dinfo *)malloc(sizeof(Dinfo));

  D->qa  = qa;
  D->Dap  = dmatrix(0,qa/2-1,0,qa/2-1);
  D->Dam  = dmatrix(0,qa/2-1,0,qa/2-1);

  D->Dapt = dmatrix(0,qa/2-1,0,qa/2-1);
  D->Damt = dmatrix(0,qa/2-1,0,qa/2-1);


  double **Da  = dmatrix(0,qa-1,0,qa-1);
  double **Dat = dmatrix(0,qa-1,0,qa-1);

  getzw(qa,&za,&w,'a');

  dgll(Da,Dat,za,qa);

  for(i=0;i<qa/2;++i)
    for(j=0;j<qa/2;++j){
      D->Dap[i][j] = 0.5*(Da[i][j]+Da[i][qa-1-j]);
      D->Dam[i][j] = 0.5*(Da[i][j]-Da[i][qa-1-j]);
    }

  for(i=0;i<qa/2;++i)
    for(j=0;j<qa/2;++j){
      D->Dapt[j][i] = 0.5*(Da[i][j]+Da[i][qa-1-j]);
      D->Damt[j][i] = 0.5*(Da[i][j]-Da[i][qa-1-j]);
    }

  return D;
}

/*-------------------------------------------------------------------*
 *  link list for Differential matrices at quadrature points         *
 *-------------------------------------------------------------------*/
static Dinfo  **getD_base = NULL;

void gethalfD(int qa,
        double ***dap, double ***dapt,
        double ***dam, double ***damt){
  /* check link list */

  if(!getD_base)
    getD_base  = (Dinfo**) calloc(QGmax+1,sizeof(Dinfo*));

  if(!getD_base[qa])
    getD_base[qa] = addD(qa);

  *dap  = getD_base[qa]->Dap;
  *dapt = getD_base[qa]->Dapt;
  *dam  = getD_base[qa]->Dam;
  *damt = getD_base[qa]->Damt;

  return;
}

void grad_h(int qa, int qb, double *u, double *ua, double *ub, double *wk){
  int         qah = qa/2;
  int         i,j;
  double    **dapt,**damt,**dap,**dam;
  gethalfD(qa,&dap,&dapt,&dam,&damt);

#if 0

  // do 'a' and 'b' derivatives at same time
  double *wka =  wk+qa*qb;
  double *wkb = wka+qa*qb;

  /* calculate du/da */

  for(i=0;i<qb;++i)
    dvadd(qah, u+i*qa, 1, u+i*qa+qa-1, -1, wk+i*qah, 1);
  //  for(i=0;i<qb;++i)
    //    dvadd(qah, u+i, qa, u+i+qa*(qa-1), -qa, wk+qb*qbh+i*qah,1);
  for(i=0;i<qah;++i)
    dvadd(qa, u+i*qa, 1, u+qa*(qa-1-i), 1, wk+qb*qbh+i,qah);

  //  dgemm('n','n',qah,qa+qb,qah,1.,*dapt,qah,wk,qah,0.,wka,qah);
  mxm(wk,qa+qb,*dapt,qah,wka,qah);

  for(i=0;i<qb;++i)
    dvsub(qah, u+i*qa, 1, u+i*qa+qa-1, -1, wk+i*qah, 1);

  for(i=0;i<qah;++i)
    dvsub(qa, u+i*qa, 1, u+qa*(qa-1-i), 1, wk+qb*qbh+i,qah);
  //  for(i=0;i<qa;++i)
  //    dvsub(qah, u+i, qa, u+i+qa*(qa-1), -qa, wk+qb*qbh+i*qah,1);

  //  dgemm('n','n',qah,qb+qa,qah,1.,*damt,qah,wk,qah,0.,wkb,qah);
  mxm(wk,qa+qb,*damt,qah,wkb,qah);

  for(i=0;i<qb;++i){
    dvadd(qah, wka+i*qah, 1, wkb+i*qah, 1, ua+i*qa, 1);
    dvsub(qah, wkb+i*qah, 1, wka+i*qah, 1, ua+i*qa+qa-1, -1);
  }
  for(i=0;i<qa;++i){
    dvadd(qah, wka+i*qah+qa*qah, 1, wkb+i*qah+qa*qah, 1, ub+i, qa);
    dvsub(qah, wkb+i*qah+qa*qah, 1, wka+i*qah+qa*qah, 1, ub+i+qa*(qb-1), -qa);
  }
#endif

#if 1
  double *wka =  wk+qa*qb;
  double *wkb = wka+qa*qb;

  /* calculate du/da */
  // even terms
  for(i=0;i<qb;++i)
    for(j=0;j<qah;++j)
      wk[i*qah+j] = u[i*qa+j]+u[i*qa+qa-1-j];

  //  dgemm('n','n',qah,qa,qah,1.,*dapt,qah,wk,qah,0.,wka,qah);
  mxm(wk,qb,*dapt,qah,wka,qah);

  // odd terms
  for(i=0;i<qb;++i)
    for(j=0;j<qah;++j)
      wk[i*qah+j] = u[i*qa+j]-u[i*qa+qa-1-j];
  //  dgemm('n','n',qah,qb,qah,1.,*damt,qah,wk,qah,0.,wkb,qah);
  mxm(wk,qb,*damt,qah,wkb,qah);

  // reconstruct first half
  for(i=0;i<qb;++i)
    for(j=0;j<qah;++j)
      ua[i*qa+j]      = wka[i*qah+j]+wkb[i*qah+j];

  // reconstruct second half
  for(i=0;i<qb;++i)
    for(j=0;j<qah;++j)
      ua[i*qa+qa-1-j] = wkb[i*qah+j]-wka[i*qah+j];


  // Calculate du/db
  // even terms
  for(i=0;i<qah;++i)
    dvadd(qa, u+i*qa, 1, u+(qa-1-i)*qa, 1, wk+i*qa, 1);

  //  dgemm('n','n',qa,qah,qah,1.,wk,qa,*dap,qah,0.,wka,qa);
  mxm(*dap,qah,wk,qah,wka,qa);

  // odd terms
  for(i=0;i<qah;++i)
    dvsub(qa, u+i*qa, 1, u+(qa-1-i)*qa, 1, wk+i*qa, 1);

  //  dgemm('n','n',qa,qah,qah,1.,wk,qa,*dam,qah,0.,wkb,qa);
  mxm(*dam,qah,wk,qah,wkb,qa);

  // reconstruct
  dvadd(qa*qah, wka, 1, wkb, 1, ub, 1);
  for(i=0;i<qah;++i)
    dvsub(qa, wkb+i*qa, 1, wka+i*qa, 1, ub+(qa-1-i)*qa, 1);
#endif
}
