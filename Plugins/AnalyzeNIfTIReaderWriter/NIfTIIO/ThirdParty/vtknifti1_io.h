/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtknifti1_io.h
 
*****===================================================================*****
*****         File nifti1_io.h == Declarations for nifti1_io.c          *****
*****...................................................................*****
*****            This code is released to the public domain.            *****
*****...................................................................*****
*****  Author: Robert W Cox, SSCC/DIRP/NIMH/NIH/DHHS/USA/EARTH          *****
*****  Date:   August 2003                                              *****
*****...................................................................*****
*****  Neither the National Institutes of Health (NIH), nor any of its  *****
*****  employees imply any warranty of usefulness of this software for  *****
*****  any purpose, and do not assume any liability for damages,        *****
*****  incidental or otherwise, caused by any use of this document.     *****
*****===================================================================*****

   Modified by: Mark Jenkinson (FMRIB Centre, University of Oxford, UK)
   Date: July/August 2004 

      Mainly adding low-level IO and changing things to allow gzipped files
      to be read and written
      Full backwards compatibility should have been maintained

   Modified by: Rick Reynolds (SSCC/DIRP/NIMH, National Institutes of Health)
   Date: December 2004

      Modified and added many routines for I/O.

   Modified by: Joseph Hennessey (Center for Imaging Science, Johns Hopkins Unviversity)
   Date: March 2010

      Converted to C++

=========================================================================*/
// .NAME vtknifti1_io - vtknifti1_io
// .SECTION Description
// vtknifti1_io 
//
// .SECTION See Also
// 

#ifndef vtknifti1_io_h
#define vtknifti1_io_h

#include "vtkObject.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef DONT_INCLUDE_ANALYZE_STRUCT
#define DONT_INCLUDE_ANALYZE_STRUCT  /*** not needed herein ***/
#endif
#include "vtknifti1.h"                  /*** NIFTI-1 header specification ***/

#include "vtkznzlib.h"

/********************** Some sample data structures **************************/

typedef struct mat44
{                   /** 4x4 matrix struct **/
  float m[4][4] ;
} mat44 ;

typedef struct mat33
{                   /** 3x3 matrix struct **/
  float m[3][3] ;
} mat33 ;

/*...........................................................................*/

/*! \enum analyze_75_orient_code
 *  \brief Old-style analyze75 orientation
 *         codes.
 */
typedef enum _analyze75_orient_code {
  a75_transverse_unflipped = 0,
  a75_coronal_unflipped = 1,
  a75_sagittal_unflipped = 2,
  a75_transverse_flipped = 3,
  a75_coronal_flipped = 4,
  a75_sagittal_flipped = 5,
  a75_orient_unknown = 6
} analyze_75_orient_code;

/*! \struct nifti_image
    \brief High level data structure for open nifti datasets in the
           nifti1_io API.  Note that this structure is not part of the
           nifti1 format definition; it is used to implement one API
           for reading/writing formats in the nifti1 format.
 */
