#ifndef H_NEKTAR
#define H_NEKTAR

/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/include/nektar.h,v $
 * $Revision: 1.9 $
 * $Date: 2006/08/15 19:31:06 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/


#ifdef PARALLEL
#include <mpi.h>
#endif

#include <hotel.h>
#include "stokes_solve.h"


#ifdef PBC_1D_LIN_ATREE
#include "PBC_1D_LIN_ATREE.h"
#endif

#ifdef ACCELERATOR
#include "PREDICTOR.h"
#endif

#ifdef POD_ACCELERATOR
#include "POD_PREDICTOR.h"
#endif


using namespace MatSolve;

#ifdef PARALLEL
#define exit(a) {MPI_Abort(MPI_COMM_WORLD, -1); exit(a-1);}
extern "C"
{
#include "gs.h"
}
#endif

#define MAXDIM 3

/* general include files */
#include <math.h>
#include <veclib.h>


/* parameters */
#define HP_MAX  128   /* Maximum number of history points */

typedef enum {
  Splitting,                    /* Splitting scheme                        */
  StokesSlv,                    /* Stokes solver                           */
  SubStep,                      /* Substepping scheme                      */
  SubStep_StokesSlv,            /* Stokes solver                           */
  Adjoint_StokesSlv,
  TurbCurr                      /* Splitting scheme with turbulent current */
} SLVTYPE;


typedef enum {                  /* ......... ACTION Flags .......... */
  Rotational,                   /* N(U) = U x curl U                 */
  Convective,                   /* N(U) = U . grad U                 */
  Stokes,                       /* N(U) = 0  [drive force only]      */
  Alternating,                  /* N(U^n,T) = U.grad T               */
                                /* N(U^(n+1),T) = div (U T)          */
  Linearised,                   /* Linearised convective term        */
  StokesS,                      /* Steady Stokes Solve               */
  Oseen,                        /* Oseen flow                        */
  OseenForce,
  StokesForce,                  /* Stokes forcing terms              */
  LambForce,                    /* N(U) = u^2 Curl w - j             */
  Pressure,                     /* div (U'/dt)                       */
  Viscous,                      /* (U' - dt grad P) Re / dt          */
  Prep,                         /* Run the PREP phase of MakeF()     */
  Post,                         /* Run the POST phase of MakeF()     */
  Transformed,                  /* Transformed  space                */
  Physical,                     /* Phyiscal     space                */
  TransVert,                    /* Take history points from vertices */
  TransEdge,                    /* Take history points from mid edge */
  Poisson   = 0,
  Helmholtz = 1,
  Laplace   = 2
} ACTION;

typedef struct hpnt {           /* ......... HISTORY POINT ......... */
  int         id         ;      /* ID number of the element          */
  int         i, j, k    ;      /* Location in the mesh ... (i,j,k)  */
  char        flags             /* The fields to echo.               */
            [_MAX_FIELDS];      /*   (up to maxfields)               */
  ACTION      mode       ;      /* Physical or Fourier space         */
  struct hpnt *next      ;      /* Pointer to the next point         */
} HisPoint;

// Ce107
typedef struct intepts {
  int     npts;
  Coord    X;
  double **ui;
} Intepts;


typedef struct gf_ {                  /* ....... Green's Function ........ */
  int           order                 ; /* Time-order of the current field   */
  Bndry        *Gbc                   ; /* Special boundary condition array  */
  Element_List *basis                 ; /* Basis velocity (U or W)           */
  Element_List *Gv[MAXDIM][_MAX_ORDER]; /* Green's function velocities       */
  Element_List *Gp        [_MAX_ORDER]; /* Green's function pressure         */
  double        Fg        [_MAX_ORDER]; /* Green's function "force"          */
} GreensF;


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

/* time dependent function inputs -- ie. womersley flow */
typedef struct time_str {  /* string input      */
  char *TimeDepend;
} Time_str;

/* info for a function of the form:
   t = (ccos[i]*cos(wnum[i]*t) + csin[i]*sin(wnum[i]*i*t)) */

