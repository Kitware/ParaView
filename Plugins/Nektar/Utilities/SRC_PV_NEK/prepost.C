/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/prepost.C,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/08 14:18:48 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/

#include <math.h>
#include <veclib.h>
#include "nektar.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>



/* local functions */
static void     Summary      (void);
int             bandwidth(Element *E, Bsystem *Bsys);
static int      suminterior    (Element *E);


struct ssn_tag {
  char name[FILENAME_MAX];    /* name of session without postfix   */
  char fld [FILENAME_MAX];    /* Output (field) file               */
  char his [FILENAME_MAX];    /* History point file                */
  char rea [FILENAME_MAX];    /* Input file (from pre-processor)   */
  char fce [FILENAME_MAX];    /* Forces file                       */
} session;


static struct {              /* Default options for Nekscal         */
  char *name;
  int   val;
  char *descrip;
} nektar_ops[] = {
  "order"     ,0, "-n #   ... run with # of modes ",
  "binary"    ,1, "-a     ... write output in ascii format",
  "verbose"   ,0, "-v     ... plot divergence error",
  "variable"  ,0, "-var   ... run with variable order (uses [].ord file)",
  "checkpt"   ,0, "-chk   ... dump checkpoint fields at \'iostep\' steps",
  "oldhybrid" ,0, "-old   ... read old hybrid format field files",
  "iterative" ,0, "-i     ... iterative solve",
  "Initcond"  ,0, "-I     ... set up initial condition",
  "recursive" ,0, "-r #   ... recursive static condesation with # recursions",
  "timeavg"   ,0, "-T     ... evaluate time average field",
  "scalar"    ,0, "",
   0, 0, 0
};

void parse_args(int argc, char *argv[])
{
  int     i;
  char    c;

#ifdef DEBUG
  init_debug();
//  mallopt(M_DEBUG,0);
#endif

  /* initialize the parser and install the defaults */

  manager_init();

  for (i = 0; nektar_ops[i].name; i++)
    option_set(nektar_ops[i].name, nektar_ops[i].val);

  if(argc == 1) goto showUsage;

  while (--argc && (*++argv)[0] == '-') {
    if (strcmp (*argv+1, "chk") == 0) {
      option_set("checkpt",1);
      continue;
    }
    else if (strcmp (*argv+1, "var") == 0) {
      option_set("variable",1);
      continue;
    }
    else if (strcmp (*argv+1, "old") == 0) {
      option_set("oldhybrid",1);
      continue;
    }
    while (c = *++argv[0])
      switch (c) {
      case 'a':
  option_set("binary",0);
  break;
      case 'b':
  fprintf(stderr,"Option b is now automatic\n");
  break;
      case 'c':
  { int n;
    if (*++argv[0])
      n = atoi (*argv);
    else {
      n = atoi (*++argv);
      argc--;
    }
    option_set("TORDER",n);
    (*argv)[1] = '\0';
  break;
  }
      case 'i':
  option_set("iterative",1);
  break;
      case 'I':
  option_set("Initcond",1);
  break;
      case 'm':
  option_set("mixediter",1);
  break;
      case 'n':
  { int n;
    if (*++argv[0])
      n = atoi (*argv);
    else {
      n = atoi (*++argv);
      argc--;
    }
    option_set("NORDER.REQ",n);
    (*argv)[1] = '\0';
  }
  break;
      case 'r':
  { int n;
    if (*++argv[0])
      n = atoi (*argv);
    else {
      n = atoi (*++argv);
      argc--;
    }
    option_set("recursive",n);
    (*argv)[1] = '\0';
  }
  break;
      case 'S':
  {
    option_set("SLICES",1);
    break;
  }
      case 't':
  { double n;
    if (*++argv[0])
      n = atof (*argv);
    else {
      n = atof (*++argv);
      argc--;
    }
    dparam_set("THETA",n);
    (*argv)[1] = '\0';
  }
  break;
      case 'T':
  option_set("timeavg",1);
  break;
      case 'v':
  option_set("verbose",2);
  break;
      case 'V':
  option_set("tvarying",1);
  break;
      default:
  goto showUsage;
      }
  }
  return;

 showUsage:
  ROOTONLY{
    fputs("usage: nektar [options] file[.rea]\n\n"
    "options:\n", stderr);
    for (i = 0; nektar_ops[i].name; i++)
      fprintf(stderr, "%s\n", nektar_ops[i].descrip);
  }
  exit(-1);
}

void LocalNumScheme  (Element_List *E, Bsystem *Bsys, Gmap *gmap);
Gmap *GlobalNumScheme(Element_List *E, Bndry *Ebc);

void free_Mesh_Facets(Element_List *Mesh);
void free_Mesh_Structure(Element_List *Mesh);
void free_Mesh_Bcs(Bndry *MeshBcs);

Element_List *Mesh;

