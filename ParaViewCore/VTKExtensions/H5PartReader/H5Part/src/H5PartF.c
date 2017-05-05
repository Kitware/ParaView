#include "H5Part.h"
#include "Underscore.h"
#include <vtk_hdf5.h>

#if defined(F77_SINGLE_UNDERSCORE)
#define F77NAME(a,b) a
#elif defined(F77_CRAY_UNDERSCORE)
#define F77NAME(a,b) b
#elif defined(F77_NO_UNDERSCORE)
#else
#error Error, no way to determine how to construct fortran bindings
#endif

#if ! defined(F77_NO_UNDERSCORE)

/* open/close interface */
#define h5pt_openr F77NAME (      \
     h5pt_openr_,   \
     H5PT_OPENR )
#define h5pt_openw F77NAME (      \
     h5pt_openw_,   \
     H5PT_OPENW )
#define h5pt_opena F77NAME (      \
     h5pt_opena_,   \
     H5PT_OPENA )
#define h5pt_openr_par F77NAME (     \
     h5pt_openr_par_,  \
     H5PT_OPENR_PAR )
#define h5pt_openw_par F77NAME (     \
     h5pt_openw_par_,  \
     H5PT_OPENW_PAR )
#define h5pt_opena_par F77NAME (     \
     h5pt_opena_par_,  \
     H5PT_OPENA_PAR )
#define h5pt_close F77NAME (      \
     h5pt_close_,   \
     H5PT_CLOSE) 

/* writing interface */
#define h5pt_setnpoints F77NAME (     \
     h5pt_setnpoints_,  \
     H5PT_SETNPOINTS )
#define h5pt_setstep F77NAME (      \
     h5pt_setstep_,   \
     H5PT_SETSTEP )
#define h5pt_writedata_r8 F77NAME (     \
     h5pt_writedata_r8_,  \
     H5PT_WRITEDATA_R8 )
#define h5pt_writedata_i8 F77NAME (     \
     h5pt_writedata_i8_,  \
     H5PT_WRITEDATA_I8 )

/* Reading interface  (define dataset, step, particles, attributes) */
#define h5pt_getnsteps F77NAME (     \
     h5pt_getnsteps_,  \
     H5PT_GETNSTEPS )
#define h5pt_getndatasets F77NAME (     \
     h5pt_getndatasets_,  \
     H5PT_GETNDATASETS )
#define h5pt_getnpoints F77NAME (     \
     h5pt_getnpoints_,  \
     H5PT_GETNPOINTS )
#define h5pt_getdatasetname F77NAME (     \
     h5pt_getdatasetname_,  \
     H5PT_GETDATASETNAME )
#define h5pt_getnumpoints F77NAME (     \
     h5pt_getnumpoints_,  \
     H5PT_GETNUMPOINTS )

/* Views and parallelism */
#define h5pt_setview F77NAME (      \
     h5pt_setview_,   \
     H5PT_SETVIEW )
#define h5pt_resetview F77NAME (     \
     h5pt_resetview_,  \
     H5PT_RESETVIEW )
#define h5pt_hasview F77NAME (      \
     h5pt_hasview_,   \
     H5PT_HASVIEW )
#define h5pt_getview F77NAME (      \
     h5pt_getview_,   \
     H5PT_GETVIEW )

/* Reading data */
#define h5pt_readdata_r8 F77NAME (     \
     h5pt_readdata_r8_,  \
     H5PT_READDATA_R8 )
#define h5pt_readdata_i8 F77NAME (     \
     h5pt_readdata_i8_,  \
     H5PT_READDATA_I8 )
#define h5pt_readdata F77NAME (      \
     h5pt_readdata_,   \
     H5PT_READDATA )

/* Writing attributes */
#define h5pt_writefileattrib_r8 F77NAME (    \
     h5pt_writefileattrib_r8_, \
     H5PT_WRITEFILEATTRIB_R8 )
#define h5pt_writefileattrib_i8 F77NAME (    \
     h5pt_writefileattrib_i8_, \
     H5PT_WRITEFILEATTRIB_I8 )
#define h5pt_writefileattrib_string F77NAME (    \
     h5pt_writefileattrib_string_, \
     H5PT_writefileattrib_string )
#define h5pt_writestepattrib_r8 F77NAME (    \
     h5pt_writestepattrib_r8_, \
     H5PT_WRITESTEPATTRIB_R8 )
