/*---------------------------------------------------------------------------*
 *      H.igh  O.rder  T.riangular  E.lement  L.ibrary   header              *
 *---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/include/hotel.h,v $
 * $Revision: 1.3 $
 * $Date: 2006/08/10 17:46:10 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/

#ifndef  HOTEL_H
#define  HOTEL_H

/* general include files */

/* default integers */
#define Max_Nverts 8
#define Max_Nedges 12
#define Max_Nfaces 6

/* General parameter limits  */
#define _MAX_FIELDS       24   /* Maximum fields in a solution file        */
#define _MAX_ORDER         3   /* Maximum time integration order           */

#define Tri_DIM  2
#define NTri_verts  3
#define NTri_edges  3
#define NTri_faces  1

#define Quad_DIM  2
#define NQuad_verts  4
#define NQuad_edges  4
#define NQuad_faces  1

#define Tet_DIM  3
#define NTet_verts  4
#define NTet_edges  6
#define NTet_faces  4

#define Pyr_DIM  3
#define NPyr_verts  5
#define NPyr_edges  8
#define NPyr_faces  5

#define Prism_DIM  3
#define NPrism_verts  6
#define NPrism_edges  9
#define NPrism_faces  5

#define Hex_DIM  3
#define NHex_verts  8
#define NHex_edges  12
#define NHex_faces  6

// Particle code tolerances
#define P_TOL 1e-12

/* conflicts */
#define Coord Coord_Nek

#ifdef PARALLEL
#include <mpi.h>
#endif


#include <stdio.h>
#include <stdlib.h>

#define ERR fprintf(stderr,"Accessing virtual function\n");


#include "hstruct.h"
#include "nekstruct.h"
#include "work.h"
#include "Tri.h"
#include "Quad.h"
#include "Tet.h"
#include "Pyr.h"
#include "Prism.h"
#include "Hex.h"

#ifdef DEBUG
#include "dbutils.h"
#include "memcheck.h"
extern Telapse timetest[_MAX_TIMECALL];
extern FILE *debug_out;
#endif

/* parallel information */
extern  ParaInfo pllinfo[51];
extern  ParaInfo pBCinfo[51];
#define ROOTONLY    if (mynode() == 0)
#define DO_PARALLEL if (pllinfo[get_active_handle()].nprocs > 1)

/* this is just a function which can be called so that the DO_PARALLEL
   flag works at beginning of prepost.c */
#define pllinfo_init()  pllinfo[get_active_handle()].nprocs = numnodes(); \
                        pllinfo[get_active_handle()].procid = mynode();


int get_active_handle();

#define MSGTAG    599

void init_comm (int *argc, char **argv[]);
void gdsum     (double *x, int n, double *work);
void gisum     (int    *x, int n, int    *work);
void gdmax     (double *x, int n, double *work);
void gimax     (int    *x, int n, int    *work);
void exit_comm (void);
double dclock_mpi(void);


void mpi_dsend (double *buf, int len, int send_to, int tag);
void mpi_isend (int *buf, int len, int send_to, int tag);
void mpi_drecv (double *buf, int len, int receive_from, int tag);
void mpi_irecv (int *buf, int len, int receive_from, int tag);

void BCreduce (double *bc, Bsystem *Ubsys);
int  numnodes ();
int  mynode   ();
void gsync    ();
void parallel_gather     (double *w, Bsystem *B);
void GatherBlockMatrices (Element *U,Bsystem *B);
void Set_Comm_GatherBlockMatrices (Element *U, Bsystem *B);

void create_comm_BC(int Nout, int *face_counter);
int  create_comm_BC_inlet_outlet(int Ninl, int *Nfaces_per_inlet, int *Nnodes_inlet,
                                 int Nout, int *Nfaces_per_outlet,int *Nnodes_outlet);

/* time test */
#include <time.h>
extern double bclock, eclock, savinit, setrhs, solbnd, addinit, solint;
extern double bclock1, eclock1, helmtim, resttim;

/* externals */
extern int QGmax; /* Maximum quadrature points in element for global domain */
extern int LGmax; /* Maximum expansion order for global domain              */

extern int LZero; /* Flag to determine whether to use Legendre quad zeros   */

/* Macros */

