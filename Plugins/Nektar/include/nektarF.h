/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/include/nektarF.h,v $
 * $Revision: 1.8 $
 * $Date: 2006/08/10 17:46:10 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
#include <hotel.h>

// MSB: Added for PSE -----------------------
#include <iostream>                        //
#include "MatSolve.h"                      //
using namespace MatSolve;                  //
// MSB: Added for PSE -----------------------

class MatSolve::StokesMatrix;

#ifdef PARALLEL
#include <mpi.h>
#define exit(a) {MPI_Abort(MPI_COMM_WORLD, -1); exit(a-1);}
#endif
#include <Fourier_List.h>

#ifndef DIM
#error  Please define DIM to be 2 or 3!
#endif

/* general include files */
#include <math.h>
#include <veclib.h>

/* parameters */
#define HP_MAX  128   /* Maximum number of history points */
int mynode();
int numnodes();

/* message tags */
#define W_MSG   9
#define F_MSG   99
#define H_MSG   999

#define ROOT if(mynode() == 0)
#define PROC1 if(mynode() == 1)

typedef enum {/* Fourier Transform Flags       */
 PtoF = -1,   /* Physical to Fourier Transform */
 FtoP = 1     /* Fourier to Physical Transform */
} FtransDir;

typedef enum {                  /* ........... SLVTYPE Flags ............. */
  Splitting,                    /* Splitting scheme                        */
  StokesSlv                     /* Stokes solver                           */
} SLVTYPE;

typedef enum {                    /* ......... ACTION Flags .......... */
  Rotational,                     /* N(U) = U x curl U                 */
  Convective,                     /* N(U) = U . grad U                 */
  Stokes,                         /* N(U) = 0  [drive force only]      */
  Alternating,                    /* N(U^n,T) = U.grad T               */
                                  /* N(U^(n+1),T) = div (U T)          */
  Linearised,                     /* Linearised convective term        */
  StokesS,                        /* MSB: Steady Stokes Solve          */
  Oseen,                          /* MSB: Oseen flow                   */
  OseenForce,                     /* MSB: Oseen forcing terms          */
  StokesForce,                    /* MSB: Stokes forcing terms         */
  PSEForce,                       /* MSB: PSE forcing terms            */
  Pressure,                       /* div (U'/dt)                       */
  Viscous,                        /* (U' - dt grad P) Re / dt          */
  Prep,                           /* Run the PREP phase of MakeF()     */
  Post,                           /* Run the POST phase of MakeF()     */
  Transformed,                    /* Transformed  space                */
  Quadrature,                     /* Quadrature   space                */
  Physical,                       /* Physical     space                */
  Fourier,                        /* Fourier      space                */
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

typedef struct intepts {
  int     npts           ;
  Coord    X             ;
  double **ui            ;
  double **ufr           ;
} Intepts;

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
  int nv_psolve;
} Gmap;


#ifdef MAP
typedef struct mppng {            /* ......... Mapping ............... */
  int       NZ                  ; /* Number of z-planes                */
  double    time                ; /* Current time                      */
  double   *d                   ; /* Displacement                      */
  double   *z                   ; /*   (z-deriv)                       */
  double   *zz                  ; /*   (zz-deriv)                      */
  double   *t                   ; /* Velocity                          */
  double   *tt                  ; /* Acceleration                      */
  double   *tz                  ; /*   (tz-deriv)                      */
  double   *tzz                 ; /*   (tzz-deriv)                     */
  double   *f                   ; /* Force                             */
} Map;

typedef struct mstatstr {         /* Moving Statistics information     */
  double *x;                      /* vector holding the x-coordinate   */
  double *y;                      /* vector holding the y-coordinate   */
  int    *sgnx;                   /* vector holding the sign of dx/dt  */
  int    *sgny;                   /* vector holding the sign of dy/dt  */
  int    *nstat;                  /* vector holding the # of samples   */
  int     n;                      /* number of sampling (x,y) points   */
} MStatStr;
#endif

/* Solution Domain contains global information about solution domain */
typedef struct dom {
  char     *name;                /* Name of run                       */
  char     **soln;
  FILE     *fld_file;            /* field file pointer                */
  FILE     *his_file;            /* history file pointer              */
  FILE     *int_file;            /* interpolation file pointer        */
  FILE     *fce_file;            /* force file                        */
  HisPoint *his_list;            /* link list for history points      */
  Intepts  *int_list;            /* link list for interpolation points*/

  Element_List  *U,  *V,  *W, *P;/* Velocity and Pressure fields      */
  Element_List  *Uf, *Vf, *Wf;   /* --------------------------------- */

// MSB: Added for PSE ----------------------------
  Element_List *Pf, *Ps; // MSB: Ps for Stokes  //
  StokesMatrix **StkSlv;                         //
// MSB: Added for PSE ----------------------------

  double **u, **v, **w;          /* Field storage */
  double **uf,**vf,**wf;         /* Non-linear forcing storage */
  double  *us, *vs, *ws;

 //  double **wbcs;
  Bndry    **Ubc,**Vbc,**Wbc;    /* Boundary  conditons               */
  Bndry    **Pbc;
  Bsystem  **Usys;               /* Velocity Helmholtz matrix system  */
  Bsystem  **Vsys;               /* Velocity Helmholtz matrix system  */
  Bsystem  **Wsys;               /* Velocity Helmholtz matrix system  */
  Bsystem  **Pressure_sys;       /* pressure Poisson   matrix system  */

#ifdef MAP
  Map      *mapx,  *mapy;        /* Mapping in x and y                */
  MStatStr *mstat;               /* Moving Statistics information     */
  Element_List  *Wlast;
#endif
} Domain;

