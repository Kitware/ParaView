/*---------------------------------------------------------------------------*
 *                        RCS Information                                    *
 *                                                                           *
 * $Source: /homedir/cvs/Nektar/include/nekstruct.h,v $
 * $Revision: 1.8 $
 * $Date: 2006/08/10 17:46:10 $
 * $Author: ssherw $
 * $State: Exp $
 *---------------------------------------------------------------------------*/
/*
 * This file contains all structures used by the Nektar source code
 */

#ifndef NEKSTRUCT_H
#define NEKSTRUCT_H


#define MAXFIELDS 15

class Vert;
class Edge;
class Face;
class Element;
class Bsystem;
class Dorp;
class Metric;
class Coord;
class Geom;
class C_Arc;
class C_Naca4;
class C_Cylinder;
class C_Cone;
class C_Sheet;
class C_Spiral;
class C_Taurus;
class C_Ellipse;
class C_Free;

//class CurveInfo;
union CurveInfo;
class Curve;
class Cmodes;
class Element;
class Bndry;
class MatSys;
class MatPre;

class P_Diag;
class P_Block;
union PreInfo;
union Precond;

class Recur;
class Rsolver;
class Bsystem;
class Field;
class Element_List;
class Fourier_List;
class Membrane;

class Corner;

enum Nek_Facet_Type{
  Nek_Vert,
  Nek_Edge,
  Nek_Face,
  Nek_Tri,
  Nek_Quad,
  Nek_Nodal_Quad,
  Nek_Nodal_Tri,
  Nek_Tet,
  Nek_Pyr,
  Nek_Prism,
  Nek_Hex,
  Nek_Element,
  Nek_Seg,
  Nek_Quad_red
};

// J == Jacobii coefficients
// Q == quadrature values
// F == Fourier coefficients
// P == Physical coefficients

enum Nek_State{
  JF,
  JP,
  QF,
  QP,
  J,
  Q
  };

  // Ce107

enum Nek_Trans_Type{
  J_to_Q,    // Jacobi basis     -> Quadrature basis
  Q_to_J,    // Quadrature basis -> Jacobi basis
  F_to_P,    // Fourier basis    -> Physical basis
  P_to_F,    // Physical basis   -> Fourier basis
  O_to_Q,    // Orthogonal basis -> Quadrature basis
  Q_to_O,    // Quadrature basis -> Orthogonal basis
  F_to_P32,  // Fourier basis    -> Physical basis (dealiased)
  P_to_F32  // Physical basis   -> Fourier basis  (dealiased)
};



class Dorp{
 public:
  double d;                 /* double  value for straigth sided elements   */
  double *p;               /* pointer for curved side elements            */
};



/* Vertex Structure on triangle/tetrahedral */
class Vert {       /* ........... VERTEX definition ............. */
 public:
  int           id;         /* vertex id                                   */
  int           gid;        /* Global vertex id                            */
  int           eid;        /* Element id                                  */
  int           solve;      /* solution values to be solved for            */
  int           surf_eid;   /* Element number of surface element above     */
  int           surf_vid;   /* vertex number of surface vertex above     */

  double        x, y, z;    /* x,y and z coordinates of vertex             */
  double        *hj;        /* value of solution at vertex                 */
  Vert          *base;      /* pointer to base   edge in connected group   */
  Vert          *link;      /* Pointer to linked edge in connected group   */
};

/* Edge Structure on triangle/tetrahedral */
class Edge {       /* ........... EDGE definition ............... */
 public:
  int           id;         /* edge id                                     */
  int           gid;        /* Global edge id                              */
  int           eid;        /* Element id                                  */
  int           l;          /* Number of modes along this edge             */
  int           con;        /* connectivity trip                           */
  double        *hj;        /* modal value of solution along boundary      */
  Edge          *base;      /* pointer to base   edge in connected group   */
  Edge          *link;      /* Pointer to linked edge in connected group   */

  int           qedg;       /* number of gauss quadrature points in edge   */
  double        *h;         /* physical space storage for edge information */
  double        *jac;       /* edge jacobean/element jacobian at gauss pts */
  double        weight;     /* area weighting for viscous terms            */
  Coord         *norm;      /* edge normals at gauss points                */
};

/* Face Structure on triangle/tetrahedral */
class Face {       /* ........... FACE definition ............... */
 public:
  int           id;         /* face id                                     */
  int           gid;        /* Global face id                              */
  int           eid;        /* Element id                                  */
  int           l;          /* Number of modes along this face             */
  int           con;        /* connectivity trip for dsum                  */
  double        **hj;       /* modal values along faces                    */
  Face          *link;      /* Pointer to the linked face if any           */

  int           qface;       /* number of gauss quadrature points in edge   */
  double        *h;         /* physical space storage for edge information */
  double        *jac;       /* face jacobean/element jacobian at gauss pts */
  double        weight;     /* area weighting for viscous terms            */

  Coord         *n;      /* face normals  at gauss points                */
  Coord         *t;      /* face tangents at gauss points                */
  Coord         *b;      /* edge binormal at gauss points                */

};

class Coord{
 public:
  double *x;
  double *y;
  double *z;
};

/* geometric information */
class Geom{
 public:
  int    id;                /* geom structure id          */
  double dx[Max_Nverts];    /* dx between vertices        */
  double dy[Max_Nverts];    /* dy between vertices        */

