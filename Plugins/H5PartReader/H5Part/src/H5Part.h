#ifndef H5Part_h
#define H5Part_h

#include <stdlib.h>
#include <stdarg.h>
#include <vtk_hdf5.h>

#ifdef H5PART_HAS_MPI
#include <mpi.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "H5PartTypes.h"


#define H5PART_SUCCESS  0
#define H5PART_ERR_NOMEM -12
#define H5PART_ERR_INVAL -22
#define H5PART_ERR_BADFD -77

#define H5PART_ERR_INIT         -200
#define H5PART_ERR_NOENTRY -201

#define H5PART_ERR_MPI  -201
#define H5PART_ERR_HDF5  -202


#define H5PART_READ  0x01
#define H5PART_WRITE  0x02
#define H5PART_APPEND  0x03


#define H5PART_INT64  ((h5part_int64_t)H5T_NATIVE_INT64)
#define H5PART_FLOAT64  ((h5part_int64_t)H5T_NATIVE_DOUBLE)
#define H5PART_CHAR  ((h5part_int64_t)H5T_NATIVE_CHAR)

/*========== File Opening/Closing ===============*/
H5PartFile*
H5PartOpenFile(
 const char *filename,
 const unsigned flags
 );

#define H5PartOpenFileSerial(x,y) H5PartOpenFile(x,y)

#ifdef H5PART_HAS_MPI
H5PartFile*
H5PartOpenFileParallel (
 const char *filename,
 const unsigned flags,
 MPI_Comm communicator
 );

H5PartFile*
H5PartOpenFileParallelIndependent (
 const char *filename,
 const unsigned flags,
 MPI_Comm communicator
 );
#endif


h5part_int64_t
H5PartCloseFile (
 H5PartFile *f
 );


/*============== File Writing Functions ==================== */
h5part_int64_t
H5PartDefineStepName (
 H5PartFile *f,
 const char *name,
 const h5part_int64_t width
 );

h5part_int64_t
H5PartSetNumParticles ( 
 H5PartFile *f, 
 const h5part_int64_t nparticles
 );

h5part_int64_t
H5PartWriteDataFloat64 (
 H5PartFile *f,
 const char *name,
 const h5part_float64_t *array
 );

h5part_int64_t
H5PartWriteDataInt64 (
 H5PartFile *f,
 const char *name,
 const h5part_int64_t *array
 );

/*================== File Reading Routines =================*/
h5part_int64_t
H5PartSetStep (
 H5PartFile *f,
 const h5part_int64_t step
 );

h5part_int64_t
H5PartHasStep (
 H5PartFile *f,
 const h5part_int64_t step
 );

h5part_int64_t
H5PartGetNumSteps (
 H5PartFile *f
 );

h5part_int64_t
H5PartGetNumDatasets (
 H5PartFile *f
 );

h5part_int64_t
H5PartGetDatasetName (
 H5PartFile *f,
 const h5part_int64_t idx,
 char *name,
 const h5part_int64_t maxlen
 );

h5part_int64_t
H5PartGetDatasetInfo (
 H5PartFile *f,
 const h5part_int64_t idx,
 char *name,
 const h5part_int64_t maxlen,
 h5part_int64_t *type,
 h5part_int64_t *nelem);


h5part_int64_t
H5PartGetNumParticles (
 H5PartFile *f
 );

h5part_int64_t
H5PartSetView (
 H5PartFile *f,
 const h5part_int64_t start,
 const h5part_int64_t end
 );


h5part_int64_t
H5PartGetView (
 H5PartFile *f,
 h5part_int64_t *start,
 h5part_int64_t *end
 );

h5part_int64_t
H5PartHasView (
 H5PartFile *f
 );

h5part_int64_t
H5PartResetView (
 H5PartFile *f
 );

h5part_int64_t
H5PartSetCanonicalView (
 H5PartFile *f
 );

h5part_int64_t
H5PartReadDataFloat64(
 H5PartFile *f,
 const char *name,
 h5part_float64_t *array
 );

h5part_int64_t
H5PartReadDataInt64 (
 H5PartFile *f,
 const char *name,
 h5part_int64_t *array
 );

h5part_int64_t
H5PartReadParticleStep (
 H5PartFile *f,
 const h5part_int64_t step,
 h5part_float64_t *x, /* particle positions */
 h5part_float64_t *y,
 h5part_float64_t *z,
 h5part_float64_t *px, /* particle momenta */
 h5part_float64_t *py,
 h5part_float64_t *pz,
 h5part_int64_t *id /* and phase */
 );

/**********==============Attributes Interface============***************/
/* currently there is file attributes:  Attributes bound to the file
   and step attributes which are bound to the current timestep.  You 
   must set the timestep explicitly before writing the attributes (just
   as you must do when you write a new dataset.  Currently there are no
   attributes that are bound to a particular data array, but this could
   easily be done if required.
*/
h5part_int64_t
H5PartWriteStepAttrib (
 H5PartFile *f,
 const char *attrib_name,
 const h5part_int64_t attrib_type,
 const void *attrib_value,
 const h5part_int64_t attrib_nelem
 );

h5part_int64_t
H5PartWriteFileAttrib (
 H5PartFile *f,
 const char *attrib_name,
 const h5part_int64_t attrib_type,
 const void *attrib_value,
 const h5part_int64_t attrib_nelem
 );

h5part_int64_t
H5PartWriteFileAttribString (
 H5PartFile *f,
 const char *name,
 const char *attrib
 );

h5part_int64_t
H5PartWriteStepAttribString ( 
 H5PartFile *f,
 const char *name,
 const char *attrib
 );

h5part_int64_t
H5PartGetNumStepAttribs ( /* for current filestep */
 H5PartFile *f
 );

h5part_int64_t
H5PartGetNumFileAttribs (
 H5PartFile *f
 );

h5part_int64_t
H5PartGetStepAttribInfo (
 H5PartFile *f,
 const h5part_int64_t attrib_idx,
 char *attrib_name,
 const h5part_int64_t len_of_attrib_name,
 h5part_int64_t *attrib_type,
 h5part_int64_t *attrib_nelem
 );

h5part_int64_t
H5PartGetFileAttribInfo (
 H5PartFile *f,
 const h5part_int64_t idx,
 char *name,
 const h5part_int64_t maxnamelen,
 h5part_int64_t *type,
 h5part_int64_t *nelem
 );

h5part_int64_t
H5PartReadStepAttrib (
 H5PartFile *f,
 const char *name,
 void *data
 );

h5part_int64_t
H5PartReadFileAttrib (
 H5PartFile *f,
 const char *name,
 void *data
 );

h5part_int64_t
H5PartSetVerbosityLevel (
 const h5part_int64_t level
 );

h5part_int64_t
H5PartSetErrorHandler (
 const h5part_error_handler handler
 );

h5part_int64_t
H5PartGetErrno (
 void
 );

h5part_error_handler
H5PartGetErrorHandler (
 void
 );

h5part_int64_t
H5PartReportErrorHandler (
 const char *funcname,
 const h5part_int64_t eno,
 const char *fmt,
 ...
 );

h5part_int64_t
H5PartAbortErrorHandler (
 const char *funcname,
 const h5part_int64_t eno,
 const char *fmt,
 ...
 );

#ifdef __cplusplus
}
#endif

#endif
