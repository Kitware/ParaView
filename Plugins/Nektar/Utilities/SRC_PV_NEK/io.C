/*
 * Input functions based on the NEKTON spectral element data model
 */
/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $RCSfile: io.C,v $
 * $Revision: 1.2 $
 * $Author: ssherw $
 * $Date: 2006/05/08 14:18:48 $
 * $State: Exp $
 * ------------------------------------------------------------------------- */
#include "nektar.h"

#include <cstdio>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <cmath>

#include <veclib.h>
#include "Quad.h"
#include "Tri.h"
#include "Tet.h"
#include "Hex.h"
#include "Prism.h"
#include "Pyr.h"

#include "free_surface.C"

/* .....  Section names ..... */
#define  SEC_PARAMS   sections[0]             /* Parameters                 */
#define  SEC_PSCALS   sections[1]             /* Passive scalars            */
#define  SEC_LOGICS   sections[2]             /* Logical switches           */
#define  SEC_MESH     sections[3]             /* Mesh definition            */
#define  SEC_CURVE    sections[4]             /* Curved sides               */
#define  SEC_BCS      sections[5]             /* Boundary conditions        */
#define  SEC_ICS      sections[6]             /* Initial conditions         */
#define  SEC_DF       sections[7]             /* Drive force                */
#define  SEC_HIST     sections[8]             /* History points             */
#define  SEC_DFUNCS   sections[10]            /* Drive force                */

static int   checklist   (char *name, char *list[]);
static char *findSection (char *name, char *buf, FILE *fp);
static int   bedgcmp     (const void *b1, const void *b2);


static char *sections[] = {
  "Paramaters",  "Passive Scalars",     "LOGICAL",     "MESH", "Curved sides",
  "BOUNDARY",    "INITIAL CONDITIONS",  "DRIVE FORCE",  "HISTORY", "THERMAL",
  "DRIVE FUNCT"};

/* ------------------------------------------------------------------------ *
 * Spectral Element Data Model                                              *
 *                                                                          *
 * This file contains functions to read a spectral element input file in    *
 * the NEKTON format.  The file format is fairly specific, expecting the    *
 * following sections in order:                                             *
 *                                                                          *
 *     PARAMETERS -> Passive scalars -> Logical switches -> MESH      ->    *
 *     BC's       -> INITIAL COND.'s -> DRIVE FORCE      -> Variables ->    *
 *     HISTORY POINTS                                                       *
 *                                                                          *
 * The uppercase sections are the ones which are actually used.  In the     *
 * parameters section, the following should be defined if you are solving   *
 * Stokes or Navier-Stokes:                                                 *
 *                                                                          *
 *     KINVIS              Viscosity, or 1/Re                               *
 *     NSTEP | FINTIME     Number of steps or simulation time.              *
 *     DELT                Time step                                        *
 *                                                                          *
 * Return value: none                                                       *
 * ------------------------------------------------------------------------ */

void ReadParams (FILE *fp)
{
  int  n;
  char buf[BUFSIZ], value[25], name[25], c;
  double val, dt;

  static char *dspecial[] = { "KINVIS", "LAMBDA", "IOTIME", "KC", "LZ",
                               "FRNDX","FRNDY","FRNDZ" , "PECLET", 0 };
  static char *ispecial[] = { "TEQN", "EQTYPE", "CFLSTEP", "HISSTEP", 0 };


  rewind (fp);
  for (n = 0; n < 4; n++) fgets(buf, BUFSIZ, fp);

  if (sscanf(buf, "%d", &n) != 1)
    {fputs("ReadParams: can't read # of parameters", stderr);exit(-1);}

  while (n--) {
    fgets (buf, BUFSIZ, fp);
    if(sscanf(buf, "%25s%25s", value, name) == 2) {
      if (checklist (name, dspecial))
  dparam_set (name, scalar(value));
      else if (checklist (name, ispecial))
  iparam_set (name, (int) scalar(value));
      else if (isupper(c = *name) && 'I' <= c && c <= 'N')
  iparam_set(name, (int) scalar(value));
      else
  dparam_set(name, scalar(value));
    }
  }


  /* The following section is only for unsteady problems */

  if (!option("scalar")) {
    if ((dt=dparam("DELT")) == 0.) {
      if ((dt=dparam("DT")) != 0.)
  dparam_set("DELT", dt);
      else
  {fputs("ReadParams: no time step specified", stderr);exit(-1);}
    }

    /* FINTIME overrides NSTEPS */

    if ((val=dparam("FINTIME")) > 0.)
      iparam_set("NSTEPS", n =  (int) ((val+.5*dt)/dt));
    else
      dparam_set("FINTIME", val = (n=iparam("NSTEPS"))*dt);

    /* IOTIME overrides IOSTEP */

    n   = clamp(iparam("IOSTEP"), 0 , n);
    val = clamp(dparam("IOTIME"), 0., val);

    if (val == 0. && n == 0) {
      n   = iparam("NSTEPS");
      val = dparam("FINTIME");
    } else if (val > 0.)
      n   = (int) ((val + .5 * dt) / dt);
    else
      val = n * dt;

    iparam_set("IOSTEP", n);
    dparam_set("IOTIME", val);
  }

  /* check to see if there is a command line input for norder */
  /* set modes to be equal to norder if it is not already set */

  if(n = option("NORDER.REQ")) iparam_set("MODES",n);
  if(!iparam("MODES")        ) iparam_set("MODES",iparam("NORDER"));

  return;
}

static int checklist (char *name, char *list[])
{
  do
    if (strcmp(name,*list) == 0)
      return 1;
  while
    (*++list);

  return 0;
}

/* The next two functions are optional */

void ReadPscals (FILE *fp)
{
  int  n;
  char buf[BUFSIZ];

  fgets(buf, BUFSIZ, fp);
  if (sscanf(buf,"%d", &n) != 1)
    {fputs("ReadPscals: no passive scalar data", stderr);exit(-1);}

  while (n--) fgets(buf, BUFSIZ, fp);
  return;
}

void ReadLogics (FILE *fp)
{
  int  n;
  char buf[BUFSIZ];

  fgets(buf, BUFSIZ, fp);
  if (sscanf(buf, "%d", &n) != 1 || !strstr(buf, SEC_LOGICS))
    {fputs("ReadLogics: no logical switches found\n", stderr); exit(-1);}

  while (n--) fgets(buf, BUFSIZ, fp);
  return;
}

static void ReadCurve(FILE *fp, Element_List *new_E);
void  ReadOrderFile(char *name, Element_List *E);