#if 0
Domain *PreProcess(int argc, char **argv)
{
  FILE     *rea_file;
  Element  *E;
  Element_List *U,*P;
  Bndry    *Meshbc,*Ubc,*Pbc;
  Bsystem  *Ubsys,*Pbsys;
  Domain   *omega;
  int       k,Je;
  double    Re,dt;
  Gmap     *gmap;
  Gmap     *gmap_p;
  int       procid = mynode();
  int       active_handle = get_active_handle();


  /* Create the new domain and open the input files */

  parse_args(argc, argv);

  option_set("GSLEVEL", min(pllinfo[active_handle].nprocs,8));
  option_set("FAMOFF" , 1);

  omega = (Domain*) malloc(sizeof(Domain));

  sprintf(session.name,"%s"     , strtok(argv[argc-1],"."));
  sprintf(session.rea, "%s.rea" , argv[argc-1]);
  sprintf(session.fld, "%s.fld" , argv[argc-1]);
  sprintf(session.his, "%s.his" , argv[argc-1]);

#ifdef DEBUGNOT
  DO_PARALLEL{
    char *buf = (char*) calloc(BUFSIZ,sizeof(char));
    sprintf(buf, "%s.dbx.%d", argv[argc-1], procid);
    debug_out = fopen(buf, "w");
  }
#endif

  ROOTONLY
    sprintf(session.fce, "%s.fce" , argv[argc-1]);

  omega->name     = argv[argc-1];

  rea_file        = fopen(session.rea,"r");
  if (rea_file == (FILE*) NULL)
    error_msg(PreProcess(): Could not open input file(s));

  omega->fld_file = fopen(session.fld,"w");
  ROOTONLY
    omega->fce_file = fopen(session.fce,"w");

  /* Read the input parameters */

  ReadParams  (rea_file);
  ReadPscals  (rea_file);
  ReadLogics  (rea_file);

  Je = iparam("INTYPE");

  /* Build the mesh */
  Mesh   = ReadMesh(rea_file, session.name);
  Meshbc = ReadMeshBCs(rea_file,Mesh);
  gmap   = GlobalNumScheme(Mesh, Meshbc);

  U = LocalMesh(Mesh,session.name);
  P = U->gen_aux_field ('p');

  DO_PARALLEL{ // recall global numbering to put partition vertices first
    free_gmap(gmap);
    Reflect_Global_Velocity    (Mesh, Meshbc, 0);
    gmap  = GlobalNumScheme    (Mesh, Meshbc);
    Replace_Numbering          (omega->U, Mesh);
  }

  /* set up velocity system using U */
  Ubc   = ReadBCs (rea_file,U->fhead); /* also sets up edge links system    */
  Ubsys = gen_bsystem(U,gmap);        /* set up velocity matrix structure  */

  ROOTONLY Summary ();

  /* set up pressure system */
  free_gmap(gmap);

  Set_Global_Pressure     (Mesh, Meshbc);
  gmap_p  = GlobalNumScheme (Mesh, Meshbc);

  Replace_Numbering     (P, Mesh);
  Pbc   = BuildPBCs     (P, Ubc);
  Pbsys = gen_bsystem   (P, gmap_p);

  free_gmap(gmap_p);

  omega->Pf = P->gen_aux_field('p');
  omega->Uf = U->gen_aux_field('u');

  omega->V   = U->gen_aux_field ('v');
  omega->Vbc = ReadBCs       (rea_file,omega->V->fhead);

  if(option("REFLECT1")||option("REFLECT0")){
    Reflect_Global_Velocity    (Mesh, Meshbc, 1);
    gmap  = GlobalNumScheme    (Mesh, Meshbc);
    Replace_Numbering          (omega->V, Mesh);
    omega->Vsys = gen_bsystem  (omega->V, gmap);
    free_gmap(gmap);
  }
  else
    omega->Vsys = omega->Usys;

  omega->W   = U->gen_aux_field ('w');
  omega->Wbc = ReadBCs       (rea_file,omega->W->fhead);
  omega->Wf  = omega->W->gen_aux_field('w');

  if(option("REFLECT2")||
     (option("REFLECT0")&&option("REFLECT1")&&(!option("REFLECT2")))){
    Reflect_Global_Velocity   (Mesh, Meshbc, 2);
    gmap  = GlobalNumScheme   (Mesh, Meshbc);
    Replace_Numbering         (omega->W, Mesh);
    omega->Wsys = gen_bsystem (omega->W, gmap);
    free_gmap(gmap);
  }
  else if (!option("REFLECT0"))
    omega->Wsys = omega->Usys;
  else if (!option("REFLECT1")){
    Replace_Local_Numbering(omega->W,omega->V);
    omega->Wsys = omega->Vsys;
  }

  omega->Vf  = omega->V->gen_aux_field('v');

  omega->U     = U;  omega->Ubc   = Ubc;
  omega->P     = P;  omega->Pbc   = Pbc;

  dt = dparam("DELT");
  omega->soln = NULL;
  ReadSoln(rea_file, omega);

  Re = 1.0 / dparam("KINVIS");

  ReadKinvis (omega);

  Pbsys->lambda = (Metric*) calloc(P->nel, sizeof(Metric));
  ROOTONLY{
    fprintf(stdout,"Generating pressure system [.");
    fflush(stdout);
  }
  GenMat (P,Pbc,Pbsys,Pbsys->lambda,Helm);
  ROOTONLY{
    fprintf(stdout,"]\n");
    fflush(stdout);
  }

  Ubsys->lambda = (Metric*) calloc(U->nel, sizeof(Metric));

  if(omega->kinvis->p){
    Ubsys->lambda->p = dvector(0, U->htot-1);
    dcopy( U->htot, omega->kinvis->p,1,Ubsys->lambda->p, 1);
  }

  int skip = 0;
  for(k=0;k<U->nel;++k){
    Ubsys->lambda[k].d = Re*getgamma(1)/dt;
    if(omega->kinvis[k].p)
      Ubsys->lambda[k].p = Ubsys->lambda->p + skip;

    skip += U->flist[k]->qtot;
  }

  ROOTONLY {
    fprintf(stdout,"Generating velocity system [.");
    fflush(stdout);
  }
  GenMat (U,Ubc,Ubsys,Ubsys->lambda,Helm);
  ROOTONLY {
    fprintf(stdout,"]\n");
    fflush(stdout);
  }
#ifndef PARALLEL
  if(!option("tvarying")){
    DirBCs(omega->U,omega->Ubc,Ubsys,Helm);
    DirBCs(omega->V,omega->Vbc,Ubsys,Helm);
    DirBCs(omega->W,omega->Wbc,Ubsys,Helm);
    omega->U->zerofield();
    omega->V->zerofield();
    omega->W->zerofield();
  }
#endif

  omega->Usys = Ubsys;
  omega->Wsys = Ubsys;
  omega->Pressure_sys = Pbsys;

  DO_PARALLEL
    sprintf(session.his,"%s.%d",session.his,pllinfo[active_handle].procid);

  omega->his_file = fopen(session.his,"w");
  ROOTONLY omega->fce_file = fopen(session.fce,"w");

  ReadICs_P     (rea_file, omega, Mesh);

  ReadDF      (rea_file, U->fhead->dim());
  ReadHisData (rea_file, omega);
  ReadDFunc   (rea_file, omega, Mesh);

  free_Mesh_Facets(Mesh); // free edges, vertices
  free_Mesh_Structure(Mesh);

  // Set up multistep storage
  omega->u  = (double**) malloc(Je*sizeof(double*));
  for(k = 0; k < Je; ++k){
    omega->u[k] = dvector(0,U->htot*U->nz-1);
    dzero(U->htot*U->nz, omega->u[k], 1);
  }
  omega->uf = (double**) malloc(Je*sizeof(double*));
  for(k = 0; k < Je; ++k){
    omega->uf[k] = dvector(0,omega->Uf->htot*omega->Uf->nz-1);
    dzero(U->htot*U->nz, omega->uf[k], 1);
  }

  omega->v  = (double**) malloc(Je*sizeof(double*));
  for(k = 0; k < Je; ++k){
    omega->v[k] = dvector(0,omega->V->htot*omega->V->nz-1);
    dzero(U->htot*U->nz, omega->v[k], 1);
  }

  omega->vf  = (double**) malloc(Je*sizeof(double*));
  for(k = 0; k < Je; ++k){
    omega->vf[k] = dvector(0,omega->Vf->htot*omega->Vf->nz-1);
    dzero(U->htot*U->nz, omega->vf[k], 1);
  }

  omega->w  = (double**) malloc(Je*sizeof(double*));
  for(k = 0; k < Je; ++k){
    omega->w[k] = dvector(0,omega->W->htot*omega->W->nz-1);
    dzero(U->htot*U->nz, omega->w[k], 1);
  }

  omega->wf  = (double**) malloc(Je*sizeof(double*));
  for(k = 0; k < Je; ++k){
    omega->wf[k] = dvector(0,omega->Wf->htot*omega->Wf->nz-1);
    dzero(U->htot*U->nz, omega->wf[k], 1);
  }


 /* this needs to be set after LocalMesh so that pllinfo is defined */
  DO_PARALLEL{
    sprintf(session.fld, "%s.fld.hdr.%d", omega->name,pllinfo[active_handle].procid);
    omega->fld_file = fopen(session.fld,"w");
    sprintf(session.fld, "%s.fld.dat.%d", omega->name,pllinfo[active_handle].procid);
    omega->dat_file = fopen(session.fld,"w");
  }
  else{
    sprintf(session.fld, "%s.fld", omega->name);
    omega->fld_file = fopen(session.fld,"w");
    omega->dat_file = omega->fld_file;
 }
  fclose(rea_file);
  return omega;
}
#endif