#define h5pt_writestepattrib_i8 F77NAME (    \
     h5pt_writestepattrib_i8_, \
     H5PT_WRITESTEPATTRIB_I8 )
#define h5pt_writestepattrib_string F77NAME (    \
     h5pt_writestepattrib_string_, \
     H5PT_WRITESTEPATTRIB_STRING )

/* Reading attributes */
#define h5pt_getnstepattribs F77NAME (     \
     h5pt_getnstepattribs_,  \
     H5PT_GETNSTEPATTRIBS )
#define h5pt_getnfileattribs F77NAME (     \
     h5pt_getnfileattribs_,  \
     H5PT_GETNFILEATTRIBS )
#define h5pt_getstepattribinfo F77NAME (    \
     h5pt_getstepattribinfo_, \
     H5PT_GETSTEPATTRIBINFO )
#define h5pt_getfileattribinfo F77NAME (    \
     h5pt_getfileattribinfo_, \
     H5PT_GETFILEATTRIBINFO )
#define h5pt_readstepattrib F77NAME (     \
     h5pt_readstepattrib_,  \
     H5PT_READSTEPATTRIB )
#define h5pt_readstepattrib_r8 F77NAME (    \
     h5pt_readstepattrib_r8_, \
     H5PT_READSTEPATTRIB_R8 )
#define h5pt_readstepattrib_i8 F77NAME (    \
     h5pt_readstepattrib_i8_, \
     H5PT_READSTEPATTRIB_I8 )
#define h5pt_readstepattrib_string F77NAME (    \
     h5pt_readstepattrib_string_, \
     H5PT_READSTEPATTRIB_STRING )
#define h5pt_readfileattrib F77NAME (     \
     h5pt_readfileattrib_,  \
     H5PT_READFILEATTRIB )
#define h5pt_readfileattrib_r8 F77NAME (    \
     h5pt_readfileattrib_r8_, \
     H5PT_READFILEATTRIB_R8 )
#define h5pt_readfileattrib_i8 F77NAME (    \
     h5pt_readfileattrib_i8_, \
     H5PT_READFILEATTRIB_I8 )
#define h5pt_readfileattrib_string F77NAME (    \
     h5pt_readfileattrib_string_, \
     H5PT_READFILEATTRIB_STRING )

/* error handling */
#define h5pt_set_verbosity_level F77NAME (    \
     h5pt_set_verbosity_level_, \
     H5PT_SET_VERBOSITY_LEVEL )

#endif

char *
_H5Part_strdupfor2c (
 const char *s,
 const ssize_t len
 ) {

 char *dup = (char*)malloc ( len + 1 );
 strncpy ( dup, s, len );
 char *p = dup + len;
 do {
  *p-- = '\0';
 } while ( *p == ' ' );
 return dup;
}

char *
_H5Part_strc2for (
 char * const str,
 const ssize_t l_str
 ) {

 size_t len = strlen ( str );
 memset ( str+len, ' ', l_str-len );

 return str;
}

/* open/close interface */
h5part_int64_t
h5pt_openr (
 const char *file_name,
 const int l_file_name
 ) {

 char *file_name2 = _H5Part_strdupfor2c ( file_name, l_file_name );

 H5PartFile* f = H5PartOpenFile ( file_name2, H5PART_READ );

 free ( file_name2 );
 return (h5part_int64_t)(size_t)f; 
}

h5part_int64_t
h5pt_openw (
 const char *file_name,
 const int l_file_name
 ) {

 char *file_name2 = _H5Part_strdupfor2c ( file_name, l_file_name );

 H5PartFile* f = H5PartOpenFile ( file_name2, H5PART_WRITE );

 free ( file_name2 );
 return (h5part_int64_t)(size_t)f; 
}

h5part_int64_t
h5pt_opena (
 const char *file_name,
 const int l_file_name
 ) {
 
 char *file_name2 = _H5Part_strdupfor2c ( file_name, l_file_name );

 H5PartFile* f = H5PartOpenFile ( file_name2, H5PART_APPEND );

 free ( file_name2 );
 return (h5part_int64_t)(size_t)f;
}

#ifdef PARALLEL_IO
h5part_int64_t
h5pt_openr_par (
 const char *file_name,
 MPI_Comm *comm,
 const int l_file_name
 ) {

 char *file_name2 = _H5Part_strdupfor2c ( file_name, l_file_name );

 H5PartFile* f = H5PartOpenFileParallel (
  file_name2, H5PART_READ, *comm );

 free ( file_name2 );
 return (h5part_int64_t)(size_t)f; 
}