  double dz[Max_Nverts];    /* dz between vertices        */
  Dorp   rx,ry,rz;          /* r geometric factors (3D)   */
  Dorp   sx,sy,sz;          /* s geometric factors (3D)   */
  Dorp   tx,ty,tz;          /* t geometric factors (3D)   */

  Dorp    jac;              /* Jacobian                   */
  Element *elmt;            /* generating element         */
  Geom    *next;            /* next structure in list     */
#ifdef ALE
   int change_con;
#endif
  int     singular;         /* flag to show if mapping is singular */
};

/* --------     C U R V E D   S I D E   T Y P E S     ---------- */
class C_Arc {    /* .............. Arc .............. */
 public:
  double   xc, yc;          /* Coordinates of the center         */
  double   radius;          /* Radius of the curve               */
};                          /* --------------------------------- */

class C_Naca4 {             /* .............. Arc .............. */
 public:
  double   xl, yl;          /* Leading  edge coordinates         */
  double   xt, yt;          /* Trailing edge coordinates         */
  double   thickness;       /* Thickness                         */
};                          /* --------------------------------- */

class C_Sin {
 public:
  double  xo, yo;
  double  amp;
  double  wavelength;
};

class C_File {               /* ............. File .............. */
 public:
  char *   name;             /* The name of the file to read      */
  double   xoffset;          /* Possible offsets of the           */
  double   yoffset;          /*    geometry for the current edge  */
  int      vert[Max_Nverts]; /* Possible offsets of the           */
};                           /* --------------------------------- */

class C_Recon{
  public:
  int      forceint;
  double   vn0[3];
  double   vn1[3];
  double   vn2[3];
};

class C_Ellipse {
 public:
  double  xo, yo;
  double  rmin, rmaj;
};

class C_Free {
  public:
    int nvc;  // number of curves in v direction
    int nwc;  // number of curves in w direction
    int Vperiodic; // = 0 for nonperiodic (in V) domain; = 1 for periodic domain
    int Wperiodic; // = 0 for nonperiodic (in W) domain; = 1 for periodic domain


    int Icell, Jcell; // index of a cell for which interpolation matrix exists

    double vl,wl;     // local  coordinates
    double vg,wg;     // global coordinates
    char   tag;       // label of curved surface

    double **coordX;  // x - coordinates
    double **coordY;  // y - coordinates
    double **coordZ;  // z - coordinates

    double **dx_dv;   // derivatieves dx/dv
    double **dy_dv;   // derivatieves dy/dv
    double **dz_dv;   // derivatieves dz/dv
    double **dx_dw;   // derivatieves dx/dw
    double **dy_dw;   // derivatieves dy/dw
    double **dz_dw;   // derivatieves dz/dw

    double **dx_dvdw; // derivatieves d^2x/dvdw
    double **dy_dvdw; // derivatieves d^2y/dvdw
    double **dz_dvdw; // derivatieves d^2z/dvdw

    double **C,**CT;  // C - standard interpolating  matrix  CT = transpose(C)
    double *V;        // V = [1 v v^2 v^3]
    double *W;        // W = [1 w w^2 w^3]

    double **M;      // interpolator matrix r = V'*C*M*CT*W

    double **CMCTx,**CMCTy,**CMCTz;  // CMCT = C*M*CT

    // allocate memory
    void allocate_memory();

    //  read from file and save databas with index = index_DB
    void load_from_grdFile(int index_DB, FILE *pFile);

    //  compute 1st derivatives in v and w direction
    //    dX_dv, dX_dw;   (X=x,y,z)
    void set_1der();

    // compute d^r / [dvdw]
    void set_2der();

    //  for given global parametric coordinates "v" and  "w" returns x,y,z
    void interpolate2d(double v1, double w1,  double *xyz);

    //  for given global parametric coordinates "v" and  "w" returns x,y,z  and derivatives:
    //   dXdv, dXdw;  X=(x,y,z)
    void interpolate_dvw_2d(double v1, double w1, double *xyz, double *dXvw, double *dYvw, double *dZvw);

    // projects point(x,y,z) onto surface and
    //   returns vw coordinates and new x,y,z coordinate
    void get_vw_safe(double *xyz,double *vw);
//    void get_vw(double *xyz,double *vw);
    int get_vw(double *xyz,double *vw);
    int get_vw(double *xyz,double *vw, double range, int Npoints, double *error);


};

// new for 3d

class C_Cylinder {/* .......... Cylinder ............. */
 public:
  double   xc, yc, zc;       /* Point on cylinder axis            */
  double   ax, ay, az;       /* Direction vector of axis          */
  double   radius;           /* Radius of the cylinder            */
};                /* --------------------------------- */

class C_Cone {    /* ............ Cone   ............. */
 public:
  double   xc, yc, zc;       /* Point at cone apex                */
  double   ax, ay, az;       /* Direction vector of axis          */
  double   alpha;            /* Expansion factor (rad = alpha z)  */
};

class C_Sphere {    /* ............ Cone   ............. */
 public:
  double   xc, yc, zc;       /* center of sphere                  */
  double   radius;           /* radius of sphere                  */
};                  /* --------------------------------- */

class C_Sheet {/* .......... Sheet ......................... */
 public:
  double   xc, yc, zc;       /* Point on sheet axis                     */
  double   ax, ay, az;       /* Direction vector of axis                */
  double   twist;            /* Twist of sheet rate/z unit              */
  double   zerotwistz;       /* Twist of the sheet in at zero 'z'       */
};                   /* --------------------------------------- */