#define max(a,b) ( (b) < (a) ? (a) : (b) )
#define min(a,b) ( (b) > (a) ? (a) : (b) )
#define clamp(t,a,b)  (max (min(t,b), a) )

#define set_state(E,c) {Element *El; for(El=E;El;El=El->next)  El->state = c;}
#define error_msg(a) {fprintf(stderr,#a"\n"); exit(-1);}

#define RxV(u,Rv,Re,Nv,Ne,Nf,v) \
   if(Ne){dgemv('T',Ne+Nf,Nv,1.0,*Rv,Ne+Nf,u+Nv,1,1.0,v,1);\
    if(Nf)dgemv('T',Nf,Ne,1.0,*Re,Nf,u+Nv+Ne,1,1.0,v+Nv,1);}

#define RTxV(u,Rv,Re,Nv,Ne,Nf,v) \
  if(Ne){ if(Nf) dgemv('N',Nf,Ne,1.0,*Re,Nf,u+Nv,1,1.0,v+Nv+Ne,1);\
  dgemv('N',Ne+Nf,Nv,1.0,*Rv,Ne+Nf,u,1,1.0,v+Nv,1);}

#define RvxV(u,Rvi,Nv,Ne,Nf,v) \
  if(Ne) dgemv('T',Ne+Nf,Nv,1.0,*Rvi,Ne+Nf,u+Nv,1,1.0,v,1);

#define RvTxV(u,Rvi,Nv,Ne,Nf,v) \
  if(Ne) dgemv('N',Ne+Nf,Nv,1.0,*Rvi,Ne+Nf,u,1,1.0,v+Nv,1);

#define IRxV(u,Rvi,Re,Nv,Ne,Nf,v) \
  if(Ne){dgemv('T',Ne+Nf,Nv,1.0,*Rvi,Ne+Nf,u+Nv,1,1.0,v,1);\
   if(Nf) dgemv('T',Nf,Ne,-1.0,*Re,Nf,u+Nv+Ne,1,1.0,v+Nv,1);}

#define IRTxV(u,Rvi,Re,Nv,Ne,Nf,v) \
  if(Ne){ if(Nf) dgemv('N',Nf,Ne,-1.0,*Re,Nf,u+Nv,1,1.0,v+Nv+Ne,1);\
     dgemv('N',Ne+Nf,Nv,1.0,*Rvi,Ne+Nf,u,1,1.0,v+Nv,1);}

enum SolveType{
  Mass,            /* Invert mass matrix      */
  Helm             /* Invert Helmholtz matrix */
  };

enum InterpDir{
  a2a,             /* interpolate from 'a' gll points to 'a' gll points */
  a2b,             /* interpolate from 'a' gll points to 'b' grj points */
  a2g,             /* interpolate from 'a' gll points to 'g' gl  points */

  b2a,             /* interpolate from 'b' grj points to 'a' gll points */
  b2b,             /* interpolate from 'b' grj points to 'b' gll points */
  b2c,             /* interpolate from 'b' grj points to 'c' grj points */
  b2g,             /* interpolate from 'b' gll points to 'g' gl  points */
  b2h,             /* interpolate from 'b' grj points to 'h' gl  points */

  c2b,             /* interpolate from 'c' grj points to 'b' grj points */
  c2c,             /* interpolate from 'c' grj points to 'c' grj points */
  c2g,             /* interpolate from 'c' gll points to 'g' gl  points */
  c2h,             /* interpolate from 'c' grj points to 'h' gl  points */

  g2a,             /* interpolate from 'g' gl  points to 'a' gll   points */
  g2b,             /* interpolate from 'g' gl  points to 'b' grj   points */
  g2c,             /* interpolate from 'g' gl  points to 'c' grj   points */
  g2g,             /* interpolate from 'g' gl  points to 'g' gl  points */

  h2b,             /* interpolate from 'h' gl  points to 'b' grj  points */
  h2c             /* interpolate from 'h' gl  points to 'c' grj  points */
};

/* functions in Basis.C */
void  set_LZero (int val);

void   getzw (int q, double **z, double **w, char dir);
void   getim (int q1, int q2, double ***im, InterpDir dir);

void   normalise_mode(int q, double *, char dir);

