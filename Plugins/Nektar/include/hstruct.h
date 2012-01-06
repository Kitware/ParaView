/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/include/hstruct.h,v $
 * $Revision: 1.3 $
 * $Date: 2006/05/13 09:15:23 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
/*
 * This file contains all structures used by the Hotel Library
 */

#ifndef HSTRUCT_H
#define HSTRUCT_H


typedef struct mminfo {
  int     L;          /* basis order     */
  double *mat;        /* factored matrix */
  struct mminfo *next;
} MMinfo;



typedef struct coninfo {
  int cprocid; /* processor to send information                    */
  int datlen;  /* length of assembled communicated data            */
  int nedges;  /* number of edges between local proc and cprocid   */
  int *elmtid; /* local element id of connecting edge              */
  int *edgeid;  /* edge id within local element for connecting data */
  struct coninfo *next;
} ConInfo;



typedef struct para_info{
  int nprocs;         /* number of processors                    */
  int procid;         /* processor id                            */
  int gnel;           /* global number of elements               */
  int nloop;          /* number of elements in loop              */
  int *eloop;         /* loop over elements                      */


  /* the next three are global information */
  int *etypes;        /* type of each element  (global)          */
  int *eedges;        /* sum of Edges + Faces for each element   */
  int *efaces;        /* sum of Edges + Faces for each element   */
  int *efacets;       /* cumulative sum of Edges + Faces for each element   */
  int *cumfacets;     /* cumulative sum of Edges + Faces for each element   */

  int *partition;     /* list of which partition each global element is in */

  int ncprocs;           /* number of connecting processors           */
  ConInfo *cinfo;        /* information about the connecting edge/face*/
  int npface;            /* number of faces/edges along parallel ptch */
  int *npfeid;           /* elmt ids of global patched faces/edges    */
  int *npfside;          /* face ids of global patched faces/edges    */

  int *AdjacentPartitions; /* list of partitions which share at least one vertex with this partition */
  int  NAdjacentPartitions;/* number of adjacent partitions */

} ParaInfo;

typedef struct mode{
  double *a; /* modal component in a direction g^1(a) */
  double *b; /* modal component in b direction g^2(b) */
  double *c; /* modal component in c direction g^3(c) */
} Mode;

typedef struct bstore{
  int q;             /* quadrature order   */
  double **s;        /* vector of pointers */
  struct bstore *next;
} Bstore;

typedef struct basis {
  int        id;    /* id given by L                  */
  int     qa,qb,qc; /* quadrature points used         */
  Mode      *vert;  /* vertex   basis info            */
  Mode     **edge;  /* edge     basis info            */
  Mode    ***face;  /* face     basis info            */
  Mode    ***intr;  /* interior basis info            */
  Bstore  *sa;      /* link list to stored bases in a */
  Bstore  *sb;      /* link list to stored bases in b */
  Bstore  *sc;      /* link list to stored bases in c */
  struct basis  *next;
} Basis;


typedef struct locmat{
  int    asize;
  int    csize;
  int    *list;
  double **a;
  double **b;
  double **c;
  double **d;
} LocMat;



typedef struct locmatdiv{
  int     bsize;
  int     isize;
  int     rows;
  double **Atot;
  double **Dxb;
  double **Dxi;
  double **Dyb;
  double **Dyi;
} LocMatDiv;




#endif
