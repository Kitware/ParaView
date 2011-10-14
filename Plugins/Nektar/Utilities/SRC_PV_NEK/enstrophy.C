/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/enstrophy.C,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/08 14:18:48 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <veclib.h>
#include <nektar.h>
#include <gen_utils.h>

int DIM;

/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "entrosphy";
char *usage  = "entrosphy:  [options]  -r file[.rea]  input[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "-a      ... calculate entrosphy over all field \n"
  "-l #    ... Value of contour to be extracted (defaut 0) \n"
  "-f #    ... location of field to be extracted \n"
#if DIM == 2
  "-n #    ... Number of mesh points. Default is 15\n";
#else
  "-n #    ... Number of mesh points.\n";
#endif

typedef struct body{
  int N;       /* number of faces    */
  int *elmt;   /* element # of face  */
  int *faceid; /* face if in element */
} Body;

static Body bdy;

typedef struct range{
  double x[2];
  double y[2];
  double z[2];
} Range;

static Range *rnge;

typedef struct iso{
  int n;
  double *x;
  double *y;
  double *z;
} Iso;

static int  setup (FileList *f, Element_List **U, int *nftot);
static void ReadCopyField (FileList *f, Element_List **U);
static void Get_Body(FILE *fp);
static void dump_faces(FILE *out, Element_List **E, Coord X, int nel, int zone,
           int nfields);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void Isocontour(Element_List **E, FILE *out, int nfields);
static void Write_Contour(FILE *out, char typ, int nel, Iso **iso, double val);
static double Dump_Contour(FILE *out, int nnodes, double *x, double *y,
       double *z, double *c, double *wm,
         int nelmts, int **tets, double val, int zoneNum);
static int Check_range(Element *E);




main (int argc, char *argv[]){
  register int  i,k;
  int       dump=0,nfields,nftot;
  FileList  f;
  Element_List **master;

  init_comm(&argc, &argv);

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  master  = (Element_List **) malloc(_MAX_FIELDS*sizeof(Element_List *));
  nfields = setup (&f, master, &nftot);

  ReadCopyField(&f,master);

  DIM = master[0]->fhead->dim();


  Isocontour(master,f.out.fp, nfields);

  exit_comm();

  return 0;
}

/* Inner project w.r.t. orthogonal modes */

void OrthoInnerProduct(Element_List *EL, Element_List *ELf){
  Element *U, *Uf;
  if(EL->fhead->dim() == 2)
    for(U=EL->fhead,Uf = ELf->fhead;U;U = U->next,Uf = Uf->next)
      Uf->Ofwd(U->h[0], Uf->h[0], Uf->lmax);
  else
    for(U=EL->fhead,Uf = ELf->fhead;U;U = U->next,Uf = Uf->next)
      Uf->Ofwd(U->h_3d[0][0], Uf->h_3d[0][0], Uf->lmax);
}

void OrthoJTransBwd(Element_List *EL, Element_List *ELf){
  Element *U, *Uf;

 if(EL->fhead->dim() == 2)
   for(U=EL->fhead,Uf = ELf->fhead;U;U = U->next,Uf = Uf->next)
     Uf->Obwd(U->h[0], Uf->h[0], Uf->lmax);
 else
   for(U=EL->fhead,Uf = ELf->fhead;U;U = U->next,Uf = Uf->next)
     Uf->Obwd(U->h_3d[0][0], Uf->h_3d[0][0], Uf->lmax);
}


void init_ortho_basis(void);

Gmap *gmap;

int  setup (FileList *f, Element_List **U, int *nft){
  int    i,shuff;
  int    nfields,nftot;
  Field  fld;
  extern Element_List *Mesh;
  char   buf[BUFSIZ];

  memset(&fld, '\0', sizeof (Field));
  readHeader(f->in.fp,&fld,&shuff);
  rewind(f->in.fp);

  nfields = strlen(fld.type);
  nftot   = nfields;
  nftot  += (fld.dim == 3) ? 4:2;

  ReadParams  (f->rea.fp);
  if((i=iparam("NORDER.REQ"))!=UNSET){
    iparam_set("LQUAD",i+1);
    iparam_set("MQUAD",i+1);
  }
  else{
    iparam_set("LQUAD",fld.lmax+1);
    iparam_set("MQUAD",fld.lmax+1);
  }

  iparam_set("MODES",iparam("LQUAD")-1);

  /* Generate the list of elements */
  Mesh = ReadMesh(f->rea.fp, strtok(f->rea.name,"."));
  gmap = GlobalNumScheme(Mesh, (Bndry *)NULL);
  U[0] = LocalMesh(Mesh,strtok(f->rea.name,"."));
  U[0]->fhead->type = fld.type[0];

  init_ortho_basis();

  if(f->mesh.name) Get_Body(f->mesh.fp);

  for(i = 1; i < nfields; ++i){
    U[i] = U[0]->gen_aux_field('u');
    U[i]->fhead->type = fld.type[i];
  }

  for(i = nfields; i < nftot; ++i){
    U[i] = U[0]->gen_aux_field('u');
    U[i]->fhead->type = 'W';
  }

  freeField(&fld);

#ifdef PARALLEL
  // redefine output file to be processor specific

  if(f->out.fp == stdout){
    ROOTONLY
      fprintf(stderr,"Refining output file to iso_dump\n");
    sprintf(buf,"iso_dump_%d",pllinfo.procid);
    f->out.fp = fopen(buf,"w");
  }
  else{
    fclose(f->out.fp);
    sprintf(buf,"%s_%d",f->out.name,pllinfo.procid);
    f->out.fp = fopen(buf,"w");
  }
#endif

  nft[0] = nftot;
  return nfields;
}

