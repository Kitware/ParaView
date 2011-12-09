/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/orienter.C,v $
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

/* Each of the following strings MUST be defined */
static char  usage_[128];

char *prog   = "vort";
char *usage  = "vort:  [options]  -r file[.rea]  input[.fld]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "-q      ... quadrature point spacing. Default is even spacing\n"
  "-R      ... range data information. must have mesh file specified\n"
  "-b     ... make body elements specified in mesh file\n"
  "-n #    ... Number of mesh points.";

static void parse_util_args (int argc, char *argv[]);
static void Write(Element_List **E, FILE *out, int nfields);

double TOL = 1e-7;

void get_int(char *str, int *res){
  char buf[BUFSIZ];
  fprintf(stderr, "%s:   ", str);
  fgets  (buf, BUFSIZ, stdin);
  while(buf[0] == '#')
    fgets  (buf, BUFSIZ, stdin);
  sscanf(buf, "%d", res);
}

void get_double(char *str, double *res){
  char buf[BUFSIZ];
  fprintf(stderr, "%s:   ", str);
  fgets  (buf, BUFSIZ, stdin);
  while(buf[0] == '#')
    fgets  (buf, BUFSIZ, stdin);
  sscanf(buf, "%lf", res);
}

void get_char(char *str, char *res){
  char buf[BUFSIZ];
  char tmp[2];
  fprintf(stderr, "%s:   ", str);
  fgets  (buf, BUFSIZ, stdin);
  while(buf[0] == '#')
    fgets  (buf, BUFSIZ, stdin);
  sscanf(buf, "%1s", tmp);
  res[0] = tmp[0];
}

void get_string(char *str, char *res){
  char buf[BUFSIZ];

  fprintf(stderr, "%s:   ", str);
  fgets  (buf, BUFSIZ, stdin);
  while(buf[0] == '#')
    fgets  (buf, BUFSIZ, stdin);
  sprintf(res, strtok(buf, "\n"));
}


void add_bc(Bndry **OldBc, Bndry *NewBc){

  if(!*OldBc){
    OldBc[0] = NewBc;
    NewBc->next = NULL;
    return;
  }
  NewBc->next = OldBc[0];
  OldBc[0] = NewBc;
}

double XPERIOD = 0.;
double YPERIOD = 0.;
double ZPERIOD = 0.;

int neighbourtest(Element *A, int aid, Element *B, int bid){

  if(A->Nfverts(aid) != B->Nfverts(bid))
    return 0;

  int i;
  double ax = 0., ay = 0., az = 0.;
  for(i=0;i<A->Nfverts(aid);++i){
    ax += A->vert[A->vnum(aid, i)].x;
    ay += A->vert[A->vnum(aid, i)].y;
    az += A->vert[A->vnum(aid, i)].z;
  }

  for(i=0;i<B->Nfverts(bid);++i){
    ax -= B->vert[B->vnum(bid, i)].x;
    ay -= B->vert[B->vnum(bid, i)].y;
    az -= B->vert[B->vnum(bid, i)].z;
  }
  ax /= (double)B->Nfverts(bid);
  ay /= (double)B->Nfverts(bid);
  az /= (double)B->Nfverts(bid);
  if((ax*ax+ay*ay+az*az) > TOL){
    if((ay*ay+az*az) < TOL && fabs(fabs(ax)-XPERIOD) < TOL )
      return 1;
    else if((ax*ax+az*az) < TOL && fabs(fabs(ay)-YPERIOD) < TOL)
      return 1;
    else if((ax*ax+ay*ay) < TOL && fabs(fabs(az)-ZPERIOD) < TOL)
      return 1;

    return 0;
  }
  return 1;
}

void set_link(Element *A, int aid, Element *B, int bid){
  A->face[aid].link =  B->face+bid;
  B->face[bid].link =  A->face+bid;
}

class Grid{
public:
  char    *domainname;
  FILE    *domainfile;

  int      totverts;      // total number of unique vertices
  double  *xcoords;       // x-coordinates of vertices
  double  *ycoords;       // y-coordinates of vertices
  double  *zcoords;       // z-coordinates of vertices


  int      nel;           // number of elements
  int     *elmtids;       // actual id of elements
  int     *nverts;        // number of vertices in each element
  int    **vertids;       // matrix of global element vertex ids

  int    **vertexmap;    // list of local vertex which is the base vertex

  Grid     (char *);
  Grid     (Grid*);
  void RemoveHexes (void);
  void RemovePrisms(void);
  void FixTetOrientation(void);
  void ImportTetOrientation(Grid *);
  void ImportPrismOrientation(Grid *);
  void RenumberPrisms();
  void FixPrismOrientation(void);
  Element_List *gen_aux_field();
};

void get_next_line(FILE *fp, char *buf){
  fgets  (buf, BUFSIZ, fp);
  while(buf[0] == '#')
    fgets  (buf, BUFSIZ, fp);
}

#define TOLVERT 1e-5
static int same (double z1, double z2)
{
  if (fabs(z1-z2) < TOLVERT)
    return 1;
  else
    return 0;
}

static char *findSection (char *name, char *buf, FILE *fp)
{
  char *p;

  while (p = fgets (buf, BUFSIZ, fp))
    if (strstr (p, name))
      break;

  return p;
}

