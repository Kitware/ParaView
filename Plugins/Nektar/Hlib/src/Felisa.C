/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Hlib/src/Felisa.C,v $
 * $Revision: 1.3 $
 * $Date: 2006/08/10 17:44:01 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/

#include <hotel.h>
#include <math.h>
#include <veclib.h>
#include <felisa.h>
#include <string.h>

#include <stdio.h>

#define SCALTOL 1.5
#define TOLV 1E-2

typedef struct fcurve{
  int    id;
  double u;
  struct fcurve *next;
} Fcurve;

typedef struct fsurf{
  int    id;
  double u;
  double v;
  struct fsurf *next;
} Fsurf;

typedef struct fvert{
  int      ncurve;  /* number of curves   that vertex lies in */
  int      nface;   /* number of surfaces that vertex lies in */
  double   x,y,z;   /* global coordinates                     */
  Fcurve  *fcur;    /* curve   information                    */
  Fsurf   *fsur;    /* surface information                    */
} Fvert;

typedef struct splineDat{
  int nu[2];
  double *r;
} SplineDat;

/* local variables to file */
static Fvert      *felvert;
static int         nfvert, nfcurve, nfsurf, nfface, **surfids,Loaded_Surf_File;
static SplineDat  *sdat, *cdat;

// set up face 'fac' of element E with the felisa information
// and modify the global id's in the curve structure
void Felisa_fillElmt(Element *E, int fac, int felfac){

  E->vert[E->vnum(fac,0)].x = felvert[surfids[felfac][0]].x;
  E->vert[E->vnum(fac,0)].y = felvert[surfids[felfac][0]].y;
  E->vert[E->vnum(fac,0)].z = felvert[surfids[felfac][0]].z;

  E->vert[E->vnum(fac,1)].x = felvert[surfids[felfac][1]].x;
  E->vert[E->vnum(fac,1)].y = felvert[surfids[felfac][1]].y;
  E->vert[E->vnum(fac,1)].z = felvert[surfids[felfac][1]].z;

  E->vert[E->vnum(fac,2)].x = felvert[surfids[felfac][2]].x;
  E->vert[E->vnum(fac,2)].y = felvert[surfids[felfac][2]].y;
  E->vert[E->vnum(fac,2)].z = felvert[surfids[felfac][2]].z;


  E->curve->info.file.vert[0] = surfids[felfac][0];
  E->curve->info.file.vert[1] = surfids[felfac][1];
  E->curve->info.file.vert[2] = surfids[felfac][2];
}