typedef struct nifti_image
{                /*!< Image storage struct **/

  int ndim ;                    /*!< last dimension greater than 1 (1..7) */
  int nx ;                      /*!< dimensions of grid array             */
  int ny ;                      /*!< dimensions of grid array             */
  int nz ;                      /*!< dimensions of grid array             */
  int nt ;                      /*!< dimensions of grid array             */
  int nu ;                      /*!< dimensions of grid array             */
  int nv ;                      /*!< dimensions of grid array             */
  int nw ;                      /*!< dimensions of grid array             */
  int dim[8] ;                  /*!< dim[0]=ndim, dim[1]=nx, etc.         */
  size_t nvox ;                    /*!< number of voxels = nx*ny*nz*...*nw   */
  int nbyper ;                  /*!< bytes per voxel, matches datatype    */
  int datatype ;                /*!< type of data in voxels: DT_* code    */

  float dx ;                    /*!< grid spacings      */
  float dy ;                    /*!< grid spacings      */
  float dz ;                    /*!< grid spacings      */
  float dt ;                    /*!< grid spacings      */
  float du ;                    /*!< grid spacings      */
  float dv ;                    /*!< grid spacings      */
  float dw ;                    /*!< grid spacings      */
  float pixdim[8] ;             /*!< pixdim[1]=dx, etc. */

  float scl_slope ;             /*!< scaling parameter - slope        */
  float scl_inter ;             /*!< scaling parameter - intercept    */

  float cal_min ;               /*!< calibration parameter, minimum   */
  float cal_max ;               /*!< calibration parameter, maximum   */

  int qform_code ;              /*!< codes for (x,y,z) space meaning  */
  int sform_code ;              /*!< codes for (x,y,z) space meaning  */

  int freq_dim  ;               /*!< indexes (1,2,3, or 0) for MRI    */
  int phase_dim ;               /*!< directions in dim[]/pixdim[]     */
  int slice_dim ;               /*!< directions in dim[]/pixdim[]     */

  int   slice_code  ;           /*!< code for slice timing pattern    */
  int   slice_start ;           /*!< index for start of slices        */
  int   slice_end   ;           /*!< index for end of slices          */
  float slice_duration ;        /*!< time between individual slices   */

  /*! quaternion transform parameters
    [when writing a dataset, these are used for qform, NOT qto_xyz]   */
  float quatern_b , quatern_c , quatern_d ,
        qoffset_x , qoffset_y , qoffset_z ,
        qfac      ;

  mat44 qto_xyz ;               /*!< qform: transform (i,j,k) to (x,y,z) */
  mat44 qto_ijk ;               /*!< qform: transform (x,y,z) to (i,j,k) */

  mat44 sto_xyz ;               /*!< sform: transform (i,j,k) to (x,y,z) */
  mat44 sto_ijk ;               /*!< sform: transform (x,y,z) to (i,j,k) */

  float toffset ;               /*!< time coordinate offset */

  int xyz_units  ;              /*!< dx,dy,dz units: NIFTI_UNITS_* code  */
  int time_units ;              /*!< dt       units: NIFTI_UNITS_* code  */

  int nifti_type ;              /*!< 0==ANALYZE, 1==NIFTI-1 (1 file),
                                                 2==NIFTI-1 (2 files),
                                                 3==NIFTI-ASCII (1 file) */
  int   intent_code ;           /*!< statistic type (or something)       */
  float intent_p1 ;             /*!< intent parameters                   */
  float intent_p2 ;             /*!< intent parameters                   */
  float intent_p3 ;             /*!< intent parameters                   */
  char  intent_name[16] ;       /*!< optional description of intent data */

  char descrip[80]  ;           /*!< optional text to describe dataset   */
  char aux_file[24] ;           /*!< auxiliary filename                  */

  char *fname ;                 /*!< header filename (.hdr or .nii)         */
  char *iname ;                 /*!< image filename  (.img or .nii)         */
  int   iname_offset ;          /*!< offset into iname where data starts    */
  int   swapsize ;              /*!< swap unit in image data (might be 0)   */
  int   byteorder ;             /*!< byte order on disk (MSB_ or LSB_FIRST) */
  void *data ;                  /*!< pointer to data: nbyper*nvox bytes     */

  int                num_ext ;  /*!< number of extensions in ext_list       */
  nifti1_extension * ext_list ; /*!< array of extension structs (with data) */
  analyze_75_orient_code analyze75_orient; /*!< for old analyze files, orient */

} nifti_image ;



/* struct for return from nifti_image_read_bricks() */
typedef struct nifti_brick_list 
{
  int       nbricks;    /* the number of allocated pointers in 'bricks' */
  size_t    bsize;      /* the length of each data block, in bytes      */
  void   ** bricks;     /* array of pointers to data blocks             */
} nifti_brick_list;


/*****************************************************************************/
/*------------------ NIfTI version of ANALYZE 7.5 structure -----------------*/

/* (based on fsliolib/dbh.h, but updated for version 7.5) */