static  void ReadCopyField (FileList *f, Element_List **U){
  int i,dump;
  int nfields;
  Field fld;

  memset(&fld, '\0', sizeof (Field));

  if(option("oldhybrid"))
    set_nfacet_list(U[0]);

  dump = readField (f->in.fp, &fld);
  if (!dump) error_msg(Restart: no dumps read from restart file);
  rewind (f->in.fp);

  nfields = strlen(fld.type);

  for(i = 0; i < nfields; ++i)
    copyfield(&fld,i,U[i]->fhead);

  freeField(&fld);
}

static void Isocontour(Element_List **E, FILE *out, int nfields){
  register int i,j,k,m,e;
  int      qa,cnt,sum,*interior,**tetT,**tetP;
  int      nnodesT,nnodesP,nelmtsT,nelmtsP,BooleanT,BooleanP;
  const int    nel = E[0]->nel;
  int      dim = E[0]->fhead->dim(),ntot;
  int      fieldid,norder;
  double   *z,*wm, ave, *w, *h;
  Coord    XT,XP;
  char     *outformat;
  Element  *F,*G,*W[3];
  Iso       **iso;


  fieldid = iparam("FIELDID");

  if(fieldid >= nfields){
    fprintf(stderr,"field id was greater than number of fields setting to "
      "nfields-1\n");
    fieldid = nfields-1;
  }

  for(i=0;i<nfields;++i)
    E[i]->Trans(E[i],J_to_Q);

  interior = ivector(0,E[0]->nel-1);
  iso      = (Iso **)malloc(E[0]->nel*sizeof(Iso *));

  if(E[0]->fhead->dim() == 3){
    double ***numT;
    double ***numP;
    int dim = 3;
    double entros = 0;

    ROOTONLY{
      fprintf(out,"TITLE = \"Field: %c ; Level: %lg\"\n",
        E[fieldid]->fhead->type,dparam("CONTOURVAL"));
      fprintf(out,"VARIABLES = \"x\" \"y\" \"z\"");
      fprintf(out," \"%c\"", E[fieldid]->fhead->type);
      fputc('\n',out);
    }

    /* dump data */

    BooleanP=1;
    BooleanT=1;
    norder = iparam("NORDER.REQ");
    norder = (norder == UNSET)? 0: norder+1;

    for(m=0; m<E[0]->nel; ++m){
      F=E[0]->flist[m];
      if(norder)
  qa = norder;
      else
  qa = F->qa;

      switch(F->identify()){
      case Nek_Prism:
         if (BooleanP==1){

    i = max(qa,QGmax);
          XP.x = dvector(0,i*i*i-1);
          XP.y = dvector(0,i*i*i-1);
          XP.z = dvector(0,i*i*i-1);

    h = dvector(0,qa*qa*qa-1);
    w = dvector(0,qa*qa*qa-1);

          sum=0;
          for (i=qa;i>0;i--){
            sum+=i;
          }

          nnodesP = qa*sum;
          nelmtsP = (((qa*qa)-(2*qa)+1)*(qa-1)*3);
          tetP = imatrix(0,nelmtsP-1,0,3);
          numP = dtarray(0,qa-1,0,qa-1,0,qa-1);

          for (cnt=0, k=0; k<qa; k++)
            for (j=0; j<qa; j++)
        for (i=0; i<qa-k; i++)
          numP[i][j][k] = ++cnt;

    for (cnt=0, k=0; k<qa-1; k++){
            for (j=0; j<qa-1; j++){
        for (i=0; i<qa-k-1; i++){
    tetP[cnt  ][0] = (int) numP[i]  [j]  [k]   ;
          tetP[cnt  ][1] = (int) numP[i+1][j]  [k]   ;
          tetP[cnt  ][2] = (int) numP[i]  [j]  [k+1] ;
          tetP[cnt++][3] = (int) numP[i]  [j+1][k]   ;

          tetP[cnt  ][0] = (int) numP[i+1][j]  [k]   ;
          tetP[cnt  ][1] = (int) numP[i]  [j]  [k+1] ;
          tetP[cnt  ][2] = (int) numP[i]  [j+1][k]   ;
          tetP[cnt++][3] = (int) numP[i+1][j+1][k]   ;

          tetP[cnt  ][0] = (int) numP[i]  [j]  [k+1] ;
          tetP[cnt  ][1] = (int) numP[i]  [j+1][k]   ;
          tetP[cnt  ][2] = (int) numP[i+1][j+1][k]   ;
          tetP[cnt++][3] = (int) numP[i]  [j+1][k+1] ;

          if (i<qa-k-2){
            tetP[cnt  ][0] = (int) numP[i+1][j]  [k]   ;
            tetP[cnt  ][1] = (int) numP[i+1][j]  [k+1] ;
            tetP[cnt  ][2] = (int) numP[i]  [j]  [k+1] ;
            tetP[cnt++][3] = (int) numP[i+1][j+1][k]   ;

            tetP[cnt  ][0] = (int) numP[i+1][j]  [k+1] ;
            tetP[cnt  ][1] = (int) numP[i]  [j]  [k+1] ;
            tetP[cnt  ][2] = (int) numP[i+1][j+1][k]   ;
            tetP[cnt++][3] = (int) numP[i]  [j+1][k+1] ;

            tetP[cnt ][0] = (int) numP[i+1][j] [k+1] ; tetP[cnt ][1] =
        (int) numP[i+1][j+1][k] ; tetP[cnt ][2] =
          (int) numP[i+1][j+1][k+1] ; tetP[cnt++][3] = (int) numP[i]
      [j+1][k+1] ;
    }
        }
      }
    }
    BooleanP=0;
  }

  if(Check_range(F)){
    F->coord(&XP);
    ntot = Interp_symmpts(F,qa,XP.x,XP.x,'p');
    ntot = Interp_symmpts(F,qa,XP.y,XP.y,'p');
    ntot = Interp_symmpts(F,qa,XP.z,XP.z,'p');
    G = E[fieldid]->flist[m];
    Interp_symmpts(G,G->qa,G->h_3d[0][0],h,'p');

          W[0]  = E[0]->flist[m];
          W[1]  = E[1]->flist[m];
          W[2]  = E[2]->flist[m];
    dvmul  (W[0]->qtot,W[0]->h_3d[0][0],1,W[0]->h_3d[0][0],1,
      W[0]->h_3d[0][0],1);
    dvvtvp (W[1]->qtot,W[1]->h_3d[0][0],1,W[1]->h_3d[0][0],1,
      W[0]->h_3d[0][0],1,W[0]->h_3d[0][0],1);
    dvvtvp (W[2]->qtot,W[2]->h_3d[0][0],1,W[2]->h_3d[0][0],1,
      W[0]->h_3d[0][0],1,W[0]->h_3d[0][0],1);

          Interp_symmpts(W[0],W[0]->qa,W[0]->h_3d[0][0],w,'p');

    entros += Dump_Contour(out,nnodesT,XT.x,XT.y,XT.z,h,
        w,nelmtsT,tetT,  dparam("CONTOURVAL"),(m+1));
  }
  break;
      case Nek_Tet:
  if (BooleanT==1){


    i = max(qa,QGmax);
          XT.x = dvector(0,i*i*i-1);
          XT.y = dvector(0,i*i*i-1);
          XT.z = dvector(0,i*i*i-1);

    h = dvector(0,qa*qa*qa-1);
    w = dvector(0,qa*qa*qa-1);

          nnodesT = ((qa*(qa+1)*(qa+2))/6);
          nelmtsT = (qa-1)*(qa-1)*(qa-1);
          tetT = imatrix(0,nelmtsT-1,0,3);
          numT = dtarray(0,qa-1,0,qa-1,0,qa-1);

          for(cnt=1, k=0; k<qa; ++k)
            for(j=0; j<qa-k; ++j)
        for(i=0; i<qa-k-j; ++i, ++cnt)
          numT[i][j][k] = cnt;

          for(k=0, cnt=0; k<qa-1; ++k){
            for(j=0; j<qa-1-k; ++j){
        for(i=0; i<qa-2-k-j; ++i){
          tetT[cnt  ][0] = (int) numT[i]  [j]  [k]   ;
          tetT[cnt  ][1] = (int) numT[i+1][j]  [k]   ;
          tetT[cnt  ][2] = (int) numT[i]  [j+1][k]   ;
          tetT[cnt++][3] = (int) numT[i]  [j]  [k+1] ;

          tetT[cnt  ][0] = (int) numT[i+1][j]  [k]   ;
          tetT[cnt  ][1] = (int) numT[i]  [j+1][k]   ;
          tetT[cnt  ][2] = (int) numT[i]  [j]  [k+1] ;
          tetT[cnt++][3] = (int) numT[i+1][j]  [k+1] ;

          tetT[cnt  ][0] = (int) numT[i+1][j]  [k+1] ;
          tetT[cnt  ][1] = (int) numT[i]  [j]  [k+1] ;
          tetT[cnt  ][2] = (int) numT[i]  [j+1][k+1] ;
          tetT[cnt++][3] = (int) numT[i]  [j+1][k]   ;

          tetT[cnt  ][0] = (int) numT[i+1][j+1][k]   ;
          tetT[cnt  ][1] = (int) numT[i]  [j+1][k]   ;
          tetT[cnt  ][2] = (int) numT[i+1][j]  [k]   ;
          tetT[cnt++][3] = (int) numT[i+1][j]  [k+1] ;

          tetT[cnt  ][0] = (int) numT[i+1][j+1][k]   ;
          tetT[cnt  ][1] = (int) numT[i]  [j+1][k]   ;
          tetT[cnt  ][2] = (int) numT[i+1][j]  [k+1] ;
          tetT[cnt++][3] = (int) numT[i]  [j+1][k+1] ;

          if(i < qa-3-k-j){
            tetT[cnt  ][0] = (int) numT[i]  [j+1][k+1] ;
            tetT[cnt  ][1] = (int) numT[i+1][j+1][k+1] ;
            tetT[cnt  ][2] = (int) numT[i+1][j]  [k+1] ;
            tetT[cnt++][3] = (int) numT[i+1][j+1][k]   ;
                 }
        }
        tetT[cnt  ][0] = (int) numT[qa-2-k-j][j]  [k]   ;
        tetT[cnt  ][1] = (int) numT[qa-1-k-j][j]  [k]   ;
        tetT[cnt  ][2] = (int) numT[qa-2-k-j][j+1][k]   ;
        tetT[cnt++][3] = (int) numT[qa-2-k-j][j]  [k+1] ;
      }
          }
    BooleanT=0;
        }

  if(Check_range(F)){
          F->coord(&XT);
          ntot = Interp_symmpts(F,qa,XT.x,XT.x,'p');
          ntot = Interp_symmpts(F,qa,XT.y,XT.y,'p');
          ntot = Interp_symmpts(F,qa,XT.z,XT.z,'p');
          G = E[fieldid]->flist[m];
          Interp_symmpts(G,G->qa,G->h_3d[0][0],h,'p');

    // calculate entrosphy here
    W[0]  = E[0]->flist[m];
    W[1]  = E[1]->flist[m];
    W[2]  = E[2]->flist[m];

    dvmul  (W[0]->qtot,W[0]->h_3d[0][0],1,W[0]->h_3d[0][0],1,
      W[0]->h_3d[0][0],1);
    dvvtvp (W[1]->qtot,W[1]->h_3d[0][0],1,W[1]->h_3d[0][0],1,
      W[0]->h_3d[0][0],1,W[0]->h_3d[0][0],1);
    dvvtvp (W[2]->qtot,W[2]->h_3d[0][0],1,W[2]->h_3d[0][0],1,
      W[0]->h_3d[0][0],1,W[0]->h_3d[0][0],1);

          Interp_symmpts(W[0],W[0]->qa,W[0]->h_3d[0][0],w,'p');


    entros += Dump_Contour(out,nnodesT,XT.x,XT.y,XT.z,h,w,nelmtsT,tetT,
         dparam("CONTOURVAL"),(m+1));
  }
        break;

      default:
        fprintf(stderr,"Isocontour is not set up for this element type \n");
        exit(1);
        break;
      }
    }

    fprintf(stderr,"Enstrophy contained in structure: %lf \n",entros);


    if (BooleanT==0){
      free_dtarray(numT,0,0,0);
      free_imatrix(tetT,0,0);
      free(XT.x); free(XT.y); free(XT.z);
    }
    if (BooleanP==0){
      free_dtarray(numP,0,0,0);
      free_imatrix(tetP,0,0);
      free(XP.x); free(XP.y); free(XP.z);
    }

  } else {
    fprintf(stderr,"isocon not set up in 2d\n");
  }
}