class C_Spiral {/* .......... Spiral ....................... */
 public:
  double   xc, yc, zc;       /* Point on spiral axis                     */
  double   ax, ay, az;       /* Direction vector of axis                */
  double   axialradius;      /* axial radius                            */
  double   piperadius;       /* pipe radius                             */
  double   pitch;            /* Twist of spiral  rate/z unit              */
  double   zerotwistz;       /* Twist of the sheet in at zero 'z'       */
};

class C_Taurus {/* .......... Taurus ....................... */
 public:
  double   xc, yc, zc;       /* Point on taurus axis                     */
  double   ax, ay, az;       /* Direction vector of axis                */
  double   axialradius;      /* axial radius                            */
  double   piperadius;       /* pipe radius                             */
};

class C_Naca3d {/* .......... Naca3d ....................... */
 public:
  Coord    *origin;
  Coord    *axis;
  Coord    *lead;
  Coord    *locz;
  double   length;           /* length of foil                          */
  double   thickness;        /* % of length                             */
};

class C_Naca2d {/* .......... Naca3d ....................... */
 public:
  double   length;           /* length of foil                          */
  double   thickness;        /* % of length                             */
  double   xo;
  double   yo;
};


union CurveInfo {    /* ....... Curved Side Infos ....... */

  C_Arc      arc;           /* an arc with specified curvature   */
  C_Naca4    nac;           /* Naca4 digit aerofoil              */
  C_File     file;          /* Spline fit from file data         */
  C_Sin      sin;
  C_Naca2d   nac2d;        /* Symmetric naca */
  C_Ellipse  ellipse;

  C_Cylinder cyl;           /* cylinder specification            */
  C_Cone     cone;          /* cone     specification            */
  C_Sphere   sph;           /* sphere   specification            */
  C_Sheet    she;           /* sheet    specification            */
  C_Spiral   spi;           /* spiral   specification            */
  C_Taurus   tau;
  C_Naca3d   nac3d;         /* 3d Naca 00%% specification        */
  C_Free     free;          /* parametric surface                */
  C_Recon    recon;         /* surface obtained by smoothing     */

};                /* --------------------------------- */

enum  CurveType{            /* ....... Curved Side Types ....... */
  T_Straight,               /* straight edge                     */
  T_Arc,                    /* arc                               */
  T_Naca4,                  /* Naca 4 digit aerofoil             */
  T_File,                   /* Spline fit from file data         */
  T_Sin,                    /* Sin curve                          */
  T_Naca2d,                 /* Symmetric Naca foil */
  T_Ellipse,                /* Ellipse                          */

  T_Cylinder,               /* cylinder                          */
  T_Cone,                   /* cone                              */
  T_Sphere,                 /* sphere                            */
  T_Sheet,                  /* sheet                             */
  T_Spiral,                 /* spiral                            */
  T_Taurus,                 /* taurus                            */
  T_Naca3d,                 /* 2d naca 00%% foil with hom. direction */
  T_Curved,                 /* straight edge                     */
  T_Free,                   /* parametric surface                */
  T_Recon                   /* surface obtained by smoothing     */
};

class Curve {      /* .... CURVED SIDE Definition ..... */
 public:
  int          id;          /* curve id */
  int          face;        /* face id of curved side            */
  CurveType    type;        /* curve types                       */
  CurveInfo    info;        /* curve defs                        */
  Curve       *next;
};                    /* --------------------------------- */

// Warning
#define MAXFACETS  12

class Cmodes{
 public:
  double     *Cedge[MAXFACETS];  /* Modes of curved co-ordinates along edge */
  double    **Cface[Max_Nfaces]; /* Modes of curved co-ordinates along face */
};

/* Element Structure on triangle/tetrahedral */
class Element{
 public:
  int     id;               /* Element number                             */
  char    type;             /* Element type                               */
  char    state;            /* transformed state: 'p'=physical,           */
                /*                    't'=transformed         */
  int     Nverts;           /* Number of vertices                         */
  int     Nedges;           /* Number of vertices                         */
  int     Nfaces;           /* Number of edges                            */

  int     interior_l;       /* number of a modes in interior              */
  int     lmax;             /* maximum l order in element                 */
  int     Nmodes;           /* number of expansion modes in element       */
  int     Nbmodes;          /* number of boundary expansion modes         */
  int     qa;               /* number of quadrature points for a dir.     */
  int     qb;               /* number of quadrature points for b dir.     */
  int     qc;               /* number of quadrature points for c dir.     */
  int     qtot;             /* total number of quadrature points          */

  Vert    *vert;            /* start of vert structure for element        */
  Edge    *edge;            /* start of edge structure for element        */
  Face    *face;            /* start of face structure for element        */

  double  **h;              /*  h(x,y)   physical                         */

  double  ***h_3d;          // 3d field storage
  double  ***hj_3d;         // 3d interior modes storage

  Curve   *curve;           /* curve structure for this element           */
  Cmodes  *curvX;           /* curved surface modal co-ordinates          */
  Geom    *geom;            /* Geometric factors for element              */
  Element *next;            /* link list to next element                  */

  int dgL;                  /* order to use in discontinuous galerkin code */

  Element();                                         // default constructor
  Element(const Element&);                           // copy constructor
  Element(Element*);                                 // copy constructor

