/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/vort_coreOnly.C,v $
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

char *prog   = "vort";
char *usage  = "vort:  [options]  -r file[.rea]  input[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "-q      ... quadrature point spacing. Default is even spacing\n"
  "-R      ... range data information. must have mesh file specified\n"
  "-b      ... make body elements specified in mesh file\n"
  "-p      ... project vorticity by inverting mass matrix\n"
  "-s      ... use stretching measure instead of lambda 2 \n"
  "-d      ... dump output to a field file (implied -p)\n"
  "-f      ... dump in equispace finite element data format \n"
  "-D      ... used Denis Doorly normalisation of lambda\n"
#if DIM == 2
  "-n #    ... Number of mesh points. Default is 15\n";
#else
  "-n #    ... Number of mesh points.";
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
static void Calc_Vort (FileList *f, Element_List **U, int nfields);
static void Get_Body(FILE *fp);
static void dump_faces(FILE *out, Element_List **E, Coord X, int nel, int zone,
           int nfields);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void WriteS(Element_List **E, FILE *out, int nfields);
static void WriteC(Element_List **E, FILE *out, int nfields);
static void Write(Element_List **E, FILE *out, int nfields);

void solve(Element_List *U, Element_List *Uf,
     Bndry *Ubc, Bsystem *Ubsys,SolveType Stype)
{
  SetBCs (U,Ubc,Ubsys);
  Solve  (U,Uf,Ubc,Ubsys,Stype);
}


