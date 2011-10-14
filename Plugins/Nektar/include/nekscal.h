/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/include/nekscal.h,v $
 * $Revision: 1.2 $
 * $Date: 2006/05/13 09:15:23 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/

#ifdef PARALLEL
#include <mpi.h>
#endif

#include <hotel.h>
/* general include files */
#include <math.h>
#include <veclib.h>

#include "MatSolve.h"
using namespace MatSolve;

#include "DG_Helm.h"

#define max(a,b) ( (b) < (a) ? (a) : (b) )
#define min(a,b) ( (b) > (a) ? (a) : (b) )

#ifdef PARALLEL
#define exit(a) {MPI_Abort(MPI_COMM_WORLD, -1); exit(a-1);}
extern "C"
{
#include "gs.h"
}
#endif

typedef enum {
  Poisson,       /* poisson   equation         */
  Helmholtz,     /* Helmholtz equation         */
  Laplace,       /* Laplace   equation         */
  Wave,          /* wave      equation         */
  Project,       /* projection operator        */
  AdvDiff,       /* Advection Diffusion        */
  SteadyAdvDiff, /* Steady Advection Diffusion */
  DGHelmholtz,   /* DG Helmtholtz equaion      */
  Aposterr       /* error     estimator        */
}EQTYPE;

#define HP_MAX  128   /* Maximum number of history points */


typedef enum {                    /* ......... ACTIONFlags .......... */
  TransVert,                      /* Take history points from vertices */
  TransEdge                       /* Take history points from mid edge */
} ACTION;

typedef struct hpnt {             /* ......... HISTORY POINT ......... */
  int         id         ;        /* ID number of the element          */
  int         i, j, k    ;        /* Location in the mesh ... (i,j,k)  */
  char        flags               /* The fields to echo.               */
              [MAXFIELDS];        /*   (up to maxfields)               */
  ACTION      mode       ;        /* Physical or Fourier space         */
  struct hpnt *next      ;        /* Pointer to the next point         */
} HisPoint;

/* Solution Domain contains global information about solution domain */
typedef struct dom {
  char     *name;       /* name of run        */
  char     **soln;      /* solution           */

  FILE     *his_file;   /* history file pointer  */
  FILE     *fld_file;   /* field file pointer    */
  FILE     *dat_file;   /* field file pointer    */

  Bsystem  *Usys;

  Element_List  *U;
  Element_List  *V;
  Element_List  *Uf;
  Element_List  *Us;
  Bndry         *Ubc;
  double        **u, **us, **uf;

  MatSolve::NekMat  *NMat;
  MatSolve::DGMatrix *DGMat;

#ifdef ALE
  Element_List  *P,*V,*Z,*W,*X,*IX,*Y,*Xf,*Pf,*Vf, *Wf;
  Bsystem *Vsys, *Wsys, *Xsys, *Zsys, *Ysys, *Psys;
  Bndry *Pbc, *Vbc, *Wbc, *Xbc, *Ybc, *Zbc;
  Corner *corn;
  Element_List *MV;
  Element_List *MVf;
  Element_List *Ptri;
  Element_List *Ptri_f;
  Element_List *Zeta;
  Bndry *Ptri_bc;
  Bsystem *Ptri_sys;
  double **v;                   /* Field storage */
  double **w;                   /* Field storage */
  double **x;
  double **y;
  double **z;
  double **zeta;
  double **p;
  double **ztri;
  double **vf;                   /* Non-linear forcing storage */
  double **wf;                   /* Non-linear forcing storage */

  double **vs;
  double **ws;
  double **ps;

  double **dz;                    /* free-surface dz/dt storage */

  double **mu, **mv, **mw, **mx, **my, **mz;
  double *velocity;
  double *position;

#endif

  HisPoint *his_list;       /* link list for history points */
} Domain;

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

/* functions in nekscal.c */
void solve(Element_List *U, Element_List *Uf,
     Bndry *Ubc, Bsystem *Ubsys,SolveType Stype);
void zero_non_boundary(Element_List *E);

/* functions in prepost.c*/
void        parse_args (int, char **);
void        PostProcess(Domain *, int, double);
Domain     *PreProcess (int argc, char **argv);
Bsystem    *gen_bsystem(Element_List *UL, Gmap *gmap);
void Make_curve(Bndry *Pbc);
Gmap *SurfaceNumScheme(Element_List *UL, Bndry *Ebc);

/* functions in io.c */
void      ReadParams    (FILE *);
void      ReadPscals    (FILE *);
void      ReadLogics    (FILE *);
Element_List  *ReadMesh      (FILE *,char*);
void      ReadICs       (FILE *, Domain *omega , Element_List *Mesh);
void      summary       (void);
void      ReadSetLink   (FILE *fp, Element_List *U);
Bndry    *ReadBCs       (FILE *fp, Element *U);
Bndry    *ReadMeshBCs (FILE *fp, Element_List *Mesh);
Bndry    *bsort         (Bndry *, int );
void      read_connect  (char *name, Element *E);
void      ReadOrderFile (char *name, Element_List *E);
void      ReadDF(FILE *fp, int nforces, ...);
void      ReadSoln(FILE* fp, Domain* omega);
void      ReadPosition(Domain *omega);
void      ReadHisData    (FILE *fp, Domain *omega);