void PostProcess(Domain *omega, int step, double time){
  Element_List **V;
  FILE  *fp[2];
  int nfields;
  nfields = omega->U->fhead->dim()+1;

  fp[0] = omega->fld_file;
  fp[1] = omega->dat_file;


  V = (Element_List**) malloc(nfields*sizeof(Element_List*));

  V[0] = omega->U;
  V[1] = omega->V;
  V[2] = omega->W;
  V[3] = omega->P;


  Writefld(fp, omega->name, step, time, nfields, V);

  DO_PARALLEL{
    fclose(fp[0]);
    fclose(fp[1]);
  }
  else
    fclose(fp[0]);

   ROOTONLY  fclose(omega->his_file);
   ROOTONLY  fclose(omega->fce_file);
}

/* ----------------------------------------------------------------------- *
 * Summary() - Print a summary of the input data                           *
 *                                                                         *
 * This function collects the echo of the input data which used to be      *
 * scattered throughout this file.                                         *
 *                                                                         *
 * ----------------------------------------------------------------------- */

static void Summary (void)
{

   printf ("Input File          : %s\n",session.name);
  printf ("Reynolds number     : %g\n",1./dparam("KINVIS"));
#ifdef OSSCYL
  printf ("KC   number         : %g\n",dparam("KC"));
  printf ("beta number         : %g\n",1./(dparam("KC")*dparam("KINVIS")));
#endif
  printf ("Time step           : %g\n",dparam("DELT"));
  printf ("Integration order   : %d\n",iparam("INTYPE"));
    if(!option("variable"))
     printf("Number of modes     : %d\n",iparam("MODES"));
  else
     printf("Number of modes     : variable\n");
  printf ("Number of elements  : %d\n",iparam("ELEMENTS"));
  printf ("Number of Families  : %d\n",iparam("FAMILIES"));
  DO_PARALLEL{
    printf ("Number of Processors: %d\n",pllinfo[get_active_handle()].nprocs);
    printf ("Gs level            : %d\n",option("GSLEVEL"));
  }
  fputs ("Equation type       : ", stdout);
  switch (iparam("EQTYPE")) {
  case Rotational:
    puts("Navier-Stokes (rotational)");
    break;
  case Convective:
    puts("Navier-Stokes (convective)");
    break;
  case Stokes:
    puts("Stokes flow");
    break;
  default:
    puts("undefined");
    break;
  }

  printf("Integration time    : %g, or %d steps\n",
   dparam("FINTIME"), iparam("NSTEPS"));
  printf("I/O time for saves  : %g, or %d steps",
   dparam("IOTIME"),  iparam("IOSTEP"));

  if  (option("checkpt"))
    fputs (" [checkpoint]\n", stdout);
  else {
    putchar ('\n');
    if(iparam("NSTEPS"))
      if (iparam("NSTEPS") / iparam("IOSTEP") > 10)
  fputs ("Summary: " "You have more than 10 dumps..."
          "did you want to use -chk?\n", stderr);
  }

  if(option("iterative")){
    printf("Solver Type         : Iterative ");
    switch((int) dparam("PRECON")){
    case Pre_Diag:
      printf("(Precon = diagonal)\n");
      break;
    case Pre_Block:
      printf("(Precon = block)\n");
      break;
    case Pre_None:
      printf("(Precon = none)\n");
      break;
    case Pre_LEnergy:
      printf("(Precon = low energy  block)\n");
      break;
    }
  }
  else{
    printf("Solver Type         : Direct ");
    if(option("recursive"))
      printf("Mutlilevel \n");
    else
      printf("RCM \n");
  }

  return;
}

/* This is a function to sort out the global numbering scheme for      *
 * vertices, edges and faces. It also sets up the list of cumulative   *
 * indices for edges and vertices.                                     */

static void set_bmap       (Element *, Bsystem *);

Bsystem *gen_bsystem(Element_List *UL, Gmap *gmap){
  Bsystem  *Bsys;
  Element  *E=UL->fhead;
  Bsys       = (Bsystem *)calloc(1,sizeof(Bsystem));

  DO_PARALLEL /* force iterative solver if in parallel */
    option_set("iterative",1);

  if(option("iterative")){
    Bsys->smeth = iterative;
    Bsys->Precon = (enum PreType) (int) dparam("PRECON");
  }

  /* basis numbering scheme for vertices, edges and faces */
  LocalNumScheme(UL,Bsys,gmap);

  if(option("recursive")&&!option("iterative"))
    Mlevel_SC_decom(UL,Bsys);
  else  if(Bsys->smeth == direct){
    bandwidthopt(E,Bsys,'a');
    ROOTONLY{
      fprintf(stderr,"rcm bandwidth (%c)   : %d [%d (%d)] \n",E->type,
        bandwidth(E,Bsys),Bsys->nsolve,suminterior(E));
      fflush(stdout);
    }
  }

  set_bmap(E,Bsys);

  Bsys->families = iparam("FAMILIES");
  return Bsys;
}

static void  setGid(Element_List *E);