Grid::Grid(char *fname){
  register int i,j;
  char buf[BUFSIZ];

  if(option("REAFILE")){
  register int k,l;
  int   dim;
  double **xloc, **yloc, **zloc;
  int    **locvid;

  sprintf(buf, "%s.rea",strtok(fname,"\n"));

  domainname = strdup(buf);
  domainfile = fopen(domainname,"r");

  if(!findSection("MESH",buf,domainfile))
    {fputs("Grid_rea: Section not found\n", stderr); exit(-1);}

  get_next_line(domainfile, buf);
  sscanf(buf,"%d%d",&nel,&dim);

  if(dim != 3){ fputs("Grid_rea: Mesh is not 3D\n",stderr); exit(-1);}

  xloc = dmatrix(0,nel-1,0,Max_Nverts-1);
  yloc = dmatrix(0,nel-1,0,Max_Nverts-1);
  zloc = dmatrix(0,nel-1,0,Max_Nverts-1);

  nverts  = ivector(0, nel-1);

  /* read local mesh */
  for(k = 0; k < nel; k++) {
    get_next_line(domainfile, buf);  /* element header */

    if(strstr(buf,"Hex") || strstr(buf,"hex")){
      fscanf(domainfile,"%lf%lf%lf%lf%lf%lf%lf%lf",
       xloc[k],xloc[k]+1,xloc[k]+2,xloc[k]+3,xloc[k]+4,
       xloc[k]+5,xloc[k]+6,xloc[k]+7);
      fscanf(domainfile,"%lf%lf%lf%lf%lf%lf%lf%lf",
       yloc[k],yloc[k]+1,yloc[k]+2,yloc[k]+3,yloc[k]+4,
       yloc[k]+5,yloc[k]+6,yloc[k]+7);
      fscanf(domainfile,"%lf%lf%lf%lf%lf%lf%lf%lf",
       zloc[k],zloc[k]+1,zloc[k]+2,zloc[k]+3,zloc[k]+4,
       zloc[k]+5,zloc[k]+6,zloc[k]+7);
      get_next_line(domainfile, buf);  /* get remainder of line */
      nverts[k] = 8;
    }
    else if(strstr(buf,"Prism") || strstr(buf,"prism")){
      fscanf(domainfile,"%lf%lf%lf%lf%lf%lf",
       xloc[k],xloc[k]+1,xloc[k]+2,xloc[k]+3,xloc[k]+4,xloc[k]+5);
      fscanf(domainfile,"%lf%lf%lf%lf%lf%lf",
       yloc[k],yloc[k]+1,yloc[k]+2,yloc[k]+3,yloc[k]+4,yloc[k]+5);
      fscanf(domainfile,"%lf%lf%lf%lf%lf%lf",
       zloc[k],zloc[k]+1,zloc[k]+2,zloc[k]+3,zloc[k]+4,zloc[k]+5);
      get_next_line(domainfile, buf);  /* get remainder of line */
      nverts[k] = 6;
    }
    else if(strstr(buf,"Pyr") || strstr(buf,"pyr")){
      fscanf(domainfile,"%lf%lf%lf%lf%lf",
       xloc[k],xloc[k]+1,xloc[k]+2,xloc[k]+3,xloc[k]+4);
      fscanf(domainfile,"%lf%lf%lf%lf%lf",
       yloc[k],yloc[k]+1,yloc[k]+2,yloc[k]+3,yloc[k]+4);
      fscanf(domainfile,"%lf%lf%lf%lf%lf",
       zloc[k],zloc[k]+1,zloc[k]+2,zloc[k]+3,zloc[k]+4);
      get_next_line(domainfile, buf);  /* get remainder of line */
      nverts[k] = 5;
    }
    else{ /* assume it is a tet */
      fscanf(domainfile,"%lf%lf%lf%lf",xloc[k],xloc[k]+1,xloc[k]+2,xloc[k]+3);
      fscanf(domainfile,"%lf%lf%lf%lf",yloc[k],yloc[k]+1,yloc[k]+2,yloc[k]+3);
      fscanf(domainfile,"%lf%lf%lf%lf",zloc[k],zloc[k]+1,zloc[k]+2,zloc[k]+3);
      get_next_line(domainfile, buf);  /* get remainder of line */
      nverts[k] = 4;
    }
  }

  /* search through elements and search for similar elements */

  vertids = imatrix(0,nel-1,0,Max_Nverts-1);
  ifill(nel*Max_Nverts,-1,vertids[0],1);

  totverts=0;
  for(k = 0; k < nel; ++k)
    for(i= 0; i < nverts[k]; ++i)
      if(vertids[k][i] == -1){
  vertids[k][i] = totverts++;
  for(l = k+1; l < nel; ++l)
    for(j = 0; j < nverts[l]; ++j)
      if(vertids[l][j] == -1){
        if(same(xloc[l][j],xloc[k][i]) && same(yloc[l][j],yloc[k][i])
     && same(zloc[l][j],zloc[k][i]))
    vertids[l][j] = vertids[k][i];
      }
      }

  xcoords = dvector(0, totverts-1);
  ycoords = dvector(0, totverts-1);
  zcoords = dvector(0, totverts-1);

  for(k = 0; k < nel; ++k)
    for(i= 0; i < nverts[k]; ++i){
      xcoords[vertids[k][i]] = xloc[k][i];
      ycoords[vertids[k][i]] = yloc[k][i];
      zcoords[vertids[k][i]] = zloc[k][i];
    }

  free_dmatrix(xloc,0,0);
  free_dmatrix(yloc,0,0);
  free_dmatrix(zloc,0,0);

  }
  else{
    char language;

    sprintf(buf, strtok(fname, "\n"));

    domainname = strdup(buf);
    domainfile = fopen(domainname, "r");

    get_next_line(domainfile, buf);
    sscanf(buf, "%c", &language);
    get_next_line(domainfile, buf);
    sscanf(buf, "%d", &totverts);

    xcoords = dvector(0, totverts-1);
    ycoords = dvector(0, totverts-1);
    zcoords = dvector(0, totverts-1);

     for(i=0;i<totverts;++i){
      get_next_line(domainfile, buf);
      sscanf(buf, "%lf %lf %lf", xcoords+i, ycoords+i, zcoords+i);
    }
    get_next_line(domainfile, buf);

    sscanf(buf, "%d", &nel);

    nverts  = ivector(0, nel-1);
    vertids = imatrix(0, nel-1, 0, Max_Nverts-1);
    ifill(nel*Max_Nverts, -1, vertids[0], 1);

    for(i=0;i<nel;++i){
      get_next_line(domainfile, buf);
      sscanf(buf, "%d", nverts+i);
      switch(nverts[i]){
      case 4:
  sscanf(buf, "%*d %d %d %d %d",
         vertids[i], vertids[i]+1, vertids[i]+2, vertids[i]+3);
  break;
      case 5:
  sscanf(buf, "%*d %d %d %d %d %d",
         vertids[i], vertids[i]+1, vertids[i]+2, vertids[i]+3,
         vertids[i]+4);
  break;
      case 6:
  sscanf(buf, "%*d %d %d %d %d %d %d",
         vertids[i], vertids[i]+1, vertids[i]+2, vertids[i]+3,
         vertids[i]+4, vertids[i]+5);
  break;
      case 8:
  sscanf(buf, "%*d %d %d %d %d %d %d %d %d",
         vertids[i], vertids[i]+1, vertids[i]+2, vertids[i]+3,
         vertids[i]+4, vertids[i]+5, vertids[i]+6, vertids[i]+7);
  break;
      }
      if(language == 'F'){
  for(j=0;j<nverts[i];++j)
    vertids[i][j] -=1;
      }
    }
  }

  elmtids = ivector(0, nel-1);
  for(i=0;i<nel;++i)
    elmtids[i] = i;

  vertexmap = imatrix(0, nel-1, 0, Max_Nverts-1);
  ifill(nel*Max_Nverts, -1, vertexmap[0], 1);
  for(i=0;i<nel;++i)
    for(j=0;j<nverts[i];++j)
      vertexmap[i][j] = j;

}