void get_moda_G  (int size, double ***mat);
void get_moda_GL (int size, double ***mat);
void get_moda_GR (int size, double ***mat);
void get_modb_GR (int size, double ****mat);
void get_modb_G (int size, double ****mat);
void get_modc_GR (int size, double ****mat);
void get_modc_G (int size, double ****mat);

void init_ortho_basis(void);

void init_mod_basis(void);
void get_modA (int size, double ***mat);
void get_modB (int size, double ****mat);

void   Tri_reset_basis(void);
void   Quad_reset_basis(void);
void   Tet_reset_basis(void);
void   Pyr_reset_basis(void);
void   Prism_reset_basis(void);
void   Hex_reset_basis(void);

void reset_bases();

/* functions in Solve.C */
void Solve (Element_List *U, Element_List *Uf, Bndry *Ubc, Bsystem *Ubsys,
      SolveType Stype);
void solve_interior (Element_List *, Bsystem *);
void SetBCs (Element_List *U, Bndry *Ubc, Bsystem *Ubsys);
void PackMatrix (double **a, int n, double *b, int bwidth);
void FacMatrix  (double * a, int n, int bwidth);
void GathrBndry(Element_List *U, double *u, Bsystem *B);
void ScatrBndry(double *u, Element_List *U, Bsystem *B);
void SignChange(Element_List *E, Bsystem *B);
void solve_boundary(Element_List *, Element_List *,
        double *, double *,Bsystem *);
void setup_signchange(Element_List *U, Bsystem *B);

void A(Element_List *U, Element_List *Uf, Bsystem *B, double *p, double *w);
void A_fast(Element_List *U, Element_List *Uf, Bsystem *B, double *p,
      double *w);
void gddot(double *alpha, double *r, double *s, double *mult,
       int nsolve);
void parallel_gather(double *w, Bsystem *B);

void FacMatrixSym(double *a, int *pivot, int n, int bwidth);
double One_elmt_PCG(int nsolve, double *A, double *Minv, double *p);


/* functions in SolveR.C */
void Recur_setrhs    (Rsolver *R, double *rhs);
//void Recur_backslv   (Rsolver *R, double *rhs);
void Recur_backslv   (Rsolver *R, double *rhs, char trip);
void Recur_Bsolve_CG (Bsystem *B, double *p, char type);

/* functions in H_Solve_cg.C */
void Solve_CG (Element_List *U, Element_List *F,
         Bndry *Ubc, Bsystem *B, SolveType Stype);

/* functions in Matrix.C */
int bandwidth(Element *E, Bsystem *Bsys);
LocMat *Locmat_mem  (int Lmat);
void    Locmat_free (LocMat *m, int Lmax);
void    GenMat(Element_List *U, Bndry *Ubc, Bsystem *Ubsys, Metric *lambda,
      SolveType Stype);
void    GenMatAgain(Element_List *U, Bndry *Ubc, Bsystem *Ubsys,
        Metric *lambda, SolveType Stype);
void GenSelectMat(Element_List *U, Bndry *Ubc, Bsystem *Ubsys,
      int *list, Metric *lambda,SolveType Stype);

void  Add_robin_matrix(Element *E, LocMat *helm, Bndry *rbc);
void get_mmat2d  (double **mat, int L, Element *E);
void MemPrecon(Element *U, Bsystem *B);
void UpdatePrecon(Element_List *U, Bsystem *Ubsys);
void Bsystem_mem_free(Bsystem *Ubsys, Element_List *U);
void Setup_Preconditioner(Element_List *U, Bndry *Ubc, Bsystem *Ubsys, Metric *lambda, SolveType Stype);

void InvtPrecon(Bsystem *B);


/* functions in MatrixR.C */
void MemRecur        (Bsystem *B, int level, char type );
void Project_Recur   (int eid, int n, double *a, int *map,
          int lev, Bsystem *B);
void Condense_Recur  (Bsystem *B, int lev, char type);
void Rinvert_a       (Bsystem *B, char type);
void Rpack_a         (Bsystem *B, char type);

void PackMatrixV     (double *a, int n, double *b, int bwidth, char trip);

#if 0

/* functions in MatrixR.C */
void MemRecur        (Bsystem *B, int level);
void Project_Recur   (int eid, int n, double *a, int *map,
          int lev, Bsystem *B);
void Condense_Recur  (Bsystem *B, int lev);
void Rinvert_a       (Bsystem *B);
void Rpack_a         (Bsystem *B);
#endif