Element_List *ReadMesh (FILE *fp, char* session_name)
{
  int     nel, L, qa, qb, qc=0, k, fdim;
  char    buf[BUFSIZ];
  char    buf_a[BUFSIZ];
  Element **new_E;
  register int i;
  double  xscal = dparam("XSCALE");
  double  yscal = dparam("YSCALE");
  double  zscal = dparam("ZSCALE");
  double  xmove = dparam("XMOVE");
  double  ymove = dparam("YMOVE");
  double  zmove = dparam("ZMOVE");

  option_set("NZ",1);
  option_set("NZTOT",1);

  /* set up modes and quadrature points */

  if(!( L = iparam("MODES")))
    {fputs("ReadMesh: Number of modes not specified\n",stderr);exit(-1);}

  /* note quadrature order reset for variable order runs */
  if(qa = iparam("LQUAD"));
  else qa = L + 1;

  if(qb = iparam("MQUAD"));
  else qb = L;

  if (!findSection (SEC_MESH, buf, fp))
    {fputs("ReadMesh: Section not found\n", stderr); exit(-1);}

  if (sscanf(fgets(buf,BUFSIZ,fp), "%d%d", &nel,&fdim) != 2)
   {fputs("ReadMesh: unable to get the number of elements\n",stderr);exit(1);}

  //ROOTONLY
  //fprintf(stdout, "ReadMesh: File is %d dimensional\n", k);

  iparam_set("ELEMENTS", nel);
  /* Set up a new element vector */
  QGmax = max(max(qa,qb),qc);
  LGmax = L;
  init_mod_basis();

  new_E = (Element**) malloc(nel*sizeof(Element*));

  Coord X;
  X.x = dvector(0,Max_Nverts-1);
  X.y = dvector(0,Max_Nverts-1);
  X.z = dvector(0,Max_Nverts-1);

  /* Read in mesh information */
  for(k = 0; k < nel; k++) {
    fgets(buf, BUFSIZ, fp);  /* element header */

    if(strstr(buf,"Tet") || strstr(buf,"tet")){
      new_E[k]   = new       Tet(k,'u', L, qa, qb, qb, &X);
    }
    else if(strstr(buf,"Hex") || strstr(buf,"hex")){
      new_E[k]   = new       Hex(k,'u', L, qa, qa, qa, &X);
    }
    else if(strstr(buf,"Prism") || strstr(buf,"prism")){
      new_E[k]   = new       Prism(k,'u', L, qa, qa, qb, &X);
    }
    else if(strstr(buf,"Pyr") || strstr(buf,"pyr")){
      new_E[k]   = new       Pyr(k,'u', L, qa, qa, qb, &X);
    }
    else if(strstr(buf,"Qua") || strstr(buf,"qua")){
  new_E[k] = new       Quad(k,'u', L, qa, qa,  0, &X);
    }
    else{
      if(fdim == 2)
  new_E[k] = new       Tri (k,'u', L, qa, qb,  0, &X);
      else
  new_E[k] = new       Tet(k,'u', L, qa, qb, qb, &X);
    }

    /* -X- Coordinates */
    for (i = 0; i < new_E[k]->Nverts; i++){
      fscanf (fp, "%lf", &(new_E[k]->vert[i].x));
      if(xscal)  new_E[k]->vert[i].x *= xscal;
      if(xmove)  new_E[k]->vert[i].x += xmove;
    }


    fgets(buf_a, BUFSIZ, fp);  /* finish the line */

    /* -Y- Coordinates */
    for (i = 0; i < new_E[k]->Nverts; i++){
      fscanf (fp, "%lf", &(new_E[k]->vert[i].y));
      if(yscal)  new_E[k]->vert[i].y *= yscal;
      if(ymove)  new_E[k]->vert[i].y += ymove;
    }

    fgets(buf_a, BUFSIZ, fp);  /* finish the line */

    if(new_E[k]->dim() == 3){
      /* -Z- Coordinates */
      for (i = 0; i < new_E[k]->Nverts; i++){
  fscanf (fp, "%lf", &(new_E[k]->vert[i].z));
  if(zscal)  new_E[k]->vert[i].z *= zscal;
  if(zmove)  new_E[k]->vert[i].z += zmove;
      }
      fgets(buf_a, BUFSIZ, fp);  /* finish the line */
    }
  }

  for(k = 0; k < nel-1; ++k)     new_E[k]->next = new_E[k+1];
  new_E[k]->next = (Element*) NULL;

  Element_List* E_List = (Element_List*) new Element_List(new_E, nel);

  if(option("variable"))
    ReadOrderFile (session_name,E_List);


  /* Read the curved side information */
  ReadCurve   (fp,E_List);
  //fprintf(stderr, "io.C: ReadMesh: after ReadCurve()\n");
  ReadSetLink (fp,E_List);
  //fprintf(stderr, "io.C: ReadMesh: after ReadSetLink()\n");
  for(k = 0; k < nel; ++k)
    new_E[k]->set_curved_elmt(E_List);

  //fprintf(stderr, "io.C: ReadMesh: after set_curved_elmt loop()\n");
  Free_Felisa_data(); /* free's up felisa data if declared */
  //fprintf(stderr,"io.C: ReadMesh: after Free_Felisa_data()\n");
  return E_List;
}

/* functions assosiated with "Free" (parametric) curved boundaries */
void get_nvw_per_DB(FILE *pFile, int index_DB, int *nvc, int *nwc);

/*  functions assosiated with "Free" curved elements */
void get_nvw_per_DB(FILE *pFile, int index_DB, int *nvc, int *nwc){
  int i,tmp,nw,nv;

  rewind(pFile);
  fscanf(pFile, "%d ", &tmp);
  for (i = 0; i <= index_DB; i++){
    fscanf(pFile, "%d %d %d",&nw,&nv,&tmp);
    nvc[0] = nv;
    nwc[0] = nw;
  }
  rewind(pFile);
}


static void ReadCurve(FILE *fp, Element_List *new_E){
//fprintf(stderr, "io.C: ReadCurve: ENTER\n");
  register int i,j;
  int      k;
  int    eid,face;
  char   type, *p, buf[BUFSIZ];
  Curve  *curve,*ctmp;
  double fac;


  i = 2; while (i--) fgets (buf, BUFSIZ, fp);
  if(strstr(buf,"type")){
    int    ntags;
    char   *tagid;

    sscanf(buf, "%d", &ntags);

    if(ntags){
      ctmp  = (Curve *) calloc(ntags,sizeof(Curve));
      tagid = (char *)  malloc((ntags+1)*sizeof(char));

      /* read in curved info */
      for(i = 0; i < ntags; ++i){
  fgets (buf, BUFSIZ, fp);

  if(strstr(buf,"Str")||strstr(buf,"str")){  /* straight sided */
    fgets (buf, BUFSIZ, fp);
    sscanf(buf,"%1s",tagid+i);
    ctmp[i].type = T_Straight;
  }
        else if(strstr(buf,"Cir")||strstr(buf,"cir")){ /* circle */
    fgets  (buf, BUFSIZ, fp);
    sscanf (buf, "%*lf%*lf%lf%1s",&(ctmp[i].info.arc.radius),tagid+i);
    ctmp[i].type = T_Arc;
  }
        else if(strstr(buf,"Rec")||strstr(buf,"rec")){
          fgets (buf, BUFSIZ, fp);
          sscanf(buf, "%d%1s", &(ctmp[i].info.recon.forceint), tagid+i);
          ctmp[i].type = T_Recon;
        }
  else if(strstr(buf,"Cyl")||strstr(buf,"cyl")){ /* cylinder */
    fgets (buf, BUFSIZ, fp);
    sscanf (buf, "%lf%lf%lf%lf%lf%lf%lf%1s",
      &(ctmp[i].info.cyl.xc), &(ctmp[i].info.cyl.yc),
      &(ctmp[i].info.cyl.zc), &(ctmp[i].info.cyl.ax),
      &(ctmp[i].info.cyl.ay), &(ctmp[i].info.cyl.az),
      &(ctmp[i].info.cyl.radius),tagid+i);
    /* normalise ax,ay,az */
    fac = sqrt(ctmp[i].info.cyl.ax*ctmp[i].info.cyl.ax +

         ctmp[i].info.cyl.ay*ctmp[i].info.cyl.ay +
         ctmp[i].info.cyl.az*ctmp[i].info.cyl.az);
    ctmp[i].info.cyl.ax /=fac;
    ctmp[i].info.cyl.ay /=fac;
    ctmp[i].info.cyl.az /=fac;
    ctmp[i].type = T_Cylinder;
  }
  else if(strstr(buf,"Con")||strstr(buf,"con")){  /* cone */
    fgets (buf, BUFSIZ, fp);
    sscanf (buf, "%lf%lf%lf%lf%lf%lf%lf%1s",
      &(ctmp[i].info.cone.xc), &(ctmp[i].info.cone.yc),
      &(ctmp[i].info.cone.zc), &(ctmp[i].info.cone.ax),
      &(ctmp[i].info.cone.ay), &(ctmp[i].info.cone.az),
      &(ctmp[i].info.cone.alpha),tagid+i);
    /* normalise ax,ay,az */
    fac = sqrt(ctmp[i].info.cone.ax*ctmp[i].info.cone.ax +
         ctmp[i].info.cone.ay*ctmp[i].info.cone.ay +
         ctmp[i].info.cone.az*ctmp[i].info.cone.az);
    ctmp[i].info.cone.ax /=fac;
    ctmp[i].info.cone.ay /=fac;
    ctmp[i].info.cone.az /=fac;
    ctmp[i].type = T_Cone;
  }
  else if(strstr(buf,"Spi")||strstr(buf,"spi")){ /* cylinder */
    fgets (buf, BUFSIZ, fp);
    sscanf (buf, "%lf%lf%lf%lf%lf%lf%lf%lf%lf%lf%1s",
      &(ctmp[i].info.spi.xc), &(ctmp[i].info.spi.yc),
      &(ctmp[i].info.spi.zc), &(ctmp[i].info.spi.ax),
      &(ctmp[i].info.spi.ay), &(ctmp[i].info.spi.az),
      &(ctmp[i].info.spi.axialradius),
      &(ctmp[i].info.spi.piperadius),
      &(ctmp[i].info.spi.pitch),
      &(ctmp[i].info.spi.zerotwistz),tagid+i);

    /* normalise ax,ay,az */
    fac = sqrt(ctmp[i].info.spi.ax*ctmp[i].info.spi.ax +
         ctmp[i].info.spi.ay*ctmp[i].info.spi.ay +
         ctmp[i].info.spi.az*ctmp[i].info.spi.az);
    ctmp[i].info.spi.ax /= fac;
    ctmp[i].info.spi.ay /= fac;
    ctmp[i].info.spi.az /= fac;
    ctmp[i].type = T_Spiral;
  }
  else if(strstr(buf,"Sph")||strstr(buf,"sph")){ /* sphere */
    fgets (buf, BUFSIZ, fp);
    sscanf (buf, "%lf%lf%lf%lf%1s",
      &(ctmp[i].info.sph.xc), &(ctmp[i].info.sph.yc),
      &(ctmp[i].info.sph.zc), &(ctmp[i].info.sph.radius),tagid+i);
    ctmp[i].type = T_Sphere;
  }
        else if(strstr(buf,"Fre")||strstr(buf,"fre")){ //LG: Parametric surface
      //fprintf(stderr, "io.C: else if(strstr(buf,Fre)||strstr(buf,fre)) ENTER\n");

         int index_DB, Vperiodic,Wperiodic,nvc,nwc;
          char *label,*Fname;
          FILE *DB_File;

          label = (char*) calloc(BUFSIZ, sizeof(char));
          Fname = (char*) calloc(BUFSIZ, sizeof(char));

          fgets (buf, BUFSIZ, fp);
          sscanf(buf,"%d %d %d %1s %s",&index_DB,
                 &Vperiodic,&Wperiodic,tagid+i,Fname);


          index_DB = index_DB-1;
          DB_File = fopen(Fname,"r");
          get_nvw_per_DB(DB_File,index_DB,&nvc,&nwc);
          ctmp[i].info.free.nvc = nvc;
          ctmp[i].info.free.nwc = nwc;
          ctmp[i].info.free.Vperiodic = Vperiodic;
          ctmp[i].info.free.Wperiodic = Wperiodic;
          ctmp[i].info.free.tag = *(tagid+i);
          ctmp[i].info.free.allocate_memory();
    //fprintf(stderr, "io.C: BEFORE: ctmp[i].info.free.load_from_grdFile(index_DB,DB_File)\n");
          ctmp[i].info.free.load_from_grdFile(index_DB,DB_File);
    //fprintf(stderr, "io.C: AFTER: ctmp[i].info.free.load_from_grdFile(index_DB,DB_File)\n");
          ctmp[i].info.free.set_1der();
          ctmp[i].info.free.set_2der();
          ctmp[i].type = T_Free;

          fclose(DB_File);

          delete[] label;
          delete[] Fname;

    //fprintf(stderr, "io.C: else if(strstr(buf,Fre)||strstr(buf,fre)) EXIT\n");
        }
  else if(strstr(buf,"Fil")||strstr(buf,"fil")){ /* Felisa file input */
    char file[BUFSIZ];
    fgets  (buf, BUFSIZ, fp);
    sscanf (buf, "%s",file);
    ctmp[i].info.file.name = strdup(file);
    sscanf (buf, "%*s%1s",tagid+i);
    ctmp[i].type = T_File;
  }
  else if(strstr(buf,"Nac")||strstr(buf,"nac")){ /* Naca 4 series */
    char file[BUFSIZ];
    fgets  (buf, BUFSIZ, fp);
    sscanf (buf, "%lf%lf%lf%lf%lf%1s",&(ctmp[i].info.nac.xl),
      &(ctmp[i].info.nac.yl),&(ctmp[i].info.nac.xt),
      &(ctmp[i].info.nac.yt), &(ctmp[i].info.nac.thickness),
      tagid+i);
    ctmp[i].type = T_Naca4;
  }
      }

      fgets (buf, BUFSIZ, fp);
      sscanf(buf, "%d", &k);

      if (k) {
  if(!ntags) error_msg(No curved type information specified);

      for(i = 0; i < k; ++i){
     /* for some reason Linux doesn't read in first integer
       properly so am using a double read followed by a type cast */
    if (fscanf(fp, "%lf%d%1s", &fac, &eid, &type) != 3)
      error_msg(ReadMesh -- unable to read curved information);
    face = (int) fac;

    curve = (Curve *) calloc(1,sizeof(Curve));

    for(j = 0; j < ntags; ++j)
      if(tagid[j] == type){
        memcpy(curve,ctmp+j,sizeof(Curve));
        break;
      }
    if(j == ntags){
        fprintf(stderr,"can not find tag type `%c\' called in element"
        " %d face %d\n" ,type,eid+1,face);
      exit(-1);
    }

          if( curve->type == T_Recon){
            fscanf(fp, "%lf%lf%lf", curve->info.recon.vn0, curve->info.recon.vn1, curve->info.recon.vn2);
            fscanf(fp, "%lf%lf%lf", curve->info.recon.vn0+1, curve->info.recon.vn1+1, curve->info.recon.vn2+1);
            fscanf(fp, "%lf%lf%lf", curve->info.recon.vn0+2, curve->info.recon.vn1+2, curve->info.recon.vn2+2);
          }

          curve->next = (Curve *)NULL;
          curve->id = i;
          new_E->flist[--eid]->curve = curve;

    //curve->next = new_E->flist[--eid]->curve;
    //new_E->flist[eid]->curve = curve;

    /* read in Felisa vertex system */
    if(curve[0].type == T_File && new_E->fhead->dim() == 3){
      fscanf(fp,"%d%d%d",curve[0].info.file.vert,
       curve[0].info.file.vert+1, curve[0].info.file.vert+2);
      curve[0].info.file.vert[0]--;
      curve[0].info.file.vert[1]--;
      curve[0].info.file.vert[2]--;
    }

    curve[0].face    = --face;
  }
      }
      if(ntags){
  free(ctmp); free(tagid);
      }
    }
  }
  else{

    sscanf(buf, "%d", &k);

    if (k) {
      curve =  (Curve *) calloc(k,sizeof(Curve));

      for(i = 0; i < k; ++i){
  /* for some reason Linux doesn't read in first integer
     properly so am using a double read followed by a type cast */
  fgets(buf, BUFSIZ, fp);
  if (sscanf(buf, "%d%d", &face, &eid) != 2)
    error_msg(ReadMesh -- unable to read curved information);

  curve[i].next = new_E->flist[--eid]->curve;
  new_E->flist[eid]->curve = curve + i;
  curve[i].face    = --face;

  p    = buf + strlen(buf); while (isspace(*--p));
  type = toupper(*p);

  switch (type) {
  case 'S':                                   /* Straight side */
    curve[i].type = T_Straight;
    break;
  case 'C':                                   /* Circular arc */
    sscanf (buf, "%*d%*d%lf",&(curve[i].info.arc.radius));
    curve[i].type = T_Arc;
    break;
  default:
    sprintf (buf, "unknown curve type -- %c", type);
    fputs(buf,stderr);
    exit(-1);
    break;
  }
      }
    }
  }
//fprintf(stderr, "io.C: ReadCurve: EXIT\n");
}

