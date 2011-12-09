#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <veclib.h>

#define max(a,b)  (a>b) ? a:b

char *prog   = "";
char *usage  = "";
char *author = "";
char *rcsid  = "";
char *help   = "";

#define MAXSURF 100

double determ(double *x, double *y, double *z,
        int va, int vb, int vc, int vd){

  double ax = x[vb]-x[va];
  double ay = y[vb]-y[va];
  double az = z[vb]-z[va];

  double bx = x[vc]-x[va];
  double by = y[vc]-y[va];
  double bz = z[vc]-z[va];

  double cx = x[vd]-x[va];
  double cy = y[vd]-y[va];
  double cz = z[vd]-z[va];

  double dx, dy, dz, d;

  dx = ay*bz-az*by;
  dy = az*bx-ax*bz;
  dz = ax*by-ay*bx;

  d = cx*dx+cy*dy+cz*dz;
  return d;
}

int find_match_square(int va, int vb, int vc, int vd, int *flag,
          int **elmts_vertisin, int *cnt, int id){
  int j,fl;

  /* zero flags  for all elements containing va, vb and vc */
  for(j=0;j<cnt[va];++j)
    flag[elmts_vertisin[va][j]] = 0;

  for(j=0;j<cnt[vb];++j)
    flag[elmts_vertisin[vb][j]] = 0;

  for(j=0;j<cnt[vc];++j)
    flag[elmts_vertisin[vc][j]] = 0;

  for(j=0;j<cnt[vd];++j)
    flag[elmts_vertisin[vd][j]] = 0;

  /* increment flags for all elements containing va, vb or vc */
  for(j=0;j<cnt[va];++j)
    ++flag[elmts_vertisin[va][j]];

  for(j=0;j<cnt[vb];++j)
    ++flag[elmts_vertisin[vb][j]];

  for(j=0;j<cnt[vc];++j)
    ++flag[elmts_vertisin[vc][j]];

  for(j=0;j<cnt[vd];++j)
    ++flag[elmts_vertisin[vd][j]];

  /* check to see which element has all four vertices in */
  fl = 0;
  for(j=0;j<cnt[va];++j)
    if(flag[elmts_vertisin[va][j]] == 4 && elmts_vertisin[va][j] != id )
      return elmts_vertisin[va][j];
  for(j=0;j<cnt[vb];++j)
    if(flag[elmts_vertisin[vb][j]] == 4 && elmts_vertisin[vb][j] != id )
      return elmts_vertisin[vb][j];

  for(j=0;j<cnt[vc];++j)
    if(flag[elmts_vertisin[vc][j]] == 4 && elmts_vertisin[vc][j] != id )
      return elmts_vertisin[vc][j];

  for(j=0;j<cnt[vd];++j)
    if(flag[elmts_vertisin[vd][j]] == 4 && elmts_vertisin[vd][j] != id)
      return elmts_vertisin[vd][j];

  return -MAXSURF;
}

int find_match_tri(int va, int vb, int vc, int *flag, int **elmts_vertisin, int *cnt, int id){
  int j,fl;

  /* zero flags  for all elements containing va, vb and vc */
  for(j=0;j<cnt[va];++j)
    flag[elmts_vertisin[va][j]] = 0;

  for(j=0;j<cnt[vb];++j)
    flag[elmts_vertisin[vb][j]] = 0;

  for(j=0;j<cnt[vc];++j)
    flag[elmts_vertisin[vc][j]] = 0;


  /* increment flags for all elements containing va, vb or vc */
  for(j=0;j<cnt[va];++j)
    ++flag[elmts_vertisin[va][j]];

  for(j=0;j<cnt[vb];++j)
    ++flag[elmts_vertisin[vb][j]];

  for(j=0;j<cnt[vc];++j)
    ++flag[elmts_vertisin[vc][j]];

  /* check to see which element has all three vertices in */
  fl = 0;
  for(j=0;j<cnt[va];++j)
    if(flag[elmts_vertisin[va][j]] == 3 && elmts_vertisin[va][j] != id )
      return elmts_vertisin[va][j];

  for(j=0;j<cnt[vb];++j)
    if(flag[elmts_vertisin[vb][j]] == 3 && elmts_vertisin[vb][j] != id )
      return elmts_vertisin[vb][j];

  for(j=0;j<cnt[vc];++j)
    if(flag[elmts_vertisin[vc][j]] == 3 && elmts_vertisin[vc][j] != id )
      return elmts_vertisin[vc][j];

  return -MAXSURF;
}


int match_tri(int va, int vb, int vc, int nc, int **ids){
  register int i;

  for(i = 0; i < nc; ++i){
    if(va == ids[i][0])
      if((vb==ids[i][1]&&vc==ids[i][2])||(vc==ids[i][1]&&vb==ids[i][2]))
  return ids[i][3];
    if(vb == ids[i][0])
      if((va==ids[i][1]&&vc==ids[i][2])||(vc==ids[i][1]&&va==ids[i][2]))
  return ids[i][3];
    if(vc == ids[i][0])
      if((va==ids[i][1]&&vb==ids[i][2])||(va==ids[i][1]&&vb==ids[i][2]))
  return ids[i][3];
  }

  return MAXSURF;
}