main (int argc, char *argv[])
{
  register int  i,k;
  int       dump=0,nfields,nftot;
  FileList  f;
  Element_List **master;

  init_comm(&argc, &argv);

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  master  = (Element_List **) malloc(_MAX_FIELDS*sizeof(Element_List *));
  nfields = setup (&f, master, &nftot);

  ReadCopyField(&f,master);

  Calc_Vort(&f,master,nfields);

  DIM = master[0]->fhead->dim();

  if(option("FEstorage"))
    WriteS(master,f.out.fp, nftot);
  else if(option("Continuous"))
    WriteC(master,f.out.fp, nftot);
  else
    Write(master,f.out.fp, nftot);

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

static void Average_edges_div(Element_List *Us, Element_List *Uf,
            char operation){
  register int i,j,k,n;
  int     qedg,nel = Us->nel;
  double  w1,w2;
  Edge    *e;

  for(k = 0; k < nel; ++k)
    for(i = 0 ; i < Us->flist[k]->Nedges; ++i){
      if((e = Us->flist[k]->edge+i)->link){
  qedg = e->qedg;
  w1   = e->weight;
  w2   = e->link->weight;
  if(e->con == e->link->con)
    for(j = 0; j < qedg; ++j)
      Uf->flist[k]->edge[i].h[j] =e->h[j]*w1 + e->link->h[j]*w2;
  else
    for(j = 0; j < qedg; ++j)
      Uf->flist[k]->edge[i].h[j] =e->h[j]*w1 + e->link->h[qedg-1-j]*w2;
      }
      else{
  qedg = e->qedg;
  w1   = e->weight;
  w2   = e->base->weight;
  if(e->con == e->base->con)
    for(j = 0; j < qedg; ++j)
      Uf->flist[k]->edge[i].h[j] =e->h[j]*w1 + e->base->h[j]*w2;
  else
    for(j = 0; j < qedg; ++j)
      Uf->flist[k]->edge[i].h[j] =e->h[j]*w1 + e->base->h[qedg-1-j]*w2;
      }
    }
}

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
  nftot  += (fld.dim == 3) ? 5:3;

  ReadParams  (f->rea.fp);
  if((i=iparam("NORDER.REQ"))!=UNSET){
    iparam_set("LQUAD",i+1);
    iparam_set("MQUAD",i+1);
  }
  else if(option("Qpts")){
    iparam_set("LQUAD",fld.lmax+1);
    iparam_set("MQUAD",fld.lmax);
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

  DO_PARALLEL{ // recall global numbering to put partition vertices first
    free_gmap(gmap);
    Reflect_Global_Velocity    (Mesh, (Bndry *)NULL, 0);
    gmap  = GlobalNumScheme    (Mesh, (Bndry *)NULL);
    Replace_Numbering          (U[0], Mesh);
  }

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

  nft[0] = nftot;
  return nfields;
}


static  void ReadCopyField (FileList *f, Element_List **U){
  int i,dump;
  int nfields;
  Field fld;

  memset(&fld, '\0', sizeof (Field));

  dump = readField (f->in.fp, &fld);
  if (!dump) error_msg(Restart: no dumps read from restart file);
  rewind (f->in.fp);

  nfields = strlen(fld.type);

  for(i = 0; i < nfields; ++i)
    copyfield(&fld,i,U[i]->fhead);

  freeField(&fld);
}


static void Calc_Vort (FileList *f, Element_List **UU, int nfields){
  int i,j,k,qt;
  double *z, *w;

  /*************** new variables ***********************/
  int INFO;
  int denis_lambda = option("Denis_Lambda");
  char U, N;
  double S11,S12,S13,S22,S23,S33,T12,T13,T23,p11,p12,p13,p22,p23,p33;
  double *EigV = dvector(0,5);
  double *Lam = dvector(0,2);
  double **Z = dmatrix(0,2,0,2);
  double **WORK = dmatrix(0,8,0,8);
  /*****************************************************/


  for(i=0;i<nfields;++i)
    UU[i]->Trans(UU[i],J_to_Q);

  if(UU[0]->fhead->dim() == 3){

    double  **d = dmatrix(0,8,0,QGmax*QGmax*QGmax-1);

    double  *Ux=d[0], *Uy=d[1], *Uz=d[2];
    double  *Vx=d[3], *Vy=d[4], *Vz=d[5];
    double  *Wx=d[6], *Wy=d[7], *Wz=d[8];

    for(k = 0; k < UU[0]->nel; ++k){
      qt = UU[0]->flist[k]->qtot;

      UU[0]->flist[k]->Grad_d(Ux, Uy, Uz, 'a');
      UU[1]->flist[k]->Grad_d(Vx, Vy, Vz, 'a');
      UU[2]->flist[k]->Grad_d(Wx, Wy, Wz, 'a');


      /*************** added code here ************************/
      for(i=0;i<qt;++i){
  S11=S12=S13=S22=S23=S33=T12=T13=T23=0.0;
        p11=p12=p13=p22=p23=p33=0.0;
  UU[nfields+4]->flist[k]->h_3d[0][0][i]=0.0;

  S11=Ux[i];
  S12=0.5*(Uy[i]+Vx[i]);
  S13=0.5*(Uz[i]+Wx[i]);
  S22=Vy[i];
  S23=0.5*(Vz[i]+Wy[i]);
        S33=Wz[i];

  if(option("Stretching")) // zero off diagonal compoonents.
    S12 = S13 = S23 = 0;


  T12=0.5*(Uy[i]-Vx[i]);
  T13=0.5*(Uz[i]-Wx[i]);
  T23=0.5*(Vz[i]-Wy[i]);

  p11=pow(S11,2)+pow(S12,2)+pow(S13,2)-pow(T12,2)-pow(T13,2);
  p12=S11*S12+S12*S22+S13*S23-T13*T23;
  p13=S11*S13+S12*S23+S13*S33+T12*T23;
  p22=pow(S12,2)+pow(S22,2)+pow(S23,2)-pow(T12,2)-pow(T23,2);
  p23=S12*S13+S22*S23+S23*S33-T12*T13;
  p33=pow(S13,2)+pow(S23,2)+pow(S33,2)-pow(T13,2)-pow(T23,2);

  EigV[0]=p11;
  EigV[1]=p12;
  EigV[2]=p22;
  EigV[3]=p13;
  EigV[4]=p23;
  EigV[5]=p33;


  dspev('N','U',3,EigV,Lam,*Z,3,*WORK,INFO);
  if (INFO) error_msg("WARNING: Vortex Core Method -dspev- "
            "INFO is not zero\n");
  if(denis_lambda)
    UU[nfields+4]->flist[k]->h_3d[0][0][i]=Lam[1]*Lam[1]/Lam[0];
  else
    UU[nfields+4]->flist[k]->h_3d[0][0][i]=Lam[1];

      }
      /**************** added code ends here *******************/


      dvsub(qt, Wy, 1, Vz, 1, **UU[nfields  ]->flist[k]->h_3d, 1);
      dvsub(qt, Uz, 1, Wx, 1, **UU[nfields+1]->flist[k]->h_3d, 1);
      dvsub(qt, Vx, 1, Uy, 1, **UU[nfields+2]->flist[k]->h_3d, 1);

      dvadd(qt, Ux, 1, Vy, 1, **UU[nfields+3]->flist[k]->h_3d, 1);
      dvadd(qt, Wz, 1, **UU[nfields+3]->flist[k]->h_3d, 1,
      **UU[nfields+3]->flist[k]->h_3d, 1);
    }

    free_dmatrix(d,0,0);

    if(option("PROJECT")){
      double sfac = dparam("SFAC");
      double lam = 0.0;
      Bndry *Tbc = (Bndry*)NULL;

      if(sfac) lam = 1.0/sfac;

      Element_List *T    = UU[0]->gen_aux_field('T');
      Bsystem      *Bsys = gen_bsystem(T, gmap);
      Element_List *Tf   = T->gen_aux_field('T');

      ROOTONLY
  fprintf(stderr,"Projecting Lamda2 -- Generating Matrix [");

      Bsys->lambda = (Metric*) calloc(UU[0]->nel, sizeof(Metric));

      dcopy(UU[nfields+4]->htot, UU[nfields+4]->base_h, 1, Tf->base_h, 1);
      //dcopy(UU[nfields-1]->htot, UU[nfields-1]->base_h, 1, Tf->base_h, 1);
      Tf->Set_state('p');

      if(lam){
  int tot;
  Bndry *B;
  Element *E;
  double *tx = dvector(0,4*QGmax*QGmax*QGmax-1);
  double *ty,*tz,*tmp;
  ty  = tx + QGmax*QGmax*QGmax;
  tz  = ty + QGmax*QGmax*QGmax;
  tmp = tz + QGmax*QGmax*QGmax;

  // set up boundary system based on flux evaluation
  Tbc  = ReadBCs (f->rea.fp,Tf->fhead);
  for(B = Tbc; B; B = B->next){
    E = B->elmt;

    if(B->type != 'F'){
      B->type = 'F';
      B->elmt->Surface_geofac(B);
    }
    E->Grad_d(tx,ty,tz,'a');
    /* take dot product with normal */
    E->GetFace(tx, B->face, tmp);
    E->InterpToFace1(B->face,tmp,tx);
    E->GetFace(ty, B->face, tmp);
    E->InterpToFace1(B->face,tmp,ty);
    E->GetFace(tz, B->face, tmp);
    E->InterpToFace1(B->face,tmp,tz);

    if(E->Nfverts(B->face)==3){
      switch(E->identify()){
      case Nek_Prism: case Nek_Pyr:
        tot = E->qa*E->qc;
        break;
      case Nek_Tet:
        tot = E->qa*E->qb;
        break;
      }
    }
    else
      tot = E->qa*E->qb;

    if(E->curvX){
      dvmul  (tot, B->nx.p, 1, tx, 1, tmp, 1);
      dvvtvp (tot, B->ny.p, 1, ty, 1, tmp, 1, tmp, 1);
      dvvtvp (tot, B->nz.p, 1, tz, 1, tmp, 1, tmp, 1);
    }
    else{
      dsmul  (tot, B->nx.d, tx, 1, tmp, 1);
      dsvtvp (tot, B->ny.d, ty, 1, tmp, 1, tmp, 1);
      dsvtvp (tot, B->nz.d, tz, 1, tmp, 1, tmp, 1);
    }

    E->MakeFlux(B,0,tmp);
  }
  free(tx);

  for(i = 0; i < UU[0]->nel; ++i)
    Bsys->lambda[i].d = lam;

  GenMat(T, Tbc, Bsys, Bsys->lambda, Helm);
      }
      else
  GenMat(T, Tbc, Bsys, Bsys->lambda, Mass);

      ROOTONLY fprintf(stderr," ]\n");


      if(lam){
  ROOTONLY fprintf(stderr,"Projecting Lamda2 with Filter "
       "(SFAC = %lf) \n",1./lam);
  dscal(Tf->htot,-lam,Tf->base_h,1);
  solve(T, Tf, Tbc, Bsys, Helm);
      }
      else{
  ROOTONLY fprintf(stderr,"Projecting Lamda2 \n");
  solve(T, Tf, Tbc, Bsys, Mass);
      }

      T->Trans(UU[nfields+4], J_to_Q);
      dcopy(T->hjtot, T->base_hj, 1, UU[nfields+4]->base_hj, 1);
      UU[nfields+4]->fhead->type = 'l';


      if(option("DUMPFILE")){
  char out[BUFSIZ],out1[BUFSIZ];
  FILE *fp[2];

  ROOTONLY fprintf(stderr,"Writing output\n");
  sprintf(out,"%s_CoreOnly",f->rea.name);

  DO_PARALLEL{
    char out1[BUFSIZ];

    sprintf(out1,"%s.fld.hdr.%d",out,pllinfo.procid);
    fp[0] = fopen(out1,"w");
    sprintf(out1,"%s.fld.dat.%d",out,pllinfo.procid);
    fp[1] = fopen(out1,"w");
  }
        else{
    sprintf(out1,"%s.fld",out);
    fp[0] = fp[1] = fopen(out1,"w");
  }

  UU[nfields  ]->Set_state('t');
  UU[nfields+1]->Set_state('t');
  UU[nfields+2]->Set_state('t');
  UU[nfields+3]->Set_state('t');
  UU[nfields+4]->Set_state('t');

  Writefld(fp, out, 0 ,0 , 1, UU+nfields+4);

  DO_PARALLEL{
    fclose(fp[0]);
    fclose(fp[1]);
  }
        else
    fclose(fp[0]);
  DO_PARALLEL
    exit_comm();
  exit(1);
      }
    }
  }
  else{

    double  **d = dmatrix(0,3,0,QGmax*QGmax-1);
    double  *Ux = d[0], *Uy = d[1];
    double  *Vx = d[2], *Vy = d[3];
    {
      for(k = 0; k < UU[0]->nel; ++k){

  qt = UU[0]->flist[k]->qtot;

  UU[0]->flist[k]->Grad_d(Ux, Uy, 0, 'a');
  UU[1]->flist[k]->Grad_d(Vx, Vy, 0, 'a');

  dvsub(qt, Vx, 1, Uy, 1, *UU[nfields]->flist[k]->h, 1);
  dvadd(qt, Ux, 1, Vy, 1, *UU[nfields+1]->flist[k]->h, 1);
      }
    }

    if(option("PROJECT")){
      Element_List *T  = UU[0]->gen_aux_field('T');
      Bsystem *Bsys = gen_bsystem(T, gmap);
      Element_List *Tf = T->gen_aux_field('T');

      ROOTONLY fprintf(stderr,"Projecting Vorticity -- Generating Matrix [");

      Bsys->lambda = (Metric*) calloc(UU[0]->nel, sizeof(Metric));
      GenMat(T, NULL, Bsys, Bsys->lambda, Mass);
      ROOTONLY fprintf(stderr," ]\n");

      dcopy(UU[nfields]->htot, UU[nfields]->base_h, 1, Tf->base_h, 1);
      Tf->Set_state('p');
      solve(T, Tf, NULL, Bsys, Mass);
      T->Trans(UU[nfields], J_to_Q);
      dcopy(T->hjtot, T->base_hj, 1, UU[nfields]->base_hj, 1);
      UU[nfields]->fhead->type = 'a';

      dcopy(UU[nfields+1]->htot, UU[nfields+1]->base_h, 1, Tf->base_h, 1);
      Tf->Set_state('p');
      solve(T, Tf, NULL, Bsys, Mass);
      T->Trans(UU[nfields+1], J_to_Q);
      dcopy(T->hjtot, T->base_hj, 1, UU[nfields+1]->base_hj, 1);
      UU[nfields+1]->fhead->type = 'b';

      if(option("DUMPFILE")){
  char out[BUFSIZ],out1[BUFSIZ];
  FILE *fp[2];

  ROOTONLY fprintf(stderr,"Writing output\n");
  sprintf(out,"%s_vort",f->rea.name);

  DO_PARALLEL{
    char out1[BUFSIZ];

    sprintf(out1,"%s.fld.hdr.%d",out,pllinfo.procid);
    fp[0] = fopen(out1,"w");
    sprintf(out1,"%s.fld.dat.%d",out,pllinfo.procid);
    fp[1] = fopen(out1,"w");
  }
        else{
    sprintf(out1,"%s.hdr",out);
    fp[0] = fp[1] = fopen(out,"w");
  }

  UU[nfields]->Set_state('t');
  UU[nfields+1]->Set_state('t');

  Writefld(fp, out, 0, 0, 2, UU + nfields);

  DO_PARALLEL{
    fclose(fp[0]);
    fclose(fp[1]);
  }
        else
    fclose(fp[0]);
  exit(1);
      }
    }
    free_dmatrix(d,0,0);
  }
}

static int Check_range(Element *E);

static void WriteS(Element_List **E, FILE *out, int nfields){
  register int i,j,k,n,e,nelmts;
  int      qa,cnt, *interior;
  const int    nel = E[0]->nel;
  int      dim = E[0]->fhead->dim(),ntot;
  double   *z,*w, ave;
  Coord    X;
  char     *outformat;
  Element  *F;


  interior = ivector(0,E[0]->nel-1);

  if(E[0]->fhead->dim() == 3){
    double ***num;

    X.x = dvector(0,QGmax*QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax*QGmax-1);
    X.z = dvector(0,QGmax*QGmax*QGmax-1);


    ROOTONLY{
      fprintf(out,"VARIABLES = x y z");

      for(i = 0; i < nfields-5; ++i)
  fprintf(out," %c", E[i]->fhead->type);

      fprintf(out," Vorticity_x Vorticity_y Vorticity_z Divergence Lamda2");

      fputc('\n',out);
    }

    for(k = 0,n=i=0; k < E[0]->nel; ++k){
      F  = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  i += qa*(qa+1)*(qa+2)/6;
  n += (qa-1)*(qa-1)*(qa-1);
      }
    }

    switch(F->identify()){
    case Nek_Tet:
      fprintf(out,"ZONE  N=%d, E=%d, F=FEPOINT, ET=TETRAHEDRON\n",i,n);
      break;
    default:
      fprintf(stderr,"WriteS is not set up for this element type \n");
      exit(1);
      break;
    }

    /* dump data */
    for(k = 0; k <  E[0]->nel ; ++k){
      F  = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  F->coord(&X);
  ntot = Interp_symmpts(F,F->qa,X.x,X.x,'p');
  ntot = Interp_symmpts(F,F->qa,X.y,X.y,'p');
  ntot = Interp_symmpts(F,F->qa,X.z,X.z,'p');
  for(n = 0; n < nfields; ++n){
    F = E[n]->flist[k];
    Interp_symmpts(F,F->qa,F->h_3d[0][0],F->h_3d[0][0],'p');
  }
  for(i = 0; i < ntot ; ++i){
    fprintf(out,"%lg %lg %lg", X.x[i], X.y[i], X.z[i]);
    for(n = 0; n < nfields; ++n)
      fprintf(out," %lg",E[n]->flist[k]->h_3d[0][0][i]);
    fputc('\n',out);
  }
      }
    }


    /* numbering array */
    num = dtarray(0,QGmax-1,0,QGmax-1,0,QGmax-1);

    for(cnt = 1, k = 0; k < qa; ++k)
      for(j = 0; j < qa-k; ++j)
  for(i = 0; i < qa-k-j; ++i, ++cnt)
    num[i][j][k] = cnt;

    for(e = 0,n=0; e < E[0]->nel; ++e){
      F  = E[0]->flist[e];
      if(Check_range(F)){
  qa = F->qa;
  /* dump connectivity */
  switch(F->identify()){
  case Nek_Tet:
    for(k=0; k < qa-1; ++k)
      for(j = 0; j < qa-1-k; ++j){
        for(i = 0; i < qa-2-k-j; ++i){
           fprintf(out,"%d %d %d %d\n", n+(int) num[i][j][k],
      n+(int) num[i+1][j][k],
      n+(int) num[i][j+1][k], n+(int) num[i][j][k+1]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j][k],
      n+(int) num[i][j+1][k],
      n+(int) num[i][j][k+1], n+(int) num[i+1][j][k+1]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j][k+1],
      n+(int) num[i][j][k+1],
      n+(int) num[i][j+1][k+1], n+(int) num[i][j+1][k]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j+1][k],
      n+(int) num[i][j+1][k],
      n+(int) num[i+1][j][k], n+(int) num[i+1][j][k+1]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j+1][k],
      n+(int) num[i][j+1][k],
      n+(int) num[i+1][j][k+1], n+(int) num[i][j+1][k+1]);
    if(i < qa-3-k-j)
      fprintf(out,"%d %d %d %d\n", n+(int) num[i][j+1][k+1],
        n+(int) num[i+1][j+1][k+1],
        n+(int) num[i+1][j][k+1],
        n+(int) num[i+1][j+1][k]);
        }
                fprintf(out,"%d %d %d %d\n", n+(int) num[qa-2-k-j][j][k],
          n+(int) num[qa-1-k-j][j][k],
          n+(int) num[qa-2-k-j][j+1][k],
          n+(int) num[qa-2-k-j][j][k+1]);
      }
    n += qa*(qa+1)*(qa+2)/6;
    break;
  default:
    fprintf(stderr,"WriteS is not set up for this element type \n");
    exit(1);
    break;
  }
      }
    }

    free(X.x); free(X.y); free(X.z);
    free_dtarray(num,0,0,0);
  }
  else{

    ROOTONLY{
      fprintf(out,"VARIABLES = x y");

      for(i = 0; i < nfields-2; ++i)
  fprintf(out," %c", E[i]->fhead->type);

      fprintf(out," Vorticity Divergence");
      fputc('\n',out);
    }

    /* set up global number scheme for this partition */
    for(k = 0,n=0; k < E[0]->nel; ++k){
      F= E[0]->flist[k];
      qa = F->qa;

      if(F->identify() == Nek_Tri){
  nelmts += (qa-1)*(qa-1);
  n += qa*(qa+1)/2;
      }
      else{
  nelmts += 2*(qa-1)*(qa-1);
  n += qa*qa;
      }
    }

    /* treat all elements as tri's for hybrid code in tecplot */
    fprintf(out,"ZONE, N=%d, E=%d, F=FEPOINT, ET=TRIANGLE\n",n, nelmts);

    X.x = dvector(0,QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax-1);

    /* dump data */
    for(k = 0; k < E[0]->nel; ++k){
      F = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  F->coord(&X);

  ntot = Interp_symmpts(F,qa,X.x,X.x,'p');
  ntot = Interp_symmpts(F,qa,X.y,X.y,'p');
  for(n = 0; n < nfields; ++n){
    F = E[n]->flist[k];
    Interp_symmpts(F,qa,F->h[0],F->h[0],'p');
  }

  for(i = 0; i < ntot; ++i){
    fprintf(out,"%lg %lg", X.x[i], X.y[i]);
    for(n = 0; n < nfields; ++n)
      fprintf(out," %lg",E[n]->flist[k]->h[0][i]);
    fputc('\n',out);
  }
      }
    }

    /* dump connectivity */
    for(k = 0,n=0; k < E[0]->nel; ++k){
      F = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  if(F->identify() == Nek_Tri){
    for(cnt = 0,j = 0; j < qa-1; ++j){
      for(i = 0; i < qa-2-j; ++i){
        fprintf(out,"%d %d %d\n",n+cnt+i+1,n+cnt+i+2,n+cnt+qa-j+i+1);
        fprintf(out,"%d %d %d\n",n+cnt+qa-j+i+2,n+cnt+qa-j+i+1,n+cnt+i+2);
      }
      fprintf(out,"%d %d %d\n",n+cnt+qa-1-j,n+cnt+qa-j,
        n+cnt+2*qa-2*j-1);
      cnt += qa-j;
    }
    n += qa*(qa+1)/2;
  }
  else{
    for(cnt = 0,j = 0; j < qa-1; ++j){
      for(i = 0; i < qa-1; ++i){
        fprintf(out,"%d %d %d\n",cnt+i+1,cnt+i+2,cnt+qa+i+1);
        fprintf(out,"%d %d %d\n",cnt+qa+i+2,cnt+qa+i+1,cnt+i+2);
      }
      cnt += qa;
    }
    n += 2*(qa-1)*(qa-1);
  }
      }
    }
    free(X.x); free(X.y);
  }
}