static char *findSection (char *name, char *buf, FILE *fp)
{
  char *p;

  while (p = fgets (buf, BUFSIZ, fp))
    if (strstr (p, name))
      break;

  return p;
}



/* ----------------------------------------------------------------------- *
 * ReadICs() - Read Initial Conditions                                     *
 *                                                                         *
 * This function reads in the initial conditions for the velocity fields   *
 * from the .rea file.  The form of the velocity initial conditions is     *
 * determined by one of the following keywords in the line following the   *
 * beginning of the INITIAL CONDITIONS section:                            *
 *                                                                         *
 *     Default            Initial velocity field is at rest                *
 *     Restart            File name to read IC's from is given             *
 *     Given              V(x,y,z) (t = 0) specified                       *
 * ----------------------------------------------------------------------- */
static void Restart (char *name, int nfields, Element_List *U[],
         Element_List *Mesh);

void ReadICs_P (FILE *fp, Domain *omega, Element_List *Mesh)
{
  register int i;
  int      n, type, nfields, tot;
  char     buf[BUFSIZ];
  Element_List *U [MAXFIELDS];
  static  char *ictypes[] = { "PrepOnly", "Restart", "Default", "Given",
            "Hardwired", "Minion"};

  /* initialize the Element pointers */

  U[0] = omega->U; U[1] = omega->V; U[2] = omega->W; U[3] = omega->P;
  nfields = omega->U->fhead->dim();

  /* zero pressure fields */
  U[3]->zerofield();

  if (!findSection (SEC_ICS, buf, fp))
    {fprintf(stderr,"ReadICs: no initial conditions specified "
       "(last line read below) %s", buf); exit(-1);}
  if (sscanf(buf, "%d", &n) != 1)
    {fprintf(stderr,"ReadICs: # of initial conditions not specified "
       "(last line read below) %s", buf);exit(-1);}

  if (option("prep"))              /* Preprocessor */
    type = 0;
  else if (n == 0)                 /* Default      */
    type = 2;
  else {                           /* Find it...   */
    n--; fgets (buf, BUFSIZ, fp);
    for (i = 1; i < 6; i++)
      if (strstr(buf, ictypes[type = i])) break;
  }

  switch (type) {
  case 0:
    break;                      /* Pre-processor only */
  case 1: {                           /* Restart from a file */
    fgets   (buf, BUFSIZ, fp); n--;
    Restart (buf, nfields+1, U, Mesh);

    break;
  }
  case 2: {                           /* Default Initial Conditions */
    for (i = 0; i < nfields+1; i++){
      U[i]->zerofield();
      set_state(U[i]->fhead,'t');
    }
    ROOTONLY printf("Initial solution   : Default\n");
    break;
  }
  case 3: {                          /* Function(x,y,z) Initial conditions */
    char    *s;

    ROOTONLY printf("Initial solution   :");
    for (i = 0; i < nfields && n--; i++) {
      if(!(s = strchr(fgets(buf,BUFSIZ,fp),'=')))
  {fprintf(stderr,"ReadICs: the following function is invalid %s",s);
   exit(-1);}
      while (isspace(*++s));

      U[i]->Set_field(s);
      ROOTONLY {
  if(!i) printf (" %c = %s", U[i]->fhead->type, s);
  else   printf ("                     %c = %s", U[i]->fhead->type, s);
      }
    }
    break;
  }
  default:
    fputs("ReadICs: invalid initial conditions", stderr);
    exit(-1);
    break;
  }

  while (n--) fgets(buf, BUFSIZ, fp);    /* read to the end of the section */

  return;
}

