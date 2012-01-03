/*
 * Utilities for PRISM
 *
 * RCS Information
 * ------------------------------------
 * $Author: ssherw $
 * $Date: 2004/01/27 23:47:34 $
 * $RCSfile: gen_utils.h,v $
 * $Revision: 1.1 $
 * ---------------------------------------------------------------------- */

#ifndef   SEM_UTILS_H
#define   SEM_UTILS_H

#include <stdio.h>
#include <math.h>
#include <veclib.h>

#define MAXARGS  16            /* maximum # of other arguments */
#define UNSET    0xffffff      /* flag for an unset parameter  */

extern char *prog;
extern char *usage, *help,     /* defined by the application */
            *rcsid, *author;

typedef struct {               /* structure for storing fp/names */
  FILE *fp;
  char *name;
} aFile;

typedef struct {             /* .......  File structure ........ */
  aFile  in      ;           /* primary input file               */
  aFile  out     ;           /* primary output file              */
  aFile  mesh    ;           /* mesh coordinates file [optional] */
  aFile  rea     ;           /* session file          [optional] */
  aFile  mor     ;           /* connectivity file     [optional] */
} FileList;                  /* ................................ */


typedef struct {             /* .......  Mesh structure  ....... */
  int    nr, ns, nz, nel ;
  double *x, *y, *z      ;
} Input_Mesh;


/* Prototypes, listed by source file */


/* Functions from sem_utils.c */

#ifndef  error_msg
#define  error_msg(a) {fprintf(stderr,#a"\n"); exit(-1);}
#endif
int      generic_args   (int argc, char *argv[], FileList *f);
void     Setup          (FileList *f, Element **U, int nfields);
void     InterpToEqui   (int nfs, int qa, int qb, double *in, double *out);

void free_gmap(Gmap *gmap);
void Reflect_Global_Velocity(Element_List *Mesh, Bndry *Meshbcs, int dir);


#ifdef MAP
// CE107
void mapField (Element_List **E, Map *mapx, Map *mapy, int dir);
void mapfield (Element_List **E, Map *mapx, Map *mapy, int dir);
#endif

#endif
