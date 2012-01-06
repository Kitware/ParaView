/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/Utilities/src/checkrea.C,v $
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

char *prog   = "checkrea";
char *usage  = "checkrea:  [options]  input[.rea]\n";
char *author = "";
char *rcsid  = "";
char *help   = "";


/* ---------------------------------------------------------------------- */

static void setup (FileList *f, Element_List **U);
static void parse_util_args (int argc, char *argv[], FileList *f);
static void checkrea(Element_List *E, FILE *out);

main (int argc, char *argv[])
{
  FileList   f;
  Element_List   *master;

  parse_util_args(argc = generic_args (argc, argv, &f), argv, &f);
  setup (&f, &master);

  checkrea (master,f.out.fp);

  return 0;
}

static void setup (FileList *f, Element_List **U)
{
  ReadParams  (f->rea.fp);

  iparam_set("LQUAD",3);        /* set up minimum order */
  iparam_set("MQUAD",3);
  iparam_set("MODES",2);

  /* Generate the list of elements */
  *U = ReadMesh (f->rea.fp, strtok(f->rea.name,"."));

  return;
}

class Ord{
public:
  double x;
  double y;
  double z;
};

#define ANGTOL 2
static void checkrea(Element_List *EL, FILE *out){
  double abx,aby,abz, xa, xb, ya, yb;
  Vert *v;
  Element *E, *F;
  int   i,j, face;

  for(E=EL->fhead;E ; E = E->next){
    v = E->vert;

    switch(E->identify()){
    case Nek_Tri: case Nek_Quad:
      // Take curl of vectors stemming from vertex b
      if(((v[0].y-v[1].y)*(v[2].x-v[1].x)-(v[0].x-v[1].x)*(v[2].y-v[1].y))<0.0)
  fprintf(out,"Element %d is NOT counter-clockwise\n",E->id+1);
      else
  fprintf(out,"Element %d is counter-clockwise\n",E->id+1);
      break;

    case Nek_Tet:
      Ord edg[6],n[4];
      double fac,th;

      edg[0].x=v[1].x-v[0].x; edg[0].y=v[1].y-v[0].y; edg[0].z=v[1].z-v[0].z;
      edg[1].x=v[2].x-v[1].x; edg[1].y=v[2].y-v[1].y; edg[1].z=v[2].z-v[1].z;
      edg[2].x=v[2].x-v[0].x; edg[2].y=v[2].y-v[0].y; edg[2].z=v[2].z-v[0].z;
      edg[3].x=v[3].x-v[0].x; edg[3].y=v[3].y-v[0].y; edg[3].z=v[3].z-v[0].z;
      edg[4].x=v[3].x-v[1].x; edg[4].y=v[3].y-v[1].y; edg[4].z=v[3].z-v[1].z;
      edg[5].x=v[3].x-v[2].x; edg[5].y=v[3].y-v[2].y; edg[5].z=v[3].z-v[2].z;

      abx = (v[1].y-v[0].y)*(v[2].z-v[0].z) - (v[1].z-v[0].z)*(v[2].y-v[0].y);
      aby = (v[1].z-v[0].z)*(v[2].x-v[0].x) - (v[1].x-v[0].x)*(v[2].z-v[0].z);
      abz = (v[1].x-v[0].x)*(v[2].y-v[0].y) - (v[1].y-v[0].y)*(v[2].x-v[0].x);
      if(((v[3].x-v[0].x)*abx + (v[3].y-v[0].y)*aby + (v[3].z-v[0].z)*abz)<0.0)
  fprintf(out,"ERROR: Element %d is NOT counter-clockwise\n",E->id+1);
      else
  fprintf(out,"Element %d is counter-clockwise\n",E->id+1);

      // calc normal to fac 1
      n[0].x = edg[0].y*edg[2].z-edg[0].z*edg[2].y;
      n[0].y = edg[0].z*edg[2].x-edg[0].x*edg[2].z;
      n[0].z = edg[0].x*edg[2].y-edg[0].y*edg[2].x;
      fac = sqrt(n[0].x*n[0].x + n[0].y*n[0].y + n[0].z*n[0].z);
      n[0].x /= fac;      n[0].y /= fac;      n[0].z /= fac;
      // calc normal to fac 2
      n[1].x = edg[0].y*edg[3].z-edg[0].z*edg[3].y;
      n[1].y = edg[0].z*edg[3].x-edg[0].x*edg[3].z;
      n[1].z = edg[0].x*edg[3].y-edg[0].y*edg[3].x;
      fac = sqrt(n[1].x*n[1].x + n[1].y*n[1].y + n[1].z*n[1].z);
      n[1].x /= fac;      n[1].y /= fac;      n[1].z /= fac;
      // calc normal to fac 3
      n[2].x = edg[1].y*edg[4].z-edg[1].z*edg[4].y;
      n[2].y = edg[1].z*edg[4].x-edg[1].x*edg[4].z;
      n[2].z = edg[1].x*edg[4].y-edg[1].y*edg[4].x;
      fac = sqrt(n[2].x*n[2].x + n[2].y*n[2].y + n[2].z*n[2].z);
      n[2].x /= fac;      n[2].y /= fac;      n[2].z /= fac;
      // calc normal to fac
      n[3].x = edg[2].y*edg[3].z-edg[2].z*edg[3].y;
      n[3].y = edg[2].z*edg[3].x-edg[2].x*edg[3].z;
      n[3].z = edg[2].x*edg[3].y-edg[2].y*edg[3].x;
      fac = sqrt(n[3].x*n[3].x + n[3].y*n[3].y + n[3].z*n[3].z);
      n[3].x /= fac;      n[3].y /= fac;      n[3].z /= fac;

      for(i = 0; i < 3; ++i)
  for(j = i+1; j < 4; ++j){
    th =  fabs(acos(fabs(n[i].x*n[j].x+n[i].y*n[j].y+n[i].z*n[j].z)));
    th *= 180/M_PI;
    if(th  < ANGTOL){
      fprintf(out,"WARNING: Element %d is close to being a sliver "
        "(Tol < 2 deg)\n",E->id+1);
      break;
    }
  }


      break;
    }
    if(E->dim() == 2){
      for(i=0;i<E->Nedges;++i){
  fprintf(out, "Element: %d Edge: %d\n",
    E->id+1, i+1);
  xa = E->vert[i].x + E->vert[(i+1)%E->Nverts].x;
  ya = E->vert[i].y + E->vert[(i+1)%E->Nverts].y;

  if(E->edge[i].base){
    if(E->edge[i].link){
      F = EL->flist[E->edge[i].link->eid];
      face = E->edge[i].link->id;
      xb = F->vert[face].x + F->vert[(face+1)%F->Nverts].x;
      yb = F->vert[face].y + F->vert[(face+1)%F->Nverts].y;
    }
    else{
      F = EL->flist[E->edge[i].base->eid];
      face = E->edge[i].base->id;
      xb = F->vert[face].x + F->vert[(face+1)%F->Nverts].x;
      yb = F->vert[face].y + F->vert[(face+1)%F->Nverts].y;
    }
    fprintf(out, "Face center: %lg\n",
      sqrt((xa-xb)*(xa-xb)+(ya-yb)*(ya-yb)));
  }

  xa = E->vert[(i+1)%E->Nverts].x;     ya = E->vert[(i+1)%E->Nverts].y;
  xb = E->vert[(i+E->Nverts-1)%E->Nverts].x; yb = E->vert[(i+E->Nverts-1)%E->Nverts].y;
  xa -=  E->vert[i].x;  ya -=  E->vert[i].y;
  xb -=  E->vert[i].x;  yb -=  E->vert[i].y;

  xa = (xa*xb +ya*yb)/(sqrt(xa*xa+ya*ya)*sqrt(xb*xb+yb*yb));
  fprintf(out, "Angle: %lf\n", acos(xa)*180/M_PI);
      }
    }
  }
  fputs("Finished checking\n",out);

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
  break;
      default:
  fprintf(stderr, "%s: unknown option -- %c\n", prog, c);
  break;
      }
  }

  /* open the .rea file */

  if ((*argv)[0] == '-') {
    f->rea.fp = stdin;
  } else {
    strcpy (fname, *argv);
    if ((f->rea.fp = fopen(fname, "r")) == (FILE*) NULL) {
      sprintf(fname, "%s.rea", *argv);
      if ((f->rea.fp = fopen(fname, "r")) == (FILE*) NULL) {
  fprintf(stderr, "%s: unable to open the input file -- %s or %s\n",
    prog, *argv, fname);
  exit(1);
      }
    }
    f->rea.name = strdup(fname);
  }

  if (option("verbose")) {
    fprintf (stderr, "%s: in = %s, rea = %s, out = %s\n", prog,
       f->in.name   ? f->in.name   : "<stdin>",  f->rea.name,
       f->out.name  ? f->out.name  : "<stdout>");
  }

  return;
}