static void Restart (char *name, int nfields, Element_List **U,
         Element_List *Mesh){
  Field    f;
  FILE    *fp;
  char    *p, fname[FILENAME_MAX], buf[BUFSIZ];
  register int i, pos, dump, nel;

  if (sscanf(name, "%s", fname) != 1)
    {fputs("ReadICs: couldn't read the restart file name", stderr);exit(-1);}

  if ((fp = fopen(fname,"r")) == (FILE *) NULL)
    {fprintf(stderr,"ReadICs: restart file %s not found",fname); exit(-1);}

  nel   = U[0]->nel;
  dump  = 0;
  memset(&f, '\0', sizeof (Field));

  if(option("oldhybrid"))
    set_nfacet_list(U[0]);

  while (readField (fp, &f)) dump++;
  if (!dump)
    {fputs("Restart: no dumps read from restart file", stderr);exit(-1);}

  if (f.dim != U[0]->fhead->dim())
    {fputs("Restart: file if wrong dimension", stderr); exit(-1);}

  if (f.nel != nel)
    {fputs("Restart: file is the wrong size", stderr); exit(-1);}

  ROOTONLY{
    printf("Initial solution   : %s at t = %g", fname, f.time);
    putc ('\n', stdout);
  }

  for (i = 0; i < nfields; i++) {
    if (!(p = strchr (f.type, U[i]->fhead->type))) {
      sprintf (buf, "variable %c is not on file", U[i]->fhead->type);
      fprintf(stderr,"Restart: %s", buf); exit(-1);
    }
    pos = (int) (p - f.type);

    copyfield (&f,pos,U[i]->fhead);
  }

  dparam_set("STARTIME", f.time);
  dparam_set("FINTIME", dparam("FINTIME") + f.time);
  freeField (&f);
  fclose    (fp);
  return;
}

/* ----------------------------------------------------------------------- *
 * ReadDF() - Read Drive Force data                                        *
 *                                                                         *
 * This is really one function designed to handle two different types of   *
 * drive force specification.  If the "steady" flag is active, the drive   *
 * force can be an arbitary function.  Otherwise, it must be constant but  *
 * can have any direction.                                                 *
 *                                                                         *
 * If the flowrate option is active, this section is skipped entirely.     *
 *                                                                         *
 * Example:                                                                *
 *                                                                         *
 *   ***** DRIVE FORCE DATA ***** PRESSURE GRAD, FLOW, Q                   *
 * 4                   Lines of Drive force data follow                    *
 *      FFX = 0.                                                           *
 *      FFY = 0.                                                           *
 *      FFZ = 2. * KINVIS                                                  *
 * C                                                                       *
 *                                                                         *
 * The names given to these lines does not matter, but their order is      *
 * taken as (x,y,z).  The '=' MUST be present, and everything to the right *
 * of it determines the forcing functions.                                 *
 * ----------------------------------------------------------------------- */
char *FORCING_FUNCTION;

void ReadDF(FILE *fp, int nforces, ...)
{
  register int i,k;
  int  nlines;
  char buf[BUFSIZ], *p;
  static char *ftypes[] = { "FFX", "FFY", "FFZ" };

  if (!findSection (SEC_DF, buf, fp))
    {fputs("ReadDF: section not found", stderr); exit(-1);}
  if (sscanf(fgets(buf, BUFSIZ, fp), "%d", &nlines) != 1)
    {fputs("ReadDF: can't read the number of lines", stderr); exit(-1);}

  /* Don't process this section if running in pre-processor mode */

  if (option("prep")) {
    while (nlines--) fgets(buf, BUFSIZ, fp);
    return;
  }

  if (dparam("FLOWATE") != 0.)       /* check for flowrate */
    option_set ("flowrate", 1);
  else if (nlines) {                             /* read the list... */
    for (i = 0; i < nforces && nlines--; i++) {
      for(p = fgets(buf, BUFSIZ, fp); isspace(*p); p++);
  if (toupper(*p) == 'C') {
    /* do nothing */
  } else {
    if (p = strchr(buf,'='))
      dparam_set(ftypes[i], scalar(++p));
    else
      fprintf(stderr,"ReadDF -- invalid function (below)", buf);
  }
    }
  }
  ROOTONLY{
    if (option("flowrate"))
      printf("Flowrate           : %g\n", dparam("FLOWRATE"));
    else {
      printf("Drive Force        : FFx = %g\n", dparam("FFX"));
      printf("                     FFy = %g\n", dparam("FFY"));
      printf("                     FFz = %g\n", dparam("FFZ"));
    }
  }

  while (nlines--) fgets(buf, BUFSIZ, fp);    /* finish the section */
  return;
}

/* ------------------------------------------------------------------------ *
 * ReadBCs() - Read Boundary Conditions                                     *
 *                                                                          *
 * This function reads boundary information for a spectral element field    *
 * and sets up the appropriate boundary structures.  The following boundary *
 * conditions are currently recognized:                                     *
 *                                                                          *
 *   Flag      Type         Field #1       Field #2       Field #3          *
 *  -----    --------      ----------     ----------     ----------         *
 *    V      Velocity      U-velocity     V-Velocity     W-Velocity         *
 *    W      Wall          = 0            = 0            = 0                *
 *    v      v             U(x,y,z)       V(x,y,z)       W(x,y,z)           *
 *    F      Flux          U' = f1        V' = f2        W' = f3            *
 *    f      flux          U'(x,y,z)      V'(x,y,z)      W'(x,y,z)          *
 *    O      Outflow       U' = 0         V' = 0         W' = 0             *
 *    E      Element       w/ ID          w/ EDGE                           *
 *    P      Periodic      w/ ID          w/ EDGE                           *
 *                                                                          *
 * NOTES:                                                                   *
 *                                                                          *
 *   - On the first call, this function records the beginning of the BC     *
 *     section.  Subsequent calls will re-read the BC's and assign data     *
 *     from the next available parameter.  There should be a total of DIM   *
 *     parameters available (DIM <= 3).                                     *
 *                                                                          *
 *   - For boundaries with a given function of (x,y,z),  the first line     *
 *     following is taken to be U(x,y,z), the second is V(x,y,z), etc.      *
 *     Here is an example (element 1, edge 1):                              *
 *                                                                          *
 *            1  1  v   0.0     0.0     0.0                                 *
 *                 ux = (1 - y^2)*sin(t)                                    *
 *                 uy = (1 - y^2)*cos(t)                                    *
 *                 uz = 0.                                                  *
 *            1  2  E   2.0     1.0     0.0                                 *
 *                                                                          *
 *     The "=" MUST be present, and the function is defined by the string   *
 *     to the right of it.                                                  *
 *                                                                          *
 *   - NO TIME DEPENDENT BOUNDARY CONDITIONS.                               *
 *                                                                          *
 *   - This routine will read any type of boundary conditions, FLUID or     *
 *     otherwise, as long as the file is positioned correctly.              *
 *                                                                          *
 * ------------------------------------------------------------------------ */

#define on  1
#define off 0

static fpos_t bc_start, bc_stop;
static void   prescan (FILE *fp),
              posscan (FILE *fp);



/* this routine reads all the global mesh boundary conditions to
   determine the type of boundaries and generates a list of dirichlet
   boundary conditions . No extra memory is assign to set the boundary
   conditions */

Bndry *ReadMeshBCs (FILE *fp, Element_List *Mesh){
  register   int n,i;
  char       bc, buf[BUFSIZ],*p;
  int        iel, iside;
  int        ntot;
  Element    *E;
  Bndry      *bndry_list = (Bndry *) NULL,
             *new_bndry  = (Bndry *) NULL;

  prescan(fp);

  /* .... Begin Reading Boundary Conditions .... */
  for(E=Mesh->fhead;E;E=E->next){
    ntot = (E->dim() == 3) ? E->Nfaces: E->Nedges;
    for (n = 0; n < ntot; ++n){
      while(strstr((p = fgets (buf, BUFSIZ, fp)),"="));

      /* vertex solve mask is on by default */
      while (isspace(*p)) p++; bc = *p++;
      sscanf(p, "%d%d", &iel, &iside);

      if (iel - E->id != 1 || iside - n != 1) {
  sprintf(buf, "Mismatched element/side -- got %d,%d, expected %d,%d",
    iel, iside, E->id+1, n+1);
  fprintf(stderr,"ReadBCs : %s\n", buf); exit(-1);
      }

      switch (bc) {
      case 'Z':
  new_bndry = (Bndry *)calloc(1,sizeof(Bndry));
  new_bndry->type = toupper(bc);
  new_bndry->face = n;
  new_bndry->elmt = E;
      case 'W':  case 'V':
  E->set_solve(n,off);
  new_bndry = (Bndry *)calloc(1,sizeof(Bndry));
  new_bndry->type = toupper(bc);
  new_bndry->face = n;
  new_bndry->elmt = E;
  break;
      case 'M': case 't': case 'v': case 'm':
  E->set_solve(n,off);
  new_bndry = (Bndry *)calloc(1,sizeof(Bndry));
  new_bndry->type = 'V';
  new_bndry->face = n;
  new_bndry->elmt = E;
  break;
      case 'O': /* required for pressure solve */
  new_bndry = (Bndry *)calloc(1,sizeof(Bndry));
  new_bndry->type = toupper(bc);
  new_bndry->face = n;
  new_bndry->elmt = E;
  break;
      default:
  new_bndry = (Bndry *) NULL;
  break;
      }

      if (new_bndry) {
  new_bndry->next = bndry_list;
  bndry_list      = new_bndry;
      }
    }
    if(E->dim() == 2 && E->Nedges == 3)
      while(strstr((p = fgets (buf, BUFSIZ, fp)),"="));/* skip extra side */
  }
  return bndry_list;      /* Return list */
}