Grid::Grid(Grid *Orig){
  int i;
  domainname = strdup(Orig->domainname);
  domainfile = Orig->domainfile;
  totverts   = Orig->totverts;

  xcoords = dvector(0, totverts-1);
  ycoords = dvector(0, totverts-1);
  zcoords = dvector(0, totverts-1);

  dcopy(totverts, Orig->xcoords, 1, xcoords, 1);
  dcopy(totverts, Orig->ycoords, 1, ycoords, 1);
  dcopy(totverts, Orig->zcoords, 1, zcoords, 1);

  nel = Orig->nel;
  nverts = ivector(0, nel-1);
  icopy(nel, Orig->nverts, 1, nverts, 1);

  vertids = imatrix(0, nel-1, 0, Max_Nverts-1);
  icopy(nel*Max_Nverts, Orig->vertids[0], 1, vertids[0], 1);

  elmtids = ivector(0, nel-1);
  icopy(nel, Orig->elmtids, 1, elmtids, 1);

  vertexmap = imatrix(0, nel-1, 0, Max_Nverts-1);
  icopy(nel*Max_Nverts, Orig->vertexmap[0], 1, vertexmap[0], 1);
}

void Grid::RemoveHexes(){
  int i=0;
  do{
    if(nverts[i] == 8){
      icopy(nel-i-1, elmtids+i+1, 1, elmtids+i, 1);
      icopy(nel-i-1, nverts+i+1, 1, nverts+i, 1);
      icopy((nel-i-1)*Max_Nverts, vertids[i]+Max_Nverts, 1, vertids[i], 1);
      --nel;
    }
    else
      ++i;
  }
  while(i<nel);
}

void Grid::RemovePrisms(){
  int i=0,j;
  int vertextoremove;
  int vertextouse;
  int vertexa, vertexb;

  do{
    if(nverts[i] == 6){
#if 0
      vertexa = vertids[i][0];
      vertexb = vertids[i][3];

      vertextoremove = min(vertexa, vertexb);
      vertextouse    = max(vertexa, vertexb);
      for(j=0;j<Max_Nverts*nel;++j)
  vertids[0][j] =
    (vertids[0][j] == vertextoremove) ? vertextouse: vertids[0][j];

      vertexa = vertids[i][1];
      vertexb = vertids[i][2];

      vertextoremove = min(vertexa, vertexb);
      vertextouse    = max(vertexa, vertexb);

      for(j=0;j<Max_Nverts*nel;++j)
  vertids[0][j] =
    (vertids[0][j] == vertextoremove) ? vertextouse: vertids[0][j];

      vertexa = vertids[i][4];
      vertexb = vertids[i][5];

      vertextoremove = min(vertexa, vertexb);
      vertextouse    = max(vertexa, vertexb);

      for(j=0;j<Max_Nverts*nel;++j)
  vertids[0][j] =
    (vertids[0][j] == vertextoremove) ? vertextouse: vertids[0][j];
#endif
      icopy(nel-i-1, elmtids+i+1, 1, elmtids+i, 1);
      icopy(nel-i-1, nverts+i+1, 1, nverts+i, 1);
      icopy((nel-i-1)*Max_Nverts, vertids[i]+Max_Nverts, 1, vertids[i], 1);
      icopy((nel-i-1)*Max_Nverts, vertexmap[i]+Max_Nverts, 1, vertexmap[i], 1);

      --nel;
    }
    else
      ++i;
  }
  while(i<nel);
}