typedef struct nifti_analyze75 {
       /* header info fields - describes the header    overlap with NIfTI */
       /*                                              ------------------ */
       int sizeof_hdr;                  /* 0 + 4        same              */
       char data_type[10];              /* 4 + 10       same              */
       char db_name[18];                /* 14 + 18      same              */
       int extents;                     /* 32 + 4       same              */
       short int session_error;         /* 36 + 2       same              */
       char regular;                    /* 38 + 1       same              */
       char hkey_un0;                   /* 39 + 1                40 bytes */

       /* image dimension fields - describes image sizes */
       short int dim[8];                /* 0 + 16       same              */
       short int unused8;               /* 16 + 2       intent_p1...      */
       short int unused9;               /* 18 + 2         ...             */
       short int unused10;              /* 20 + 2       intent_p2...      */
       short int unused11;              /* 22 + 2         ...             */
       short int unused12;              /* 24 + 2       intent_p3...      */
       short int unused13;              /* 26 + 2         ...             */
       short int unused14;              /* 28 + 2       intent_code       */
       short int datatype;              /* 30 + 2       same              */
       short int bitpix;                /* 32 + 2       same              */
       short int dim_un0;               /* 34 + 2       slice_start       */
       float pixdim[8];                 /* 36 + 32      same              */

       float vox_offset;                /* 68 + 4       same              */
       float funused1;                  /* 72 + 4       scl_slope         */
       float funused2;                  /* 76 + 4       scl_inter         */
       float funused3;                  /* 80 + 4       slice_end,        */
                                                     /* slice_code,       */
                                                     /* xyzt_units        */
       float cal_max;                   /* 84 + 4       same              */
       float cal_min;                   /* 88 + 4       same              */
       float compressed;                /* 92 + 4       slice_duration    */
       float verified;                  /* 96 + 4       toffset           */
       int glmax,glmin;                 /* 100 + 8              108 bytes */

       /* data history fields - optional */
       char descrip[80];                /* 0 + 80       same              */
       char aux_file[24];               /* 80 + 24      same              */
       char orient;                     /* 104 + 1      NO GOOD OVERLAP   */
       char originator[10];             /* 105 + 10     FROM HERE DOWN... */
       char generated[10];              /* 115 + 10                       */
       char scannum[10];                /* 125 + 10                       */
       char patient_id[10];             /* 135 + 10                       */
       char exp_date[10];               /* 145 + 10                       */
       char exp_time[10];               /* 155 + 10                       */
       char hist_un0[3];                /* 165 + 3                        */
       int views;                       /* 168 + 4                        */
       int vols_added;                  /* 172 + 4                        */
       int start_field;                 /* 176 + 4                        */
       int field_skip;                  /* 180 + 4                        */
       int omax, omin;                  /* 184 + 8                        */
       int smax, smin;                  /* 192 + 8              200 bytes */
} nifti_analyze75;                                   /* total:  348 bytes */

#ifdef _NIFTI1_IO_C_

typedef struct nifti_global_options 
{
    int debug;               /*!< debug level for status reports  */
    int skip_blank_ext;      /*!< skip extender if no extensions  */
    int allow_upper_fext;    /*!< allow uppercase file extensions */
} nifti_global_options;

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4610) // struct 'X' can never be instantiated - user defined constructor required
#pragma warning(disable: 4510) // default constructor could not be generated
#pragma warning(disable: 4512) // assignment operator could not be generated
#endif
typedef struct nifti_type_ele
{
    int    type;           /* should match the NIFTI_TYPE_ #define */
    int    nbyper;         /* bytes per value, matches nifti_image */
    int    swapsize;       /* bytes per swap piece, matches nifti_image */
    char const * const name;           /* text string to match #define */
} nifti_type_ele;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#undef  LNI_FERR /* local nifti file error, to be compact and repetative */
#define LNI_FERR(func,msg,file)                                      \
            fprintf(stderr,"** ERROR (%s): %s '%s'\n",func,msg,file)

#undef  swap_2
#undef  swap_4
#define swap_2(s) nifti_swap_2bytes(1,&(s)) /* s: 2-byte short; swap in place */
#define swap_4(v) nifti_swap_4bytes(1,&(v)) /* v: 4-byte value; swap in place */

                        /***** isfinite() is a C99 macro, which is
                               present in many C implementations already *****/