Bndry *ReadBCs (FILE *fp, Element *U)
{
  register int   n,i;
  char       bc, buf[BUFSIZ], *p;
  int        iel, iside, nbcs=0,ntot,k;

  Element    *E;
  Bndry      *bndry_list = (Bndry *) NULL,
             *new_bndry  = (Bndry *) NULL;
  double     f[_MAX_FIELDS];
  static int fnum = 0;         /* this tracks the field numbers read */
  int active_handle = get_active_handle();

  if (!fnum) prescan(fp);
  fsetpos(fp, &bc_start);

  p = fgets (buf, BUFSIZ, fp);

 /* ....... Begin Reading Boundary Conditions ...... */
  E = U;
  for(k =0; (k < pllinfo[active_handle].gnel) && (E); ++k){

    sscanf(p, "%*1s%d%d", &iel, &iside);

    if(iel - pllinfo[active_handle].eloop[E->id] != 1){ /* skip to next element */

      ntot = (U->dim() == 3) ? pllinfo[active_handle].efaces[k] : pllinfo[active_handle].eedges[k];

      for(n = 0; n < ntot; ++n){
  while(strstr((p = fgets (buf, BUFSIZ, fp)),"="));
  if(!p){
    sprintf(buf,"Found end of file whilst reading element %d, side %d\n",
      E->id+1, n+1);
    exit(-1);
  }
      }
    }
    else{
      ntot = (E->dim() == 3) ? E->Nfaces: E->Nedges;

      for (n = 0; n < ntot; ++n){
  //  char *p = fgets (buf, BUFSIZ, fp);

  /* vertex solve mask is on by default */

  while (isspace(*p)) p++; bc = *p++;
  sscanf(p, "%d%d%lf%lf%lf", &iel, &iside, f, f+1, f+2);

  if (iel - pllinfo[active_handle].eloop[E->id] != 1 || iside - n != 1) {
    sprintf(buf, "Mismatched element/side -- got %d,%d, expected %d,%d",
      iel, iside, pllinfo[active_handle].eloop[E->id]+1, n+1);
    fprintf(stderr,"ReadBCs : %s\n", buf); exit(-1);
  }

  switch (bc) {

  case 'Z':
    f[fnum] = 0.;
    if(E->type == 'v'){
      E->set_solve(n,off);
      new_bndry = E->gen_bndry('W', n, f[fnum]);
    }
    else
      new_bndry = E->gen_bndry('F', n, f[fnum]);

    new_bndry->usrtype = bc;
    option_set("REFLECT", 1);
    break;
  case 'W':
    f[fnum] = 0.;
  case 'V': case 'M':
    E->set_solve(n,off);
    new_bndry = E->gen_bndry(bc, n, f[fnum]);
    break;
  case 'I': case 'O':
    f[fnum] = 0.;
  case 'F': case 'R':
    new_bndry = E->gen_bndry(bc, n, f[fnum]);
    break;
    /* Functional boundaries */
  case 't': case 'v': case 'm':
    E->set_solve(n,off);
  case 'f': case 'r': {
    fpos_t pos;

    for (i = 0; i <= fnum; i++) fgets(buf, BUFSIZ, fp);
    new_bndry = E->gen_bndry(bc, n, buf);

    do {
      fgetpos (fp, &pos);         /* Search through the following    */
      fgets   (buf, BUFSIZ, fp);  /* lines for the first one without */
    } while (strchr (buf, '='));  /* an '=' (function specification) */

    fsetpos (fp, &pos);
    break;
  }

  /* Element and Periodic boundaries */

  case 'E': case 'P':
    new_bndry = (Bndry *) NULL;
    break;
  default:
    fputs("ReadBCs: read a strange boundary condition", stderr);
    exit(-1);
    break;
  }
  if (new_bndry) {
    nbcs++;
    new_bndry->next = bndry_list;
    bndry_list      = new_bndry;
  }
  p = fgets (buf, BUFSIZ, fp);
      }
      if(E->dim() == 2)
  if(E->Nedges!=4)
    p = fgets (buf, BUFSIZ, fp); /* skip extra side */
      E=E->next;
    }
  }

  if (fnum++ == 0) posscan(fp); fsetpos(fp, &bc_stop);

  return bsort(bndry_list,nbcs);      /* Return the sorted list          */
}

/* ------------------------------------------------------------------------ *
 * bsort() - Sort Boundary Conditions                                       *
 *                                                                          *
 * The following function sorts boundary conditions so they are set accord- *
 * ing to precedence and not mesh order.  The following is the "hierarchy"  *
 * of boundary conditions :                                                 *
 *                                                                          *
 *    W          : Walls (zero velocity) are the most binding               *
 *    V          : Velocity boundary conditions (constant)                  *
 *    v          : Velocity function boundary conditions                    *
 *    all others                                                            *
 *                                                                          *
 * This function returns the sordid boundary list (ha ha).                  *
 *                                                                          *
 * ------------------------------------------------------------------------ */
Bndry *bsort(Bndry *bndry_list, int nbcs){
  Bndry   **tmp  = (Bndry**) malloc( nbcs * sizeof(Bndry*) );
  Bndry   *bedg  = bndry_list;
  register int i;

  if(!nbcs) return bndry_list;

  /* Copy the linked list into a regular array and sort it */

  for(i = 0 ; i < nbcs; i++, bedg = bedg->next) tmp[i] = bedg;
  qsort(tmp, nbcs, sizeof(Bndry*), bedgcmp);

  /* Create a new version of the ordered list */

  bedg  = (Bndry*) malloc (nbcs * sizeof(Bndry));
  for(i = 0; i < nbcs; i++) {
    memcpy (bedg + i, tmp[i], sizeof(Bndry));
    bedg[i].id   = i;
    bedg[i].next = bedg + i + 1;
  }
  bedg[nbcs-1].next   = (Bndry*) NULL;

  /* Free up the space occupied by the old list */

  bndry_list = bedg;
 // for(i = 0; i < nbcs; i++) free (tmp[i]);
  free (tmp);

  return bndry_list;
}

/*
 * Boundaries are sorted by their ASCII character values except for
 * types 'D' and 'N', which are sorted by their element ID numbers.
 */

static int bedgcmp(const void *b1, const void *b2)
{
  Bndry  *bedg_1 = *((Bndry**) b1),
         *bedg_2 = *((Bndry**) b2);
  char  btype_1 = bedg_1 -> type,
        btype_2 = bedg_2 -> type;

  /* Convert {D,N} -> {G,H} to get the ASCII precedence right */

  if (btype_1 == 'D') btype_1 = 'G';
  if (btype_2 == 'D') btype_2 = 'G';
  if (btype_1 == 'N') btype_1 = 'H';
  if (btype_2 == 'N') btype_2 = 'H';

  if      (btype_1 > btype_2)              /* Check ASCII code */
    return  1;
  else if (btype_1 < btype_2)
    return -1;
  else                                     /* Check element ID */
    {
      int id_1 = bedg_1 -> elmt -> id,
          id_2 = bedg_2 -> elmt -> id;

      if       (id_1 > id_2)
  return  1;
      else if  (id_1 < id_2)
  return -1;
      else                                   /* Check edge ID    */
  {
    id_1 = bedg_1 -> face;
    id_2 = bedg_2 -> face;

    if      (id_1 > id_2)
      return  1;
    else if (id_1 < id_2)
      return -1;
  }
    }

  /* If we get this far there is an error in the boundary conditions */

  fputs("bedgcmp(): " "Duplicate boundary\n",stderr); exit(-1);
  return 0;
}

static void prescan(FILE *fp)
{
  char buf[BUFSIZ];

  rewind(fp);
  findSection(SEC_BCS,buf,fp);
  if (!strstr(buf, SEC_BCS))
    error_msg(ReadBCs: do not see any boundary conditions);


  do
    fgets(buf, BUFSIZ, fp);
  while
    (strstr(buf, "NO"));

  fgetpos (fp, &bc_start);
  return;
}

static void posscan (FILE *fp)
{
  char buf[BUFSIZ];
  fgetpos (fp, &bc_stop);
  while (strstr(fgets(buf,BUFSIZ,fp), SEC_BCS))
    fgetpos (fp, &bc_stop);
  return;
}