  // A base class without a virtual d-tor? You're mad and bad.
  virtual ~Element();

  // Local Matrix builders
  virtual void MassMat (LocMat *);                   // return mass-matri(ces)
  virtual void MassMatC(LocMat *);
  virtual void HelmMatC(LocMat *, Metric *lambda);   // return Helmholtz op.
  virtual void PSE_Mat(Element *E, Metric *lambda, LocMat *pse, double *DU);
  virtual void PSE_Mat(Element *E, LocMat *pse, double *DU){
    fprintf(stderr,"Not valid call on Element"); exit(1);}
  virtual void BET_Mat(Element *P, LocMatDiv *bet, double *beta,
           double *sigma){
    fprintf(stderr,"Not valid call on Element"); exit(1);}
  virtual void LapMat  (LocMat *);                   // return Laplacian op.
  virtual void mat_free(LocMat *m);
  virtual LocMat *mat_mem();
  virtual void fill_diag_massmat();         // fill modes with mass diagonal
  virtual void fill_diag_helmmat(Metric *lambda);   // fill modes with helm diagonal


  // Memory storage functions
  virtual void Mem_J(int *, char);
  virtual void Mem_Q();

  virtual void Mem_shift(double *, double *, char);  // updates storage pters
  virtual void Mem_free();                           // frees existing storage

  // Inner Product routines
  virtual double iprod(Mode *x, Mode *y);
  virtual double iprodlap(Mode *x, Mode *y, Mode *fac);

  // Helmholtz
  virtual void HelmHoltz(Metric *lambda);
  virtual void form_diprod(double *u1, double *u2, double *u3, Mode *m);

  // Global matrix routines
  virtual void condense(LocMat *m, Bsystem *Ubsys, char trip);
  virtual void project(LocMat *m, Bsystem *Ubsys);
  virtual void LowEnergyModes(Bsystem *B, int *Ne, int *Nf,
            double ***Rv, double ***Rvi, double ***Re);
  virtual void MakeLowEnergyModes(Bsystem *B, int *Ne, int *Nf,
          double ***Rv, double ***Rvi, double ***Re);


  // Bndry routines
  virtual Bndry *gen_bndry(char bc, int face, ...);
  virtual void  update_bndry(Bndry *, int save);
  virtual void   MemBndry(Bndry *B, int face, int Je);
  virtual void JtransEdge(Bndry *B, int id, int loc, double *f);
  virtual void JtransFace(Bndry *, double *);

  virtual void MakeFlux(Bndry *B, int iface, double *f);

  // Element Identifier
  virtual Nek_Facet_Type identify(){ return Nek_Element;}     // identify

  // Transformation routines
  virtual void Trans   (Element *, Nek_Trans_Type);    // Transform to Element
  virtual void Iprod   (Element *);                    // Inner product to Elmt
  virtual void Jbwd    (Element*, Basis*);
  virtual void Jbwdfac1(int face, double *vj, double *v);
  virtual void Jfwd    (Element*);

  void EdgeJbwd(double *, int);                      // Edge J_to_Q

  // orthogonal transform routines
  virtual void Obwd  (double *, double *, int);
  virtual void Ofwd  (double *, double *, int);
  virtual void Ofwd  (double *, double *, int, int);
  virtual void Add_Surface_Contrib(Element *, double *in, char dir,
           int edge );
  virtual void Add_Surface_Contrib(Element *, double *in, char dir,
           int edge, int invjac);
  virtual void Add_Surface_Contrib(double *in, char dir,  int edge );
  virtual void Add_Surface_Contrib(double *in, char dir,  int edge, int invjac);
  virtual void Add_Surface_Contrib(Element *, double *in, char dir);
  virtual void fill_edges(double *ux, double *uy, double *uz);

  virtual void Sign_Change();                        // correct mode signs
  virtual void SubtractBC(double alpha, double  beta, int Bface, Element *out);


  // Derivatives routines
  virtual void Grad  (Element *, Element *, Element *, char Trip);
  virtual void GradT (Element *, Element *, Element *, char Trip){
    fprintf(stderr,"This function is not yet define for the region\n");
    exit(1);}
  virtual void GradT (Element *, Element *, Element *, char Trip, bool invW){
    fprintf(stderr,"This function is not yet define for the region\n");
    exit(1);}
  virtual void Grad_d (double *, double *, double *, char Trip);
  virtual void Grad_h (double *, double *, double *, double *, char Trip);
  virtual void GradT_h (double *, double *, double *, double *, char Trip){
    fprintf(stderr,"This function is not yet define for the region\n");
    exit(1);}
  virtual void GradT_h (double *, double *, double *, double *, char Trip, bool invW){
    fprintf(stderr,"This function is not yet define for the region\n");
    exit(1);}

  //Get differential matrix
  virtual void getD(double ***da, double ***dat,double ***db,double ***dbt,
        double ***dc, double ***dct);

  virtual void fillElmt(Mode *v);
  virtual void fill_gradbase(Mode *gb, Mode *m, Mode *mb, Mode *fac);
  virtual Basis *getbasis();
  virtual Basis *derbasis();

  // Co-ordinate functions
  virtual void set_curved(Curve*);                    // fix curve sides
  virtual void coord(Coord *X);                       // get quadrature coords
  virtual void fillvec(Mode *v, double *f);
  virtual void straight_elmt(Coord *X);
  virtual void curved_elmt(Coord *X);
  virtual void straight_edge(Coord *X, int edge);
  virtual void GetFaceCoord(int face, Coord *X);