static void WriteC(Element_List **E, FILE *out, int nfields){
  register int i,j,k,n,e,nelmts;
  int      qa,cnt, *interior;
  const int    nel = E[0]->nel;
  int      dim = E[0]->fhead->dim(),ntot;
  double   *z,*w, ave;
  Coord    X;
  char     *outformat;
  Element  *F;


  interior = ivector(0,E[0]->nel-1);

  if(E[0]->fhead->dim() == 3){
    double ***num;

    fprintf(stderr,"CONTINUOUS dump scheme not set up yet in 3d \n");
    exit(1);

    X.x = dvector(0,QGmax*QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax*QGmax-1);
    X.z = dvector(0,QGmax*QGmax*QGmax-1);

    ROOTONLY{
      fprintf(out,"VARIABLES = x y z");

      for(i = 0; i < nfields-5; ++i)
  fprintf(out," %c", E[i]->fhead->type);

      fprintf(out," Vorticity_x Vorticity_y Vorticity_z Divergence Lamda2");

      fputc('\n',out);
    }

    for(k = 0,n=i=0; k < E[0]->nel; ++k){
      F  = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  i += qa*(qa+1)*(qa+2)/6;
  n += (qa-1)*(qa-1)*(qa-1);
      }
    }

    switch(F->identify()){
    case Nek_Tet:
      fprintf(out,"ZONE  N=%d, E=%d, F=FEPOINT, ET=TETRAHEDRON\n",i,n);
      break;
    default:
      fprintf(stderr,"WriteS is not set up for this element type \n");
      exit(1);
      break;
    }

    /* dump data */
    for(k = 0; k < E[0]->nel; ++k){
      F  = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  F->coord(&X);
  ntot = Interp_symmpts(F,F->qa,X.x,X.x,'m');
  ntot = Interp_symmpts(F,F->qa,X.y,X.y,'m');
  ntot = Interp_symmpts(F,F->qa,X.z,X.z,'m');
  for(n = 0; n < nfields; ++n){
    F = E[n]->flist[k];
    Interp_symmpts(F,F->qa,F->h_3d[0][0],F->h_3d[0][0],'m');
  }
  for(i = 0; i < ntot; ++i){
    fprintf(out,"%lg %lg %lg", X.x[i], X.y[i], X.z[i]);
    for(n = 0; n < nfields; ++n)
      fprintf(out," %lg",E[n]->flist[k]->h_3d[0][0][i]);
    fputc('\n',out);
  }
      }
    }


    /* numbering array */
    num = dtarray(0,QGmax-1,0,QGmax-1,0,QGmax-1);

    for(cnt = 1, k = 0; k < qa; ++k)
      for(j = 0; j < qa-k; ++j)
  for(i = 0; i < qa-k-j; ++i, ++cnt)
    num[i][j][k] = cnt;

    for(e = 0,n=0; e < E[0]->nel; ++e){
      F  = E[0]->flist[e];
      if(Check_range(F)){
  qa = F->qa;
  /* dump connectivity */
  switch(F->identify()){
  case Nek_Tet:
    for(k=0; k < qa-1; ++k)
      for(j = 0; j < qa-1-k; ++j){
        for(i = 0; i < qa-2-k-j; ++i){
    fprintf(out,"%d %d %d %d\n", n+(int) num[i][j][k],
      n+(int) num[i+1][j][k],
      n+(int) num[i][j+1][k], n+(int) num[i][j][k+1]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j][k],
      n+(int) num[i][j+1][k],
      n+(int) num[i][j][k+1], n+(int) num[i+1][j][k+1]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j][k+1],
      n+(int) num[i][j][k+1],
      n+(int) num[i][j+1][k+1], n+(int) num[i][j+1][k]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j+1][k],
      n+(int) num[i][j+1][k],
      n+(int) num[i+1][j][k], n+(int) num[i+1][j][k+1]);
    fprintf(out,"%d %d %d %d\n", n+(int) num[i+1][j+1][k],
      n+(int) num[i][j+1][k],
      n+(int) num[i+1][j][k+1], n+(int) num[i][j+1][k+1]);
    if(i < qa-3-k-j)
      fprintf(out,"%d %d %d %d\n", n+(int) num[i][j+1][k+1],
        n+(int) num[i+1][j+1][k+1],
        n+(int) num[i+1][j][k+1],
        n+(int) num[i+1][j+1][k]);
        }
        fprintf(out,"%d %d %d %d\n", n+(int) num[qa-2-k-j][j][k],
          n+(int) num[qa-1-k-j][j][k],
          n+(int) num[qa-2-k-j][j+1][k],
          n+(int) num[qa-2-k-j][j][k+1]);
      }
    n += qa*(qa+1)*(qa+2)/6;
    break;
  default:
    fprintf(stderr,"WriteS is not set up for this element type \n");
    exit(1);
    break;
  }
      }
    }

    free(X.x); free(X.y); free(X.z);
    free_dtarray(num,0,0,0);
  }
  else{
    ROOTONLY{
      fprintf(out,"VARIABLES = x y");

      for(i = 0; i < nfields-2; ++i)
  fprintf(out," %c", E[i]->fhead->type);

      fprintf(out," Vorticity Divergence");
      fputc('\n',out);
    }

    /* set up global number scheme for this partition */
    for(k = 0,n=1; k < E[0]->nel; ++k){
      F= E[0]->flist[k];
      qa = F->qa;
      for(i = 0; i < F->Nverts; ++i)
  if(F->vert[0].base){
    if(F->vert[i].base == F->vert+i)
      F->vert[i].gid = n++;
  }
  else
    F->vert[i].gid = n++;

      for(i = 0; i < F->Nedges; ++i)
  if(F->edge[i].base){
    if(F->edge[i].base == F->edge+i){
      F->edge[i].gid = n;
      n += qa-2;
    }

    if(F->edge[i].base->con){ /* make sure base con is 0 */
      F->edge[i].base->con = 0;
      F->edge[i].link->con = 1;
    }
  }
  else{
    F->edge[i].gid = n;
    n+=qa-2;
  }

      interior[k] = n;
      if(F->identify() == Nek_Tri){
  nelmts += (qa-1)*(qa-1);
  n += (qa-3)*(qa-2)/2;
      }
      else{
  nelmts += 2*(qa-1)*(qa-1);
  n += (qa-2)*(qa-2);
      }
    }

    /* look over and update link lists */
    for(k = 0; k < E[0]->nel; ++k){
      F= E[0]->flist[k];
      qa = F->qa;
      for(i = 0; i < F->Nverts; ++i)
  if(F->vert[i].base)
    if(F->vert[i].base != F->vert+i)
      F->vert[i].gid = F->vert[i].base->gid;

      for(i = 0; i < F->Nedges; ++i)
  if(F->edge[i].base)
    if(F->edge[i].base != F->edge+i)
      F->edge[i].gid = F->edge[i].base->gid;
    }

    /* treat all elements as tri's for hybrid code in tecplot */
    fprintf(out,"ZONE, N=%d, E=%d, F=FEPOINT, ET=TRIANGLE\n",n-1, nelmts);

    X.x = dvector(0,QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax-1);

    /* dump data */
    for(k = 0; k < E[0]->nel; ++k){
      F = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;
  F->coord(&X);

  ntot = Interp_symmpts(F,qa,X.x,X.x,'m');
  ntot = Interp_symmpts(F,qa,X.y,X.y,'m');

  for(n = 0; n < nfields; ++n){
    F = E[n]->flist[k];
    Interp_symmpts(F,qa,F->h[0],F->h[0],'m');
  }

  F = E[0]->flist[k];
  cnt = 0;
  for(i = 0; i < F->Nverts; ++i,++cnt)
    if((!F->vert[i].base)||(F->vert[i].base == F->vert+i)){
      fprintf(out,"%lg %lg", X.x[cnt], X.y[cnt]);
      for(n = 0; n < nfields; ++n)
        fprintf(out," %lg",E[n]->flist[k]->h[0][cnt]);
      fputc('\n',out);
    }

  for(i = 0; i < F->Nedges; ++i,cnt += qa-2)
    if((!F->edge[i].base)||(F->edge[i].base == F->edge+i))
      for(j = 0; j < qa-2; ++j){
        fprintf(out,"%lg %lg", X.x[cnt+j], X.y[cnt+j]);
        for(n = 0; n < nfields; ++n)
    fprintf(out," %lg",E[n]->flist[k]->h[0][cnt+j]);
        fputc('\n',out);
      }

  for(j = cnt; j < ntot; ++j){
    fprintf(out,"%lg %lg", X.x[j], X.y[j]);
    for(n = 0; n < nfields; ++n)
      fprintf(out," %lg",E[n]->flist[k]->h[0][j]);
    fputc('\n',out);
  }
      }
    }


    int **num = imatrix(0,QGmax-1,0,QGmax-1);

    /* dump connectivity */
    for(k = 0,n=0; k < E[0]->nel; ++k){
      F = E[0]->flist[k];
      if(Check_range(F)){
  qa = F->qa;

  if(F->identify() == Nek_Tri){
    /* set up numbering */
    num[0][0]    = F->vert[0].gid;
    num[0][qa-1] = F->vert[1].gid;
    num[qa-1][0] = F->vert[2].gid;

    if(F->edge[0].con)
      for(i = 0; i < qa-2; ++i)
        num[0][i+1] = F->edge[0].gid+qa-3-i;
    else
      for(i = 0; i < qa-2; ++i)
        num[0][i+1] = F->edge[0].gid+i;

    if(F->edge[1].con)
      for(i = 0; i < qa-2; ++i)
        num[i+1][qa-2-i] = F->edge[1].gid+qa-3-i;
    else
      for(i = 0; i < qa-2; ++i)
        num[i+1][qa-2-i] = F->edge[1].gid+i;

    if(F->edge[2].con)
      for(i = 0; i < qa-2; ++i)
        num[i+1][0] = F->edge[2].gid+qa-3-i;
    else
      for(i = 0; i < qa-2; ++i)
        num[i+1][0] = F->edge[2].gid+i;

    for(cnt = 0,j = 0; j < qa-3; ++j)
      for(i = 0; i < qa-3-j; ++i,++cnt)
        num[j+1][i+1] = interior[k] + cnt;

    for(cnt = 0,j = 0; j < qa-1; ++j){
      for(i = 0; i < qa-2-j; ++i){
        fprintf(out,"%d %d %d\n",num[j][i],num[j][i+1],num[j+1][i]);
        fprintf(out,"%d %d %d\n",num[j+1][i+1],num[j+1][i],
          num[j][i+1]);
      }
      fprintf(out,"%d %d %d\n",num[j][qa-2-j],num[j][qa-1-j],
        num[j+1][qa-2-j]);
    }

  }
  else{
    fprintf(stderr,"Continuous output not set up for quads: WriteC\n");
    exit(1);
    for(cnt = 0,j = 0; j < qa-1; ++j){
      for(i = 0; i < qa-1; ++i){
        fprintf(out,"%d %d %d\n",cnt+i+1,cnt+i+2,cnt+qa+i+1);
        fprintf(out,"%d %d %d\n",cnt+qa+i+2,cnt+qa+i+1,cnt+i+2);
      }
      cnt += qa;
    }
    n += 2*(qa-1)*(qa-1);
  }
      }

    }
    free_imatrix(num,0,0);
    free(X.x); free(X.y);
  }
}

