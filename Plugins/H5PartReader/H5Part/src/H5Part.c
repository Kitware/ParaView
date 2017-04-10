/*! \mainpage H5Part: A Portable High Performance Parallel Data Interface to HDF5

Particle based simulations of accelerator beam-lines, especially in
six dimensional phase space, generate vast amounts of data. Even
though a subset of statistical information regarding phase space or
analysis needs to be preserved, reading and writing such enormous
restart files on massively parallel supercomputing systems remains
challenging. 

H5Part consists of Particles and Block structured Fields.

Developed by:

<UL>
<LI> Andreas Adelmann (PSI) </LI>
<LI> Achim Gsell (PSI) </LI>
<LI> Benedikt Oswald (PSI) </LI>

<LI> Wes Bethel (NERSC/LBNL)</LI>
<LI> John Shalf (NERSC/LBNL)</LI>
<LI> Cristina Siegerist (NERSC/LBNL)</LI>
</UL>


Papers: 

<UL>
<LI> A. Adelmann, R.D. Ryne, C. Siegerist, J. Shalf,"From Visualization to Data Mining with Large Data Sets," <i>
<a href="http://www.sns.gov/pac05">Particle Accelerator Conference (PAC05)</a></i>, Knoxville TN., May 16-20, 2005. (LBNL-57603)
<a href="http://vis.lbl.gov/Publications/2005/FPAT082.pdf">FPAT082.pdf</a>
</LI>


<LI> A. Adelmann, R.D. Ryne, J. Shalf, C. Siegerist,"H5Part: A Portable High Performance Parallel Data Interface for Particle Simulations," <i>
<a href="http://www.sns.gov/pac05">Particle Accelerator Conference (PAC05)</a></i>, Knoxville TN., May 16-20, 2005.
<a href="http://vis.lbl.gov/Publications/2005/FPAT083.pdf">FPAT083.pdf</a>
</LI>
</UL>

For further information contact: <a href="mailto:h5part@lists.psi.ch">h5part</a>

Last modified on April 19, 2007.

*/


/*!
  \defgroup h5part_c_api H5Part C API

*/
/*!
  \ingroup h5part_c_api
  \defgroup h5part_openclose File Opening and Closing
*/
/*!
  \ingroup h5part_c_api
  \defgroup h5part_write File Writing
*/  
/*!
  \ingroup h5part_c_api
  \defgroup h5part_read  File Reading
*/  
/*!
  \ingroup h5part_c_api
  \defgroup h5part_attrib Reading and Writing Attributes
*/
/*!
  \ingroup h5part_c_api
  \defgroup h5part_errhandle Error Handling
*/
/*!
  \internal
  \defgroup h5partkernel H5Part private functions 
*/


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h> /* va_arg - System dependent ?! */
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <vtk_hdf5.h>

#ifndef _WIN32
#include <unistd.h>
#else /* _WIN32 */
#include <io.h>
#define open  _open
#define close _close
#endif /* _WIN32 */

#include "H5Part.h"
#include "H5PartTypes.h"
#include "H5PartPrivate.h"
#include "H5PartErrors.h"

/********* Private Variable Declarations *************/

static unsigned   _debug = 0;
static h5part_int64_t  _h5part_errno = H5PART_SUCCESS;
static h5part_error_handler _err_handler = H5PartReportErrorHandler;
static const char *__funcname = "NONE";

/********** Declaration of private functions ******/

static h5part_int64_t
_init(
 void
 );

static h5part_int64_t
_file_is_valid (
 const H5PartFile *f
 );

/*
  error handler for hdf5
*/
static herr_t
_h5_error_handler (
 void *
 );

/*========== File Opening/Closing ===============*/

static H5PartFile*
_H5Part_open_file (
 const char *filename, /*!< [in] The name of the data file to open. */
 unsigned flags,  /*!< [in] The access mode for the file. */
 MPI_Comm comm,  /*!< [in] MPI communicator */
 int f_parallel,  /*!< [in] 0 for serial io otherwise parallel */
  int f_collective  /*!< [in] 0 for independent io otherwise collective */
 ) {
 if ( _init() < 0 ) {
  HANDLE_H5PART_INIT_ERR;
  return NULL;
 }
 _h5part_errno = H5PART_SUCCESS;
 H5PartFile *f = NULL;

 f = (H5PartFile*) malloc( sizeof (H5PartFile) );
 if( f == NULL ) {
  HANDLE_H5PART_NOMEM_ERR;
  goto error_cleanup;
 }
 memset (f, 0, sizeof (H5PartFile));

 f->groupname_step = strdup ( H5PART_GROUPNAME_STEP );
 if( f->groupname_step == NULL ) {
  HANDLE_H5PART_NOMEM_ERR;
  goto error_cleanup;
 }
 f->stepno_width = 0;

 f->xfer_prop = f->create_prop = f->access_prop = H5P_DEFAULT;

 if ( f_parallel ) {
#ifdef H5PART_HAS_MPI
  /* for the SP2... perhaps different for linux */
  MPI_Info info = MPI_INFO_NULL;

  /* ks: IBM_large_block_io */
  char large_block_io_key[] = "IBM_largeblock_io";
  char large_block_io_value[] = "true";
  MPI_Info_create(&info);
  MPI_Info_set(info, large_block_io_key, large_block_io_value);

  if (MPI_Comm_size (comm, &f->nprocs) != MPI_SUCCESS) {
   HANDLE_MPI_COMM_SIZE_ERR;
   goto error_cleanup;
  }
  if (MPI_Comm_rank (comm, &f->myproc) != MPI_SUCCESS) {
   HANDLE_MPI_COMM_RANK_ERR;
   goto error_cleanup;
  }

  f->pnparticles =
    (h5part_int64_t*) malloc (f->nprocs * sizeof (h5part_int64_t));
  if (f->pnparticles == NULL) {
   HANDLE_H5PART_NOMEM_ERR;
   goto error_cleanup;
  }
  
  f->access_prop = H5Pcreate (H5P_FILE_ACCESS);
  if (f->access_prop < 0) {
   HANDLE_H5P_CREATE_ERR;
   goto error_cleanup;
  }
#if defined(PARALLEL_IO) && defined(H5_HAVE_PARALLEL)
  if (H5Pset_fapl_mpio (f->access_prop, comm, info) < 0) {
   HANDLE_H5P_SET_FAPL_MPIO_ERR;
   goto error_cleanup;
  }
#endif  
  /* f->create_prop = H5Pcreate(H5P_FILE_CREATE); */
  f->create_prop = H5P_DEFAULT;

    if (f_collective) {
    /* currently create_prop is empty */
    /* xfer_prop:  also used for parallel I/O, during actual writes
       rather than the access_prop which is for file creation. */
    f->xfer_prop = H5Pcreate (H5P_DATASET_XFER);
    if (f->xfer_prop < 0) {
     HANDLE_H5P_CREATE_ERR;
     goto error_cleanup;
    }
#if defined(PARALLEL_IO) && defined(H5_HAVE_PARALLEL)
    if (H5Pset_dxpl_mpio (f->xfer_prop,H5FD_MPIO_COLLECTIVE) < 0) {
     HANDLE_H5P_SET_DXPL_MPIO_ERR;
     goto error_cleanup;
    }
#endif
    }

  f->comm = comm;

  MPI_Info_free(&info);
#endif
 } else {
  f->comm = 0;
  f->nprocs = 1;
  f->myproc = 0;
  f->pnparticles = 
   (h5part_int64_t*) malloc (f->nprocs * sizeof (h5part_int64_t));
 }
 if ( flags == H5PART_READ ) {
  f->file = H5Fopen (filename, H5F_ACC_RDONLY, f->access_prop);
 }
 else if ( flags == H5PART_WRITE ){
  f->file = H5Fcreate (filename, H5F_ACC_TRUNC, f->create_prop,
         f->access_prop);
  f->empty = 1;
 }
 else if ( flags == H5PART_APPEND ) {
  int fd = open (filename, O_RDONLY, 0);
  if ( (fd == -1) && (errno == ENOENT) ) {
   f->file = H5Fcreate(filename, H5F_ACC_TRUNC,
         f->create_prop, f->access_prop);
   f->empty = 1;
  }
  else if (fd != -1) {
   close (fd);
   f->file = H5Fopen (filename, H5F_ACC_RDWR,
        f->access_prop);
   /*
     The following function call returns an error,
     if f->file < 0. But we can safely ignore this.
   */
   f->timestep = _H5Part_get_num_objects_matching_pattern(
    f->file, "/", H5G_GROUP, f->groupname_step );
   if ( f->timestep < 0 ) goto error_cleanup;
  }
 }
 else {
  HANDLE_H5PART_FILE_ACCESS_TYPE_ERR ( flags );
  goto error_cleanup;
 }

 if (f->file < 0) {
  HANDLE_H5F_OPEN_ERR ( filename, flags );
  goto error_cleanup;
 }
 f->mode = flags;
 f->timegroup = -1;
 f->shape = 0;
 f->diskshape = H5S_ALL;
 f->memshape = H5S_ALL;
 f->viewstart = -1;
 f->viewend = -1;

 _H5Part_print_debug (
  "Proc[%d]: Opened file \"%s\" val=%lld",
  f->myproc,
  filename,
  (long long)(size_t)f );

 return f;

 error_cleanup:
 if (f != NULL ) {
  if (f->groupname_step) {
   free (f->groupname_step);
  }
  if (f->pnparticles != NULL) {
   free (f->pnparticles);
  }
  free (f);
 }
 return NULL;
}