/* functions in Nekvec.C */
double  ***dtarray(int nwl, int nwh, int nrl, int nrh, int ncl, int nch);
void       free_dtarray(double ***m3, int nwl, int nrl, int ncl);
double   **dtmatrix  (int l);
Mode       *mvector  (int nl, int nu);
Mode       *mvecset  (int nl, int nu, int qa, int qb, int qc);
void       free_mvec (Mode *m);
void       mvmul     (int qa, int qb, int qc, Mode *x, Mode *y, Mode *z);
double     mdot      (int qa, int qb, int qc, Mode *x, Mode *y);

/* functions in ElmtUtils.C */
void     set_QGmax     (Element *E);
void     set_LGmax     (Element *E);
Element *gen_elmt      (int nel, int type, int l, int qa, int qb, int qc);
void     mem_elmt      (Element *E, int *list, char trip);
void     MemQuadpts    (Element *E);
int      countelements (Element *U);
void     DirBCs        (Element_List *U, Bndry *Ubc, Bsystem *Ubsys, SolveType );
void DirBCs_Stokes(Element_List **V, Bndry **Vbc, Bsystem **Vbsys);
Element_List *LocalMesh(Element_List *EL, char *name);
Element_List *LocalMesh(Element_List *EL, Bndry *Bc,char *name);
void default_partitioner(Element_List *EL, int *partition);
int *test_partition_connectivity(Element_List *EL, int *partition, int *Npart);
void set_nfacet_list (Element_List *U);
void get_facet       (int nel,int *nfacet,int *emap, int shuffled);
int  Interp_symmpts(Element *E, int n, double *uj, double *ui,char storage);

/* functions in Integrate.C */
void getalpha  (double *a);
void getbeta   (double *b);
double getgamma   (int);
double getgamma   ();

void set_order (int Je);
void set_order_CNAB (int Je);
void set_order_CNAM (int Je);
void integrate_CNAB (int Je, double dt, Element_List *U, Element_List *Uf[]);
void Integrate_CNAB (int Je, double dt, Element_List *U, Element_List *Uf,
        double **u, double **uf);
void integrate_CNAB (int Je, double dt, Element_List *U, Element_List *Uf,
        double **u, double **uf);
void Integrate_AB   (int Je, double dt, Element_List *U, Element_List *Uf,
        double **u, double **uf);
void Integrate_CNAM (int Je, double dt, Element_List *U, Element_List *Uf,
        double **u, double **uf);
void Integrate_AM   (int Je, double dt, Element_List *U, Element_List *Uf,
        double **u, double **uf);
void Integrate_SS   (int Je, double dt, Element_List *U, Element_List *Uf,
         double **u,      double **uf);

/* functions in Coord.C */
void coord         (Element *E, Coord *X);
void faceMode      (Element *E, int face, Mode *v, double *f);
void straight_face (Element *E, Coord *X, int face, int trip);
void straight_edge (Element *E, Coord *X, int edge);
void GetFaceCoord  (Element *E,int face, Coord *X);

/*  Functions from manager.y */
extern "C"
{
void   manager_init  (void);
void   show_symbols  (void);
void   show_params   (void);
void   show_options  (void);

void   vector_def    (char *vlist, char *function);
void   vector_set    (int   vsize, ... /* v1, v2, ..., f(vn) */ );
double scalar        (char *function);
double dparam_set    (char *name, double value);
double dparam        (char *name);
int    option_set    (char *name, int value);
int    option        (char *name);
int    iparam_set    (char *name, int value);
int    iparam        (char *name);
}

/* functions in Fieldfiles.C */
void  Writefield  (FILE *fp, char *name, int step, double t,
       int nfields, Element_List *U[]);
int   writeField  (FILE *fp, Field *f, Element *E);
void  writeHeader (FILE *fp, Field *f);
int   writeData   (FILE *fp, Field *f);
int   data_len    (int nel,int *size, int *nfacet, int dim);
void  load_facets (Element *E, int *nfacet);
int   readField   (FILE *fp, Field *f);
int   readHeader  (FILE *fp, Field *f, int *shuffled);
int   readFieldP  (FILE *fp, Field *f, Element_List *E);
void copysurffield(Field *f, int pos, Bndry *Ubc, Element_List *U);
void  copyfield   (Field *f, int pos, Element *U);
void  copyelfield (Field fld, int nfields, int eid, Element_List **U);
void  copyelfield (Field fld, int nfields, int eid, double **data,
        Element_List **U);