void writebc(FILE *reaout, int conn,int i, int face){
  switch(conn){
  case MAXSURF:
    fprintf(stderr,"error in determining surface in elmt %d face %d\n",
      i+1,face);
    fprintf(reaout, "W %d %d 0. 0. 0.\n", i+1, face);
  break;
  case 11:
    fprintf(reaout, "v %d %d 0. 0. 0.\n", i+1, face);
    fprintf(reaout, "u = 1.\n");
    fprintf(reaout, "v = 0.\n");
    fprintf(reaout, "w = 0.\n");
  break;
  case 10:
    fprintf(reaout, "O %d %d 0. 0. 0.\n", i+1, face);
    break;
  default:
    fprintf(reaout, "W %d %d 0. 0. 0.\n", i+1, face);
  break;
  }
}


#define error_msg(a) {fprintf(stderr,#a"\n"); exit(-1);}

#define MAX_SURFACE_VERT 100000
main (int argc, char *argv[])
{
  int     Npt, Ntri, Ntet;
  double  *x, *y, *z;
  int     i,j,k,l,m,n;
  int     **face_ids,*face_flags,**tet_ids,*nelmts_vertisin, **elmts_vertisin;
  int     icnt, *cnt, **conn, *flag, iicnt;
  int     va, vb, vc, vd, fl, eid;
  int     **emap;
  char    buf[BUFSIZ];
  char    bufa[BUFSIZ];
  char    title[BUFSIZ];
  FILE    *griin, *reaout, *cfgin, *froin, *frontfp;
  double  ox,oy,oz, cx, cy,cz;
  int    *curvida, *curvidb, *curvidc, *curvtype, *facemask;
  int tetvnum[][3] = {{0,1,2}, {0,1,3}, {1,2,3},{0,2,3}};
  int prismvnum[][4] = {{0,1,2,3}, {0,1,4,-1}, {1,2,5,4},{3,2,5,-1},{0,3,4,5}};
  int   nfaces, npts, npanels, surf_id,**surfids;
  int *iscurved;
  FILE *tecfile;
  int  **pvids;
  int Npris=0, nel;
  int acnt = 0, bcnt = 0, ccnt = 0;

  if(argc != 2){
    fprintf(stderr,"Usage: blt2nek file");
    exit(-1);
  }

  sprintf(title,"%s",argv[argc-1]);

  /* essential files */
  sprintf(bufa, "%s.gri", strtok(title, "\n"));
  if(!(griin = fopen(bufa, "r")))
    error_msg(Can not open .gri file);

  sprintf(bufa, "%s.fro", strtok(title, "\n"));
  if(!(froin = fopen(bufa, "r")))
    error_msg(Can not open .fro file);

  /* optional/configuration  files */
  sprintf(bufa, "%s.cfg", strtok(title, "\n"));
  cfgin = fopen(bufa, "r");

  sprintf(bufa, "%s.fnt", strtok(title, "\n"));
  if(!(frontfp = fopen(bufa, "r"))){
    fprintf(stderr,"No .fnt file generating all tet's\n");
    npanels = 0;
  }
  else{
    fgets(buf,  BUFSIZ, frontfp);
    sscanf(buf, "%d ", &npanels);
    fprintf(stderr,"Generating Prism-Tet mesh on %d faces\n",npanels);
  }

  /* output file */
  sprintf(bufa, "%s.rea", strtok(title, "\n"));
  reaout = fopen(bufa, "w");

  fgets(buf,  BUFSIZ, griin);
  fprintf(stderr, "Reading Mesh: \n");
  fscanf(griin, "%d%d%*d", &Ntet, &Npt);
  fgets(buf,  BUFSIZ, griin);
  fgets(buf,  BUFSIZ, griin);
  fprintf(stderr, "Npts: %d \n", Npt);
  fprintf(stderr, "Ntet: %d \n", Ntet);

  pvids = imatrix(0, MAX_SURFACE_VERT-1, 0, 6);

  /* build prism mesh from information in front.dump */
  icnt  = 0;
  for(i=0; i < npanels; ++i){
    fgets(buf, BUFSIZ, frontfp);
    sscanf(buf, "%*12c%d%*13c%d", &surf_id, &Ntri);
#ifdef PRINT
    fprintf(stdout, "Surface i: %d Ntri: %d\n", surf_id,Ntri);
#endif
    for(j = 0; j < Ntri; ++j){
      fgets(buf, BUFSIZ, frontfp);
      sscanf(buf, "%d%d%d %d%d%d", pvids[icnt], pvids[icnt]+1,
       pvids[icnt]+2, pvids[icnt]+3, pvids[icnt]+4, pvids[icnt]+5);
      pvids[icnt][6] = surf_id;

      if(!pvids[icnt][3]&&!pvids[icnt][4]&&!pvids[icnt][5]){
  fprintf(stderr, "WARNING MISSING FRONT PIECE\n");
      }

      if(pvids[icnt][0] && pvids[icnt][1] && pvids[icnt][2] &&
   pvids[icnt][3] && pvids[icnt][4] && pvids[icnt][5]){
#ifdef PRINT
  fprintf(stdout, "%d %d %d %d %d %d\n",
    pvids[icnt][0], pvids[icnt][1], pvids[icnt][2],
    pvids[icnt][3], pvids[icnt][4], pvids[icnt][5]);
#endif
      ++icnt;
      }
    }
  }
  Npris = icnt;
  Ntri = Npris;

  nel = Ntet - 2*Npris;

  /* use max vertex id of second tri. face to orientate elements */
  emap = imatrix(0, nel, 0, 6);

  izero(nel*6, emap[0], 1);

  for(i=0; i < Npris; ++i){

    if(pvids[i][3] > pvids[i][4] && pvids[i][3] > pvids[i][5]){
      emap[i][0] = pvids[i][2];
      emap[i][1] = pvids[i][1];
      emap[i][2] = pvids[i][4];
      emap[i][3] = pvids[i][5];
      emap[i][4] = pvids[i][0];
      emap[i][5] = pvids[i][3];
      emap[i][6] = pvids[i][6]; /* surface id */
      ++acnt;
    }
    else if(pvids[i][4] > pvids[i][3] && pvids[i][4] > pvids[i][5]){
      emap[i][0] = pvids[i][0];
      emap[i][1] = pvids[i][2];
      emap[i][2] = pvids[i][5];
      emap[i][3] = pvids[i][3];
      emap[i][4] = pvids[i][1];
      emap[i][5] = pvids[i][4];
      emap[i][6] = pvids[i][6]; /* surface id */
      ++bcnt;
    }
    else if(pvids[i][5] > pvids[i][3] && pvids[i][5] > pvids[i][4]){
      emap[i][0] = pvids[i][1];
      emap[i][1] = pvids[i][0];
      emap[i][2] = pvids[i][3];
      emap[i][3] = pvids[i][4];
      emap[i][4] = pvids[i][2];
      emap[i][5] = pvids[i][5];
      emap[i][6] = pvids[i][6]; /* surface id */
      ++ccnt;
    }
  }

#ifdef PRINT
  fprintf(stderr, "Acnt: %d, Bcnt:  %d, Ccnt: %d\n", acnt, bcnt, ccnt);
#endif

  /* read  face vertices  leaving out first 3*Ntri */

  for(i=0; i < 3*Ntri; ++i)
    fscanf(griin, "%*d%*d%*d%*d");

  Ntet   -= 3*Ntri;
  tet_ids = imatrix(0, Ntet, 0, 4);

  for(i=0;i<Ntet;++i){
    fscanf(griin, "%d%d%d%d",
     tet_ids[i],
     tet_ids[i]+1,
     tet_ids[i]+2,
     tet_ids[i]+3);
    tet_ids[i][4] = -MAXSURF; /* default value for interior face */
  }

  fgets(buf,  BUFSIZ, griin);
  fgets(buf,  BUFSIZ, griin);

  /* read coordinates */
  x = dvector(0, Npt-1);
  y = dvector(0, Npt-1);
  z = dvector(0, Npt-1);

  fprintf(stderr, "Reading: coords\n");
  for(i=0;i<Npt;++i)
    fscanf(griin, "%lf%lf%lf", x+i,y+i,z+i);


#ifdef TEC
  fprintf(stderr,"Dumping prism .tec file\n");
  tecfile  = fopen("prisms.tec", "w");
  fprintf(tecfile, "VARIABLES = x, y, z\n");
  for(i=0;i<Npris;++i){
    fprintf(tecfile, "ZONE T=\"zone %d\" F=POINT, I=2, J=2, K=2\n",i+1);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][0]-1],  y[emap[i][0]-1], z[emap[i][0]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][1]-1],  y[emap[i][1]-1], z[emap[i][1]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][3]-1],  y[emap[i][3]-1], z[emap[i][3]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][2]-1],  y[emap[i][2]-1], z[emap[i][2]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][4]-1],  y[emap[i][4]-1], z[emap[i][4]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][4]-1],  y[emap[i][4]-1], z[emap[i][4]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][5]-1],  y[emap[i][5]-1], z[emap[i][5]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][5]-1],  y[emap[i][5]-1], z[emap[i][5]-1]);
  }
  fclose(tecfile);
