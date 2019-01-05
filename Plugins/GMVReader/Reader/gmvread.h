#ifndef _GMVREADH_
#define _GMVREADH_

#ifndef RDATA_INIT
#define EXTERN extern
#else
#define EXTERN /**/
#endif

/*  Keyword types.  */
#define NODES       1
#define CELLS       2
#define FACES       3
#define VFACES      4
#define XFACES      5
#define MATERIAL    6
#define VELOCITY    7
#define VARIABLE    8
#define FLAGS       9
#define POLYGONS   10
#define TRACERS    11
#define PROBTIME   12
#define CYCLENO    13
#define NODEIDS    14
#define CELLIDS    15
#define SURFACE    16
#define SURFMATS   17
#define SURFVEL    18
#define SURFVARS   19
#define SURFFLAG   20
#define UNITS      21
#define VINFO      22
#define TRACEIDS   23
#define GROUPS     24
#define FACEIDS    25
#define SURFIDS    26
#define CELLPES    27
#define SUBVARS    28
#define GHOSTS     29
#define VECTORS    30
#define CODENAME   48
#define CODEVER    49
#define SIMDATE    50
#define GMVEND     51
#define INVALIDKEYWORD 52
#define GMVERROR 53


/*  Data types for Nodes:  */
#define UNSTRUCT  100
#define STRUCT 101
#define LOGICALLY_STRUCT 102
#define AMR 103
#define VFACES2D 104
#define VFACES3D 105
#define NODE_V   106

/*  Data types for Cells:  */
#define GENERAL 110
#define REGULAR 111
#define VFACE2D 112
#define VFACE3D 113

/*  Data types for vectors, variables, materials, flags, tracers, groups: */
#define NODE       200
#define CELL       201
#define FACE       202
#define SURF       203
#define XYZ        204
#define TRACERDATA 205
#define VEL        206
#define ENDKEYWORD 207
#define FROMFILE   208

#define MAXKEYWORDLENGTH       8
#define MAXCUSTOMNAMELENGTH   33
#define MAXFILENAMELENGTH    300

EXTERN struct gmv_data_type
         {
          int     keyword;    /*  See above for definitions.  */
          int     datatype;   /*  See above for definitions.  */
          char    name1[MAXCUSTOMNAMELENGTH];  /*  hex, tri, etc, flag name, field name.  */
          long    num;        /*  nnodes, ncells, nsurf, ntracers.  */
          long    num2;       /*  no. of faces, number of vertices.  */

          long    ndoubledata1;
          double  *doubledata1;
          long    ndoubledata2;
          double  *doubledata2;
          long    ndoubledata3;
          double  *doubledata3;

          long    nlongdata1;
          long    *longdata1;
          long    nlongdata2;
          long    *longdata2;

          int     nchardata1;   /*  Number of 33 character string.  */
          char    *chardata1;   /*  Array of 33 character strings.  */
          int     nchardata2;   /*  Number of 33 character string.  */
          char    *chardata2;   /*  Array of 33 character strings.  */

          char    *errormsg;
         } 
     gmv_data;


EXTERN struct gmv_meshdata_type
         {
          long    nnodes; 
          long    ncells;
          long    nfaces;
          long    totfaces;
          long    totverts;
          int     intype;  /* CELLS, FACES, STRUCT, LOGICALLY_STRUCT, AMR. */
          int     nxv;  /*  nxv, nyv, nzv for STRUCT,  */
          int     nyv;  /*  LOGICALLY_STRUC and AMR.   */
          int     nzv;

          double  *x;  /*  Node x,y,zs, nnodes long.  */
          double  *y;
          double  *z;

          long    *celltoface;   /*  Cell to face pointer, ncells+1 long. */
          long    *cellfaces;    /*  Faces in cells, totfaces+1 long.  */
          long    *facetoverts;  /*  Face to verts pointer, nfaces+1 long.*/
          long    *faceverts;    /*  Verts per face, totverts long.  */
          long    *facecell1;    /*  First cell face attaches to.  */
          long    *facecell2;    /*  Second cell, nfaces long.  */
          long    *vfacepe;      /*  Vface pe no.  */
          long    *vfaceoppface;  /*  Vface opposite face no.  */
          long    *vfaceoppfacepe;  /*  Vface opposite face pe no.  */
          long    *cellnnode;    /*  No. of nodes per cell (regular cells).  */
          long    *cellnodes;    /*  Node list per cell (regular cells).  */
         } 
     gmv_meshdata;

/*  C, C++ prototypes.  */

int gmvread_checkfile(char *filnam);

int gmvread_open(char *filnam);

int gmvread_open_fromfileskip(char *filnam);

void gmvread_close(void);

void gmvread_data(void);

void gmvread_mesh(void);

void gmvread_printon();

void gmvread_printoff();

void struct2face(void);

void struct2vface(void);

#endif