/* functions in convective.c */
void CdgradV(Domain *omega);

/* functions in spectrum.c */
void   Spectrum(Domain *omega);
double Weakspec(Domain *omega);

/* functions in mlevel.C */
void Mlevel_SC_decom(Element_List *E, Bsystem *B);

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

/* function in Aposterr.c */
void aposterr(Domain *omega);

/* function in nekmesh.C */
void set_soliton_BCs(Bndry *Xbc, Bndry *Ybc);
void set_soliton_ICs(Element_List *U, Element_List *V,
         Element_List *X, Element_List *Y);

void ReadLambda(FILE* fp, Element_List *U, Bsystem *Ubsys);

/* functions in mlevel.c */
void Mlevel_SC_decom(Element_List *E, Bsystem *B);

Bndry *BuildBCs(Element_List *M, Bndry *Ubc);
void set_free_ICs(Domain *omega);
void Zeta(Domain *Omega, int step);
void Phi(Domain *Omega);
void Moving_box(Domain *Omega);
void Update_Mesh(Element_List *Z, Element_List *P);
void setup_2d(Domain *omega, Corner *corn, Bndry *Meshbc);
Element_List *setup_trilist(Bndry *Ubc, char type);
Bndry *setup_tribndry(Element_List *tri_list, Corner *corn);
Bsystem *setup_tribsystem(Element_List *tri_list, Bndry *tri_bc);
void set_vertex_links(Element_List *UL);
void set_edge_links(Element_List *UL);
void set_edge_links(Element_List *UL);
void Initial_condition(Element_List *P, Bndry *Pbc);
void eval_geoms(Element_List *U, Element_List *Uf);
void Zbc_to_Ztri_f(Bndry *Zbc, Element_List *U);
void Ztri_to_Zbc(Bndry *Zbc, Element_List *U);
Corner *Find_corners(Element_List *EL,Domain *omega);
void SetHisData(Domain *omega);
void Add_non_linear_zeta(Domain *Omega, int step);
void Add_non_linear_phi(Domain *Omega);
Bndry *bsort_zeta(Bndry *bndry_list, int nbcs);
void Surface_analyser(Domain *omega, int step, int time);
void zeroBC(Bndry *Bc, char type);
void make_wave(Bndry *Bc, char type);
void make_wave_left(Bndry *Bc, Element *);
void make_wave_right(Bndry *Bc);
void sort_gids(Element_List *tri_list, Bndry *tri_bc);
Bndry *BuildMeshBCs(Element_List *M, Bndry *Ubc);
void Update_Surface_Mesh(Bndry *Xbc, Element_List *P);
void make_diff_coeff(Bsystem *Bsys, Element_List *P, Corner *corn);

/*functions in wave.C */
Bndry *WaveBCs(Element_List *M, Bndry *Ubc);
Bndry *WaveMeshBCs(Element_List *M, Bndry *Ubc, char type);
void setUbc(Bndry *Ubc, Element_List *P);

/*functions in mesh.C */
void search_surface(Bndry *Pbc, Element_List *P);
void move_interior_points(Bndry *Zbc, Element_List *Z);
void update_boundary_points(Bndry *Zbc);

//functions in nekscal.C
void zero_non_boundary(Element_List *E);
void Smooth(Domain *Omega);
double Displacement(Bndry *Bc, int S_nbcs);
int countSbcs(Bndry *Ybc);
void BC_moving_box(Bndry *Pbc, Corner *corn);
void set2d_bc(Bndry *Bc, Element_List *EF);

/* functions in analyser */
void Analyser (Domain *omega, int step, double time);
void Write2dfield(FILE *fp, Domain *omega, int step, int time);

/* functions in DG_helm */
void dg_Helm(Domain *omega);

#ifdef INVJAC
#define IJAC 1
#else
#define IJAC 0
#endif

/* functions in dgalerkin.c */
void  set_elmt_sides   (Element_List *E);
void set_sides_geofac  (Element_List *EL);
void Jtransbwd_Orth    (Element_List *EL, Element_List *ELf);
void Jtransbwd_Orth_hj (Element_List *EL, Element_List *ELf);
void EJtransbwd_Orth   (Element *U, double *in, double *out);
void InnerProduct_Orth (Element_List *EL, Element_List *ELf);
void InnerProduct_Orth_hj (Element_List *EL, Element_List *ELf);
int  *Tri_nmap         (int l, int con);
int  *Quad_nmap        (int l, int con);
void Reset_Jac(Element_List *U);