Gmap *GlobalNumScheme(Element_List *UL, Bndry *Ebc){
  Element  *U = UL->fhead;
  register int i;
  int       nvg,nvs,neg,nes,nfg,nfs,scnt,ncnt;
  int      *gsolve,*gbmap,l,l1;
  Bndry    *Ubc;
  Vert     *v;
  Edge     *e;
  Face     *f;
  Gmap     *gmap;
  Element *E;

  gmap = (Gmap *)malloc(sizeof(Gmap));

  setGid (UL);  /* setup a global numbering scheme i.e. without boundaries*/

  /* This part of the routine re-orders the vertices, edges and then
     the faces so that the knowns are listed first. For the edges and
     the faces it also initialises a cummalative list stored in Bsys. */

  /*--------------------*/
  /* Vertex re-ordering */
  /*--------------------*/

  /* find maximum number of global vertices; */
  for(E=U,nvg=0; E; E = E->next)
    for(i = 0; i < E->Nverts; ++i)
      nvg = max(E->vert[i].gid, nvg);
  ++nvg;

  gsolve = ivector(0,nvg-1);
  gbmap  = ivector(0,nvg-1);

  ifill(nvg, 1, gsolve,1);
  // Assemble vertex solve mask to sort out multiplicity issues
  for(E=U; E; E = E->next)
    for(i = 0; i < E->Nverts; ++i) // note vert[i].solve can have mag of 1 or 2
      gsolve[E->vert[i].gid] = (gsolve[E->vert[i].gid]*E->vert[i].solve)?
  max(gsolve[E->vert[i].gid],E->vert[i].solve):0;
       //gsolve[E->vert[i].gid] &= E->vert[i].solve;

  // copy back mask
  for(E=U; E; E = E->next){
    v = E->vert;
    for(i = 0; i < E->Nverts; ++i)
     v[i].solve = gsolve[E->vert[i].gid];
  }

  scnt = 0; ncnt = 0;

  DO_PARALLEL{
    /* reset vertex solve id ordering so that points on parallel
       patches are first  */
    for(i = 0; i < nvg; ++i)
      if(gsolve[i] == 2)
  gbmap[i] =  scnt++;
      else if(gsolve[i] == 0)
  gbmap[i] = ncnt++;

    gmap->nv_psolve = scnt;

    for(i = 0; i < nvg; ++i)
      if(gsolve[i] == 1)
  gbmap[i] =  scnt++;
  }
  else{
    for(i = 0; i < nvg; ++i)
      gbmap[i] = gsolve[i]? scnt++:ncnt++;
  }
  nvs = scnt;

  /* place unknowns at the end of the pile */
  for(i = 0; i < nvg; i++)
    gbmap[i] += gsolve[i]?  0:nvs;

  /* replace vertices numbering into vertex structures */
  for(E=U; E; E = E->next)
    for(i = 0; i < E->Nverts; ++i)
      E->vert[i].gid = gbmap[E->vert[i].gid];


  /* optimise vertex ordering for iterative solver */
  if(option("iterative")&&0){ // turn off form moment since going to use static
    Bsystem B;                // condensation on vertex solve

    /* set up dummy B system */
    B.nel = UL->nel;
    B.nv_solve = nvs;
    bandwidthopt(UL->fhead,&B,'v');
  }

  free(gsolve); free(gbmap);

  /*------------------*/
  /* Edge re-ordering */
  /*------------------*/

  /* find maximum number of global edges; */
  for(E = U,neg=0; E; E = E->next)
    for(i = 0; i < E->Nedges; ++i)
      neg = max(E->edge[i].gid,neg);
  ++neg;

  gsolve = ivector(0,neg-1);
  gbmap  = ivector(0,neg-1);

  /* form edge gsolve and gbmap */
  ifill(neg, 1, gsolve,1);
  for(Ubc = Ebc; Ubc; Ubc = Ubc->next)
    if((Ubc->type == 'V')||(Ubc->type == 'W')||(Ubc->type == 'o')){
      E = Ubc->elmt;
      l = Ubc->face;
      if(E->dim() == 2)
  gsolve[E->edge[l].gid] = 0;
      else
  for(i = 0; i < E->Nfverts(l); ++i)
    gsolve[E->edge[E->ednum(l,i)].gid] = 0;
    }

  scnt = 0;  ncnt = 0;
  for(i = 0; i < neg; ++i)
    gbmap[i] = gsolve[i]? (scnt++):(ncnt++);
  nes   = scnt;

  /* place unknowns at the end of the pile */
  for(i = 0; i < neg; ++i)
    gbmap[i] += gsolve[i]?  0:nes;

  /* replace sort gid's */
  for(E = U; E; E = E->next)
    for(i = 0; i < E->Nedges; ++i)
      E->edge[i].gid = gbmap[E->edge[i].gid];

  if(U->dim() == 3){
    /*------------------*/
    /* Face re-ordering */
    /*------------------*/

    /* find maximum number of global faces; */
    for(E = U, nfg = 0; E; E = E->next)
      for(i = 0; i < E->Nfaces; ++i)
  nfg = (E->face[i].gid > nfg)? E->face[i].gid:nfg;
    ++nfg;

    gsolve = ivector(0,nfg-1);
    gbmap  = ivector(0,nfg-1);

    /* form faces part of gsolve, gbmap */
    ifill(nfg, 1, gsolve,1);
    for(Ubc = Ebc; Ubc; Ubc = Ubc->next)
      if((Ubc->type == 'V')||(Ubc->type == 'W')||(Ubc->type == 'o'))
  gsolve[Ubc->elmt->face[Ubc->face].gid] = 0;

    scnt = 0; ncnt = 0;
    for(i = 0; i < nfg; ++i)
      gbmap[i] = gsolve[i]? (scnt++):(ncnt++);
    nfs = scnt;

    /* place unknowns at the end of the pile */
    for(i = 0; i < nfg; ++i)
      gbmap[i] += gsolve[i]?  0:nfs;

    /* replace sorted gid's */
    for(E=U;E;E=E->next){
      f=E->face;
      for(i = 0; i < E->Nfaces; ++i)
  f[i].gid = gbmap[f[i].gid];
    }

    free(gsolve); free(gbmap);
  }
  else{
    nfs = 0;
    nfg = 0;
  }


 /* finally set up a global id information  */
  gmap->nvs = nvs;
  gmap->nvg = nvg;
  gmap->nes = nes;
  gmap->neg = neg;
  gmap->nfs = nfs;
  gmap->nfg = nfg;

  gmap->vgid = ivector(0,nvg-1);
  gmap->egid = ivector(0,neg-1);
  gmap->fgid = ivector(0,nfg);

  int *flen, *elen  = ivector(0,neg-1);

  /* fill list with size */
  for(E=U;E;E=E->next){
    for(i = 0; i < E->Nedges; ++i)
      elen[E->edge[i].gid] = E->edge[i].l;
  }

  if(U->dim() == 3){
    flen  = ivector(0,nfg-1);

    for(E=U;E;E=E->next)
      for(i = 0; i < E->Nfaces; ++i)
  flen[E->face[i].gid] = (E->Nfverts(i) == 3) ?
    (E->face[i].l)*(E->face[i].l+1)/2 :
    (E->face[i].l)*(E->face[i].l);
  }

  /* set up global numbering scheme */
  int cnt = 0;
  for(i = 0; i < nvs; ++i)
    gmap->vgid[i] = cnt++;

  for(i = 0; i < nes; cnt += elen[i],++i)
    gmap->egid[i] = cnt;

  if(U->dim() == 3)
    for(i = 0; i < nfs;  cnt +=flen[i],++i)
      gmap->fgid[i] = cnt;

  gmap->nsolve = cnt;

  for(i = nvs; i < nvg; ++i)
    gmap->vgid[i] = cnt++;

  for(i = nes; i < neg; cnt += elen[i],++i)
    gmap->egid[i] = cnt;

  if(U->dim() == 3)
    for(i = nfs; i < nfg;  cnt +=flen[i],++i)
      gmap->fgid[i] = cnt;

  gmap->nglobal = cnt;

  free(elen);

  if(U->dim() == 3)
    free(flen);

  return gmap;

}