#undef IS_GOOD_FLOAT
#undef FIXED_FLOAT

#ifdef isfinite       /* use isfinite() to check floats/doubles for goodness */
#  define IS_GOOD_FLOAT(x) isfinite(x)       /* check if x is a "good" float */
#  define FIXED_FLOAT(x)   (isfinite(x) ? (x) : 0)           /* fixed if bad */
#else
#  define IS_GOOD_FLOAT(x) 1                               /* don't check it */
#  define FIXED_FLOAT(x)   (x)                               /* don't fix it */
#endif

#undef  ASSIF                                 /* assign v to *p, if possible */
#define ASSIF(p,v) if( (p)!=NULL ) *(p) = (v)

#undef  MSB_FIRST
#undef  LSB_FIRST
#undef  REVERSE_ORDER
#define LSB_FIRST 1
#define MSB_FIRST 2
#define REVERSE_ORDER(x) (3-(x))    /* convert MSB_FIRST <--> LSB_FIRST */

#define LNI_MAX_NIA_EXT_LEN 100000  /* consider a longer extension invalid */

#endif  /* _NIFTI1_IO_C_ section */


class vtknifti1_io : public vtkObject
{
public:
  static vtknifti1_io *New();
  vtkTypeMacro(vtknifti1_io,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent) override;

/*****************************************************************************/
/*--------------- Prototypes of functions defined in this file --------------*/

static const char * nifti_datatype_string   ( int dt ) ;
static const char *nifti_units_string      ( int uu ) ;
static const char *nifti_intent_string     ( int ii ) ;
static const char *nifti_xform_string      ( int xx ) ;
static const char *nifti_slice_string      ( int ss ) ;
static const char *nifti_orientation_string( int ii ) ;

static int   nifti_is_inttype( int dt ) ;

static mat44 nifti_mat44_inverse( mat44 R ) ;

static mat33 nifti_mat33_inverse( mat33 R ) ;
static mat33 nifti_mat33_polar  ( mat33 A ) ;
static float nifti_mat33_rownorm( mat33 A ) ;
static float nifti_mat33_colnorm( mat33 A ) ;
static float nifti_mat33_determ ( mat33 R ) ;
static mat33 nifti_mat33_mul    ( mat33 A , mat33 B ) ;

static void  nifti_swap_2bytes ( size_t n , void *ar ) ;
static void  nifti_swap_4bytes ( size_t n , void *ar ) ;
static void  nifti_swap_8bytes ( size_t n , void *ar ) ;
static void  nifti_swap_16bytes( size_t n , void *ar ) ;
static void  nifti_swap_Nbytes ( size_t n , int siz , void *ar ) ;

static int    nifti_datatype_is_valid   (int dtype, int for_nifti);
static int    nifti_datatype_from_string(const char * name);
const char * nifti_datatype_to_string  (int dtype);

static int   nifti_get_filesize( const char *pathname ) ;
static void  swap_nifti_header ( nifti_1_header *h , int is_nifti ) ;
static void  old_swap_nifti_header( nifti_1_header *h , int is_nifti );
static int   nifti_swap_as_analyze( nifti_analyze75 *h );


/* main read/write routines */

static nifti_image *nifti_image_read_bricks(const char *hname , int nbricks,
                                     const int *blist, nifti_brick_list * NBL);
static int          nifti_image_load_bricks(nifti_image *nim , int nbricks,
                                     const int *blist, nifti_brick_list * NBL);
static void         nifti_free_NBL( nifti_brick_list * NBL );

static nifti_image *nifti_image_read    ( const char *hname , int read_data ) ;
static int          nifti_image_load    ( nifti_image *nim ) ;
static void         nifti_image_unload  ( nifti_image *nim ) ;
static void         nifti_image_free    ( nifti_image *nim ) ;

static int          nifti_read_collapsed_image( nifti_image * nim, const int dims [8],
                                         void ** data );

static int          nifti_read_subregion_image( nifti_image * nim, 
                                         int *start_index, int *region_size,
                                         void ** data );

static void         nifti_image_write   ( nifti_image * nim ) ;
static void         nifti_image_write_bricks(nifti_image * nim, 
                                      const nifti_brick_list * NBL);
static void         nifti_image_infodump( const nifti_image * nim ) ;

static void         nifti_disp_lib_hist( void ) ;     /* to display library history */
static void         nifti_disp_lib_version( void ) ;  /* to display library version */
static int          nifti_disp_matrix_orient( const char * mesg, mat44 mat );
static int          nifti_disp_type_list( int which );


static char *       nifti_image_to_ascii  ( const nifti_image * nim ) ;
static nifti_image *nifti_image_from_ascii( const char * str, int * bytes_read ) ;

static size_t       nifti_get_volsize(const nifti_image *nim) ;

/* basic file operations */
static int    nifti_set_filenames(nifti_image * nim, const char * prefix, int check,
                           int set_byte_order);
static char * nifti_makehdrname  (const char * prefix, int nifti_type, int check,
                           int comp);
static char * nifti_makeimgname  (const char * prefix, int nifti_type, int check,
                           int comp);
static int    is_nifti_file      (const char *hname);
static const char * nifti_find_file_extension(const char * name);
static int    nifti_is_complete_filename(const char* fname);
static int    nifti_validfilename(const char* fname);

static int    disp_nifti_1_header(const char * info, const nifti_1_header * hp ) ;
static void   nifti_set_debug_level( int level ) ;
static void   nifti_set_skip_blank_ext( int skip ) ;
static void   nifti_set_allow_upper_fext( int allow ) ;

static int    valid_nifti_brick_list(nifti_image * nim , int nbricks,
                              const int * blist, int disp_error);

/* znzFile operations */
static znzFile nifti_image_open(const char * hname, char * opts, nifti_image ** nim);
static znzFile nifti_image_write_hdr_img(nifti_image *nim, int write_data,
                                  const char* opts);
static znzFile nifti_image_write_hdr_img2( nifti_image *nim , int write_opts ,
               const char* opts, znzFile imgfile, const nifti_brick_list * NBL);
static size_t  nifti_read_buffer(znzFile fp, void* datatptr, size_t ntot,
                         nifti_image *nim);
static int     nifti_write_all_data(znzFile fp, nifti_image * nim,
                             const nifti_brick_list * NBL);
static size_t  nifti_write_buffer(znzFile fp, const void * buffer, size_t numbytes);
static nifti_image *nifti_read_ascii_image(znzFile fp, char *fname, int flen,
                         int read_data);
static znzFile nifti_write_ascii_image(nifti_image *nim, const nifti_brick_list * NBL,
                         const char * opts, int write_data, int leave_open);


static void nifti_datatype_sizes( int datatype , int *nbyper, int *swapsize ) ;

static void nifti_mat44_to_quatern( mat44 R ,
                             float *qb, float *qc, float *qd,
                             float *qx, float *qy, float *qz,
                             float *dx, float *dy, float *dz, float *qfac ) ;

static mat44 nifti_quatern_to_mat44( float qb, float qc, float qd,
                              float qx, float qy, float qz,
                              float dx, float dy, float dz, float qfac );

static mat44 nifti_make_orthog_mat44( float r11, float r12, float r13 ,
                               float r21, float r22, float r23 ,
                               float r31, float r32, float r33  ) ;

static int nifti_short_order(void) ;              /* CPU byte order */


/* Orientation codes that might be returned from nifti_mat44_to_orientation().*/

#define NIFTI_L2R  1    /* Left to Right         */
#define NIFTI_R2L  2    /* Right to Left         */
#define NIFTI_P2A  3    /* Posterior to Anterior */
#define NIFTI_A2P  4    /* Anterior to Posterior */
#define NIFTI_I2S  5    /* Inferior to Superior  */
#define NIFTI_S2I  6    /* Superior to Inferior  */

static void nifti_mat44_to_orientation( mat44 R , int *icod, int *jcod, int *kcod ) ;

/*--------------------- Low level IO routines ------------------------------*/

static char * nifti_findhdrname (const char* fname);
static char * nifti_findimgname (const char* fname , int nifti_type);
static int    nifti_is_gzfile   (const char* fname);

static char * nifti_makebasename(const char* fname);


/* other routines */
static nifti_1_header nifti_convert_nim2nhdr(const nifti_image* nim);
static nifti_1_header * nifti_make_new_header(const int arg_dims[], int arg_dtype);
static nifti_1_header * nifti_read_header(const char *hname, int *swapped, int check);
static nifti_image    * nifti_copy_nim_info(const nifti_image * src);
static nifti_image    * nifti_make_new_nim(const int dims[], int datatype,
                                                      int data_fill);
static nifti_image    * nifti_simple_init_nim(void);
static nifti_image    * nifti_convert_nhdr2nim(nifti_1_header nhdr,
                                        const char * fname);

static int    nifti_hdr_looks_good        (const nifti_1_header * hdr);
static int    nifti_is_valid_datatype     (int dtype);
static int    nifti_is_valid_ecode        (int ecode);
static int    nifti_nim_is_valid          (nifti_image * nim, int complain);
static int    nifti_nim_has_valid_dims    (nifti_image * nim, int complain);
static int    is_valid_nifti_type         (int nifti_type);
static int    nifti_test_datatype_sizes   (int verb);
static int    nifti_type_and_names_match  (nifti_image * nim, int show_warn);
static int    nifti_update_dims_from_array(nifti_image * nim);
static void   nifti_set_iname_offset      (nifti_image *nim);
static int    nifti_set_type_from_names   (nifti_image * nim);
static int    nifti_add_extension(nifti_image * nim, const char * data, int len,
                           int ecode );
static int    nifti_compiled_with_zlib    (void);
static int    nifti_copy_extensions (nifti_image *nim_dest,const nifti_image *nim_src);
static int    nifti_free_extensions (nifti_image *nim);
static int  * nifti_get_intlist     (int nvals , const char *str);
static char * nifti_strdup          (const char *str);
static int    valid_nifti_extensions(const nifti_image *nim);


/*-------------------- Some C convenience macros ----------------------------*/

/* NIfTI-1.1 extension codes:
   see http://nifti.nimh.nih.gov/nifti-1/documentation/faq#Q21 */

#define NIFTI_ECODE_IGNORE           0  /* changed from UNKNOWN, 29 June 2005 */

#define NIFTI_ECODE_DICOM            2  /* intended for raw DICOM attributes  */

#define NIFTI_ECODE_AFNI             4  /* Robert W Cox: rwcox@nih.gov        */
                                        /* http://afni.nimh.nih.gov/afni      */

#define NIFTI_ECODE_COMMENT          6  /* plain ASCII text only              */

#define NIFTI_ECODE_XCEDE            8  /* David B Keator: dbkeator@uci.edu   */
                                        /* http://www.nbirn.net/Resources/Users/Applications//xcede/index.htm */

#define NIFTI_ECODE_JIMDIMINFO      10  /* Mark A Horsfield: mah5@leicester.ac.uk */
                                        /* http://someplace/something         */

#define NIFTI_ECODE_WORKFLOW_FWDS   12  /* Kate Fissell: fissell@pitt.edu     */
                                        /* http://kraepelin.wpic.pitt.edu/~fissell/NIFTI_ECODE_WORKFLOW_FWDS/NIFTI_ECODE_WORKFLOW_FWDS.html */

#define NIFTI_ECODE_FREESURFER      14  /* http://surfer.nmr.mgh.harvard.edu  */

#define NIFTI_ECODE_PYPICKLE        16  /* embedded Python objects*/
                                        /* http://niftilib.sourceforge.net/pynifti */

