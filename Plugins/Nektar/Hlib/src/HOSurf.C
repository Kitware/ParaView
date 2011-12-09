#include <hotel.h>
#include <veclib.h>
#include <string.h>

#include <stdio.h>

void Tri_reset_basis(Basis *B);
Basis *Tri_addbase(int, int, int, int);
void Tri_get_point_shape(Element *, double, double, double *, double *);

// local statics which should probably be in a class
static int    QuadDist,Loaded_Surf_File,Snp,Snface;
static double **SurXpts, **SurYpts, **SurZpts;
static double **CollMat;
static int    **surfids, *CollMatIpiv;
static void Rotate(int fid, int nrot);
static void Reflect(int fid);

void Load_HO_Surface(char *name){
  register int i,j;
  int      np,nface,ntot,info;
  FILE     *infile;
  char     buf[BUFSIZ],*p;
  double   *Rpts, *Spts;

  if(Loaded_Surf_File) return; // allow for multiple calls

  // check for .hsf file first
  sprintf(buf,"%s.hsf",strtok(name,"."));
  if(!(infile = fopen(buf,"r"))){
    fprintf(stderr,"unable to open file %s or %s\n"
      "Run homesh on the .fro file to generate "
      "high order surface file \n",name,buf);
    exit(1);
  }

  // read header
  fgets(buf,BUFSIZ,infile);

  p = strchr(buf,'=');
  sscanf(++p,"%d",&np);
  p = strchr(p,'=');
  sscanf(++p,"%d",&nface);
  Snp = np; Snface = nface;

  ntot = np*(np+1)/2;

  Rpts = dvector(0,ntot-1);
  Spts = dvector(0,ntot-1);

  // read in point distribution in standard region
  fgets(buf,BUFSIZ,infile);
  fscanf(infile,"# %lf",Rpts);
  for(i = 1; i < ntot; ++i)
    fscanf(infile,"%lf",Rpts+i);
  fgets(buf,BUFSIZ,infile);

  fscanf(infile,"# %lf",Spts);
  for(i = 1; i < ntot; ++i)
    fscanf(infile,"%lf",Spts+i);
  fgets(buf,BUFSIZ,infile);

  fgets(buf,BUFSIZ,infile);

  SurXpts = dmatrix(0,nface-1,0,ntot-1);
  SurYpts = dmatrix(0,nface-1,0,ntot-1);
  SurZpts = dmatrix(0,nface-1,0,ntot-1);

  // read surface points
  for(i = 0; i < nface; ++i){
    fgets(buf,BUFSIZ,infile);
    for(j = 0; j < ntot; ++j){
      fgets(buf,BUFSIZ,infile);
      sscanf(buf,"%lf%lf%lf",SurXpts[i]+j,SurYpts[i]+j,SurZpts[i]+j);
    }
    // read input connectivity data
    for(j = 0; j < (np-1)*(np-1); ++j)
      fgets(buf,BUFSIZ,infile);
  }

  fgets(buf,BUFSIZ,infile);
  // read Vertids to match with .rea file
  surfids = imatrix(0,nface,0,4);
  for(i = 0; i < nface; ++i){
    fgets(buf,BUFSIZ,infile);
    sscanf(buf,"# %*d%d%d%d",surfids[i],surfids[i]+1,surfids[i]+2);
    surfids[i][0]--;
    surfids[i][1]--;
    surfids[i][2]--;
    surfids[i][3] = surfids[i][0] + surfids[i][1] + surfids[i][2];
  }

  Tri      T;
  T.qa   = Snp+1;
  T.qb   = Snp;
  T.lmax = Snp;
  Basis *B = Tri_addbase(Snp,Snp+1,Snp,Snp);
  double av,bv,*hr,*hs,v1,v2;

  hr = dvector(0,Snp);
  hs = dvector(0,Snp);

  // setup collocation interpolation matrix
  CollMat = dmatrix(0,ntot-1,0,ntot-1);
  for(i = 0; i < ntot; ++i){
    av = (1-Spts[i])? 2*(1+Rpts[i])/(1-Spts[i])-1: 0;
    bv = Spts[i];
    Tri_get_point_shape(&T,av,bv,hr,hs);
    for(j =0; j < ntot; ++j){
      v1 = ddot(Snp+1,hr,1,B->vert[j].a,1);
      v2 = ddot(Snp,hs,1,B->vert[j].b,1);
      CollMat[i][j] = v1*v2;
    }
  }

  Tri_reset_basis(B);

  CollMatIpiv = ivector(0,ntot-1);
  // invert matrix
  dgetrf(ntot,ntot,*CollMat,ntot,CollMatIpiv,info);
  if(info) fprintf(stderr,"Trouble factoring collocation matrix\n");


  Loaded_Surf_File = 1;

  free(hr); free(hs); free(Rpts); free(Spts);
}