  // Error functions
  virtual void   Set_field(char *string);             // set field to function
  virtual void   Error(char *string);                 // compare with function
  virtual double L2_error_elmt(char *string);
  virtual double H1_error_elmt(char *string);
  virtual void   Verror(double *u, char *string);
  virtual double Int_error( char *string);
  virtual double Norm_li();
  virtual double Norm_l2();
  virtual void   Norm_l2m(double *l2, double *area);
  virtual double Norm_h1();
  virtual void   Norm_h1m(double *h1, double  *area);

  virtual double Norm_beta();

  // Curved sides
  virtual void set_curved_elmt(Element_List*);
  virtual void CoordTransEdge(double *f, double *fhat, int edge);
  virtual void get_mmat1d(double **mat, int L);

  // Geometric factors
  virtual void set_geofac();
  virtual void free_geofac();
  virtual void move_vertices(Coord *X);

  virtual void Surface_geofac(Bndry *B);
  virtual void InterpToFace1(int from_face, double *f, double *fi);
  virtual void InterpToGaussFace(int from_face, double *f,
         int qaf, int qbf, double *fi);
  virtual void GetFace(double *, int, double*);
  virtual void dump_mesh(FILE *);

  // B.C. functions
  virtual void set_solve(int fac, int mask);
  virtual void Add_flux_terms(Bndry *Ebc);
  virtual void setbcs(Bndry *Ubc,double *bc);
  virtual int  Nfmodes(void);
  virtual int  vnum(int,int);
  virtual int  fnum(int,int);
  virtual int  fnum1(int,int);
  virtual int  ednum(int,int);
  virtual int  ednum1(int,int);
  virtual int  ednum2(int,int);


  virtual int  dim();
  virtual int  Nfverts(int);

  // Eigenvalue routines
  virtual void fill_column(double **Mat, int loc, Bsystem *B,
         int nm, int offset);
  virtual void WeakDiff(Mode *m, double *ux, double *uy,double *uz, int con);

  // field file routines
  virtual int  data_len(int *size);
  virtual void Copy_field(double *, int *);

  virtual void close_split(Element_List *EL, Bndry **, int, int *&flag);
  virtual void split_element(Element_List *EL, Bndry **, int, int *&flag);
  virtual void split_edge(int edg, Element_List *EL, Bndry **Ubc, int nfields,int *flag);

  virtual void delete_element(Element_List *EL, Bndry **, int, int *flag);
  virtual void set_edge_geofac();
  virtual void set_edge_geofac(int invjac);
  virtual void PutFace(double*, int);

  virtual void GetZW(double **za, double **wa, double **zb, double **wb,
         double **zc, double **wc);
  virtual int edvnum(int,int);

  virtual void DivMat  (LocMatDiv *, Element *P);    // return Divergence op.
  virtual void divmat_free(LocMatDiv *m);
  virtual LocMatDiv *divmat_mem(Element *P);
  virtual double get_1diag_massmat(int id);  /* get diagional component of
            mass matrix corres. to id */
  virtual void condense_stokes(LocMat *m, LocMatDiv *d, Bsystem *Ubsys,
             Bsystem *Pbsys, Element *P);
  virtual void project_stokes(Bsystem *Pbsys, int asize, int geom_id);


  // Particle.C routines
  virtual int intersect_bnd    (Coord *Xi, double *vp,
        double *dt_remain, int *face)=0;
  virtual int  lcoords2face    (Coord *Xi, int *fac)=0;
  virtual void face2lcoords    (Coord *Xi, int *fac)=0;
  virtual void Cart_to_coll    (Coord csi, Coord *A)=0;
  virtual void Coll_to_cart    (Coord A, Coord *csi)=0;


  // local identifiers
  virtual int get_face_q1(int faceid){
    fprintf(stderr,"This class does not have the function get_face_q1\n");
    exit(1);
    return 0;
  }

  virtual int get_face_q2(int faceid){
    fprintf(stderr,"This class does not have the function get_face_q2\n");
    exit(1);
    return 0;
  }
};

class Multi_RHS {
public:
  char     type;
  int      Nrhs;
  int      step;
  int      nsolve;
  double  *alpha;
  double  **bt;
  double  **xt;
  double  *xbar;
  Multi_RHS *next;
};



class Element_List{
public:
  Element  *fhead;
  Element **flist;
  int       nel;
  int       htot;   // quadrature points in one level
  int       hjtot;  // sum of Nmodes over elements in one level
  double   *base_h;
  double   *base_hj;

  // Fourier information
  Element_List **flevels;      // Fourier lists
  int           nz;
  int           nztot;

  Element_List();
  Element_List(Element **hea, int n);
  ~Element_List();

  virtual Element *operator()(int i);
  virtual Element *operator[](int i);

  virtual void Cat_mem();
  virtual void Mem_shift(double *, double *);