/* set up inter element links using the coundary connectivity information */

static void set_link (Element_List *U, int eid1, int id1, int eid2, int id2);

void ReadSetLink(FILE *fp, Element_List *U)
{
  register int   n,i;
  char       bc, buf[BUFSIZ];
  int        iel, iside, trip,ntot;
  Element    *E;
  double     f[3];

  rewind(fp);
  findSection(SEC_BCS,buf,fp);
  if (!strstr(buf, SEC_BCS))
    error_msg(ReadSetLink: do not see any boundary conditions);
  fgets(buf,BUFSIZ,fp);

  /* ....... Search through Boundary Conditions ...... */

  char *p = fgets(buf,BUFSIZ, fp);

  for(E=U->fhead,trip = 0;E;E=E->next){
    ntot = (E->dim() == 3) ? E->Nfaces: E->Nedges;

    for (n = 0; n < ntot; ++n){
      while (isspace(*p)) p++; bc = *p++;
      sscanf(p,"%d%d%lf%lf%lf", &iel, &iside, f, f+1, f+2);

      if (iel - E->id != 1 || iside - n != 1) {
  sprintf(buf, "Mismatched element/side -- got %d,%d, expected %d,%d",
    iel, iside, E->id+1, n+1);
  fprintf(stderr,"ReadSetLink : %s\n", buf); exit(-1);
      }

      if ((bc == 'E')||(bc == 'P')) /* Element and Periodic boundaries */
  set_link(U,iel-1,iside-1,(int)f[0]-1,(int)f[1]-1);
      if ((bc == 'v')||(bc == 'f')||(bc == 'r')||(bc == 'm'))
  while(strchr(p=fgets(buf,BUFSIZ,fp),'='));
      else
  p=fgets(buf,BUFSIZ,fp);
    }
    if(E->identify() == Nek_Tri || E->identify() == Nek_Nodal_Tri){
      p=fgets(buf,BUFSIZ,fp);
    }
  }

  for(E=U->fhead;E;E=E->next)
    if(E->dim() == 3)
      for(i=0;i<E->Nfaces;++i)
  if(E->face[i].link)
    if(E->face[i].link->link->eid != E->id ||
       E->face[i].link->link->id != i)
      fprintf(stderr,"Error in ReadSetLink: Elmt %d, Face %d not matched\n", E->id+1, i+1);

  return;
}


static void edge_link(Edge *edg1, Edge *edg2){
  Edge *e;
  int eid1, eid2, flag;

  eid1 = edg1->eid;  eid2 = edg2->eid;
  /* always set base to edge with lowest element id */


  if(!(edg1->base)&&!(edg2->base)){
    if(eid1 >= eid2){
      edg1->base = edg2;
      edg2->base = edg2;
      edg2->link = edg1;
    }
    else{
      edg1->base = edg1;
      edg2->base = edg1;
      edg1->link = edg2;
    }
  }
  else if(!(edg1->base)&&edg2->base){
    if(eid1 >= edg2->base->eid){
      edg1->base = edg2->base;
      for(e=edg2->base; e->link; e = e->link);
      e->link = edg1;
    }
    else{
      edg1->base = edg1;
      edg1->link = edg2->base;
      for(e=edg2->base; e; e=e->link) e->base = edg1;
    }
  }
  else if (edg1->base&&!(edg2->base)){
    if(edg1->base->eid >= eid2){
      edg2->base = edg2;
      edg2->link = edg1->base;
      for(e=edg1->base; e; e= e->link) e->base = edg2;
    }
    else{
      edg2->base = edg1->base;
      for(e=edg1->base; e->link; e = e->link);
      e->link = edg2;
    }
  }
  else{
    if(edg1->base->eid >= edg2->base->eid){

      flag = 1;
      for(e=edg2->base;e;e=e->link)
  if(e==edg1){ flag = 0; break;}
      for(e=edg1->base;e;e=e->link)
  if(e==edg2){ flag = 0; break;}
      if(flag){
  for(e=edg2->base; e->link; e = e->link);
  e->link = edg1->base;
  for(e=edg1->base; e; e = e->link) e->base = edg2->base;
      }
    }
    else if(edg1->base->eid < edg2->base->eid){
      for(e=edg1->base; e->link; e = e->link);
      e->link = edg2->base;
      for(e=edg2->base; e; e = e->link) e->base = edg1->base;
    }
  }
}

static void set_edge_con(Edge *edg1, Edge *edg2, int trip){

  if(trip){ /* set two edges to different connectivity */
    if(!(edg1->base)&&!(edg2->base))   /* set first edge to 1           */
      edg1->con = 1;
    else if(!(edg1->base)&&edg2->base) /* set undefined edge to inverse */
      edg1->con = !(edg2->con);        /* of defined edge               */
    else if (edg1->base&&!(edg2->base))
      edg2->con = !(edg1->con);
    else if(edg1->con == edg2->con)    /* inverse all edges in edg2 link */
      for(edg2=edg2->base; edg2; edg2 = edg2->link)
  edg2->con = !(edg2->con);
  }
  else{  /* set both edges to same connectivity */
    if(!(edg1->base)&&edg2->base)      /* set undefined edge to inverse */
      edg1->con = edg2->con;           /* of defined edge               */
    else if (edg1->base&&!(edg2->base))
      edg2->con = edg1->con;
    else if(edg1->con != edg2->con)    /* inverse all edges in edg2 link */
      for(edg2=edg2->base; edg2; edg2 = edg2->link)
  edg2->con = !(edg2->con);
  }
}