#ifdef PARALLEL_IO
/*!
  \ingroup h5part_openclose

  Opens file with specified filename. 

  If you open with flag \c H5PART_WRITE, it will truncate any
  file with the specified filename and start writing to it. If 
  you open with \c H5PART_APPEND, then you can append new timesteps.
  If you open with \c H5PART_READ, then it will open the file
  readonly.

  The typical extension for these files is \c .h5.
  
  H5PartFile should be treated as an essentially opaque
  datastructure.  It acts as the file handle, but internally
  it maintains several key state variables associated with 
  the file.

  \return File handle or \c NULL
 */
H5PartFile*
H5PartOpenFileParallel (
 const char *filename, /*!< [in] The name of the data file to open. */
 unsigned flags,  /*!< [in] The access mode for the file. */
 MPI_Comm comm  /*!< [in] MPI communicator */
) {
 int f_parallel   = 1; /* parallel i/o */
 int f_collective = 1; /* collective i/o */

 return _H5Part_open_file ( filename, flags, comm, f_parallel, f_collective );
}

H5PartFile*
H5PartOpenFileParallelIndependent (
 const char *filename, /*!< [in] The name of the data file to open. */
 unsigned flags,  /*!< [in] The access mode for the file. */
 MPI_Comm comm  /*!< [in] MPI communicator */
) {
 int f_parallel   = 1; /* parallel i/o */
 int f_collective = 0; /* collective i/o */

 return _H5Part_open_file ( filename, flags, comm, f_parallel, f_collective );
}
#endif

/*!
  \ingroup  h5part_openclose

  Opens file with specified filename. 

  If you open with flag \c H5PART_WRITE, it will truncate any
  file with the specified filename and start writing to it. If 
  you open with \c H5PART_APPEND, then you can append new timesteps.
  If you open with \c H5PART_READ, then it will open the file
  readonly.

  The typical extension for these files is \c .h5.
  
  H5PartFile should be treated as an essentially opaque
  datastructure.  It acts as the file handle, but internally
  it maintains several key state variables associated with 
  the file.

  \return File handle or \c NULL
 */

H5PartFile*
H5PartOpenFile (
 const char *filename, /*!< [in] The name of the data file to open. */
 unsigned flags  /*!< [in] The access mode for the file. */
 ) {

 SET_FNAME ( "H5PartOpenFile" );

 MPI_Comm comm = 0; /* dummy */
 int f_parallel = 0; /* serial open */
 int f_collective = 0; /* not applicable with serial io */

 return _H5Part_open_file ( filename, flags, comm, f_parallel, f_collective );
}

/*!
  Checks if a file was successfully opened.

  \return \c H5PART_SUCCESS or error code
 */
static h5part_int64_t
_file_is_valid (
 const H5PartFile *f /*!< filehandle  to check validity of */
 ) {

 if( f == NULL )
  return H5PART_ERR_BADFD;
 else if(f->file > 0)
  return H5PART_SUCCESS;
 else
  return H5PART_ERR_BADFD;
}

/*!
  \ingroup h5part_openclose

  Closes an open file.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartCloseFile (
 H5PartFile *f  /*!< [in] filehandle of the file to close */
 ) {

 SET_FNAME ( "H5PartCloseFile" );
 herr_t r = 0;
 _h5part_errno = H5PART_SUCCESS;

 CHECK_FILEHANDLE ( f );

 if ( f->block && f->close_block ) {
  (*f->close_block) ( f );
  f->block = NULL;
  f->close_block = NULL;
 }

 if( f->shape > 0 ) {
  r = H5Sclose( f->shape );
  if ( r < 0 ) HANDLE_H5S_CLOSE_ERR;
  f->shape = 0;
 }
 if( f->timegroup >= 0 ) {
  r = H5Gclose( f->timegroup );
  if ( r < 0 ) HANDLE_H5G_CLOSE_ERR;
  f->timegroup = -1;
 }
 if( f->diskshape != H5S_ALL ) {
  r = H5Sclose( f->diskshape );
  if ( r < 0 ) HANDLE_H5S_CLOSE_ERR;
  f->diskshape = 0;
 }
 if( f->xfer_prop != H5P_DEFAULT ) {
  r = H5Pclose( f->xfer_prop );
  if ( r < 0 ) HANDLE_H5P_CLOSE_ERR ( "f->xfer_prop" );
  f->xfer_prop = H5P_DEFAULT;
 }
 if( f->access_prop != H5P_DEFAULT ) {
  r = H5Pclose( f->access_prop );
  if ( r < 0 ) HANDLE_H5P_CLOSE_ERR ( "f->access_prop" );
  f->access_prop = H5P_DEFAULT;
 }  
 if( f->create_prop != H5P_DEFAULT ) {
  r = H5Pclose( f->create_prop );
  if ( r < 0 ) HANDLE_H5P_CLOSE_ERR ( "f->create_prop" );
  f->create_prop = H5P_DEFAULT;
 }
 if ( f->file ) {
  r = H5Fclose( f->file );
  if ( r < 0 ) HANDLE_H5F_CLOSE_ERR;
  f->file = 0;
 }
 if (f->groupname_step) {
  free (f->groupname_step);
 }
 if( f->pnparticles ) {
  free( f->pnparticles );
 }
 free( f );

 return _h5part_errno;
}

/*============== File Writing Functions ==================== */

