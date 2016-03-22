/*
  System dependend definitions
*/

#ifndef _H5PARTTYPES_H_
#define _H5PARTTYPES_H_

#ifdef   _WIN32
typedef __int64   int64_t;
#endif /* _WIN32 */

typedef int64_t   h5part_int64_t;
typedef double   h5part_float64_t;
typedef h5part_int64_t (*h5part_error_handler)( const char*, const h5part_int64_t, const char*,...)
#ifdef __GNUC__
__attribute__ ((format (printf, 3, 4)))
#endif
 ;

#ifndef H5PART_HAS_MPI
typedef int  MPI_Comm;
#endif

struct H5BlockFile;

/**
   \struct H5PartFile

   This is an essentially opaque datastructure that
   acts as the filehandle for all practical purposes.
   It is created by H5PartOpenFile<xx>() and destroyed by
   H5PartCloseFile().  
*/
struct H5PartFile {
 hid_t file;
 char *groupname_step;
 int stepno_width;
 int empty;
       
 h5part_int64_t timestep;
 hsize_t nparticles;
 
 hid_t timegroup;
 hid_t shape;
 unsigned mode;
 hid_t xfer_prop;
 hid_t create_prop;
 hid_t access_prop;
 hid_t diskshape;
 hid_t memshape;      /* for parallel I/O (this is on-disk) H5S_ALL 
    if serial I/O */
 h5part_int64_t viewstart; /* -1 if no view is available: A "view" looks */
 h5part_int64_t viewend;   /* at a subset of the data. */
  
 /**
    the number of particles in each processor.
    With respect to the "VIEW", these numbers
    can be regarded as non-overlapping subsections
    of the particle array stored in the file.
    So they can be used to compute the offset of
    the view for each processor
 */
 h5part_int64_t *pnparticles;

 /**
    Number of processors
 */
 int nprocs;
 
 /**
    The index of the processor this process is running on.
 */
 int myproc;

 /**
    MPI comnunicator
 */
 MPI_Comm comm;

 struct H5BlockStruct *block;
 h5part_int64_t (*close_block)(struct H5PartFile *f);
};

typedef struct H5PartFile H5PartFile;

#ifdef IPL_XT3
# define SEEK_END 2 
#endif



#endif