void free_gmap(Gmap *gmap){
  free(gmap->vgid);
  free(gmap->egid);
  free(gmap->fgid);
  free(gmap);
}

void LocalNumScheme(Element_List *EL, Bsystem *Bsys, Gmap *gmap){
  register  int i,k;
  int       nvk,nvs,nek,nes,ncnt,cnt;
  int       *vmap,*emap,*elen;
  int       nel = EL->nel; /* local elements */
  int       nfk,nfs,dim;
  int      *fmap,*flen;
  int      nfstot,nstot,nvstot,nestot,nvktot,nektot;
  Element   *E;

  /* This part of the routine re-orders the vertices, edges and then
     the faces so that the knowns are listed first. For the edges and
     the faces it also initialises a cummalative list stored in Bsys. */

  dim = EL->fhead->dim();

  /*-----------i---------*/
  /* Vertex re-ordering */
  /*--------------------*/

  /* find the number of solved and global vertices in local mesh; */
  vmap  = ivector(0,gmap->nvg-1);
  ifill(gmap->nvg,-1,vmap,1);

  for(E=EL->fhead;E;E=E->next)
    for(i = 0; i < E->Nverts; ++i)
      vmap[E->vert[i].gid] = 1;

  /* count the number of locally solved vertices and set up mapping */
  nvs = 0;
  for(i = 0; i < gmap->nvs; ++i)
    if(vmap[i]+1) vmap[i] = nvs++;

  /* count the number of locally known vertices and set up mapping */
  /* Note: haven't added nvs onto vmap here since need to add nsolve
     at end of routine. */
  nvk = 0;
  for(i = gmap->nvs; i < gmap->nvg; ++i)
    if(vmap[i]+1) vmap[i] = nvk++;


  /*--------------------*/
  /* Edge re-ordering   */
  /*--------------------*/

  /* find the number of solved and known edges in local mesh; */
  emap = ivector(0,gmap->neg-1);
  elen = ivector(0,gmap->neg-1);
  ifill(gmap->neg,-1,emap,1);
  ifill(gmap->neg,-1,elen,1);

  for(E=EL->fhead;E;E=E->next)
    for(i = 0; i < E->Nedges; ++i){
      emap[E->edge[i].gid] = 1;
      elen[E->edge[i].gid] = E->edge[i].l;
    }

  /* count the number of locally solved edges and make up cummalative
     edge list */
  nes  = 0;
  for(i = 0; i < gmap->nes; ++i)
    if(emap[i]+1)  emap[i]  = nes++;

  /* count the number of locally known edges and make up cummalative
     edge list*/
  nek = 0;
  for(i = gmap->nes; i < gmap->neg; ++i)
    if(emap[i]+1) emap[i]  = nes + nek++;

  /* set up cumulative edge list */
  Bsys->edge = ivector(0,nes+nek);
  ncnt = 0;
  for(i = 0, cnt = 0; i < gmap->neg; ++i){
    if(elen[i]+1){
      Bsys->edge[cnt++] = ncnt;
      ncnt += elen[i];
    }
  }
  Bsys->edge[cnt] = ncnt; /* this puts total edge length in final value */

  /* work out total number of local solve values */
  Bsys->nv_solve  = nvs;
  Bsys->ne_solve  = nes;

  Bsys->nel       = EL->nel;
  Bsys->nsolve    = nvs + Bsys->edge[nes];
  Bsys->nglobal   = nvs + nvk + Bsys->edge[nes+nek];

  if(dim == 3){
    /*--------------------*/
    /* Face re-ordering   */
    /*--------------------*/

    /* find the number of solved and known faces  in local mesh; */

    fmap  = ivector(0,gmap->nfg-1);
    flen  = ivector(0,gmap->nfg-1);
    ifill(gmap->nfg,-1,fmap,1);
    ifill(gmap->nfg,-1,flen,1);

    for(E=EL->fhead;E;E=E->next)
      for(i = 0; i < E->Nfaces; ++i){
  fmap[E->face[i].gid] = 1;
  flen[E->face[i].gid] = (E->Nfverts(i) == 3) ?
    E->face[i].l*(E->face[i].l+1)/2 : E->face[i].l*E->face[i].l;
      }

    /* count the number of locally solved faces and set up cummalative
       face list */
    nfs  = 0;
    for(i = 0; i < gmap->nfs; ++i)
      if(fmap[i]+1) fmap[i] = nfs++;


    /* count the number of locally known vertices and set up mapping */
    nfk = 0;
    for(i = gmap->nfs; i < gmap->nfg; ++i)
      if(fmap[i]+1) fmap[i] =  nfs + nfk++;

    /* set up cumulative face list */
    Bsys->face = ivector(0,nfs+nfk);
    ncnt = 0;
    for(i = 0,cnt= 0; i < gmap->nfg; ++i){
      if(flen[i]+1){
  Bsys->face[cnt++] = ncnt;
  ncnt += flen[i];
      }
    }
    Bsys->face[cnt] = ncnt; /* this puts total edge length in final value */

    /* work out total number of local solve values */
    Bsys->nf_solve  = nfs;

    Bsys->nsolve    = nvs + Bsys->edge[nes] + Bsys->face[nfs];
    Bsys->nglobal   = nvs + nvk + Bsys->edge[nes+nek] + Bsys->face[nfs+nfk];
  }

  /* We want a local ordering scheme where the solved vertices are
     first followed by the solved edges and then solved faces. We then
     wish to order the known vertices followed by the known edges and
     finally the known face which is done below */

  /* Add nsolve to known vertices */
  for(i = gmap->nvs; i < gmap->nvg; ++i)
    if(vmap[i]+1) vmap[i] += Bsys->nsolve;

  /* Sort out values in cumalative list of face and edges. If faces
     are used these need to be done first because of requried
     information stored in edge list */

  nstot  = Bsys->nsolve;

  nvstot = nvs;
  nestot = Bsys->edge[nes];
  if(dim == 3)
    nfstot = Bsys->face[nfs];
  else
    nfstot = 0;

  nvktot = nvk;
  nektot = Bsys->edge[nes+nek]-Bsys->edge[nes];

  for(i = 0; i < nes; ++i)
    Bsys->edge[i] += nvstot;
  for(i = 0; i < nek+1; ++i)
    Bsys->edge[nes+i] += nvstot + nfstot + nvktot ;

  /* Scatter new local values back to vertex edge and face id's */
  for(E=EL->fhead;E;E=E->next){
    for(i = 0; i < E->Nverts; ++i)
      E->vert[i].gid = vmap[E->vert[i].gid];

    for(i = 0; i < E->Nedges; ++i)
      E->edge[i].gid = emap[E->edge[i].gid];
  }

  if(dim == 3){
    for(i = 0; i < nfs; ++i)
      Bsys->face[i] += nvstot + nestot;
    for(i = 0; i < nfk+1; ++i)
      Bsys->face[nfs+i] += nvstot + nestot + nvktot + nektot;

    for(E=EL->fhead;E;E=E->next)
      for(i = 0; i < E->Nfaces; ++i)
  E->face[i].gid = fmap[E->face[i].gid];
  }

  /* finally if parallel option then there is enough information in
     vmap,emap,fmap and gmap to define a global-local mapping for the
     boundary degrees of freedom */
#ifdef PARALLEL
  int j;
  Pllmap *p;
  /* set up mapping for solved points */
  p = Bsys->pll = (Pllmap*)malloc(sizeof(Pllmap));
  p->nv_solve = gmap->nvs;
  p->nsolve  = gmap->nsolve;
  p->nglobal = gmap->nglobal;

  /* set up solve map */
  p->solvemap = ivector(0,Bsys->nsolve-1);

  for(i = 0; i < gmap->nvs; ++i)
    if(vmap[i]+1)
      p->solvemap[vmap[i]] = gmap->vgid[i];

  for(i = 0; i < gmap->nes; ++i)
    if(emap[i]+1)
      for(j = 0; j < elen[i]; ++j)
  p->solvemap[Bsys->edge[emap[i]] + j] = gmap->egid[i] + j;

  for(i = 0; i < gmap->nfs; ++i)
    if(fmap[i]+1)
      for(j=0; j < flen[i]; ++j)
  p->solvemap[Bsys->face[fmap[i]]+j] = gmap->fgid[i] + j;

  /* extra mappings for ddot sums in Bsolve_CG */
  p->solve = gs_init(p->solvemap,Bsys->nsolve,option("GSLEVEL"));

  if(Bsys->nsolve){
    p->mult = dvector(0,Bsys->nsolve-1);
    dfill(Bsys->nsolve,1.,p->mult,1);
    gs_gop(p->solve, p->mult, "+");
    dvrecp(Bsys->nsolve,p->mult,1,p->mult,1);
  }

  if(Bsys->nglobal - Bsys->nsolve){
    int nloc, nglo;

    nloc = Bsys->nsolve;
    nglo = gmap->nsolve;

    p->knownmap = ivector(0,Bsys->nglobal - Bsys->nsolve);

    for(i = gmap->nvs; i < gmap->nvg; ++i)
      if(vmap[i]+1)
  p->knownmap[vmap[i]-nloc] = gmap->vgid[i]-nglo;

    for(i = gmap->nes; i < gmap->neg; ++i)
      if(emap[i]+1)
  for(j=0; j < elen[i]; ++j)
    p->knownmap[Bsys->edge[emap[i]]-nloc+j] = gmap->egid[i]+j -nglo;

    for(i = gmap->nfs; i < gmap->nfg; ++i)
      if(fmap[i]+1)
  for(j=0; j < flen[i]; ++j)
    p->knownmap[Bsys->face[fmap[i]]-nloc+j] = gmap->fgid[i]+j-nglo;

  }
  else
    p->knownmap = ivector(0,0);

  /* this must be called outside of if statement since even if there
     are no points all processors must send gs_init */

  p->known=gs_init(p->knownmap,Bsys->nglobal-Bsys->nsolve,option("GSLEVEL"));
#endif

  free(vmap); free(emap);  free(elen);
  if(dim == 3){
    free(fmap);
    free(flen);
  }
}