#endif

  /* sort out connectivity */
  for(i=0;i<Ntet;++i){  /* find max vert id */
    vb = max(tet_ids[i][0], tet_ids[i][1]);
    vc = max(tet_ids[i][2], tet_ids[i][3]);
    va = max(vb,vc);
    vb = -1000;

    /* find next max vert id */
    for(j=0;j<4;++j)
      if(tet_ids[i][j] < va && tet_ids[i][j] > vb) vb = tet_ids[i][j];

    vc = -1000;
    for(j=0;j<4;++j)
      if(tet_ids[i][j] < vb && tet_ids[i][j] > vc) vc = tet_ids[i][j];

    vd = -1000;
    for(j=0;j<4;++j)
      if(tet_ids[i][j] < vc && tet_ids[i][j] > vd) vd = tet_ids[i][j];

    if(va == -1000)
      fprintf(stderr, " va f.d\n");
    if(vb == -1000)
      fprintf(stderr, " vb f.d\n");
    if(vc == -1000)
      fprintf(stderr, " vc f.d\n");
    if(vd == -1000){
      fprintf(stderr, " vd f.d: %d %d %d %d \n",va,vb,vc,vd);
      fprintf(stderr, " tet_ids: %d %d %d %d \n",
        tet_ids[i][0], tet_ids[i][1],tet_ids[i][2], tet_ids[i][3]);
    }

    /* now orientate element correctly */
    if(determ(x,y,z, vd-1, vc-1, vb-1, va-1) < 0){
      emap[i+Npris][0] = vc;
      emap[i+Npris][1] = vd;
      emap[i+Npris][2] = vb;
      emap[i+Npris][3] = va;
    }
    else{
      emap[i+Npris][0] = vd;
      emap[i+Npris][1] = vc;
      emap[i+Npris][2] = vb;
      emap[i+Npris][3] = va;
    }
  }