typedef struct time_fexp {  /* fourier expansion */
  int nmodes;
  double wnum;
  double *ccos;
  double *csin;
} Time_fexp;

typedef enum{
  ENCODED,                /* fourier encoded values */
  RAW,                    /* actual measure,ents    */
  PRESSURE,
  MASSFLOW
} TwomType;

typedef struct time_wexp {  /* Womersley expansion fourier expansion */
  int nmodes;
  int nraw;
  double scal0;
  double radius;
  double axispt[MAXDIM];
  double axisnm[MAXDIM];
  double wnum;
  double *ccos;
  double *csin;
  double *raw;
  TwomType type;
  TwomType form;
} Time_wexp;

typedef union tfuninfo {
  Time_str  string [MAXDIM];
  Time_fexp fexpand[MAXDIM];
  Time_wexp wexpand;
} TfunInfo;

typedef enum{
  String,                 /* input string            */
  Fourier,                /* input fourier expansion */
  Womersley               /* input womersley expansion */
} TfunType;

typedef struct tfuct {
  TfunType    type;             /* type of time function input */
  TfunInfo    info;             /* info for function           */
}Tfunct;


#ifdef BMOTION2D
typedef enum motiontype {
  Prescribed,             // no body motion
  Heave,                 // heave motion only
  InLine,                // Inline motion only
  Rotation,              // Rotation only
  HeaveRotation          // heave and rotation
} MotionType;

typedef struct motion2d {
  MotionType motiontype; /* enumeration list with motion type           */
  double stheta;       /* inertial moment of the section               */
  double Itheta;       /* static moment of the section                 */
  double mass;         /* mass of the section                          */
  double cc[2];        /* damping coefficients for each degree of      */
                       /* freedom, cc[0] for y and cc[1] for theta     */
  double ck[2];        /* stiffness coefficients                       */
  double cl[2];        /* scaling constants for force and moment       */
  double dp[2];        /* displacement at previous time level          */
  double dc[2];        /* displacement at current time level           */
  double vp[2];        /* first derivatives at previous time level     */
  double vc[2];        /* first derivatives at current time level      */
  double ap[2];        /* second derivatives at previous time level    */
  double ac[2];        /* second derivatives at current time level     */

  double f[3];        /* for storing forces at the previous step.      */
  double aot;          /* Angle of attach in HM mode                   */
  double x[3];
  double alpha[3];
  double Ured_y;
  double Ured_alpha;
  double zeta_y;
  double zeta_alpha;
  double mass_ratio;
  double inertial_mom;
} Motion2d;
#endif