static void INTERPOLATE   (double xj, double yj, double zj, double vj,
                      double xk, double yk, double zk, double vk,
                      double cx[], double cy[], double cz[], double val, int counter, int *p);
static void ThreeSimilar  (int i, int j, int k, int *pr, int *ps);
static void TwoPairs      (double cx[], double cy[], double cz[], int *pr);


#define size 5
static double Dump_Contour(FILE *out, int nnodes, double *x, double *y,
       double *z, double *c, double *wm, int nelmts,
       int **tets,  double val, int zoneNum){
  double entros = 0.0,fac,loc_ent;
  double cx[size], cy[size], cz[size];
  int i, j, k, ii, jj, kk, r, s, n, counter, boolean, repeat;
  double xr,xs,xt, yr,ys,yt, zr,zs,zt,jac,N[4];
  int allfield = option("ALLFIELD");


  N[0] = 1/3.;
  N[1] = 1/3.;
  N[2] = 1/3.;
  N[3] = 1/3.;


  //fprintf(stdout,"Number of small elements =%d\n",nelmts);

  for (i=0;i<nelmts;i++) {
    // work out integral over small tet
    loc_ent = 0.0;

    xr = (x[tets[i][1]-1] - x[tets[i][0]-1])/2.0;
    xs = (x[tets[i][2]-1] - x[tets[i][0]-1])/2.0;
    yr = (y[tets[i][1]-1] - y[tets[i][0]-1])/2.0;
    ys = (y[tets[i][2]-1] - y[tets[i][0]-1])/2.0;
    zr = (z[tets[i][1]-1] - z[tets[i][0]-1])/2.0;
    zs = (z[tets[i][2]-1] - z[tets[i][0]-1])/2.0;
    xt = (x[tets[i][3]-1] - x[tets[i][0]-1])/2.0;
    yt = (y[tets[i][3]-1] - y[tets[i][0]-1])/2.0;
    zt = (z[tets[i][3]-1] - z[tets[i][0]-1])/2.0;

    //fprintf(stdout,"This is small element id = %d\n",i);
    //fprintf(stdout,"xr=%lf xs=%lf \n",xr,xs);
    //fprintf(stdout,"yr=%lf ys=%lf \n",yr,ys);
    //fprintf(stdout,"zr=%lf zs=%lf \n",zr,zs);
    //fprintf(stdout,"xt=%lf yt=%lf zt=%lf \n",xt,yt,zt);
    //fprintf(stdout,"x[tets[%d][0]]=%lf y[tets[%d][0]]=%lf z[tets[%d][0]]=%lf\n",i,x[tets[i][0]],i,y[tets[i][0]],i,z[tets[i][0]]);
    //fprintf(stdout,"x[tets[%d][1]]=%lf y[tets[%d][1]]=%lf z[tets[%d][1]]=%lf\n",i,x[tets[i][1]],i,y[tets[i][1]],i,z[tets[i][1]]);
    //fprintf(stdout,"x[tets[%d][2]]=%lf y[tets[%d][2]]=%lf z[tets[%d][2]]=%lf\n",i,x[tets[i][2]],i,y[tets[i][2]],i,z[tets[i][2]]);
    //fprintf(stdout,"x[tets[%d][3]]=%lf y[tets[%d][3]]=%lf z[tets[%d][3]]=%lf\n",i,x[tets[i][3]],i,y[tets[i][3]],i,z[tets[i][3]]);


    jac = fabs(xr*(ys*zt-zs*yt) - yr*(xs*zt-zs*xt) + zr*(xs*yt-ys*xt));

    for(j = 0; j < 4; ++j){
        loc_ent += wm[tets[i][j]-1]*jac*N[j];
  //loc_ent +=jac*N[j];
  //fprintf(stdout,"volumeloc_ent=%lf\n",loc_ent);
    }
    //fprintf(stdout,"Finished loop\n");
    //fprintf(stdout,"Jacobian=%lf\n",jac);


    // multiply by factor;
    if(allfield)
      fac = 1;
    else{
      fac = 0.0;
      for(j = 0; j < 4; ++j)
  if(c[tets[i][j]-1] <= val) fac+=1.0;

      fac /= 4.0;
    }

    entros += fac*loc_ent;

    //fprintf(stdout,"fac=%lf\n",fac);
    //fprintf(stdout,"entros=%lf\n",entros);

  }

  for (repeat=1;repeat<=2;repeat++){
    n=0;
    for (i=0;i<nelmts;i++) {
      if  (!(((c[tets[i][0]-1]>val)&&(c[tets[i][1]-1]>val)&&
              (c[tets[i][2]-1]>val)&&(c[tets[i][3]-1]>val))||
             ((c[tets[i][0]-1]<val)&&(c[tets[i][1]-1]<val)&&
              (c[tets[i][2]-1]<val)&&(c[tets[i][3]-1]<val)))) {
        counter=0;

        for (j=0; j<=2; j++){
          for (k=j+1; k<=3; k++){
            if (((c[tets[i][j]-1]>=val)&&(val>=c[tets[i][k]-1]))||
                ((c[tets[i][j]-1]<=val)&&(val<=c[tets[i][k]-1]))){

              INTERPOLATE(x[tets[i][j]-1],y[tets[i][j]-1],
                          z[tets[i][j]-1],c[tets[i][j]-1],
                          x[tets[i][k]-1],y[tets[i][k]-1],
                          z[tets[i][k]-1],c[tets[i][k]-1],
                          cx, cy, cz, val, counter, &counter);                                 }
          }
        }

       if (counter==3){
          n+=1;
          if (repeat==2){
            fprintf(out,"%lg %lg %lg %lg\n%lg %lg %lg %lg\n"
        "%lg %lg %lg %lg\n", cx[0],cy[0],cz[0],
        val,cx[1],cy[1],cz[1],val,cx[2],cy[2],cz[2],val);
          }
        }
  else if (counter==4){
          n+=2;
          if (repeat==2){
              fprintf(out,"%lg %lg %lg %lg\n%lg %lg %lg %lg\n"
          "%lg %lg %lg %lg\n", cx[0],cy[0],cz[0],
          val,cx[1],cy[1],cz[1], val,cx[2],cy[2],cz[2],val);
              fprintf(out,"%lg %lg %lg %lg\n"
          "%lg %lg %lg %lg\n%lg %lg %lg %lg\n",
                      cx[1],cy[1],cz[1],val,cx[2],cy[2],cz[2],
          val,cx[3],cy[3],cz[3],val);
      }
        }
  else if (counter==5){
          n+=1;
          if (repeat==2){
            boolean=0;
            for (ii=0;ii<=2;ii++){
              for (jj=ii+1;jj<=3;jj++){
                for (kk=jj+1;kk<=4;kk++){
                  if  ((((cx[ii]-cx[jj])==0.0)&&((cy[ii]-cy[jj])==0.0)
      &&((cz[ii]-cz[jj])==0.0))&&
           (((cx[ii]-cx[kk])==0.0)&&((cy[ii]-cy[kk])==0.0)
      &&((cz[ii]-cz[kk])==0.0))){
                    boolean+=1;
                    ThreeSimilar (ii,jj,kk,&r,&s);
                    fprintf(out,"%lg %lg %lg %lg\n%lg %lg %lg %lg\n"
          "%lg %lg %lg %lg\n", cx[ii],cy[ii],cz[ii],val,
          cx[r],cy[r],cz[r],val,cx[s],cy[s],cz[s],val);
                  } else {
                    boolean+=0;
                  }
                }
              }
            }
            if (boolean==0){
              TwoPairs (cx,cy,cz,&r);
              fprintf(out,"%lg %lg %lg %lg\n%lg %lg %lg %lg\n"
          "%lg %lg %lg %lg\n", cx[0],cy[0],cz[0],val,
          cx[2],cy[2],cz[2],val,cx[r],cy[r],cz[r],val);
            }
          }
        }
  else if (counter!=0){
          if (repeat==2){
      fprintf(out,"Zone no. %d, element no. %d - ",zoneNum,i);
            fprintf(out,"No. of iso-contour points for this element is %d. "
        "Expected 0, 3, 4 or 5 only.\n",counter);
          }
        }
      }
    }

    if (repeat==2){
      for (i=1;i<=n;i++){
        fprintf(out," %d %d %d\n",(3*i-2),(3*i-1),(3*i));
      }
    } else if ((repeat==1)&&(n!=0)){
      fprintf(out,"\nZONE T=\"ZONE %d (%lg)\"\n",zoneNum,val);
      fprintf(out,"N=%d, E=%d, F=FEPOINT, ET=TRIANGLE\n", (3*n), n);
    }
  }
  return entros;
}




