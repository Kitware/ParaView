/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/fel2nek_1.C,v $
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

char *prog   = "fel2nek";
char *usage  = "fel2nek:  [options]  input[.gri]\n";
char *author = "";
char *rcsid  = "";
char *help   =
  "If a meshfile is specified then it is used to determine the value \n"
"of the boundary conditions and curved sides \n";

/* ---------------------------------------------------------------------- */
#define  MAXTYPES 50

typedef struct bndinfo {
  int  region;
  char type;
  int  nbcs;
  union binf  {
    char   *str[3];
    double  val[3];
  } data;
  struct bndinfo *next;
} Bndinfo;

typedef struct curinfo {
  int  region;
  int  eid;
  int  iside;
  char type;
  char tag;
  CurveInfo info;
  struct curinfo *next;
} Curinfo;

typedef struct cinfo {
  char  type;
  int   elmtid;
  int   vsid;
  char  *str[3];
  double f[3];
  struct cinfo *next;
} Cinfo;

typedef struct curlist{
  int eid;
  int iside;
  char type;
  struct curlist *next;
}Curlist;

typedef struct gdinfo {
  int nel;
  int npt;
  int nbnd;
  double *x,*y;
  int   **elmtpts;
  Cinfo **elmtcon;
  Bndinfo *bnd;
  Curinfo *curve;
  Curlist *clist;
} Gdinfo;


static Gdinfo *ReadGridFile(FileList *f);
static void Write(FILE *fp, Gdinfo *ginfo);
static void addcurve(Gdinfo *ginfo, int eid, int iside, char type);
static void parse_util_args (int argc, char *argv[], FileList *f);


void main (int argc, char *argv[])
{
  FileList  f;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);

  Write(f.out.fp,ReadGridFile(&f));

}

static void addcon(Cinfo *con, int elmt, int vert);
static void ReadBcond(FILE *fp, Gdinfo *ginfo);