/* Solution Domain contains global information about solution domain */
typedef struct dom {
  int      step;
  char     *name;            /* Name of run                      */
  char     *remote_hostname; /* Name of remote client            */
  char     **soln;
  double   dt;

  FILE     *fld_file;       /* field file pointer                */
  FILE     *dat_file;       /* field file pointer                */
  FILE     *his_file;       /* history file pointer              */
  FILE     *fce_file;       /* force file                        */
  FILE     *flo_file;       /* flow rate outlets/inlet           */
  FILE     *pre_file;       /* pressure  at outlets/inlet        */
  FILE     *mom_file;       /* momentum flux at outlets/inlet    */
  HisPoint *his_list;       /* link list for history points      */
  Tfunct   *Tfun;           /* time dependent into conditions    */

  Element_List  *U, *V, *W, *P;  /* Velocity and Pressure fields      */
  Element_List  *Uf;             /* --------------------------------- */
  Element_List  *Vf;             /*        Multi-step storage         */
  Element_List  *Wf;             /* --------------------------------- */
  Element_List  *Pf;
  Element_List  *Lfx,*Lfy;       /* forcing terms to lamb vector      */

  MatSolve::StokesMatrix *StkSlv;

  double **u;                   /* Field storage */
  double **v;                   /* Field storage */
  double **w;                   /* Field storage */

  double **uf;                   /* Non-linear forcing storage */
  double **vf;                   /* Non-linear forcing storage */
  double **wf;                   /* Non-linear forcing storage */

  double **lfx;                 /* lamb vector storage */
  double **lfy;


  double **us;                  /* multistep storage */
  double **vs;
  double **ws;
  double **ps;

  double **ul;                  /* Lagrangian velocity for Op Int Spl. */
  double **vl;
  double **wl;

  double **mu, **mv, **mw, **mx, **my, **mz;

  Bndry    *Ubc,*Vbc,*Wbc;      /* Boundary  conditons               */
  Bndry    *Pbc;
#ifdef ALI
  Bndry    *dUdt, *dVdt;
#endif
  Bsystem  *Usys;               /* Velocity Helmholtz matrix system  */
  Bsystem  *Vsys;               /* Velocity Helmholtz matrix system  */
  Bsystem  *Wsys;               /* Velocity Helmholtz matrix system  */
  Bsystem  *Pressure_sys;       /* pressure Poisson   matrix system  */

  double   **ForceFuncs;        /* storage for variable forcing      */
  char     **ForceStrings;      /* string definition of forcing      */

  //  Metric     *kinvis;

  // ALE structures
#ifdef ALE
  double **ztri;
   Element_List *Ptri;
  Element_List *Ptri_f;
  Bndry *Ptri_bc;
  Bsystem *Ptri_sys;
  Corner *corn;

   double  *velocity;
   double *position;
  Element_List **MeshX;
  Element_List **MeshV;
  Element_List *MeshVf;
  Bndry        **MeshBCs;
  Bsystem      *Mesh_sys;

  Element_List *Psegs;
  Element_List *PsegsF;
  Bsystem      *Psegs_sys;

  Element_List *Msegs;
  Element_List *MsegsUF;
  Element_List *MsegsVF;
  Bsystem      *Msegs_sys;

  int          *update_list;
#endif

#ifdef BMOTION2D
  FILE     *bdd_file, *bda_file;       /* motion of body file              */
  FILE     *bgy_file;                  /* energy data                      */

  Motion2d   *motion2d;                /* Body motion info                 */
#endif

#ifdef MAP
  // Ce107
  FILE     *int_file;       /* interpolation file pointer        */
  Map      *mapx,  *mapy;   /* Mapping in x and y                */
  MStatStr *mstat;          /* Moving Statistics information     */
  Intepts  *int_list;       /* link list for interpolation points*/
#endif


#ifdef PBC_1D_LIN_ATREE
  PBC1DLINATREE  *IMPBC;    /* Impedance boundary condition      */
                            /* Added by Leopold Grinberg         */
#endif

#ifdef ACCELERATOR
   PREDICTOR *ACCELERATOR_UVWP; /* ACCELERATOR for itterative solver */
                            /* Added by Leopold Grinberg             */
#endif

#ifdef POD_ACCELERATOR
   POD_PREDICTOR *POD_UVWP; /* POD ACCELERATOR for itterative solver */
                            /* Added by Leopold Grinberg             */
#endif

} Domain;

/* function in drive.c  */
void MakeF   (Domain *omega, ACTION act, SLVTYPE slvtype);
void solve(Element_List *U, Element_List *Uf,Bndry *Ubc,Bsystem *Ubsys,SolveType Stype,int step);
int Navier_Stokes(Domain *Omega, double t0, double tN);

/* functions in prepost.c */
void      parse_args (int argc,  char **argv);
void      PostProcess(Domain *omega, int, double);
Domain   *PreProcess (int argc, char **argv);
void      set_vertex_links(Element_List *UL);
void LocalNumScheme  (Element_List *E, Bsystem *Bsys, Gmap *gmap);
Gmap *GlobalNumScheme(Element_List *E, Bndry *Ebc);
void free_gmap(Gmap *gmap);
void free_Global_info(Element_List *Mesh, Bndry *Meshbc,
                Gmap *gmap, int lnel);

Bsystem *gen_bsystem(Element_List *UL, Gmap *gmap);