void  freeField  (Field *f);
int   readHeaderP(FILE* fp, Field *f, Element_List *Mesh);
Field *readFieldFiles(int nfiles, char *name, Element_List *Mesh);
void Writefld     (FILE **fp, char *name, int step, double t,
       int nfields, Element_List *UL[]);
void Writefld     (FILE **fp, char *name, int step, double t,
       int nfields, Element_List *UL, double **data, char *type);

/* functions in EigenMatrix.C */
void   EigenMatrix(double **a, int n, int bwidth);
double FullEigenMatrix(double **a, int n, int lda, int  trip );

/* functions in Utilities.C */
void Interp_point  (Element_List **U, int nfields, Coord *X, double *ui);
double interp_abc(double *vec, int qa, char type, double a);
void nomem_set_curved(Element_List *EL, Element *E);
void calc_edge_centers(Element *E, Coord *X);


int find_elmt(Element_List *EL, Coord *X);
int GetABC (Element *U, Coord *X, Coord *A);
void Interp_point_3d(Element_List **U, int nfields, Coord *X, double *ui);
int Query_element_id();

void cross_products(Coord *A, Coord *B, Coord *C, int nq);
void normalise (Coord *A, int nq);
void Interp2d  (double *ima, double *imb, double *from, int qa, int qb,
    double *to, int nqa, int nqb);

// Fuction in Transform.C
void JbwdTri (int qa, int qb, int lmax, int *lm, double *vj, double *v);
void JbwdQuad(int qa, int qb, int lmax, int *lm, double *vj, double *v);


// Functions in Tri
void gen_sin(Element *E, Curve *cur, double *x, double *y);
void gen_ellipse(Element *E, Curve *cur, double *x, double *y);


void Tri_work();
void Quad_work();
void Tet_work();
void Pyr_work();
void Prism_work();
void Hex_work();

// Functions in H_Curvi.C

void genCylinder(Curve *curve, double *x, double *y, double *z, int q);
void genCone    (Curve *curve, double *x, double *y, double *z, int q);
void genSphere  (Curve *curve, double *x, double *y, double *z, int q);
void genSheet   (Curve *curve, double *x, double *y, double *z, int q);
void genSpiral  (Curve *curve, double *x, double *y, double *z, int q);
void genTaurus  (Curve *curve, double *x, double *y, double *z, int q);
void genNaca3d  (Curve *curve, double *x, double *y, double *z, int q);
void  Tri_Face_JacProj(Bndry *B);
void Quad_Face_JacProj(Bndry *B);

/* parametric surface */
//void genFree    (Curve *curve, double *x, double *y, double *z, int qa, int qb);
void genFree(Curve *curve, double *x, double *y, double *z, char dir1, char dir2, int qa, int qb);
void genRecon   (Curve *curve, double *va, double *vb, double *vc, double *x, double *y, double *z, int q);
void gen_ellipse(Element *E, double *x, double *y);
void gen_sin(Element *E, double *x, double *y);
double Tri_naca(double L, double x, double t);
void Tri_genNaca(Element *E, Curve *curve, double *x, double *y);
void Quad_genNaca(Element *E, Curve *curve, double *x, double *y);


// Functions in Felisa.C
void genFelFile(Element *E, double *x, double *y, double *z, Curve *curve);

void gen_face_normals(Element *E);
void Felisa_fillElmt(Element *E, int fac, int felfac);
void Load_Felisa_Surface(char *name);
void Free_Felisa_data   (void);


//Functions in HOSurf.C
void Load_HO_Surface (char *name);
void genSurfFile     (Element *E, double *x, double *y,
          double *z, Curve *curve);

//functions in Mrhs.C
Multi_RHS *Get_Multi_RHS (Bsystem *Ubsys, int Nrhs, int nsolve, char type);
void Mrhs_rhs            (Element_List *U, Element_List *Uf, Bsystem *B,
        Multi_RHS *mrhs, double *rhs);
void Update_Mrhs         (Element_List *U, Element_List *Uf, Bsystem *B,
        Multi_RHS *mrhs, double *sol);
