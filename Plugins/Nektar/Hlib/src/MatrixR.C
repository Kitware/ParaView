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

/* temporary vector to find patch id of mapped point */
void MemRecur(Bsystem *B, int level, char trip){
  register int i;
  Rsolver *rslv = B->rslv;
  Recur  *rdata = rslv->rdata + level;

  rslv->A.a    = (double **) malloc (rdata->npatch*sizeof(double*));
  rdata->binvc = (double **) malloc (rdata->npatch*sizeof(double*));
  rdata->invc  = (double **) malloc (rdata->npatch*sizeof(double*));
  if(trip != 'p') /* non-positive definite systems */
    rdata->pivotc = (int **) malloc (rdata->npatch*sizeof(int *));
  rdata->bwidth_c = ivector(0,rdata->npatch-1);

  for(i = 0; i < rdata->npatch; ++i){
    if(rdata->patchlen_a[i]){
      rdata->binvc[i] = dvector(0,rdata->patchlen_a[i]*rdata->patchlen_c[i]-1);
      dzero(rdata->patchlen_a[i]*rdata->patchlen_c[i],rdata->binvc[i],1);

      rslv->A.a[i] = dvector(0,rdata->patchlen_a[i]*rdata->patchlen_a[i]-1);
      dzero(rdata->patchlen_a[i]*rdata->patchlen_a[i],rslv->A.a[i],1);
    }
    rdata->invc[i]  = dvector(0,rdata->patchlen_c[i]*rdata->patchlen_c[i]-1);
    dzero(rdata->patchlen_c[i]*rdata->patchlen_c[i],rdata->invc[i],1);
  }
}

void Project_Recur(int oldid, int n, double *a,  int *bmap, int lev,
       Bsystem *Bsys){
  register int i,j;
  int    alen,clen,ptchid,cstart,*map,cnt;
  int    *linka, *linkc;
  double *A,*B,*C;
  Recur *rdata = Bsys->rslv->rdata+lev;

  if(!n) return;

  linka = ivector(0,n-1);
  linkc = ivector(0,n-1);
  ifill(n,-1,linka,1);
  ifill(n,-1,linkc,1);
  cstart = rdata->cstart;

  ptchid = rdata->pmap[oldid];

  A    = Bsys->rslv->A.a[ptchid];
  B    = rdata->binvc[ptchid];
  C    = rdata->invc [ptchid];

  map  = rdata->map[ptchid];
  alen = rdata->patchlen_a[ptchid];
  clen = rdata->patchlen_c[ptchid];
  cnt  = cstart + isum(ptchid,rdata->patchlen_c,1);

  /* set up local mappings for matrix a to next system */
  for(i = 0; i < n; ++i){
    if(bmap[i] < Bsys->nsolve){
      if(bmap[i] < cstart){
  for(j = 0; j < alen; ++j)
    if(bmap[i] == map[j]) linka[i] = j;
      }
      else
  linkc[i] = bmap[i] - cnt;
    }
  }

  /* project a to new local storage */
  for(i = 0; i < n; ++i)
    for(j = 0; j < n; ++j)
      if((linka[i]+1)&&(linka[j]+1))
  A[alen*linka[i] + linka[j]] += a[i*n + j];
      else if((linka[i]+1)&&(linkc[j]+1))
  B[clen*linka[i] + linkc[j]] += a[i*n + j];
      else if((linkc[i]+1)&&(linkc[j]+1))
  C[clen*linkc[i] + linkc[j]] += a[i*n + j];

  free(linka); free(linkc);
}

static int  bandwidth_c(double *c, int len);