  virtual Element_List *gen_aux_field(char ty);
  virtual void Terror(char *string);
  virtual void Trans(Element_List *EL, Nek_Trans_Type ntt);
  virtual void Iprod(Element_List *EL);
  virtual void Grad(Element_List *AL, Element_List *BL, Element_List *CL, char Trip);
  virtual void GradT(Element_List *AL, Element_List *BL, Element_List *CL, char Trip);
  virtual void GradT(Element_List *AL, Element_List *BL, Element_List *CL, char Trip, bool invW);
  virtual void Grad_d (double *, double *, double *, char Trip);
  virtual void Grad_h (double *HL, double *AL, double *BL, double *CL, char Trip);
  virtual void GradT_h (double *HL, double *AL, double *BL, double *CL, char Trip);
  virtual void GradT_h (double *HL, double *AL, double *BL, double *CL, char Trip, bool invW);
  virtual void HelmHoltz(Metric *lambda);
  virtual void Set_field(char *string);
  virtual void zerofield();
  virtual void FFT(Element_List *EL, Nek_Trans_Type ntt);
  virtual void Grad_z(Element_List *EL);
  virtual void Set_state(char type);
  virtual void H_Refine(int * to_split, int nfields, Bndry **Ubc, int *flag);

};


/* structure for each face on boundary */
class Bndry{
 public:
  int        id;                  /* bndry  number                         */
  char       type;                /* type of boundary used by library      */
  char    usrtype;                /* type of boundary used by src          */

  int        face;                /* face/edge boundary conditions are for */
  double    *bvert;               /* vertex boundary information           */
  double     sigma;               /* coefficient for Robin boundary cond.  */

  // new for 3d
  double    *bedge[4];            /* edge information                      */
  double   **bface;               /* face information                      */

  double    *pedge;

  double    *DirRHS;              /* RHS vector of fixed time  Dirichlet   */
                                  /* values (not used if not initialised   */
  char      *bstring;             /* String read in from .rea file         */
  char      *blabel;           /*LG: label of baoundary, read in from .rea */

  double   *vertu;
  double   **edgeu;
  double   *faceu;

  double   *vertv;
  double   **edgev;
  double   *facev;

  double   *vertw;
  double   **edgew;
  double   *facew;

  double *xvel;
  double   *fz;
  double   *fp;
  double   *z;

  Dorp      nx;                   /* x component of surface normal         */
  Dorp      ny;                   /* y component of surface normal         */
  Dorp      nz;                   /* z component of surface normal         */

  Dorp      sjac;                 /* surface jacobean                      */
  Dorp      K;                    /* Curvature of the surface              */

  Element   *elmt;                /* element that boundary side is on      */
  Bndry     *next;

  // new for Compressible

  double    *phys_val;            /* physical values for Dirichlet b.c.    */
  double    *phys_val_g;          /* physical values for Dirichlet b.c.    */

  double   *centre;
  int      **edge_ids;
  double   uvel;
  double   vvel;
  double   *u;
  double   *v;
  double   *w;

};

/* class SMatrix added by Leopold Grinberg */
#include "SMatrix.h"


/* Direct solve matrix system */
class MatSys{
 public:
  double **inva;   /* inverse of global elemental boundary matrix      */
  double **a;      /* local a matrix used in iterative solver          */
  double **binvc;  /* b * inv(c)                                       */
  double **invc;   /* inverse of interior-interior matrix              */
  double **invcd;  /* inv(c) * d for non-symetric solver               */
  int    **cipiv;  /* pivot for inv(c) in non-symmetric solve          */
  double **d;      /* interior-boundary for non-symmetric system       */
  int    bwidth_a; /* bandwidth of boundary-boundary matrix            */
  int    *bwidth_c;/* bandwidth of interior-interior matrix            */
  double ***dbinvc;/* dib *inv(c) where i = x,y or z                   */
  int    *pivota;  /* pivot for inverse if system is not positive def. */
};

class P_Diag{
 public:
  int      ndiag;   /* number of entries      */
  double  *idiag;   /* inverse of diagonal    */
};

class P_Diag_Stokes{
 public:
  int      ndiag;   /* number of entries      */
  double  *idiag;   /* inverse of diagonal    */
  int      nbcb;    /* size of b inv(c) b     */
  double  *binvcb;  /* b^T inv(c) b           */
};

class P_Block{
 public:
  int      nvert;   /* number of vertices     */
  double  *ivert;   /* vertex preconditioner  */
  int     *Ledge;   /* list of L values       */
  int      nedge;   /* number of edges        */
  double **iedge;   /* edge   preconditioner  */
  int      nface;   /* number of faces        */
  int     *Lface;   /* list of L values       */
  double **iface;   /* face   preconditioner  */
};

class P_Block_Stokes{
public:
  int      nvert;   /* number of vertices     */
  double  *ivert;   /* vertex preconditioner  */
  int     *Ledge;   /* list of L values       */
  int      nedge;   /* number of edges        */
  double **iedge;   /* edge   preconditioner  */
  int      nface;   /* number of faces        */
  int     *Lface;   /* list of L values       */
  double **iface;   /* face   preconditioner  */
  int      nbcb;    /* size of b inv(c) b     */
  double  *binvcb;  /* b^T inv(c) b           */
};

class P_LEnergy{
 public:
  int      nvert;       /* number of vertices             */
  int      bw;          /* band width of vertex system    */
  double  **ivert;      /* vertex preconditioner          */
  /* These next two matrices are only used in parallel solve */
  double  **ivert_B;    /* Boundary-interior vertex preconditioner */
  double  **ivert_C;    /* interior-interior vertex preconditioner  */