h5part_int64_t
h5pt_openw_par (
 const char *file_name,
 MPI_Comm *comm,
 const int l_file_name
 ) {

 char *file_name2 = _H5Part_strdupfor2c ( file_name, l_file_name );

 H5PartFile* f = H5PartOpenFileParallel (
  file_name2, H5PART_WRITE, *comm );

 free ( file_name2 );
 return (h5part_int64_t)(size_t)f; 
}

h5part_int64_t
h5pt_opena_par (
 const char *file_name,
 MPI_Comm *comm,
 const int l_file_name
 ) {
 
 char *file_name2 = _H5Part_strdupfor2c ( file_name, l_file_name );
       
       H5PartFile* f = H5PartOpenFileParallel (
               file_name2, H5PART_APPEND, *comm );
       
       free ( file_name2 );
       return (h5part_int64_t)(size_t)f;
}
#endif

h5part_int64_t
h5pt_close (
 const h5part_int64_t *f
 ) {
 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartCloseFile ( filehandle );
}

/*==============Writing and Setting Dataset info========*/

h5part_int64_t
h5pt_readstep (
 const h5part_int64_t *f,
 const h5part_int64_t *step,
 h5part_float64_t *x,
 h5part_float64_t *y,
 h5part_float64_t *z,
 h5part_float64_t *px,
 h5part_float64_t *py,
 h5part_float64_t *pz,
 h5part_int64_t *id
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartReadParticleStep (
  filehandle,(*step)-1,x,y,z,px,py,pz,id);
}


h5part_int64_t
h5pt_setnpoints (
 const h5part_int64_t *f,
 h5part_int64_t *np
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartSetNumParticles ( filehandle, *np );
}

h5part_int64_t
h5pt_setstep (
 const h5part_int64_t *f,
 h5part_int64_t *step ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartSetStep ( filehandle, (*step)-1 );
}

h5part_int64_t
h5pt_writedata_r8 (
 const h5part_int64_t *f,
 const char *name,
 const h5part_float64_t *data,
 const int l_name ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 char *name2 = _H5Part_strdupfor2c ( name, l_name );

 h5part_int64_t herr = H5PartWriteDataFloat64 (
  filehandle, name2, data );

 free ( name2 );

 return herr;
}

h5part_int64_t
h5pt_writedata_i8 (
 const h5part_int64_t *f,
 const char *name,
 const h5part_int64_t *data,
 const int l_name ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 char *name2 = _H5Part_strdupfor2c ( name, l_name );

 h5part_int64_t herr = H5PartWriteDataInt64 (
  filehandle, name2, data );

 free ( name2 );

 return herr;
}

/*==============Reading Data Characteristics============*/

h5part_int64_t
h5pt_getnsteps (
 const h5part_int64_t *f
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartGetNumSteps ( filehandle );
}

h5part_int64_t
h5pt_getndatasets (
 const h5part_int64_t *f
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartGetNumDatasets ( filehandle );
}

h5part_int64_t
h5pt_getnpoints (
 const h5part_int64_t *f
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartGetNumParticles ( filehandle );
}

h5part_int64_t
h5pt_getdatasetname ( 
 const h5part_int64_t *f,
 const h5part_int64_t *index,
 char *name,
 const int l_name
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 h5part_int64_t herr =  H5PartGetDatasetName (
  filehandle, *index, name, l_name );

 _H5Part_strc2for ( name, l_name );
 return herr;
}

h5part_int64_t
h5pt_getnumpoints (
 const h5part_int64_t *f
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartGetNumParticles( filehandle );
}

/*=============Setting and getting views================*/

h5part_int64_t
h5pt_setview (
 const h5part_int64_t *f,
 const h5part_int64_t *start,
 const h5part_int64_t *end
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartSetView ( filehandle, *start, *end );
}

h5part_int64_t
h5pt_resetview (
 const h5part_int64_t *f
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartResetView ( filehandle );
}

h5part_int64_t
h5pt_hasview (
 const h5part_int64_t *f
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartHasView ( filehandle );
}

h5part_int64_t
h5pt_getview (
 const h5part_int64_t *f,
 h5part_int64_t *start,
 h5part_int64_t *end
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartGetView ( filehandle, start, end);
}
/*==================Reading data ============*/
h5part_int64_t
h5pt_readdata_r8 (
 const h5part_int64_t *f,
 const char *name,
 h5part_float64_t *array,
 const int l_name
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 char *name2 = _H5Part_strdupfor2c ( name, l_name );

 h5part_int64_t herr = H5PartReadDataFloat64 (
  filehandle, name2, array );

 free ( name2 );
 return herr;
}