/* functions in io.c */
void      ReadParams     (FILE *rea);
void      ReadPscals     (FILE *rea);
void      ReadLogics     (FILE *rea);
Element_List  *ReadMesh       (FILE *rea,char *);
void      ReadKinvis     (Domain *);
void      ReadICs        (FILE *, Domain *);
void      ReadDF         (FILE *fp, int nforces, ...);
void      summary        (void);
void      ReadSetLink    (FILE *fp, Element_List *U);
void      ReadSetLink    (FILE *fp, Element *U);
Bndry    *ReadBCs        (FILE *fp, Element *U);
Bndry *ReadMeshBCs (FILE *fp, Element_List *Mesh);
Bndry    *bsort          (Bndry *, int );
void      read_connect   (FILE *name, Element_List *);
void      ReadOrderFile  (char *name,Element_List *E);
void      ReadHisData    (FILE *fp, Domain *omega);
void      ReadSoln       (FILE* fp, Domain* omega);
void      ReadDFunc      (FILE *fp, Domain *Omega);
void      ReadWave       (FILE *fp, double **wave, Element_List *U);
void      ReadTimeDepend (FILE *fp, Domain *omega);

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
void bandwidthopt (Element *E, Bsystem *Bsys, char trip);
void MinOrdering   (int nsols,  Fctcon *ptcon, int *newmap);
void addfct(Fctcon *con, int *pts, int n);
void free_Fctcon   (Fctcon *con, int n);

/* functions in recurrSC.c */
void Recursive_SC_decom(Element *E, Bsystem *B);

/* functions in convective.c */
void VdgradV (Domain *omega);
void CdgradV (Domain *omega);

/* functions in DN.C */
void DN(Domain *omega);

/* functions in rotational.c */
void VxOmega (Domain *omega);

/* functions in divergenceVv.c */
void DivVv (Domain *omega);

/* functions in lambforce.C */
void LambForcing(Domain *omega);

/* functions in pressure.c */
Bndry *BuildPBCs (Element_List *P, Bndry *temp);
void   SetPBCs   (Domain *omega);
void Set_Global_Pressure(Element_List *Mesh, Bndry *Meshbcs);
void Replace_Numbering(Element_List *UL, Element_List *GUL);
void Replace_Local_Numbering(Element_List *UL, Element_List *GUL);
void set_delta(int Je);

/* function in stokes.c      */
void StokesBC (Domain *omega);

/* functions in analyser.c   */
void Analyser (Domain *omega, int step, double time);

/* functions in forces */
void  forces (Domain *omega, int step, double time);
void  Forces (Domain *omega, double time, double *F,int writoutput);
void  forces_split (Domain *omega, int step, double time);  //added by Leopold Grinberg


/* functions in sections */
int cnt_srf_elements(Domain *omega);
Element_List *setup_surflist(Domain *omega, Bndry *Ubc, char type);


// Functions in ALE
Bndry *BuildMeshBCs(Element_List *M, Bndry *Ubc);
void Update_Mesh(Domain *Omega, int Je, double dt);
//void   Update_Mesh (Domain *Omega);
void   setup_ALE   (Domain *Omega);
void   setup_ALE   (Domain *Omega, Element_List *, Bndry *);
void Update_Mesh_Velocity(Domain *Omega);
void Set_Mesh(Domain *Omega);
void set_ALE_ICs(Domain *omega);
void set_soliton_ICs(Element_List *U, Element_List *V);
void set_soliton_BCs(Bndry *Ubc, Bndry *Vbc, char ch);
void update_paddle(Element_List *, Bndry *);

// Functions in smoother
Element_List *setup_seglist(Bndry *Ubc, char type);
Bsystem      *setup_segbsystem(Element_List *seg_list);
void          fill_seglist(Element_List *seg_list, Bndry *Ubc);
void          fill_bcs(Element_List *seg_list, Bndry *Ubc);
void          update_seg_vertices(Element_List *, Bndry *);
void update_surface(Element_List *EL, Element_List *EL_F, Bsystem *Bsys, Bndry *BCs);
//void smooth_surface(Element_List *EL, Element_List *EL_F,
//        Bsystem *Bsys, Bndry *BCs);
void smooth_surface(Domain *, Element_List *segs, Bsystem *Bsys, Bndry *BCs);
void test_surface(Bndry *Ubc, char type);