void Load_Felisa_Surface(char *name){
  register int i,j;
  int      v[3],face,err,n;
  double   len,tolg,up,vp,X[3];
  FILE     *infile;
  char     buf[BUFSIZ];
  Fcurve   *fc;
  Fsurf    *ff;

  if(Loaded_Surf_File) return; // allow for multiple calls

  if(!(infile = fopen(name,"r"))){
    sprintf(buf,"%s.fro",strtok(name,"."));
    if(!(infile = fopen(buf,"r"))){
      fprintf(stderr,"unable to open file %s or %s\n",name,buf);
      exit(-1);
    }
  }

  fgets(buf,BUFSIZ,infile);
  sscanf(buf,"%d%d%*d%*d%d%d",&nfface,&nfvert,&nfcurve,&nfsurf);

  felvert = (Fvert *)calloc(nfvert,sizeof(Fvert));
  cdat = (SplineDat *)malloc(nfcurve*sizeof(SplineDat));
  sdat = (SplineDat *)malloc(nfsurf*sizeof(SplineDat));

  /* store global vertices as a check */
  for(i = 0; i < nfvert; ++i){
    fgets(buf,BUFSIZ,infile);
    sscanf(buf,"%*d%lf%lf%lf",&felvert[i].x,&felvert[i].y,&felvert[i].z);
  }

  /* read in global vertices */
  surfids = imatrix(0,nfface,0,4);
  for(i = 0; i < nfface; ++i){
    fgets(buf,BUFSIZ,infile);
    sscanf(buf,"%*d%d%d%d%d",surfids[i],surfids[i]+1,surfids[i]+2,
     surfids[i]+3);
    surfids[i][0]--;
    surfids[i][1]--;
    surfids[i][2]--;
    surfids[i][3]--;
    surfids[i][4] = surfids[i][0] + surfids[i][1] + surfids[i][2];
  }

  /* assemble curve information into vertex structure */
  for(i = 0; i < nfcurve; ++i){
    fscanf(infile,"%*d%d",&n);
    for(j = 0; j < n; ++j){
      fscanf(infile,"%d%lf",v,&up);
      v[0]--;

      fc = felvert[v[0]].fcur;
      felvert[v[0]].fcur = (Fcurve *)malloc(sizeof(Fcurve));
      felvert[v[0]].fcur->id = i;
      felvert[v[0]].fcur->u = up;
      felvert[v[0]].fcur->next = fc;
      felvert[v[0]].ncurve++;
    }
  }

  /* assemble face information into vertex structure */
  for(i = 0; i < nfsurf; ++i){
    fscanf(infile,"%*d%d",&n);
    for(j = 0; j < n; ++j){
      fscanf(infile,"%d%lf%lf",v,&up,&vp);
      v[0]--;

      ff = felvert[v[0]].fsur;
      felvert[v[0]].fsur = (Fsurf *)malloc(sizeof(Fsurf));
      felvert[v[0]].fsur->id = i;
      felvert[v[0]].fsur->u  = up;
      felvert[v[0]].fsur->v  = vp;
      felvert[v[0]].fsur->next = ff;
      felvert[v[0]].nface++;
    }
  }

  /* Load Spline Data for curves  */
  for(i = 0; i < nfcurve; ++i){
    fscanf(infile,"%*d%d",cdat[i].nu);
    cdat[i].r = dvector(0,6*cdat[i].nu[0]-1);

    for(j = 0; j < 6*cdat[i].nu[0]; ++j)
      fscanf(infile,"%lf",cdat[i].r+j);
  }

  /* Load Spline Data for curves  */
  for(i = 0; i < nfsurf; ++i){
    fscanf(infile,"%*d%d%d",sdat[i].nu,sdat[i].nu+1);
    sdat[i].r = dvector(0,12*sdat[i].nu[0]*sdat[i].nu[1]-1);

    for(j = 0; j < 12*sdat[i].nu[0]*sdat[i].nu[1]; ++j)
      fscanf(infile,"%lf",sdat[i].r+j);
  }

  tolg = 0.0;
  for(i = 0; i < nfvert; ++i){
    for(ff = felvert[i].fsur; ff; ff = ff->next){
      fgsurf(0,sdat[ff->id].nu[0],sdat[ff->id].nu[1],sdat[ff->id].r,
       ff->u,ff->v,X,err);
      len = sqrt(pow(X[0]-felvert[i].x,2)+pow(X[1]-felvert[i].y,2)+
     pow(X[2]-felvert[i].z,2));
      tolg = max(tolg,len);

    }
  }
  ROOTONLY
    fprintf(stderr,"Felisa info - tolg  : %lf \n",SCALTOL*tolg);
  tolg *= SCALTOL;
  settolg(tolg);

  fprintf(stderr,"Generating felisa surface [");
  Loaded_Surf_File = 1;
}

void Free_Felisa_data(void){
  register int i;
  Fcurve *fc,*fc1;
  Fsurf  *ff,*ff1;

  if(Loaded_Surf_File){
    for(i = 0; i < nfvert; ++i){
      fc = felvert[i].fcur;
      while(fc){
  fc1 = fc->next;
  free(fc);
  fc = fc1;
      }
      ff = felvert[i].fsur;
      while(ff){
  ff1 = ff->next;
  free(ff);
  ff = ff1;
      }
    }
    free(felvert);

    for(i = 0; i < nfcurve; ++i)
      free(cdat[i].r);
    free(cdat);

    for(i = 0; i < nfsurf; ++i)
      free(sdat[i].r);
    free(sdat);

    free_imatrix(surfids,0,0);
    fprintf(stderr,"]\n");
    Loaded_Surf_File = 0;
  }
}

static void MinLength( int np, double **xl, SplineDat *sdat );
static void dump_uv(int felsu,int nu,int nv, double **u, double **v, int eid);
static void Findmin1D(int np, double *u, double *zw, double **xg,
          SplineDat *cdat, int eflag);
static void Findmin(int np, double *u, double *v, double *z, SplineDat *sdat,
        int felsu);
static void Findmin2d(int n1, int n2, double **u, double **v, double *z1,
          double *z2, SplineDat *sdat, int felsu);

