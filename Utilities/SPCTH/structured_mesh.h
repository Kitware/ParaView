#ifndef STRUCTURED_MESH_H
#define STRUCTURED_MESH_H

/* Id
 *
 * structured_mesh.h - structured mesh structures
 *
 * David A. Crawford
 * Computational Physics and Mechanics
 * Sandia National Laboratories
 * Albuquerque, New Mexico 87185
 *
 */

typedef struct
{
  /* Block activity flag */

  int allocated;
  int active;

  /* Block generation (for adaptive meshes) */

  int level;

  /* Definition of Grid */

  int Nx;
  int Ny;
  int Nz;

  double *x;
  double *y;
  double *z;

  /* Boundary conditions:
     -1 = internal boundary, ghost cells contain valid data
      0 = reflecting boundary, ghost cells should be copied from adjacent internal cells
     >0 = other boundary condition */

  int BXbot;
  int BXtop;
  int BYbot;
  int BYtop;
  int BZbot;
  int BZtop;

  /* Definition of Cell-Centered Fields */
  /* Ordering: CField,Z,Y,X */
 
  double ****CField;

  /* Definition of Material Fields */
  /* Ordering: MField,Mat,Z,Y,X */

  double *****MField;
  
} Structured_Block_Data;

typedef struct
{
  /* Geometry type */

  int IGM;

  /* Number of dimensions */

  int Ndim;

  /* Min and Max global mesh dimensions */

  double Gmin[3];
  double Gmax[3];

  /* Maximum levels for adaptive meshes */

  int max_level;

  /* Registration of Cell-Centered Fields */

  int NCFields;
  char **CField_id;
  char **CField_comment;
  int *CField_int;

  /* Number of materials */

  int Nmat; 
  int MaxMat;

  /* Registration of Material Fields */

  int NMFields;
  char **MField_id;
  char **MField_comment;
  /* This includes one id for each field and (material+1) */
  int *MField_int;

  /* Number of blocks */

  int Nblocks;

  /* Block data */

  Structured_Block_Data *block;

  /* Number of tracers */

  int NTracers;

  /* Tracer id */

  int *Tracer_id;

  /* Location of tracers */

  double *XTracer, *YTracer, *ZTracer;

  /* Tracer Location Logicals

    Note: These indices use FORTRAN array ordering (first location is 1)
    
    Tracer block id if (LTracer[n] <=0) then tracer is not active or not
     on this processor */

  int *LTracer;

  /* Tracer cell location indices (within a block)
     ITracer[n] = X index
     JTracer[n] = Y index
     KTracer[n] = Z index  */

  int *ITracer, *JTracer, *KTracer;

  /* Indicator information for AMR */

  int NIndicators;
  int MaxBin;
  int *NBins;
  double **Histogram;
  double *HistMin, *HistMax;
  int *HistType;
  double *RefAbove, *RefBelow;
  double *UnrAbove, *UnrBelow;

} Structured_Mesh_Data;

#endif /* STRUCTURED_MESH_H */