void genSurfFile(Element *E, double *x, double *y, double *z, Curve *curve){
  register int i,j,k;
  int      info,eid,l,lm[4];
  int      q1 = E->qa, q2 = E->qb,*vertid,fid, cnt, cnt1;
  int      face = curve->face;
  int      ntot = Snp*(Snp+1)/2;
  double   **sj;
  Coord    X;
  static int *chkfid;

  if(!chkfid){
    chkfid = ivector(0,Snface-1);
    izero(Snface,chkfid,1);
  }

  vertid = curve->info.file.vert;

  if(E->identify() == Nek_Prism)
    q2 = E->qc;

  // find face id;
  cnt = vertid[0] + vertid[1] + vertid[2];
  for(i = 0; i < Snface; ++i)
    if(surfids[i][3] == cnt){ // just search vertices with same vertex id sum
      if(vertid[0] == surfids[i][0]){
  if((vertid[1] == surfids[i][1])||(vertid[1] == surfids[i][2])){

    chkfid[i]++;
    if(chkfid[i] > 1) // check form mutiple calls to same face
      fprintf(stderr,"gensurfFile: Error face %d is being "
        "operated on multiple times\n",i);

    if(vertid[1] == surfids[i][2]){
      Rotate(i,1);
      Reflect(i);
    }
    fid = i;
    break;
  }
      }
      else if(vertid[0] == surfids[i][1]){
  if((vertid[1] == surfids[i][0])||(vertid[1] == surfids[i][2])){
    chkfid[i]++;
    if(chkfid[i] > 1) // check form mutiple calls to same face
      fprintf(stderr,"gensurfFile: Error face %d is being "
        "operated on multiple times\n",i);

    if(vertid[1] == surfids[i][0])
      Reflect(i);
    else
      Rotate(i,2);

    fid = i;
    break;
  }
      }
      else if(vertid[0] == surfids[i][2]){
  if((vertid[1] == surfids[i][0])||(vertid[1] == surfids[i][1])){

    chkfid[i]++;
    if(chkfid[i] > 1) // check form mutiple calls to same face
      fprintf(stderr,"gensurfFile: Error face %d is being "
        "operated on multiple times\n",i);

    if(vertid[1] == surfids[i][1]){
      Rotate(i,2);
      Reflect(i);
    }
    else
      Rotate(i,1); // set up to rotate Feisal to Nektar
    fid = i;
    break;
  }
      }
    }

  // invert basis
  dgetrs('T', ntot, 1, *CollMat,ntot,CollMatIpiv,SurXpts[fid],ntot,info);
  if(info) fprintf(stderr,"Trouble solve collocation X matrix\n");
  dgetrs('T', ntot, 1, *CollMat,ntot,CollMatIpiv,SurYpts[fid],ntot,info);
  if(info) fprintf(stderr,"Trouble solve collocation Y matrix\n");
  dgetrs('T', ntot, 1, *CollMat,ntot,CollMatIpiv,SurZpts[fid],ntot,info);
  if(info) fprintf(stderr,"Trouble solve collocation Z matrix\n");

  // Take out require modes and Backward transformation

  // base it on LGmax at present although might cause problems with
  // trijbwd if LGmax > qa

  sj = dmatrix(0,2,0,LGmax*(LGmax+1)/2-1);
  dzero(3*LGmax*(LGmax+1)/2,sj[0],1);

  lm[0] = LGmax-2;
  lm[1] = LGmax-2;
  lm[2] = LGmax-2;
  lm[3] = max(LGmax-3,0);

  dcopy(3,SurXpts[fid],1,sj[0],1);
  dcopy(3,SurYpts[fid],1,sj[1],1);
  dcopy(3,SurZpts[fid],1,sj[2],1);

  cnt = cnt1 = 3;
  for(i=0;i<3;++i){
    l   = lm[i];
    dcopy(min(Snp-2,l), SurXpts[fid]+cnt,1,sj[0]+cnt1,1);
    dcopy(min(Snp-2,l), SurYpts[fid]+cnt,1,sj[1]+cnt1,1);
    dcopy(min(Snp-2,l), SurZpts[fid]+cnt,1,sj[2]+cnt1,1);
    cnt  += Snp-2;
    cnt1 += l;
  }

  l = lm[3];

  for(i=0;i<l;++i){
    dcopy(min(Snp-3,l)-i,SurXpts[fid]+cnt,1,sj[0]+cnt1,1);
    dcopy(min(Snp-3,l)-i,SurYpts[fid]+cnt,1,sj[1]+cnt1,1);
    dcopy(min(Snp-3,l)-i,SurZpts[fid]+cnt,1,sj[2]+cnt1,1);
    cnt  += Snp-3-i;
    cnt1 += l-i;
  }

  JbwdTri(q1,q2,LGmax,lm,sj[0],x);
  JbwdTri(q1,q2,LGmax,lm,sj[1],y);
  JbwdTri(q1,q2,LGmax,lm,sj[2],z);

  free_dmatrix(sj,0,0);
}