void Condense_Recur(Bsystem *Bsys, int lev, char trip){
  register int i,j,k;
  Recur *rdata = Bsys->rslv->rdata + lev;
  int    npatch = rdata->npatch, *bwidth_c = rdata->bwidth_c;
  int    csize,asize,bw,info, *pivot;
  double *A, *B, *C;
  double *c, **b;

  for(i = 0; i < npatch; ++i){
    bw = bwidth_c[i] = bandwidth_c(rdata->invc[i],rdata->patchlen_c[i]);

    asize = rdata->patchlen_a[i];
    csize = rdata->patchlen_c[i];

    A = Bsys->rslv->A.a[i];
    B = rdata->binvc[i];
    C = rdata->invc[i];

    /* pack and factor C */

    if(csize){
      if(2*bw < csize)
  c = dvector(0,csize*bw-1);
      else{
  c = dvector(0,csize*(csize+1)/2-1);
  if(trip != 'p')
    pivot = ivector(0,csize-1);
      }

      PackMatrixV (C,csize,c,bw,'l');
      if(trip == 'p') /* positive definite system */
  FacMatrix   (c,csize,bw);
      else{
  FacMatrixSym(c,pivot,csize,bw);
  rdata->pivotc[i] = pivot;
      }

      free(rdata->invc[i]);
      rdata->invc[i]   = C = c;

      if(asize){
  /* copy B into b for A-BCB */
  b = dmatrix(0,asize-1,0,csize-1);
  dcopy(asize*csize,B,1,*b,1);

  /* calc inv(c)*b^T */
  if(trip == 'p'){
    if(2*bw < csize)
      dpbtrs('L',csize,bw-1,asize,C, bw,B,csize,info);
    else
      dpptrs('L',csize,asize,C,B,csize,info);
  }
  else{
    if(2*bw < csize){
      error_msg(error in codense_recur);
    }
    else
      dsptrs('L',csize,asize,C,pivot,B,csize,info);
  }

  /* A - b inv(c) b^T */
  for(j = 0; j < asize; ++j)
    for(k = 0; k < asize; ++k)
      A[j*asize+k] -= ddot(csize,b[j],1,B+k*csize,1);

  free_dmatrix(b,0,0);
      }
    }
  }
}

static int  bandwidth_c(double *c, int len){
  register int i;
  int bwidth = 0,cnt;

  for(i = 0; i < len; ++i){
    cnt = len-1;
    /* find first non-zero entry in row */
    while(!c[i*len + cnt]) --cnt;
    bwidth = max(bwidth,cnt+1-i);
  }

  return bwidth;
}
/* pack 'a' matrix into 'b' where a and b are both passed as vectors*/

void PackMatrixV(double *a, int n, double *b, int bwidth, char trip){
  register int i;

  if(n>2*bwidth){ /* banded symmetric lower triangular form */
    double *s;

    if(trip == 'l'){
      for(i = 0,s=b; i < n-bwidth; ++i,s+=bwidth)
  dcopy(bwidth,a+i*n+i,1,s,1);

      for(i = n-bwidth; i < n; ++i,s+=bwidth)
  dcopy(n-i,a+i*n+i,1,s,1);
    }
    else
      error_msg(banded upper form not set up in PackMatrixV);
  }
  else{
    register int j;
    if(trip == 'l'){
      /* symmetric lower triangular form */
      for(i=0, j=0; i < n; j+=n-i++)
  dcopy(n-i, a+i*n+i, 1, b+j, 1);
    }
    else{
      /* symmetric upper triangular form */
      for(i=0, j=0; i < n; j+= ++i)
  dcopy(i+1, a+i*n, 1, b+j, 1);
    }
  }
}