h5part_int64_t
H5PartDefineStepName (
 H5PartFile *f,
 const char *name,
 const h5part_int64_t width
 ) {
 f->groupname_step = strdup ( name );
 if( f->groupname_step == NULL ) {
  return HANDLE_H5PART_NOMEM_ERR;
 }
 f->stepno_width = (int)width;
 
 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_write

  Set number of particles for current time-step.

  This function's sole purpose is to prevent 
  needless creation of new HDF5 DataSpace handles if the number of 
  particles is invariant throughout the simulation. That's its only reason 
  for existence. After you call this subroutine, all subsequent 
  operations will assume this number of particles will be written.


  \return \c H5PART_SUCCESS or error code
 */
h5part_int64_t
H5PartSetNumParticles (
 H5PartFile *f,   /*!< [in] Handle to open file */
 h5part_int64_t nparticles /*!< [in] Number of particles */
 ) {

 SET_FNAME ( "H5PartSetNumParticles" );
 int r;
#ifdef PARALLEL_IO
#ifdef HDF5V160
 hssize_t start[1];
#else
 hsize_t start[1];
#endif

 hsize_t stride[1];
 hsize_t count[1];
 hsize_t total;
 hsize_t dmax = H5S_UNLIMITED;
 register int i;
#endif

 CHECK_FILEHANDLE( f );

#ifndef PARALLEL_IO
 /*
   if we are not using parallel-IO, there is enough information
    to know that we can short circuit this routine.  However,
    for parallel IO, this is going to cause problems because
    we don't know if things have changed globally
 */
 if ( f->nparticles == (hsize_t)(nparticles)) {
  return H5PART_SUCCESS;
 }
#endif
 if ( f->diskshape != H5S_ALL ) {
  r = H5Sclose( f->diskshape );
  if ( r < 0 ) return HANDLE_H5S_CLOSE_ERR;
  f->diskshape = H5S_ALL;
 }
 if(f->memshape != H5S_ALL) {
  r = H5Sclose( f->memshape );
  if ( r < 0 ) return HANDLE_H5S_CLOSE_ERR;
  f->memshape = H5S_ALL;
 }
 if( f->shape ) {
  r = H5Sclose(f->shape);
  if ( r < 0 ) return HANDLE_H5S_CLOSE_ERR;
 }
 f->nparticles =(hsize_t) nparticles;
#ifndef PARALLEL_IO
 f->shape = H5Screate_simple (1,
         &(f->nparticles),
         NULL);
 if ( f->shape < 0 ) HANDLE_H5S_CREATE_SIMPLE_ERR ( f->nparticles );

#else /* PARALLEL_IO */
 /*
   The Gameplan here is to declare the overall size of the on-disk
   data structure the same way we do for the serial case.  But
   then we must have additional "DataSpace" structures to define
   our in-memory layout of our domain-decomposed portion of the particle
   list as well as a "selection" of a subset of the on-disk 
   data layout that will be written in parallel to mutually exclusive
   regions by all of the processors during a parallel I/O operation.
   These are f->shape, f->memshape and f->diskshape respectively.
 */

 /*
   acquire the number of particles to be written from each MPI process
 */
      if (f->nprocs==1) {
        /* serial mode f->pnparticles is NULL */
        total = nparticles;
      }
      else {
 r = MPI_Allgather (
  &nparticles, 1, MPI_LONG_LONG,
  f->pnparticles, 1, MPI_LONG_LONG,
  f->comm);
 if ( r != MPI_SUCCESS) {
  return HANDLE_MPI_ALLGATHER_ERR;
 }
 if ( f->myproc == 0 ) {
  _H5Part_print_debug ( "Particle offsets:" );
  for(i=0;i<f->nprocs;i++) 
   _H5Part_print_debug ( " np=%lld",
           (long long) f->pnparticles[i] );
 }
 /* should I create a selection here? */

 /* compute start offsets */
 stride[0] = 1;
 start[0] = 0;
 for (i=0; i<f->myproc; i++) {
  start[0] += f->pnparticles[i];
 }
 
        /* compute total nparticles */
 total = 0;
 for (i=0; i < f->nprocs; i++) {
  total += f->pnparticles[i];
 }
      }


 /* declare overall datasize */
 f->shape = H5Screate_simple (1, &total, &total);
 if (f->shape < 0) return HANDLE_H5S_CREATE_SIMPLE_ERR ( total );


 /* declare overall data size  but then will select a subset */
 f->diskshape = H5Screate_simple (1, &total, &total);
 if (f->diskshape < 0) return HANDLE_H5S_CREATE_SIMPLE_ERR ( total );

 /* declare local memory datasize */
 f->memshape = H5Screate_simple (1, &(f->nparticles), &dmax);
 if (f->memshape < 0)
  return HANDLE_H5S_CREATE_SIMPLE_ERR ( f->nparticles );

    if (f->nprocs>1) {
 count[0] = nparticles;
 r = H5Sselect_hyperslab (
  f->diskshape,
  H5S_SELECT_SET,
  start,
  stride,
  count, NULL );
 if ( r < 0 ) return HANDLE_H5S_SELECT_HYPERSLAB_ERR;
    }

 if ( f->timegroup < 0 ) {
  r = _H5Part_set_step ( f, 0 );
  if ( r < 0 ) return r;
  
 }
#endif
 return H5PART_SUCCESS;
}

static h5part_int64_t
_write_data (
 H5PartFile *f,  /*!< IN: Handle to open file */
 const char *name, /*!< IN: Name to associate array with */
 const void *array, /*!< IN: Array to commit to disk */
 const hid_t type /*!< IN: Type of data */
 ) {
 herr_t herr;
 hid_t dataset_id;

 _H5Part_print_debug ( "Create a dataset[%s] mounted on the "
         "timestep %lld",
         name, (long long)f->timestep );

 dataset_id = H5Dcreate ( 
  f->timegroup,
  name,
  type,
  f->shape,
  H5P_DEFAULT );
 if ( dataset_id < 0 )
  return HANDLE_H5D_CREATE_ERR ( name, f->timestep );

#ifdef COLLECTIVE_IO
 herr = H5Dwrite (
  dataset_id,
  type,
  f->memshape,
  f->diskshape,
  f->xfer_prop,
  array );
#else
 herr = H5Dwrite (
  dataset_id,
  type,
  f->memshape,
  f->diskshape,
  H5P_DEFAULT,
  array );
#endif

 if ( herr < 0 ) return HANDLE_H5D_WRITE_ERR ( name, f->timestep );

 herr = H5Dclose ( dataset_id );
 if ( herr < 0 ) return HANDLE_H5D_CLOSE_ERR;

 f->empty = 0;

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_write

  Write array of 64 bit floating point data to file.

  After setting the number of particles with \c H5PartSetNumParticles() and
  the current timestep using \c H5PartSetStep(), you can start writing datasets
  into the file. Each dataset has a name associated with it (chosen by the
  user) in order to facilitate later retrieval. The name of the dataset is
  specified in the parameter \c name, which must be a null-terminated string.

  There are no restrictions on naming of datasets, but it is useful to arrive
  at some common naming convention when sharing data with other groups.

  The writing routines also implicitly store the datatype of the array so that
  the array can be reconstructed properly on other systems with incompatible
  type representations.

  All data that is written after setting the timestep is associated with that
  timestep. While the number of particles can change for each timestep, you
  cannot change the number of particles in the middle of a given timestep.

  The data is committed to disk before the routine returns.

  \return \c H5PART_SUCCESS or error code
 */
h5part_int64_t
H5PartWriteDataFloat64 (
 H5PartFile *f,  /*!< [in] Handle to open file */
 const char *name, /*!< [in] Name to associate array with */
 const h5part_float64_t *array /*!< [in] Array to commit to disk */
 ) {

 SET_FNAME ( "H5PartWriteDataFloat64" );
 h5part_int64_t herr;

 CHECK_FILEHANDLE ( f );
 CHECK_WRITABLE_MODE( f );
 CHECK_TIMEGROUP( f );

 herr = _write_data ( f, name, (void*)array, H5T_NATIVE_DOUBLE );
 if ( herr < 0 ) return herr;

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_write

  Write array of 64 bit integer data to file.

  After setting the number of particles with \c H5PartSetNumParticles() and
  the current timestep using \c H5PartSetStep(), you can start writing datasets
  into the file. Each dataset has a name associated with it (chosen by the
  user) in order to facilitate later retrieval. The name of the dataset is
  specified in the parameter \c name, which must be a null-terminated string.

  There are no restrictions on naming of datasets, but it is useful to arrive
  at some common naming convention when sharing data with other groups.

  The writing routines also implicitly store the datatype of the array so that
  the array can be reconstructed properly on other systems with incompatible
  type representations.

  All data that is written after setting the timestep is associated with that
  timestep. While the number of particles can change for each timestep, you
  cannot change the number of particles in the middle of a given timestep.

  The data is committed to disk before the routine returns.

  \return \c H5PART_SUCCESS or error code
 */
h5part_int64_t
H5PartWriteDataInt64 (
 H5PartFile *f,  /*!< [in] Handle to open file */
 const char *name, /*!< [in] Name to associate array with */
 const h5part_int64_t *array /*!< [in] Array to commit to disk */
 ) {

 SET_FNAME ( "H5PartOpenWriteDataInt64" );

 h5part_int64_t herr;

 CHECK_FILEHANDLE ( f );
 CHECK_WRITABLE_MODE( f );
 CHECK_TIMEGROUP( f );

 herr = _write_data ( f, name, (void*)array, H5T_NATIVE_INT64 );
 if ( herr < 0 ) return herr;

 return H5PART_SUCCESS;
}

/********************** reading and writing attribute ************************/

/********************** private functions to handle attributes ***************/

/*!
  \ingroup h5partkernel
  @{
*/

/*!
   Normalize HDF5 type
*/
hid_t
_H5Part_normalize_h5_type (
 hid_t type
 ) {
 H5T_class_t tclass = H5Tget_class ( type );
 int size = H5Tget_size ( type );

 switch ( tclass ){
 case H5T_INTEGER:
  if ( size==8 ) {
   return H5T_NATIVE_INT64;
  }
  else if ( size==1 ) {
   return H5T_NATIVE_CHAR;
  }
  break;
 case H5T_FLOAT:
  return H5T_NATIVE_DOUBLE;
 default:
  ; /* NOP */
 }
 _H5Part_print_warn ( "Unknown type %d", (int)type );

 return -1;
}

h5part_int64_t
_H5Part_read_attrib (
 hid_t id,
 const char *attrib_name,
 void *attrib_value
 ) {

 herr_t herr;
 hid_t attrib_id;
 hid_t space_id;
 hid_t type_id;
 hid_t mytype;
 // hsize_t nelem;

 attrib_id = H5Aopen_name ( id, attrib_name );
 if ( attrib_id <= 0 ) return HANDLE_H5A_OPEN_NAME_ERR( attrib_name );

 mytype = H5Aget_type ( attrib_id );
 if ( mytype < 0 ) return HANDLE_H5A_GET_TYPE_ERR;

 space_id = H5Aget_space ( attrib_id );
 if ( space_id < 0 ) return HANDLE_H5A_GET_SPACE_ERR;

 // nelem = H5Sget_simple_extent_npoints ( space_id );
 // Never gone to happen: if ( nelem < 0 ) return HANDLE_H5S_GET_SIMPLE_EXTENT_NPOINTS_ERR;

 type_id = _H5Part_normalize_h5_type ( mytype );

 herr = H5Aread (attrib_id, type_id, attrib_value );
 if ( herr < 0 ) return HANDLE_H5A_READ_ERR;

 herr = H5Sclose ( space_id );
 if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;

 herr = H5Tclose ( mytype );
 if ( herr < 0 ) return HANDLE_H5T_CLOSE_ERR;

 herr = H5Aclose ( attrib_id );
 if ( herr < 0 ) return HANDLE_H5A_CLOSE_ERR;

 return H5PART_SUCCESS;
}

h5part_int64_t
_H5Part_write_attrib (
 hid_t id,
 const char *attrib_name,
 const hid_t attrib_type,
 const void *attrib_value,
 const hsize_t attrib_nelem
 ) {

 herr_t herr;
 hid_t space_id;
 hid_t attrib_id;

 space_id = H5Screate_simple (1, &attrib_nelem, NULL);
 if ( space_id < 0 )
  return HANDLE_H5S_CREATE_SIMPLE_ERR ( attrib_nelem );

 attrib_id = H5Acreate ( 
  id,
  attrib_name,
  attrib_type,
  space_id,
  H5P_DEFAULT );
 if ( attrib_id < 0 ) return HANDLE_H5A_CREATE_ERR ( attrib_name );

 herr = H5Awrite ( attrib_id, attrib_type, attrib_value);
 if ( herr < 0 ) return HANDLE_H5A_WRITE_ERR ( attrib_name );

 herr = H5Aclose ( attrib_id );
 if ( herr < 0 ) return HANDLE_H5A_CLOSE_ERR;

 herr = H5Sclose ( space_id );
 if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;

 return H5PART_SUCCESS;
}

h5part_int64_t
_H5Part_get_attrib_info (
 hid_t id,
 const h5part_int64_t attrib_idx,
 char *attrib_name,
 const h5part_int64_t len_attrib_name,
 h5part_int64_t *attrib_type,
 h5part_int64_t *attrib_nelem
 ) {

 herr_t herr;
 hid_t attrib_id;
 hid_t mytype;
 hid_t space_id;

 attrib_id = H5Aopen_idx ( id, (unsigned int)attrib_idx );
 if ( attrib_id < 0 ) return HANDLE_H5A_OPEN_IDX_ERR ( attrib_idx );

 if ( attrib_nelem ) {
  space_id =  H5Aget_space ( attrib_id );
  if ( space_id < 0 ) return HANDLE_H5A_GET_SPACE_ERR;

  *attrib_nelem = H5Sget_simple_extent_npoints ( space_id );
  if ( *attrib_nelem < 0 )
   return HANDLE_H5S_GET_SIMPLE_EXTENT_NPOINTS_ERR;

  herr = H5Sclose ( space_id );
  if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
 }
 if ( attrib_name ) {
  herr = H5Aget_name (
   attrib_id,
   (size_t)len_attrib_name,
   attrib_name );
  if ( herr < 0 ) return HANDLE_H5A_GET_NAME_ERR;
 }
 if ( attrib_type ) {
  mytype = H5Aget_type ( attrib_id );
  if ( mytype < 0 ) return HANDLE_H5A_GET_TYPE_ERR;

  *attrib_type = _H5Part_normalize_h5_type ( mytype );

  herr = H5Tclose ( mytype );
  if ( herr < 0 ) return HANDLE_H5T_CLOSE_ERR;
 }
 herr = H5Aclose ( attrib_id);
 if ( herr < 0 ) return HANDLE_H5A_CLOSE_ERR;

 return H5PART_SUCCESS;
}

/********************** attribute API ****************************************/

/*!
  \ingroup h5part_attrib

  Writes a string attribute bound to a file.

  This function creates a new attribute \c name with the string \c value as
  content. The attribute is bound to the file associated with the file handle 
  \c f.

  If the attribute already exists an error will be returned. There
  is currently no way to change the content of an existing attribute.

  \return \c H5PART_SUCCESS or error code   
*/
h5part_int64_t
H5PartWriteFileAttribString (
 H5PartFile *f,  /*!< [in] Handle to open file */
 const char *attrib_name,/*!< [in] Name of attribute to create */
 const char *attrib_value/*!< [in] Value of attribute */ 
 ) {

 SET_FNAME ( "H5PartWriteFileAttribString" );

 CHECK_FILEHANDLE ( f );
 CHECK_WRITABLE_MODE( f );

 hid_t group_id = H5Gopen(f->file,"/");
 if ( group_id < 0 ) return HANDLE_H5G_OPEN_ERR( "/" );

 h5part_int64_t herr = _H5Part_write_attrib (
  group_id,
  attrib_name,
  H5T_NATIVE_CHAR,
  attrib_value,
  strlen ( attrib_value ) + 1 );
 if ( herr < 0 ) return herr;

 herr = H5Gclose ( group_id );
 if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes a string attribute bound to the current time-step.

  This function creates a new attribute \c name with the string \c value as
  content. The attribute is bound to the current time step in the file given
  by the file handle \c f.

  If the attribute already exists an error will be returned. There
  is currently no way to change the content of an existing attribute.

  \return \c H5PART_SUCCESS or error code   
*/

h5part_int64_t
H5PartWriteStepAttribString (
 H5PartFile *f,  /*!< [in] Handle to open file */
 const char *attrib_name,/*!< [in] Name of attribute to create */
 const char *attrib_value/*!< [in] Value of attribute */ 
 ) {

 SET_FNAME ( "H5PartWriteStepAttribString" );

 CHECK_FILEHANDLE ( f );
 CHECK_WRITABLE_MODE( f );
 CHECK_TIMEGROUP( f );

 h5part_int64_t herr = _H5Part_write_attrib (
  f->timegroup,
  attrib_name,
  H5T_NATIVE_CHAR,
  attrib_value,
  strlen ( attrib_value ) + 1 );
 if ( herr < 0 ) return herr;

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes a attribute bound to the current time-step.

  This function creates a new attribute \c name with the string \c value as
  content. The attribute is bound to the current time step in the file given
  by the file handle \c f.

  The value of the attribute is given the parameter \c type, which must be one
  of \c H5T_NATIVE_DOUBLE, \c H5T_NATIVE_INT64 of \c H5T_NATIVE_CHAR, the array
  \c value and the number of elements \c nelem in the array.

  If the attribute already exists an error will be returned. There
  is currently no way to change the content of an existing attribute.

  \return \c H5PART_SUCCESS or error code   
*/

h5part_int64_t
H5PartWriteStepAttrib (
 H5PartFile *f,   /*!< [in] Handle to open file */
 const char *attrib_name, /*!< [in] Name of attribute */
 const h5part_int64_t attrib_type,/*!< [in] Type of value. */
 const void *attrib_value, /*!< [in] Value of attribute */ 
 const h5part_int64_t attrib_nelem/*!< [in] Number of elements */
 ){

 SET_FNAME ( "H5PartWriteStepAttrib" );

 h5part_int64_t herr;

 CHECK_FILEHANDLE ( f );
 CHECK_WRITABLE_MODE( f );
 CHECK_TIMEGROUP( f );

 herr = _H5Part_write_attrib (
  f->timegroup,
  attrib_name,
  (const hid_t)attrib_type,
  attrib_value,
  attrib_nelem );
 if ( herr < 0 ) return herr;

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Writes a attribute bound to a file.

  This function creates a new attribute \c name with the string \c value as
  content. The attribute is bound to the file file given by the file handle
  \c f.

  The value of the attribute is given the parameter \c type, which must be one
  of H5T_NATIVE_DOUBLE, H5T_NATIVE_INT64 of H5T_NATIVE_CHAR, the array \c value
  and the number of elements \c nelem in the array.

  If the attribute already exists an error will be returned. There
  is currently no way to change the content of an existing attribute.

  \return \c H5PART_SUCCESS or error code   
*/

h5part_int64_t
H5PartWriteFileAttrib (
 H5PartFile *f,   /*!< [in] Handle to open file */
 const char *attrib_name, /*!< [in] Name of attribute */
 const h5part_int64_t attrib_type,/*!< [in] Type of value. */
 const void *attrib_value, /*!< [in] Value of attribute */ 
 const h5part_int64_t attrib_nelem/*!< [in] Number of elements */
 ) {

 SET_FNAME ( "H5PartWriteFileAttrib" );

 h5part_int64_t herr;
 hid_t group_id;

 CHECK_FILEHANDLE ( f );
 CHECK_WRITABLE_MODE ( f );

 group_id = H5Gopen(f->file,"/");
 if ( group_id < 0 ) return HANDLE_H5G_OPEN_ERR( "/" );

 herr = _H5Part_write_attrib (
  group_id,
  attrib_name,
  (const hid_t)attrib_type,
  attrib_value,
  attrib_nelem );
 if ( herr < 0 ) return herr;

 herr = H5Gclose ( group_id );
 if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Gets the number of attributes bound to the current step.

  \return Number of attributes bound to current time step or error code.
*/
h5part_int64_t
H5PartGetNumStepAttribs (
 H5PartFile *f   /*!< [in] Handle to open file */
 ) {

 SET_FNAME ( "H5PartGetNumStepAttribs" );
 h5part_int64_t nattribs;

 CHECK_FILEHANDLE ( f );

 nattribs = H5Aget_num_attrs(f->timegroup);
 if ( nattribs < 0 ) HANDLE_H5A_GET_NUM_ATTRS_ERR;

 return nattribs;
}

/*!
  \ingroup h5part_attrib

  Gets the number of attributes bound to the file.

  \return Number of attributes bound to file \c f or error code.
*/
h5part_int64_t
H5PartGetNumFileAttribs (
 H5PartFile *f   /*!< [in] Handle to open file */
 ) {

 SET_FNAME ( "H5PartGetNumFileAttribs" );
 herr_t herr;
 h5part_int64_t nattribs;

 CHECK_FILEHANDLE ( f );

 hid_t group_id = H5Gopen ( f->file, "/" );
 if ( group_id < 0 ) HANDLE_H5G_OPEN_ERR ( "/" );

 nattribs = H5Aget_num_attrs ( group_id );
 if ( nattribs < 0 ) HANDLE_H5A_GET_NUM_ATTRS_ERR;

 herr = H5Gclose ( group_id );
 if ( herr < 0 ) HANDLE_H5G_CLOSE_ERR;
 return nattribs;
}

/*!
  \ingroup h5part_attrib

  Gets the name, type and number of elements of the step attribute
  specified by its index.

  This function can be used to retrieve all attributes bound to the
  current time-step by looping from \c 0 to the number of attribute
  minus one.  The number of attributes bound to the current
  time-step can be queried by calling the function
  \c H5PartGetNumStepAttribs().

  \return \c H5PART_SUCCESS or error code 
*/
h5part_int64_t
H5PartGetStepAttribInfo (
 H5PartFile *f,   /*!< [in]  Handle to open file */
 const h5part_int64_t attrib_idx,/*!< [in]  Index of attribute to
                get infos about */
 char *attrib_name,  /*!< [out] Name of attribute */
 const h5part_int64_t len_of_attrib_name,
     /*!< [in]  length of buffer \c name */
 h5part_int64_t *attrib_type, /*!< [out] Type of value. */
 h5part_int64_t *attrib_nelem /*!< [out] Number of elements */
 ) {
 
 SET_FNAME ( "H5PartGetStepAttribInfo" );
 h5part_int64_t herr;

 CHECK_FILEHANDLE( f );

 herr = _H5Part_get_attrib_info (
  f->timegroup,
  attrib_idx,
  attrib_name,
  len_of_attrib_name,
  attrib_type,
  attrib_nelem );
 if ( herr < 0 ) return herr;

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Gets the name, type and number of elements of the file attribute
  specified by its index.

  This function can be used to retrieve all attributes bound to the
  file \c f by looping from \c 0 to the number of attribute minus
  one.  The number of attributes bound to file \c f can be queried
  by calling the function \c H5PartGetNumFileAttribs().

  \return \c H5PART_SUCCESS or error code 
*/

h5part_int64_t
H5PartGetFileAttribInfo (
 H5PartFile *f,   /*!< [in]  Handle to open file */
 const h5part_int64_t attrib_idx,/*!< [in]  Index of attribute to get
                infos about */
 char *attrib_name,  /*!< [out] Name of attribute */
 const h5part_int64_t len_of_attrib_name,
     /*!< [in]  length of buffer \c name */
 h5part_int64_t *attrib_type, /*!< [out] Type of value. */
 h5part_int64_t *attrib_nelem /*!< [out] Number of elements */
 ) {

 SET_FNAME ( "H5PartGetFileAttribInfo" );
 hid_t group_id;
 h5part_int64_t herr;

 CHECK_FILEHANDLE( f );

 group_id = H5Gopen(f->file,"/");
 if ( group_id < 0 ) return HANDLE_H5G_OPEN_ERR( "/" );

 herr = _H5Part_get_attrib_info (
  group_id,
  attrib_idx,
  attrib_name,
  len_of_attrib_name,
  attrib_type,
  attrib_nelem );
 if ( herr < 0 ) return herr;

 herr = H5Gclose ( group_id );
 if ( herr < 0 ) return herr;

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Reads an attribute bound to current time-step.

  \return \c H5PART_SUCCESS or error code 
*/
h5part_int64_t
H5PartReadStepAttrib (
 H5PartFile *f,   /*!< [in]  Handle to open file */
 const char *attrib_name, /*!< [in] Name of attribute to read */
 void *attrib_value  /*!< [out] Value of attribute */
 ) {

 SET_FNAME ( "H5PartReadStepAttrib" );

 h5part_int64_t herr;

 CHECK_FILEHANDLE( f );

 herr = _H5Part_read_attrib ( f->timegroup, attrib_name, attrib_value );
 if ( herr < 0 ) return herr;

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_attrib

  Reads an attribute bound to file \c f.

  \return \c H5PART_SUCCESS or error code 
*/
h5part_int64_t
H5PartReadFileAttrib ( 
 H5PartFile *f,
 const char *attrib_name,
 void *attrib_value
 ) {

 SET_FNAME ( "H5PartReadFileAttrib" );

 hid_t group_id;
 h5part_int64_t herr;

 CHECK_FILEHANDLE( f );

 group_id = H5Gopen(f->file,"/");
 if ( group_id < 0 ) return HANDLE_H5G_OPEN_ERR( "/" );

 herr = _H5Part_read_attrib ( group_id, attrib_name, attrib_value );
 if ( herr < 0 ) return herr;

 herr = H5Gclose ( group_id );
 if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;

 return H5PART_SUCCESS;
}


/*================== File Reading Routines =================*/
/*
  H5PartSetStep:


  So you use this to random-access the file for a particular timestep.
  Failure to explicitly set the timestep on each read will leave you
  stuck on the same timestep for *all* of your reads.  That is to say
  the writes auto-advance the file pointer, but the reads do not
  (they require explicit advancing by selecting a particular timestep).
*/

h5part_int64_t
_H5Part_set_step (
 H5PartFile *f,   /*!< [in]  Handle to open file */
 const h5part_int64_t step /*!< [in]  Time-step to set. */
 ) {

 char name[128];

 sprintf (
  name,
  "%s#%0*lld",
  f->groupname_step, f->stepno_width, (long long) step );
 herr_t herr = H5Gget_objinfo( f->file, name, 1, NULL );
 if ( (f->mode != H5PART_READ) && ( herr >= 0 ) ) {
  return HANDLE_H5PART_STEP_EXISTS_ERR ( step );
 }

 if ( f->timegroup >= 0 ) {
  herr = H5Gclose ( f->timegroup );
  if ( herr < 0 ) return HANDLE_H5G_CLOSE_ERR;
 }
 f->timegroup = -1;
 f->timestep = step;

 if( f->mode == H5PART_READ ) {
  _H5Part_print_info (
   "Proc[%d]: Set step to #%lld for file %lld",
   f->myproc,
   (long long)step,
   (long long)(size_t) f );

  f->timegroup = H5Gopen ( f->file, name ); 
  if ( f->timegroup < 0 ) return HANDLE_H5G_OPEN_ERR( name );
 }
 else {
  _H5Part_print_debug (
   "Proc[%d]: Create step #%lld for file %lld", 
   f->myproc,
   (long long)step,
   (long long)(size_t) f );

  f->timegroup = H5Gcreate ( f->file, name, 0 );
  if ( f->timegroup < 0 ) return HANDLE_H5G_CREATE_ERR ( name );
 }

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_read

  Set the current time-step.

  When writing data to a file the current time step must be set first
  (even if there is only one). In write-mode this function creates a new
  time-step! You are not allowed to step to an already existing time-step.
  This prevents you from overwriting existing data. Another consequence is,
  that you \b must write all data before going to the next time-step.

  In read-mode you can use this function to random-access the file for a
  particular timestep.

  \return \c H5PART_SUCCESS or error code 
*/
h5part_int64_t
H5PartSetStep (
 H5PartFile *f,   /*!< [in]  Handle to open file */
 const h5part_int64_t step /*!< [in]  Time-step to set. */
 ) {

 SET_FNAME ( "H5PartSetStep" );

 CHECK_FILEHANDLE ( f );

 return _H5Part_set_step ( f, step );
}

/********************** query file structure *********************************/

/*!
  \ingroup h5part_kernel

  Iterator for \c H5Giterate().
*/
herr_t
_H5Part_iteration_operator (
 hid_t group_id,  /*!< [in]  group id */
 const char *member_name,/*!< [in]  group name */
 void *operator_data /*!< [in,out] data passed to the iterator */
 ) {

 struct _iter_op_data *data = (struct _iter_op_data*)operator_data;
 herr_t herr;
 H5G_stat_t objinfo;

 if ( data->type != H5G_UNKNOWN ) {
  herr = H5Gget_objinfo ( group_id, member_name, 1, &objinfo );
  if ( herr < 0 ) return (herr_t)HANDLE_H5G_GET_OBJINFO_ERR ( member_name );

  if ( objinfo.type != data->type )
   return 0;/* don't count, continue iteration */
 }

 if ( data->name && (data->stop_idx == data->count) ) {
  memset ( data->name, 0, data->len );
  strncpy ( data->name, member_name, data->len-1 );
  
  return 1; /* stop iteration */
 }
 /*
   count only if pattern is NULL or member name matches
 */
 if ( !data->pattern ||
      (strncmp (member_name, data->pattern, strlen(data->pattern)) == 0)
       ) {
  data->count++;
 }
 return 0;  /* continue iteration */
}

/*!
  \ingroup h5part_kernel

  Iterator for \c H5Giterate().
*/
h5part_int64_t
_H5Part_get_num_objects (
 hid_t group_id,
 const char *group_name,
 const hid_t type
 ) {

 return _H5Part_get_num_objects_matching_pattern (
  group_id,
  group_name,
  type,
  NULL );
}

/*!
  \ingroup h5part_kernel

  Iterator for \c H5Giterate().
*/
h5part_int64_t
_H5Part_get_num_objects_matching_pattern (
 hid_t group_id,
 const char *group_name,
 const hid_t type,
 char * const pattern
 ) {

 h5part_int64_t herr;
 int idx = 0;
 struct _iter_op_data data;

 memset ( &data, 0, sizeof ( data ) );
 data.type = type;
 data.pattern = pattern;

 herr = H5Giterate ( group_id, group_name, &idx,
       _H5Part_iteration_operator, &data );
 if ( herr < 0 ) return herr;
 
 return data.count;
}

/*!
  \ingroup h5part_kernel

  Iterator for \c H5Giterate().
*/
h5part_int64_t
_H5Part_get_object_name (
 hid_t group_id,
 const char *group_name,
 const hid_t type,
 const h5part_int64_t idx,
 char *obj_name,
 const h5part_int64_t len_obj_name
 ) {

 herr_t herr;
 struct _iter_op_data data;
 int iterator_idx = 0;

 memset ( &data, 0, sizeof ( data ) );
 data.stop_idx = (hid_t)idx;
 data.type = type;
 data.name = obj_name;
 data.len = (size_t)len_obj_name;

 herr = H5Giterate ( group_id, group_name, &iterator_idx,
       _H5Part_iteration_operator,
       &data );
 if ( herr < 0 ) return (h5part_int64_t)herr;

 if ( herr == 0 ) HANDLE_H5PART_NOENTRY_ERR( group_name,
          type, idx );

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_read

  Query whether a particular step already exists in the file
  \c f.

  It works for both reading and writing of files

  \return      true or false
*/
h5part_int64_t
H5PartHasStep (
 H5PartFile *f,  /*!< [in]  Handle to open file */
 h5part_int64_t step /*!< [in]  Step number to query */
 ) {
  
 SET_FNAME ( "H5PartHasStep" );

 CHECK_FILEHANDLE( f );

 char name[128];
 sprintf ( name, "%s#%0*lld", f->groupname_step, f->stepno_width, (long long) step );
 herr_t herr = H5Gget_objinfo( f->file, name, 1, NULL );

 return ( herr >= 0 );
}


/*!
  \ingroup h5part_read

  Get the number of time-steps that are currently stored in the file
  \c f.

  It works for both reading and writing of files, but is probably
  only typically used when you are reading.

  \return number of time-steps or error code
*/
h5part_int64_t
H5PartGetNumSteps (
 H5PartFile *f   /*!< [in]  Handle to open file */
 ) {

 SET_FNAME ( "H5PartGetNumSteps" );

 CHECK_FILEHANDLE( f );

 return _H5Part_get_num_objects_matching_pattern (
  f->file,
  "/",
  H5G_UNKNOWN,
  f->groupname_step );
}

/*!
  \ingroup h5part_read

  Get the number of datasets that are stored at the current time-step.

  \return number of datasets in current timestep or error code
*/

h5part_int64_t
H5PartGetNumDatasets (
 H5PartFile *f   /*!< [in]  Handle to open file */
 ) {

 SET_FNAME ( "H5PartGetNumDatasets" );

 char stepname[128];

 CHECK_FILEHANDLE( f );

 sprintf (
  stepname,
  "%s#%0*lld",
  f->groupname_step, f->stepno_width, (long long) f->timestep );

 return _H5Part_get_num_objects ( f->file, stepname, H5G_DATASET );
}

/*!
  \ingroup h5part_read

  This reads the name of a dataset specified by it's index in the current
  time-step.

  If the number of datasets is \c n, the range of \c _index is \c 0 to \c n-1.

  \result \c H5PART_SUCCESS
*/
h5part_int64_t
H5PartGetDatasetName (
 H5PartFile *f,   /*!< [in]  Handle to open file */
 const h5part_int64_t idx, /*!< [in]  Index of the dataset */
 char *name,   /*!< [out] Name of dataset */
 const h5part_int64_t len_of_name/*!< [in]  Size of buffer \c name */
 ) {

 SET_FNAME ( "H5PartGetDatasetName" );

 char stepname[128];

 CHECK_FILEHANDLE ( f );
 CHECK_TIMEGROUP ( f );

 sprintf (
  stepname,
  "%s#%0*lld",
  f->groupname_step, f->stepno_width, (long long) f->timestep );

 return _H5Part_get_object_name (
  f->file,
  stepname,
  H5G_DATASET,
  idx,
  name,
  len_of_name );
}

/*!
  \ingroup h5part_read

  Gets the name, type and number of elements of a dataset specified by it's
  index in the current time-step.

  Type is one of \c H5T_NATIVE_DOUBLE or \c H5T_NATIVE_INT64.

  \return \c H5PART_SUCCESS
*/
h5part_int64_t
H5PartGetDatasetInfo (
 H5PartFile *f,  /*!< [in]  Handle to open file */
 const h5part_int64_t idx,/*!< [in]  Index of the dataset */
 char *dataset_name, /*!< [out] Name of dataset */
 const h5part_int64_t len_dataset_name,
    /*!< [in]  Size of buffer \c dataset_name */
 h5part_int64_t *type, /*!< [out] Type of data in dataset */
 h5part_int64_t *nelem /*!< [out] Number of elements. */
 ) {

 SET_FNAME ( "H5PartGetDatasetInfo" );

 h5part_int64_t herr;
 hid_t dataset_id;
 hid_t mytype;
 char step_name[128];

 CHECK_FILEHANDLE ( f );
 CHECK_TIMEGROUP ( f );

 sprintf (
  step_name,
  "%s#%0*lld",
  f->groupname_step, f->stepno_width, (long long) f->timestep );

 herr = _H5Part_get_object_name (
  f->file,
  step_name,
  H5G_DATASET,
  idx,
  dataset_name,
  len_dataset_name );
 if ( herr < 0 ) return herr;

 *nelem = _H5Part_get_num_particles ( f );
 if ( *nelem < 0 ) return *nelem;

 dataset_id = H5Dopen ( f->timegroup, dataset_name );
 if ( dataset_id < 0 ) HANDLE_H5D_OPEN_ERR ( dataset_name );

 mytype = H5Dget_type ( dataset_id );
 if ( mytype < 0 ) HANDLE_H5D_GET_TYPE_ERR;

 if(type)
  *type = (h5part_int64_t) _H5Part_normalize_h5_type ( mytype );

 herr = H5Tclose(mytype);
 if ( herr < 0 ) HANDLE_H5T_CLOSE_ERR;

 herr = H5Dclose(dataset_id);
 if ( herr < 0 ) HANDLE_H5D_CLOSE_ERR;

 return H5PART_SUCCESS;
}

static hid_t
_get_diskshape_for_reading (
 H5PartFile *f,
 hid_t dataset
 ) {

 herr_t r;

 hid_t space = H5Dget_space(dataset);
 if ( space < 0 ) return (hid_t)HANDLE_H5D_GET_SPACE_ERR;

 if ( H5PartHasView(f) ){ 
  hsize_t stride;
  hsize_t count;
#ifdef HDF5V160
  hssize_t start;
#else
  hsize_t start;
#endif
  _H5Part_print_debug ( "Selection is available" );

  /* so, is this selection inclusive or exclusive? */
  start = f->viewstart;
  count = f->viewend - f->viewstart; /* to be inclusive */
  stride=1;

  /* now we select a subset */
  if ( f->diskshape > 0 ) {
   r = H5Sselect_hyperslab (
    f->diskshape, H5S_SELECT_SET,
    &start, &stride, &count, NULL);
   if ( r < 0 ) return (hid_t)HANDLE_H5S_SELECT_HYPERSLAB_ERR;
  }
  /* now we select a subset */
  r = H5Sselect_hyperslab (
   space,H5S_SELECT_SET,
   &start, &stride, &count, NULL );
  if ( r < 0 ) return (hid_t)HANDLE_H5S_SELECT_HYPERSLAB_ERR;

  _H5Part_print_debug (
   "Selection: range=%d:%d, npoints=%d s=%d",
   (int)f->viewstart,(int)f->viewend,
   (int)H5Sget_simple_extent_npoints(space),
   (int)H5Sget_select_npoints(space) );
 } else {
  _H5Part_print_debug ( "Selection" );
 }
 return space;
}

static hid_t
_get_memshape_for_reading (
 H5PartFile *f,
 hid_t dataset
 ) {

 if(H5PartHasView(f)) {
  hsize_t dmax=H5S_UNLIMITED;
  hsize_t len = f->viewend - f->viewstart;
  hid_t r = H5Screate_simple(1,&len,&dmax);
  if ( r < 0 ) return (hid_t)HANDLE_H5S_CREATE_SIMPLE_ERR ( len );
  return r;
 }
 else {
  return H5S_ALL;
 }
}

h5part_int64_t
_H5Part_get_num_particles (
 H5PartFile *f   /*!< [in]  Handle to open file */
 ) {

 h5part_int64_t herr;
 hid_t space_id;
 hid_t dataset_id;
 char dataset_name[128];
 char step_name[128];
 hsize_t nparticles;

 /* Get first dataset in current time-step */
 sprintf (
  step_name,
  "%s#%0*lld",
  f->groupname_step, f->stepno_width, (long long) f->timestep );

 herr = _H5Part_get_object_name (
  f->file,
  step_name,
  H5G_DATASET,
  0,
  dataset_name, sizeof (dataset_name) );
 if ( herr < 0 ) return herr;

 dataset_id = H5Dopen ( f->timegroup, dataset_name );
 if ( dataset_id < 0 ) 
  return HANDLE_H5D_OPEN_ERR ( dataset_name );

 space_id = _get_diskshape_for_reading ( f, dataset_id );
 if ( space_id < 0 ) return (h5part_int64_t)space_id;

 if ( H5PartHasView ( f ) ) {
  nparticles = H5Sget_select_npoints ( space_id );
  // Never gone to happen: if ( nparticles < 0 ) return HANDLE_H5S_GET_SELECT_NPOINTS_ERR;
 }
 else {
  nparticles = H5Sget_simple_extent_npoints ( space_id );
  // Never gone to happen: if ( nparticles < 0 )
  // Never gone to happen:  return HANDLE_H5S_GET_SIMPLE_EXTENT_NPOINTS_ERR;
 }
 if ( space_id != H5S_ALL ) {
  herr = H5Sclose ( space_id );
  if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
 }
 herr = H5Dclose ( dataset_id );
 if ( herr < 0 ) return HANDLE_H5D_CLOSE_ERR;

 return (h5part_int64_t) nparticles;
}

/*!
  \ingroup h5part_read

  This gets the number of particles stored in the current timestep. 
  It will arbitrarily select a time-step if you haven't already set
  the timestep with \c H5PartSetStep().

  \return number of particles in current timestep or an error
  code.
 */
h5part_int64_t
H5PartGetNumParticles (
 H5PartFile *f   /*!< [in]  Handle to open file */
 ) {

 SET_FNAME ( "H5PartGetNumParticles" );

 CHECK_FILEHANDLE( f );

 if ( f->timegroup < 0 ) {
  h5part_int64_t herr = _H5Part_set_step ( f, 0 );
  if ( herr < 0 ) return herr;
 }

 return _H5Part_get_num_particles ( f );
}

static h5part_int64_t
_reset_view (
  H5PartFile *f   /*!< [in]  Handle to open file */
 ) {      

 herr_t herr = 0;

 f->viewstart = -1;
 f->viewend = -1;
 if ( f->shape != 0 ){
  herr = H5Sclose(f->shape);
  if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
  f->shape=0;
 }
 if(f->diskshape!=0 && f->diskshape!=H5S_ALL){
  herr = H5Sclose(f->diskshape);
  if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
  f->diskshape=H5S_ALL;
 }
 f->diskshape = H5S_ALL;
 if(f->memshape!=0 && f->memshape!=H5S_ALL){
  herr = H5Sclose ( f->memshape );
  if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
  f->memshape=H5S_ALL;
 }
 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_read
*/
h5part_int64_t
H5PartResetView (
  H5PartFile *f   /*!< [in]  Handle to open file */
 ) {
 SET_FNAME ( "H5PartResetView" );

 CHECK_FILEHANDLE( f );
 CHECK_READONLY_MODE ( f );

 return _reset_view ( f );
}

/*!
  \ingroup h5part_read
*/
h5part_int64_t
H5PartHasView (
  H5PartFile *f   /*!< [in]  Handle to open file */
 ) {
 SET_FNAME ( "H5PartResetView" );

 CHECK_FILEHANDLE( f );
 CHECK_READONLY_MODE ( f );

 return  ( f->viewstart >= 0 ) && ( f->viewend >= 0 );
}

static h5part_int64_t
_set_view (
 H5PartFile *f,   /*!< [in]  Handle to open file */
 h5part_int64_t start,  /*!< [in]  Start particle */
 h5part_int64_t end  /*!< [in]  End particle */
 ) {
 h5part_int64_t herr = 0;
 hsize_t total;
 hsize_t stride = 1;
 hsize_t dmax = H5S_UNLIMITED;

 _H5Part_print_debug (
  "Set view (%lld,%lld).",
  (long long)start,(long long)end);

 herr = _reset_view ( f );
 if ( herr < 0 ) return herr;

 if ( start == -1 && end == -1 ) return H5PART_SUCCESS;

 /*
   View has been reset so H5PartGetNumParticles will tell
   us the total number of particles.

   For now, we interpret start=-1 to mean 0 and 
   end==-1 to mean end of file
 */
 total = (hsize_t) _H5Part_get_num_particles ( f );
 // Never gone to happen: if ( total < 0 ) return HANDLE_H5PART_GET_NUM_PARTICLES_ERR ( total );

 if ( start == -1 ) start = 0;
 if ( end == -1 )   end = total;

 _H5Part_print_debug ( "Total nparticles=%lld", (long long)total );

 /* so, is this selection inclusive or exclusive? 
    it appears to be inclusive for both ends of the range.
 */
 if ( end < start ) {
  _H5Part_print_warn (
   "Nonfatal error. "
   "End of view (%lld) is less than start (%lld).",
   (long long)end, (long long)start );
  end = start; /* ensure that we don't have a range error */
 }
 /* setting up the new view */
 f->viewstart =  start;
 f->viewend =    end;
 f->nparticles = end - start + 1;
 
 /* declare overall datasize */
 f->shape = H5Screate_simple ( 1, &total, &total );
 if ( f->shape < 0 )
  return HANDLE_H5S_CREATE_SIMPLE_ERR ( total );

 /* declare overall data size  but then will select a subset */
 f->diskshape= H5Screate_simple ( 1, &total, &total );
 if ( f->diskshape < 0 )
  return HANDLE_H5S_CREATE_SIMPLE_ERR ( total );

 /* declare local memory datasize */
 f->memshape = H5Screate_simple(1,&(f->nparticles),&dmax);
 if ( f->memshape < 0 )
  return HANDLE_H5S_CREATE_SIMPLE_ERR ( f->nparticles );

 herr = H5Sselect_hyperslab ( 
  f->diskshape,
  H5S_SELECT_SET,
  (hsize_t*)&start,
  &stride,
  &total,
  NULL );
 if ( herr < 0 ) return HANDLE_H5S_SELECT_HYPERSLAB_ERR;

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_read

  For parallel I/O or for subsetting operations on the datafile, the
  \c H5PartSetView() function allows you to define a subset of the total
  particle dataset to read.  The concept of "view" works for both serial
  and for parallel I/O.  The "view" will remain in effect until a new view
  is set, or the number of particles in a dataset changes, or the view is
  "unset" by calling \c H5PartSetView(file,-1,-1);

  Before you set a view, the \c H5PartGetNumParticles() will return the
  total number of particles in the current time-step (even for the parallel
  reads).  However, after you set a view, it will return the number of
  particles contained in the view.

  The range is inclusive (the start and the end index).

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartSetView (
 H5PartFile *f,   /*!< [in]  Handle to open file */
 const h5part_int64_t start, /*!< [in]  Start particle */
 const h5part_int64_t end /*!< [in]  End particle */
 ) {

 SET_FNAME ( "H5PartSetView" );

 CHECK_FILEHANDLE( f );
 CHECK_READONLY_MODE ( f );

 if ( f->timegroup < 0 ) {
  h5part_int64_t herr = _H5Part_set_step ( f, 0 );
  if ( herr < 0 ) return herr;
 }

 return _set_view ( f, start, end );
}

/*!
  \ingroup h5part_read

   Allows you to query the current view. Start and End
   will be \c -1 if there is no current view established.
   Use \c H5PartHasView() to see if the view is smaller than the
   total dataset.

   \return       the number of elements in the view 
*/
h5part_int64_t
H5PartGetView (
 H5PartFile *f,   /*!< [in]  Handle to open file */
 h5part_int64_t *start,  /*!< [out]  Start particle */
 h5part_int64_t *end  /*!< [out]  End particle */
 ) {

 SET_FNAME ( "H5PartGetView" );

 CHECK_FILEHANDLE( f );

 if ( f->timegroup < 0 ) {
  h5part_int64_t herr = _H5Part_set_step ( f, 0 );
  if ( herr < 0 ) return herr;
 }

 h5part_int64_t viewstart = 0;
 h5part_int64_t viewend = 0;

 if ( f->viewstart >= 0 )
  viewstart = f->viewstart;

 if ( f->viewend >= 0 ) {
  viewend = f->viewend;
 }
 else {
  viewend = _H5Part_get_num_particles ( f );
  if ( viewend < 0 )
   return HANDLE_H5PART_GET_NUM_PARTICLES_ERR ( viewend );
 }

 if ( start ) *start = viewstart;
 if ( end )   *end = viewend;

 return viewend - viewstart;
}

/*!
  \ingroup h5part_read

  If it is too tedious to manually set the start and end coordinates
  for a view, the \c H5SetCanonicalView() will automatically select an
  appropriate domain decomposition of the data arrays for the degree
  of parallelism and set the "view" accordingly.

  \return  H5PART_SUCCESS or error code
*/
/*
  \note
  There is a bug in this function:
  If (NumParticles % f->nprocs) != 0  then
  the last  (NumParticles % f->nprocs) particles are not handled!
*/

h5part_int64_t
H5PartSetCanonicalView (
 H5PartFile *f   /*!< [in]  Handle to open file */
 ) {

 SET_FNAME ( "H5PartSetCanonicalView" );

 h5part_int64_t herr;

 CHECK_FILEHANDLE( f );
 CHECK_READONLY_MODE ( f )

 herr = _reset_view ( f );
 if ( herr < 0 ) return HANDLE_H5PART_SET_VIEW_ERR( herr, -1, -1 );

#ifdef PARALLEL_IO
 h5part_int64_t start = 0;
 h5part_int64_t end = 0;
 h5part_int64_t n = 0;
 int i = 0;
 
 if ( f->timegroup < 0 ) {
  herr = _H5Part_set_step ( f, 0 );
  if ( herr < 0 ) return herr;
 }
 n = _H5Part_get_num_particles ( f );
 if ( n < 0 ) return HANDLE_H5PART_GET_NUM_PARTICLES_ERR ( n );
 /* 
    now lets query the attributes for this group to see if there
    is a 'pnparticles' group that contains the offsets for the
    processors.
 */
 if ( _H5Part_read_attrib (
       f->timegroup,
       "pnparticles", f->pnparticles ) < 0) {
  /*
    Attribute "pnparticles" is not available.  So
    subdivide the view into NP mostly equal pieces
  */
  n /= f->nprocs;
  for ( i=0; i<f->nprocs; i++ ) {
   f->pnparticles[i] = n;
  }
 }

 for ( i = 0; i < f->myproc; i++ ){
  start += f->pnparticles[i];
 }
 end = start + f->pnparticles[f->myproc] - 1;
 herr = _set_view ( f, start, end );
 if ( herr < 0 ) return HANDLE_H5PART_SET_VIEW_ERR ( herr, start, end );

#endif

 return H5PART_SUCCESS;
}

static h5part_int64_t
_read_data (
 H5PartFile *f,  /*!< [in] Handle to open file */
 const char *name, /*!< [in] Name to associate dataset with */
 void *array,  /*!< [out] Array of data */
 const hid_t type
 ) {

 herr_t herr;
 hid_t dataset_id;
 hid_t space_id;
 hid_t memspace_id;

 if ( f->timegroup < 0 ) {
  h5part_int64_t h5err = _H5Part_set_step ( f, f->timestep );
  if ( h5err < 0 ) return h5err;
 }
 dataset_id = H5Dopen ( f->timegroup, name );
 if ( dataset_id < 0 ) return HANDLE_H5D_OPEN_ERR ( name );

 space_id = _get_diskshape_for_reading ( f, dataset_id );
 if ( space_id < 0 ) return (h5part_int64_t)space_id;

 memspace_id = _get_memshape_for_reading ( f, dataset_id );
 if ( memspace_id < 0 ) return (h5part_int64_t)memspace_id;

#ifdef INDEPENDENT_IO
 herr = H5Dread (
  dataset_id,
  type,
  memspace_id,  /* shape/size of data in memory (the
        complement to disk hyperslab) */
  space_id,  /* shape/size of data on disk 
        (get hyperslab if needed) */
  H5P_DEFAULT,  /* ignore... its for parallel reads */
  array );
#else
 herr = H5Dread (
  dataset_id,
  type,
  memspace_id,  /* shape/size of data in memory (the
        complement to disk hyperslab) */
  space_id,  /* shape/size of data on disk 
        (get hyperslab if needed) */
  f->xfer_prop,  /* ignore... its for parallel reads */
  array );
#endif

 if ( herr < 0 ) return HANDLE_H5D_READ_ERR ( name, f->timestep );

 if ( space_id != H5S_ALL ) {
  herr = H5Sclose (space_id );
  if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;
 }

 if ( memspace_id != H5S_ALL )
  herr = H5Sclose ( memspace_id );
  if ( herr < 0 ) return HANDLE_H5S_CLOSE_ERR;

 herr = H5Dclose ( dataset_id );
 if ( herr < 0 ) return HANDLE_H5D_CLOSE_ERR;
 
 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_read

  Read array of 64 bit floating point data from file.

  When retrieving datasets from disk, you ask for them
  by name. There are no restrictions on naming of arrays,
  but it is useful to arrive at some common naming
  convention when sharing data with other groups.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartReadDataFloat64 (
 H5PartFile *f,  /*!< [in] Handle to open file */
 const char *name, /*!< [in] Name to associate dataset with */
 h5part_float64_t *array /*!< [out] Array of data */
 ) {

 SET_FNAME ( "H5PartReadDataFloat64" );

 h5part_int64_t herr;

 CHECK_FILEHANDLE( f );

 herr = _read_data ( f, name, array, H5T_NATIVE_DOUBLE );
 if ( herr < 0 ) return herr;

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_read

  Read array of 64 bit floating point data from file.

  When retrieving datasets from disk, you ask for them
  by name. There are no restrictions on naming of arrays,
  but it is useful to arrive at some common naming
  convention when sharing data with other groups.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartReadDataInt64 (
 H5PartFile *f,  /*!< [in] Handle to open file */
 const char *name, /*!< [in] Name to associate dataset with */
 h5part_int64_t *array /*!< [out] Array of data */
 ) {

 SET_FNAME ( "H5PartReadDataInt64" );

 h5part_int64_t herr;

 CHECK_FILEHANDLE( f );

 herr = _read_data ( f, name, array, H5T_NATIVE_INT64 );
 if ( herr < 0 ) return herr;

 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_read

  This is the mongo read function that pulls in all of the data for a
  given timestep in one shot. It also takes the timestep as an argument
  and will call \c H5PartSetStep() internally so that you don't have to 
  make that call separately.

  \note
  See also \c H5PartReadDataInt64() and \c H5PartReadDataFloat64() if you want
  to just read in one of the many datasets.

  \return \c H5PART_SUCCESS or error code
*/
h5part_int64_t
H5PartReadParticleStep (
 H5PartFile *f,  /*!< [in]  Handle to open file */
 h5part_int64_t step, /*!< [in]  Step to read */
 h5part_float64_t *x, /*!< [out] Buffer for dataset named "x" */
 h5part_float64_t *y, /*!< [out] Buffer for dataset named "y" */
 h5part_float64_t *z, /*!< [out] Buffer for dataset named "z" */
 h5part_float64_t *px, /*!< [out] Buffer for dataset named "px" */
 h5part_float64_t *py, /*!< [out] Buffer for dataset named "py" */
 h5part_float64_t *pz, /*!< [out] Buffer for dataset named "pz" */
 h5part_int64_t *id /*!< [out] Buffer for dataset named "id" */
 ) {

 SET_FNAME ( "H5PartReadParticleStep" );
 h5part_int64_t herr;

 CHECK_FILEHANDLE( f );

 herr = _H5Part_set_step ( f, step );
 if ( herr < 0 ) return herr;

 herr = _read_data ( f, "x", (void*)x, H5T_NATIVE_DOUBLE );
 if ( herr < 0 ) return herr;

 herr = _read_data ( f, "y", (void*)y, H5T_NATIVE_DOUBLE );
 if ( herr < 0 ) return herr;

 herr = _read_data ( f, "z", (void*)z, H5T_NATIVE_DOUBLE );
 if ( herr < 0 ) return herr;

 herr = _read_data ( f, "px", (void*)px, H5T_NATIVE_DOUBLE );
 if ( herr < 0 ) return herr;

 herr = _read_data ( f, "py", (void*)py, H5T_NATIVE_DOUBLE );
 if ( herr < 0 ) return herr;

 herr = _read_data ( f, "pz", (void*)pz, H5T_NATIVE_DOUBLE );
 if ( herr < 0 ) return herr;

 herr = _read_data ( f, "id", (void*)id, H5T_NATIVE_INT64 );
 if ( herr < 0 ) return herr;

 return H5PART_SUCCESS;
}

/****************** error handling ******************/

/*!
  \ingroup h5part_errhandle

  Set verbosity level to \c level.

  \return \c H5PART_SUCCESS
*/
h5part_int64_t
H5PartSetVerbosityLevel (
 h5part_int64_t level
 ) {

 _debug = (unsigned int)level;
 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_errhandle

  Set error handler to \c handler.

  \return \c H5PART_SUCCESS
*/
h5part_int64_t
H5PartSetErrorHandler (
 h5part_error_handler handler
 ) {
 _err_handler = handler;
 return H5PART_SUCCESS;
}

/*!
  \ingroup h5part_errhandle

  Get current error handler.

  \return Pointer to error handler.
*/
h5part_error_handler
H5PartGetErrorHandler (
 void
 ) {
 return _err_handler;
}

/*!
  \ingroup h5part_errhandle

  Get last error code.

  \return error code
*/
h5part_int64_t
H5PartGetErrno (
 void
 ) {
 return _h5part_errno;
}

/*!
  \ingroup h5part_errhandle

  This is the H5Part default error handler.  If an error occures, an
  error message will be printed and an error number will be returned.

  \return value given in \c eno
*/
h5part_int64_t
H5PartReportErrorHandler (
 const char *funcname,
 const h5part_int64_t eno,
 const char *fmt,
 ...
 ) {

 _h5part_errno = eno;
 if ( _debug > 0 ) {
  va_list ap;
  va_start ( ap, fmt );
  _H5Part_vprint_error ( fmt, ap );
  va_end ( ap );
 }
 return _h5part_errno;
}

/*!
  \ingroup h5part_errhandle

  If an error occures, an error message will be printed and the
  program exists with the error code given in \c eno.
*/
h5part_int64_t
H5PartAbortErrorHandler (
 const char *funcname,
 const h5part_int64_t eno,
 const char *fmt,
 ...
 ) {

 _h5part_errno = eno;
 if ( _debug > 0 ) {
  va_list ap;
  va_start ( ap, fmt );
  fprintf ( stderr, "%s: ", funcname );
  vfprintf ( stderr, fmt, ap );
  fprintf ( stderr, "\n" );
 }
 exit (-(int)_h5part_errno);
}

/*!
  Initialize H5Part
*/
static h5part_int64_t
_init ( void ) {
 static int __init = 0;

 herr_t r5;
 if ( ! __init ) {
  r5 = H5Eset_auto ( _h5_error_handler, NULL );
  if ( r5 < 0 ) return H5PART_ERR_INIT;
 }
 __init = 1;
 return H5PART_SUCCESS;
}
/*! @} */

static herr_t
_h5_error_handler ( void* unused ) {
 
 if ( _debug >= 5 ) {
  H5Eprint (stderr);
 }
 return 0;
}

static void
_vprint (
 FILE* f,
 const char *prefix,
 const char *fmt,
 va_list ap
 ) {
 char *fmt2 = (char*)malloc( strlen ( prefix ) +strlen ( fmt ) + strlen ( __funcname ) + 16 );
 if ( fmt2 == NULL ) return;
 sprintf ( fmt2, "%s: %s: %s\n", prefix, __funcname, fmt ); 
 vfprintf ( stderr, fmt2, ap );
 free ( fmt2 );
}

void
_H5Part_vprint_error (
 const char *fmt,
 va_list ap
 ) {

 if ( _debug < 1 ) return;
 _vprint ( stderr, "E", fmt, ap );
}

void
_H5Part_print_error (
 const char *fmt,
 ...
 ) {

 va_list ap;
 va_start ( ap, fmt );
 _H5Part_vprint_error ( fmt, ap );
 va_end ( ap );
}

void
_H5Part_vprint_warn (
 const char *fmt,
 va_list ap
 ) {

 if ( _debug < 2 ) return;
 _vprint ( stderr, "W", fmt, ap );
}

void
_H5Part_print_warn (
 const char *fmt,
 ...
 ) {

 va_list ap;
 va_start ( ap, fmt );
 _H5Part_vprint_warn ( fmt, ap );
 va_end ( ap );
}

void
_H5Part_vprint_info (
 const char *fmt,
 va_list ap
 ) {

 if ( _debug < 3 ) return;
 _vprint ( stdout, "I", fmt, ap );
}

void
_H5Part_print_info (
 const char *fmt,
 ...
 ) {

 va_list ap;
 va_start ( ap, fmt );
 _H5Part_vprint_info ( fmt, ap );
 va_end ( ap );
}

void
_H5Part_vprint_debug (
 const char *fmt,
 va_list ap
 ) {

 if ( _debug < 4 ) return;
 _vprint ( stdout, "D", fmt, ap );
}

void
_H5Part_print_debug (
 const char *fmt,
 ...
 ) {

 va_list ap;
 va_start ( ap, fmt );
 _H5Part_vprint_debug ( fmt, ap );
 va_end ( ap );
}

void
_H5Part_set_funcname (
 const char  *fname
 ) {
 __funcname = fname;
}

const char *
_H5Part_get_funcname (
 void
 ) {
 return __funcname;
}