void Grid::RenumberPrisms(){
  int i,j;
  int vertextoremove;
  int vertextouse;
  int vertexa, vertexb;

  int *map = ivector(0, totverts-1);
  for(i=0;i<totverts;++i)
    map[i] = i;

  for(i=0;i<nel;++i){
    if(nverts[i] == 6){
      vertexa = map[vertids[i][0]];
      vertexb = map[vertids[i][3]];

      vertextoremove = min(vertexa, vertexb);
      vertextouse    = max(vertexa, vertexb);
      map[vertextoremove] = vertextouse;

      vertexa = map[vertids[i][1]];
      vertexb = map[vertids[i][2]];

      vertextoremove = min(vertexa, vertexb);
      vertextouse    = max(vertexa, vertexb);
      map[vertextoremove] = vertextouse;

      vertexa = map[vertids[i][4]];
      vertexb = map[vertids[i][5]];

      vertextoremove = min(vertexa, vertexb);
      vertextouse    = max(vertexa, vertexb);
      map[vertextoremove] = vertextouse;
    }
  }
  for(i=0;i<nel;++i)
    for(j=0;j<nverts[i];++j)
      vertids[i][j] = map[vertids[i][j]];
  free(map);
}

double checktetvolume(int *gid, double *x, double *y, double *z){
  double ax = x[gid[1]]-x[gid[0]];
  double ay = y[gid[1]]-y[gid[0]];
  double az = z[gid[1]]-z[gid[0]];

  double bx = x[gid[2]]-x[gid[0]];
  double by = y[gid[2]]-y[gid[0]];
  double bz = z[gid[2]]-z[gid[0]];

  double cx = x[gid[3]]-x[gid[0]];
  double cy = y[gid[3]]-y[gid[0]];
  double cz = z[gid[3]]-z[gid[0]];

  double dx, dy, dz, d;

  dx = ay*bz-az*by;
  dy = az*bx-ax*bz;
  dz = ax*by-ay*bx;

  d = cx*dx+cy*dy+cz*dz;
  return d;
}


void Grid::FixTetOrientation(){

  int i,j,k, tmp;

  // max vertex becomes top

  int *lid = ivector(0, 4-1);
  int *gid = ivector(0, 4-1);

  for(i=0;i<nel;++i){
    icopy(4, vertids[i], 1, gid, 1);

    for(j=0;j<4;++j)
      lid[j] = j;

    // order the local ids (minimum global id first)
    for(j=0;j<4;++j)
      for(k=j+1;k<4;++k){
  if(gid[j]>gid[k]){
    tmp    = gid[k];
    gid[k] = gid[j];
    gid[j] = tmp;

    tmp    = lid[k];
    lid[k] = lid[j];
    lid[j] = tmp;
  }
      }
    if(checktetvolume(gid, xcoords, ycoords, zcoords) < 0){
      tmp = lid[0];
      lid[0] = lid[1];
      lid[1] = tmp;
    }

    icopy(4, lid, 1, vertexmap[i], 1);
  }
  free(lid);
  free(gid);
}

void Grid::ImportTetOrientation(Grid *fromgrid){
  int i,j;
  int lid;
  for(i=0;i<fromgrid->nel;++i){
    lid = fromgrid->elmtids[i];
    icopy(4, fromgrid->vertexmap[i], 1, vertexmap[lid], 1);
  }
}

void Grid::ImportPrismOrientation(Grid *fromgrid){
  int i,j;
  int lid;
  for(i=0;i<fromgrid->nel;++i){
    if(fromgrid->nverts[i] == 6){
      lid = fromgrid->elmtids[i];
      icopy(6, fromgrid->vertexmap[i], 1, vertexmap[lid], 1);
    }
  }
}

void Grid::FixPrismOrientation(){
  int i, j, k, tmp;
  int *lid = ivector(0, 6-1);
  int *gid = ivector(0, 6-1);

  for(i=0;i<nel;++i){
    if(nverts[i] == 6){

      icopy(6, vertids[i], 1, gid, 1);

      for(j=0;j<6;++j)
  lid[j] = j;

      // order the local ids (maximum global id first)
      for(k=1;k<6;++k){
  if(gid[0]<gid[k]){
    tmp    = gid[k];
    gid[k] = gid[0];
    gid[0] = tmp;

    tmp    = lid[k];
    lid[k] = lid[0];
    lid[0] = tmp;
  }
      }
      // we now know that the local id of the maximum global id is in lid[0]
      if(lid[0] == 4 || lid[0] == 5){
  // nothing to do
      }
      else if(lid[0] == 1 || lid[0] == 2){
  // must rotate around clockwise
  vertexmap[i][0] = 4;
  vertexmap[i][1] = 0;
  vertexmap[i][2] = 3;
  vertexmap[i][3] = 5;
  vertexmap[i][4] = 1;
  vertexmap[i][5] = 2;
      }
      else if(lid[0] == 0 || lid[0] == 3){
  // must rotate around clockwise
  vertexmap[i][0] = 1;
  vertexmap[i][1] = 4;
  vertexmap[i][2] = 5;
  vertexmap[i][3] = 2;
  vertexmap[i][4] = 0;
  vertexmap[i][5] = 3;
      }

    }
  }
  free(lid);
  free(gid);
}

Element_List *Grid::gen_aux_field(){
  int     L, qa, qb, qc=0, k;
  char    buf[BUFSIZ];
  char    buf_a[BUFSIZ];
  Element **new_E;
  register int i;

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

  if(qc = iparam("NQUAD"));
  else qc = L;

  iparam_set("ELEMENTS", nel);
  /* Set up a new element vector */
  QGmax = max(max(qa,qb),qc);
  LGmax = L;

  new_E = (Element**) malloc(nel*sizeof(Element*));

  Coord X;
  X.x = dvector(0,Max_Nverts-1);
  X.y = dvector(0,Max_Nverts-1);
  X.z = dvector(0,Max_Nverts-1);

  /* Read in mesh information */
  for(k = 0; k < nel; k++) {
    for(i=0;i<nverts[k];++i){
      X.x[i] = xcoords[vertids[k][vertexmap[k][i]]];
      X.y[i] = ycoords[vertids[k][vertexmap[k][i]]];
      X.z[i] = zcoords[vertids[k][vertexmap[k][i]]];
    }

    switch(nverts[k]){
    case 4:
      new_E[k]   = new       Tet(k,'u', L, qa, qb, qb, &X);
      break;
    case 8:
      new_E[k]   = new       Hex(k,'u', L, qa, qa, qa, &X);
      break;
    case 6:
      new_E[k]   = new       Prism(k,'u', L, qa, qa, qb, &X);
      break;
    case 5:
      new_E[k]   = new       Pyr(k,'u', L, qa, qa, qb, &X);
      break;
    }
  }

  for(k = 0; k < nel-1; ++k)     new_E[k]->next = new_E[k+1];
  new_E[k]->next = (Element*) NULL;

  Element_List* E_List = (Element_List*) new Element_List(new_E, nel);

  E_List->Cat_mem();
  Tri_work();
  Quad_work();

  for(k = 0; k < nel; ++k)
    new_E[k]->set_curved_elmt(E_List);
  for(k = 0; k < nel; ++k)
    new_E[k]->set_geofac();

  return E_List;
}