#define SurfFac 0.2
void genFelFile(Element *E, double *x, double *y, double *z, Curve *curve){
  register int i,j;
  int *fvertid,eflag[Max_Nverts],face,err;
  int qa = E->qa, qb = E->qb,felsu = 0;
  double **xl,**xg,**u,**v,uc,u1,u2,v1,v2,*za,*zb,*w,*zb1,len,d1,d2,deluv;
  Fcurve  *fc,*fc1;
  Fsurf   *fs,*fs1;
  int nfv,cnt,nonlinear=1,nltrip=0;
  int *iflag = ivector(0, nfsurf-1);
  double *utmp,*vtmp,len1,len2;
  utmp = dvector(0,2*QGmax+2);
  vtmp = utmp + QGmax+1;
  static int surfcount;

  if(iparam("NONONLINEAR"))
    nonlinear = 0;

  surfcount++;
  if(!(surfcount%10)) /* put a counter to screen to asses surface generation*/
    fprintf(stderr,"* ");

  if(E->identify() == Nek_Prism)
    qb = E->qc;

  getzw(qa,&za,&w,'a');
  getzw(qb,&zb,&w,'b');
  getzw(qb+1,&zb1,&w,'a');

  u  = dmatrix(0,qb-1,0,qa-1);
  v  = dmatrix(0,qb-1,0,qa-1);
  xl = dmatrix(0,QGmax-1,0,1);
  xg = dmatrix(0,max(4,QGmax)-1,0,2);

  face  = curve->face;
  fvertid = curve->info.file.vert;

  nfv = E->Nfverts(face);
  if(nfv != 3){
    fprintf(stderr, "genFelFile: not setup for quad. faces\n");
    exit(-1);
  }

  if((E->identify() != Nek_Tet)&&(E->identify() != Nek_Prism)){
    fprintf(stderr, "genFelFile: only set up for tetrahedra"
      " and triangular faces of prisms \n");
    exit(-1);
  }

  /* check to see if vertices match */
  for(i = 0; i < nfv; ++i)
    if((fabs(felvert[fvertid[i]].x-E->vert[E->vnum(face,i)].x) >
  TOLV*(1+fabs(felvert[fvertid[i]].x)))||
       (fabs(felvert[fvertid[i]].y-E->vert[E->vnum(face,i)].y) >
  TOLV*(1+fabs(felvert[fvertid[i]].y)))||
       (fabs(felvert[fvertid[i]].z-E->vert[E->vnum(face,i)].z) >
  TOLV*(1+fabs(felvert[fvertid[i]].z)))){
      fprintf(stderr,"Surface Vertex %d in element %d does "
        "not match vertex %d in felisa file %s\n",
        i+1,E->id+1,fvertid[i]+1,curve->info.file.name);
    }

  /* first check to see if any edge is on a curve */
  for(i = 0; i < nfv; ++i){
    eflag[i] = -1;
    if(felvert[fvertid[i]].ncurve&&felvert[fvertid[(i+1)%3]].ncurve)
      for(fc = felvert[fvertid[i]].fcur; fc; fc = fc->next)
  for(fc1 = felvert[fvertid[(i+1)%3]].fcur; fc1; fc1 = fc1->next)
    if(fc->id == fc1->id)
      eflag[i] = fc->id;
  }

  /* find felisa surface id */
  izero(nfsurf, iflag, 1);
  for(i = 0; i < nfv; ++i){
    for(fs = felvert[fvertid[i]].fsur; fs; fs = fs->next)
  ++iflag[fs->id];
  }

  /* this finds first occurance */
  cnt = 0;
  for(j=0;j<nfsurf;++j)
    if(iflag[j] == 3){
  felsu = j;
  cnt++;
      }

  if(cnt > 1){
    /* if cnt > 1 then vertices lie on two surfaces so we have to
       resort to using the surfids list from .fro file to identify the
       surface */

    cnt = fvertid[0] + fvertid[1] + fvertid[2];
    felsu = 0;

    for(i = 0; i < nfface; ++i)
      if(surfids[i][4] == cnt){ // just search vertices with same vertex id sum
  if(fvertid[0] == surfids[i][0]){
    if((fvertid[1] == surfids[i][1])||(fvertid[1] == surfids[i][2])){
      felsu = surfids[i][3];
      break;
    }
  }
  else if(fvertid[0] == surfids[i][1]){
    if((fvertid[1] == surfids[i][0])||(fvertid[1] == surfids[i][2])){
      felsu = surfids[i][3];
      break;
    }
  }
  else if(fvertid[0] == surfids[i][2]){
    if((fvertid[1] == surfids[i][0])||(fvertid[1] == surfids[i][1])){
      felsu = surfids[i][3];
      break;
    }
  }
      }

    if(!felsu){
      fprintf(stderr,"can not find felisa face for curved "
        "face %d in element %d\n",face+1,E->id+1);
      exit(-1);
    }
  }

  // only set up for tetrahedra and prism triangular faces */
  /* calculate x,y,z, points on any edge along a curve */
  for(i = 0; i < 3; ++i)
    if(eflag[i]+1){
      /* calculate x y z coordinates based on quadrature distribution in
   parametric space */
      switch(i){
      case 0:
  for(fc = felvert[fvertid[0]].fcur; fc; fc = fc->next)
    if(fc->id == eflag[i])
      u1 = fc->u;
  for(fc = felvert[fvertid[1]].fcur; fc; fc = fc->next)
    if(fc->id == eflag[i])
      u2 = fc->u;

  for(j = 0; j < qa; ++j)
    utmp[j] = u1*(1-za[j])*0.5 + u2*(1+za[j])*0.5;

  if(nonlinear)
    Findmin1D(qa,utmp,za,xg,cdat,eflag[i]);

  for(err=0,j = 0; j < qa; ++j){
    fgcurv(0,cdat[eflag[i]].nu[0],cdat[eflag[i]].r,utmp[j],xg[j],err);

    if(err == -1){
      fprintf(stderr,"failed to find coordinate along "
        "curve in element %d\n",E->id+1);
      exit(-1);
    }
  }

        locuv(qa,*xg,*xl,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r);

  for(j = 0; j < qa; ++j){
    u[0][j] = xl[j][0];
    v[0][j] = xl[j][1];
  }
  break;
      case 1:
  /* find u value at felvertices */
  for(fc = felvert[fvertid[1]].fcur; fc; fc = fc->next)
    if(fc->id == eflag[i])
      u1 = fc->u;

  for(fc = felvert[fvertid[2]].fcur; fc; fc = fc->next)
    if(fc->id == eflag[i])
      u2 = fc->u;

  for(err=0,j = 0; j < qb+1; ++j)
    utmp[j] = u1*(1-zb1[j])*0.5 + u2*(1+zb1[j])*0.5;

  if(nonlinear)
    Findmin1D(qb+1,utmp,zb1,xg,cdat,eflag[i]);

  for(err=0,j = 0; j < qb; ++j){
    fgcurv(0,cdat[eflag[i]].nu[0],cdat[eflag[i]].r,utmp[j],xg[j],err);

    if(err == -1){
      fprintf(stderr,"failed to find coordinate along "
        "curve in element %d\n",E->id+1);
      exit(-1);
    }
        }

        locuv(qb,*xg,*xl,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r);

  for(j = 0; j < qb; ++j){
    u[j][qa-1] = xl[j][0];
    v[j][qa-1] = xl[j][1];
  }
  break;
      case 2:
  /* find u value at felvertices */
  for(fc = felvert[fvertid[0]].fcur; fc; fc = fc->next)
    if(fc->id == eflag[i])
      u1 = fc->u;
  for(fc = felvert[fvertid[2]].fcur; fc; fc = fc->next)
    if(fc->id == eflag[i])
      u2 = fc->u;

  for(err=0,j = 0; j < qb+1; ++j)
    utmp[j] = u1*(1-zb1[j])*0.5 + u2*(1+zb1[j])*0.5;

  if(nonlinear)
    Findmin1D(qb+1,utmp,zb1,xg,cdat,eflag[i]);

  for(err=0,j = 0; j < qb; ++j){
    fgcurv(0,cdat[eflag[i]].nu[0],cdat[eflag[i]].r,utmp[j],xg[j],err);

    if(err == -1){
      fprintf(stderr,"failed to find coordinate along "
        "curve in element %d\n",E->id+1);
      exit(-1);
    }
        }

        locuv(qb,*xg,*xl,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r);

  for(j = 0; j < qb; ++j){
    u[j][0] = xl[j][0];
    v[j][0] = xl[j][1];
  }
  break;
      }
    }
    else{
      /* calculate u,v as:
       * 1.- linear fit between vertices
       * 2.- non-linear: min. length  .... JP July 99
       */
      switch(i){
      case 0:
  /* find vertex data */
  for(fs = felvert[fvertid[0]].fsur; fs; fs = fs->next)
    if(fs->id == felsu){
      u1 = fs->u;
      v1 = fs->v;
    }
  for(fs = felvert[fvertid[1]].fsur; fs; fs = fs->next)
    if(fs->id == felsu){
      u2 = fs->u;
      v2 = fs->v;
    }

  for(j = 0; j < qa; ++j){
    u[0][j] = u1*(1-za[j])*0.5 + u2*(1+za[j])*0.5;
    v[0][j] = v1*(1-za[j])*0.5 + v2*(1+za[j])*0.5;
  }

  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[0][0],v[0][1],xg[0],err);
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[0][1],v[0][1],xg[1],err);
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[0][qa-2],v[0][qa-2],xg[2],err);
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[0][qa-1],v[0][qa-1],xg[3],err);

  len1 = sqrt((xg[1][0] - xg[0][0])*(xg[1][0] - xg[0][0]) +
        (xg[1][1] - xg[0][1])*(xg[1][1] - xg[0][1]) +
        (xg[1][2] - xg[0][2])*(xg[1][2] - xg[0][2]));
  len2 = sqrt((xg[3][0] - xg[2][0])*(xg[3][0] - xg[2][0]) +
        (xg[3][1] - xg[2][1])*(xg[3][1] - xg[2][1]) +
        (xg[3][2] - xg[2][2])*(xg[3][2] - xg[2][2]));

  if(nonlinear){
    Findmin(qa, u[0], v[0], za, sdat, felsu);
    nltrip = 1;
  }

  break;
      case 1:
  /* find felvertex data */
  for(fs = felvert[fvertid[1]].fsur; fs; fs = fs->next)
    if(fs->id == felsu){
      u1 = fs->u;
      v1 = fs->v;
    }
  for(fs = felvert[fvertid[2]].fsur; fs; fs = fs->next)
    if(fs->id == felsu){
      u2 = fs->u;
      v2 = fs->v;
    }

  for(j = 0; j < qb; ++j){
    u[j][qa-1] = u1*(1-zb[j])*0.5 + u2*(1+zb[j])*0.5;
    v[j][qa-1] = v1*(1-zb[j])*0.5 + v2*(1+zb[j])*0.5;
  }

  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[0][qa-1],v[0][qa-1],xg[0],err);
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[1][qa-1],v[1][qa-1],xg[1],err);
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[qb-1][qa-1],v[qb-1][qa-1],xg[2],err);
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u2,v2,xg[3],err);

  len1 = sqrt((xg[1][0] - xg[0][0])*(xg[1][0] - xg[0][0]) +
        (xg[1][1] - xg[0][1])*(xg[1][1] - xg[0][1]) +
        (xg[1][2] - xg[0][2])*(xg[1][2] - xg[0][2]));
  len2 = sqrt((xg[3][0] - xg[2][0])*(xg[3][0] - xg[2][0]) +
        (xg[3][1] - xg[2][1])*(xg[3][1] - xg[2][1]) +
        (xg[3][2] - xg[2][2])*(xg[3][2] - xg[2][2]));

  if(nonlinear){
    dcopy(qb,u[0]+qa-1,qa,utmp,1); utmp[qb] = u2;
    dcopy(qb,v[0]+qa-1,qa,vtmp,1); vtmp[qb] = v2;
    Findmin(qb+1, utmp, vtmp, zb1, sdat, felsu);
    dcopy(qb,utmp,1,u[0]+qa-1,qa);
    dcopy(qb,vtmp,1,v[0]+qa-1,qa);
    nltrip = 1;
  }
  break;
      case 2:
  /* find felvertex data */
  for(fs = felvert[fvertid[0]].fsur; fs; fs = fs->next)
    if(fs->id == felsu){
      u1 = fs->u;
      v1 = fs->v;
    }
  for(fs = felvert[fvertid[2]].fsur; fs; fs = fs->next)
    if(fs->id == felsu){
      u2 = fs->u;
      v2 = fs->v;
    }
  for(j = 0; j < qb; ++j){
    u[j][0] = u1*(1-zb[j])*0.5 + u2*(1+zb[j])*0.5;
    v[j][0] = v1*(1-zb[j])*0.5 + v2*(1+zb[j])*0.5;
  }

  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[0][0],v[0][0],xg[0],err);
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[1][0],v[1][0],xg[1],err);
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[qb-1][0],v[qb-1][0],xg[2],err);
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u2,v2,xg[3],err);

  len1 = sqrt((xg[1][0] - xg[0][0])*(xg[1][0] - xg[0][0]) +
        (xg[1][1] - xg[0][1])*(xg[1][1] - xg[0][1]) +
        (xg[1][2] - xg[0][2])*(xg[1][2] - xg[0][2]));
  len2 = sqrt((xg[3][0] - xg[2][0])*(xg[3][0] - xg[2][0]) +
        (xg[3][1] - xg[2][1])*(xg[3][1] - xg[2][1]) +
        (xg[3][2] - xg[2][2])*(xg[3][2] - xg[2][2]));

  if(nonlinear){
    dcopy(qb,u[0],qa,utmp,1); utmp[qb] = u2;
    dcopy(qb,v[0],qa,vtmp,1); vtmp[qb] = v2;
    Findmin(qb+1, utmp, vtmp, zb1, sdat, felsu);
    dcopy(qb,utmp,1,u[0],qa);
    dcopy(qb,vtmp,1,v[0],qa);
    nltrip = 1;
  }
  break;
      default:
  break;
      }
    }


  /* get top vertices for blending */
  for(fs = felvert[fvertid[2]].fsur; fs; fs = fs->next)
    if(fs->id == felsu){
      u2 = fs->u;
      v2 = fs->v;
    }


  /* blend u,v in the interior based on edge values */
  for(i = 1; i < qa-1; ++i)
    for(j = 1; j < qb; ++j){

      u[j][i] = u[0][i]*(1-zb[j])*0.5 + u[j][0]*(1-za[i])*0.5 +
          u[j][qa-1]*(1+za[i])*0.5 - (1-za[i])*(1-zb[j])*0.25*u[0][0] -
                (1+za[i])*(1-zb[j])*0.25*u[0][qa-1];
      v[j][i] = v[0][i]*(1-zb[j])*0.5 + v[j][0]*(1-za[i])*0.5 +
          v[j][qa-1]*(1+za[i])*0.5 - (1-za[i])*(1-zb[j])*0.25*v[0][0] -
                (1+za[i])*(1-zb[j])*0.25*v[0][qa-1];
    }


  /* turn off if an edge lies on curve */
  //if((eflag[0]+1)&&(eflag[1]+1)&&(eflag[2]+1))

  if(nltrip){
    double **ustore,**vstore;
    double utmp,vtmp;
    ustore = dmatrix(0,qb,0,qa-1);
    vstore = dmatrix(0,qb,0,qa-1);

    for(i = 0; i < qb; ++i){
      dcopy(qa,u[i],1,ustore[i],1);
      dcopy(qa,v[i],1,vstore[i],1);
    }

    dcopy(qa,&u2,0,ustore[qb],1);
    dcopy(qa,&v2,0,vstore[qb],1);

    Findmin2d(qa,qb+1,ustore,vstore,za,zb1,sdat,felsu);
    for(i = 0; i < qb; ++i){
      dcopy(qa,ustore[i],1,u[i],1);
      dcopy(qa,vstore[i],1,v[i],1);
    }

    free_dmatrix(ustore,0,0);
    free_dmatrix(vstore,0,0);
  }

  dump_uv(felsu,qa,qb,u,v,E->id+1);

  /* find x,y,z from  u,v, */
  for(i = 0; i < qa; ++i)
    for(j = 0; j < qb; ++j){
      fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
             u[j][i],v[j][i],*xg,err);
      if(err == -1){
  fprintf(stderr,"failed to find coordinate along "
    "surface in element %d\n",E->id+1);
  exit(-1);
      }
      x[i + j*qa] = xg[0][0];
      y[i + j*qa] = xg[0][1];
      z[i + j*qa] = xg[0][2];

    }

  free(iflag);
  free_dmatrix(u,0,0);  free_dmatrix(v,0,0);
  free_dmatrix(xl,0,0); free_dmatrix(xg,0,0);

  free(utmp);
}