void Recur_Update_Mrhs   (Bsystem *B, Multi_RHS *mrhs, double *sol);


// function in Group_ops.C
void form_groups(Element_List *EL);
void Grad(Element_List *EL, Element_List *dEdx, Element_List *dEdy, Element_List *dEdz);


/* functions in H_Solve_Stokes.C */
void Precon_Stokes  (Element_List *V, Bsystem *B, double *r, double *z);
void A_Stokes       (Element_List **V, Bsystem **B, double *p, double *w,
         double **wk);
void Solve_Stokes(Element_List **V, Element_List **Vf, Bndry **Vbc,
      Bsystem **Vbsys);
void GathrBndry_Stokes(Element_List **V,double *u, Bsystem **B);


/* functions in Matrix_Stokes.C */
void GenMat_Stokes(Element_List *U, Element_List *P,
       Bsystem *Ubsys, Bsystem *Pbsys, Metric *lambda);
void FacFullMatrix (double *a, int n, int *ipiv, int bwidth);
void PackFullMatrix(double **a, int n, double *b, int bwidth);

// Functions in H_Matrix.C
void  Tri_HelmMat(Element *T, LocMat *helm, double lambda);






/* local function definitions */
#define Interp(mat,from,nf,to,nt) (mxva(mat,nf,1,from,1,to,1,nt,nf))

void tecmatrix(FILE *fp, double **data, int asize, int bsize);


void Tet_faceMode(Element *E, int face, Mode *v, double *f);
double Tri_mass_mprod(Element *E, Mode *m, double *wvec);

double Quad_mass_mprod(Element *E, Mode *m, double *wvec);

void Tri_mvmul2d(int qa, int qb, int qc, Mode *x, Mode *y, Mode *z);

Basis *Tri_addbase(int L, int qa, int qb, int qc);
void Tri_reset_basis(Basis *B);

Basis *Tri_mem_base(int L, int qa, int qb, int qc);
Basis *Quad_mem_base(int L, int qa, int qb, int qc);
Basis *Tet_mem_base(int L, int qa, int qb, int qc);
Basis *Pyr_mem_base(int L, int qa, int qb, int qc);
Basis *Prism_mem_base(int L, int qa, int qb, int qc);
Basis *Hex_mem_base(int L, int qa, int qb, int qc);

void Tri_mem_modes(Basis *b);
void Quad_mem_modes(Basis *b);
void Tet_mem_modes(Basis *b);
void Pyr_mem_modes(Basis *b);
void Prism_mem_modes(Basis *b);
void Hex_mem_modes(Basis *b);


 void  Tri_set_vertices(Mode *v, int q, char dir);
 void  Quad_set_vertices(Basis *b, int q, char dir);
 void  Tet_set_vertices(Mode *v, int q, char dir);
 void  Pyr_set_vertices(Mode *v, int q, char dir);
 void  Prism_set_vertices(Mode *v, int q, char dir);
 void  Hex_set_vertices(Mode *v, int q, char dir);

 void Tri_set_edges(Basis *b, int q, char dir);
 void Quad_set_edges(Basis *b, int q, char dir);
 void Tet_set_edges(Basis *b, int q, char dir);
 void Pyr_set_edges(Basis *b, int q, char dir);
 void Prism_set_edges(Basis *b, int q, char dir);
 void Hex_set_edges(Basis *b, int q, char dir);

 void Tri_set_faces(Basis *b, int q, char dir);
 void Quad_set_faces(Basis *, int , char );
 void Tet_set_faces(Basis *b, int q, char dir);
 void Pyr_set_faces(Basis *b, int q, char dir);
 void Prism_set_faces(Basis *b, int q, char dir);
 void Hex_set_faces(Basis *b, int q, char dir);

void Tet_faceMode(Element *E, int face, Mode *v, double *f);
void Pyr_faceMode(Element *E, int face, Mode *v, double *f);
void Prism_faceMode(Element *E, int face, Mode *v, double *f);
void Hex_faceMode(Element *E, int face, Mode *v, double *f);


// Functions in Interp_point.C
#define  find_coords_2d(E,xo,yo,a,b) Find_coords_2d(E,xo,yo,a,b,'n')
#define  find_coords_3d(E,xo,yo,zo,a,b,c) Find_coords_3d(E,xo,yo,zo,a,b,c,'n')