#if 0
void Grid::Connect(){
  flag = ivector(0, Ntet-1);
  izero( Ntet, flag, 1);
  fprintf(stderr, "Doing the conn. job\n");
  for(i=0;i<Ntet;++i){

    if(!(i%100))
      fprintf(stderr, ".");

    va = emap[i][0]-1;
    vb = emap[i][1]-1;
    vc = emap[i][2]-1;

    for(j=0;j<cnt[va];++j)
      flag[vertintet[va][j]] = 0;

    for(j=0;j<cnt[vb];++j)
      flag[vertintet[vb][j]] = 0;

    for(j=0;j<cnt[vc];++j)
      flag[vertintet[vc][j]] = 0;

    for(j=0;j<cnt[va];++j)
      ++flag[vertintet[va][j]];

    for(j=0;j<cnt[vb];++j)
      ++flag[vertintet[vb][j]];

    for(j=0;j<cnt[vc];++j)
      ++flag[vertintet[vc][j]];

    fl = 0;
    for(j=0;j<cnt[va];++j)
      if(flag[vertintet[va][j]] == 3 && vertintet[va][j] != i ){
  conn[i][0] = vertintet[va][j];
  fl = 1;
  break;
      }
    if(!fl)
    for(j=0;j<cnt[vb];++j)
      if(flag[vertintet[vb][j]] == 3 && vertintet[vb][j] != i ){
  conn[i][0] = vertintet[vb][j];
  fl = 1;
  break;
      }
    if(!fl)
    for(j=0;j<cnt[vc];++j)
      if(flag[vertintet[vc][j]] == 3 && vertintet[vc][j] != i ){
  conn[i][0] = vertintet[vc][j];
  fl = 1;
  break;
      }

    va = emap[i][0]-1;
    vb = emap[i][1]-1;
    vc = emap[i][3]-1;

    for(j=0;j<cnt[va];++j)
      flag[vertintet[va][j]] = 0;

    for(j=0;j<cnt[vb];++j)
      flag[vertintet[vb][j]] = 0;

    for(j=0;j<cnt[vc];++j)
      flag[vertintet[vc][j]] = 0;


    for(j=0;j<cnt[va];++j)
      ++flag[vertintet[va][j]];

    for(j=0;j<cnt[vb];++j)
      ++flag[vertintet[vb][j]];

    for(j=0;j<cnt[vc];++j)
      ++flag[vertintet[vc][j]];


    fl = 0;
    for(j=0;j<cnt[va];++j)
      if(flag[vertintet[va][j]] == 3 && vertintet[va][j] != i ){
  conn[i][1] = vertintet[va][j];
  fl = 1;
  break;
      }
    if(!fl)
    for(j=0;j<cnt[vb];++j)
      if(flag[vertintet[vb][j]] == 3 && vertintet[vb][j] != i ){
  conn[i][1] = vertintet[vb][j];
  fl = 1;
  break;
      }
    if(!fl)
    for(j=0;j<cnt[vc];++j)
      if(flag[vertintet[vc][j]] == 3 && vertintet[vc][j] != i ){
  conn[i][1] = vertintet[vc][j];
  fl = 1;
  break;
      }

    va = emap[i][1]-1;
    vb = emap[i][2]-1;
    vc = emap[i][3]-1;

    for(j=0;j<cnt[va];++j)
      flag[vertintet[va][j]] = 0;

    for(j=0;j<cnt[vb];++j)
      flag[vertintet[vb][j]] = 0;

    for(j=0;j<cnt[vc];++j)
      flag[vertintet[vc][j]] = 0;


    for(j=0;j<cnt[va];++j)
      ++flag[vertintet[va][j]];

    for(j=0;j<cnt[vb];++j)
      ++flag[vertintet[vb][j]];

    for(j=0;j<cnt[vc];++j)
      ++flag[vertintet[vc][j]];

    fl = 0;
    for(j=0;j<cnt[va];++j)
      if(flag[vertintet[va][j]] == 3 && vertintet[va][j] != i ){
  conn[i][2] = vertintet[va][j];
  fl = 1;
  break;
      }
    if(!fl)
    for(j=0;j<cnt[vb];++j)
      if(flag[vertintet[vb][j]] == 3 && vertintet[vb][j] != i ){
  conn[i][2] = vertintet[vb][j];
  fl = 1;
  break;
      }
    if(!fl)
    for(j=0;j<cnt[vc];++j)
      if(flag[vertintet[vc][j]] == 3 && vertintet[vc][j] != i ){
  conn[i][2] = vertintet[vc][j];
  fl = 1;
  break;
      }

    va = emap[i][0]-1;
    vb = emap[i][2]-1;
    vc = emap[i][3]-1;

    for(j=0;j<cnt[va];++j)
      flag[vertintet[va][j]] = 0;

    for(j=0;j<cnt[vb];++j)
      flag[vertintet[vb][j]] = 0;

    for(j=0;j<cnt[vc];++j)
      flag[vertintet[vc][j]] = 0;


    for(j=0;j<cnt[va];++j)
      ++flag[vertintet[va][j]];

    for(j=0;j<cnt[vb];++j)
      ++flag[vertintet[vb][j]];

    for(j=0;j<cnt[vc];++j)
      ++flag[vertintet[vc][j]];

        fl = 0;
    for(j=0;j<cnt[va];++j)
      if(flag[vertintet[va][j]] == 3 && vertintet[va][j] != i ){
  conn[i][3] = vertintet[va][j];
  fl = 1;
  break;
      }
    if(!fl)
    for(j=0;j<cnt[vb];++j)
      if(flag[vertintet[vb][j]] == 3 && vertintet[vb][j] != i ){
  conn[i][3] = vertintet[vb][j];
  fl = 1;
  break;
      }
    if(!fl)
    for(j=0;j<cnt[vc];++j)
      if(flag[vertintet[vc][j]] == 3 && vertintet[vc][j] != i ){
  conn[i][3] = vertintet[vc][j];
  fl = 1;
  break;
      }
  }
}
#endif