/* pack local A matrices and factor system */
void Rinvert_a(Bsystem *B, char trip){
  register int i,j,n;
  Recur   *rdata  = B->rslv->rdata + B->rslv->nrecur-1;
  int    **map    = rdata->map;
  int      npatch = rdata->npatch;
  int      nsolve = rdata->cstart;
  int      asize,high,low,bwidth=0,*pivot;
  double  *inva,*a;

  if(!nsolve) return;

  /* find bandwidth of inverted system */
  for(i = 0; i < npatch; ++i){
    low = 1000000; high = 0;
    for(j = 0; j < rdata->patchlen_a[i]; ++j){
      low  = min(map[i][j],low);
      high = max(map[i][j],high);
    }
    bwidth = max(high-low+1,bwidth);
  }

  B->rslv->Ainfo.bwidth_a = bwidth;

  if(2*bwidth < nsolve){
    inva = dvector(0,bwidth*nsolve-1);
    dzero(bwidth*nsolve,inva,1);

    /* store as packed banded system */
    for(n = 0; n < npatch; ++n){
      asize = rdata->patchlen_a[n];
      a     = B->rslv->A.a[n];
      for(i = 0; i < asize; ++i)
  for(j  = i; j < asize; ++j)
    if(map[n][j] >= map[n][i])
      inva[map[n][i]*bwidth + (map[n][j]-map[n][i])] += a[i*asize+ j];
    else
      inva[map[n][j]*bwidth + (map[n][i]-map[n][j])] += a[i*asize+ j];
    }

    if(B->singular){
      register int k;
      int gid = B->singular-1;

      dzero(bwidth,inva + gid*bwidth,1);
      for(k = 0; k < min(bwidth,gid+1); ++k)
  inva[(gid-k)*bwidth+ k] = 0.0;
      inva[gid*bwidth] = 1.0;
    }
  }
  else{
    inva = dvector(0,nsolve*(nsolve+1)/2-1);
    dzero(nsolve*(nsolve+1)/2,inva,1);
    if(trip != 'p')
      pivot = ivector(0,nsolve-1);

    /* store as symmetric system */
    for(n = 0; n < npatch; ++n){
      asize = rdata->patchlen_a[n];
      a     = B->rslv->A.a[n];
      for(i = 0; i < asize; ++i)
  for(j  = i; j < asize; ++j)
    if(map[n][j] <= map[n][i])
      inva[(map[n][j]*(map[n][j]+1))/2 + (nsolve-map[n][j])*map[n][j]
     + (map[n][i]-map[n][j])] += a[i*asize+j];
    else
      inva[(map[n][i]*(map[n][i]+1))/2 + (nsolve-map[n][i])*map[n][i]
     + (map[n][j]-map[n][i])] += a[i*asize+j];
    }

    if(B->singular){
      register int k;
      int gid = B->singular-1;
      double *s;
      s = inva+gid;
      for(k = 0; k < gid; ++k, s += nsolve-k )
  s[0] = 0.0;
      dzero(nsolve-gid,s,1);
      s[0] = 1.0;
    }

  }

#ifdef DUMP_SC
  writesystem('l',inva,nsolve,bwidth);
  exit(-1);
#endif
#ifdef SC_SPEC
  EigenMatrix(&inva,nsolve,bwidth);
  exit(-1);
#endif

  if(trip == 'p') /* positive definite system */
    FacMatrix(inva,nsolve,bwidth);
  else
    FacMatrixSym(inva,pivot,nsolve,bwidth);

  for(i = 0; i < npatch; ++i)
    free(B->rslv->A.a[i]);
  free(B->rslv->A.a);

  B->rslv->A.inva = inva;
  if(trip != 'p')
    B->rslv->A.pivota = pivot;
}

static void SetupRecur_Precon(Bsystem *B);

void Rpack_a(Bsystem *B, char trip){
  register int n;
  Recur   *rdata  = B->rslv->rdata + B->rslv->nrecur-1;
  int      npatch = rdata->npatch;
  int      nsolve = rdata->cstart;
  int      *alen  = rdata->patchlen_a;
  double   **a = B->rslv->A.a,*packa;

  if(!nsolve) return;

  /* put a matrices in packed symmetric format */
  for(n = 0; n < npatch; ++n){
    packa = dvector(0,alen[n]*(alen[n]+1)/2-1);
    PackMatrixV(a[n],alen[n],packa,alen[n],trip);
    free(a[n]);
    B->rslv->A.a[n] = packa;
  }
  SetupRecur_Precon(B);
}