#define NSURF 200

static void dump_uv(int felsu,int nu,int nv, double **u, double **v, int eid){
  register int i,j;
  char file[BUFSIZ];
  static  int init[NSURF];
  FILE  *fp;

  if(felsu +1 > NSURF){
    fprintf(stderr,"error in dump_uv: NSURF needs increasing\n");
    exit(-1);
  }

  sprintf(file,"uvmap_%d.tec",felsu);

  if(!init[felsu]){
    fp = fopen(file,"w");
    fprintf(fp,"VARIABLES = u,v\n");
    init[felsu] = 1;
  }
  else
    fp = fopen(file,"a");


  fprintf(fp,"ZONE T=\"Elmt %d\" F=POINT, I=%d, J=%d\n",eid,nu,nv);

  for(i = 0; i < nv; ++i)
    for(j = 0; j < nu; ++j)
     fprintf(fp,"%lf %lf\n",u[i][j],v[i][j]);

     fclose(fp);
}


#define NITER 5000

static void Findmin1D(int np, double *u, double *zw, double **xg,
        SplineDat *cdat, int eflag){
  register int  i,it,icount,iu;
  int     keepu,err;
  double ulen, uint,u1;
  double ratio, almin,len1,len2,un;
  double *alen,tol;

  tol = dparam("findminTOL");
  tol = (tol)? tol: 0.3;

  // Determine the step length based on the length of the segment

  ulen = fabs(u[np-1]-u[0]);
  uint = tol*ulen/(double) (np*np);

  alen = dvector(0,2);

  //  Minimization in the curved segment

  for(it = 0; it < NITER; ++it){
    icount = 0;

    for(i = 1; i < np-1; ++i){
      // Identify the neighbouring points
      fgcurv(0,cdat[eflag].nu[0],cdat[eflag].r,u[i-1],xg[0],err);
      fgcurv(0,cdat[eflag].nu[0],cdat[eflag].r,u[i+1],xg[2],err);

      // Calculates the ratio of the sampling point lengths
      ratio = (zw[i+1]-zw[i])/(zw[i]-zw[i-1]);
      almin = 1.e+36;

      u1 = u[i];


      // Select a set of 3 sampling points
      for(iu = 0; iu < 3; ++iu){
        un = u1 + (iu-1)*uint;

  fgcurv(0,cdat[eflag].nu[0],cdat[eflag].r,un,xg[1],err);

  if(err)
    alen[iu] = almin;
  else{
    len1 = (xg[1][0]-xg[0][0])*(xg[1][0]-xg[0][0]) +
      (xg[1][1]-xg[0][1])*(xg[1][1]-xg[0][1])
      + (xg[1][2]-xg[0][2])*(xg[1][2]-xg[0][2]);
    len2 = (xg[1][0]-xg[2][0])*(xg[1][0]-xg[2][0]) +
      (xg[1][1]-xg[2][1])*(xg[1][1]-xg[2][1])
        + (xg[1][2]-xg[2][2])*(xg[1][2]-xg[2][2]);
    alen[iu] = ratio*len1 + len2;
  }

  if(alen[iu] < almin){
    keepu = iu;
    almin = alen[iu];
    u[i] = un;
  }
      }

      // Is the starting point is the minimum point? If so increment count.
      if(keepu == 1) icount++;
    }

    // if icount = np-2 then no points have moved so exit loop.
    if(icount == np-2) break;
  }

  //fprintf(stderr,"iterations: %d\n",it);
  if(it == NITER)
    fprintf(stderr, "Warning: More that %d iteration in findmin\n",NITER);

  free(alen);
}