main (int argc, char *argv[]){
  int  i,j,k;

  manager_init();
  parse_util_args(argc, argv);

  iparam_set("LQUAD",3);
  iparam_set("MQUAD",3);
  iparam_set("NQUAD",3);
  iparam_set("MODES",iparam("LQUAD")-1);

  char *buf = (char*) calloc(BUFSIZ, sizeof(char));
  char *fname = (char*) calloc(BUFSIZ, sizeof(char));
  get_string("Enter name of input file", buf);
  sprintf(fname, strtok(buf, "\n"));

  Grid *grid  = new Grid(fname);
  Grid *grida = new Grid(grid);

  grida->RenumberPrisms();
  grida->FixPrismOrientation();
  grid->ImportPrismOrientation(grida);
  grida->RemovePrisms();

  grida->RemoveHexes();
  grida->FixTetOrientation();
  grid->ImportTetOrientation(grida);

  Element_List *U =  grid->gen_aux_field();
  int nel = U->nel;

  get_string("Enter name of output file", buf);
  sprintf(fname, strtok(buf, "\n"));
  FILE *fout = fopen(fname, "w");


  int Nfields, Nbdry;
  get_int   ("Enter number of fields", &Nfields);
  get_int   ("Enter number of boundaries (including default)", &Nbdry);

  char **eqn = (char**) calloc(Nfields, sizeof(char*));

  for(i=0;i<Nfields;++i){
    eqn[i] = (char*)calloc(BUFSIZ, sizeof(char));
  }

  Bndry *Bc;
  Bndry **Vbc = (Bndry**) calloc(Nfields, sizeof(Bndry*));
  char *bndryeqn = (char*) calloc(BUFSIZ, sizeof(char));
  int **bcmatrix = imatrix(0, U->nel-1, 0, Max_Nfaces-1);
  izero(U->nel*Max_Nfaces, bcmatrix[0], 1);

  int nb;
  char type;
  char curved, curvetype;
  Curve *cur;
  int curveid = 0;

  for(nb=0;nb<Nbdry;++nb){
    if(nb != Nbdry-1){
      fprintf(stderr, "#\nBoundary: %d\n#\n", nb+1);
      get_string("Enter function which has roots at boundary", bndryeqn);
      fprintf(stderr, "#\n");
    }
    else{
      fprintf(stderr, "#\nDefault Boundary:\n#\n");
    }
    get_char("Enter character type\n(v=velocity)\n"
       "(W=wall)\n(O=outflow)\n(s=flux (Compressible only))\n",
       &type);
    fprintf(stderr, "#\n");
    switch(type){
    case 'W': case 'O':
      break;
    case 'v': case 's':
      for(i=0;i<Nfields;++i){
  get_string("Enter function definition", eqn[i]);
  fprintf(stderr, "\n");
      }
    }

    get_char("Is this boundary curved (y/n)?", &curved);
    if(curved == 'y'){
      ++curveid;
      get_char("Enter curve type\n(S=sphere)\n(C=cylinder)\n(T=taurus)\n",
         &curvetype);
      switch(curvetype){
      case 'S':{
  double cx, cy, cz, cr;
  get_double("Enter center x-coord", &cx);
  get_double("Enter center y-coord", &cy);
  get_double("Enter center z-coord", &cz);
  get_double("Enter radius", &cr);
  cur = (Curve*) calloc(1, sizeof(Curve));
  cur->type = T_Sphere;
  cur->info.sph.xc = cx;
  cur->info.sph.yc = cy;
  cur->info.sph.zc = cz;
  cur->info.sph.radius = cr;
  cur->id = curveid;
  break;
      }
      case 'C':{
  double cx, cy, cz, cr;
  double ax, ay, az;
  get_double("Enter point on axis x-coord", &cx);
  get_double("Enter point on axis y-coord", &cy);
  get_double("Enter point on axis z-coord", &cz);
  get_double("Enter axis vector x-coord", &ax);
  get_double("Enter axis vector y-coord", &ay);
  get_double("Enter axis vector z-coord", &az);
  get_double("Enter radius", &cr);

  cur = (Curve*) calloc(1, sizeof(Curve));
  cur->type = T_Cylinder;
  cur->info.cyl.xc = cx;
  cur->info.cyl.yc = cy;
  cur->info.cyl.zc = cz;
  cur->info.cyl.ax = ax;
  cur->info.cyl.ay = ay;
  cur->info.cyl.az = az;
  cur->info.cyl.radius = cr;
  cur->id = curveid;
  break;
      }
      }

    }
    if(nb == Nbdry-1)
      break;

    double res;
    Element *E;
    for(E=U->fhead;E;E=E->next){
      for(i=0;i<E->Nfaces;++i){
  for(j=0;j<E->Nfverts(i);++j){
    vector_def("x y z",bndryeqn);
    vector_set(1,
         &(E->vert[E->fnum(i,j)].x),
         &(E->vert[E->fnum(i,j)].y),
         &(E->vert[E->fnum(i,j)].z),
         &res);
    if(fabs(res)> TOL)
      break;
  }
  if(j==E->Nfverts(i)){
    if(curved == 'y'){
      E->curve = (Curve*) calloc(1, sizeof(Curve));
      memcpy(E->curve, cur, sizeof(Curve));
      E->curve->face = i;
    }
    switch(type){
    case 'W': case 'O':
      for(j=0;j<Nfields;++j){
        Bc = E->gen_bndry(type, i, 0.);
        Bc->type = type;
        add_bc(&Vbc[j], Bc);
      }
      bcmatrix[E->id][i] = 1;
      break;
    case 'v': case 's':
      for(j=0;j<Nfields;++j){
        Bc = E->gen_bndry(type, i, eqn[j]);
        Bc->type = type;
        add_bc(&Vbc[j], Bc);
      }
      bcmatrix[E->id][i] = 1;
      break;
    }
  }
      }
    }
  }

  char is_periodic;
  get_char("Is there periodicity in the x-direction (y/n)?", &is_periodic);
  if(is_periodic == 'y')
    get_double("Enter periodic length",&XPERIOD);
  get_char("Is there periodicity in the y-direction (y/n)?", &is_periodic);
  if(is_periodic == 'y')
    get_double("Enter periodic length",&YPERIOD);
  get_char("Is there periodicity in the z-direction (y/n)?", &is_periodic);
  if(is_periodic == 'y')
    get_double("Enter periodic length",&ZPERIOD);

  // Do remaining connections
  Element *E, *F;
  for(E=U->fhead;E;E=E->next)
    for(i=0;i<E->Nfaces;++i)
      if(!bcmatrix[E->id][i])
  for(F=E;F;F=F->next)
    for(j=0;j<F->Nfaces;++j)
      if(!bcmatrix[F->id][j])
        if(neighbourtest(E,i,F,j) && !(E->id == F->id && i==j)){
    bcmatrix[E->id][i] = 2;
    bcmatrix[F->id][j] = 2;
    set_link(E,i,F,j);
    break;
        }

  // if the default bc is curved the make default bndries curved
  if(curved == 'y'){
    for(E=U->fhead;E;E=E->next)
      for(i=0;i<E->Nfaces;++i)
  if(!bcmatrix[E->id][i]){
    E->curve = (Curve*) calloc(1, sizeof(Curve));
    memcpy(E->curve, cur, sizeof(Curve));
    E->curve->face = i;
  }
  }

  fprintf(fout, "****** PARAMETERS *****\n");
  fprintf(fout, " SolidMes \n");
  fprintf(fout, " 3 DIMENSIONAL RUN\n");
  fprintf(fout, " 0 PARAMETERS FOLLOW\n");
  fprintf(fout, "0  Lines of passive scalar data follows2 CONDUCT; 2RHOCP\n");
  fprintf(fout, " 0  LOGICAL SWITCHES FOLLOW\n");
  fprintf(fout, "Dummy line from old nekton file\n");
  fprintf(fout, "**MESH DATA** x,y,z, values of vertices 1,2,3,4.\n");
  fprintf(fout, "%d   3       1   NEL NDIM NLEVEL\n", U->nel);


  for(E=U->fhead;E;E=E->next){
    switch(E->identify()){
    case Nek_Tet:
      fprintf(fout, "Element %d Tet\n", E->id+1);
      break;
    case Nek_Pyr:
      fprintf(fout, "Element %d Pyr\n", E->id+1);
      break;
    case Nek_Prism:
      fprintf(fout, "Element %d Prism\n", E->id+1);
      break;
    case Nek_Hex:
      fprintf(fout, "Element %d Hex\n", E->id+1);
      break;
    }

    for(i=0;i<E->Nverts;++i)
      fprintf(fout, "%lf ", E->vert[i].x);
    fprintf(fout, "\n");
    for(i=0;i<E->Nverts;++i)
      fprintf(fout, "%lf ", E->vert[i].y);
    fprintf(fout, "\n");
    for(i=0;i<E->Nverts;++i)
      fprintf(fout, "%lf ", E->vert[i].z);
    fprintf(fout, "\n");
  }

  fprintf(fout, "***** CURVED SIDE DATA ***** \n");

  fprintf(fout, "%d Number of curve types\n", curveid);

  for(i=0;i<curveid;++i){
    int flag = 0;
    for(E=U->fhead;!flag && E;E=E->next){
      if(E->curve && E->curve->type != T_Straight){
  if(E->curve->id == i+1){
    switch(E->curve->type){
    case T_Sphere:
      fprintf(fout, "Sphere\n");
      fprintf(fout, "%lf %lf %lf %lf %c\n",
        E->curve->info.sph.xc,
        E->curve->info.sph.yc,
        E->curve->info.sph.zc,
        E->curve->info.sph.radius,
        'a'+i+1);
      flag = 1;
      break;
    case T_Cylinder:
      fprintf(fout, "Cylinder\n");
      fprintf(fout, "%lf %lf %lf   %lf %lf %lf %lf %c\n",
        E->curve->info.cyl.xc,
          E->curve->info.cyl.yc,
        E->curve->info.cyl.zc,
        E->curve->info.cyl.ax,
        E->curve->info.cyl.ay,
        E->curve->info.cyl.az,
        E->curve->info.cyl.radius,
          'a'+i+1);
      flag = 1;
      break;
    }
  }
      }
    }
  }
  int ncurvedsides = 0;
  for(E=U->fhead;E;E=E->next){
    if(E->curve && E->curve->type != T_Straight)
      ++ncurvedsides;
  }

  fprintf(fout, "%d Curved sides follow\n", ncurvedsides);
  for(E=U->fhead;E;E=E->next)
    if(E->curve && E->curve->type != T_Straight)
      fprintf(fout, "%d %d %c\n", E->curve->face+1, E->id+1, 'a'+E->curve->id);

  fprintf(fout, "***** BOUNDARY CONDITIONS ***** \n");
  fprintf(fout, "***** FLUID BOUNDARY CONDITIONS ***** \n");
  for(E=U->fhead;E;E=E->next){
    for(j=0;j<E->Nfaces;++j){
      if(bcmatrix[E->id][j] == 2){
  fprintf(fout, "E %d %d %d %d\n", E->id+1, j+1,
    E->face[j].link->eid+1, E->face[j].link->id+1);
      }
      else if(bcmatrix[E->id][j] == 1){
  for(i=0;i<Nfields;++i)
    for(Bc=Vbc[i];Bc;Bc=Bc->next){
      if(Bc->elmt == E && Bc->face == j){
        if(i==0)
    switch(Bc->type){
    case 'W':
      fprintf(fout, "W %d %d 0. 0. 0.\n", E->id+1, j+1);
      break;
    case 'O':
      fprintf(fout, "O %d %d 0. 0. 0.\n", E->id+1, j+1);
      break;
    case 'v':
      fprintf(fout, "v %d %d 0. 0. 0.\n", E->id+1, j+1);
      break;
    case 's':
      fprintf(fout, "s %d %d 0. 0. 0.\n", E->id+1, j+1);
      break;
    }
        if(Bc->type == 's' || Bc->type == 'v')
    fprintf(fout, "%c=%s\n", 'u'+i,Bc->bstring);
        break;
      }
    }
      }
      else{
  switch(type){
  case 'W':
    fprintf(fout, "W %d %d 0. 0. 0.\n", E->id+1, j+1);
    break;
  case 'O':
    fprintf(fout, "O %d %d 0. 0. 0.\n", E->id+1, j+1);
    break;
  case 'v':
    fprintf(fout, "v %d %d 0. 0. 0.\n", E->id+1, j+1);
    break;
  case 's':
    fprintf(fout, "s %d %d 0. 0. 0.\n", E->id+1, j+1);
    break;
  }
  if(type == 's' || type == 'v')
    for(i=0;i<Nfields;++i)
      fprintf(fout, "%c=%s\n", 'u'+i,eqn[i]);
      }
    }
  }
  char bufa[BUFSIZ];

  fprintf(fout, "***** NO THERMAL BOUNDARY CONDITIONS *****\n");
  get_char("Are you using a field file restart (y/n)?", &type);

  if(type == 'y'){
    fprintf(fout, "%d         INITIAL CONDITIONS *****\n",1);

    get_string("Enter name of restart file:", buf);
    fprintf(fout, "Restart\n");
    fprintf(fout, "%s\n", buf);
  }
  else{
    fprintf(fout, "%d         INITIAL CONDITIONS *****\n",1+Nfields);
    fprintf(fout, "Given\n");

    for(i=0;i<Nfields;++i){
      sprintf(bufa, "Field %d ", i+1);
      get_string(bufa, buf);
      fprintf(fout, "%s\n",  buf);
    }
  }

  fprintf(fout, "***** DRIVE FORCE DATA ***** PRESSURE GRAD, FLOW, Q\n");
  get_char("Are you using a forcing function (y/n)?", &type);

  if(type == 'y'){
    fprintf(fout, "%d                Lines of Drive force data follow\n",
      Nfields);

    for(i=0;i<Nfields;++i){
      sprintf(bufa, "FF%c = ", 'X'+i);
      get_string(bufa,buf);
      fprintf(fout, "FF%c = %s\n",'X'+i, buf);
    }
  }
  else{
    fprintf(fout, "0                 Lines of Drive force data follow\n");
  }

  fprintf(fout, "***** Variable Property Data ***** Overrrides Parameter data.\n");
  fprintf(fout, " 1 Lines follow.\n");
  fprintf(fout, " 0 PACKETS OF DATA FOLLOW\n");
  fprintf(fout, "***** HISTORY AND INTEGRAL DATA *****\n");
   get_char("Are you using history points (y/n)?", &type);
  if(type == 'y'){
    int npoints;

    get_int ("Enter number of points", &npoints);
    fprintf(fout, " %d   POINTS.  Hcode, I,J,H,IEL\n", npoints);
    for(i=0;i<npoints;++i){
      sprintf(bufa, "Enter element number for point %d", i+1);
      get_int(bufa,&j);
      sprintf(bufa, "Enter vertex  number for point %d", i+1);
      get_int(bufa, &k);
      fprintf(fout, "UVWP H %d 1 1 %d\n", k, j);
    }
  }
  else{
    fprintf(fout, " 0   POINTS.  Hcode, I,J,H,IEL\n");
  }

  fprintf(fout, " ***** OUTPUT FIELD SPECIFICATION *****\n");
  fprintf(fout, "  0 SPECIFICATIONS FOLLOW\n");

  return 0;
}

/* --------------------------------------------------------------------- *
 * parse_args() -- Parse application arguments                           *
 *                                                                       *
 * This program only supports the generic utility arguments.             *
 * --------------------------------------------------------------------- */

static void parse_util_args (int argc, char *argv[]){
  char  c;
  int   i;
  char  fname[FILENAME_MAX];

  if (argc == 0) {
    fputs (usage, stderr);
    exit  (1);
  }

  while (--argc && (*++argv)[0] == '-') {
    while (c = *++argv[0])
      switch (c) {
      case 'r': /* use rea files as input */
  option_set("REAFILE",1);
  break;
      default:
  fprintf(stderr, "%s: unknown option -- %c\n", prog, c);
  break;
      }
  }

  return;
}