  double  *mult;        /* multiplicity                   */
  int     *Ledge;       /* list of L values               */
  int      nedge;       /* number of edges                */
  double **iedge;       /* edge   preconditioner          */
  int      nface;       /* number of faces                */
  int     *Lface;       /* list of L values               */
  double **iface;       /* face   preconditioner          */
  double *levert;       /* inverse of low energy vertices */

  SMatrix *SM_local;    /* sparce matrix to store local
                           values of ivert                */
  int *DESC_ivert, *DESC_rhs;
  int *BLACS_PARAMS;
  int *ivert_ipvt;
  double **ivert_local;
  int *map_row;
  char ivert_type;
  int first_row;        /* A_local[row = 0][] = A_global[row = first_row][] */
  int first_col;        /* A_local[][col = 0] = A_global[][col = first_col] */
  int *col_displs;      /* displacement vector and receive counter          */
  int *col_rcvcnt;      /* required for parallel_dgemv                      */
  int  *GS_col_sendcntr;  /*  */
  int  *GS_col_recvcntr;
  int **GS_col_index_list_send;
  int **GS_col_index_list_recv;


  double **GS_col_sendbuf;
  double **GS_col_recvbuf;
  int Npartners_send,Npartners_recv;
  int *GS_col_partner_list_send;  /* list of proc. ranks to wich data will be sent */
  int *GS_col_partner_list_recv;  /* list of proc. ranks from wich data will be received */

  int  *GS_row_sendcntr;  /*  */
  int  *GS_row_recvcntr;
  int **GS_row_index_list_send;
  int **GS_row_index_list_recv;
  double **GS_row_sendbuf;
  double **GS_row_recvbuf;
  double *GS_row_sendbuf_new;
  double *GS_row_recvbuf_new;
  int *GS_row_partner_list_send;  /* list of proc. ranks to wich data will be sent */
  int *GS_row_partner_list_recv;  /* list of proc. ranks from wich data will be received */
};

class P_Overlap{
public:
  int      ndiag;   /* number of entries      */
  double  *idiag;   /* inverse of diagonal    */

  int npatches;
  int **maskflags;
  int **solveflags;
};


enum OverlapType{
  NoOverlap,
  File,
  VertexPatch,
  ElementPatch,
  Coarse
};


class Overlap{
 public:
  OverlapType type;    // type of patch to use
    int         coarse;  // switch for coarse space
    int         npatches; // # of patches to use in the overlapping precon.
    int         Ntotal;   // total # of d.o.f
    int     **maskflags;  // npatchesxnel matrix of solve flags 1=solve, 0=mask
    double **solveflags;  // npatchesxnglobal matrix of solve flags 1=solve, 0=mask
    int    **patchmaps;
    int     *patchlengths;
    double **patchinvA;
    double  *patchex;
    int     *patchbw;
};

union PreInfo{
  P_Diag           diag;
  P_Block          block;
  P_Diag_Stokes    diagst;
  P_Block_Stokes   blockst;
  P_LEnergy        lenergy;
  P_Overlap        overlap;
};


enum PreType{
  Pre_Diag,
  Pre_Block,
  Pre_None,
  Pre_LEnergy,
  Pre_Diag_Stokes,
  Pre_Block_Stokes,
  Pre_Overlap,
  Pre_PLEnergy_Udiag
};


class MatPre{
 public:
  PreInfo  info;
};

typedef enum{
  direct,    /* direct    solver */
  iterative  /* iterative solver */
} SolMeth;

class  Recur{
 public:
  int      id;          /* recursion level                           */
  int      cstart;
  int      npatch;      /* number of patches in level                */
  int     *patchlen_a;  /* length of boundary of patch               */
  int     *patchlen_c;  /* length of interior of patch               */
  int    **map;         /* local to global mapping of patch boundary */
  int     *bwidth_c;    /* bandwidth of interior matrices            */
  int     *pmap;        /* old to new patch mapping (necc. for L=2)  */
  double **invc;        /* inverted interior system                  */
  double **binvc;       /* boundary coupling by inverted interior sy */

  int    **pivotc;      /* pivot for c inverse if a is not positive def. */
};

class Blockp { /* info required to set up recursive block precon */
 public:
  int   nge;            /* number of global edges in innner solve         */
  int  *ngv;            /* number of global vertices in each patch        */
  int  *nle;            /* number of  local edges in each patch           */
  int  **edglen;        /* edgelen of local bndy edges                    */
  int  **edgid;         /* global id of local contributions               */

  int   ngf;            /* number of global faces in innner solve         */
  int  *nlf;            /* number of  local faces in each patch           */
  int  **faclen;        /* edgelen of local bndy faces                    */
  int  **facgid;        /* global id of local contributions               */

  int  nlgid;           /* number of global vertices in interior patches  */
  int  **lgid;          /* global id of local contributions               */
};


union Precond{
  Blockp blk;
};

class Rsolver{
 public:
  int     nrecur;    /* number of recurrsive solves of the boundary system */
  Recur  *rdata;     /* recurrsive info about solver                       */
  int    max_asize;  /* maximum value of asize in recursions               */
  union{
    int    bwidth_a; /* bandwidth of inva if it is used                    */
    int    nv_solve; /* number of vertices in A (required for iter. solve) */
  } Ainfo;
  struct{
    double *inva;
    double **a;
    int    *pivota;
  } A;
  /* final boundary system                              */
  Precond *precon;   /* preconditioner for iterative solver if required    */
};

#include "csgs.h"
//#include "sgs.h"
//#include "gs_nektar.h"