typedef struct vertnum {
  int    id;
  struct vertnum *base;
  struct vertnum *link;
} Vertnum;

#if 1
static void setGid(Element_List *UL){
  Element  *U = UL->fhead;
  register int i,j,k;
  int      nvg, face, eid, edgeid, faceid, vertmax;
  const    int nel = UL->nel;
  Edge     *e,*ed;
  Face     *f;
  Vert     *vb,*v,*vc;
  Element *E;

  /* set vector of consequative numbers */
  /* set up vertex list */

  if(U->dim() == 2){
    for(E = U; E; E = E->next)
      for(i = 0; i < E->Nedges; ++i){
  for(j = 0; j < E->dim(); ++j){

    v = E->vert + E->fnum(i,j);

    if(E->edge[i].base){
      if(E->edge[i].link){
        eid  = E->edge[i].link->eid;
        face = E->edge[i].link->id;
      }
      else{
        eid  = E->edge[i].base->eid;
        face = E->edge[i].base->id;
      }

      vb = UL->flist[eid]->vert[UL->flist[eid]->fnum1(face,j)].base;

      if(eid < E->id){  /* connect to lower element */
        if(!v->base) v->base = v;

        /* search through all points and assign to same base */
        for(;vb->link;vb = vb->link);
        if(vb->base != v->base) vb->link = v->base;
        for(v = v->base;v; v = v->link) v->base = vb->base;
      }
      else if(!v->base) v->base = v;
    }
    else if(!v->base) v->base = v;
  }
      }
  }

  if(U->dim() == 3){
    double x, y, z, x1, y1, z1, cx, cy, cz, TOL = 1E-5;
    int vn, vn1, flag, nfv;
    Element *F;

    for(E = U; E; E = E->next)
      for(i = 0; i < E->Nfaces; ++i){
  for(j = 0; j < E->Nfverts(i); ++j){
    vn = E->vnum(i,j);

    v = E->vert + vn;

    if(E->face[i].link){
      eid  = E->face[i].link->eid;
      face = E->face[i].link->id;
      F    = UL->flist[eid];

      nfv = F->Nfverts(face);

      x = E->vert[vn].x, y = E->vert[vn].y, z = E->vert[vn].z;

      cx = 0.0; cy = 0.0; cz = 0.0;
      for(k=0;k<nfv;++k){
        cx += F->vert[F->vnum(face,k)].x - E->vert[E->vnum(i,k)].x;
        cy += F->vert[F->vnum(face,k)].y - E->vert[E->vnum(i,k)].y;
        cz += F->vert[F->vnum(face,k)].z - E->vert[E->vnum(i,k)].z;
      }
      cx /= 1.*nfv;      cy /= 1.*nfv;      cz /= 1.*nfv;

      // loop through vertices on neighbour face
      // break out when match is made
      flag = 1;
      for(k=0;k < nfv;++k){
        vn1 = F->vnum(face,k);
        x1  = F->vert[vn1].x-cx;
        y1  = F->vert[vn1].y-cy;
        z1  = F->vert[vn1].z-cz;
        if(sqrt((x1-x)*(x1-x)+ (y1-y)*(y1-y) + (z1-z)*(z1-z)) < TOL){
    flag = 0;
    break;
        }
      }

      if(flag)
        fprintf(stderr, "Error in SetGid  Elmt:%d Face:%d Vertex:%d\n",
          E->id, i, j);

      vb = F->vert[vn1].base;

      /* connect to lower element */

      if(eid <= E->id){
        if(!v->base) v->base = v;

        /* search through all points and assign to same base */
        if(vb){
    for(;vb->link;vb = vb->link);
    if(vb->base != v->base) vb->link = v->base;
    for(v = v->base;v; v = v->link) v->base = vb->base;
        }
      }
      else if(!v->base) v->base = v;
    }
    else if(!v->base) v->base = v;
  }
      }
  }

  /* number vertices consequatively - start from 1 initially */
  for(E = U, nvg = 1; E; E = E->next)
    for(i = 0; i < E->Nverts; ++i)
      if(!E->vert[i].base->gid)
  E->vert[i].gid = E->vert[i].base->gid = nvg++;
      else
  E->vert[i].gid = E->vert[i].base->gid;
  nvg--;

  /* subtract off extra 1 value to make numbering start from zero */
  for(E = U; E; E = E->next)
    for(i = 0; i < E->Nverts; ++i)
      E->vert[i].gid -= 1;

  /* set gid's to -1 */
  for(E = U; E; E = E->next){
    for(i = 0; i < E->Nedges; ++i) E->edge[i].gid = -1;
    for(i = 0; i < E->Nfaces; ++i) E->face[i].gid = -1;
  }

  /* at present just number edge and faces consequatively */
  faceid = 0; edgeid = 0;
  for(E = U; E; E = E->next){
    e = E->edge;
    for(i = 0; i < E->Nedges; ++i){
      if(e[i].gid==-1)
  if(e[i].base){
    for(ed = e[i].base; ed; ed = ed->link)
      ed->gid = edgeid;
    ++edgeid;
  }
  else
    e[i].gid = edgeid++;
    }
    f = E->face;
    for(i = 0; i < E->Nfaces; ++i)
      if(f[i].gid==-1)
  if(f[i].link){
    f[i].gid       = faceid;
    f[i].link->gid = faceid++;
  }
  else
    f[i].gid       = faceid++;
  }

  return;
}
#else
static void setGid(Element_List *UL){
  Element  *U = UL->fhead;
  register int i,j,k;
  int      nvg, face, eid, edgeid, faceid, vertmax;
  const    int nel = UL->nel;
  Edge     *e,*ed;
  Face     *f;
  Vertnum  *V,*vb,*v,*vc;

  Element *E;

  /* set vector of consequative numbers */
  /* set up vertex list */
  vertmax = Max_Nverts;

  V = (Vertnum *) calloc(vertmax*nel,sizeof(Vertnum));

  if(U->dim() == 2){
    for(E = U; E; E = E->next)
      for(i = 0; i < E->Nedges; ++i){
  for(j = 0; j < E->dim(); ++j){

    v = V+E->id*vertmax + E->fnum(i,j);

    if(E->edge[i].base){
      if(E->edge[i].link){
        eid  = E->edge[i].link->eid;
        face = E->edge[i].link->id;
      }
      else{
        eid  = E->edge[i].base->eid;
        face = E->edge[i].base->id;
      }

      vb = V[eid*vertmax + UL->flist[eid]->fnum1(face,j)].base;

      if(eid < E->id){  /* connect to lower element */
        if(!v->base) v->base = v;

        /* search through all points and assign to same base */
        for(;vb->link;vb = vb->link);
        if(vb->base != v->base) vb->link = v->base;
        for(v = v->base;v; v = v->link) v->base = vb->base;
      }
      else if(!v->base) v->base = v;
    }
    else if(!v->base) v->base = v;
  }
      }
  }

  if(U->dim() == 3){
    double x, y, z, x1, y1, z1, cx, cy, cz, TOL = 1E-5;
    int vn, vn1, flag, nfv;
    Element *F;

    for(E = U; E; E = E->next)
      for(i = 0; i < E->Nfaces; ++i){
  for(j = 0; j < E->Nfverts(i); ++j){
    vn = E->vnum(i,j);

    v = V+E->id*vertmax + vn;

    if(E->face[i].link){
      eid  = E->face[i].link->eid;
      face = E->face[i].link->id;
      F    = UL->flist[eid];

      nfv = F->Nfverts(face);

      x = E->vert[vn].x, y = E->vert[vn].y, z = E->vert[vn].z;

      cx = 0.0; cy = 0.0; cz = 0.0;
      for(k=0;k<nfv;++k){
        cx += F->vert[F->vnum(face,k)].x - E->vert[E->vnum(i,k)].x;
        cy += F->vert[F->vnum(face,k)].y - E->vert[E->vnum(i,k)].y;
        cz += F->vert[F->vnum(face,k)].z - E->vert[E->vnum(i,k)].z;
      }
      cx /= 1.*nfv;      cy /= 1.*nfv;      cz /= 1.*nfv;

      // loop through vertices on neighbour face
      // break out when match is made
      flag = 1;
      for(k=0;k < nfv;++k){
        vn1 = F->vnum(face,k);
        x1  = F->vert[vn1].x-cx;
        y1  = F->vert[vn1].y-cy;
        z1  = F->vert[vn1].z-cz;
        if(sqrt((x1-x)*(x1-x)+ (y1-y)*(y1-y) + (z1-z)*(z1-z)) < TOL){
    flag = 0;
    break;
        }
      }

      if(flag)
        fprintf(stderr, "Error in SetGid  Elmt:%d Face:%d Vertex:%d\n",
          E->id, i, j);

      vb = V[eid*vertmax + vn1].base;

      /* connect to lower element */

      if(eid <= E->id){
        if(!v->base) v->base = v;

        /* search through all points and assign to same base */
        if(vb){
    for(;vb->link;vb = vb->link);
    if(vb->base != v->base) vb->link = v->base;
    for(v = v->base;v; v = v->link) v->base = vb->base;
        }
      }
      else if(!v->base) v->base = v;
    }
    else if(!v->base) v->base = v;
  }
      }
  }

  /* number vertices consequatively */
  for(E = U, nvg = 1; E; E = E->next)
    for(i = 0; i < E->Nverts; ++i)
      if(!V[E->id*vertmax+i].base->id)
  V[E->id*vertmax+i].id = V[E->id*vertmax+i].base->id = nvg++;
      else
  V[E->id*vertmax+i].id = V[E->id*vertmax+i].base->id;
  nvg--;

  for(E = U; E; E = E->next)
    for(i = 0; i < E->Nverts; ++i)
      E->vert[i].gid = V[E->id*vertmax+i].id-1;

  /* set gid's to -1 */
  for(E = U; E; E = E->next){
    for(i = 0; i < E->Nedges; ++i) E->edge[i].gid = -1;
    for(i = 0; i < E->Nfaces; ++i) E->face[i].gid = -1;
  }

  /* at present just number edge and faces consequatively */
  faceid = 0; edgeid = 0;
  for(E = U; E; E = E->next){
    e = E->edge;
    for(i = 0; i < E->Nedges; ++i){
      if(e[i].gid==-1)
  if(e[i].base){
    for(ed = e[i].base; ed; ed = ed->link)
      ed->gid = edgeid;
    ++edgeid;
  }
  else
    e[i].gid = edgeid++;
    }
    f = E->face;
    for(i = 0; i < E->Nfaces; ++i)
      if(f[i].gid==-1)
  if(f[i].link){
    f[i].gid       = faceid;
    f[i].link->gid = faceid++;
  }
  else
    f[i].gid       = faceid++;
  }

  free(V);
  return;
}
#endif
static int suminterior(Element *E){
  int sum=0;
  for(;E;E = E->next)
    sum += E->Nmodes - E->Nbmodes;

  return sum;
}