static Gdinfo *ReadGridFile(FileList *f){
  register int  i,j,k;
  char     buf[BUFSIZ];
  int      nel,npt,nbnd,vid,vid1,eid,eid1,**bndpts,trip;
  Gdinfo   *ginfo = (Gdinfo *)malloc(sizeof(Gdinfo));
  Cinfo    *con,*c,*c1;
  FILE     *fp = f->in.fp;

  /* read boundary condition file */
  if(f->mesh.name) ReadBcond(f->mesh.fp,ginfo);

  fgets(buf,BUFSIZ,fp);
  fgets(buf,BUFSIZ,fp);

  sscanf(buf,"%d%d%d%*d",&nel,&npt,&nbnd);
  ginfo->nel     = nel;
  ginfo->npt     = npt;
  ginfo->nbnd    = nbnd;
  ginfo->elmtpts = imatrix(0,nel-1,0,2);
  ginfo->x       = dvector(0,npt-1);
  ginfo->y       = dvector(0,npt-1);
  bndpts         = imatrix(0,nel-1,0,2);

  ginfo->elmtcon    = (Cinfo**)malloc(nel*sizeof(Cinfo *));
  ginfo->elmtcon[0] = (Cinfo *)calloc(3*nel,sizeof(Cinfo) );

  for(k = 1; k < nel; ++k) ginfo->elmtcon[k] = ginfo->elmtcon[k-1]+3;

  con = (Cinfo *)calloc(npt,sizeof(Cinfo));

  /* read element vertex co-ordinate indices */
  fgets(buf,BUFSIZ,fp);
  for(k = 0; k < nel; ++k){
    fgets (buf,BUFSIZ,fp);
    sscanf(buf,"%*d%d%d%d",ginfo->elmtpts[k],
     ginfo->elmtpts[k]+1,ginfo->elmtpts[k]+2);
  }

  /* read co-ordinates */
  fgets(buf,BUFSIZ,fp);
  for(k = 0; k < npt; ++k){
    fgets (buf,BUFSIZ,fp);
    sscanf(buf,"%*d%lf%lf",ginfo->x+k,ginfo->y+k);
  }

  /* read boundary info */
  fgets(buf,BUFSIZ,fp);
  for(k = 0; k < nbnd; ++k){
    fgets (buf,BUFSIZ,fp);
    sscanf(buf,"%d%d%*d%d",bndpts[k],bndpts[k]+1,bndpts[k]+2);
  }

  /* sort out connectivity */
  /* get list of all elements at a vertex */
  for(k = 0; k < nel; ++k)
    for(i = 0; i < 3; ++i)
      addcon(con+ginfo->elmtpts[k][i]-1,k+1,i);

  /* search through element list and fill out element connectivity */
  for(i = 0; i < npt; ++i){
    for(c = con[i].next; c; c = c->next){
      vid = (c->vsid+1)%3;
      eid =  c->elmtid;
      for(c1 = con[i].next; c1; c1 = c1->next){
  vid1 = (c1->vsid+2)%3;
  eid1 =  c1->elmtid;
  if(ginfo->elmtpts[eid-1][vid] == ginfo->elmtpts[eid1-1][vid1]){
    ginfo->elmtcon[eid -1][c->vsid].type   = 'E';
    ginfo->elmtcon[eid -1][c->vsid].elmtid = eid1;
    ginfo->elmtcon[eid -1][c->vsid].vsid   = vid1;
    ginfo->elmtcon[eid1-1][vid1   ].type   = 'E';
    ginfo->elmtcon[eid1-1][vid1   ].elmtid = eid;
    ginfo->elmtcon[eid1-1][vid1   ].vsid   = c->vsid;
  }
      }
    }
  }

  /* check through list and match any missing element with boundary values */
  for(k = 0; k < nel; ++k)
    for(i = 0; i < 3; ++i)
      if(!ginfo->elmtcon[k][i].elmtid){
  vid  = ginfo->elmtpts[k][i];
  vid1 = ginfo->elmtpts[k][(i+1)%3];
  /* check to see that this side is a boundary */
  for(j = 0,trip=1; j < nbnd; ++j)
    if(vid == bndpts[j][0] && vid1 == bndpts[j][1]){
      /* set boundary condition from given values if present */
      if(f->mesh.name){
        register int  i1;
        Bndinfo *b;
        Curinfo *c;

        for(b=ginfo->bnd; b; b = b->next)
    if(b->region == bndpts[j][2]){
      ginfo->elmtcon[k][i].type = b->type;
      switch(b->type){
      case 'W': case 'O': case 'S': case 'B': case 'M': case 'I':
      case 'Z':
        break;
      case 'V': case 'F':
        dcopy(b->nbcs,b->data.val,1,ginfo->elmtcon[k][i].f,1);
        break;
      case 'v': case 'f': case 'm':
        for(i1=0; i1 < b->nbcs; ++i1)
          ginfo->elmtcon[k][i].str[i1] = b->data.str[i1];
        break;
      }
    }

        for(c=ginfo->curve; c; c = c->next){
    if(c->region == bndpts[j][2]){
      /* reset co-ordinates */
      switch(c->type){
      case 'C': /* arc */
        {
          double theta;

          theta = atan2(ginfo->y[vid-1]-c->info.arc.yc,
            ginfo->x[vid-1]-c->info.arc.xc);

          ginfo->x[vid-1] = fabs(c->info.arc.radius)*cos(theta)
      + c->info.arc.xc;
          ginfo->y[vid-1] = fabs(c->info.arc.radius)*sin(theta)
      + c->info.arc.yc;

          theta = atan2(ginfo->y[vid1-1]-c->info.arc.yc,
            ginfo->x[vid1-1]-c->info.arc.xc);

          ginfo->x[vid1-1] = fabs(c->info.arc.radius)*cos(theta)
      + c->info.arc.xc;
          ginfo->y[vid1-1] = fabs(c->info.arc.radius)*sin(theta)
      + c->info.arc.yc;
          break;
        }
      }

      /* add curved information about element to ginfo */
      addcurve(ginfo,k+1,i,c->tag);

    }
        }
      }
      else
        ginfo->elmtcon[k][i].type = 'W';

      trip=0;
    }
  if(trip)
    fprintf(stderr,"side %d of element %d using coordinate indices"
      " (%d,%d) is not a boundary\n",i+1,k+1,vid,vid1);
      }

  /* free the con list */
  for(k = 0; k < npt; ++k){
    c = con[k].next;
    while(c){
      c1 = c->next;
      free(c);
      c = c1;
    }
  }
  free(con); free_imatrix(bndpts,0,0);

  return ginfo;
}

static void addcon(Cinfo *con, int elmt, int vert){
  Cinfo *c;

  /* go to end of list */
  for(c = con; c->next; c = c->next);

  c->next = (Cinfo *)calloc(1,sizeof(Cinfo));
  c = c->next;

  c->elmtid = elmt;
  c->vsid   = vert;
}