// function to rotate triangular data in SurfXpts anti-clockwise nrot times
static void Rotate(int fid, int nrot){
  register int i,j,n,cnt;
  double **tmp;

  tmp = dmatrix(0,Snp-1,0,Snp-1);

  for(n = 0; n < nrot; ++n){
    for(cnt=i = 0; i < Snp; ++i)
      for(j = 0; j < Snp-i; ++j,cnt++)
  tmp[i][j] = SurXpts[fid][cnt];

    for(cnt=i = 0; i < Snp; ++i)
      for(j = 0; j < Snp-i; ++j,cnt++)
  SurXpts[fid][cnt] = tmp[Snp-1-i-j][i];

    for(cnt=i = 0; i < Snp; ++i)
      for(j = 0; j < Snp-i; ++j,cnt++)
  tmp[i][j] = SurYpts[fid][cnt];

    for(cnt=i = 0; i < Snp; ++i)
      for(j = 0; j < Snp-i; ++j,cnt++)
  SurYpts[fid][cnt] = tmp[Snp-1-i-j][i];

    for(cnt=i = 0; i < Snp; ++i)
      for(j = 0; j < Snp-i; ++j,cnt++)
  tmp[i][j] = SurZpts[fid][cnt];

    for(cnt=i = 0; i < Snp; ++i)
      for(j = 0; j < Snp-i; ++j,cnt++)
  SurZpts[fid][cnt] = tmp[Snp-1-i-j][i];
  }

  free_dmatrix(tmp,0,0);
}


// function to reflect triangular data in SurfXpts about y axis
static void Reflect(int fid){
  register int i,j,cnt;
  double **tmp;

  tmp = dmatrix(0,Snp-1,0,Snp-1);

  for(cnt=i = 0; i < Snp; ++i)
    for(j = 0; j < Snp-i; ++j,cnt++)
      tmp[i][Snp-i-1-j] = SurXpts[fid][cnt];

    for(cnt=i = 0; i < Snp; ++i)
      for(j = 0; j < Snp-i; ++j,cnt++)
  SurXpts[fid][cnt] = tmp[i][j];

    for(cnt=i = 0; i < Snp; ++i)
      for(j = 0; j < Snp-i; ++j,cnt++)
  tmp[i][Snp-i-1-j] = SurYpts[fid][cnt];

    for(cnt=i = 0; i < Snp; ++i)
      for(j = 0; j < Snp-i; ++j,cnt++)
  SurYpts[fid][cnt] = tmp[i][j];

    for(cnt=i = 0; i < Snp; ++i)
      for(j = 0; j < Snp-i; ++j,cnt++)
  tmp[i][Snp-i-1-j] = SurZpts[fid][cnt];

    for(cnt=i = 0; i < Snp; ++i)
      for(j = 0; j < Snp-i; ++j,cnt++)
  SurZpts[fid][cnt] = tmp[i][j];

  free_dmatrix(tmp,0,0);
}