h5part_int64_t
h5pt_readdata_i8 (
 const h5part_int64_t *f,
 const char *name,
 h5part_int64_t *array,
 const int l_name
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 char *name2 = _H5Part_strdupfor2c ( name, l_name );

 h5part_int64_t herr = H5PartReadDataInt64 (
  filehandle, name2, array );

 free ( name2 );
 return herr;
}

/*=================== Attributes ================*/

/* Writeing attributes */
h5part_int64_t
h5pt_writefileattrib_r8 (
 const h5part_int64_t *f,
 const char *attrib_name,
 const h5part_float64_t *attrib_value,
 const h5part_int64_t *attrib_nelem,
 const int l_attrib_name
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 char *attrib_name2 = _H5Part_strdupfor2c (attrib_name,l_attrib_name);
 
 h5part_int64_t herr = H5PartWriteFileAttrib (
  filehandle,
  attrib_name2, H5T_NATIVE_DOUBLE, attrib_value, *attrib_nelem );

 free ( attrib_name2 );
 return herr;
}

h5part_int64_t
h5pt_writefileattrib_i8 (
 const h5part_int64_t *f,
 const char *attrib_name,
 const h5part_int64_t *attrib_value,
 const h5part_int64_t *attrib_nelem,
 const int l_attrib_name
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 char *attrib_name2 = _H5Part_strdupfor2c (attrib_name,l_attrib_name);
 
 h5part_int64_t herr = H5PartWriteFileAttrib (
  filehandle,
  attrib_name2, H5T_NATIVE_INT64, attrib_value, *attrib_nelem );

 free ( attrib_name2 );
 return herr;
}

h5part_int64_t
h5pt_writefileattrib_string (
 const h5part_int64_t *f,
 const char *attrib_name,
 const char *attrib_value,
 const int l_attrib_name,
 const int l_attrib_value
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 char *attrib_name2 = _H5Part_strdupfor2c (attrib_name,l_attrib_name);
 char *attrib_value2= _H5Part_strdupfor2c (attrib_value,l_attrib_value);

 h5part_int64_t herr = H5PartWriteFileAttribString (
  filehandle, attrib_name2, attrib_value2 );

 free ( attrib_name2 );
 free ( attrib_value2 );
 return herr;
}

h5part_int64_t
h5pt_writestepattrib_r8 ( 
 const h5part_int64_t *f,
 const char *attrib_name,
 const h5part_float64_t *attrib_value,
 const h5part_int64_t *attrib_nelem,
 const int l_attrib_name
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 char *attrib_name2 = _H5Part_strdupfor2c ( attrib_name, l_attrib_name );
 
 h5part_int64_t herr = H5PartWriteStepAttrib (
  filehandle,
  attrib_name2, H5T_NATIVE_DOUBLE, attrib_value, *attrib_nelem );

 free ( attrib_name2 );
 return herr;
}

h5part_int64_t
h5pt_writestepattrib_i8 (
 const h5part_int64_t *f,
 const char *attrib_name,
 const h5part_int64_t *attrib_value,
 const h5part_int64_t *attrib_nelem,
 const int l_attrib_name
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 char *attrib_name2 = _H5Part_strdupfor2c ( attrib_name, l_attrib_name );

 h5part_int64_t herr = H5PartWriteStepAttrib (
  filehandle,
  attrib_name2, H5T_NATIVE_INT64, attrib_value, *attrib_nelem );

 free ( attrib_name2 );
 return herr;
}

h5part_int64_t
h5pt_writestepattrib_string (
 const h5part_int64_t *f,
 const char *attrib_name,
 const char *attrib_value,
 const int l_attrib_name,
 const int l_attrib_value
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 char *attrib_name2 = _H5Part_strdupfor2c (attrib_name,l_attrib_name);
 char *attrib_value2= _H5Part_strdupfor2c (attrib_value,l_attrib_value);

 h5part_int64_t herr = H5PartWriteStepAttribString (
  filehandle, attrib_name2, attrib_value2 );

 free ( attrib_name2 );
 free ( attrib_value2 );
 return herr;
}

/* Reading attributes ************************* */

h5part_int64_t
h5pt_getnstepattribs (
 const h5part_int64_t *f
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartGetNumStepAttribs ( filehandle );
}