static void ReadBcond(FILE *fp, Gdinfo *ginfo){
  register int  i,k;
  int      nbcs;
  char     buf[BUFSIZ],*s;
  Bndinfo *b;
  Curinfo *c,*new_c;

  /* search for boundary data */
  rewind(fp);
  for(k = 0; k < MAXTYPES; ++k){
    while((s=fgets(buf,BUFSIZ,fp)) && !strstr(s,"Bndry"));
    if(s){
      if(ginfo->bnd){
  for(b=ginfo->bnd; b->next; b = b->next);
  b->next = (Bndinfo *)calloc(1,sizeof(Bndinfo));
  b = b->next;
      }
      else
  b = ginfo->bnd = (Bndinfo *)calloc(1,sizeof(Bndinfo));

      fscanf(fp,"%d", &b->region); fgets(buf,BUFSIZ,fp);
      fscanf(fp,"%1s",&b->type);   fgets(buf,BUFSIZ,fp);

      switch(b->type){
      case 'W': case 'O': case 'S': case 'B': case 'M': case 'I': case 'Z':
  break;
      case 'V': case 'F':
  fscanf(fp,"%d",&nbcs);fgets(buf,BUFSIZ,fp);
  b->nbcs = nbcs;
  for(i = 0; i < nbcs; ++i)
    fscanf(fp,"%lf",b->data.val+i);
  break;
      case 'v': case 'f': case 'm':
  fscanf(fp,"%d",&nbcs);fgets(buf,BUFSIZ,fp);
  b->nbcs = nbcs;
  for(i = 0; i < nbcs; ++i){
    fgets(buf,BUFSIZ,fp);
    b->data.str[i] = strdup(buf);
  }
  break;
      default:
  fprintf(stderr,"Unknown boundary type %c\n",b->type);
  break;
      }
    }
  }

  /* read curve information if any */
  rewind(fp);
  for(k = 0; k < MAXTYPES; ++k){
    while((s=fgets(buf,BUFSIZ,fp)) && !strstr(s,"Curve"));
    if(s){

      new_c = (Curinfo *)calloc(1,sizeof(Curinfo));

      fscanf(fp,"%d",&new_c->region); fgets(buf,BUFSIZ,fp);
      fscanf(fp,"%1s",&new_c->type);  fgets(buf,BUFSIZ,fp);

      new_c->tag = new_c->type;

      if(ginfo->curve){
  for(c=ginfo->curve; c; c = c->next)
    if(c->tag == new_c->tag) // check to see if tag has been used
      new_c->tag = c->tag+1;

  for(c=ginfo->curve; c->next; c = c->next);
  c->next = new_c;
      }
      else
  ginfo->curve = new_c;

      switch(new_c->type){
      case 'C':
  fscanf(fp,"%lf",&new_c->info.arc.radius);
  fgets(buf,BUFSIZ,fp);
  fscanf(fp,"%lf%lf",&new_c->info.arc.xc,&new_c->info.arc.yc);
  fgets(buf,BUFSIZ,fp);
  break;
      case 'S':
  break;
      default:
  fprintf(stderr,"type %c not known\n",new_c->type);
  break;
      }
    }
  }
}

static void addcurve(Gdinfo *ginfo, int eid, int iside, char type){
  Curlist *c;

  if(ginfo->clist){
    for(c=ginfo->clist; c->next; c = c->next);
    c->next = (Curlist *)calloc(1,sizeof(Curlist));
    c = c->next;
  }
  else
    c = ginfo->clist = (Curlist *)calloc(1,sizeof(Curlist));

  c->eid   = eid;
  c->iside = iside;
  c->type  = type;
}

/* write output file */
static void print_preamble (FILE *);
static void print_postamble(FILE *);