static void Findmin(int np, double *u, double *v, double *zw,
        SplineDat *sdat, int felsu){
  register int  i,it,icount,iu,iv;
  int   keepu,keepv,err;
  double ulen, uint,vint, x1[3],x2[3],x3[3],u1,v1;
  double ratio, almin,len1,len2,un,vn;
  double **alen,tol;

  tol = dparam("findminTOL");
  tol = (tol)? tol: 0.3;

  // Determine the step length based on the length of the segment

  ulen = sqrt((u[np-1]-u[0])*(u[np-1]-u[0])+(v[np-1]-v[0])*(v[np-1]-v[0]));
  uint = vint = tol*ulen/(double) (np*np);

  alen = dmatrix(0,2,0,2);

  //  Minimization in the parameter plane (2D)

  for(it = 0; it < NITER; ++it){
    icount = 0;

    for(i = 1; i < np-1; ++i){
      // Identify the neighbouring points
      fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
       u[i-1],v[i-1],x1,err);
      fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
       u[i+1],v[i+1],x3,err);

      // Calculates the ratio of the sampling point lengths
      ratio = (zw[i+1]-zw[i])/(zw[i]-zw[i-1]);
      almin = 1.e+36;

      u1 = u[i];
      v1 = v[i];

      // Select a set of 9 sampling points
      for(iu = 0; iu < 3; ++iu){
        un = u1 + (iu-1)*uint;
        for(iv = 0; iv < 3; ++iv){
          vn = v1 + (iv-1)*vint;

          fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
       un,vn,x2,err);

    if(err)
      alen[iv][iu] = almin;
    else{
      len1 = (x2[0]-x1[0])*(x2[0]-x1[0]) + (x2[1]-x1[1])*(x2[1]-x1[1])
        + (x2[2]-x1[2])*(x2[2]-x1[2]);
      len2 = (x2[0]-x3[0])*(x2[0]-x3[0]) + (x2[1]-x3[1])*(x2[1]-x3[1])
        + (x2[2]-x3[2])*(x2[2]-x3[2]);
      alen[iv][iu] = ratio*len1 + len2;
    }

          if(alen[iv][iu] < almin){
            keepu = iu;
            keepv = iv;
            almin = alen[iv][iu];
            u[i] = un;
            v[i] = vn;
          }

        }
      }

      // Is the starting point is the minimum point? If so increment count.
      if((keepu == 1)&&(keepv == 1)) icount++;
    }

    // if icount = np-2 then no points have moved so exit loop.
    if(icount == np-2) break;
  }

  //fprintf(stderr,"iterations: %d\n",it);
  if(it == NITER)
    fprintf(stderr, "Warning: More that %d iteration in findmin\n",NITER);

  free_dmatrix(alen,0,0);
}