/************************/
/* Function INTERPOLATE */
/************************/

static void INTERPOLATE (double xj, double yj, double zj, double cj,
                  double xk, double yk, double zk, double ck,
                  double cx[], double cy[], double cz[], double val,
       int counter, int *p){
  double factor;

  /*  This IF statement removes the possibility of division by divider=0.
      "k" acts as a counter to keep track of the number of interpolations
      performed so that the apprpriate number of triangles (1 for triangle,
      2 for square)   may be dumped out in the calling function. */

  if ((fabs(ck-cj))>0.0){
    factor=(val-cj)/(ck-cj);
    cx[counter]=xj+(factor*(xk-xj));
    cy[counter]=yj+(factor*(yk-yj));
    cz[counter]=zj+(factor*(zk-zj));
    *p=++counter;
  }
}

/*************************/
/* Function ThreeSimilar */
/*************************/

static void ThreeSimilar (int i, int j, int k, int *pr, int *ps)

/*  The logic of the following SWITCH statement may only be understood with a "peusdo truth-table" supplied by
    the author. */

{
    switch (i+j+k){
        case (3) :           *pr=3; *ps=4;  break;
        case (4) :           *pr=2; *ps=4;  break;
        case (5) : if (j==1){*pr=2; *ps=3;}
                   else     {*pr=1; *ps=4;} break;
        case (6) : if (i==0){*pr=1; *ps=3;}
                   else     {*pr=0; *ps=4;} break;
        case (7) : if (i==0){*pr=1; *ps=2;}
                   else     {*pr=0; *ps=3;} break;
        case (8) :           *pr=0; *ps=2;  break;
        case (9) :           *pr=0; *ps=1;  break;
        default  : printf ("Error in 5-point triangulation in ThreeSimilar") ; break;
    }
}