#ifdef TEC
  fprintf(stderr,"Dumping tet .tec file\n");
  tecfile = fopen("tets.tec","w");
  fprintf(tecfile, "VARIABLES = x, y, z\n");
  for(i=Npris;i<nel;++i){
    fprintf(tecfile, "ZONE T=\"zone %d\" F=POINT, I=2, J=2, K=2\n",i+1);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][0]-1], y[emap[i][0]-1],  z[emap[i][0]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][1]-1], y[emap[i][1]-1],  z[emap[i][1]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][2]-1], y[emap[i][2]-1],  z[emap[i][2]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][2]-1], y[emap[i][2]-1],  z[emap[i][2]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][3]-1], y[emap[i][3]-1],  z[emap[i][3]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][3]-1], y[emap[i][3]-1],  z[emap[i][3]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][3]-1], y[emap[i][3]-1],  z[emap[i][3]-1]);
    fprintf(tecfile, "%lf %lf %lf\n",
      x[emap[i][3]-1], y[emap[i][3]-1],  z[emap[i][3]-1]);
  }
  fclose(tecfile);
#endif

  fprintf(stderr, "Checking conn\n");
  fprintf(stderr, "Finding how many tet.s share each vertex\n");


  icnt = 0;
  for(i=0;i<Npris;++i)
    for(j=0;j<6;++j)
      icnt = max(icnt, emap[i][j]);
  for(i=0;i<Ntet;++i)
    for(j=0;j<4;++j)
      icnt = max(icnt, emap[i+Npris][j]);
  fprintf(stderr, "Npt: %d Max id: %d\n", Npt, icnt);

  /* Now find the connectivity */
  nelmts_vertisin  = ivector(0, Npt-1);
  izero( Npt, nelmts_vertisin, 1);
  for(i=0;i<Npris;++i)
    for(j=0;j<6;++j)
      ++nelmts_vertisin[emap[i][j]-1];  /* take 1 off vertex id */

  for(i=0;i<Ntet;++i)
    for(j=0;j<4;++j)
      ++nelmts_vertisin[emap[i+Npris][j]-1];  /* take 1 off vertex id */


  elmts_vertisin = (int**) calloc(Npt, sizeof(int*));
  for(i=0;i<Npt;++i)
    elmts_vertisin[i] = ivector(0, nelmts_vertisin[i]-1);

  cnt = ivector(0, Npt-1);
  izero( Npt, cnt, 1);

  for(i=0;i<Npris;++i)
    for(j=0;j<6;++j){
      elmts_vertisin[emap[i][j]-1][cnt[emap[i][j]-1]] = i;
      ++cnt[emap[i][j]-1];
    }

  for(i=0;i<Ntet;++i)
    for(j=0;j<4;++j){
      elmts_vertisin[emap[i+Npris][j]-1][cnt[emap[i+Npris][j]-1]] = i+Npris;
      ++cnt[emap[i+Npris][j]-1];
    }

  /* sort out connectivity */
  conn = imatrix(0, nel-1, 0, 4);
  ifill(nel*5, -MAXSURF, conn[0], 1);

  flag = ivector(0, nel-1);
  izero( nel, flag, 1);
  fprintf(stderr, "Doing the conn. job\n");

  /* finding connections */
  for(i=0;i<nel;++i){

    if(!(i%100))
      fprintf(stderr, ".");

    /* test face k */
    if(i < Npris){

      /* face 0 */
      va = emap[i][prismvnum[0][0]]-1;
      vb = emap[i][prismvnum[0][1]]-1;
      vc = emap[i][prismvnum[0][2]]-1;
      vd = emap[i][prismvnum[0][3]]-1;

      conn[i][0] =
  find_match_square(va, vb, vc, vd, flag, elmts_vertisin, cnt, i);

      /* face 1 */
      va = emap[i][prismvnum[1][0]]-1;
      vb = emap[i][prismvnum[1][1]]-1;
      vc = emap[i][prismvnum[1][2]]-1;

      conn[i][1] =
  find_match_tri(va, vb, vc, flag, elmts_vertisin, cnt, i);

      /* face 2 */
      va = emap[i][prismvnum[2][0]]-1;
      vb = emap[i][prismvnum[2][1]]-1;
      vc = emap[i][prismvnum[2][2]]-1;
      vd = emap[i][prismvnum[2][3]]-1;

      conn[i][2] =
  find_match_square(va, vb, vc, vd, flag, elmts_vertisin, cnt, i);

      /* face 3 */
      va = emap[i][prismvnum[3][0]]-1;
      vb = emap[i][prismvnum[3][1]]-1;
      vc = emap[i][prismvnum[3][2]]-1;

      conn[i][3] =
  find_match_tri(va, vb, vc, flag, elmts_vertisin, cnt, i);

      /* face 4 */
      va = emap[i][prismvnum[4][0]]-1;
      vb = emap[i][prismvnum[4][1]]-1;
      vc = emap[i][prismvnum[4][2]]-1;
      vd = emap[i][prismvnum[4][3]]-1;

      conn[i][4] =
  find_match_square(va, vb, vc, vd, flag, elmts_vertisin, cnt, i);

    }
    else{

      for(j=0;j<4;++j){
  /* face j */
  va = emap[i][tetvnum[j][0]]-1;
  vb = emap[i][tetvnum[j][1]]-1;
  vc = emap[i][tetvnum[j][2]]-1;

  conn[i][j] =
    find_match_tri(va, vb, vc, flag, elmts_vertisin, cnt, i);
      }
    }
  }

  /* read the .fro file for all surface id's */
  fgets(buf,BUFSIZ,froin);
  sscanf(buf,"%d%d",&nfaces,&npts);
  surfids = imatrix(0,nfaces-1,0,3);
  for(i = 0; i < npts; ++i) /* read past coords */
    fgets(buf,BUFSIZ,froin);

  for(i = 0; i < nfaces; ++i){ /* read in surface and global id's */
    fgets(buf,BUFSIZ,froin);
    sscanf(buf,"%*d%d%d%d%d",surfids[i],surfids[i]+1,
     surfids[i]+2,surfids[i]+3);
  }

  fprintf(stderr,"\n Matching faces to Felisa surface id's \n");
  /* find surfaces matching unconnected faces */
  for(i = 0; i < Npris; ++i){
    if(conn[i][0] == -MAXSURF)
      if((conn[i][0] = -match_tri(emap[i][0],emap[i][1],emap[i][2],
         nfaces,surfids)) == -MAXSURF)
  conn[i][0] = -match_tri(emap[i][0],emap[i][1],emap[i][3],
        nfaces,surfids);
    if(conn[i][1] == -MAXSURF)
      conn[i][1] = -pvids[i][6];

    if(conn[i][2] == -MAXSURF)
      if((conn[i][2] = -match_tri(emap[i][1],emap[i][2],emap[i][4],
            nfaces,surfids) == -MAXSURF))
  conn[i][2] = -match_tri(emap[i][1],emap[i][2],emap[i][5],
         nfaces,surfids);
    if(conn[i][3] == -MAXSURF)
      conn[i][3] = -pvids[i][6];

    if(conn[i][4] == -MAXSURF)
      if((conn[i][4] = -match_tri(emap[i][0],emap[i][3],emap[i][4],
            nfaces,surfids) == -MAXSURF))
  conn[i][4] = -match_tri(emap[i][0],emap[i][3],emap[i][5],
        nfaces,surfids);
  }

  for(i = 0; i < Ntet; ++i){
    if(conn[i+Npris][0] == -MAXSURF)
      conn[i+Npris][0] = -match_tri(emap[i+Npris][0],emap[i+Npris][1],
            emap[i+Npris][2],nfaces,surfids);
    if(conn[i+Npris][1] == -MAXSURF)
      conn[i+Npris][1] = -match_tri(emap[i+Npris][0],emap[i+Npris][1],
            emap[i+Npris][3],nfaces,surfids);
    if(conn[i+Npris][2] == -MAXSURF)
      conn[i+Npris][2] = -match_tri(emap[i+Npris][1],emap[i+Npris][2],
            emap[i+Npris][3],nfaces,surfids);
    if(conn[i+Npris][3] == -MAXSURF)
      conn[i+Npris][3] = -match_tri(emap[i+Npris][0],emap[i+Npris][2],
            emap[i+Npris][3],nfaces,surfids);
  }

  /* assume face 2 of every prism is curved */

  /* Write out mesh */

  fprintf(reaout, "****** PARAMETERS *****\n");
  fprintf(reaout, " SolidMes \n");
  fprintf(reaout, " 3 DIMENSIONAL RUN\n");
  fprintf(reaout, " 0 PARAMETERS FOLLOW\n");
  fprintf(reaout, "0  Lines of passive scalar data follows2 CONDUCT; 2RHOCP\n");
  fprintf(reaout, " 0  LOGICAL SWITCHES FOLLOW\n");
  fprintf(reaout, "Dummy line from old nekton file\n");
  fprintf(reaout, "**MESH DATA** x,y,z, values of vertices 1,2,3,4.\n");

  fprintf(reaout, "%d 	3	1   NEL NDIM NLEVEL\n", nel);

  for(i=0;i<Npris;++i){
    fprintf(reaout, "Element %d Prism\n", i+1);
    fprintf(reaout, "%le %le %le %le %le %le\n",
      x[emap[i][0]-1], x[emap[i][1]-1],
      x[emap[i][2]-1], x[emap[i][3]-1],
      x[emap[i][4]-1], x[emap[i][5]-1]);

    fprintf(reaout, "%le %le %le %le %le %le\n",
      y[emap[i][0]-1], y[emap[i][1]-1],
      y[emap[i][2]-1], y[emap[i][3]-1],
      y[emap[i][4]-1], y[emap[i][5]-1]);

    fprintf(reaout, "%le %le %le %le %le %le\n",
      z[emap[i][0]-1], z[emap[i][1]-1],
      z[emap[i][2]-1], z[emap[i][3]-1],
      z[emap[i][4]-1], z[emap[i][5]-1]);
  }

  for(i=Npris;i<nel;++i){
    fprintf(reaout, "Element %d Tet\n", i+1);
    fprintf(reaout, "%le %le %le %le\n",
      x[emap[i][0]-1], x[emap[i][1]-1],
      x[emap[i][2]-1], x[emap[i][3]-1]);

    fprintf(reaout, "%le %le %le %le\n",
      y[emap[i][0]-1], y[emap[i][1]-1],
      y[emap[i][2]-1], y[emap[i][3]-1]);

    fprintf(reaout, "%le %le %le %le\n",
      z[emap[i][0]-1], z[emap[i][1]-1],
      z[emap[i][2]-1], z[emap[i][3]-1]);
  }


  fprintf(reaout, "***** CURVED SIDE DATA ***** \n");
  fprintf(reaout, "1 Number of curve types\n");
  fprintf(reaout, "File\n");
  fprintf(reaout, " %s.fro a\n",title);

  fprintf(reaout, "%d Curved sides follow\n", Npris);

  for(i=0;i<Npris;++i){
    fprintf(reaout, "%d %d a %d %d %d\n", 2, i+1,
      emap[i][prismvnum[1][0]],
      emap[i][prismvnum[1][1]],
      emap[i][prismvnum[1][2]]);
  }


  fprintf(reaout, "***** BOUNDARY CONDITIONS ***** \n");
  fprintf(reaout, "***** FLUID BOUNDARY CONDITIONS ***** \n");

  for(i=0;i<Npris;++i){

    if(conn[i][0] > -1){
      eid = conn[i][0];
      if(conn[eid][0] == i)
  fprintf(reaout, "E %d %d %d %d\n", i+1, 1, eid+1, 1);
      else if(conn[eid][2] == i)
  fprintf(reaout, "E %d %d %d %d\n", i+1, 1, eid+1, 3);
      else if(conn[eid][4] == i)
  fprintf(reaout, "E %d %d %d %d\n", i+1, 1, eid+1, 5);
      else{
  fprintf(stderr, "WARNING UNMATCHED PRISM side 1\n");
  fprintf(reaout, "W %d %d 0. 0. 0.\n", i+1, 1);
      }
    }
    else
      writebc(reaout,-conn[i][0],i,1);


    if(conn[i][1] > -1){
      eid = conn[i][1];

      if(eid < Npris){
  if(conn[eid][1] == i)
    fprintf(reaout, "E %d %d %d %d\n", i+1, 2, eid+1, 2);
  else if(conn[eid][3] == i)
    fprintf(reaout, "E %d %d %d %d\n", i+1, 2, eid+1, 4);
  else{
    fprintf(stderr, "WARNING UNMATCHED PRISM side 2\n");
    fprintf(reaout, "W %d %d 0. 0. 0.\n", i+1, 2);
  }
      }
      else{
  if(conn[eid][0] == i)
    fprintf(reaout, "E %d %d %d %d\n", i+1, 2, eid+1, 1);
  else if(conn[eid][1] == i)
    fprintf(reaout, "E %d %d %d %d\n", i+1, 2, eid+1, 2);
  else if(conn[eid][2] == i)
    fprintf(reaout, "E %d %d %d %d\n", i+1, 2, eid+1, 3);
  else if(conn[eid][3] == i)
    fprintf(reaout, "E %d %d %d %d\n", i+1, 2, eid+1, 4);
  else{
    fprintf(stderr, "WARNING UNMATCHED PRISM to Tet side 2\n");
    fprintf(reaout, "W %d %d 0. 0. 0.\n", i+1, 2);
  }
      }
    }
    else
      writebc(reaout,-conn[i][1],i,2);

    if(conn[i][2] > -1){
      eid = conn[i][2];
      if(conn[eid][0] == i)
  fprintf(reaout, "E %d %d %d %d\n", i+1, 3, eid+1, 1);
      else if(conn[eid][2] == i)
  fprintf(reaout, "E %d %d %d %d\n", i+1, 3, eid+1, 3);
      else if(conn[eid][4] == i)
  fprintf(reaout, "E %d %d %d %d\n", i+1, 3, eid+1, 5);
      else{
  fprintf(stderr, "WARNING UNMATCHED PRISM to Tet side 3\n");
  fprintf(reaout, "W %d %d 0. 0. 0.\n", i+1, 3);
      }
    }
    else
      writebc(reaout,-conn[i][2],i,3);

    if(conn[i][3] > -1){
      eid = conn[i][3];

      if(eid< Npris){
  if(conn[eid][1] == i)
    fprintf(reaout, "E %d %d %d %d\n", i+1, 4, eid+1, 2);
  else if(conn[eid][3] == i)
    fprintf(reaout, "E %d %d %d %d\n", i+1, 4, eid+1, 4);
  else{
    fprintf(stderr, "WARNING UNMATCHED PRISM  side 4\n");
    fprintf(reaout, "W %d %d 0. 0. 0.\n", i+1, 4);
  }
      }
      else{
  if(conn[eid][0] == i)
    fprintf(reaout, "E %d %d %d %d\n", i+1, 4, eid+1, 1);
  else if(conn[eid][1] == i)
    fprintf(reaout, "E %d %d %d %d\n", i+1, 4, eid+1, 2);
  else if(conn[eid][2] == i)
    fprintf(reaout, "E %d %d %d %d\n", i+1, 4, eid+1, 3);
  else if(conn[eid][3] == i)
    fprintf(reaout, "E %d %d %d %d\n", i+1, 4, eid+1, 4);
  else{
    fprintf(stderr, "WARNING UNMATCHED PRISM to Tet side 4\n");
    fprintf(reaout, "W %d %d 0. 0. 0.\n", i+1, 4);
  }
      }
    }
    else
      writebc(reaout,-conn[i][3],i,4);

    if(conn[i][4] > -1){
      eid = conn[i][4];
      if(conn[eid][0] == i)
  fprintf(reaout, "E %d %d %d %d\n", i+1, 5, eid+1, 1);
      else if(conn[eid][2] == i)
  fprintf(reaout, "E %d %d %d %d\n", i+1, 5, eid+1, 3);
      else if(conn[eid][4] == i)
  fprintf(reaout, "E %d %d %d %d\n", i+1, 5, eid+1, 5);
      else{
  fprintf(stderr, "WARNING UNMATCHED PRISM  side 5\n");
  fprintf(reaout, "W %d %d 0. 0. 0.\n", i+1, 5);
      }
    }
    else
      writebc(reaout,-conn[i][4],i,5);
  }

  for(i=0;i<Ntet;++i){
#if 0
    fprintf(stdout, "id: %d  (%d,%d,%d,%d)\n",
      i,conn[i+Npris][0],
      conn[i+Npris][1],conn[i+Npris][2],conn[i+Npris][3]);
#endif
    if(conn[i+Npris][0] > -1){
      eid = conn[i+Npris][0];

      if(eid< Npris){
  if(conn[eid][1] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 1, eid+1, 2);
  else if(conn[eid][3] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 1, eid+1, 4);
  else{
    fprintf(stderr, "WARNING UNMATCHED Tet to PRISM side 1\n");
    fprintf(reaout, "O %d %d 0. 0. 0.\n", i+Npris+1, 1);
  }
      }
      else{
  if(conn[eid][0] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 1, eid+1, 1);
  else if(conn[eid][1] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 1, eid+1, 2);
  else if(conn[eid][2] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 1, eid+1, 3);
  else if(conn[eid][3] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 1, eid+1, 4);
  else{
    fprintf(stderr, "WARNING UNMATCHED Tet to Tet side 2\n");
    fprintf(reaout, "O %d %d 0. 0. 0.\n", i+Npris+1, 1);
  }
      }
    }
    else
      writebc(reaout,-conn[i+Npris][0],i+Npris,1);

    if(conn[i+Npris][1] > -1){
      eid = conn[i+Npris][1];

      if(eid< Npris){
  if(conn[eid][1] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 2, eid+1, 2);
  else if(conn[eid][3] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 2, eid+1, 4);
  else{
    fprintf(stderr, "WARNING UNMATCHED Tet to PRISM side 2\n");
    fprintf(stderr, "WARNING UNMATCHED elmt: %d face : %d  should match to: %d\n", i+Npris+1, 2, eid+1);
    fprintf(stderr, "WARNING UNMATCHED elmt: %d actually matches to : %d,%d\n", eid+1, conn[eid][1], conn[eid][3]);

    fprintf(reaout, "O %d %d 0. 0. 0.\n", i+Npris+1, 2);
  }
      }
      else{
  if(conn[eid][0] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 2, eid+1, 1);
  else if(conn[eid][1] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 2, eid+1, 2);
  else if(conn[eid][2] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 2, eid+1, 3);
  else if(conn[eid][3] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 2, eid+1, 4);
  else{
    fprintf(stderr, "WARNING UNMATCHED Tet to Tet side 2\n");
    fprintf(reaout, "O %d %d 0. 0. 0.\n", i+Npris+1, 2);
  }
      }
    }
    else
      writebc(reaout,-conn[i+Npris][1],i+Npris,2);

    if(conn[i+Npris][2] > -1){
      eid = conn[i+Npris][2];

      if(eid< Npris){
  if(conn[eid][1] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 3, eid+1, 2);
  else if(conn[eid][3] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 3, eid+1, 4);
  else{
    fprintf(reaout, "O %d %d 0. 0. 0.\n", i+Npris+1, 3);
    fprintf(stderr, "WARNING UNMATCHED Tet to Tet side 3\n");
  }
      }
      else{
  if(conn[eid][0] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 3, eid+1, 1);
  else if(conn[eid][1] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 3, eid+1, 2);
  else if(conn[eid][2] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 3, eid+1, 3);
  else if(conn[eid][3] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 3, eid+1, 4);
  else{
    fprintf(stderr, "WARNING UNMATCHED Tet to Tet side 3\n");
    fprintf(reaout, "O %d %d 0. 0. 0.\n", i+Npris+1, 3);
  }
      }
    }
    else
      writebc(reaout,-conn[i+Npris][2],i+Npris,3);


    if(conn[i+Npris][3] > -1){
      eid = conn[i+Npris][3];

      if(eid< Npris){
  if(conn[eid][1] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 4, eid+1, 2);
  else if(conn[eid][3] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 4, eid+1, 4);
  else{
    fprintf(stderr, "WARNING UNMATCHED PRISM to Tet side 4\n");
    fprintf(reaout, "O %d %d 0. 0. 0.\n", i+Npris+1, 4);
  }
      }
      else{
  if(conn[eid][0] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 4, eid+1, 1);
  else if(conn[eid][1] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 4, eid+1, 2);
  else if(conn[eid][2] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 4, eid+1, 3);
  else if(conn[eid][3] == i+Npris)
    fprintf(reaout, "E %d %d %d %d\n", i+Npris+1, 4, eid+1, 4);
  else{
    fprintf(stderr, "WARNING UNMATCHED Tet to Tet side 4\n");
    fprintf(reaout, "O %d %d 0. 0. 0.\n", i+Npris+1, 4);
  }
      }
    }
    else
      writebc(reaout,-conn[i+Npris][3],i+Npris,4);
  }

  fprintf(reaout, "***** NO THERMAL BOUNDARY CONDITIONS *****\n");
  fprintf(reaout, "1         INITIAL CONDITIONS *****\n");
  fprintf(reaout, "Default\n");
  fprintf(reaout, "***** DRIVE FORCE DATA ***** PRESSURE GRAD, FLOW, Q\n");
  fprintf(reaout, "0                 Lines of Drive force data follow\n");
  fprintf(reaout, "***** Variable Property Data ***** Overrrides Parameter data.\n");
  fprintf(reaout, " 1 Lines follow.\n");
  fprintf(reaout, " 0 PACKETS OF DATA FOLLOW\n");
  fprintf(reaout, "***** HISTORY AND INTEGRAL DATA *****\n");
  fprintf(reaout, " 0   POINTS.  Hcode, I,J,H,IEL\n");
  fprintf(reaout, " ***** OUTPUT FIELD SPECIFICATION *****\n");
  fprintf(reaout, "  0 SPECIFICATIONS FOLLOW\n");

}