h5part_int64_t
h5pt_getnfileattribs (
 const h5part_int64_t *f
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;

 return H5PartGetNumFileAttribs ( filehandle );
}

h5part_int64_t
h5pt_getstepattribinfo (
 const h5part_int64_t *f,
 const h5part_int64_t *idx,
 char *name,
 h5part_int64_t *nelem,
 const int l_name
 ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;
 h5part_int64_t type;

 h5part_int64_t herr = H5PartGetStepAttribInfo ( 
  filehandle, *idx, name, l_name, &type, nelem);

 _H5Part_strc2for( name, l_name );
 return herr;
}

h5part_int64_t
h5pt_getfileattribinfo (
 const h5part_int64_t *f,
 const h5part_int64_t *idx,
 char *name,
 h5part_int64_t *nelem,
 const int l_name ) {

 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;
 h5part_int64_t type;

 h5part_int64_t herr = H5PartGetFileAttribInfo ( 
  filehandle, *idx, name, l_name, &type, nelem);

 _H5Part_strc2for( name, l_name );
 return herr;
}

h5part_int64_t
h5pt_readstepattrib (
 const h5part_int64_t *f,
 const char *attrib_name,
 void *attrib_value,
 const int l_attrib_name
 ) {
 
 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;
 
 char * attrib_name2 = _H5Part_strdupfor2c (attrib_name,l_attrib_name);

 h5part_int64_t herr = H5PartReadStepAttrib (
  filehandle, attrib_name2, attrib_value );

 free ( attrib_name2 );
 return herr;
}

h5part_int64_t
h5pt_readstepattrib_r8 (
 const h5part_int64_t *f,
 const char *attrib_name,
 h5part_float64_t *attrib_value,
 const int l_attrib_name
 ) {
 
 return h5pt_readstepattrib (
  f, attrib_name, attrib_value, l_attrib_name );
}

h5part_int64_t
h5pt_readstepattrib_i8 (
 const h5part_int64_t *f,
 const char *attrib_name,
 h5part_int64_t *attrib_value,
 const int l_attrib_name
 ) {
 
 return h5pt_readstepattrib (
  f, attrib_name, attrib_value, l_attrib_name );
}

h5part_int64_t
h5pt_readstepattrib_string (
 const h5part_int64_t *f,
 const char *attrib_name,
 char *attrib_value,
 const int l_attrib_name,
 const int l_attrib_value
 ) {
 
 h5part_int64_t herr = h5pt_readstepattrib (
  f, attrib_name, attrib_value, l_attrib_name );

 _H5Part_strc2for ( attrib_value, l_attrib_value );
 return herr;
}


h5part_int64_t
h5pt_readfileattrib (
 const h5part_int64_t *f,
 const char *attrib_name,
 void *attrib_value,
 const int l_attrib_name
 ) {
 
 H5PartFile *filehandle = (H5PartFile*)(size_t)*f;
 
 char * attrib_name2 = _H5Part_strdupfor2c (attrib_name,l_attrib_name);

 h5part_int64_t herr = H5PartReadFileAttrib (
  filehandle, attrib_name2, attrib_value );

 free ( attrib_name2 );
 return herr;
}

h5part_int64_t
h5pt_readfileattrib_r8 (
 const h5part_int64_t *f,
 const char *attrib_name,
 h5part_float64_t *attrib_value,
 const int l_attrib_name
 ) {
 return h5pt_readfileattrib (
  f, attrib_name, attrib_value, l_attrib_name );
}

h5part_int64_t
h5pt_readfileattrib_i8 (
 const h5part_int64_t *f,
 const char *attrib_name,
 h5part_int64_t *attrib_value,
 const int l_attrib_name
 ) {
 return h5pt_readfileattrib (
  f, attrib_name, attrib_value, l_attrib_name );
}

h5part_int64_t
h5pt_readfileattrib_string (
 const h5part_int64_t *f,
 const char *attrib_name,
 char *attrib_value,
 const int l_attrib_name,
 const int l_attrib_value
 ) {
 
 h5part_int64_t herr = h5pt_readfileattrib (
  f, attrib_name, attrib_value, l_attrib_name );

 _H5Part_strc2for ( attrib_value, l_attrib_value );
 return herr;
}

h5part_int64_t
h5pt_set_verbosity_level (
 const h5part_int64_t *level
 ) {
 return H5PartSetVerbosityLevel ( *level );
}