/*********************/
/* Function TwoPairs */
/*********************/

static void TwoPairs (double cx[], double cy[], double cz[], int *pr)

/*  The logic of the following SWITCH statement may only be understood with a "peusdo truth-table" supplied by
    the author. */

{
    if (((cx[0]-cx[1])==0.0)&&((cy[0]-cy[1])==0.0)&&((cz[0]-cz[1])==0.0)){
        if (((cx[2]-cx[3])==0.0)&&((cy[2]-cy[3])==0.0)&&((cz[2]-cz[3])==0.0)){
            *pr=4;
        } else {
            *pr=3;
        }
    } else {
        *pr=1;
    }
}


#ifdef EXCLUDE
static int Check_range(Element *E){
  if(rnge){
    register int i;

    for(i = 0; i < Nvert; ++i){
      if((E->vert[i].x < rnge->x[0])||(E->vert[i].x > rnge->x[1])) return 0;
      if((E->vert[i].y < rnge->y[0])||(E->vert[i].y > rnge->y[1])) return 0;
      if(DIM == 3)
  if((E->vert[i].z < rnge->z[0])||(E->vert[i].z > rnge->z[1])) return 0;
    }
  }
  return 1;
}
#else
static int Check_range(Element *E){
  if(rnge){
    register int i;

    for(i = 0; i < E->Nverts; ++i){
      if(DIM == 3){
  if((E->vert[i].x > rnge->x[0])&&(E->vert[i].x < rnge->x[1])
     && (E->vert[i].y > rnge->y[0])&&(E->vert[i].y < rnge->y[1])
     && (E->vert[i].z > rnge->z[0])&&(E->vert[i].z < rnge->z[1])) return 1;
      }
      else
  if((E->vert[i].x > rnge->x[0])&&(E->vert[i].x < rnge->x[1])
     && (E->vert[i].y > rnge->y[0])&&(E->vert[i].y < rnge->y[1])) return 1;
    }
    return 0;
  }
  else
    return 1;
}
#endif