static void Write(FILE *fp, Gdinfo *ginfo){
  register int  i,j,k;
  const    int nel = ginfo->nel;
  double   *x = ginfo->x;
  double   *y = ginfo->y;

  print_preamble(fp);

  /* write mesh */
  fprintf(fp,
  "**MESH DATA** 1st line is X of corner 1,2,3,4. 2nd line is Y.\n");
  fprintf(fp,"\t %d \t %d \t %d \t NEL,NDIM,NELV\n",nel,2,nel);

  for(i = 0; i < nel; ++i){
    fprintf(fp,"\tELEMENT    %d   [  1A]    GROUP     0\n",i+1);
    for(j = 0; j < 3; ++j)
      fprintf(fp,"  %12.6lf",x[ginfo->elmtpts[i][j]-1]);
    fprintf(fp,"  %12.6lf\n",x[ginfo->elmtpts[i][0]-1]);
    for(j = 0; j < 3; ++j)
      fprintf(fp,"  %12.6lf",y[ginfo->elmtpts[i][j]-1]);
    fprintf(fp,"  %12.6lf\n",y[ginfo->elmtpts[i][0]-1]);
  }

  fprintf(fp," ***** CURVED SIDE DATA ***** \n");
  if(ginfo->clist){
    int      n;
    Curlist *cl;
    Curinfo *c;

    /* count to see how many types */
    for(n=0,c = ginfo->curve; c; c = c->next) ++n;

    fprintf(fp,"%d\tNumber of curve types \n",n);

    for(c = ginfo->curve; c; c = c->next)
      switch(c->type){
      case 'C':
  fprintf(fp,"Circle\n");
  fprintf(fp,"%lf  %lf  %lf  %c     xc yc rad tagid\n",c->info.arc.xc,
    c->info.arc.yc,c->info.arc.radius,c->tag);
  break;
      case 'S':
  fprintf(fp,"Straight sided\n");
  fprintf(fp,"%c\ttagid\n",c->tag);
  break;
      default:
  fprintf(stderr,"unknown curve type %c\n",c->tag);
      }

    for(n=0,cl = ginfo->clist; cl; cl = cl->next) ++n;
    fprintf(fp,"%d\tnumber of curved sides, (iside, iel, tagid) \n",n);
    for(cl=ginfo->clist,n=0;cl;cl = cl->next,++n){
      fprintf(fp,"%d  %d  %c  \t",cl->iside+1,cl->eid,cl->type);
      if((n+1)%4 == 0) fprintf(fp,"\n");
    }
    if(!(n%4 == 0))fprintf(fp,"\n");
  }
  else
    fprintf(fp," 0 Curved sides follow IEDGE,IEL,CURVE(I),I=1,5, CCURVE \n");

    /* write fluid boundary conditions */
  fprintf(fp," ***** BOUNDARY CONDITIONS *****\n");
  fprintf(fp," ***** FLUID BOUNDARY CONDITIONS *****\n");
  for(i = 0; i < nel; ++i){
    for(j = 0; j < 3; ++j){
      switch(ginfo->elmtcon[i][j].type){
      case 'E':
  fprintf(fp,"E  %d  %d  %12.6lf  %12.6lf      0.000000 \n",i+1,j+1,
    (double) ginfo->elmtcon[i][j].elmtid,
    (double) ginfo->elmtcon[i][j].vsid+1);
  break;
      case 'W': case 'O': case 'S': case 'B': case 'M': case 'I': case 'Z':
  fprintf(fp,"%c  %d  %d      0.000000      0.000000      0.000000 \n",
    ginfo->elmtcon[i][j].type,i+1,j+1);
  break;
      case 'V': case 'F':
  fprintf(fp,"%c  %d  %d      %12.6lf      %12.6lf      %12.6lf \n",
    ginfo->elmtcon[i][j].type,i+1,j+1,ginfo->elmtcon[i][j].f[0],
    ginfo->elmtcon[i][j].f[1],ginfo->elmtcon[i][j].f[2]);
  break;
      case 'v': case 'f':  case 'm':
  fprintf(fp,"%c  %d  %d      0.000000      0.000000      0.000000 \n",
    ginfo->elmtcon[i][j].type,i+1,j+1);
  k = 0;
  while(ginfo->elmtcon[i][j].str[k])
    fprintf(fp,"\t%s",ginfo->elmtcon[i][j].str[k++]);
  break;
      default:
  fprintf(stderr,"Unknown boundary type %c in Write\n",
    ginfo->elmtcon[i][j].type);
  fprintf(fp,"W  %d  %d      0.000000      0.000000      0.000000 \n",
    i+1,j+1);
      }
    }
    fprintf(fp, "not used\n");
  }
  print_postamble(fp);
}

static void print_preamble(FILE *fp){
  fprintf(fp,"****** PARAMETERS *****\n");
  fprintf(fp,"    Felisa-Nektar file \n");
  fprintf(fp,"  2 DIMENSIONAL RUN\n");
  fprintf(fp,"  1 PARAMETERS FOLLOW\n");
  fprintf(fp,"  7.00000         NORDER\n");
  fprintf(fp,"  0  Lines of passive scalar data follows2 CONDUCT; 2RHOCP\n");
  fprintf(fp,"  0  LOGICAL SWITCHES FOLLOW\n");
  fprintf(fp,"  Not used \n");
}

static void print_postamble(FILE *fp){
  fprintf(fp,"***** NO THERMAL BOUNDARY CONDITIONS *****\n");
  fprintf(fp,"  0         INITIAL CONDITIONS *****\n");
  fprintf(fp," ***** DRIVE FORCE DATA ***** PRESSURE GRAD, FLOW, Q\n");
  fprintf(fp," 0                 Lines of Drive force data follow\n");
  fprintf(fp," ***** Variable Property Data ***** Overrrides Parameter data.\n");
  fprintf(fp,"  1 Lines follow.\n");
  fprintf(fp,"  0 PACKETS OF DATA FOLLOW\n");
  fprintf(fp," ***** HISTORY AND INTEGRAL DATA *****\n");
  fprintf(fp,"  0   POINTS.  Hcode, I,J,H,IEL\n");
  fprintf(fp," ***** OUTPUT FIELD SPECIFICATION *****\n");
  fprintf(fp,"  0 SPECIFICATIONS FOLLOW\n");
}


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

  while (--argc && (*++argv)[0] == '-') {
    while (c = *++argv[0])                  /* more to parse... */
      switch (c) {
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
      sprintf(fname, "%s.gri", *argv);
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