/* function in drive.C  */
void solve(Element_List *U, Element_List *Uf,
     Bndry **Ubc,Bsystem **Ubsys,SolveType Stype,int step);
int Navier_Stokes(Domain *Omega, double t0, double tN);

/* functions in prepost.C */
void      parse_args (int argc,  char **argv);
void      PostProcess(Domain *omega, int, double);
Domain   *PreProcess (int argc, char **argv);
int       backup         (char *path1);

/* functions in io.C */
void SetEpsilon(FILE* fp, Element_List *U, Bndry *Ubc,
    Metric *lambda); /* SV */
void      ReadParams     (FILE *rea);
void      ReadPscals     (FILE *rea);
void      ReadLogics     (FILE *rea);
Element_List  *ReadMesh       (FILE *rea,char *);
void      ReadICs        (FILE *, Domain *);
void      ReadDF         (FILE *fp, int nforces, ...);
void      summary        (void);
void      ReadSetLink    (FILE *fp, Element_List U[]);
#ifdef MAP
Bndry    **ReadBCs        (FILE *fp, Element_List U[], double *x, double *y);
#else
Bndry    **ReadBCs        (FILE *fp, Element_List U[]);
#endif
Bndry    *bsort          (Bndry *, int );
void      read_connect   (FILE *name, Element_List *);
void      ReadOrderFile  (char *name,Element_List *E);
void      ReadHisData    (FILE *fp, Domain *omega);
void      ReadIntData    (FILE *fp, Domain *omega);
void      ReadSoln       (FILE *fp, Domain *omega);
#ifdef MAP
void      ReadMStatPoints(FILE* fp, Domain* omega);
#endif
void      ReadWave(FILE *fp, double **wave, Element_List *U, Domain *omega);

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

/* functions in mlevel.C */
void Mlevel_SC_decom(Element_List *E, Bsystem *B);

/* functions in convective.C */
void VdgradV (Domain *omega);
void IsoStats(Domain *omega);

#if defined (FLOK)|| defined (LNS)
/* functions in DN.C */
void DN(Domain *omega);
#endif

/* functions in rotational.C */
void      VxOmega (Domain *omega);

/* functions in pressure.C */
Bndry *BuildPBCs (Element_List *P, Bndry *temp);
void   SetPBCs   (Domain *omega);
void   GetFace   (Element *E, double *from, int face, double *to);

/* function in stokes.C      */
void StokesBC (Domain *omega);

/* functions in analyser.C   */
void Analyser (Domain *omega, int step, double time);
void averagefields (Domain *omega, int nsteps, double time);
#ifdef MAP
double VolInt (Element_List *U, double shift);
#else
double VolInt (Element_List *U);
#endif
double L2norm (Element_List *V);
double GL2(Element_List *Div);
void average_u2_avg(Domain *omega, int step, double time);
void average_u2e_avg(Domain *omega, int step, double time); /* VARV */
void   init_avg(Domain *omega);

/* functions in mpinektar.c */
void gsync ();


/* functions in wannier.C */
#ifdef HEIMINEZ
void Heim_define_ICs (Element_List  *V);
void Heim_reset_ICs  (Element_List **V);
void Heim_error      (Element_List **V, int j);
void Heim_Energy     (Element_List **V, int j, double *l2, double *area);
void Heim_get_sol    (double ***h, double ***hd);
#endif
/* functions in cfl.C */
double cfl_checker(Domain *omega, double dt);

/* Functions in Fourier_List.C */
void init_rfft_struct();
int parid(int i);
double Beta(int i);
void setup_transfer_space(Domain *omega);

int  readFieldF (FILE *fp, Field *f, Element_List *E);
int writeFieldF (FILE *fp, Field *f);
void WritefieldF(FILE *fp, char *name, int step, double t,
     int nfields, Element_List *U[]);
void MWritefieldF(FILE *fp, char *name, int step, double t,
     int nfields, Element_List *U, double **mat, char *types);
void pWriteFieldF(FILE *fp, char *name, char *fname, int step, double t,
      int nfields, Element_List *U[]);
void pMWriteFieldF(FILE *fp, char *name, char *fname, int step, double t,
      int nfields, Element_List *U, double **mat, char *types);
void copyfieldF(Field *f, int pos, Element_List *U);
void TransformBCs(Domain *Omega, Nek_Trans_Type ntt);
#ifdef MAP
void GTerror(Element_List *U, Map *mapx, Map *mapy, char *string);
#else
void GTerror(Element_List *U, char *string);
#endif

double zmesh(int);
#ifdef MAP
void Grad_zz(Element_List *ELin, Element_List *ELout);
void Grad_zz_d(Element_List *ELin, double *ELout);

void Set_mapped_field(Element_List *EL, char *string, Map *mapx, Map *mapy);
#endif

/* functions in forces */
void  forces (Domain *omega, double time);

/* functions in interp */
void interp(Domain *omega);

void Jtransbwd_Orth      (Element_List *EL, Element_List *ELf);
void Jtransbwd_Orth_Plane(Element_List *EL, Element_List *ELf);

// MSB: Added for PSE------------------------------------
// Needs to be included after Domain is defined        //
#include "stokes_solve_F.h"                              //
// MSB: Added for PSE------------------------------------