static void Findmin2d(int n1, int n2, double **u, double **v, double *z1,
          double *z2, SplineDat *sdat, int felsu){
  register int  i,j,it,icount,iu,iv;
  int   keepu,keepv,err;
  double ulen, uint,vint, x1[3],x2[3],x3[3],x4[3],x5[3],u1,v1;
  double almin,len1,len2,len3,len4,totlen1,totlen2,un,vn,A,B,C,D,pen;
  double **alen,tol;

  tol = dparam("findminTOL");
  tol = (tol)? tol: 0.3;

  /* Determine the step length based on the average length
     bounding surface and order  */

  ulen = (sqrt((u[0][n1-1]-u[0][0])*(u[0][n1-1]-u[0][0]) +
         (v[0][n1-1]-v[0][0])*(v[0][n1-1]-v[0][0])) +
    sqrt((u[n2-1][0]-u[0][0])*(u[n2-1][0]-u[0][0]) +
         (v[n2-1][0]-v[0][0])*(v[n2-1][0]-v[0][0])) +
    sqrt((u[n2-1][n1-1]-u[0][n1-1])*(u[n2-1][n1-1]-u[0][n1-1]) +
         (v[n2-1][n1-1]-v[0][n1-1])*(v[n2-1][n1-1]-v[0][n1-1])))/3.0;

  uint = vint = tol*ulen/(double) (n1*n1);

  alen = dmatrix(0,2,0,2);

  //  Minimization in the parameter plane (2D)

  for(it = 0; it < NITER; ++it){
    icount = 0;

    for(i = 1; i < n1-1; ++i){
      for(j = 1; j < n2-1; ++j){
  // Identify the neighbouring points
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[j][i-1],v[j][i-1],x1,err);
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[j][i+1],v[j][i+1],x3,err);
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[j-1][i],v[j-1][i],x4,err);
  fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
         u[j+1][i],v[j+1][i],x5,err);

  totlen1 = (x3[0]-x1[0])*(x3[0]-x1[0]) + (x3[1]-x1[1])*(x3[1]-x1[1])
    + (x3[2]-x1[2])*(x3[2]-x1[2]);
  totlen2 = (x5[0]-x4[0])*(x5[0]-x4[0]) + (x5[1]-x4[1])*(x5[1]-x4[1])
    + (x5[2]-x4[2])*(x5[2]-x4[2]);

  /*A = (z1[i+1]-z1[i])/(z1[i+1]-z1[i-1]);
    B = (z1[i]-z1[i-1])/(z1[i+1]-z1[i-1]);
    C = (z2[j+1]-z2[j])/(z2[j+1]-z2[j-1]);
    D = (z2[j]-z2[j-1])/(z2[j+1]-z2[j-1]); */

  A = 1.0/(z1[i]-z1[i-1]);
  B = 1.0/(z1[i+1]-z1[i]);
  C = 1.0/(z2[j]-z2[j-1]);
  D = 1.0/(z2[j+1]-z2[j]);

  // Calculates the ratio of the sampling point lengths
  almin = 1.e+36;

  u1 = u[j][i];
  v1 = v[j][i];

  // Select a set of 9 sampling points
  for(iu = 0; iu < 3; ++iu){
    un = u1 + (iu-1)*uint;
    for(iv = 0; iv < 3; ++iv){
      vn = v1 + (iv-1)*vint;

      fgsurf(0,sdat[felsu].nu[0],sdat[felsu].nu[1],sdat[felsu].r,
       un,vn,x2,err);
      //if(err)
      //fprintf(stderr,"warning fgsurf error in Findmin2d\n");

      if(err)
        alen[iv][iu] = almin;
      else{
        len1 = (x2[0]-x1[0])*(x2[0]-x1[0]) + (x2[1]-x1[1])*(x2[1]-x1[1])
    + (x2[2]-x1[2])*(x2[2]-x1[2]);
        len2 = (x2[0]-x3[0])*(x2[0]-x3[0]) + (x2[1]-x3[1])*(x2[1]-x3[1])
    + (x2[2]-x3[2])*(x2[2]-x3[2]);
        len3 = (x2[0]-x4[0])*(x2[0]-x4[0]) + (x2[1]-x4[1])*(x2[1]-x4[1])
    + (x2[2]-x4[2])*(x2[2]-x4[2]);
        len4 = (x2[0]-x5[0])*(x2[0]-x5[0]) + (x2[1]-x5[1])*(x2[1]-x5[1])
    + (x2[2]-x5[2])*(x2[2]-x5[2]);
        pen = len1 + len2 - totlen1 + len3 + len4 - totlen2;

        alen[iv][iu]  = A*len1 + B*len2 + C*len3 + D*len4; // + pen;


      }

      if(alen[iv][iu] < almin){
        keepu = iu;
        keepv = iv;
        almin = alen[iv][iu];
        u[j][i] = un;
        v[j][i] = vn;
      }
    }
  }

  // Is the starting point is the minimum point? If so increment count.
  if((keepu == 1)&&(keepv == 1)) icount++;
      }
    }

    // if icount = (n1-2)*(n2-2) then no points have moved so exit loop.
    if(icount == (n1-2)*(n2-2)) break;
  }

  //  fprintf(stderr,"2d iterations: %d\n",it);
  if(it == NITER)
    fprintf(stderr, "Warning: More that %d iteration in findmin\n",NITER);

  free_dmatrix(alen,0,0);
}