static void set_bmap(Element *U, Bsystem *B){
  register int i,j,k,n;
  int   l;
  const int nel = B->nel;
  int  **bmap;
  Element *E;

  /* declare memory */
  bmap = (int **) malloc(nel*sizeof(int *));
  for(E=U,l=0;E;E=E->next) l += E->Nbmodes;

  bmap[0] = ivector(0,l-1);
  for(i = 0, E=U; i < nel-1; ++i, E=E->next)
    bmap[i+1] = bmap[i] + E->Nbmodes;

  /* fill with bmaps */
  for(E=U;E;E=E->next){
    for(j = 0; j < E->Nverts; ++j)
      bmap[E->id][j] = E->vert[j].gid;

    for(j = 0,n = E->Nverts; j < E->Nedges; ++j,n+=l){
      l = E->edge[j].l;
      for(k = 0; k < l; ++k)
        bmap[E->id][n+k] = B->edge[E->edge[j].gid] + k;
    }

    if(E->dim() == 3)
      for(k = 0; k < E->Nfaces; ++k){
  l = E->face[k].l;

  if(E->Nfverts(k) == 3){ // triangle face
    l = l*(l+1)/2;
    for(i = 0; i < l; ++i)
      bmap[E->id][n+i] = B->face[E->face[k].gid] + i;
    n += l;
  }
  else{  // square face
    if(E->face[k].con < 4){
      for(i=0;i<l;++i)
        for(j=0;j<l;++j)
    bmap[E->id][n+j+i*l] = B->face[E->face[k].gid]+j+i*l;
    }
    else{ // transpose modes
      for(i=0;i<l;++i)
        for(j=0;j<l;++j)
    bmap[E->id][n+i+j*l] = B->face[E->face[k].gid]+j+i*l;
      //      E->face[k].con -= 4;
    }
    n += l*l;
  }
      }
  }

  B->bmap = bmap;
}

