/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/include/nektarG.h,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/13 09:15:23 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#include <hotel.h>

#ifndef DIM
#error  Please define DIM to be 2 or 3!
#endif

/* general include files */
#include <math.h>
#include <veclib.h>

/* parameters */
#define HP_MAX  128   /* Maximum number of history points */

typedef enum {                    /* ......... ACTION Flags .......... */
  Rotational,                     /* N(U) = U x curl U                 */
  Convective,                     /* N(U) = U . grad U                 */
  Stokes,                         /* N(U) = 0  [drive force only]      */
  Alternating,                    /* N(U^n,T) = U.grad T               */
                                  /* N(U^(n+1),T) = div (U T)          */
  Pressure,                       /* div (U'/dt)                       */
  Viscous,                        /* (U' - dt grad P) Re / dt          */
  Prep,                           /* Run the PREP phase of MakeF()     */
  Post,                           /* Run the POST phase of MakeF()     */
  Gauge,                          //
  Transformed,                    /* Transformed  space                */
  Physical,                       /* Phyiscal     space                */
  TransVert,                      /* Take history points from vertices */
  TransEdge,                      /* Take history points from mid edge */
  Poisson   = 0,
  Helmholtz = 1,
  Laplace   = 2
} ACTION;

typedef struct hpnt {             /* ......... HISTORY POINT ......... */
  int         id         ;        /* ID number of the element          */
  int         i, j, k    ;        /* Location in the mesh ... (i,j,k)  */
  char        flags               /* The fields to echo.               */
            [_MAX_FIELDS];        /*   (up to maxfields)               */
  ACTION      mode       ;        /* Physical or Fourier space         */
  struct hpnt *next      ;        /* Pointer to the next point         */
} HisPoint;


typedef struct gf_ {                  /* ....... Green's Function ........ */
  int           order               ; /* Time-order of the current field   */
  Bndry        *Gbc                 ; /* Special boundary condition array  */
  Element_List *basis               ; /* Basis velocity (U or W)           */
  Element_List *Gv[DIM][_MAX_ORDER]; /* Green's function velocities       */
  Element_List *Gp     [_MAX_ORDER]; /* Green's function pressure         */
  double        Fg     [_MAX_ORDER]; /* Green's function "force"          */
} GreensF;



/* Solution Domain contains global information about solution domain */
typedef struct dom {
  char     *name;           /* Name of run                       */
  char     **soln;
  FILE     *fld_file;       /* field file pointer                */
  FILE     *his_file;       /* history file pointer              */
#ifdef FORCES
  FILE     *fce_file;       /* force file                        */
#endif
  HisPoint *his_list;       /* link list for history points      */
  Element_List  **A, **Af;
  Element_List  *Phi, *PhiF;
  Element_List  *U, *V, *W;     /* Velocity and Pressure fields      */
  Element_List  *Uf, *Vf, *Wf;     /* Velocity and Pressure fields      */

  Bndry    *Ubc,*Vbc,*Wbc;      /* Boundary  conditons               */
  Bndry    **Abc;
  Bndry    *PhiBC;
  Bsystem  *A_sys;            /* pressure Poisson   matrix system  */
  Bsystem  *Phi_sys;            /* pressure Poisson   matrix system  */
  Bsystem  *U_sys;            /* pressure Poisson   matrix system  */

  Element_List **Asegs;
  Bsystem       *Asegs_sys;

  double  ***a;
  double  ***af;

} Domain;

/* function in drive.c  */
void solve(Element_List *U, Element_List *Uf,Bndry *Ubc,Bsystem *Ubsys,SolveType Stype,int step);

/* functions in prepost.c */
void      parse_args (int argc,  char **argv);
void      PostProcess(Domain *omega, int, double);
Domain   *PreProcess (int argc, char **argv);
void      set_vertex_links(Element_List *UL);

/* functions in io.c */
void      ReadParams     (FILE *rea);
void      ReadPscals     (FILE *rea);
void      ReadLogics     (FILE *rea);
Element_List  *ReadMesh       (FILE *rea,char *);
void      ReadICs        (FILE *, Domain *);
void      ReadDF         (FILE *fp, int nforces, ...);
void      summary        (void);
void      ReadSetLink    (FILE *fp, Element U[]);
Bndry    *ReadBCs        (FILE *fp, Element U[]);
Bndry    *bsort          (Bndry *, int );
void      read_connect   (FILE *name, Element_List *);
void      ReadOrderFile  (char *name,Element_List *E);
void      ReadHisData    (FILE *fp, Domain *omega);
void      ReadSoln       (FILE* fp, Domain* omega);
void      ReadDFunc      (FILE *fp, Domain *Omega);

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

/* functions in convective.c */
void VdgradV (Domain *omega);

/* functions in rotational.c */
void VxOmega (Domain *omega);

/* functions in pressure.c */
Bndry *BuildPBCs (Element_List *P, Bndry *temp);
void   SetPBCs   (Domain *omega);
void   GetFace   (Element *E, double *from, int face, double *to);

/* function in stokes.c      */
void StokesBC (Domain *omega);

/* functions in analyser.c   */
void Analyser (Domain *omega, int step, double time);

#ifdef FORCES
/* functions in forces */
void  forces (Domain *omega, double time);
#endif

// functions in gauge.C

Bndry *BuildABCs  (Element_List *, Bndry *, int);
Bndry *BuildPhiBCs(Element_List *, Bndry *);
void SetABCs(Domain *omega);

// Functions in smoother
Element_List *setup_seglist(Bndry *Ubc, char type);
Bsystem      *setup_segbsystem(Element_List *seg_list);
void          fill_seglist(Element_List *seg_list, Bndry *Ubc);
void          fill_bcs(Element_List *seg_list, Bndry *Ubc);
void          update_seg_vertices(Element_List *, Bndry *);
void update_surface(Element_List *EL, Element_List *EL_F,\
        Bsystem *Bsys, Bndry *BCs);
void smooth_surface(Domain *, Element_List *segs, Bsystem *Bsys, Bndry *BCs);