        /* LONI MiND codes: http://www.loni.ucla.edu/twiki/bin/view/Main/MiND */
#define NIFTI_ECODE_MIND_IDENT      18  /* Vishal Patel: vishal.patel@ucla.edu*/
#define NIFTI_ECODE_B_VALUE         20
#define NIFTI_ECODE_SPHERICAL_DIRECTION 22
#define NIFTI_ECODE_DT_COMPONENT    24
#define NIFTI_ECODE_SHC_DEGREEORDER 26  /* end LONI MiND codes                */

#define NIFTI_ECODE_VOXBO           28  /* Dan Kimberg: www.voxbo.org         */

#define NIFTI_ECODE_CARET           30  /* John Harwell: john@brainvis.wustl.edu */
                                        /* http://brainvis.wustl.edu/wiki/index.php/Caret:Documentation:CaretNiftiExtension */

#define NIFTI_MAX_ECODE             30  /******* maximum extension code *******/

/* nifti_type file codes */
#define NIFTI_FTYPE_ANALYZE   0
#define NIFTI_FTYPE_NIFTI1_1  1
#define NIFTI_FTYPE_NIFTI1_2  2
#define NIFTI_FTYPE_ASCII     3
#define NIFTI_MAX_FTYPE       3    /* this should match the maximum code */

/*------------------------------------------------------------------------*/
/*-- the rest of these apply only to nifti1_io.c, check for _NIFTI1_IO_C_ */
/*                                                    Feb 9, 2005 [rickr] */

/*------------------------------------------------------------------------*/
protected:
  vtknifti1_io();
  ~vtknifti1_io() override;


/*---------------------------------------------------------------------------*/
/* prototypes for internal functions - not part of exported library          */

/* extension routines */
static int  nifti_read_extensions( nifti_image *nim, znzFile fp, int remain );
static int  nifti_read_next_extension( nifti1_extension * nex, nifti_image *nim, int remain, znzFile fp );
static int  nifti_check_extension(nifti_image *nim, int size,int code, int rem);
static void update_nifti_image_for_brick_list(nifti_image * nim , int nbricks);
static int  nifti_add_exten_to_list(nifti1_extension *  new_ext, nifti1_extension ** list, int new_length);
static int  nifti_fill_extension(nifti1_extension * ext, const char * data, int len, int ecode);

/* NBL routines */
static int  nifti_load_NBL_bricks(nifti_image * nim , int * slist, int * sindex, nifti_brick_list * NBL, znzFile fp );
static int  nifti_alloc_NBL_mem(  nifti_image * nim, int nbricks, nifti_brick_list * nbl);
static int  nifti_copynsort(int nbricks, const int *blist, int **slist, int **sindex);
static int  nifti_NBL_matches_nim(const nifti_image *nim, const nifti_brick_list *NBL);

/* for nifti_read_collapsed_image: */
static int  rci_read_data(nifti_image *nim, int *pivots, int *prods, int nprods, const int dims[], char *data, znzFile fp, size_t base_offset);
static int  rci_alloc_mem(void ** data, int prods[8], int nprods, int nbyper );
static int  make_pivot_list(nifti_image * nim, const int dims[], int pivots[], int prods[], int * nprods );

/* misc */
static int   compare_strlist   (const char * str, char ** strlist, int len);
static int   fileext_compare   (const char * test_ext, const char * known_ext);
static int   fileext_n_compare (const char * test_ext, const char * known_ext, size_t maxlen);
static int   is_mixedcase      (const char * str);
static int   is_uppercase      (const char * str);
static int   make_lowercase    (char * str);
static int   make_uppercase    (char * str);
static int   need_nhdr_swap    (short dim0, int hdrsize);
static int   print_hex_vals    (const char * data, int nbytes, FILE * fp);
static int   unescape_string   (char *str);  /* string utility functions */
static char *escapize_string   (const char *str);

/* internal I/O routines */
static znzFile nifti_image_load_prep( nifti_image *nim );
static int     has_ascii_header(znzFile fp);
/*---------------------------------------------------------------------------*/

static int nifti_fileexists(const char* fname);
static void compute_strides(int *strides,const int *size,int nbyper);
static int nifti_write_extensions(znzFile fp, nifti_image *nim);
static int nifti_extension_size(nifti_image *nim);

  private:
  vtknifti1_io(const vtknifti1_io&) = delete;
  void operator=(const vtknifti1_io&) = delete;

};

#endif /* _NIFTI_IO_HEADER_ */