static void Write(Element_List **E, FILE *out, int nfields){
  register int i,j,k,n;
  int      qa,qb,qc,qt,zone;
  const int    nel = E[0]->nel;
  int      dim = E[0]->fhead->dim();
  double   *z,*w, ave;
  Coord    X;
  char     *outformat;
  Element  *F;

  if(!option("Qpts")){
    /* transform to equispaced */
    for(i=0;i<nfields;++i)
      OrthoInnerProduct(E[i],E[i]);
    reset_bases();
    init_ortho_basis(); /* this is a lazy way of reseting ortho basis */

    for(j=2;j<QGmax+2;++j){
      getzw(j,&z,&w,'a');
      for(i = 0; i < j; ++i) z[i] = 2.0*i/(double)(j-1) -1.0;

      getzw(j,&z,&w,'b');
      for(i = 0; i < j; ++i) z[i] = 2.0*i/(double)(j-1) -1.0;
    }
    if(dim == 3)
      for(j=2;j<QGmax+2;++j){
  getzw(j,&z,&w,'c');
  for(i = 0; i < j; ++i) z[i] = 2.0*i/(double)(j-1) -1.0;
      }

    for(i=0;i<nfields;++i)
      OrthoJTransBwd(E[i],E[i]);
  }


  if(E[0]->fhead->dim() == 3){
    X.x = dvector(0,QGmax*QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax*QGmax-1);
    X.z = dvector(0,QGmax*QGmax*QGmax-1);

    ROOTONLY{
      fprintf(out,"VARIABLES = x y z");

      for(i = 0; i < nfields-5; ++i)
  fprintf(out," %c", E[i]->fhead->type);

      fprintf(out," Vorticity_x Vorticity_y Vorticity_z Divergence Lamda2");
      fputc('\n',out);
    }

    for(k = 0,zone=0; k < E[0]->nel; ++k){
      F = E[0]->flist[k];
      if(Check_range(F)){
  F->coord(&X);
  fprintf(out,"ZONE T=\"Triangle %d\", I=%d, J=%d, K=%d, F=POINT\n",
    ++zone,F->qa,F->qb,F->qc);
  for(i = 0; i < F->qtot; ++i){
    fprintf(out,"%lg %lg %lg", X.x[i], X.y[i], X.z[i]);
    for(n = 0; n < nfields; ++n)
      fprintf(out," %lg",E[n]->flist[k]->h_3d[0][0][i]);
    fputc('\n',out);
  }
      }
    }
    if(bdy.N) dump_faces(out,E,X,k,zone,nfields);
    free(X.x); free(X.y); free(X.z);
  }
  else{
    ROOTONLY{
      fprintf(out,"VARIABLES = x y");

      for(i = 0; i < nfields-2; ++i)
  fprintf(out," %c", E[i]->fhead->type);

      fprintf(out," Vorticity Divergence");
      fputc('\n',out);
    }

    X.x = dvector(0,QGmax*QGmax-1);
    X.y = dvector(0,QGmax*QGmax-1);

    for(k = 0,zone=0; k < E[0]->nel; ++k){
      F = E[0]->flist[k];
      if(Check_range(F)){
  F->coord(&X);

  fprintf(out,"ZONE T=\"Triangle %d\", I=%d, J=%d, F=POINT\n",
    ++zone,F->qa,F->qb);
  for(i = 0; i < F->qtot; ++i){
    fprintf(out,"%lg %lg", X.x[i], X.y[i]);
    for(n = 0; n < nfields; ++n)
      fprintf(out," %lg",E[n]->flist[k]->h[0][i]);
    fputc('\n',out);
  }
      }
    }
    free(X.x); free(X.y);
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
      case 'b':
  option_set("Body",1);
  break;
      case 'c':
  option_set("Continuous",1);
  break;
      case 'f':
  option_set("FEstorage",1);
  break;
      case 'R':
  option_set("Range",1);
  break;
      case 'q':
  option_set("Qpts",1);
  break;
      case 'd':
  option_set("DUMPFILE",1);
      case 'p':
  option_set("PROJECT",1);
  break;
      case '2':
  option_set("Stretching",1);
  break;
      case 'i':
  option_set("iterative",1);
  break;
      case 'D':
  option_set("Denis_Lambda",1);
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
