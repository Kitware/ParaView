/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/isocon.C,v $
 * $Revision: 1.12 $
 * $Date: 2006/08/15 19:31:06 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <list>
#include <string>
#include <algorithm>

#include <veclib.h>
#include <nektar.h>
#include <gen_utils.h>

#include "isoutils.h"

using namespace std;

int DIM;

/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "isocont";
char *usage  = "isocont:  [options]  -r file[.rea]  input[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "-l #    ... Value of contour to be extracted (defaut 0) \n"
  "-g      ... global condense finite element output\n"
  "-e      ... dump each individual element\n"
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


static int  setup (FileList *f, Element_List **U, int *nftot);
static void ReadCopyField (FileList *f, Element_List **U);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void Isocontour(Element_List **E, std::ostream &out, int nfields);
static Iso *Extract_Contour(int nnodes, double *x, double *y,
       double *z, double *c, int nelmts, int **tets,
          double val, int zoneNum);
static int Check_range(Element *E);

main (int argc, char *argv[]){
  register int  i,k;
  int       dump=0,nfields=0,nftot;
  FileList  f;
  Element_List **master;

  init_comm(&argc, &argv);

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  master  = (Element_List **) malloc(_MAX_FIELDS*sizeof(Element_List *));
  nfields = setup (&f, master, &nftot);

  ReadCopyField(&f,master);

  DIM = master[0]->fhead->dim();

  if(f.out.name){
    std::ofstream out (f.out.name);
    Isocontour(master,out, nfields);
  }
  else
    Isocontour(master,cout, nfields);


  exit_comm();

  return 0;
}

/* Inner project w.r.t. orthogonal modes */

Gmap *gmap;