void free_Mesh_Facets(Element_List *Mesh){
  Element *E;
  Curve    *cur;

  // free vertices
  for(E=Mesh->fhead;E;E=E->next){
    free(E->vert);
    free(E->edge);
    free(E->face);
    if(E->curve){
      cur = E->curve->next;
      free(E->curve);
      E->curve = cur;
    }
    if(E->curvX)
      free(E->curvX);
  }
}

void free_Mesh_Structure(Element_List *Mesh){

   Element *E,*F;

   // free vertices
   for(E=Mesh->fhead;E;){
     F = E->next;
     delete(E);
     E = F;
   }

   free(Mesh->flist);
   delete(Mesh);
}

void free_Mesh_Bcs(Bndry *MeshBcs){
  Bndry *aBc, *Bc;

  for(Bc=MeshBcs;Bc;){
    aBc = Bc->next;
    free(Bc);
    Bc = aBc;
  }
}



/* this function resets the global mesh and boundary conditions so that
   they are correctly set for the velocity */

void Reflect_Global_Velocity(Element_List *Mesh, Bndry *Meshbcs, int dir){
  register int i,j,id;
  Element *U;
  Bndry   *B;
  int flag = 0;
  int active_handle = get_active_handle();


  /* reset solve mask */
  for(U = Mesh->fhead; U; U = U->next)
    for(i = 0; i < U->Nverts; ++i)
      U->vert[i].solve = 1;

  DO_PARALLEL{ /* set interface solve masks to 2 */
    if((Mesh->fhead->dim() == 3)&&(pllinfo[active_handle].partition)){
      for(U = Mesh->fhead; U; U = U->next)
  for(i = 0; i < U->Nfaces; ++i)
    if(U->face[i].link)
      if(pllinfo[active_handle].partition[U->id]
         != pllinfo[active_handle].partition[U->face[i].link->eid])
        for(j = 0; j < U->Nfverts(i); ++j){
    id = U->vnum(i,j);
    U->vert[id].solve *= 2;
    //clamp for mult calls
    U->vert[id].solve = min(U->vert[id].solve,2);
        }
    }
  }

  /* Loop through boundary conditions and set Dirichlet boundaries */
  for(B = Meshbcs; B; B = B->next){
    switch (B->usrtype) {
    case 'V': case 'v':
    case 'W':
      B->type = 'V';
      U = B->elmt;
      for(i = 0; i < U->Nfverts(B->face); ++i)
  U->vert[U->vnum(B->face,i)].solve = 0;
      break;
    case 'Z':
      U = B->elmt;
      if(B->bvert[0] == dir){
  B->type = 'V';
  for(i = 0; i < U->Nfverts(B->face); ++i)
    U->vert[U->vnum(B->face,i)].solve = 0;
      }
      else
  B->type = 'F';
      break;
    }
  }
}


/* replace vertex, edge and face numbering using the global numbering
   scheme specified in GU */

void Replace_Numbering(Element_List *UL, Element_List *GUL){
  Element *U, *GU;
  int i,k;
  int active_handle = get_active_handle();

  for(k = 0; k < UL->nel; ++k){
    U  = UL->flist[k];
    GU = GUL->flist[pllinfo[active_handle].eloop[k]];
    for(i = 0; i < U->Nverts; ++i){
      U->vert[i].gid   = GU->vert[i].gid;
      U->vert[i].solve = GU->vert[i].solve;
    }

    for(i = 0; i < U->Nedges; ++i)
      U->edge[i].gid   = GU->edge[i].gid;

    for(i = 0; i < U->Nfaces; ++i)
      U->face[i].gid   = GU->face[i].gid;
  }
}