static void set_link(Element_List *U, int eid1, int id1, int eid2, int id2){

  if(U->fhead->dim() == 2){

    // 2d

    int con_nodal[4][4]= {{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}};
    int con_modal[4][4]= {{1,1,0,0},{1,1,0,0},{0,0,1,1},{0,0,1,1}};
    int **con;

    con = imatrix(0,3,0,3);

    if(U->fhead->identify() == Nek_Nodal_Quad ||
       U->fhead->identify() == Nek_Nodal_Tri)
      icopy(16, con_nodal[0], 1, con[0], 1);
    else
      icopy(16, con_modal[0], 1, con[0], 1);

    /* set up connectivity inverses if required */
    /* like faces meet */
    Element *E=U->flist[eid1],*F=U->flist[eid2];

    if(eid1>eid2)
      E->edge[id1].con = con[id1][id2];
    else
      F->edge[id2].con = con[id1][id2];

    /* set links */
    if(eid1 < eid2){
      E->edge[id1].base = E->edge + id1;
      E->edge[id1].link = F->edge + id2;
      F->edge[id2].base = E->edge + id1;
    }

    if(eid1 == eid2 && id1 < id2){
      E->edge[id1].base = E->edge + id1;
      E->edge[id1].link = F->edge + id2;
      F->edge[id2].base = E->edge + id1;
    }
    free(con[0]);
    free(con);
  }
  else{

    // 3d
    register int i,j;

    int con_tet[4] = {1,2,2,1};
    int con_pyr[5] = {0,2,2,1,1};
    //    int con_pri[5] = {0,2,0,2,0};
    int con_pri[5] = {0,2,0,1,0};
    int *con_test_a;
    int *con_test_b;

    if(eid1 > eid2) return; /* just do first connection in list */

    /* set up connectivity inverses if required */
    /* like faces meet */
    Element *E=U->flist[eid1],*F=U->flist[eid2];

    switch(E->identify()){
    case Nek_Tet:
      con_test_a = con_tet;
      break;
    case Nek_Pyr:
      con_test_a = con_pyr;
      break;
    case Nek_Prism:
      con_test_a = con_pri;
      break;
    }

    switch(F->identify()){
    case Nek_Tet:
      con_test_b = con_tet;
      break;
    case Nek_Pyr:
      con_test_b = con_pyr;
      break;
    case Nek_Prism:
      con_test_b = con_pri;
      break;
    }

    /* set up connectivity inverses if required  and edge links */
    /* like faces meet */

    int nfv1 = E->Nfverts(id1);
    int nfv2 = F->Nfverts(id2);

    if(nfv1 != nfv2){
      fprintf(stderr,
        "Set_link: Error in connectivity, two unlike faces connected\n");
      exit(-1);
    }

    if(nfv1 == 3){  // triangle meets triangle

      if(con_test_a[id1] == con_test_b[id2]){
  E->face[id1].con = 1;

  set_edge_con(E->edge+E->ednum(id1,0), F->edge+F->ednum1(id2,0),1);
  edge_link   (E->edge+E->ednum(id1,0), F->edge+F->ednum1(id2,0));

  for(i = 1; i < nfv1; ++i){
    set_edge_con(E->edge+E->ednum(id1,i), F->edge+F->ednum1(id2,i),0);
    edge_link   (E->edge+E->ednum(id1,i), F->edge+F->ednum1(id2,i));
  }
      }
      else{
  for(i = 0; i < nfv1; ++i){
    set_edge_con(E->edge+E->ednum(id1,i), F->edge+F->ednum(id2,i),0);
    edge_link   (E->edge+E->ednum(id1,i), F->edge+F->ednum(id2,i));
  }
      }

      /* set face links */
      E->face[id1].link = F->face + id2;
      F->face[id2].link = E->face + id1;

    }
    else{   // quad. face meets quad. face

      double x, y, z, x1, y1, z1, cx, cy, cz;
      double TOL = 1e-5;

      int map[4];
      map[0] = map[1] = map[2] = map[3] = -1;
      // find average of face vertices for periodic case
      // subtract vector between centres
      cx = 0.0; cy = 0.0; cz = 0.0;

      for(j=0;j<nfv1;++j){
  cx += F->vert[F->vnum(id2,j)].x - E->vert[E->vnum(id1,j)].x;
  cy += F->vert[F->vnum(id2,j)].y - E->vert[E->vnum(id1,j)].y;
  cz += F->vert[F->vnum(id2,j)].z - E->vert[E->vnum(id1,j)].z;
      }
      cx /= 1.*nfv1;      cy /= 1.*nfv1;      cz /= 1.*nfv1;

      for(j=0;j < nfv1;++j){
  x = E->vert[E->vnum(id1,j)].x;
  y = E->vert[E->vnum(id1,j)].y;
  z = E->vert[E->vnum(id1,j)].z;
  for(i=0;i < nfv2;++i){
    x1 = F->vert[F->vnum(id2,i)].x-cx;
    y1 = F->vert[F->vnum(id2,i)].y-cy;
    z1 = F->vert[F->vnum(id2,i)].z-cz;
    if(sqrt((x1-x)*(x1-x) + (y1-y)*(y1-y) + (z1-z)*(z1-z)) < TOL){
      map[j] = i;
      break;
    }
  }
      }

      //      fprintf(stdout, "map: %d %d %d %d \n", map[0], map[1], map[2], map[3]);

      if(i==nfv2){
  fprintf(stderr,
    "Set_link: elmt:%d face:%d and elmt:%d face:%d do not align",
    eid1, id1, eid2, id2);
  exit(-1);
      }

      int con[4] = {0,0,0,0};

      if(map[1] == (map[0]+1)%nfv2){ // same orientation
  switch(map[0]){
  case 0:
    E->face[id1].con = 0;   // nothing to do
    break;
  case 1:
    E->face[id1].con = 2+4; // negate 'b' and transpose
    con[1] = con[3] = 1;    // negate 'b' edges
    break;
  case 2:
    E->face[id1].con = 3;   // negate 'a' and 'b'
    con[0] = con[1] = con[2] = con[3] = 1;
    break;
  case 3:
    E->face[id1].con = 1+4; // negate 'a' and transpose
    con[0] = con[2] = 1;    // negate 'a' edges
    break;
  }


  for(i = 0; i < nfv1; ++i){
    set_edge_con(E->edge+E->ednum(id1,i),
           F->edge+F->ednum(id2,(map[0]+i)%nfv2),con[i]);
    edge_link   (E->edge+E->ednum(id1,i),
           F->edge+F->ednum(id2,(map[0]+i)%nfv2));
  }
      }
      else{                         // mirror orientation
  switch(map[0]){
  case 0:
    E->face[id1].con = 4;   // transpose
    break;
  case 1:
    E->face[id1].con = 1; // negate 'a'
    con[0] = con[2] = 1;  // negate 'a' edges
    break;
  case 2:
    E->face[id1].con = 3+4; // negate 'a' and 'b' and transpose
    con[0] = con[1] = con[2] = con[3] = 1;
    break;
  case 3:
    E->face[id1].con = 2; // negate 'b'
    con[1] = con[3] = 1;  // negate 'b' edges
    break;
  }
  int tmp_map[][4] = {{3,2,1,0},{0,3,2,1},{1,0,3,2},{2,1,0,3}};

  for(i = 0; i < nfv1; ++i){
    set_edge_con(E->edge+E->ednum(id1,i),
           F->edge+F->ednum(id2,tmp_map[map[0]][i]),con[i]);
    edge_link   (E->edge+E->ednum(id1,i),
           F->edge+F->ednum(id2,tmp_map[map[0]][i]));
  }
      }

      /* set face links */
      E->face[id1].link = F->face + id2;
      F->face[id2].link = E->face + id1;
    }
  }
}
// needs to be fixed

void  ReadOrderFile(char *name, Element_List *E){
#if 0 /* not currently configured */
  register int i,k;
  int *size,n;
  const int nel = E->nel;
  char buf[BUFSIZ],*p;
  FILE *fp;

  int Max_size = Max_Nfaces+Max_Nedges+1;

  sprintf(buf,"%s.ord",name);
  if(!(fp = fopen(buf,"r"))){
    fprintf(stderr,"ERROR: file \"%s\" does not exist\n",buf);
    exit(-1);
  }

  size = ivector(0,Max_Nedges+Max_Nfaces+1 -1);

  /* skip comments */
  while(p=fgets(buf,BUFSIZ,fp))
    if(strstr(p,"Modes")){
      for(k=0;k<nel;++k){
  fscanf(fp,"%*d");
  if(E->flist[i]->dim() == 2)
    for(i = 0; i < E->flist[i]->Nedges+1; ++i)
      fscanf(fp,"%d",size+i);
  else
    for(i = 0; i < E->flist[i]->Nedges+E->flist->Nfaces; ++i)
      fscanf(fp,"%d",size+i);

  E->flist[k]->Mem_J(size,'n');
  E->flist[k]->Mem_Q();
      }
      break;
    }
    else if (strstr(p,"Function")){
      double x[Max_size],y[Max_size],z[Max_size],f[Max_size];
      extern int ednum[][3];

      p = fgets(buf,BUFSIZ,fp);

      if((p = (strchr(p,'='))) == (char *) NULL)
  error_msg(CheckOrderFIle: Illegal function definition);
#if DIM == 2
      vector_def("x y",++p);
#else
      vector_def("x y z",++p);
#endif
      double max_f;
      Element *F;

      for(k=0;k<nel;++k){
  F=E->flist[k];
#if DIM == 2
  for(i = 0; i < F->Nverts; ++i){
    x[i] = 0.5*(F->vert[i].x + F->vert[(i+1)%F->Nverts].x);
    y[i] = 0.5*(F->vert[i].y + F->vert[(i+1)%F->Nverts].y);
  }
  vector_set(F->Nverts,x,y,f);
  /* set fourth side to be consistent with highest edge */
  max_f=0.0;
  for(i = 0; i < F->Nedges; ++i)
    max_f = (f[i]>max_f) ? f[i]:max_f;
  f[F->Nverts] =(F->identify()==Nek_Tri || F->identify()==Nek_Nodal_Tri)
    ? max_f-1.0:max_f;

  if(n=option("aposterr"))
    for(i = 0; i < F->Nedges+1; ++i) size[i] = (int)max(f[i],0.0) + n;
  else
    for(i = 0; i < F->Nedges+1; ++i) size[i] = (int)max(f[i],0.0);
#else
  for(i = 0; i < 3; ++i){
    x[i] = 0.5*(E[k].vert[i].x + E[k].vert[(i+1)%3].x);
    y[i] = 0.5*(E[k].vert[i].y + E[k].vert[(i+1)%3].y);
    z[i] = 0.5*(E[k].vert[i].z + E[k].vert[(i+1)%3].z);
  }
  for(i = 0; i < 3; ++i){
    x[i+3] = 0.5*(E[k].vert[i].x + E[k].vert[3].x);
    y[i+3] = 0.5*(E[k].vert[i].y + E[k].vert[3].y);
    z[i+3] = 0.5*(E[k].vert[i].z + E[k].vert[3].z);
  }

  vector_set(6,x,y,z,f);
  /* set faces to be maximun of surrounding edges */
  /* and edges to be maximun of faces             */
  f[10] = 0;
  for(i = 0; i < 4; ++i){
    f[i+6] = max(max(f[ednum[i][0]],f[ednum[i][1]]),f[ednum[i][2]])-1;
    f[10]  = max(f[10],f[i+6]);
  }
  --f[10];

  for(i = 0; i < 11; ++i) size[i] = (int)max(f[i],0.0);
#endif
  F->Mem_J(size,'n');
  F->Mem_Q();
      }
      break;
    }
  option_set("variable",1);

  free(size);
#else
  error_msg(ReadOrderFile needs redefining);
#endif
}


/* ----------------------------------------------------------------------- *
 * ReadHisData() - Read History and Integral data                          *
 *                                                                         *
 * This function reads in and sets up structures for the processing of     *
 * history points.  This is comprised of a linked list of structures which *
 * are processed by the Analyzer().                                        *
 *                                                                         *
 * ----------------------------------------------------------------------- */

static HisPoint *appendHisPoint (HisPoint *list, HisPoint *hp);