/* --------------------------------------------------------------------- *
 * parse_args() -- Parse application arguments                           *
 *                                                                       *
 * This program only supports the generic utility arguments.             *
 * --------------------------------------------------------------------- */

static void parse_util_args (int argc, char *argv[], FileList *f)
{
  char  c;
  int   i;
  char  fname[FILENAME_MAX];

  if (argc == 0) {
    fputs (usage, stderr);
    exit  (1);
  }

  iparam_set("Nout", UNSET);

  while (--argc && (*++argv)[0] == '-') {
    while (c = *++argv[0])                  /* more to parse... */
      switch (c) {
      case 'a':
  option_set("ALLFIELD",1);
  break;
      case 'l':
  if (*++argv[0])
    dparam_set("CONTOURVAL", atof(*argv));
  else {
    dparam_set("CONTOURVAL", atof(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;
      case 'f':
  if (*++argv[0])
    iparam_set("FIELDID", atoi(*argv));
  else {
    iparam_set("FIELDID", atoi(*++argv));
    argc--;
  }
  break;
      default:
  fprintf(stderr, "%s: unknown option -- %c\n", prog, c);
  break;
      }
  }

  /* open input file */

  if ((*argv)[0] == '-') {
    f->in.fp = stdin;
  } else {
    strcpy (fname, *argv);
    if ((f->in.fp = fopen(fname, "r")) == (FILE*) NULL) {
      sprintf(fname, "%s.fld", *argv);
      if ((f->in.fp = fopen(fname, "r")) == (FILE*) NULL) {
  fprintf(stderr, "%s: unable to open the input file -- %s or %s\n",
    prog, *argv, fname);
  exit(1);
      }
    }
    f->in.name = strdup(fname);
  }

  if (option("verbose")) {
    fprintf (stderr, "%s: in = %s, rea = %s, out = %s\n", prog,
       f->in.name   ? f->in.name   : "<stdin>",  f->rea.name,
       f->out.name  ? f->out.name  : "<stdout>");
  }

  return;
}

static void Get_Body(FILE *fp){
  register int i;
  char buf[BUFSIZ],*s;
  int  N;

  if(option("Range")){
    rnge = (Range *)malloc(sizeof(Range));
    rewind(fp);  /* search for range data */
    while(s && !strstr((s=fgets(buf,BUFSIZ,fp)),"Range"));

    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf",rnge->x,rnge->x+1);
    fgets(buf,BUFSIZ,fp);
    sscanf(buf,"%lf%lf",rnge->y,rnge->y+1);
    if(DIM == 3){
      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%lf%lf",rnge->z,rnge->z+1);
    }
  }

  if(option("Body")){
    rewind(fp);/* search for body data  */
    while(s && !strstr((s=fgets(buf,BUFSIZ,fp)),"Body"));

    if(s!=NULL){

      fgets(buf,BUFSIZ,fp);
      sscanf(buf,"%d",&N);

      bdy.N = N;
      bdy.elmt   = ivector(0,N-1);
      bdy.faceid = ivector(0,N-1);

      for(i = 0; i < N; ++i){
  fgets(buf,BUFSIZ,fp);
  sscanf(buf,"%d%d",bdy.elmt+i,bdy.faceid+i);
  --bdy.elmt[i];
  --bdy.faceid[i];
      }
    }
  }
}

static void dump_faces(FILE *out, Element_List **U, Coord X, int nel,
           int zone, int nfields){
  Element *F;
  int qa, qb, qc,fac, cnt;
  register int i,j,k,l,n;
  double **tmp;
  int data_skip = 0;
  int size_skip = 0;

  tmp  = dmatrix(0, nfields, 0, QGmax*QGmax*QGmax-1);

  k = 0;
  for(l=0;l<bdy.N;++l){
    F = U[0]->flist[bdy.elmt[l]];

    fac = bdy.faceid[l];

    if(F->identify() == Nek_Tet)
      switch(fac){
      case 0:
  qa = F->qa;      qb = F->qb;
  break;
      case 1:
  qa = F->qa;      qb = F->qc;
  break;
      case 2: case 3:
  qa = F->qb;      qb = F->qc;
  break;
      }
    else if(F->identify() == Nek_Hex){
      switch(fac){
      case 0: case 5:
  qa = F->qa;      qb = F->qb;
  break;
      case 1: case 3:
  qa = F->qa;      qb = F->qc;
  break;
      case 2: case 4:
  qa = F->qb;      qb = F->qc;
  break;
      }
    }
    else if(F->identify() == Nek_Prism){
      switch(fac){
      case 0:
  qa = F->qa;      qb = F->qb;
  break;
      case 1: case 3:
  qa = F->qa;      qb = F->qc;
  break;
      case 2: case 4:
  qa = F->qb;      qb = F->qc;
  break;
      }
    }

    cnt = F->qa*F->qb;

    F->GetFaceCoord(fac, &X);

    fprintf(out,"ZONE T=\"ELEMENT %d\", I=%d, J=%d, K=%d, F=POINT\n",
      ++zone,qa,qb,1);

    for(n = 0; n < nfields; ++n)
      U[n]->flist[F->id]->GetFace(**U[n]->flist[F->id]->h_3d, fac, tmp[n]);

    for(i = 0; i < cnt; ++i){
      fprintf(out,"%lg %lg %lg ",X.x[i], X.y[i], X.z[i]);
      for(n = 0; n < nfields; ++n)
  fprintf(out,"%lg ",tmp[n][i]);
      fputc('\n',out);
    }
    ++k;
  }
  free_dmatrix(tmp,0,0);
}


void  set_elmt_edges(Element_List *EL){
  register int i,j,k,c;
  Edge    *e;
  int     nel = EL->nel,edgtot;
  double  tot,*wa,*wb,*wk,w;
  Element *E;

  wk = dvector(0,QGmax-1);


  for(k = 0; k < nel; ++k)   /* set up number of quadrature points */
    for(i = 0,e=EL->flist[k]->edge; i < EL->flist[k]->Nedges; ++i)
      e[i].qedg = EL->flist[e[i].eid]->lmax;

  edgtot = 0;
  for(k = 0; k < nel; ++k){
    E = EL->flist[k];
    getzw(E->qa,&wa,&wa,'a'); // ok

    if(E->identify() == Nek_Tri)
      getzw(E->qb,&wb,&wb,'b'); // ok
    else
      getzw(E->qb,&wb,&wb,'a'); // ok

    for(i = 0,e=E->edge; i < E->Nedges; ++i){

      /* set up weights for based on edge area */
      if(E->curvX){
  for(j = 0; j < E->qb; ++j)
    wk[j] = ddot(E->qa,wa,1,E->geom->jac.p+j*E->qa,1);
  e[i].weight = ddot(E->qb,wb,1,wk,1);
      }
      else{
  e[i].weight = E->geom->jac.d;
      }

      /* loop through and reset to minimum */
      if(e[i].base){
  /* see if there is an adjacent edge */
  if(e[i].link)
    e[i].qedg = max(e[i].qedg,e[i].link->qedg);
  else
    e[i].qedg = max(e[i].qedg,e[i].base->qedg);
  edgtot += e[i].qedg;
      }
      else{
  e[i].link = (Edge *)calloc(1,sizeof(Edge));
  //e[i].link[0] = e[i];
  memcpy(e[i].link,e+i,sizeof(Edge));
  e[i].link->base = e+i;
  edgtot += 2*e[i].qedg;
      }
    }
  }

  /* declare memory for edges at once to keep it together */
  EL->flist[0]->edge->h = dvector(0,edgtot-1);
  dzero(edgtot,EL->flist[0]->edge->h,1);
  c = 0;
  for(k = 0; k < nel; ++k) {
    E = EL->flist[k];
    for(i = 0,e=E->edge; i < E->Nedges; ++i){
      e[i].h = EL->flist[0]->edge->h + c;
      c += e[i].qedg;
      if(!e[i].base){
  e[i].link->h = EL->flist[0]->edge->h + c;
  c += e[i].qedg;
      }
    }
  }

  // viscous weights
  /* loop through edges and calculate edge weights as ratio of the local
     area of adjacent elements - otherwise set it to be 1 */
  for(E=EL->fhead;E;E = E->next){
    for(i = 0; i < E->Nedges; ++i)
      if(E->edge[i].base){
  if(E->edge[i].link){
    tot = E->edge[i].weight + E->edge[i].link->weight;

    w = E->edge[i].weight/tot;
    E->edge[i].weight =  E->edge[i].link->weight/tot;
    E->edge[i].link->weight = w;
  }
      }
      else{
  E->edge[i].weight = 1;
  E->edge[i].link->weight = 0;
      }
  }
  free(wk);
}

/* set up outward facing normals along faces as well as the edge
   jacobeans divided by the jacobean for the triangle. All points
   are evaluated at the  gauss quadrature points */

void set_edge_geofac(Element_List *EL){
  Element *E;
  int i,j,k;

  for(E=EL->fhead;E;E = E->next)
    E->set_edge_geofac();

}


static void Write_Contour(FILE *out, char type, int nel,
        Iso **iso, double val){
  register int i,j;
  int cnt;

  for(cnt = 0,i = 0; i < nel; ++i)
    cnt += iso[i]->n;

  fprintf(out,"ZONE T=\"Countour (%lg)\"\n",val);
  fprintf(out,"N=%d, E=%d, F=FEPOINT, ET=TRIANGLE\n", (3*cnt), cnt);

  for(i = 0; i < nel; ++i)
    for(j = 0; j < iso[i]->n; ++j)
      fprintf(out,"%lg %lg %lg %lg\n%lg %lg %lg %lg\n"
        "%lg %lg %lg %lg\n",iso[i]->x[3*j],iso[i]->y[3*j],
        iso[i]->z[3*j],val,iso[i]->x[3*j+1],iso[i]->y[3*j+1],
        iso[i]->z[3*j+1],val,iso[i]->x[3*j+2],iso[i]->y[3*j+2],
        iso[i]->z[3*j+2],val);

  for (i=0; i < cnt; ++i)
    fprintf(out," %d %d %d\n",3*i+1,3*i+2,3*i+3);

}