int  setup (FileList *f, Element_List **U, int *nft){
  int    i,shuff;
  int    nfields,nftot;
  Field  fld;
  extern Element_List *Mesh;

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
  else if(option("Qpts")){
    iparam_set("LQUAD",fld.lmax+1);
    iparam_set("MQUAD",fld.lmax+1);
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
  char buf[256];
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

static void GetTetNum(int qa, int& nnodesT, int& nelmtsT, int **&tetT);

static void GetPrismNum(int qa, int& nnodesP, int& nelmtsP, int **&tetP);
static void GetPrismNumBwd(int qa, int& nnodesP, int& nelmtsP, int **&tetP);

static void Isocontour(Element_List **E, std::ostream &out, int nfields){
  register   int i,j,k,m,e;
  int        qa,cnt,sum,*interior,**tet;
  int        nnodes,nelmts;
  const int  nel = E[0]->nel;
  int        dim = E[0]->fhead->dim(),ntot;
  int        fieldid;
  int       Maxqa;
  double    *z,*w, ave;
  Coord      X;
  char       *outformat;
  Element    *F,*G;
  Iso        **iso;

  fieldid = iparam("FIELDID");

  if(fieldid >= nfields){
    cerr << "field id was greater than number of fields setting to "
      "nfields-1\n";
    fieldid = nfields-1;
  }

  for(i=0;i<nfields;++i)
    E[i]->Trans(E[i],J_to_Q);

  interior = ivector(0,E[0]->nel-1);
  iso      = (Iso **)malloc(E[0]->nel*sizeof(Iso *));

  if(E[0]->fhead->dim() == 3){

    ROOTONLY{
      out << "TITLE = \"Field: "<<E[fieldid]->fhead->type<< " ; Level: "
    << dparam("CONTOURVAL")<<"\"\n";
      out << "VARIABLES = \"x\" \"y\" \"z\"";
      out << " \"" << E[fieldid]->fhead->type <<"\"" << endl;
    }

    /* Extraxt  data */

    Maxqa = -1;

    for(m=0; m<E[0]->nel; ++m){
      F = E[0]->flist[m];
      qa = F->qa;

      // set up coordinate storage space
      if(qa > Maxqa){
  if(m){
    free(X.x); free(X.y); free(X.z);
  }
  Maxqa = qa;
  X.x = dvector(0,Maxqa*Maxqa*Maxqa-1);
  X.y = dvector(0,Maxqa*Maxqa*Maxqa-1);
  X.z = dvector(0,Maxqa*Maxqa*Maxqa-1);
      }

      // set up local tet numbering for interpolated points
      switch(F->identify()){
      case Nek_Prism:
    if(F->edge->con)
        GetPrismNumBwd(qa,nnodes,nelmts,tet);
    else
        GetPrismNum(qa,nnodes,nelmts,tet);
  break;
      case Nek_Tet:
  GetTetNum(qa,nnodes,nelmts,tet);
  break;
      default:
        fprintf(stderr,"Isocontour is not set up for this element type \n");
        exit(1);
        break;
      }

      // extract isocontour and dump or store
      if(Check_range(F)){
  F->coord(&X);
  ntot = Interp_symmpts(F,qa,X.x,X.x,'p');
  ntot = Interp_symmpts(F,qa,X.y,X.y,'p');
  ntot = Interp_symmpts(F,qa,X.z,X.z,'p');

  G = E[fieldid]->flist[m];
  Interp_symmpts(G,G->qa,G->h_3d[0][0],G->h_3d[0][0],'p');

  iso[m] = Extract_Contour(nnodes,X.x,X.y,X.z,G->h_3d[0][0],
         nelmts,tet,dparam("CONTOURVAL"),(m+1));
  iso[m]->condense();
  if(!option("GlobalCondense"))
    iso[m]->write(out,dparam("CONTOURVAL"),(int) dparam("iso_mincells"));
      }
    }

    // dump output if data was concatinated
    if(option("GlobalCondense")){
      Iso Giso;
      Giso.globalcondense(E[0]->nel,iso,E[0]);
      Giso.separate_regions();
      Giso.write(out,dparam("CONTOURVAL"),(int) dparam("iso_mincells"));
    }

    free(X.x); free(X.y); free(X.z);
  }
  else {
    cerr << "isocon not set up in 2d\n";
  }
}


static void GetPrismNum(int qa, int& nnodesP, int& nelmtsP, int **&tetP){
  register int i,j,k,cnt;
  double      ***numP;
  static int  ***StoretetP;
  static int  init = 1;

  if(init){
    StoretetP   = (int ***)calloc(2*QGmax,sizeof(int **));
    StoretetP  -= 1;
    init = 0;
  }

  nnodesP = qa*qa*(qa+1)/2;
  nelmtsP = (((qa*qa)-(2*qa)+1)*(qa-1)*3);

  if(StoretetP[qa]){
    tetP  = StoretetP[qa];
    return;
  }
  else{

    StoretetP[qa] = imatrix(0,nelmtsP-1,0,3);
    numP          = dtarray(0,qa-1,0,qa-1,0,qa-1);


    for (cnt=0, k=0; k<qa; k++)
      for (j=0; j<qa; j++)
  for (i=0; i<qa-k; i++)
    numP[i][j][k] = cnt++;


    for (cnt=0, k=0; k<qa-1; k++)
    {
  for (j=0; j<qa-1; j++)
  {
      for (i=0; i<qa-k-1; i++)
      {
    StoretetP[qa][cnt  ][0] = (int) numP[i]   [j]  [k]   ;
    StoretetP[qa][cnt  ][1] = (int) numP[i+1] [j]  [k]   ;
    StoretetP[qa][cnt  ][2] = (int) numP[i]   [j]  [k+1] ;
    StoretetP[qa][cnt++][3] = (int) numP[i+1] [j+1][k]   ;

    StoretetP[qa][cnt  ][0] = (int) numP[i]  [j]  [k]   ;
    StoretetP[qa][cnt  ][1] = (int) numP[i]  [j]  [k+1] ;
    StoretetP[qa][cnt  ][2] = (int) numP[i]  [j+1][k]   ;
    StoretetP[qa][cnt++][3] = (int) numP[i+1][j+1][k]   ;

    StoretetP[qa][cnt  ][0] = (int) numP[i]  [j]  [k+1] ;
    StoretetP[qa][cnt  ][1] = (int) numP[i]  [j+1][k]   ;
    StoretetP[qa][cnt  ][2] = (int) numP[i+1][j+1][k]   ;
    StoretetP[qa][cnt++][3] = (int) numP[i]  [j+1][k+1] ;

    if (i<qa-k-2)
    {
        StoretetP[qa][cnt  ][0] = (int) numP[i+1][j]  [k]   ;
        StoretetP[qa][cnt  ][1] = (int) numP[i+1][j]  [k+1] ;
        StoretetP[qa][cnt  ][2] = (int) numP[i]  [j]  [k+1] ;
        StoretetP[qa][cnt++][3] = (int) numP[i+1][j+1][k]   ;

        StoretetP[qa][cnt  ][0] = (int) numP[i+1][j]  [k+1] ;
        StoretetP[qa][cnt  ][1] = (int) numP[i]  [j]  [k+1] ;
        StoretetP[qa][cnt  ][2] = (int) numP[i+1][j+1][k]   ;
        StoretetP[qa][cnt++][3] = (int) numP[i+1]  [j+1][k+1] ;
        StoretetP[qa][cnt  ][0] = (int) numP[i][j] [k+1] ;
        StoretetP[qa][cnt  ][1] = (int) numP[i+1][j+1][k] ;
        StoretetP[qa][cnt  ][2] = (int) numP[i+1][j+1][k+1] ;
        StoretetP[qa][cnt++][3] = (int) numP[i][j+1][k+1] ;
    }
      }
  }
    }

    tetP  = StoretetP [qa];
    free_dtarray( numP,0,0,0);
  }
}


static void GetPrismNumBwd(int qa, int& nnodesP, int& nelmtsP, int **&tetP){
  register int i,j,k,cnt;
  double      ***numP;
  static int  ***StoretetP;
  static int  init = 1;

  if(init){
    StoretetP   = (int ***)calloc(2*QGmax,sizeof(int **));
    StoretetP  -= 1;
    init = 0;
  }

  nnodesP = qa*qa*(qa+1)/2;
  nelmtsP = (((qa*qa)-(2*qa)+1)*(qa-1)*3);

  if(StoretetP[qa]){
    tetP  = StoretetP[qa];
    return;
  }
  else{

    StoretetP[qa] = imatrix(0,nelmtsP-1,0,3);
    numP          = dtarray(0,qa-1,0,qa-1,0,qa-1);


    for (cnt=0, k=0; k<qa; k++)
      for (j=0; j<qa; j++)
  for (i=0; i<qa-k; i++)
    numP[i][j][k] = cnt++;


    for (cnt=0, k=0; k<qa-1; k++)
    {
  for (j=0; j<qa-1; j++)
  {
      for (i=0; i<qa-k-1; i++)
      {
    StoretetP[qa][cnt  ][0] = (int) numP[i]  [j]  [k]   ;
    StoretetP[qa][cnt  ][1] = (int) numP[i+1][j]  [k]   ;
    StoretetP[qa][cnt  ][2] = (int) numP[i]  [j]  [k+1] ;
    StoretetP[qa][cnt++][3] = (int) numP[i+1][j+1][k]   ;

    StoretetP[qa][cnt  ][0] = (int) numP[i]  [j]  [k]   ;
    StoretetP[qa][cnt  ][1] = (int) numP[i]  [j]  [k+1] ;
    StoretetP[qa][cnt  ][2] = (int) numP[i]  [j+1][k]   ;
    StoretetP[qa][cnt++][3] = (int) numP[i+1][j+1][k]   ;

    StoretetP[qa][cnt  ][0] = (int) numP[i]  [j]  [k+1] ;
    StoretetP[qa][cnt  ][1] = (int) numP[i]  [j+1][k]   ;
    StoretetP[qa][cnt  ][2] = (int) numP[i+1][j+1][k]   ;
    StoretetP[qa][cnt++][3] = (int) numP[i]  [j+1][k+1] ;

    if (i<qa-k-2)
    {
        StoretetP[qa][cnt  ][0] = (int) numP[i+1][j]  [k]   ;
        StoretetP[qa][cnt  ][1] = (int) numP[i+1][j]  [k+1] ;
        StoretetP[qa][cnt  ][2] = (int) numP[i]  [j]  [k+1] ;
        StoretetP[qa][cnt++][3] = (int) numP[i+1][j+1][k]   ;

        StoretetP[qa][cnt  ][0] = (int) numP[i+1][j]  [k+1] ;
        StoretetP[qa][cnt  ][1] = (int) numP[i]  [j]  [k+1] ;
        StoretetP[qa][cnt  ][2] = (int) numP[i+1][j+1][k]   ;
        StoretetP[qa][cnt++][3] = (int) numP[i+1] [j+1][k+1] ;
        StoretetP[qa][cnt  ][0] = (int) numP[i][j] [k+1] ;
        StoretetP[qa][cnt  ][1] = (int) numP[i+1][j+1][k] ;
        StoretetP[qa][cnt  ][2] = (int) numP[i+1][j+1][k+1] ;
        StoretetP[qa][cnt++][3] = (int) numP[i][j+1][k+1] ;
    }
      }
  }
    }

    tetP  = StoretetP [qa];
    free_dtarray( numP,0,0,0);
  }
}


static void GetTetNum(int qa, int& nnodesT, int& nelmtsT, int **&tetT){
  register int i,j,k,cnt;
  double      ***numT;
  static int  ***StoretetT;
  static int init = 1;

  if(init){
    StoretetT   = (int ***)calloc(2*QGmax,sizeof(int **));
    StoretetT  -= 1;
    init = 0;
  }

  nnodesT = ((qa*(qa+1)*(qa+2))/6);
  nelmtsT = (qa-1)*(qa-1)*(qa-1);

  if(StoretetT[qa]){
     tetT  = StoretetT[qa];
    return;
  }
  else{

    StoretetT[qa]  = imatrix(0,nelmtsT-1,0,3);
    numT           = dtarray(0,qa-1,0,qa-1,0,qa-1);

    for(cnt=0, k=0; k<qa; ++k)
      for(j=0; j<qa-k; ++j)
  for(i=0; i<qa-k-j; ++i, ++cnt)
    numT[i][j][k] = cnt;

    for(k=0, cnt=0; k<qa-1; ++k){
      for(j=0; j<qa-1-k; ++j){
  for(i=0; i<qa-2-k-j; ++i){

    StoretetT[qa][cnt  ][0] = (int) numT[i]  [j]  [k]   ;
    StoretetT[qa][cnt  ][1] = (int) numT[i+1][j]  [k]   ;
    StoretetT[qa][cnt  ][2] = (int) numT[i]  [j+1][k]   ;
    StoretetT[qa][cnt++][3] = (int) numT[i]  [j]  [k+1] ;

    StoretetT[qa][cnt  ][0] = (int) numT[i+1][j]  [k]   ;
    StoretetT[qa][cnt  ][1] = (int) numT[i]  [j+1][k]   ;
    StoretetT[qa][cnt  ][2] = (int) numT[i]  [j]  [k+1] ;
    StoretetT[qa][cnt++][3] = (int) numT[i+1][j]  [k+1] ;

    StoretetT[qa][cnt  ][0] = (int) numT[i+1][j]  [k+1] ;
    StoretetT[qa][cnt  ][1] = (int) numT[i]  [j]  [k+1] ;
    StoretetT[qa][cnt  ][2] = (int) numT[i]  [j+1][k+1] ;
    StoretetT[qa][cnt++][3] = (int) numT[i]  [j+1][k]   ;

    StoretetT[qa][cnt  ][0] = (int) numT[i+1][j+1][k]   ;
    StoretetT[qa][cnt  ][1] = (int) numT[i]  [j+1][k]   ;
    StoretetT[qa][cnt  ][2] = (int) numT[i+1][j]  [k]   ;
    StoretetT[qa][cnt++][3] = (int) numT[i+1][j]  [k+1] ;

    StoretetT[qa][cnt  ][0] = (int) numT[i+1][j+1][k]   ;
    StoretetT[qa][cnt  ][1] = (int) numT[i]  [j+1][k]   ;
    StoretetT[qa][cnt  ][2] = (int) numT[i+1][j]  [k+1] ;
    StoretetT[qa][cnt++][3] = (int) numT[i]  [j+1][k+1] ;

    if(i < qa-3-k-j){
      StoretetT[qa][cnt  ][0] = (int) numT[i]  [j+1][k+1] ;
      StoretetT[qa][cnt  ][1] = (int) numT[i+1][j+1][k+1] ;
      StoretetT[qa][cnt  ][2] = (int) numT[i+1][j]  [k+1] ;
      StoretetT[qa][cnt++][3] = (int) numT[i+1][j+1][k]   ;
    }

  }

  StoretetT[qa][cnt  ][0] = (int) numT[qa-2-k-j][j]  [k]   ;
  StoretetT[qa][cnt  ][1] = (int) numT[qa-1-k-j][j]  [k]   ;
  StoretetT[qa][cnt  ][2] = (int) numT[qa-2-k-j][j+1][k]   ;
  StoretetT[qa][cnt++][3] = (int) numT[qa-2-k-j][j]  [k+1] ;
      }
    }

    tetT   = StoretetT[qa];

    free_dtarray(numT,0,0,0);
  }
}


static void Interpolate   (double xj, double yj, double zj, double vj,
                      double xk, double yk, double zk, double vk,
                      double cx[], double cy[], double cz[], double val, int&
         counter);
static void ThreeSimilar  (int i, int j, int k, int *pr, int *ps);
static void TwoPairs      (double cx[], double cy[], double cz[], int *pr);


static void Dump_Contour(FILE *out, int nnodes, double *x, double *y,
       double *z, double *c, int nelmts, int **tets,
       double val, int zoneNum){
  double cx[5], cy[5], cz[5];
  int faceid[5];
  int i, j, k, ii, jj, kk, r, s, n, counter, boolean, repeat;

  for (repeat=1;repeat<=2;repeat++){
    n=0;
    for (i=0;i<nelmts;i++) {
      if  (!(((c[tets[i][0]]>val)&&(c[tets[i][1]]>val)&&
              (c[tets[i][2]]>val)&&(c[tets[i][3]]>val))||
             ((c[tets[i][0]]<val)&&(c[tets[i][1]]<val)&&
              (c[tets[i][2]]<val)&&(c[tets[i][3]]<val)))) {
        counter=0;

        for (j=0; j<=2; j++){
          for (k=j+1; k<=3; k++){
            if (((c[tets[i][j]]>=val)&&(val>=c[tets[i][k]]))||
                ((c[tets[i][j]]<=val)&&(val<=c[tets[i][k]]))){

              Interpolate(x[tets[i][j]],y[tets[i][j]],
                          z[tets[i][j]],c[tets[i][j]],
                          x[tets[i][k]],y[tets[i][k]],
                          z[tets[i][k]],c[tets[i][k]],
        cx, cy, cz, val, counter);
            }
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
}


static Iso *Extract_Contour(int nnodes, double *x, double *y,  double *z,
          double *c, int nelmts, int **tets,
          double val, int zoneNum){
  double cx[5], cy[5], cz[5];
  int    faceid [5];
  int i, j, k, ii, jj, kk, r, s, n, counter, boolean, repeat;

  Iso *iso;

  iso = new Iso [1];

  iso->set_eid(zoneNum);

  n = 0;
  for (i=0;i<nelmts;i++) {
    // check to see if val is between vertex values
    if  (!(((c[tets[i][0]]>val)&&(c[tets[i][1]]>val)&&
      (c[tets[i][2]]>val)&&(c[tets[i][3]]>val))||
     ((c[tets[i][0]]<val)&&(c[tets[i][1]]<val)&&
      (c[tets[i][2]]<val)&&(c[tets[i][3]]<val)))) {

      // loop over all edges and interpolate if contour is between vertex values
      for (counter = 0, j=0; j<=2; j++){
  for (k=j+1; k<=3; k++){
    if (((c[tets[i][j]]>=val)&&(val>=c[tets[i][k]]))||
        ((c[tets[i][j]]<=val)&&(val<=c[tets[i][k]]))){

      Interpolate(x[tets[i][j]],y[tets[i][j]],
      z[tets[i][j]],c[tets[i][j]],
      x[tets[i][k]],y[tets[i][k]],
      z[tets[i][k]],c[tets[i][k]],
      cx, cy, cz, val, counter);
    }
  }
      }

      switch(counter){
      case 3:
  n+=1;
  iso->xyz_realloc(3*n);

  for(j = 0; j < 3; ++j){
    iso->set_x(3*(n-1)+j,cx[j]);
    iso->set_y(3*(n-1)+j,cy[j]);
    iso->set_z(3*(n-1)+j,cz[j]);
  }
  break;
      case 4:
  n+=2;
  iso->xyz_realloc(3*n);

  for(j = 0; j < 3; ++j){
    iso->set_x(3*(n-2)+j,cx[j]);
    iso->set_y(3*(n-2)+j,cy[j]);
    iso->set_z(3*(n-2)+j,cz[j]);

    iso->set_x(3*(n-1)+j,cx[j+1]);
    iso->set_y(3*(n-1)+j,cy[j+1]);
    iso->set_z(3*(n-1)+j,cz[j+1]);
  }
  break;
      case 5:
  n+=1;

  iso->xyz_realloc(3*n);

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

    iso->set_x(3*(n-1)  ,cx[ii]);
    iso->set_x(3*(n-1)+1,cx[r]);
    iso->set_x(3*(n-1)+2,cx[s]);

    iso->set_y(3*(n-1)  ,cy[ii]);
    iso->set_y(3*(n-1)+1,cy[r]);
    iso->set_y(3*(n-1)+2,cy[s]);

    iso->set_z(3*(n-1)  ,cz[ii]);
    iso->set_z(3*(n-1)+1,cz[r]);
    iso->set_z(3*(n-1)+2,cz[s]);
        }
        else {
    boolean+=0;
        }
      }
    }
  }
  if (boolean==0){
    TwoPairs (cx,cy,cz,&r);

    iso->set_x(3*(n-1)  ,cx[0]);
    iso->set_x(3*(n-1)+1,cx[2]);
    iso->set_x(3*(n-1)+2,cx[r]);

    iso->set_y(3*(n-1)  ,cy[0]);
    iso->set_y(3*(n-1)+1,cy[2]);
    iso->set_y(3*(n-1)+2,cy[r]);

    iso->set_z(3*(n-1),  cz[0]);
    iso->set_z(3*(n-1)+1,cz[2]);
    iso->set_z(3*(n-1)+2,cz[r]);

  }
  break;
      }
    }
  }

  iso->set_ntris(n);
  return iso;
}


/************************/
/* Function Interpolate */
/************************/

static void Interpolate (double xj, double yj, double zj,  double cj,
       double xk, double yk, double zk, double ck,
       double cx[], double cy[], double cz[], double val,
       int& counter){
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

    ++counter;
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
      case 'l':
  if (*++argv[0])
    dparam_set("CONTOURVAL", atof(*argv));
  else {
    dparam_set("CONTOURVAL", atof(*++argv));
    argc--;
  }
  (*argv)[1] = '\0';
  break;
      case 'e':
  option_set("DumpElmt",1);
  break;
      case 'g':
  option_set("GlobalCondense",1);
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