void ReadHisData (FILE *fp, Domain *omega)
{
  char      buf[BUFSIZ], *p;
  int       cnt, npts, nel, i;
  HisPoint *hp, *his_list = (HisPoint*)NULL;
  int active_handle = get_active_handle();

  if (!findSection(SEC_HIST, buf, fp))
    {fputs("ReadHisData: Section not found", stderr); exit(-1);}
  if (sscanf(fgets(buf, BUFSIZ, fp), "%d", &npts) != 1)
    {fputs("ReadHisData: can't read the number of points", stderr); exit(-1);}

  nel    = iparam("ELEMENTS");

  while (npts--) {
    hp = (HisPoint*) calloc(1, sizeof(HisPoint));
    for (p = fgets(buf, BUFSIZ, fp); !isdigit(*p); p++)
      if (isalpha(*p=tolower(*p)))
  switch(*p){
  case 'h': /* treat as vertex point */
    hp->mode = TransVert;
    break;
  case 'e': /* treat as mid point of edge */
    hp->mode = TransEdge;
    break;
  default:
    strncat(hp->flags, p, 1);
  }

    hp->i = strtol (p, &p, 10);
    hp->j = strtol (p, &p, 10);
    hp->k = strtol (p, &p, 10);

    hp->id = atoi (p);

    DO_PARALLEL{ /* Check to see if this element is in the partition  */
      hp->id--;
      for(i = 0; i < pllinfo[active_handle].nloop; ++i)
  if(hp->id == pllinfo[active_handle].eloop[i]){
    hp->id = i;
    break;
  }
      if(i == pllinfo[active_handle].nloop){
  free(hp);
  continue;
      }
      else if((--hp->i) > omega->U->flist[i]->Nedges){
  free(hp);
  continue;
      }
    }
    else{
      if ((--hp->i) > omega->U->flist[atoi(p)-1]->Nedges || (hp->id--) > nel)
  { free(hp); continue; }
    }
    his_list = appendHisPoint (his_list, hp);
  }


  /* This history step can be overridden by setting HISSTEP */
  option_set ("hisstep", iparam ("HISSTEP") > 0 ?
        iparam ("HISSTEP") : max(1, iparam("NSTEPS") / 1000));

  if (omega->his_list = his_list) {
    /* Echo the history points */
    ROOTONLY
      puts("\nHistory points:");
    DO_PARALLEL{
      for (hp = his_list, cnt = 0; hp; hp = hp->next) {
  if(hp->mode == TransVert)
    printf ("%2d: Proc %d, vertex = %2d, elmt = %3d, fields = %s\n",
      ++cnt,pllinfo[active_handle].procid, hp->i + 1, pllinfo[active_handle].eloop[hp->id] + 1,
      hp->flags);
  else if(hp->mode == TransEdge)
    printf ("%2d: Proc %d, edge   = %2d, elmt = %3d, fields = %s\n",
      ++cnt,pllinfo[active_handle].procid, hp->i + 1, pllinfo[active_handle].eloop[hp->id] + 1,
      hp->flags);
      }
    }
    else
      {
  for (hp = his_list, cnt = 0; hp; hp = hp->next) {
    if(hp->mode == TransVert)
      printf ("%2d: vertex = %2d, elmt = %3d, fields = %s\n", ++cnt,
        hp->i + 1, hp->id + 1, hp->flags);
    else if(hp->mode == TransEdge)
      printf ("%2d: edge   = %2d, elmt = %3d, fields = %s\n", ++cnt,
        hp->i + 1, hp->id + 1, hp->flags);
  }
      }
    putchar ('\n');
  }
  return;
}

static HisPoint *appendHisPoint (HisPoint *list, HisPoint *hp)
{
  HisPoint *h = list;

  if (h) {
    while (h->next) h = h->next;
    h->next = hp;
  } else
    list = hp;

  return list;
}


void ReadSoln(FILE* fp, Domain* omega){
  char buf[BUFSIZ], *s;
  int  i;
  int nfields = omega->U->fhead->dim() +1;
  rewind(fp);

  omega->soln = (char**) malloc(nfields*sizeof(char*));

  if(findSection("Exact", buf, fp)){
    for(i=0;i<nfields;++i){
      if(s = strchr(fgets(buf,BUFSIZ,fp),'=')){
  while (isspace(*++s));
  omega->soln[i] = (char*) malloc(BUFSIZ*sizeof(char));
  strcpy(omega->soln[i],s);
      }
      else{
  omega->soln[i] = (char*) malloc(BUFSIZ*sizeof(char));
  sprintf(omega->soln[i],"0.0");
      }
    }
    ROOTONLY{
      fprintf(stderr,"Exact U : %s",omega->soln[0]);
      fprintf(stderr,"Exact V : %s",omega->soln[1]);
      fprintf(stderr,"Exact W : %s",omega->soln[2]);
      fprintf(stderr,"Exact P : %s",omega->soln[3]);
    }
  }
  else
    for(i=0;i<nfields;++i){
      omega->soln[i] = (char*) malloc(BUFSIZ*sizeof(char));
      sprintf(omega->soln[i],"0.0");
    }

  rewind(fp);
}

void ReadDFunc(FILE *fp, Domain *Omega, Element_List *Mesh){
  int  nlines, i;
  char buf[BUFSIZ], *p;
  double alpha = dparam("FCESCAL");

  if (!findSection (SEC_DFUNCS, buf, fp))
    {
      ROOTONLY fputs("ReadDFunc: section not found\n", stderr);
      Omega->ForceFuncs = (double**)0;
      rewind(fp);
      return;
    }
  if (sscanf(fgets(buf, BUFSIZ, fp), "%d", &nlines) != 1)
    {fputs("ReadDFunc: can't read the number of lines\n", stderr); exit(-1);}

  Omega->ForceFuncs =
    dmatrix(0, Omega->U->fhead->dim()-1, 0, Omega->U->htot-1);

  if(strstr(p = fgets(buf, BUFSIZ, fp),"File")){
    fgets(buf, BUFSIZ, fp);
    Element_List *UL[3];
    UL[0] = Omega->Uf; UL[0]->fhead->type = 'h';
    UL[1] = Omega->Vf; UL[1]->fhead->type = 'i';
    UL[2] = Omega->Wf; UL[2]->fhead->type = 'j';
    Restart(buf, 3, UL, Mesh);
    UL[0]->Trans(UL[0], J_to_Q);  UL[0]->fhead->type = 'u';
    UL[1]->Trans(UL[1], J_to_Q);  UL[1]->fhead->type = 'v';
    UL[2]->Trans(UL[2], J_to_Q);  UL[2]->fhead->type = 'w';;

    if(alpha != 0.0){
      fprintf(stdout,"Modifying forcing by scaling factor = %lf\n",alpha);
      dscal(UL[0]->htot, alpha, UL[0]->base_h, 1);
      dscal(UL[1]->htot, alpha, UL[1]->base_h, 1);
      dscal(UL[2]->htot, alpha, UL[2]->base_h, 1);
    }


    dcopy(UL[0]->htot, UL[0]->base_h, 1, Omega->ForceFuncs[0], 1);
    dcopy(UL[0]->htot, UL[1]->base_h, 1, Omega->ForceFuncs[1], 1);
    dcopy(UL[0]->htot, UL[2]->base_h, 1, Omega->ForceFuncs[2], 1);
    return;
  }

  if (p = strchr (buf, '=')) {
    while (isspace(*++p));
    Omega->Uf->Set_field(p);
    dcopy(Omega->Uf->htot, Omega->Uf->base_h, 1, Omega->ForceFuncs[0],1);
  }

  if (p = strchr (fgets(buf, BUFSIZ, fp), '=')) {
    while (isspace(*++p));
    Omega->Uf->Set_field(p);
    dcopy(Omega->Uf->htot, Omega->Uf->base_h, 1, Omega->ForceFuncs[1],1);
  }
  if (p = strchr (fgets(buf, BUFSIZ, fp), '=')) {
    while (isspace(*++p));
    Omega->Uf->Set_field(p);
    dcopy(Omega->Uf->htot, Omega->Uf->base_h, 1, Omega->ForceFuncs[2],1);
  }
}

#if 0
void ReadKinvis(Domain* omega){
  int  i,k, skip;
  Element_List *U = omega->U;
  Element *E;

  double sponpos = dparam("SPONPOS");
  double sponlen = dparam("SPONLEN");
  double sponmag = dparam("SPONMAG");

  Coord X;
  X.x  = dvector(0, QGmax*QGmax*QGmax-1);
  X.y  = dvector(0, QGmax*QGmax*QGmax-1);
  X.z  = dvector(0, QGmax*QGmax*QGmax-1);

  for(skip=0, k=0;k<U->nel;skip+=U->flist[k]->qtot,++k){
    E = U->flist[k];
    if(sponlen){
      E->coord(&X);
      omega->kinvis[k].p = omega->kinvis->p + skip;

      for(i=0;i<E->qtot;++i)
  omega->kinvis[k].p[i] = (X.x[i] < sponpos) ? 1.:
  //  (1+cos(M_PI*(X.x[i]-sponpos)/sponlen))*.5;
  (1. + sponmag*(X.x[i]-sponpos)*(X.x[i]-sponpos));
    }
  }
  if(sponlen)
    fprintf(stdout, "ReadKinvis: sponge length: %lf sponge position: %lf \n",
      sponlen, sponpos);

  free(X.x);    free(X.y); free(X.z);
}
#endif
