
/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/include/nekcomp.h,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/13 09:15:23 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#include <hotel.h>
#define EPSil    0.000001
#define Gamma    1.4      /* new for shock tube */
#define MU       1.
#define Prandtl  0.72

/* parameters */
#define HP_MAX  128   /* Maximum number of history points */

#define MAXFIELDVAL 100000 /* field value which will force code to exit if
            exceeded */

extern int Nfields;

#ifndef DIM
#error  Please define DIM to be 2 or 3!
#endif

/* general include files */
#include <math.h>
#include <veclib.h>


typedef enum {
  Scalar,        /* scalar advection equation */
  Burgers,       /* Burgers equation          */
  Euler,         /* Euler equation            */
  Parabolic,     /* Parabolic equation        */
  NavierStokes,  /* Navier Stokes equation    */
  Mhd_Euler,     /* Euler with Magnetic field         */
  Mhd_NS,        /* Navier Stokes with Magnetic field */
  Mhd_NS_pot,    /* Navier Stokes with Magnetic field */
  NonCon_Mhd_Euler
}EQTYPE;

typedef enum {                    /* ......... ACTION Flags .......... */
  Transformed,                    /* Transformed  space                */
  Physical                        /* Phyiscal     space                */
} ACTION;

typedef struct hpnt {             /* ......... HISTORY POINT ......... */
  int         id         ;        /* ID number of the element          */
  int         i, j, k    ;        /* Location in the mesh ... (i,j,k)  */
  char        flags               /* The fields to echo.               */
              [MAXFIELDS];        /*   (up to maxfields)               */
  ACTION      mode       ;        /* Physical or transformed  space    */
  struct hpnt *next      ;        /* Pointer to the next point         */
} HisPoint;


/* local structure for global number */
typedef struct gmapping {
  int nvs;
  int nvg;
  int nes;
  int neg;
  int *vgid;
  int *egid;
  int nfs;
  int nfg;
  int *fgid;

  int nsolve;
  int nglobal;
} Gmap;




/* Solution Domain contains global information about solution domain */
typedef struct dom {
  char     *name;                       /* the name of the program */
#ifdef PARALLEL
  FILE     **fld_file;                   /* field file pointer */
#else
  FILE     *fld_file;                   /* field file pointer */
#endif

  FILE     *his_file;                   /* history file pointer */
  FILE     *fce_file;                   /* history file pointer */

  Bsystem  *Ubsys;

  Element_List  *Us [MAXFIELDS];               /* state vectors    */
  Element_List  *Uf [MAXFIELDS];               /* RHS for Adams-B  */

  double       ***u;
  double       ***uf;


  Bndry    *Ubc[MAXFIELDS];               /* boundary conditions for u,v,T */

  HisPoint *his_list;                   /* link list for history points */
  int      *edge_size;                  /* number of Gauss points for
               the edges: for conv. step */

  Element_List *Gauge;
  Element_List *GaugeF;
  Bsystem      *Gauge_sys;
  Bndry        *GaugeBc;
  double      **gauge;
  double      **gauges;
  double      **gaugef;
  char **soln;

} Domain;


/* functions in drive.c */

/* functions in advection.c */
void   Fill_edges          (Element_List **Us, Bndry **Ubc);
void   Conv_Step           (Domain *Omega);
void   Calc_surface_int    (Element_List *Us,  Element_List *Uf);
void   GetEdge             (Element_List *E, double *from, int face, double *to);
void   Add_surface_contrib (Element_List *Us, Element_List *Uf);

/* functions in prepost.c*/
void        parse_args   (int, char **);
void        PostProcess  (Domain *, int, double);
Domain     *PreProcess   (char *);
void        Comp_Entropy (Element_List **U, Element_List *S);

/* functions in io.c */
void      ReadParams    (FILE *);
void      ReadPscals    (FILE *);
void      ReadLogics    (FILE *);
Element_List  *ReadMesh      (FILE *, char *);
void      ReadICs       (FILE *fp, Domain *omega);
void      ReadICs       (FILE *fp, Domain *omega, Element_List *Mesh);
void      summary       (void);
void      ReadSetLink   (FILE *fp, Element_List U[]);
Bndry    *ReadBCs       (FILE *fp, Element_List U[]);
Bndry    *bsort         (Bndry *, int );
void      ReadSetLink   (FILE *fp, Element_List U[]);
void      read_connect  (char *name, Element_List *E);
void      ReadOrderFile (char *name, Element_List *E);
void ReadSoln(FILE* fp, Domain* omega);

/* structure specific to bwoptim and recurSC */
typedef struct facet{
  int  id;
  struct facet *next;
} Facet;

typedef struct fctcon{
  int ncon;
  Facet *f;
} Fctcon;


/* function in bwoptim.c */
void bandwidthopt (Element *E, Bsystem *Bsys);
void MinOrdering   (int nsols,  Fctcon *ptcon, int *newmap);
void addfct(Fctcon *con, int *pts, int n);
void free_Fctcon   (Fctcon *con, int n);

/* functions in recurrSC.c */
void Recursive_SC_decom(Element *E, Bsystem *B);

/* functions in visc.c */
void Diff_Step_Parabolic (Domain *Omega);
void Diff_Step_NS        (Domain *Omega);
void Set_Dir_BCs         (Element_List **U, Bndry **Ubc);

void Save_Conv    (Domain *Omega);
void Restore_Conv (Domain *Omega);
void Free_Conv    (void);

// functions in analyser
void Analyser (Domain *omega, int step, double time);


// functions in Compress
void set_elmt_edges(Element_List *E);
void set_edge_geofac(Element_List *U);
void Comp_Div(Element *E, double *u, double *v, double *w, double *div, double *wk);


// functions in Basis_ortho

void InnerProduct_Orth(Element_List *U, Element_List *Uf);
void Jtransbwd_Orth(Element_List *U, Element_List *Uf);

void InnerProduct_Orth(Element_List *U, Element_List *Uf,int);
void Jtransbwd_Orth(Element_List *U, Element_List *Uf,int);

void Invert_orth_mass(Element_List *U);
void EInvert_orth_mass(double *in, Element *U);
void EJtransbwd_Orth(Element *U, double *in, double *out, double *wk);
void EInnerProduct_Orth(Element *U, double *in, double *out, double *wk);


// function in forces
void forces(Domain *Omega, double time);

// functions in mpinektar
void exchange_edges(Element_List **Us);
void exchange_edges(Element_List **Us, Bsystem *Ubsys);
void SendRecvRep(void *buf, int len, int proc);

#ifdef PARALLEL
void init_mpi(int *, char **);
void LocalNumScheme  (Element_List *E, Bsystem *Bsys, Gmap *gmap);
Gmap *GlobalNumScheme(Element_List *E, Bndry *Ebc);
void free_gmap(Gmap *gmap);

void set_con_info(Element_List *U, int nel, int *partition);
void ReadICs (FILE *fp, Domain *omega, Element_List *Mesh);
Bndry *ReadMeshBCs (FILE *fp, Element_List *Mesh);
Bsystem *gen_bsystem(Element_List *, Gmap *);

#endif


#if 0
#ifndef CPS
#define CPS
double       st, cps = (double)CLOCKS_PER_SEC;
#endif
#define Timing(s) \
 fprintf(stdout,"%s Took %g seconds\n",s,(clock()-st)/cps); \
 st = clock();
#else
#define Timing(s) \
 /* Nothing */
#endif