void Tri_get_point_shape(Element *E, double a, double b,
       double *hr, double *hs);

double eval_field_at_pt_2d (int qa, int qb, double *field,
          double *hr, double *hs);
double eval_field_at_pt_3d (int qa, int qb, int qc, double *field,
          double *hr, double *hs, double *ht);
int    point_in_box_2d     (double x, double y, int boxid);
int    point_in_box_3d     (double x, double y, double z, int boxid);
int    find_elmt_coords    (Element_List *U, Coord X,int *eid,Coord *A, char trip);
int    Find_coords_2d      (Element *E, double xo, double yo,
          double *a, double *b, char trip);
int    Find_coords_3d      (Element *E, double xo, double yo,  double zo,
          double *a, double *b, double *c, char trip);
int    Find_elmt_coords_2d (Element_List *U, double xo, double yo,
          int *eid, double *a, double *b);
int    find_elmt_coords_2d (Element_List *U, double xo, double yo,
          int *eid, double *a, double *b);
int    Find_elmt_coords_3d (Element_List *U, double xo, double yo, double zo,
          int *eid, double *a, double *b, double *c);
int    find_elmt_coords_3d (Element_List *U, double xo, double yo, double zo,
          int *eid, double *a, double *b, double *c);
void   Prt_find_local_coords   (Element_List *U, Coord X,
                            int *Eid, Coord *A, char status);
void   find_local_coords   (Element_List *U, Coord *X, int Npts,
                            int *Eids, Coord *A);
void   Find_local_coords   (Element_List *U, Coord *X, int Npts,
          int **Eids, Coord **A);
void Interpolate_field_values (Element_List **U, int Nfields, int Npts,
             int *Eids, Coord *A, double ***data);
void get_point_shape_3d    (Element *E, double a, double b, double c,
          double *hr, double *hs, double *ht);
void get_point_shape_2d    (Element *E, double a, double b,
          double *hr, double *hs);
void reset_starteid        (int eid);

void get_point_shape(Element *E, Coord A, double **h);



//Functions in ScaLapackNektar.C
void InitBLAC(Bsystem *B);
void  GatherScatterIvert(Bsystem *Bsys);
void matrix_2DcyclicToNormal(int *BLACS_PARAMS, double **A_2D, int *first_row_col);
int parallel_dgemv(int *BLACS_PARAMS, double **M_local,double *V, double *ANS, int first_row, int first_col, int *disps, int *rcvcnt);
void parallel_dgemv_params(int *BLACS_PARAMS, int  *displs, int *rcvcnt);
void set_mapping_parallel_dgemv(Bsystem *Bsys);
void scatter_vector(Bsystem *Bsys, double *v_block, double *v_local);
void gather_vector(Bsystem *Bsys, double *v_global, double *work);

void blacs_gridinit_nektar(int *BLACS_PARAMS, int *DESCA, int *DESCB);
void blacs_pdgetrf_nektar (int *BLACS_PARAMS, int *DESCA, int *ipvt, double **inva_LOC);
void blacs_pdgetri_nektar (int *BLACS_PARAMS, int *DESCA, int *ipvt, double **inva_LOC);
void blacs_pdgetrs_nektar (int *BLACS_PARAMS, int *DESCA, int *DESCB, double **inva_LOC, int *ipvt, double *RHS);
void pdgemv_nektar(int *BLACS_PARAMS, int *DESCA, int *DESCB, double **A, int *ipvt, double *RHS);
void blacs_dgather_rhs_nektar (int *BLACS_PARAMS, double *A);
void blacs_dscather_rhs_nektar(int *BLACS_PARAMS, double *A);

void scatter_topology_nektar(int *BLACS_PARAMS, double *ztmp, double *work);
void gather_topology_nektar(int *BLACS_PARAMS, double *V, double *work);
int  get_block_size(int length, int Nproc);
void get_proc_grid(int Nproc, int *nrow, int *ncol);
void update_inva_LOC( SMatrix *SM ,int *BLACS_PARAMS, double **inva_LOC, char storage_type);
void get_gather_map(int *BLACS_PARAMS, char dir, int *map);

#endif /* end of hotel.h declarations */