// functions in magnetic
void Set_Global_Magnetic(Element_List *Mesh, Bndry *Meshbcs);
void ReadAppliedMag(FILE* fp, Domain* omega);

double cfl_checker     (Domain *omega, double dt);
double full_cfl_checker(Domain *omega, double dt, int *eid_max);

// functions in mpinektar.C
void init_comm(int*, char ***);
void exit_comm();
void exchange_sides(int Nfields, Element_List **Us);
void SendRecvRep(void *buf, int len, int proc);

/* functions in mlevel.C */
void Mlevel_SC_decom(Element_List *E, Bsystem *B);

/* functions in womersley.C */
void WomInit(Domain *Omega);
void SetWomSol (Domain *Omega, double time, int save);
void SaveWomSol(Domain *Omega);

void zbesj_(double *ZR, double *ZI, double *FNU, int *KODE, int *N,
         double *CYR, double *CYI, int *NZ, int *IERR);
void WomError    (Domain *omega, double time);
void SetWomField (Domain *omega, double *u, double *v, double *w, double time);

/* functions in wannier.C */
#ifdef HEIMINEZ
void Heim_define_ICs (Element_List  *V);
void Heim_reset_ICs  (Element_List **V);
void Heim_error      (Element_List **V);
#endif
/* functions in structsolve.C */
void CalcMeshVels(Domain *omega);


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

/* ce107 changes begin */
int       backup         (char *path1);
void      ReadIntData    (FILE *fp, Domain *omega);
void      ReadSoln       (FILE *fp, Domain *omega);
void      ReadMStatPoints(FILE* fp, Domain* omega);
void      averagefields  (Domain *omega, int nsteps, double time);
double    VolInt         (Element_List *U, double shift);
double    VolInt         (Element_List *U);

double            L2norm (Element_List *V);
void       average_u2_avg(Domain *omega, int step, double time);
void             init_avg(Domain *omega);

/* functions in interp */
void interp(Domain *omega);

/* functions in nektar.C */
void AssembleLocal(Element_List *U, Bsystem *B);

/* functions in dgalerkin.c */
void set_elmt_sides    (Element_List *E);
void set_sides_geofac  (Element_List *EL);
void Jtransbwd_Orth    (Element_List *EL, Element_List *ELf);
void EJtransbwd_Orth   (Element *U, double *in, double *out);
void InnerProduct_Orth (Element_List *EL, Element_List *ELf);
int  *Tri_nmap         (int l, int con);
int  *Quad_nmap        (int l, int con);

/* subcycle.C */
void SubCycle(Domain *Omega, int Torder);
void Upwind_edge_advection(Element_List *U, Element_List *V,
          Element_List *UF, Element_List *VF);
void Add_surface_contrib(Element_List *Us, Element_List *Uf);
void Fill_edges(Element_List *U, Element_List *Uf, Bndry *Ubc,double t);


#ifdef BMOTION2D
/* functions in bodymotion.c */
void set_motion (Domain *omega, char *bddfile, char *bdafile);
void Bm2daddfce (Domain *omega);
void ResetBc    (Domain *omega);
void IntegrateStruct(Domain *omega, double time, int step, double dt);
#endif

#ifdef ALE
Corner *Find_corners(Element_List *EL,Domain *omega);
void sort_gids(Element_List *tri_list, Bndry *tri_bc);
void make_diff_coeff(Bsystem *Bsys, Element_List *P, Corner *corn);
void SetHisData(Domain *omega);
void setup_2d(Domain *omega, Corner *corn, Bndry *Meshbc);
Element_List *setup_trilist(Bndry *Ubc, char type);
Bndry *setup_tribndry(Element_List *tri_list, Corner *corn);
Bsystem *setup_tribsystem(Element_List *tri_list, Bndry *tri_bc);
void      ReadPosition(Domain *omega);
Gmap *SurfaceNumScheme(Element_List *UL, Bndry *Ebc);
Bndry *bsort_zeta(Bndry *bndry_list, int nbcs);
#endif
// MSB: Added for PSE------------------------------------
// Needs to be included after Domain is defined        //
#include "stokes_solve_F.h"                              //
#endif