static void SetupRecur_Precon(Bsystem *B){
  MatPre   *M     = B->Pmat = (MatPre *)malloc(sizeof(MatPre));

  /* at present just store diagonal of whole matrix in M->ivert */
  switch(B->Precon){
  case Pre_Diag: /* diagonal preconditioner */
    {
      Rsolver    *R = B->rslv;
      int      nsolve = R->rdata[R->nrecur-1].cstart;
      double   **a    = R->A.a;
      Recur    *rdata = R->rdata + R->nrecur-1;
      int      npatch = rdata->npatch;
      int      *alen  = rdata->patchlen_a;
      int      **map  = rdata->map;
      int       i,n,pos;

      M->info.diag.ndiag = nsolve;
      M->info.diag.idiag = dvector(0,nsolve-1);
      dzero(nsolve,M->info.diag.idiag,1);

      for(n = 0; n < npatch; ++n)
  for(i = 0,pos = 0; i < alen[n]; pos +=alen[n]-i,++i)
    M->info.diag.idiag[map[n][i]] += a[n][pos];
      InvtPrecon(B);
    }
    break;
  case Pre_Block: /* block diagonal preconditioner */
    {
#if 0
      register int j,k;
      int      al, gid, gid1, pos, nvs = B->rslv->Ainfo.nv_solve;
      Blockp   Blk = B->rslv->precon->blk;

      M->nvert = nvs;
      if(nvs) {
  M->ivert = dvector(0, nvs*(nvs+1)/2-1);
  dzero(nvs*(nvs+1)/2,M->ivert,1);
      }

      M->nedge = Blk.nlgid;
      M->Ledge = ivector(0,M->nedge-1);
      M->iedge = (double **)malloc(M->nedge*sizeof(double *));

      for(i = 0; i < npatch; ++i)
  for(j = 0; j < Blk.nle[i]; ++j)
    M->Ledge[Blk.lgid[i][j]] = Blk.edglen[i][j];


      for(i = 0; i < M->nedge; ++i){
  M->iedge[i] = dvector(0,M->Ledge[i]*(M->Ledge[i]+1)/2-1);
  dzero(M->Ledge[i]*(M->Ledge[i]+1)/2,M->iedge[i],1);
      }

      /* For the iterative solver all the global vertex dof are
   listed first then the local dof and finally the local edges */
      for(i = 0; i < npatch; ++i){
  al = alen[i];

  for(j =0,pos = 0; j < Blk.ngv[i]; ++j, pos += al--){
    gid = map[i][j];
    M->ivert[(gid*(gid+1))/2 + gid*(nvs-gid)] += a[i][pos];
    for(k = j+1; k < Blk.ngv[i]; ++k){
      gid1 = map[i][k];
      if(gid < gid1)
        M->ivert[(gid*(gid+1))/2 + gid*(nvs-gid) + gid1-gid]
    += a[i][pos+k-j];
      else if(gid == gid1) /* special case of periodic element */
        M->ivert[(gid*(gid+1))/2 + gid*(nvs-gid1)]
    += 2*a[i][pos+k-j];
      else
        M->ivert[(gid1*(gid1+1))/2 + gid1*(nvs-gid1) + gid-gid1]
    += a[i][pos+k-j];
    }
  }

  /* set up local blocks - note: it is assumed that the local
     vertices are ordered in a similar fashion either side of
     the patch */
  for(j = 0; j < Blk.nle[i]; ++j){
    for(k = 0,n=0; k < Blk.edglen[i][j];pos += al--,
        n += Blk.edglen[i][j]-k, ++k)
      dvadd(Blk.edglen[i][j]-k,a[i]+pos,1,M->iedge[Blk.lgid[i][j]]+n,1,
      M->iedge[Blk.lgid[i][j]]+n,1);
  }
      }
      InvtPrecon(B, nsolve);
#endif
    fprintf(stderr,"H_MatrixR.C: iterative -r not implemented\n");
    }
    break;
  case Pre_None: /* no preconditioner */
    break;
  default:
    error_msg(Unknown value of PRECON);
    break;
  }
}