/* mapping information for parallel solver          */
class Pllmap{
 public:
  int  nv_solve;            /* global vertices to solve                   */
  int  nsolve;              /* global nsolve                              */
  int  nglobal;             /* global unkowns                             */
  int  nv_gpsolve;          /* number of global vertices along partitions */
  int  nv_lpsolve;          /* number of global vertices along partitions */
                            /* within local partition                     */
  int    singular;          /* id of local singular vert (0 if not in part)*/
  int    *solvemap;         /* local to global map of solved variables    */
  int    *knownmap;         /* local to global map of known  variables    */
  struct gather_scatter_id *solve; /* solve map for 'gs' routine          */
  struct gather_scatter_id *known; /* known map for 'gs' routine          */

#ifdef PARALLEL
  CSGS csgs_solve;
  CSGS csgs_known;
#endif

  double *mult;             /* multiplicity of data over processors       */
};

class Metric{
 public:
  double d;                 /* double  value for straigth sided elements   */
  double *p;                /* pointer for curved side elements            */
  double **wave;            /* wave speed for oseen solver                 */
  double *epsilon;          /* Spectral Viscosity Addition SV              */
  int    MN;                /* Spectral Viscosity Addition SV              */
};

class Bsystem{
 public:
  SolMeth smeth;    /* solution method -- direct or iterative           */
  int nel;          /* number of elements                               */
  int nv_solve;     /* number of vertices to solve for                  */
  int ne_solve;     /* number of edges to solve for                     */
  int nf_solve;     /* number of faces to solve for                     */

  int nsolve;       /* number of solution points in boundary system     */
  int nglobal;      /* number of global boundary points                 */
  int *edge;        /* cumulative list of global edge start position    */
  int *face;        /* cumulative list of global face start position    */

  int **bmap;       /* list of bmaps in each element for scat/gath ops  */
  int singular;     /* If activated then have singular poisson case     */
  int families;     /* number of families                               */
  Metric *lambda;   /* Helmholtz constant                               */ // list of NEL Metric.s
  MatSys *Gmat;     /* Global  Matrix System                            */
  PreType  Precon;  /* preconditioner type                              */
#if 0
  int    Precon;    /* preconditioner type                              */
#endif
  MatPre  *Pmat;    /* Proconditioning Matrix System                    */
  Rsolver *rslv;    /* Recursive solver information                     */
  Pllmap  *pll;     /* mapping information for parallel solver          */
  double  *signchange; /* connectivity Z matrix */
  struct gather_scatter_id *egather; /* parallel gather for LE precon   */
  struct gather_scatter_id *fgather; /* parallel gather for LE precon   */

#ifdef PARALLEL
  CSGS gs;
 // GS_nektar GS_egather;
 // SGS *SGS_egather;
#endif

  // Multiple RHS
  Multi_RHS *mrhs;

  // Overlapping info.
  Overlap      *overlap;
};

class Field {   /* ..... Field FILE structure ...... */
 public:
  char*   name;              /* session name                        */
  char*   created;           /* date and time created               */
  char    state;             /* transformed state of data           */
  int     dim;               /* Dimension of run                    */
  int     nel;               /* number of elements                  */
  int     lmax;              /* maximum l in fields                 */
  int     step;              /* time step number                    */
  int     nz;                /* number of fourier planes            */
  double  lz;                /* periodic length                     */

  double  time;              /* time                                */
  double  time_step;         /* time step                           */
  double  kinvis;            /* kinematic viscosity                 */
  char*   format;            /* file format string                  */
  char    type[MAXFIELDS];   /* list of variable types              */
  double* data[MAXFIELDS];   /* array of data values                */
  int*    nfacet;            /* array number of facets in elements  */
  int*    size;              /* array giving size of data           */
  int*    emap;              /* local to global mapping of elements */

};                           /* --------------------------------- */



class Membrane{
public:

  Membrane();                                     //constructor
  Membrane(double tension, double mass_density, int lbc, int rbc);
  Membrane(Membrane &M);                          //copy constructor
  ~Membrane();                                    //destuctor
  void Setup(Element_List *EL, Bndry *UBc);
  void CalcMeshVelocity(Element_List *EL, Bndry *UBc, Bndry *VBc, int nstep, double dt);

  //---------------------------------------------------------------
  int nelmt;             //number of elements across top
  int norder;            //num quad points per element
  double * xpts;         //x coords from left to right
  double * netp;         //net pressure at xpts
  double * deform;       //deformation of the membrane
  double * velmem;       //velocity of the membrane
  int ** top_elmts;      //[][0] = elmt number; [][1] = face num;
  int ** bottom_elmts;
  int * BCflags;
  double *physdata;  /*Physical data of the membrane ([0] = tension: [1] = mass density)*/
  double phead;
  int pvert;
  int pelmt;
};
#ifdef ALE
class Corner{
public:
   int elem[7];
   int vertex[7];
   double x[7];
   double y[7];
};
#endif

#ifdef PBC_1D_LIN_ATREE

class PBC1DLINATREE;

/* Impedance outlet boundary condtions.    */
/* class added by Leopold Grinberg         */
#include "small_atree.h"
#include "PBC_1D_LIN_ATREE.h"

#endif


#include "Tri.h"
#include "Quad.h"
#include "Prism.h"
#include "Pyr.h"
#include "Tet.h"
#include "Hex.h"

#endif