void plotdmatv(double *mat, int n, int m){
  int i,j,count = 0;

  for(i = 0; i < n; ++i){
    for(j = 0; j < m; ++j)
      if(fabs(mat[i*m+j]) > 10e-12){
  if(mat[i*m+j] > 0) putchar('+');
  else              putchar('*');
  ++count;
      }
      else
  putchar('-');
    printf("\n");
  }

  printf("%d values above 10e-12 \n",count);
  return;
}
#if 0
void Calc_PRSC_spectrum(Element *U, Bsystem *B){
  register int i,j,k,n;
  double   **A,*aloc;
  int      l,nbl,cnt;
  int      nrecur = B->rslv->nrecur;
  Recur    *rdata = B->rslv->rdata + nrecur-1;
  int      npatch = rdata->npatch;
  int      **bmap = rdata->map;
  int      nsolve = rdata->cstart;


  if(B->smeth != iterative) {
    fputc('\n',stderr);
    error_msg(must run with -i option to call Calc_PSC_spectrum);
  }

  if(!nsolve){
    fprintf(stderr,"Calc_PRSC_spectrum: no boundary system, returning\n");
    return;
  }

  /* Generate SC matrix from local contributions */
  A = dmatrix(0,nsolve-1,0,nsolve-1);
  dzero(nsolve*nsolve, A[0], 1);

  for(k = 0; k < npatch; ++k){
    nbl = rdata->patchlen_a[k];

    aloc = B->rslv->A.a[k];
    /* fill matrix from symmetrically packed local matrices */
    cnt = 0;
    for(i = 0; i < nbl; ++i){
      /* do diagonal */
      A[bmap[k][i]][bmap[k][i]] += aloc[cnt++];
      for(j = i+1; j < nbl; ++j){
  A[bmap[k][i]][bmap[k][j]] += aloc[cnt];
  A[bmap[k][j]][bmap[k][i]] += aloc[cnt++];
      }
    }
  }

  /* operate preconditioner on A[i] */
  for(i = 0; i < nsolve; ++i)
    Precon(B,nsolve,A[i],A[i]);

  /* get eigenvalues of invM*A */
  fprintf(stderr,"\n Calculating eigenvalues\n"); fflush(stderr);
  FullEigenMatrix(*A,nsolve,nsolve,0);

  free_dmatrix(A,0,0);
}
#ifdef 1
{
  int memory = 0;
      int nrecur = Ubsys->rslv->nrecur;
      Recur *r = Ubsys->rslv->rdata;
      int  nsolve = r[nrecur-1].cstart;

      for(i = 0 ; i < nrecur; ++i)
  for(j = 0; j < r[i].npatch; ++j)
    memory += r[i].patchlen_c[j]*(r[i].patchlen_c[j]+1)/2
      + r[i].patchlen_c[j]*r[i].patchlen_a[j];

      if(Ubsys->smeth == direct){
  int  bwidth = Ubsys->rslv->Ainfo.bwidth_a;
  if(2*bwidth < nsolve)
    memory += bwidth*nsolve;
  else
    memory += nsolve*(nsolve+1)/2;
      fprintf(stderr,"\nBandwidth a = %d [%d]\n",bwidth,nsolve);
      }
      else{
  int mem_iter = 0;
  for(i = 0; i < r[nrecur-1].npatch; ++i)
    mem_iter +=r[nrecur-1].patchlen_a[i]*(r[nrecur-1].patchlen_a[i]+1)/2;
  memory += mem_iter;
  fprintf(stderr,"\n boundary system size: %d\n",mem_iter);
      }
      fprintf(stderr,"\nMemory = %d\n", memory);
    }
#endif
#endif
