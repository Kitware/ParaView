/*
 * Copyright (C) 1998 NCSA
 *                    All rights reserved.
 *
 * Programmer:  Robb Matzke <matzke@llnl.gov>
 *              Tuesday, March 31, 1998
 */

#define H5T_PACKAGE             /*suppress error about including H5Tpkg   */

#include "H5private.h"          /*generic functions                       */
#include "H5Dprivate.h"         /*datasets (for H5Tcopy)                  */
#include "H5Iprivate.h"         /*ID functions                            */
#include "H5Eprivate.h"         /*error handling                          */
#include "H5FLprivate.h"        /*free lists                              */
#include "H5Gprivate.h"         /*groups                                  */
#include "H5HGprivate.h"        /*global heap                             */
#include "H5MMprivate.h"        /*memory management                       */
#include "H5Pprivate.h"         /*property lists                          */
#include "H5Sprivate.h"         /*data space                              */
#include "H5Tpkg.h"             /*data-type functions                     */

#define PABLO_MASK      H5T_mask

#define H5T_COMPND_INC  64      /*typical max numb of members per struct */

/* Interface initialization */
static int interface_initialize_g = 0;
#define INTERFACE_INIT H5T_init_interface
static herr_t H5T_init_interface(void);

/*
 * Predefined data types. These are initialized at runtime in H5Tinit.c and
 * by H5T_init_interface() in this source file.
 */
hid_t H5T_IEEE_F32BE_g                  = FAIL;
hid_t H5T_IEEE_F32LE_g                  = FAIL;
hid_t H5T_IEEE_F64BE_g                  = FAIL;
hid_t H5T_IEEE_F64LE_g                  = FAIL;

hid_t H5T_STD_I8BE_g                    = FAIL;
hid_t H5T_STD_I8LE_g                    = FAIL;
hid_t H5T_STD_I16BE_g                   = FAIL;
hid_t H5T_STD_I16LE_g                   = FAIL;
hid_t H5T_STD_I32BE_g                   = FAIL;
hid_t H5T_STD_I32LE_g                   = FAIL;
hid_t H5T_STD_I64BE_g                   = FAIL;
hid_t H5T_STD_I64LE_g                   = FAIL;
hid_t H5T_STD_U8BE_g                    = FAIL;
hid_t H5T_STD_U8LE_g                    = FAIL;
hid_t H5T_STD_U16BE_g                   = FAIL;
hid_t H5T_STD_U16LE_g                   = FAIL;
hid_t H5T_STD_U32BE_g                   = FAIL;
hid_t H5T_STD_U32LE_g                   = FAIL;
hid_t H5T_STD_U64BE_g                   = FAIL;
hid_t H5T_STD_U64LE_g                   = FAIL;
hid_t H5T_STD_B8BE_g                    = FAIL;
hid_t H5T_STD_B8LE_g                    = FAIL;
hid_t H5T_STD_B16BE_g                   = FAIL;
hid_t H5T_STD_B16LE_g                   = FAIL;
hid_t H5T_STD_B32BE_g                   = FAIL;
hid_t H5T_STD_B32LE_g                   = FAIL;
hid_t H5T_STD_B64BE_g                   = FAIL;
hid_t H5T_STD_B64LE_g                   = FAIL;
hid_t H5T_STD_REF_OBJ_g                 = FAIL;
hid_t H5T_STD_REF_DSETREG_g             = FAIL;

hid_t H5T_UNIX_D32BE_g                  = FAIL;
hid_t H5T_UNIX_D32LE_g                  = FAIL;
hid_t H5T_UNIX_D64BE_g                  = FAIL;
hid_t H5T_UNIX_D64LE_g                  = FAIL;

hid_t H5T_C_S1_g                        = FAIL;

hid_t H5T_FORTRAN_S1_g                  = FAIL;

hid_t H5T_NATIVE_SCHAR_g                = FAIL;
hid_t H5T_NATIVE_UCHAR_g                = FAIL;
hid_t H5T_NATIVE_SHORT_g                = FAIL;
hid_t H5T_NATIVE_USHORT_g               = FAIL;
hid_t H5T_NATIVE_INT_g                  = FAIL;
hid_t H5T_NATIVE_UINT_g                 = FAIL;
hid_t H5T_NATIVE_LONG_g                 = FAIL;
hid_t H5T_NATIVE_ULONG_g                = FAIL;
hid_t H5T_NATIVE_LLONG_g                = FAIL;
hid_t H5T_NATIVE_ULLONG_g               = FAIL;
hid_t H5T_NATIVE_FLOAT_g                = FAIL;
hid_t H5T_NATIVE_DOUBLE_g               = FAIL;
hid_t H5T_NATIVE_LDOUBLE_g              = FAIL;
hid_t H5T_NATIVE_B8_g                   = FAIL;
hid_t H5T_NATIVE_B16_g                  = FAIL;
hid_t H5T_NATIVE_B32_g                  = FAIL;
hid_t H5T_NATIVE_B64_g                  = FAIL;
hid_t H5T_NATIVE_OPAQUE_g               = FAIL;
hid_t H5T_NATIVE_HADDR_g                = FAIL;
hid_t H5T_NATIVE_HSIZE_g                = FAIL;
hid_t H5T_NATIVE_HSSIZE_g               = FAIL;
hid_t H5T_NATIVE_HERR_g                 = FAIL;
hid_t H5T_NATIVE_HBOOL_g                = FAIL;

hid_t H5T_NATIVE_INT8_g                 = FAIL;
hid_t H5T_NATIVE_UINT8_g                = FAIL;
hid_t H5T_NATIVE_INT_LEAST8_g           = FAIL;
hid_t H5T_NATIVE_UINT_LEAST8_g          = FAIL;
hid_t H5T_NATIVE_INT_FAST8_g            = FAIL;
hid_t H5T_NATIVE_UINT_FAST8_g           = FAIL;

hid_t H5T_NATIVE_INT16_g                = FAIL;
hid_t H5T_NATIVE_UINT16_g               = FAIL;
hid_t H5T_NATIVE_INT_LEAST16_g          = FAIL;
hid_t H5T_NATIVE_UINT_LEAST16_g         = FAIL;
hid_t H5T_NATIVE_INT_FAST16_g           = FAIL;
hid_t H5T_NATIVE_UINT_FAST16_g          = FAIL;

hid_t H5T_NATIVE_INT32_g                = FAIL;
hid_t H5T_NATIVE_UINT32_g               = FAIL;
hid_t H5T_NATIVE_INT_LEAST32_g          = FAIL;
hid_t H5T_NATIVE_UINT_LEAST32_g         = FAIL;
hid_t H5T_NATIVE_INT_FAST32_g           = FAIL;
hid_t H5T_NATIVE_UINT_FAST32_g          = FAIL;

hid_t H5T_NATIVE_INT64_g                = FAIL;
hid_t H5T_NATIVE_UINT64_g               = FAIL;
hid_t H5T_NATIVE_INT_LEAST64_g          = FAIL;
hid_t H5T_NATIVE_UINT_LEAST64_g         = FAIL;
hid_t H5T_NATIVE_INT_FAST64_g           = FAIL;
hid_t H5T_NATIVE_UINT_FAST64_g          = FAIL;

/*
 * Alignment constraints for native types. These are initialized at run time
 * in H5Tinit.c
 */
size_t H5T_NATIVE_SCHAR_ALIGN_g         = 0;
size_t H5T_NATIVE_UCHAR_ALIGN_g         = 0;
size_t H5T_NATIVE_SHORT_ALIGN_g         = 0;
size_t H5T_NATIVE_USHORT_ALIGN_g        = 0;
size_t H5T_NATIVE_INT_ALIGN_g           = 0;
size_t H5T_NATIVE_UINT_ALIGN_g          = 0;
size_t H5T_NATIVE_LONG_ALIGN_g          = 0;
size_t H5T_NATIVE_ULONG_ALIGN_g         = 0;
size_t H5T_NATIVE_LLONG_ALIGN_g         = 0;
size_t H5T_NATIVE_ULLONG_ALIGN_g        = 0;
size_t H5T_NATIVE_FLOAT_ALIGN_g         = 0;
size_t H5T_NATIVE_DOUBLE_ALIGN_g        = 0;
size_t H5T_NATIVE_LDOUBLE_ALIGN_g       = 0;

/*
 * Alignment constraints for C9x types. These are initialized at run time in
 * H5Tinit.c if the types are provided by the system. Otherwise we set their
 * values to 0 here (no alignment calculated).
 */
size_t H5T_NATIVE_INT8_ALIGN_g          = 0;
size_t H5T_NATIVE_UINT8_ALIGN_g         = 0;
size_t H5T_NATIVE_INT_LEAST8_ALIGN_g    = 0;
size_t H5T_NATIVE_UINT_LEAST8_ALIGN_g   = 0;
size_t H5T_NATIVE_INT_FAST8_ALIGN_g     = 0;
size_t H5T_NATIVE_UINT_FAST8_ALIGN_g    = 0;

size_t H5T_NATIVE_INT16_ALIGN_g         = 0;
size_t H5T_NATIVE_UINT16_ALIGN_g        = 0;
size_t H5T_NATIVE_INT_LEAST16_ALIGN_g   = 0;
size_t H5T_NATIVE_UINT_LEAST16_ALIGN_g  = 0;
size_t H5T_NATIVE_INT_FAST16_ALIGN_g    = 0;
size_t H5T_NATIVE_UINT_FAST16_ALIGN_g   = 0;

size_t H5T_NATIVE_INT32_ALIGN_g         = 0;
size_t H5T_NATIVE_UINT32_ALIGN_g        = 0;
size_t H5T_NATIVE_INT_LEAST32_ALIGN_g   = 0;
size_t H5T_NATIVE_UINT_LEAST32_ALIGN_g  = 0;
size_t H5T_NATIVE_INT_FAST32_ALIGN_g    = 0;
size_t H5T_NATIVE_UINT_FAST32_ALIGN_g   = 0;

size_t H5T_NATIVE_INT64_ALIGN_g         = 0;
size_t H5T_NATIVE_UINT64_ALIGN_g        = 0;
size_t H5T_NATIVE_INT_LEAST64_ALIGN_g   = 0;
size_t H5T_NATIVE_UINT_LEAST64_ALIGN_g  = 0;
size_t H5T_NATIVE_INT_FAST64_ALIGN_g    = 0;
size_t H5T_NATIVE_UINT_FAST64_ALIGN_g   = 0;


/*
 * The path database. Each path has a source and destination data type pair
 * which is used as the key by which the `entries' array is sorted.
 */
static struct {
    int npaths;         /*number of paths defined               */
    int apaths;         /*number of paths allocated             */
    H5T_path_t  **path;         /*sorted array of path pointers         */
    int nsoft;          /*number of soft conversions defined    */
    int asoft;          /*number of soft conversions allocated  */
    H5T_soft_t  *soft;          /*unsorted array of soft conversions    */
} H5T_g;

/* The overflow handler */
H5T_overflow_t H5T_overflow_g = NULL;

/* Local static functions */
static herr_t H5T_print_stats(H5T_path_t *path, int *nprint/*in,out*/);

/* Declare the free list for H5T_t's */
H5FL_DEFINE(H5T_t);

/* Declare the free list for H5T_path_t's */
H5FL_DEFINE(H5T_path_t);


/*-------------------------------------------------------------------------
 * Function:    H5T_init
 *
 * Purpose:     Initialize the interface from some other package.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, December 16, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_init(void)
{
    FUNC_ENTER(H5T_init, FAIL);
    /* FUNC_ENTER() does all the work */
    FUNC_LEAVE(SUCCEED);
}

/*--------------------------------------------------------------------------
NAME
   H5T_init_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_interface()
   
RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.

--------------------------------------------------------------------------*/
static herr_t
H5T_init_interface(void)
{
    H5T_t       *dt = NULL;
    hid_t       fixedpt=-1, floatpt=-1, string=-1, compound=-1, enum_type=-1;
    hid_t       vlen_type=-1, bitfield=-1, array_type=-1;
    hsize_t  dim[1]={1};    /* Dimension info for array datatype */
    herr_t      status;
    herr_t      ret_value=FAIL;

    FUNC_ENTER(H5T_init_interface, FAIL);

    /* Initialize the atom group for the file IDs */
    if (H5I_init_group(H5I_DATATYPE, H5I_DATATYPEID_HASHSIZE,
                       H5T_RESERVED_ATOMS, (H5I_free_t)H5T_close)<0) {
        HGOTO_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL,
                     "unable to initialize interface");
    }

    /* Make certain there aren't too many classes of datatypes defined */
    /* Only 16 (numbered 0-15) are supported in the current file format */
    assert(H5T_NCLASSES<16);

    /*
     * Initialize pre-defined native data types from code generated during
     * the library configuration by H5detect.
     */
    if (H5TN_init_interface()<0) {
        HGOTO_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL,
                     "unable to initialize interface");
    }

    /*------------------------------------------------------------
     * Defaults for C9x types
     *------------------------------------------------------------ 
     */

    /* int8 */
    if (H5T_NATIVE_INT8_g<0) {
        dt = H5I_object(H5T_NATIVE_INT8_g = H5Tcopy(H5T_NATIVE_INT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 1;
        dt->u.atomic.prec = 8;
    }
    if (H5T_NATIVE_UINT8_g<0) {
        dt = H5I_object(H5T_NATIVE_UINT8_g = H5Tcopy(H5T_NATIVE_UINT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 1;
        dt->u.atomic.prec = 8;
    }
    if (H5T_NATIVE_INT_LEAST8_g<0) {
        dt = H5I_object(H5T_NATIVE_INT_LEAST8_g=H5Tcopy(H5T_NATIVE_INT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 1;
        dt->u.atomic.prec = 8;
    }
    if (H5T_NATIVE_UINT_LEAST8_g<0) {
        dt = H5I_object(H5T_NATIVE_UINT_LEAST8_g=H5Tcopy(H5T_NATIVE_UINT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 1;
        dt->u.atomic.prec = 8;
    }
    if (H5T_NATIVE_INT_FAST8_g<0) {
        dt = H5I_object(H5T_NATIVE_INT_FAST8_g=H5Tcopy(H5T_NATIVE_INT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 1;
        dt->u.atomic.prec = 8;
    }
    if (H5T_NATIVE_UINT_FAST8_g<0) {
        dt = H5I_object(H5T_NATIVE_UINT_FAST8_g=H5Tcopy(H5T_NATIVE_UINT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 1;
        dt->u.atomic.prec = 8;
    }
    
    /* int16 */
    if (H5T_NATIVE_INT16_g<0) {
        dt = H5I_object(H5T_NATIVE_INT16_g = H5Tcopy(H5T_NATIVE_INT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 2;
        dt->u.atomic.prec = 16;
    }
    if (H5T_NATIVE_UINT16_g<0) {
        dt = H5I_object(H5T_NATIVE_UINT16_g = H5Tcopy(H5T_NATIVE_UINT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 2;
        dt->u.atomic.prec = 16;
    }
    if (H5T_NATIVE_INT_LEAST16_g<0) {
        dt = H5I_object(H5T_NATIVE_INT_LEAST16_g=H5Tcopy(H5T_NATIVE_INT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 2;
        dt->u.atomic.prec = 16;
    }
    if (H5T_NATIVE_UINT_LEAST16_g<0) {
        dt = H5I_object(H5T_NATIVE_UINT_LEAST16_g=H5Tcopy(H5T_NATIVE_UINT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 2;
        dt->u.atomic.prec = 16;
    }
    if (H5T_NATIVE_INT_FAST16_g<0) {
        dt = H5I_object(H5T_NATIVE_INT_FAST16_g=H5Tcopy(H5T_NATIVE_INT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 2;
        dt->u.atomic.prec = 16;
    }
    if (H5T_NATIVE_UINT_FAST16_g<0) {
        dt = H5I_object(H5T_NATIVE_UINT_FAST16_g=H5Tcopy(H5T_NATIVE_UINT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 2;
        dt->u.atomic.prec = 16;
    }
    
    /* int32 */
    if (H5T_NATIVE_INT32_g<0) {
        dt = H5I_object(H5T_NATIVE_INT32_g = H5Tcopy(H5T_NATIVE_INT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 4;
        dt->u.atomic.prec = 32;
    }
    if (H5T_NATIVE_UINT32_g<0) {
        dt = H5I_object(H5T_NATIVE_UINT32_g = H5Tcopy(H5T_NATIVE_UINT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 4;
        dt->u.atomic.prec = 32;
    }
    if (H5T_NATIVE_INT_LEAST32_g<0) {
        dt = H5I_object(H5T_NATIVE_INT_LEAST32_g=H5Tcopy(H5T_NATIVE_INT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 4;
        dt->u.atomic.prec = 32;
    }
    if (H5T_NATIVE_UINT_LEAST32_g<0) {
        dt = H5I_object(H5T_NATIVE_UINT_LEAST32_g=H5Tcopy(H5T_NATIVE_UINT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 4;
        dt->u.atomic.prec = 32;
    }
    if (H5T_NATIVE_INT_FAST32_g<0) {
        dt = H5I_object(H5T_NATIVE_INT_FAST32_g=H5Tcopy(H5T_NATIVE_INT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 4;
        dt->u.atomic.prec = 32;
    }
    if (H5T_NATIVE_UINT_FAST32_g<0) {
        dt = H5I_object(H5T_NATIVE_UINT_FAST32_g=H5Tcopy(H5T_NATIVE_UINT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 4;
        dt->u.atomic.prec = 32;
    }
    
    /* int64 */
    if (H5T_NATIVE_INT64_g<0) {
        dt = H5I_object(H5T_NATIVE_INT64_g = H5Tcopy(H5T_NATIVE_INT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 8;
        dt->u.atomic.prec = 64;
    }
    if (H5T_NATIVE_UINT64_g<0) {
        dt = H5I_object(H5T_NATIVE_UINT64_g = H5Tcopy(H5T_NATIVE_UINT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 8;
        dt->u.atomic.prec = 64;
    }
    if (H5T_NATIVE_INT_LEAST64_g<0) {
        dt = H5I_object(H5T_NATIVE_INT_LEAST64_g=H5Tcopy(H5T_NATIVE_INT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 8;
        dt->u.atomic.prec = 64;
    }
    if (H5T_NATIVE_UINT_LEAST64_g<0) {
        dt = H5I_object(H5T_NATIVE_UINT_LEAST64_g=H5Tcopy(H5T_NATIVE_UINT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 8;
        dt->u.atomic.prec = 64;
    }
    if (H5T_NATIVE_INT_FAST64_g<0) {
        dt = H5I_object(H5T_NATIVE_INT_FAST64_g=H5Tcopy(H5T_NATIVE_INT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 8;
        dt->u.atomic.prec = 64;
    }
    if (H5T_NATIVE_UINT_FAST64_g<0) {
        dt = H5I_object(H5T_NATIVE_UINT_FAST64_g=H5Tcopy(H5T_NATIVE_UINT_g));
        dt->state = H5T_STATE_IMMUTABLE;
        dt->size = 8;
        dt->u.atomic.prec = 64;
    }
    

    /*------------------------------------------------------------
     * Native types
     *------------------------------------------------------------ 
     */

    /* 1-byte bit field */
    dt = H5I_object (H5T_NATIVE_B8_g = H5Tcopy (H5T_NATIVE_UINT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_BITFIELD;
    dt->size = 1;
    dt->u.atomic.prec = 8;
    
    /* 2-byte bit field */
    dt = H5I_object (H5T_NATIVE_B16_g = H5Tcopy (H5T_NATIVE_UINT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_BITFIELD;
    dt->size = 2;
    dt->u.atomic.prec = 16;
    
    /* 4-byte bit field */
    dt = H5I_object (H5T_NATIVE_B32_g = H5Tcopy (H5T_NATIVE_UINT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_BITFIELD;
    dt->size = 4;
    dt->u.atomic.prec = 32;
    
    /* 8-byte bit field */
    dt = H5I_object (H5T_NATIVE_B64_g = H5Tcopy (H5T_NATIVE_UINT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_BITFIELD;
    dt->size = 8;
    dt->u.atomic.prec = 64;
    
    /* Opaque data */
    if (NULL==(dt = H5FL_ALLOC(H5T_t,1))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "memory allocation failed");
    }
    dt->state = H5T_STATE_IMMUTABLE;
    dt->ent.header = HADDR_UNDEF;
    dt->type = H5T_OPAQUE;
    dt->size = 1;
    dt->u.opaque.tag = H5MM_strdup("");
    if ((H5T_NATIVE_OPAQUE_g = H5I_register(H5I_DATATYPE, dt)) < 0) {
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                    "unable to initialize H5T layer");
    }

    /* haddr_t */
    dt = H5I_object(H5T_NATIVE_HADDR_g=H5Tcopy(H5T_NATIVE_UINT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = sizeof(haddr_t);
    dt->u.atomic.prec = 8*dt->size;
    dt->u.atomic.offset = 0;

    /* hsize_t */
    dt = H5I_object (H5T_NATIVE_HSIZE_g = H5Tcopy (H5T_NATIVE_UINT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = sizeof(hsize_t);
    dt->u.atomic.prec = 8*dt->size;
    dt->u.atomic.offset = 0;
    
    /* hssize_t */
    dt = H5I_object (H5T_NATIVE_HSSIZE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = sizeof(hssize_t);
    dt->u.atomic.prec = 8*dt->size;
    dt->u.atomic.offset = 0;
    
    /* herr_t */
    dt = H5I_object (H5T_NATIVE_HERR_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = sizeof(herr_t);
    dt->u.atomic.prec = 8*dt->size;
    dt->u.atomic.offset = 0;

    /* hbool_t */
    dt = H5I_object (H5T_NATIVE_HBOOL_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = sizeof(hbool_t);
    dt->u.atomic.prec = 8*dt->size;
    dt->u.atomic.offset = 0;


    /*------------------------------------------------------------
     * IEEE Types
     *------------------------------------------------------------ 
     */

    /* IEEE 4-byte little-endian float */
    dt = H5I_object (H5T_IEEE_F32LE_g = H5Tcopy (H5T_NATIVE_DOUBLE_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 4;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 32;
    dt->u.atomic.order = H5T_ORDER_LE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.f.sign = 31;
    dt->u.atomic.u.f.epos = 23;
    dt->u.atomic.u.f.esize = 8;
    dt->u.atomic.u.f.ebias = 0x7f;
    dt->u.atomic.u.f.mpos = 0;
    dt->u.atomic.u.f.msize = 23;
    dt->u.atomic.u.f.norm = H5T_NORM_IMPLIED;
    dt->u.atomic.u.f.pad = H5T_PAD_ZERO;

    /* IEEE 4-byte big-endian float */
    dt = H5I_object (H5T_IEEE_F32BE_g = H5Tcopy (H5T_NATIVE_DOUBLE_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 4;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 32;
    dt->u.atomic.order = H5T_ORDER_BE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.f.sign = 31;
    dt->u.atomic.u.f.epos = 23;
    dt->u.atomic.u.f.esize = 8;
    dt->u.atomic.u.f.ebias = 0x7f;
    dt->u.atomic.u.f.mpos = 0;
    dt->u.atomic.u.f.msize = 23;
    dt->u.atomic.u.f.norm = H5T_NORM_IMPLIED;
    dt->u.atomic.u.f.pad = H5T_PAD_ZERO;

    /* IEEE 8-byte little-endian float */
    dt = H5I_object (H5T_IEEE_F64LE_g = H5Tcopy (H5T_NATIVE_DOUBLE_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 8;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 64;
    dt->u.atomic.order = H5T_ORDER_LE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.f.sign = 63;
    dt->u.atomic.u.f.epos = 52;
    dt->u.atomic.u.f.esize = 11;
    dt->u.atomic.u.f.ebias = 0x03ff;
    dt->u.atomic.u.f.mpos = 0;
    dt->u.atomic.u.f.msize = 52;
    dt->u.atomic.u.f.norm = H5T_NORM_IMPLIED;
    dt->u.atomic.u.f.pad = H5T_PAD_ZERO;

    /* IEEE 8-byte big-endian float */
    dt = H5I_object (H5T_IEEE_F64BE_g = H5Tcopy (H5T_NATIVE_DOUBLE_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 8;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 64;
    dt->u.atomic.order = H5T_ORDER_BE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.f.sign = 63;
    dt->u.atomic.u.f.epos = 52;
    dt->u.atomic.u.f.esize = 11;
    dt->u.atomic.u.f.ebias = 0x03ff;
    dt->u.atomic.u.f.mpos = 0;
    dt->u.atomic.u.f.msize = 52;
    dt->u.atomic.u.f.norm = H5T_NORM_IMPLIED;
    dt->u.atomic.u.f.pad = H5T_PAD_ZERO;

    /*------------------------------------------------------------
     * Other "standard" types
     *------------------------------------------------------------ 
     */
    

    /* 1-byte little-endian (endianness is irrelevant) signed integer */
    dt = H5I_object (H5T_STD_I8LE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 1;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 8;
    dt->u.atomic.order = H5T_ORDER_LE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_2;
    
    /* 1-byte big-endian (endianness is irrelevant) signed integer */
    dt = H5I_object (H5T_STD_I8BE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 1;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 8;
    dt->u.atomic.order = H5T_ORDER_BE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_2;
    
    /* 2-byte little-endian signed integer */
    dt = H5I_object (H5T_STD_I16LE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 2;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 16;
    dt->u.atomic.order = H5T_ORDER_LE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_2;
    
    /* 2-byte big-endian signed integer */
    dt = H5I_object (H5T_STD_I16BE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 2;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 16;
    dt->u.atomic.order = H5T_ORDER_BE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_2;
    
    /* 4-byte little-endian signed integer */
    dt = H5I_object (H5T_STD_I32LE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 4;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 32;
    dt->u.atomic.order = H5T_ORDER_LE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_2;
    
    /* 4-byte big-endian signed integer */
    dt = H5I_object (H5T_STD_I32BE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 4;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 32;
    dt->u.atomic.order = H5T_ORDER_BE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_2;
    
    /* 8-byte little-endian signed integer */
    dt = H5I_object (H5T_STD_I64LE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 8;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 64;
    dt->u.atomic.order = H5T_ORDER_LE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_2;
    
    /* 8-byte big-endian signed integer */
    dt = H5I_object (H5T_STD_I64BE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 8;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 64;
    dt->u.atomic.order = H5T_ORDER_BE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_2;
    
    /* 1-byte little-endian (endianness is irrelevant) unsigned integer */
    dt = H5I_object (H5T_STD_U8LE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 1;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 8;
    dt->u.atomic.order = H5T_ORDER_LE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_NONE;
    
    /* 1-byte big-endian (endianness is irrelevant) unsigned integer */
    dt = H5I_object (H5T_STD_U8BE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 1;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 8;
    dt->u.atomic.order = H5T_ORDER_BE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_NONE;
    
    /* 2-byte little-endian unsigned integer */
    dt = H5I_object (H5T_STD_U16LE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 2;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 16;
    dt->u.atomic.order = H5T_ORDER_LE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_NONE;
    
    /* 2-byte big-endian unsigned integer */
    dt = H5I_object (H5T_STD_U16BE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 2;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 16;
    dt->u.atomic.order = H5T_ORDER_BE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_NONE;
    
    /* 4-byte little-endian unsigned integer */
    dt = H5I_object (H5T_STD_U32LE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 4;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 32;
    dt->u.atomic.order = H5T_ORDER_LE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_NONE;
    
    /* 4-byte big-endian unsigned integer */
    dt = H5I_object (H5T_STD_U32BE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 4;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 32;
    dt->u.atomic.order = H5T_ORDER_BE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_NONE;
    
    /* 8-byte little-endian unsigned integer */
    dt = H5I_object (H5T_STD_U64LE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 8;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 64;
    dt->u.atomic.order = H5T_ORDER_LE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_NONE;
    
    /* 8-byte big-endian unsigned integer */
    dt = H5I_object (H5T_STD_U64BE_g = H5Tcopy (H5T_NATIVE_INT_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->size = 8;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 64;
    dt->u.atomic.order = H5T_ORDER_BE;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.i.sign = H5T_SGN_NONE;
    
    /* 1-byte big endian bit field (order is irrelevant) */
    dt = H5I_object (H5T_STD_B8BE_g = H5Tcopy (H5T_STD_U8BE_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_BITFIELD;

    /* 1-byte little-endian bit field (order is irrelevant) */
    dt = H5I_object (H5T_STD_B8LE_g = H5Tcopy (H5T_STD_U8LE_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_BITFIELD;

    /* 2-byte big endian bit field */
    dt = H5I_object (H5T_STD_B16BE_g = H5Tcopy (H5T_STD_U16BE_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_BITFIELD;

    /* 2-byte little-endian bit field */
    dt = H5I_object (H5T_STD_B16LE_g = H5Tcopy (H5T_STD_U16LE_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_BITFIELD;

    /* 4-byte big endian bit field */
    dt = H5I_object (H5T_STD_B32BE_g = H5Tcopy (H5T_STD_U32BE_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_BITFIELD;

    /* 4-byte little-endian bit field */
    dt = H5I_object (H5T_STD_B32LE_g = H5Tcopy (H5T_STD_U32LE_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_BITFIELD;

    /* 8-byte big endian bit field */
    dt = H5I_object (H5T_STD_B64BE_g = H5Tcopy (H5T_STD_U64BE_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_BITFIELD;

    /* 8-byte little-endian bit field */
    dt = H5I_object (H5T_STD_B64LE_g = H5Tcopy (H5T_STD_U64LE_g));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_BITFIELD;



    /*------------------------------------------------------------
     * The Unix architecture for dates and times.
     *------------------------------------------------------------ 
     */

    /* 4-byte time_t, big-endian */
    dt = H5I_object (H5T_UNIX_D32BE_g = H5Tcopy (H5T_STD_U32BE));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_TIME;
    
    /* 4-byte time_t, little-endian */
    dt = H5I_object (H5T_UNIX_D32LE_g = H5Tcopy (H5T_STD_U32LE));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_TIME;
    
    /* 8-byte time_t, big-endian */
    dt = H5I_object (H5T_UNIX_D64BE_g = H5Tcopy (H5T_STD_U64BE));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_TIME;
    
    /* 8-byte time_t, little-endian */
    dt = H5I_object (H5T_UNIX_D64LE_g = H5Tcopy (H5T_STD_U64LE));
    dt->state = H5T_STATE_IMMUTABLE;
    dt->type = H5T_TIME;
    
    /*------------------------------------------------------------
     * The `C' architecture
     *------------------------------------------------------------ 
     */

    /* One-byte character string */
    if (NULL==(dt = H5FL_ALLOC(H5T_t,1))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "memory allocation failed");
    }
    dt->state = H5T_STATE_IMMUTABLE;
    dt->ent.header = HADDR_UNDEF;
    dt->type = H5T_STRING;
    dt->size = 1;
    dt->u.atomic.order = H5T_ORDER_NONE;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 8 * dt->size;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.s.cset = H5T_CSET_ASCII;
    dt->u.atomic.u.s.pad = H5T_STR_NULLTERM;
    if ((H5T_C_S1_g = H5I_register(H5I_DATATYPE, dt)) < 0) {
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                    "unable to initialize H5T layer");
    }

    /*------------------------------------------------------------
     * The `Fortran' architecture
     *------------------------------------------------------------ 
     */

    /* One-byte character string */
    if (NULL==(dt = H5FL_ALLOC(H5T_t,1))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "memory allocation failed");
    }
    dt->state = H5T_STATE_IMMUTABLE;
    dt->ent.header = HADDR_UNDEF;
    dt->type = H5T_STRING;
    dt->size = 1;
    dt->u.atomic.order = H5T_ORDER_NONE;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 8 * dt->size;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.s.cset = H5T_CSET_ASCII;
    dt->u.atomic.u.s.pad = H5T_STR_SPACEPAD;
    if ((H5T_FORTRAN_S1_g = H5I_register(H5I_DATATYPE, dt)) < 0) {
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                    "unable to initialize H5T layer");
    }

    /*------------------------------------------------------------
     * Pointer types
     *------------------------------------------------------------ 
     */

    /* Object pointer (i.e. object header address in file) */
    if (NULL==(dt = H5FL_ALLOC(H5T_t,1))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "memory allocation failed");
    }
    dt->state = H5T_STATE_IMMUTABLE;
    dt->ent.header = HADDR_UNDEF;
    dt->type = H5T_REFERENCE;
    dt->size = H5R_OBJ_REF_BUF_SIZE;
    dt->u.atomic.order = H5T_ORDER_NONE;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 8 * dt->size;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.r.rtype = H5R_OBJECT;
    if ((H5T_STD_REF_OBJ_g = H5I_register(H5I_DATATYPE, dt)) < 0) {
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                    "unable to initialize H5T layer");
    }
    
    /* Dataset Region pointer (i.e. selection inside a dataset) */
    if (NULL==(dt = H5FL_ALLOC(H5T_t,1))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "memory allocation failed");
    }
    dt->state = H5T_STATE_IMMUTABLE;
    dt->ent.header = HADDR_UNDEF;
    dt->type = H5T_REFERENCE;
    dt->size = H5R_DSET_REG_REF_BUF_SIZE;
    dt->u.atomic.order = H5T_ORDER_NONE;
    dt->u.atomic.offset = 0;
    dt->u.atomic.prec = 8 * dt->size;
    dt->u.atomic.lsb_pad = H5T_PAD_ZERO;
    dt->u.atomic.msb_pad = H5T_PAD_ZERO;
    dt->u.atomic.u.r.rtype = H5R_DATASET_REGION;
    if ((H5T_STD_REF_DSETREG_g = H5I_register(H5I_DATATYPE, dt)) < 0) {
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                    "unable to initialize H5T layer");
    }

    /*
     * Register conversion functions beginning with the most general and
     * ending with the most specific.
     */
    fixedpt = H5T_NATIVE_INT;
    floatpt = H5T_NATIVE_FLOAT;
    string  = H5T_C_S1;
    bitfield = H5T_STD_B8LE;
    compound = H5Tcreate(H5T_COMPOUND, 1);
    enum_type = H5Tcreate(H5T_ENUM, 1);
    vlen_type = H5Tvlen_create(H5T_NATIVE_INT);
    array_type = H5Tarray_create(H5T_NATIVE_INT,1,dim,NULL);
    status = 0;

    status |= H5Tregister(H5T_PERS_SOFT, "i_i",
                          fixedpt, fixedpt,
                          H5T_conv_i_i);
    status |= H5Tregister(H5T_PERS_SOFT, "f_f",
                          floatpt, floatpt,
                          H5T_conv_f_f);
    status |= H5Tregister(H5T_PERS_SOFT, "s_s",
                          string, string,
                          H5T_conv_s_s);
    status |= H5Tregister(H5T_PERS_SOFT, "b_b",
                          bitfield, bitfield,
                          H5T_conv_b_b);
    status |= H5Tregister(H5T_PERS_SOFT, "ibo",
                          fixedpt, fixedpt,
                          H5T_conv_order);
    status |= H5Tregister(H5T_PERS_SOFT, "ibo(opt)",
                          fixedpt, fixedpt,
                          H5T_conv_order_opt);
    status |= H5Tregister(H5T_PERS_SOFT, "fbo",
                          floatpt, floatpt,
                          H5T_conv_order);
    status |= H5Tregister(H5T_PERS_SOFT, "fbo(opt)",
                          floatpt, floatpt,
                          H5T_conv_order_opt);
    status |= H5Tregister(H5T_PERS_SOFT, "struct(no-opt)",
                          compound, compound,
                          H5T_conv_struct);
    status |= H5Tregister(H5T_PERS_SOFT, "struct(opt)",
                          compound, compound,
                          H5T_conv_struct_opt);
    status |= H5Tregister(H5T_PERS_SOFT, "enum",
                          enum_type, enum_type,
                          H5T_conv_enum);
    status |= H5Tregister(H5T_PERS_SOFT, "vlen",
                          vlen_type, vlen_type,
                          H5T_conv_vlen);
    status |= H5Tregister(H5T_PERS_SOFT, "array",
                          array_type, array_type,
                          H5T_conv_array);
    
    status |= H5Tregister(H5T_PERS_HARD, "u32le_f64le",
                          H5T_STD_U32LE_g, H5T_IEEE_F64LE_g,
                          H5T_conv_i32le_f64le);
    status |= H5Tregister(H5T_PERS_HARD, "i32le_f64le",
                          H5T_STD_I32LE_g, H5T_IEEE_F64LE_g,
                          H5T_conv_i32le_f64le);

    /*
     * Native conversions should be listed last since we can use hardware to
     * perform the conversion.  We list the odd types like `llong', `long',
     * and `short' before the usual types like `int' and `char' so that when
     * diagnostics are printed we favor the usual names over the odd names
     * when two or more types are the same size.
     */

    /* floating point */
    status |= H5Tregister(H5T_PERS_HARD, "flt_dbl",
                          H5T_NATIVE_FLOAT, H5T_NATIVE_DOUBLE,
                          H5T_conv_float_double);
    status |= H5Tregister(H5T_PERS_HARD, "dbl_flt",
                          H5T_NATIVE_DOUBLE, H5T_NATIVE_FLOAT,
                          H5T_conv_double_float);

    /* from long_long */
    status |= H5Tregister(H5T_PERS_HARD, "llong_ullong",
                          H5T_NATIVE_LLONG, H5T_NATIVE_ULLONG,
                          H5T_conv_llong_ullong);
    status |= H5Tregister(H5T_PERS_HARD, "ullong_llong",
                          H5T_NATIVE_ULLONG, H5T_NATIVE_LLONG,
                          H5T_conv_ullong_llong);
    status |= H5Tregister(H5T_PERS_HARD, "llong_long",
                          H5T_NATIVE_LLONG, H5T_NATIVE_LONG,
                          H5T_conv_llong_long);
    status |= H5Tregister(H5T_PERS_HARD, "llong_ulong",
                          H5T_NATIVE_LLONG, H5T_NATIVE_ULONG,
                          H5T_conv_llong_ulong);
    status |= H5Tregister(H5T_PERS_HARD, "ullong_long",
                          H5T_NATIVE_ULLONG, H5T_NATIVE_LONG,
                          H5T_conv_ullong_long);
    status |= H5Tregister(H5T_PERS_HARD, "ullong_ulong",
                          H5T_NATIVE_ULLONG, H5T_NATIVE_ULONG,
                          H5T_conv_ullong_ulong);
    status |= H5Tregister(H5T_PERS_HARD, "llong_short",
                          H5T_NATIVE_LLONG, H5T_NATIVE_SHORT,
                          H5T_conv_llong_short);
    status |= H5Tregister(H5T_PERS_HARD, "llong_ushort",
                          H5T_NATIVE_LLONG, H5T_NATIVE_USHORT,
                          H5T_conv_llong_ushort);
    status |= H5Tregister(H5T_PERS_HARD, "ullong_short",
                          H5T_NATIVE_ULLONG, H5T_NATIVE_SHORT,
                          H5T_conv_ullong_short);
    status |= H5Tregister(H5T_PERS_HARD, "ullong_ushort",
                          H5T_NATIVE_ULLONG, H5T_NATIVE_USHORT,
                          H5T_conv_ullong_ushort);
    status |= H5Tregister(H5T_PERS_HARD, "llong_int",
                          H5T_NATIVE_LLONG, H5T_NATIVE_INT,
                          H5T_conv_llong_int);
    status |= H5Tregister(H5T_PERS_HARD, "llong_uint",
                          H5T_NATIVE_LLONG, H5T_NATIVE_UINT,
                          H5T_conv_llong_uint);
    status |= H5Tregister(H5T_PERS_HARD, "ullong_int",
                          H5T_NATIVE_ULLONG, H5T_NATIVE_INT,
                          H5T_conv_ullong_int);
    status |= H5Tregister(H5T_PERS_HARD, "ullong_uint",
                          H5T_NATIVE_ULLONG, H5T_NATIVE_UINT,
                          H5T_conv_ullong_uint);
    status |= H5Tregister(H5T_PERS_HARD, "llong_schar",
                          H5T_NATIVE_LLONG, H5T_NATIVE_SCHAR,
                          H5T_conv_llong_schar);
    status |= H5Tregister(H5T_PERS_HARD, "llong_uchar",
                          H5T_NATIVE_LLONG, H5T_NATIVE_UCHAR,
                          H5T_conv_llong_uchar);
    status |= H5Tregister(H5T_PERS_HARD, "ullong_schar",
                          H5T_NATIVE_ULLONG, H5T_NATIVE_SCHAR,
                          H5T_conv_ullong_schar);
    status |= H5Tregister(H5T_PERS_HARD, "ullong_uchar",
                          H5T_NATIVE_ULLONG, H5T_NATIVE_UCHAR,
                          H5T_conv_ullong_uchar);
    
    /* From long */
    status |= H5Tregister(H5T_PERS_HARD, "long_llong",
                          H5T_NATIVE_LONG, H5T_NATIVE_LLONG,
                          H5T_conv_long_llong);
    status |= H5Tregister(H5T_PERS_HARD, "long_ullong",
                          H5T_NATIVE_LONG, H5T_NATIVE_ULLONG,
                          H5T_conv_long_ullong);
    status |= H5Tregister(H5T_PERS_HARD, "ulong_llong",
                          H5T_NATIVE_ULONG, H5T_NATIVE_LLONG,
                          H5T_conv_ulong_llong);
    status |= H5Tregister(H5T_PERS_HARD, "ulong_ullong",
                          H5T_NATIVE_ULONG, H5T_NATIVE_ULLONG,
                          H5T_conv_ulong_ullong);
    status |= H5Tregister(H5T_PERS_HARD, "long_ulong",
                          H5T_NATIVE_LONG, H5T_NATIVE_ULONG,
                          H5T_conv_long_ulong);
    status |= H5Tregister(H5T_PERS_HARD, "ulong_long",
                          H5T_NATIVE_ULONG, H5T_NATIVE_LONG,
                          H5T_conv_ulong_long);
    status |= H5Tregister(H5T_PERS_HARD, "long_short",
                          H5T_NATIVE_LONG, H5T_NATIVE_SHORT,
                          H5T_conv_long_short);
    status |= H5Tregister(H5T_PERS_HARD, "long_ushort",
                          H5T_NATIVE_LONG, H5T_NATIVE_USHORT,
                          H5T_conv_long_ushort);
    status |= H5Tregister(H5T_PERS_HARD, "ulong_short",
                          H5T_NATIVE_ULONG, H5T_NATIVE_SHORT,
                          H5T_conv_ulong_short);
    status |= H5Tregister(H5T_PERS_HARD, "ulong_ushort",
                          H5T_NATIVE_ULONG, H5T_NATIVE_USHORT,
                          H5T_conv_ulong_ushort);
    status |= H5Tregister(H5T_PERS_HARD, "long_int",
                          H5T_NATIVE_LONG, H5T_NATIVE_INT,
                          H5T_conv_long_int);
    status |= H5Tregister(H5T_PERS_HARD, "long_uint",
                          H5T_NATIVE_LONG, H5T_NATIVE_UINT,
                          H5T_conv_long_uint);
    status |= H5Tregister(H5T_PERS_HARD, "ulong_int",
                          H5T_NATIVE_ULONG, H5T_NATIVE_INT,
                          H5T_conv_ulong_int);
    status |= H5Tregister(H5T_PERS_HARD, "ulong_uint",
                          H5T_NATIVE_ULONG, H5T_NATIVE_UINT,
                          H5T_conv_ulong_uint);
    status |= H5Tregister(H5T_PERS_HARD, "long_schar",
                          H5T_NATIVE_LONG, H5T_NATIVE_SCHAR,
                          H5T_conv_long_schar);
    status |= H5Tregister(H5T_PERS_HARD, "long_uchar",
                          H5T_NATIVE_LONG, H5T_NATIVE_UCHAR,
                          H5T_conv_long_uchar);
    status |= H5Tregister(H5T_PERS_HARD, "ulong_schar",
                          H5T_NATIVE_ULONG, H5T_NATIVE_SCHAR,
                          H5T_conv_ulong_schar);
    status |= H5Tregister(H5T_PERS_HARD, "ulong_uchar",
                          H5T_NATIVE_ULONG, H5T_NATIVE_UCHAR,
                          H5T_conv_ulong_uchar);
    
    /* From short */
    status |= H5Tregister(H5T_PERS_HARD, "short_llong",
                          H5T_NATIVE_SHORT, H5T_NATIVE_LLONG,
                          H5T_conv_short_llong);
    status |= H5Tregister(H5T_PERS_HARD, "short_ullong",
                          H5T_NATIVE_SHORT, H5T_NATIVE_ULLONG,
                          H5T_conv_short_ullong);
    status |= H5Tregister(H5T_PERS_HARD, "ushort_llong",
                          H5T_NATIVE_USHORT, H5T_NATIVE_LLONG,
                          H5T_conv_ushort_llong);
    status |= H5Tregister(H5T_PERS_HARD, "ushort_ullong",
                          H5T_NATIVE_USHORT, H5T_NATIVE_ULLONG,
                          H5T_conv_ushort_ullong);
    status |= H5Tregister(H5T_PERS_HARD, "short_long",
                          H5T_NATIVE_SHORT, H5T_NATIVE_LONG,
                          H5T_conv_short_long);
    status |= H5Tregister(H5T_PERS_HARD, "short_ulong",
                          H5T_NATIVE_SHORT, H5T_NATIVE_ULONG,
                          H5T_conv_short_ulong);
    status |= H5Tregister(H5T_PERS_HARD, "ushort_long",
                          H5T_NATIVE_USHORT, H5T_NATIVE_LONG,
                          H5T_conv_ushort_long);
    status |= H5Tregister(H5T_PERS_HARD, "ushort_ulong",
                          H5T_NATIVE_USHORT, H5T_NATIVE_ULONG,
                          H5T_conv_ushort_ulong);
    status |= H5Tregister(H5T_PERS_HARD, "short_ushort",
                          H5T_NATIVE_SHORT, H5T_NATIVE_USHORT,
                          H5T_conv_short_ushort);
    status |= H5Tregister(H5T_PERS_HARD, "ushort_short",
                          H5T_NATIVE_USHORT, H5T_NATIVE_SHORT,
                          H5T_conv_ushort_short);
    status |= H5Tregister(H5T_PERS_HARD, "short_int",
                          H5T_NATIVE_SHORT, H5T_NATIVE_INT,
                          H5T_conv_short_int);
    status |= H5Tregister(H5T_PERS_HARD, "short_uint",
                          H5T_NATIVE_SHORT, H5T_NATIVE_UINT,
                          H5T_conv_short_uint);
    status |= H5Tregister(H5T_PERS_HARD, "ushort_int",
                          H5T_NATIVE_USHORT, H5T_NATIVE_INT,
                          H5T_conv_ushort_int);
    status |= H5Tregister(H5T_PERS_HARD, "ushort_uint",
                          H5T_NATIVE_USHORT, H5T_NATIVE_UINT,
                          H5T_conv_ushort_uint);
    status |= H5Tregister(H5T_PERS_HARD, "short_schar",
                          H5T_NATIVE_SHORT, H5T_NATIVE_SCHAR,
                          H5T_conv_short_schar);
    status |= H5Tregister(H5T_PERS_HARD, "short_uchar",
                          H5T_NATIVE_SHORT, H5T_NATIVE_UCHAR,
                          H5T_conv_short_uchar);
    status |= H5Tregister(H5T_PERS_HARD, "ushort_schar",
                          H5T_NATIVE_USHORT, H5T_NATIVE_SCHAR,
                          H5T_conv_ushort_schar);
    status |= H5Tregister(H5T_PERS_HARD, "ushort_uchar",
                          H5T_NATIVE_USHORT, H5T_NATIVE_UCHAR,
                          H5T_conv_ushort_uchar);
    
    /* From int */
    status |= H5Tregister(H5T_PERS_HARD, "int_llong",
                          H5T_NATIVE_INT, H5T_NATIVE_LLONG,
                          H5T_conv_int_llong);
    status |= H5Tregister(H5T_PERS_HARD, "int_ullong",
                          H5T_NATIVE_INT, H5T_NATIVE_ULLONG,
                          H5T_conv_int_ullong);
    status |= H5Tregister(H5T_PERS_HARD, "uint_llong",
                          H5T_NATIVE_UINT, H5T_NATIVE_LLONG,
                          H5T_conv_uint_llong);
    status |= H5Tregister(H5T_PERS_HARD, "uint_ullong",
                          H5T_NATIVE_UINT, H5T_NATIVE_ULLONG,
                          H5T_conv_uint_ullong);
    status |= H5Tregister(H5T_PERS_HARD, "int_long",
                          H5T_NATIVE_INT, H5T_NATIVE_LONG,
                          H5T_conv_int_long);
    status |= H5Tregister(H5T_PERS_HARD, "int_ulong",
                          H5T_NATIVE_INT, H5T_NATIVE_ULONG,
                          H5T_conv_int_ulong);
    status |= H5Tregister(H5T_PERS_HARD, "uint_long",
                          H5T_NATIVE_UINT, H5T_NATIVE_LONG,
                          H5T_conv_uint_long);
    status |= H5Tregister(H5T_PERS_HARD, "uint_ulong",
                          H5T_NATIVE_UINT, H5T_NATIVE_ULONG,
                          H5T_conv_uint_ulong);
    status |= H5Tregister(H5T_PERS_HARD, "int_short",
                          H5T_NATIVE_INT, H5T_NATIVE_SHORT,
                          H5T_conv_int_short);
    status |= H5Tregister(H5T_PERS_HARD, "int_ushort",
                          H5T_NATIVE_INT, H5T_NATIVE_USHORT,
                          H5T_conv_int_ushort);
    status |= H5Tregister(H5T_PERS_HARD, "uint_short",
                          H5T_NATIVE_UINT, H5T_NATIVE_SHORT,
                          H5T_conv_uint_short);
    status |= H5Tregister(H5T_PERS_HARD, "uint_ushort",
                          H5T_NATIVE_UINT, H5T_NATIVE_USHORT,
                          H5T_conv_uint_ushort);
    status |= H5Tregister(H5T_PERS_HARD, "int_uint",
                          H5T_NATIVE_INT, H5T_NATIVE_UINT,
                          H5T_conv_int_uint);
    status |= H5Tregister(H5T_PERS_HARD, "uint_int",
                          H5T_NATIVE_UINT, H5T_NATIVE_INT,
                          H5T_conv_uint_int);
    status |= H5Tregister(H5T_PERS_HARD, "int_schar",
                          H5T_NATIVE_INT, H5T_NATIVE_SCHAR,
                          H5T_conv_int_schar);
    status |= H5Tregister(H5T_PERS_HARD, "int_uchar",
                          H5T_NATIVE_INT, H5T_NATIVE_UCHAR,
                          H5T_conv_int_uchar);
    status |= H5Tregister(H5T_PERS_HARD, "uint_schar",
                          H5T_NATIVE_UINT, H5T_NATIVE_SCHAR,
                          H5T_conv_uint_schar);
    status |= H5Tregister(H5T_PERS_HARD, "uint_uchar",
                          H5T_NATIVE_UINT, H5T_NATIVE_UCHAR,
                          H5T_conv_uint_uchar);

    /* From char */
    status |= H5Tregister(H5T_PERS_HARD, "schar_llong",
                          H5T_NATIVE_SCHAR, H5T_NATIVE_LLONG,
                          H5T_conv_schar_llong);
    status |= H5Tregister(H5T_PERS_HARD, "schar_ullong",
                          H5T_NATIVE_SCHAR, H5T_NATIVE_ULLONG,
                          H5T_conv_schar_ullong);
    status |= H5Tregister(H5T_PERS_HARD, "uchar_llong",
                          H5T_NATIVE_UCHAR, H5T_NATIVE_LLONG,
                          H5T_conv_uchar_llong);
    status |= H5Tregister(H5T_PERS_HARD, "uchar_ullong",
                          H5T_NATIVE_UCHAR, H5T_NATIVE_ULLONG,
                          H5T_conv_uchar_ullong);
    status |= H5Tregister(H5T_PERS_HARD, "schar_long",
                          H5T_NATIVE_SCHAR, H5T_NATIVE_LONG,
                          H5T_conv_schar_long);
    status |= H5Tregister(H5T_PERS_HARD, "schar_ulong",
                          H5T_NATIVE_SCHAR, H5T_NATIVE_ULONG,
                          H5T_conv_schar_ulong);
    status |= H5Tregister(H5T_PERS_HARD, "uchar_long",
                          H5T_NATIVE_UCHAR, H5T_NATIVE_LONG,
                          H5T_conv_uchar_long);
    status |= H5Tregister(H5T_PERS_HARD, "uchar_ulong",
                          H5T_NATIVE_UCHAR, H5T_NATIVE_ULONG,
                          H5T_conv_uchar_ulong);
    status |= H5Tregister(H5T_PERS_HARD, "schar_short",
                          H5T_NATIVE_SCHAR, H5T_NATIVE_SHORT,
                          H5T_conv_schar_short);
    status |= H5Tregister(H5T_PERS_HARD, "schar_ushort",
                          H5T_NATIVE_SCHAR, H5T_NATIVE_USHORT,
                          H5T_conv_schar_ushort);
    status |= H5Tregister(H5T_PERS_HARD, "uchar_short",
                          H5T_NATIVE_UCHAR, H5T_NATIVE_SHORT,
                          H5T_conv_uchar_short);
    status |= H5Tregister(H5T_PERS_HARD, "uchar_ushort",
                          H5T_NATIVE_UCHAR, H5T_NATIVE_USHORT,
                          H5T_conv_uchar_ushort);
    status |= H5Tregister(H5T_PERS_HARD, "schar_int",
                          H5T_NATIVE_SCHAR, H5T_NATIVE_INT,
                          H5T_conv_schar_int);
    status |= H5Tregister(H5T_PERS_HARD, "schar_uint",
                          H5T_NATIVE_SCHAR, H5T_NATIVE_UINT,
                          H5T_conv_schar_uint);
    status |= H5Tregister(H5T_PERS_HARD, "uchar_int",
                          H5T_NATIVE_UCHAR, H5T_NATIVE_INT,
                          H5T_conv_uchar_int);
    status |= H5Tregister(H5T_PERS_HARD, "uchar_uint",
                          H5T_NATIVE_UCHAR, H5T_NATIVE_UINT,
                          H5T_conv_uchar_uint);
    status |= H5Tregister(H5T_PERS_HARD, "schar_uchar",
                          H5T_NATIVE_SCHAR, H5T_NATIVE_UCHAR,
                          H5T_conv_schar_uchar);
    status |= H5Tregister(H5T_PERS_HARD, "uchar_schar",
                          H5T_NATIVE_UCHAR, H5T_NATIVE_SCHAR,
                          H5T_conv_uchar_schar);

    /*
     * The special no-op conversion is the fastest, so we list it last. The
     * data types we use are not important as long as the source and
     * destination are equal.
     */
    status |= H5Tregister(H5T_PERS_HARD, "no-op",
                          H5T_NATIVE_INT, H5T_NATIVE_INT,
                          H5T_conv_noop);

    if (status<0) {
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                    "unable to register conversion function(s)");
    }
    
    ret_value = SUCCEED;
 done:
    if (compound>=0) H5Tclose(compound);
    if (enum_type>=0) H5Tclose(enum_type);
    if (vlen_type>=0) H5Tclose(vlen_type);
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_unlock_cb
 *
 * Purpose:     Clear the immutable flag for a data type.  This function is
 *              called when the library is closing in order to unlock all
 *              registered data types and thus make them free-able.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, April 27, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5T_unlock_cb (void *_dt, const void UNUSED *key)
{
    H5T_t       *dt = (H5T_t *)_dt;
    
    FUNC_ENTER (H5T_unlock_cb, FAIL);
    assert (dt);
    if (H5T_STATE_IMMUTABLE==dt->state) {
        dt->state = H5T_STATE_RDONLY;
    }
    FUNC_LEAVE (0);
    key = 0;
}


/*-------------------------------------------------------------------------
 * Function:    H5T_term_interface
 *
 * Purpose:     Close this interface.
 *
 * Return:      Success:        Positive if any action might have caused a
 *                              change in some other interface; zero
 *                              otherwise.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Friday, November 20, 1998
 *
 * Modifications:
 *      Robb Matzke, 1998-06-11
 *      Statistics are only printed for conversion functions that were
 *      called.
 *-------------------------------------------------------------------------
 */
int
H5T_term_interface(void)
{
    int i, nprint=0, n=0;
    H5T_path_t  *path = NULL;

    if (interface_initialize_g) {
        /* Unregister all conversion functions */
        for (i=0; i<H5T_g.npaths; i++) {
            path = H5T_g.path[i];
            assert (path);

            if (path->func) {
                H5T_print_stats(path, &nprint/*in,out*/);
                path->cdata.command = H5T_CONV_FREE;
                if ((path->func)(FAIL, FAIL, &(path->cdata),
                                 (hsize_t)0, 0, 0, NULL, NULL,H5P_DEFAULT)<0) {
#ifdef H5T_DEBUG
                    if (H5DEBUG(T)) {
                        fprintf (H5DEBUG(T), "H5T: conversion function "
                                 "0x%08lx failed to free private data for "
                                 "%s (ignored)\n",
                                 (unsigned long)(path->func), path->name);
                    }
#endif
                    H5E_clear(); /*ignore the error*/
                }
            }
            H5T_close (path->src);
            H5T_close (path->dst);
        H5FL_FREE(H5T_path_t,path);
            H5T_g.path[i] = NULL;
        }

        /* Clear conversion tables */
        H5T_g.path = H5MM_xfree(H5T_g.path);
        H5T_g.npaths = H5T_g.apaths = 0;
        H5T_g.soft = H5MM_xfree(H5T_g.soft);
        H5T_g.nsoft = H5T_g.asoft = 0;

        /* Unlock all datatypes, then free them */
        H5I_search (H5I_DATATYPE, H5T_unlock_cb, NULL);
        H5I_destroy_group(H5I_DATATYPE);

        /* Mark interface as closed */
        interface_initialize_g = 0;
        n = 1; /*H5I*/
    }
    return n;
}


/*-------------------------------------------------------------------------
 * Function:    H5Tcreate
 *
 * Purpose:     Create a new type and initialize it to reasonable values.
 *              The type is a member of type class TYPE and is SIZE bytes.
 *
 * Return:      Success:        A new type identifier.
 *
 *              Failure:        Negative
 *
 * Errors:
 *              ARGS      BADVALUE      Invalid size. 
 *              DATATYPE  CANTINIT      Can't create type. 
 *              DATATYPE  CANTREGISTER  Can't register data type atom. 
 *
 * Programmer:  Robb Matzke
 *              Friday, December  5, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tcreate(H5T_class_t type, size_t size)
{
    H5T_t       *dt = NULL;
    hid_t       ret_value = FAIL;

    FUNC_ENTER(H5Tcreate, FAIL);
    H5TRACE2("i","Ttz",type,size);

    /* check args */
    if (size <= 0) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid size");
    }

    /* create the type */
    if (NULL == (dt = H5T_create(type, size))) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "unable to create type");
    }

    /* Make it an atom */
    if ((ret_value = H5I_register(H5I_DATATYPE, dt)) < 0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL,
                      "unable to register data type atom");
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Topen
 *
 * Purpose:     Opens a named data type.
 *
 * Return:      Success:        Object ID of the named data type.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, June  1, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Topen(hid_t loc_id, const char *name)
{
    H5G_entry_t *loc = NULL;
    H5T_t       *type = NULL;
    hid_t       ret_value = FAIL;
    
    FUNC_ENTER (H5Topen, FAIL);
    H5TRACE2("i","is",loc_id,name);

    /* Check args */
    if (NULL==(loc=H5G_loc (loc_id))) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "no name");
    }

    /* Open it */
    if (NULL==(type=H5T_open (loc, name))) {
        HRETURN_ERROR (H5E_DATATYPE, H5E_CANTOPENOBJ, FAIL,
                       "unable to open named data type");
    }

    /* Register the type and return the ID */
    if ((ret_value=H5I_register (H5I_DATATYPE, type))<0) {
        H5T_close (type);
        HRETURN_ERROR (H5E_DATATYPE, H5E_CANTREGISTER, FAIL,
                       "unable to register named data type");
    }

    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tcommit
 *
 * Purpose:     Save a transient data type to a file and turn the type handle
 *              into a named, immutable type.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, June  1, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tcommit(hid_t loc_id, const char *name, hid_t type_id)
{
    H5G_entry_t *loc = NULL;
    H5T_t       *type = NULL;
    
    FUNC_ENTER (H5Tcommit, FAIL);
    H5TRACE3("e","isi",loc_id,name,type_id);

    /* Check arguments */
    if (NULL==(loc=H5G_loc (loc_id))) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "no name");
    }
    if (H5I_DATATYPE!=H5I_get_type (type_id) ||
        NULL==(type=H5I_object (type_id))) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }

    /* Commit the type */
    if (H5T_commit (loc, name, type)<0) {
        HRETURN_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL,
                       "unable to commit data type");
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tcommitted
 *
 * Purpose:     Determines if a data type is committed or not.
 *
 * Return:      Success:        TRUE if committed, FALSE otherwise.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, June  4, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Tcommitted(hid_t type_id)
{
    H5T_t       *type = NULL;
    
    FUNC_ENTER (H5Tcommitted, FAIL);
    H5TRACE1("b","i",type_id);

    /* Check arguments */
    if (H5I_DATATYPE!=H5I_get_type (type_id) ||
        NULL==(type=H5I_object (type_id))) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }

    FUNC_LEAVE (H5T_STATE_OPEN==type->state || H5T_STATE_NAMED==type->state);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tcopy
 *
 * Purpose:     Copies a data type.  The resulting data type is not locked.
 *              The data type should be closed when no longer needed by
 *              calling H5Tclose().
 *
 * Return:      Success:        The ID of a new data type.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 4 Jun 1998
 *      The returned type is always transient and unlocked.  If the TYPE_ID
 *      argument is a dataset instead of a data type then this function
 *      returns a transient, modifiable data type which is a copy of the
 *      dataset's data type.
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tcopy(hid_t type_id)
{
    H5T_t       *dt = NULL;
    H5T_t       *new_dt = NULL;
    H5D_t       *dset = NULL;
    hid_t       ret_value = FAIL;

    FUNC_ENTER(H5Tcopy, FAIL);
    H5TRACE1("i","i",type_id);

    switch (H5I_get_type (type_id)) {
    case H5I_DATATYPE:
        /* The argument is a data type handle */
        if (NULL==(dt=H5I_object (type_id))) {
            HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
        }
        break;

    case H5I_DATASET:
        /* The argument is a dataset handle */
        if (NULL==(dset=H5I_object (type_id))) {
            HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a dataset");
        }
        if (NULL==(dt=H5D_typeof (dset))) {
            HRETURN_ERROR (H5E_DATASET, H5E_CANTINIT, FAIL,
                           "unable to get the dataset data type");
        }
        break;

    default:
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL,
                       "not a data type or dataset");
    }

    /* Copy */
    if (NULL == (new_dt = H5T_copy(dt, H5T_COPY_TRANSIENT))) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "unable to copy");
    }

    /* Atomize result */
    if ((ret_value = H5I_register(H5I_DATATYPE, new_dt)) < 0) {
        H5T_close(new_dt);
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL,
                      "unable to register data type atom");
    }
    
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tclose
 *
 * Purpose:     Frees a data type and all associated memory.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tclose(hid_t type_id)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tclose, FAIL);
    H5TRACE1("e","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_IMMUTABLE==dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "immutable data type");
    }

    /* When the reference count reaches zero the resources are freed */
    if (H5I_dec_ref(type_id) < 0) {
        HRETURN_ERROR(H5E_ATOM, H5E_BADATOM, FAIL, "problem freeing id");
    }
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tequal
 *
 * Purpose:     Determines if two data types are equal.
 *
 * Return:      Success:        TRUE if equal, FALSE if unequal
 *
 *              Failure:        Negative
 *
 * Errors:
 *
 * Programmer:  Robb Matzke
 *              Wednesday, December 10, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Tequal(hid_t type1_id, hid_t type2_id)
{
    const H5T_t         *dt1 = NULL;
    const H5T_t         *dt2 = NULL;
    htri_t              ret_value = FAIL;

    FUNC_ENTER(H5Tequal, FAIL);
    H5TRACE2("b","ii",type1_id,type2_id);

    /* check args */
    if (H5I_DATATYPE != H5I_get_type(type1_id) ||
        NULL == (dt1 = H5I_object(type1_id)) ||
        H5I_DATATYPE != H5I_get_type(type2_id) ||
        NULL == (dt2 = H5I_object(type2_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    ret_value = (0 == H5T_cmp(dt1, dt2)) ? TRUE : FALSE;

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tlock
 *
 * Purpose:     Locks a type, making it read only and non-destructable.  This
 *              is normally done by the library for predefined data types so
 *              the application doesn't inadvertently change or delete a
 *              predefined type.
 *
 *              Once a data type is locked it can never be unlocked unless
 *              the entire library is closed.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, January  9, 1998
 *
 * Modifications:
 *
 *      Robb Matzke, 1 Jun 1998
 *      It is illegal to lock a named data type since we must allow named
 *      types to be closed (to release file resources) but locking a type
 *      prevents that.
 *-------------------------------------------------------------------------
 */
herr_t
H5Tlock(hid_t type_id)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tlock, FAIL);
    H5TRACE1("e","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_NAMED==dt->state || H5T_STATE_OPEN==dt->state) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "unable to lock named data type");
    }
    if (H5T_lock (dt, TRUE)<0) {
        HRETURN_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL,
                       "unable to lock transient data type");
    }
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_class
 *
 * Purpose:     Returns the data type class identifier for data type TYPE_ID.
 *
 * Return:      Success:        One of the non-negative data type class
 *                              constants.
 *
 *              Failure:        H5T_NO_CLASS (Negative)
 *
 * Programmer:  Robb Matzke
 *              Monday, December  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_class_t
H5Tget_class(hid_t type_id)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tget_class, H5T_NO_CLASS);
    H5TRACE1("Tt","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_NO_CLASS, "not a data type");
    }
    
    FUNC_LEAVE(H5T_get_class(dt));
}


/*-------------------------------------------------------------------------
 * Function:    H5T_get_class
 *
 * Purpose:     Returns the data type class identifier for a datatype ptr.
 *
 * Return:      Success:        One of the non-negative data type class
 *                              constants.
 *
 *              Failure:        H5T_NO_CLASS (Negative)
 *
 * Programmer:  Robb Matzke
 *              Monday, December  8, 1997
 *
 * Modifications:
 *      Broke out from H5Tget_class - QAK - 6/4/99
 *
 *-------------------------------------------------------------------------
 */
H5T_class_t
H5T_get_class(const H5T_t *dt)
{
    H5T_class_t ret_value;

    FUNC_ENTER(H5T_get_class, H5T_NO_CLASS);

    assert(dt);
    
    /* Lie to the user if they have a VL string and tell them it's in the string class */
    if(dt->type==H5T_VLEN && dt->u.vlen.type==H5T_VLEN_STRING)
        ret_value=H5T_STRING;
    else
        ret_value=dt->type;

    FUNC_LEAVE(ret_value);
}   /* end H5T_get_class() */


/*-------------------------------------------------------------------------
 * Function:    H5Tdetect_class
 *
 * Purpose:     Check whether a datatype contains (or is) a certain type of
 *              datatype.
 *
 * Return:      TRUE (1) or FALSE (0) on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, November 29, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5Tdetect_class(hid_t type, H5T_class_t cls)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER (H5Tdetect_class, FAIL);
    H5TRACE2("b","iTt",type,cls);
    
    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type) ||
            NULL == (dt = H5I_object(type))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_NO_CLASS, "not a data type");
    }
    if (!(cls>H5T_NO_CLASS && cls<H5T_NCLASSES)) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_NO_CLASS, "not a data type class");
    }

    FUNC_LEAVE(H5T_detect_class(dt,cls));
}


/*-------------------------------------------------------------------------
 * Function:    H5T_detect_class
 *
 * Purpose:     Check whether a datatype contains (or is) a certain type of
 *              datatype.
 *
 * Return:      TRUE (1) or FALSE (0) on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, November 29, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5T_detect_class (H5T_t *dt, H5T_class_t cls)
{
    int         i;

    FUNC_ENTER (H5T_detect_class, FAIL);
    
    assert(dt);
    assert(cls>H5T_NO_CLASS && cls<H5T_NCLASSES);

    /* Check if this type is the correct type */
    if(dt->type==cls)
        HRETURN(TRUE);

    /* check for types that might have the correct type as a component */
    switch(dt->type) {
        case H5T_COMPOUND:
            for (i=0; i<dt->u.compnd.nmembs; i++) {
                /* Check if this field's type is the correct type */
                if(dt->u.compnd.memb[i].type->type==cls)
                    HRETURN(TRUE);

                /* Recurse if it's VL, compound or array */
                if(dt->u.compnd.memb[i].type->type==H5T_COMPOUND || dt->u.compnd.memb[i].type->type==H5T_VLEN || dt->u.compnd.memb[i].type->type==H5T_ARRAY)
                    HRETURN(H5T_detect_class(dt->u.compnd.memb[i].type,cls));
            } /* end for */
            break;

        case H5T_ARRAY:
        case H5T_VLEN:
        case H5T_ENUM:
            HRETURN(H5T_detect_class(dt->parent,cls));
            break;

        default:
            break;
    } /* end if */

    FUNC_LEAVE (FALSE);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_size
 *
 * Purpose:     Determines the total size of a data type in bytes.
 *
 * Return:      Success:        Size of the data type in bytes.  The size of
 *                              data type is the size of an instance of that
 *                              data type.
 *
 *              Failure:        0 (valid data types are never zero size)
 *
 * Programmer:  Robb Matzke
 *              Monday, December  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
H5Tget_size(hid_t type_id)
{
    H5T_t       *dt = NULL;
    size_t      size;

    FUNC_ENTER(H5Tget_size, 0);
    H5TRACE1("z","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a data type");
    }
    
    /* size */
    size = H5T_get_size(dt);

    FUNC_LEAVE(size);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_size
 *
 * Purpose:     Sets the total size in bytes for a data type (this operation
 *              is not permitted on compound data types).  If the size is
 *              decreased so that the significant bits of the data type
 *              extend beyond the edge of the new size, then the `offset'
 *              property is decreased toward zero.  If the `offset' becomes
 *              zero and the significant bits of the data type still hang
 *              over the edge of the new size, then the number of significant
 *              bits is decreased.
 *
 *              Adjusting the size of an H5T_STRING automatically sets the
 *              precision to 8*size.
 *
 *              All data types have a positive size.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Moved the real work into a private function.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_size(hid_t type_id, size_t size)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tset_size, FAIL);
    H5TRACE2("e","iz",type_id,size);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (size <= 0 && size!=H5T_VARIABLE) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "size must be positive");
    }
    if (size == H5T_VARIABLE && dt->type!=H5T_STRING) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "only strings may be variable length");
    }
    if (H5T_ENUM==dt->type && dt->u.enumer.nmembs>0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not allowed after members are defined");
    }
    if (H5T_COMPOUND==dt->type || H5T_ARRAY==dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for this datatype");
    }

    /* Do the work */
    if (H5T_set_size(dt, size)<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "unable to set size for data type");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_order
 *
 * Purpose:     Returns the byte order of a data type.
 *
 * Return:      Success:        A byte order constant
 *
 *              Failure:        H5T_ORDER_ERROR (Negative)
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
H5T_order_t
H5Tget_order(hid_t type_id)
{
    H5T_t               *dt = NULL;
    H5T_order_t         order;

    FUNC_ENTER(H5Tget_order, H5T_ORDER_ERROR);
    H5TRACE1("To","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_ORDER_ERROR,
                      "not a data type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_COMPOUND==dt->type || H5T_OPAQUE==dt->type || H5T_ARRAY ==dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5T_ORDER_ERROR,
                      "operation not defined for specified data type");
    }

    /* Order */
    assert(H5T_is_atomic(dt));
    order = dt->u.atomic.order;

    FUNC_LEAVE(order);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_order
 *
 * Purpose:     Sets the byte order for a data type.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_order(hid_t type_id, H5T_order_t order)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tset_order, FAIL);
    H5TRACE2("e","iTo",type_id,order);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (order < 0 || order > H5T_ORDER_NONE) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "illegal byte order");
    }
    if (H5T_ENUM==dt->type && dt->u.enumer.nmembs>0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not allowed after members are defined");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_COMPOUND==dt->type || H5T_OPAQUE==dt->type || H5T_ARRAY==dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5T_ORDER_ERROR,
                      "operation not defined for specified data type");
    }

    /* Commit */
    assert(H5T_is_atomic(dt));
    dt->u.atomic.order = order;
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_precision
 *
 * Purpose:     Gets the precision of a data type.  The precision is
 *              the number of significant bits which, unless padding is
 *              present, is 8 times larger than the value returned by
 *              H5Tget_size().
 *
 * Return:      Success:        Number of significant bits
 *
 *              Failure:        0 (all atomic types have at least one
 *                              significant bit)
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
size_t
H5Tget_precision(hid_t type_id)
{
    H5T_t       *dt = NULL;
    size_t      prec;

    FUNC_ENTER(H5Tget_precision, 0);
    H5TRACE1("z","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a data type");
    }
    if (dt->parent) dt = dt->parent;    /*defer to parent*/
    if (H5T_COMPOUND==dt->type || H5T_OPAQUE==dt->type || H5T_ARRAY==dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5T_ORDER_ERROR,
                      "operation not defined for specified data type");
    }
    
    /* Precision */
    assert(H5T_is_atomic(dt));
    prec = dt->u.atomic.prec;

    FUNC_LEAVE(prec);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_precision
 *
 * Purpose:     Sets the precision of a data type.  The precision is
 *              the number of significant bits which, unless padding is
 *              present, is 8 times larger than the value returned by
 *              H5Tget_size().
 *
 *              If the precision is increased then the offset is decreased
 *              and then the size is increased to insure that significant
 *              bits do not "hang over" the edge of the data type.
 *
 *              The precision property of strings is read-only.
 *
 *              When decreasing the precision of a floating point type, set
 *              the locations and sizes of the sign, mantissa, and exponent
 *              fields first.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Moved real work to a private function.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_precision(hid_t type_id, size_t prec)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tset_precision, FAIL);
    H5TRACE2("e","iz",type_id,prec);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (prec <= 0) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "precision must be positive");
    }
    if (H5T_ENUM==dt->type && dt->u.enumer.nmembs>0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not allowed after members are defined");
    }

    /* Do the work */
    if (H5T_set_precision(dt, prec)<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "unable to set precision");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_offset
 *
 * Purpose:     Retrieves the bit offset of the first significant bit.  The
 *              signficant bits of an atomic datum can be offset from the
 *              beginning of the memory for that datum by an amount of
 *              padding. The `offset' property specifies the number of bits
 *              of padding that appear to the "right of" the value.  That is,
 *              if we have a 32-bit datum with 16-bits of precision having
 *              the value 0x1122 then it will be layed out in memory as (from
 *              small byte address toward larger byte addresses):
 *
 *                  Big      Big       Little   Little
 *                  Endian   Endian    Endian   Endian
 *                  offset=0 offset=16 offset=0 offset=16
 *
 *              0:  [ pad]   [0x11]    [0x22]   [ pad]
 *              1:  [ pad]   [0x22]    [0x11]   [ pad]
 *              2:  [0x11]   [ pad]    [ pad]   [0x22]
 *              3:  [0x22]   [ pad]    [ pad]   [0x11]
 *
 * Return:      Success:        The offset (non-negative)
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
int
H5Tget_offset(hid_t type_id)
{
    H5T_t       *dt = NULL;
    int offset;

    FUNC_ENTER(H5Tget_offset, -1);
    H5TRACE1("Is","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an atomic data type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_COMPOUND==dt->type || H5T_OPAQUE==dt->type || H5T_ARRAY==dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for specified data type");
    }
    
    /* Offset */
    assert(H5T_is_atomic(dt));
    offset = (int)dt->u.atomic.offset;

    FUNC_LEAVE(offset);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_offset
 *
 * Purpose:     Sets the bit offset of the first significant bit.  The
 *              signficant bits of an atomic datum can be offset from the
 *              beginning of the memory for that datum by an amount of
 *              padding. The `offset' property specifies the number of bits
 *              of padding that appear to the "right of" the value.  That is,
 *              if we have a 32-bit datum with 16-bits of precision having
 *              the value 0x1122 then it will be layed out in memory as (from
 *              small byte address toward larger byte addresses):
 *
 *                  Big      Big       Little   Little
 *                  Endian   Endian    Endian   Endian
 *                  offset=0 offset=16 offset=0 offset=16
 *
 *              0:  [ pad]   [0x11]    [0x22]   [ pad]
 *              1:  [ pad]   [0x22]    [0x11]   [ pad]
 *              2:  [0x11]   [ pad]    [ pad]   [0x22]
 *              3:  [0x22]   [ pad]    [ pad]   [0x11]
 *
 *              If the offset is incremented then the total size is
 *              incremented also if necessary to prevent significant bits of
 *              the value from hanging over the edge of the data type.
 *
 *              The offset of an H5T_STRING cannot be set to anything but
 *              zero. 
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Moved real work to a private function.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_offset(hid_t type_id, size_t offset)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tset_offset, FAIL);
    H5TRACE2("e","iz",type_id,offset);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an atomic data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (H5T_STRING == dt->type && offset != 0) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "offset must be zero for this type");
    }
    if (H5T_ENUM==dt->type && dt->u.enumer.nmembs>0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not allowed after members are defined");
    }

    /* Do the real work */
    if (H5T_set_offset(dt, offset)<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "unable to set offset");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_pad
 *
 * Purpose:     Gets the least significant pad type and the most significant
 *              pad type and returns their values through the LSB and MSB
 *              arguments, either of which may be the null pointer.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, January  9, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works with derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tget_pad(hid_t type_id, H5T_pad_t *lsb/*out*/, H5T_pad_t *msb/*out*/)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tget_pad, FAIL);
    H5TRACE3("e","ixx",type_id,lsb,msb);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_COMPOUND==dt->type || H5T_OPAQUE==dt->type || H5T_ARRAY==dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for specified data type");
    }
    
    /* Get values */
    assert(H5T_is_atomic(dt));
    if (lsb) *lsb = dt->u.atomic.lsb_pad;
    if (msb) *msb = dt->u.atomic.msb_pad;

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_pad
 *
 * Purpose:     Sets the LSB and MSB pad types.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, January  9, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works with derived data types.
 *      
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_pad(hid_t type_id, H5T_pad_t lsb, H5T_pad_t msb)
{
    H5T_t *dt = NULL;

    FUNC_ENTER(H5Tset_pad, FAIL);
    H5TRACE3("e","iTpTp",type_id,lsb,msb);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (lsb < 0 || lsb >= H5T_NPAD || msb < 0 || msb >= H5T_NPAD) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid pad type");
    }
    if (H5T_ENUM==dt->type && dt->u.enumer.nmembs>0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not allowed after members are defined");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_COMPOUND==dt->type || H5T_OPAQUE==dt->type || H5T_ARRAY==dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for specified data type");
    }

    /* Commit */
    assert(H5T_is_atomic(dt));
    dt->u.atomic.lsb_pad = lsb;
    dt->u.atomic.msb_pad = msb;

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_sign
 *
 * Purpose:     Retrieves the sign type for an integer type.
 *
 * Return:      Success:        The sign type.
 *
 *              Failure:        H5T_SGN_ERROR (Negative)
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works with derived data types.
 *-------------------------------------------------------------------------
 */
H5T_sign_t
H5Tget_sign(hid_t type_id)
{
    H5T_t               *dt = NULL;
    H5T_sign_t          sign;

    FUNC_ENTER(H5Tget_sign, H5T_SGN_ERROR);
    H5TRACE1("Ts","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_SGN_ERROR,
                      "not an integer data type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_INTEGER!=dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5T_SGN_ERROR,
                      "operation not defined for data type class");
    }
    
    /* Sign */
    sign = dt->u.atomic.u.i.sign;

    FUNC_LEAVE(sign);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_sign
 *
 * Purpose:     Sets the sign property for an integer.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works with derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_sign(hid_t type_id, H5T_sign_t sign)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tset_sign, FAIL);
    H5TRACE2("e","iTs",type_id,sign);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an integer data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (sign < 0 || sign >= H5T_NSGN) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "illegal sign type");
    }
    if (H5T_ENUM==dt->type && dt->u.enumer.nmembs>0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not allowed after members are defined");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_INTEGER!=dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for data type class");
    }
    
    /* Commit */
    dt->u.atomic.u.i.sign = sign;
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_fields
 *
 * Purpose:     Returns information about the locations of the various bit
 *              fields of a floating point data type.  The field positions
 *              are bit positions in the significant region of the data type.
 *              Bits are numbered with the least significant bit number zero.
 *
 *              Any (or even all) of the arguments can be null pointers.
 *
 * Return:      Success:        Non-negative, field locations and sizes are
 *                              returned through the arguments.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works with derived data types.
 *-------------------------------------------------------------------------
 */
herr_t
H5Tget_fields(hid_t type_id, size_t *spos/*out*/,
              size_t *epos/*out*/, size_t *esize/*out*/,
              size_t *mpos/*out*/, size_t *msize/*out*/)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tget_fields, FAIL);
    H5TRACE6("e","ixxxxx",type_id,spos,epos,esize,mpos,msize);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_FLOAT != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for data type class");
    }
    
    /* Get values */
    if (spos) *spos = dt->u.atomic.u.f.sign;
    if (epos) *epos = dt->u.atomic.u.f.epos;
    if (esize) *esize = dt->u.atomic.u.f.esize;
    if (mpos) *mpos = dt->u.atomic.u.f.mpos;
    if (msize) *msize = dt->u.atomic.u.f.msize;

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_fields
 *
 * Purpose:     Sets the locations and sizes of the various floating point
 *              bit fields.  The field positions are bit positions in the
 *              significant region of the data type.  Bits are numbered with
 *              the least significant bit number zero.
 *
 *              Fields are not allowed to extend beyond the number of bits of
 *              precision, nor are they allowed to overlap with one another.
 *              
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *      
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_fields(hid_t type_id, size_t spos, size_t epos, size_t esize,
              size_t mpos, size_t msize)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tset_fields, FAIL);
    H5TRACE6("e","izzzzz",type_id,spos,epos,esize,mpos,msize);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_FLOAT != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for data type class");
    }
    if (epos + esize > dt->u.atomic.prec) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "exponent bit field size/location is invalid");
    }
    if (mpos + msize > dt->u.atomic.prec) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "mantissa bit field size/location is invalid");
    }
    if (spos >= dt->u.atomic.prec) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "sign location is not valid");
    }
    
    /* Check for overlap */
    if (spos >= epos && spos < epos + esize) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "sign bit appears within exponent field");
    }
    if (spos >= mpos && spos < mpos + msize) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "sign bit appears within mantissa field");
    }
    if ((mpos < epos && mpos + msize > epos) ||
        (epos < mpos && epos + esize > mpos)) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "exponent and mantissa fields overlap");
    }
    
    /* Commit */
    dt->u.atomic.u.f.sign = spos;
    dt->u.atomic.u.f.epos = epos;
    dt->u.atomic.u.f.mpos = mpos;
    dt->u.atomic.u.f.esize = esize;
    dt->u.atomic.u.f.msize = msize;

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_ebias
 *
 * Purpose:     Retrieves the exponent bias of a floating-point type.
 *
 * Return:      Success:        The bias
 *
 *              Failure:        0
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works with derived data types.
 *-------------------------------------------------------------------------
 */
size_t
H5Tget_ebias(hid_t type_id)
{
    H5T_t       *dt = NULL;
    size_t      ebias;

    FUNC_ENTER(H5Tget_ebias, 0);
    H5TRACE1("z","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a data type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_FLOAT != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, 0,
                      "operation not defined for data type class");
    }
    
    /* bias */
    ebias = dt->u.atomic.u.f.ebias;

    FUNC_LEAVE(ebias);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_ebias
 *
 * Purpose:     Sets the exponent bias of a floating-point type.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works with derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_ebias(hid_t type_id, size_t ebias)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tset_ebias, FAIL);
    H5TRACE2("e","iz",type_id,ebias);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_FLOAT != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for data type class");
    }

    /* Commit */
    dt->u.atomic.u.f.ebias = ebias;

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_norm
 *
 * Purpose:     Returns the mantisssa normalization of a floating-point data
 *              type.
 *
 * Return:      Success:        Normalization ID
 *
 *              Failure:        H5T_NORM_ERROR (Negative)
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works with derived data types.
 *
 *-------------------------------------------------------------------------
 */
H5T_norm_t
H5Tget_norm(hid_t type_id)
{
    H5T_t       *dt = NULL;
    H5T_norm_t  norm;

    FUNC_ENTER(H5Tget_norm, H5T_NORM_ERROR);
    H5TRACE1("Tn","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_NORM_ERROR,
                      "not a data type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_FLOAT != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5T_NORM_ERROR,
                      "operation not defined for data type class");
    }
    
    /* norm */
    norm = dt->u.atomic.u.f.norm;

    FUNC_LEAVE(norm);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_norm
 *
 * Purpose:     Sets the mantissa normalization method for a floating point
 *              data type.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_norm(hid_t type_id, H5T_norm_t norm)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tset_norm, FAIL);
    H5TRACE2("e","iTn",type_id,norm);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (norm < 0 || norm > H5T_NORM_NONE) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "illegal normalization");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_FLOAT != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for data type class");
    }
    
    /* Commit */
    dt->u.atomic.u.f.norm = norm;
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_inpad
 *
 * Purpose:     If any internal bits of a floating point type are unused
 *              (that is, those significant bits which are not part of the
 *              sign, exponent, or mantissa) then they will be filled
 *              according to the value of this property.
 *
 * Return:      Success:        The internal padding type.
 *
 *              Failure:        H5T_PAD_ERROR (Negative)
 *
 * Programmer:  Robb Matzke
 *              Friday, January  9, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
H5T_pad_t
H5Tget_inpad(hid_t type_id)
{
    H5T_t       *dt = NULL;
    H5T_pad_t   pad;

    FUNC_ENTER(H5Tget_inpad, H5T_PAD_ERROR);
    H5TRACE1("Tp","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_PAD_ERROR,
                      "not a data type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_FLOAT != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5T_PAD_ERROR,
                      "operation not defined for data type class");
    }
    
    /* pad */
    pad = dt->u.atomic.u.f.pad;

    FUNC_LEAVE(pad);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_inpad
 *
 * Purpose:     If any internal bits of a floating point type are unused
 *              (that is, those significant bits which are not part of the
 *              sign, exponent, or mantissa) then they will be filled
 *              according to the value of this property.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, January  9, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_inpad(hid_t type_id, H5T_pad_t pad)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tset_inpad, FAIL);
    H5TRACE2("e","iTp",type_id,pad);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (pad < 0 || pad >= H5T_NPAD) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "illegal internal pad type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_FLOAT != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for data type class");
    }
    
    /* Commit */
    dt->u.atomic.u.f.pad = pad;
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_cset
 *
 * Purpose:     HDF5 is able to distinguish between character sets of
 *              different nationalities and to convert between them to the
 *              extent possible.
 *              
 * Return:      Success:        The character set of an H5T_STRING type.
 *
 *              Failure:        H5T_CSET_ERROR (Negative)
 *
 * Programmer:  Robb Matzke
 *              Friday, January  9, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
H5T_cset_t
H5Tget_cset(hid_t type_id)
{
    H5T_t       *dt = NULL;
    H5T_cset_t  cset;

    FUNC_ENTER(H5Tget_cset, H5T_CSET_ERROR);
    H5TRACE1("Tc","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_CSET_ERROR,
                      "not a data type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_STRING != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5T_CSET_ERROR,
                      "operation not defined for data type class");
    }
    
    /* result */
    cset = dt->u.atomic.u.s.cset;

    FUNC_LEAVE(cset);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_cset
 *
 * Purpose:     HDF5 is able to distinguish between character sets of
 *              different nationalities and to convert between them to the
 *              extent possible.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, January  9, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works with derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_cset(hid_t type_id, H5T_cset_t cset)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tset_cset, FAIL);
    H5TRACE2("e","iTc",type_id,cset);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (cset < 0 || cset >= H5T_NCSET) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "illegal character set type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_STRING != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for data type class");
    }
    
    /* Commit */
    dt->u.atomic.u.s.cset = cset;
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_strpad
 *
 * Purpose:     The method used to store character strings differs with the
 *              programming language: C usually null terminates strings while
 *              Fortran left-justifies and space-pads strings.  This property
 *              defines the storage mechanism for the string.
 *              
 * Return:      Success:        The character set of an H5T_STRING type.
 *
 *              Failure:        H5T_STR_ERROR (Negative)
 *
 * Programmer:  Robb Matzke
 *              Friday, January  9, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
H5T_str_t
H5Tget_strpad(hid_t type_id)
{
    H5T_t       *dt = NULL;
    H5T_str_t   strpad;

    FUNC_ENTER(H5Tget_strpad, H5T_STR_ERROR);
    H5TRACE1("Tz","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_STR_ERROR, "not a data type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_STRING != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, H5T_STR_ERROR,
                      "operation not defined for data type class");
    }
    
    /* result */
    strpad = dt->u.atomic.u.s.pad;

    FUNC_LEAVE(strpad);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_strpad
 *
 * Purpose:     The method used to store character strings differs with the
 *              programming language: C usually null terminates strings while
 *              Fortran left-justifies and space-pads strings.  This property
 *              defines the storage mechanism for the string.
 *
 *              When converting from a long string to a short string if the
 *              short string is H5T_STR_NULLPAD or H5T_STR_SPACEPAD then the
 *              string is simply truncated; otherwise if the short string is
 *              H5T_STR_NULLTERM it will be truncated and a null terminator
 *              is appended.
 *
 *              When converting from a short string to a long string, the
 *              long string is padded on the end by appending nulls or
 *              spaces.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, January  9, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_strpad(hid_t type_id, H5T_str_t strpad)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tset_strpad, FAIL);
    H5TRACE2("e","iTz",type_id,strpad);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (strpad < 0 || strpad >= H5T_NSTR) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "illegal string pad type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_STRING != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for data type class");
    }
    
    /* Commit */
    dt->u.atomic.u.s.pad = strpad;
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_nmembers
 *
 * Purpose:     Determines how many members TYPE_ID has.  The type must be
 *              either a compound data type or an enumeration data type.
 *
 * Return:      Success:        Number of members defined in the data type.
 *
 *              Failure:        Negative
 *
 * Errors:
 *
 * Programmer:  Robb Matzke
 *              Monday, December  8, 1997
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works with enumeration data types.
 *-------------------------------------------------------------------------
 */
int
H5Tget_nmembers(hid_t type_id)
{
    H5T_t       *dt = NULL;
    int ret_value = FAIL;

    FUNC_ENTER(H5Tget_num_members, FAIL);
    H5TRACE1("Is","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }

    if (H5T_COMPOUND==dt->type) {
        ret_value = dt->u.compnd.nmembs;
    } else if (H5T_ENUM==dt->type) {
        ret_value = dt->u.enumer.nmembs;
    } else {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "operation not supported for type class");
    }
    
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_member_name
 *
 * Purpose:     Returns the name of a member of a compound or enumeration
 *              data type. Members are stored in no particular order with
 *              numbers 0 through N-1 where N is the value returned by
 *              H5Tget_nmembers().
 *
 * Return:      Success:        Ptr to a string allocated with malloc().  The
 *                              caller is responsible for freeing the string.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works with enumeration data types.
 *-------------------------------------------------------------------------
 */
char *
H5Tget_member_name(hid_t type_id, int membno)
{
    H5T_t       *dt = NULL;
    char        *ret_value = NULL;

    FUNC_ENTER(H5Tget_member_name, NULL);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a data type");
    }

    switch (dt->type) {
    case H5T_COMPOUND:
        if (membno<0 || membno>=dt->u.compnd.nmembs) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                          "invalid member number");
        }
        ret_value = H5MM_xstrdup(dt->u.compnd.memb[membno].name);
        break;

    case H5T_ENUM:
        if (membno<0 || membno>=dt->u.enumer.nmembs) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                          "invalid member number");
        }
        ret_value = H5MM_xstrdup(dt->u.enumer.name[membno]);
        break;
        
    default:
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, NULL,
                      "operation not supported for type class");
    }

    /* Value */
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_member_offset
 *
 * Purpose:     Returns the byte offset of the beginning of a member with
 *              respect to the beginning of the compound data type datum.
 *
 * Return:      Success:        Byte offset.
 *
 *              Failure:        Zero. Zero is a valid offset, but this
 *                              function will fail only if a call to
 *                              H5Tget_member_dims() fails with the same
 *                              arguments.
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
H5Tget_member_offset(hid_t type_id, int membno)
{
    H5T_t       *dt = NULL;
    size_t      offset = 0;

    FUNC_ENTER(H5Tget_member_offset, 0);
    H5TRACE2("z","iIs",type_id,membno);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id)) ||
        H5T_COMPOUND != dt->type) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, 0, "not a compound data type");
    }
    if (membno < 0 || membno >= dt->u.compnd.nmembs) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, 0, "invalid member number");
    }

    /* Value */
    offset = dt->u.compnd.memb[membno].offset;

    FUNC_LEAVE(offset);
}

#ifdef WANT_H5_V1_2_COMPAT

/*-------------------------------------------------------------------------
 * Function:    H5Tget_member_dims
 *
 * Purpose:     Returns the dimensionality of the member.  The dimensions and
 *              permuation vector are returned through arguments DIMS and
 *              PERM, both arrays of at least four elements.  Either (or even
 *              both) may be null pointers.
 *
 * Return:      Success:        A value between zero and four, inclusive.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *  Moved into compatibility section for v1.2 functions and patched to
 *      use new array datatypes - QAK, 11/13/00
 *
 *-------------------------------------------------------------------------
 */
int
H5Tget_member_dims(hid_t type_id, int membno,
                   size_t dims[]/*out*/, int perm[]/*out*/)
{
    H5T_t       *dt = NULL;
    int ndims, i;

    FUNC_ENTER(H5Tget_member_dims, FAIL);
    H5TRACE4("Is","iIsxx",type_id,membno,dims,perm);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
            NULL == (dt = H5I_object(type_id)) ||
            H5T_COMPOUND != dt->type) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a compound data type");
    }
    if (membno < 0 || membno >= dt->u.compnd.nmembs) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid member number");
    }

    /* Check if this field is an array datatype */
    if(dt->u.compnd.memb[membno].type->type==H5T_ARRAY) {
        /* Value */
        ndims = dt->u.compnd.memb[membno].type->u.array.ndims;
        for (i = 0; i < ndims; i++) {
            if (dims)
                dims[i] = dt->u.compnd.memb[membno].type->u.array.dim[i];
            if (perm)
                perm[i] = dt->u.compnd.memb[membno].type->u.array.perm[i];
        }
    } /* end if */
    else 
        ndims=0;

    FUNC_LEAVE(ndims);
}
#endif /* WANT_H5_V1_2_COMPAT */


/*-------------------------------------------------------------------------
 * Function:    H5Tget_member_class
 *
 * Purpose:     Returns the datatype class of a member of a compound datatype.
 *
 * Return:      Success: Non-negative
 *
 *              Failure: H5T_NO_CLASS
 *
 * Programmer:  Quincey Koziol
 *              Thursday, November  9, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_class_t
H5Tget_member_class(hid_t type_id, int membno)
{
    H5T_t       *dt = NULL;
    H5T_class_t ret_value = H5T_NO_CLASS;

    FUNC_ENTER(H5Tget_member_class, H5T_NO_CLASS);
    H5TRACE2("Tt","iIs",type_id,membno);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
            NULL == (dt = H5I_object(type_id)) || H5T_COMPOUND != dt->type)
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, H5T_NO_CLASS, "not a compound data type");
    if (membno < 0 || membno >= dt->u.compnd.nmembs)
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, H5T_NO_CLASS, "invalid member number");

    /* Value */
    ret_value = dt->u.compnd.memb[membno].type->type;

    FUNC_LEAVE(ret_value);
} /* end H5Tget_member_class() */


/*-------------------------------------------------------------------------
 * Function:    H5Tget_member_type
 *
 * Purpose:     Returns the data type of the specified member.  The caller
 *              should invoke H5Tclose() to release resources associated with
 *              the type.
 *
 * Return:      Success:        An OID of a copy of the member data type;
 *                              modifying the returned data type does not
 *                              modify the member type.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *      Robb Matzke, 4 Jun 1998
 *      If the member type is a named type then this function returns a
 *      handle to the re-opened named type.
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tget_member_type(hid_t type_id, int membno)
{
    H5T_t       *dt = NULL, *memb_dt = NULL;
    hid_t       memb_type_id;

    FUNC_ENTER(H5Tget_member_type, FAIL);
    H5TRACE2("i","iIs",type_id,membno);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id)) ||
        H5T_COMPOUND != dt->type) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a compound data type");
    }
    if (membno < 0 || membno >= dt->u.compnd.nmembs) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid member number");
    }
    
#ifdef WANT_H5_V1_2_COMPAT
    /* HDF5-v1.2.x returns the base type of an array field, not the array
     * field itself.  Emulate this difference. - QAK
     */
    if(dt->u.compnd.memb[membno].type->type==H5T_ARRAY) {
        /* Copy parent's data type into an atom */
        if (NULL == (memb_dt = H5T_copy(dt->u.compnd.memb[membno].type->parent,
                                        H5T_COPY_REOPEN))) {
            HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                          "unable to copy member data type");
        }
    } /* end if */
    else {
        /* Copy data type into an atom */
        if (NULL == (memb_dt = H5T_copy(dt->u.compnd.memb[membno].type,
                                        H5T_COPY_REOPEN))) {
            HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                          "unable to copy member data type");
        }
    } /* end if */

#else /* WANT_H5_V1_2_COMPAT */
    /* Copy data type into an atom */
    if (NULL == (memb_dt = H5T_copy(dt->u.compnd.memb[membno].type,
                                    H5T_COPY_REOPEN))) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "unable to copy member data type");
    }
#endif /* WANT_H5_V1_2_COMPAT */
    if ((memb_type_id = H5I_register(H5I_DATATYPE, memb_dt)) < 0) {
        H5T_close(memb_dt);
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL,
                      "unable register data type atom");
    }
    
    FUNC_LEAVE(memb_type_id);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tinsert
 *
 * Purpose:     Adds another member to the compound data type PARENT_ID.  The
 *              new member has a NAME which must be unique within the
 *              compound data type. The OFFSET argument defines the start of
 *              the member in an instance of the compound data type, and
 *              MEMBER_ID is the type of the new member.
 *
 * Return:      Success:        Non-negative, the PARENT_ID compound data
 *                              type is modified to include a copy of the
 *                              member type MEMBER_ID.
 *
 *              Failure:        Negative
 *
 * Errors:
 *
 * Programmer:  Robb Matzke
 *              Monday, December  8, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tinsert(hid_t parent_id, const char *name, size_t offset, hid_t member_id)
{
    H5T_t       *parent = NULL;         /*the compound parent data type */
    H5T_t       *member = NULL;         /*the atomic member type        */

    FUNC_ENTER(H5Tinsert, FAIL);
    H5TRACE4("e","iszi",parent_id,name,offset,member_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(parent_id) ||
        NULL == (parent = H5I_object(parent_id)) ||
        H5T_COMPOUND != parent->type) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a compound data type");
    }
    if (H5T_STATE_TRANSIENT!=parent->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "parent type read-only");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no member name");
    }
    if (H5I_DATATYPE != H5I_get_type(member_id) ||
        NULL == (member = H5I_object(member_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }

    /* Insert */
    if (H5T_insert(parent, name, offset, member) < 0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINSERT, FAIL,
                      "unable to insert member");
    }
    
    FUNC_LEAVE(SUCCEED);
}

#ifdef WANT_H5_V1_2_COMPAT

/*-------------------------------------------------------------------------
 * Function:    H5Tinsert_array
 *
 * Purpose:     Adds another member to the compound data type PARENT_ID. The
 *              new member has a NAME which must be unique within the
 *              compound data type.  The OFFSET argument defines the start of
 *              the member in an instance of the compound data type and
 *              MEMBER_ID is the type of the new member.  The member is an
 *              array with NDIMS dimensionality and the size of the array is
 *              DIMS. The total member size should be relatively small.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, July  7, 1998
 *
 * Modifications:
 *  Moved into compatibility section for v1.2 functions and patched to
 *      use new array datatypes - QAK, 11/13/00
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tinsert_array(hid_t parent_id, const char *name, size_t offset,
                 int ndims, const size_t dim[/*ndims*/], const int *perm,
                 hid_t member_id)
{
    H5T_t       *parent = NULL;         /*the compound parent data type */
    H5T_t       *member = NULL;         /*the atomic member type        */
    H5T_t       *array = NULL;          /*the array type        */
    hsize_t newdim[H5S_MAX_RANK];   /* Array to hold the dimensions */
    int i;

    FUNC_ENTER(H5Tinsert_array, FAIL);
    H5TRACE7("e","iszIs*[a3]z*Isi",parent_id,name,offset,ndims,dim,perm,
             member_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(parent_id) ||
            NULL == (parent = H5I_object(parent_id)) ||
            H5T_COMPOUND != parent->type) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a compound data type");
    }
    if (H5T_STATE_TRANSIENT!=parent->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "parent type read-only");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no member name");
    }
    if (ndims<0 || ndims>4) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid dimensionality");
    }
    if (ndims>0 && !dim) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no dimensions specified");
    }
    for (i=0; i<ndims; i++) {
        if (dim[i]<1) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid dimension");
        }
        newdim[i]=dim[i];
    }
    if (H5I_DATATYPE != H5I_get_type(member_id) ||
            NULL == (member = H5I_object(member_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }

    /* Create temporary array datatype for inserting field */
    if ((array=H5T_array_create(member,ndims,newdim,perm))==NULL)
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to create array datatype");

    /* Insert */
    if (H5T_insert(parent, name, offset, array) < 0)
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINSERT, FAIL, "unable to insert member");

    /* Close array datatype */
    if (H5T_close(array) < 0)
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINSERT, FAIL, "unable to close array type");
    
    FUNC_LEAVE(SUCCEED);
}
#endif /* WANT_H5_V1_2_COMPAT */


/*-------------------------------------------------------------------------
 * Function:    H5Tpack
 *
 * Purpose:     Recursively removes padding from within a compound data type
 *              to make it more efficient (space-wise) to store that data.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tpack(hid_t type_id)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tpack, FAIL);
    H5TRACE1("e","i",type_id);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id)) ||
        H5T_COMPOUND != dt->type) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a compound data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "data type is read-only");
    }

    /* Pack */
    if (H5T_pack(dt) < 0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "unable to pack compound data type");
    }
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tenum_create
 *
 * Purpose:     Create a new enumeration data type based on the specified
 *              TYPE, which must be an integer type.
 *
 * Return:      Success:        ID of new enumeration data type
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December 22, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tenum_create(hid_t parent_id)
{
    H5T_t       *parent = NULL;         /*base integer data type        */
    H5T_t       *dt = NULL;             /*new enumeration data type     */
    hid_t       ret_value = FAIL;       /*return value                  */
    
    FUNC_ENTER(H5Tenum_create, FAIL);
    H5TRACE1("i","i",parent_id);

    /* Check args */
    if (H5I_DATATYPE!=H5I_get_type(parent_id) ||
        NULL==(parent=H5I_object(parent_id)) ||
        H5T_INTEGER!=parent->type) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an integer data type");
    }

    /* Build new type */
    if (NULL==(dt = H5FL_ALLOC(H5T_t,1))) {
        HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                      "memory allocation failed");
    }
    dt->type = H5T_ENUM;
    dt->parent = H5T_copy(parent, H5T_COPY_ALL);
    dt->size = dt->parent->size;
    dt->ent.header = HADDR_UNDEF;

    /* Atomize the type */
    if ((ret_value=H5I_register(H5I_DATATYPE, dt))<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL,
                      "unable to register data type atom");
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tenum_insert
 *
 * Purpose:     Insert a new enumeration data type member into an enumeration
 *              type. TYPE is the enumeration type, NAME is the name of the
 *              new member, and VALUE points to the value of the new member.
 *              The NAME and VALUE must both be unique within the TYPE. VALUE
 *              points to data which is of the data type defined when the
 *              enumeration type was created.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, December 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tenum_insert(hid_t type, const char *name, void *value)
{
    H5T_t       *dt=NULL;
    
    FUNC_ENTER(H5Tenum_insert, FAIL);
    H5TRACE3("e","isx",type,name,value);

    /* Check args */
    if (H5I_DATATYPE!=H5I_get_type(type) ||
        NULL==(dt=H5I_object(type))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_ENUM!=dt->type) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "not an enumeration data type");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified");
    }
    if (!value) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no value specified");
    }

    /* Do work */
    if (H5T_enum_insert(dt, name, value)<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "unable to insert new enumeration member");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_super
 *
 * Purpose:     Returns the type from which TYPE is derived. In the case of
 *              an enumeration type the return value is an integer type.
 *
 * Return:      Success:        Type ID for base data type.
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, December 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tget_super(hid_t type)
{
    H5T_t       *dt=NULL, *super=NULL;
    hid_t       ret_value=FAIL;
    
    FUNC_ENTER(H5Tget_super, FAIL);
    H5TRACE1("i","i",type);

    if (H5I_DATATYPE!=H5I_get_type(type) ||
        NULL==(dt=H5I_object(type))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (!dt->parent) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "not a derived data type");
    }
    if (NULL==(super=H5T_copy(dt->parent, H5T_COPY_ALL))) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "unable to copy parent data type");
    }
    if ((ret_value=H5I_register(H5I_DATATYPE, super))<0) {
        H5T_close(super);
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL,
                      "unable to register parent data type");
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_member_value
 *
 * Purpose:     Return the value for an enumeration data type member.
 *
 * Return:      Success:        non-negative with the member value copied
 *                              into the memory pointed to by VALUE.
 *
 *              Failure:        negative, VALUE memory is undefined.
 *
 * Programmer:  Robb Matzke
 *              Wednesday, December 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tget_member_value(hid_t type, int membno, void *value/*out*/)
{
    H5T_t       *dt=NULL;
    
    FUNC_ENTER(H5Tget_member_value, FAIL);
    H5TRACE3("i","iIsx",type,membno,value);

    if (H5I_DATATYPE!=H5I_get_type(type) ||
        NULL==(dt=H5I_object(type))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_ENUM!=dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "operation not defined for data type class");
    }
    if (membno<0 || membno>=dt->u.enumer.nmembs) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid member number");
    }
    if (!value) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "null value buffer");
    }

    HDmemcpy(value, dt->u.enumer.value + membno*dt->size, dt->size);

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tenum_nameof
 *
 * Purpose:     Finds the symbol name that corresponds to the specified VALUE
 *              of an enumeration data type TYPE. At most SIZE characters of
 *              the symbol name are copied into the NAME buffer. If the
 *              entire symbol anem and null terminator do not fit in the NAME
 *              buffer then as many characters as possible are copied (not
 *              null terminated) and the function fails.
 *
 * Return:      Success:        Non-negative.
 *
 *              Failure:        Negative, first character of NAME is set to
 *                              null if SIZE allows it.
 *
 * Programmer:  Robb Matzke
 *              Monday, January  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tenum_nameof(hid_t type, void *value, char *name/*out*/, size_t size)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tenum_nameof, FAIL);
    H5TRACE4("e","ixxz",type,value,name,size);

    /* Check args */
    if (H5I_DATATYPE!=H5I_get_type(type) ||
        NULL==(dt=H5I_object(type))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_ENUM!=dt->type) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "not an enumeration data type");
    }
    if (!value) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no value supplied");
    }
    if (!name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name buffer supplied");
    }

    if (NULL==H5T_enum_nameof(dt, value, name, size)) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "nameof query failed");
    }
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tenum_valueof
 *
 * Purpose:     Finds the value that corresponds to the specified NAME f an
 *              enumeration TYPE. The VALUE argument should be at least as
 *              large as the value of H5Tget_size(type) in order to hold the
 *              result.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, January  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tenum_valueof(hid_t type, const char *name, void *value/*out*/)
{
    H5T_t       *dt = NULL;

    FUNC_ENTER(H5Tenum_valueof, FAIL);
    H5TRACE3("e","isx",type,name,value);

    /* Check args */
    if (H5I_DATATYPE!=H5I_get_type(type) ||
        NULL==(dt=H5I_object(type))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_ENUM!=dt->type) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "not an enumeration data type");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name");
    }
    if (!value) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no value buffer");
    }

    if (H5T_enum_valueof(dt, name, value)<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "valueof query failed");
    }
    FUNC_LEAVE(SUCCEED);
}
    

/*-------------------------------------------------------------------------
 * Function:    H5Tvlen_create
 *
 * Purpose:     Create a new variable-length data type based on the specified
 *              BASE_TYPE.
 *
 * Return:      Success:        ID of new VL data type
 *
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, May 20, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tvlen_create(hid_t base_id)
{
    H5T_t       *base = NULL;           /*base data type        */
    H5T_t       *dt = NULL;             /*new VL data type      */
    hid_t       ret_value = FAIL;       /*return value                  */
    
    FUNC_ENTER(H5Tvlen_create, FAIL);
    H5TRACE1("i","i",base_id);

    /* Check args */
    if (H5I_DATATYPE!=H5I_get_type(base_id) ||
        NULL==(base=H5I_object(base_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL,
                      "not an valid base datatype");
    }

    /* Build new type */
    if (NULL==(dt = H5FL_ALLOC(H5T_t,1))) {
        HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                      "memory allocation failed");
    }
    dt->ent.header = HADDR_UNDEF;
    dt->type = H5T_VLEN;

    /*
     * Force conversions (i.e. memory to memory conversions should duplicate
     * data, not point to the same VL sequences)
     */
    dt->force_conv = TRUE;
    dt->parent = H5T_copy(base, H5T_COPY_ALL);

    /* This is a sequence, not a string */
    dt->u.vlen.type = H5T_VLEN_SEQUENCE;

    /* Set up VL information */
    if (H5T_vlen_mark(dt, NULL, H5T_VLEN_MEMORY)<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid VL location");
    }

    /* Atomize the type */
    if ((ret_value=H5I_register(H5I_DATATYPE, dt))<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL,
                      "unable to register datatype");
    }

    FUNC_LEAVE(ret_value);
}



/*-------------------------------------------------------------------------
 * Function:    H5Tset_tag
 *
 * Purpose:     Tag an opaque datatype with a unique ASCII identifier.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, May 20, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_tag(hid_t type_id, const char *tag)
{
    H5T_t       *dt=NULL;

    FUNC_ENTER(H5Tset_tag, FAIL);
    H5TRACE2("e","is",type_id,tag);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (H5T_STATE_TRANSIENT!=dt->state) {
        HRETURN_ERROR(H5E_ARGS, H5E_CANTINIT, FAIL, "data type is read-only");
    }
    if (H5T_OPAQUE!=dt->type) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an opaque data type");
    }
    if (!tag) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no tag");
    }

    /* Commit */
    H5MM_xfree(dt->u.opaque.tag);
    dt->u.opaque.tag = H5MM_strdup(tag);
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_tag
 *
 * Purpose:     Get tha tag associated with an opaque datatype.
 *
 * Return:      A pointer to an allocated string. The caller should free
 *              the string. NULL is returned for errors.
 *
 * Programmer:  Robb Matzke
 *              Thursday, May 20, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
char *
H5Tget_tag(hid_t type_id)
{
    H5T_t       *dt=NULL;
    char        *ret_value=NULL;

    FUNC_ENTER(H5Tget_tag, NULL);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(type_id) ||
        NULL == (dt = H5I_object(type_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a data type");
    }
    if (dt->parent) dt = dt->parent; /*defer to parent*/
    if (H5T_OPAQUE != dt->type) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL,
                      "operation not defined for data type class");
    }
    
    /* result */
    if (NULL==(ret_value=H5MM_strdup(dt->u.opaque.tag))) {
        HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                      "memory allocation failed");
    }
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tregister
 *
 * Purpose:     Register a hard or soft conversion function for a data type
 *              conversion path.  The path is specified by the source and
 *              destination data types SRC_ID and DST_ID (for soft functions
 *              only the class of these types is important). If FUNC is a
 *              hard function then it replaces any previous path; if it's a
 *              soft function then it replaces all existing paths to which it
 *              applies and is used for any new path to which it applies as
 *              long as that path doesn't have a hard function.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, January  9, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tregister(H5T_pers_t pers, const char *name, hid_t src_id, hid_t dst_id,
            H5T_conv_t func)
{
    H5T_t       *src=NULL;              /*source data type descriptor   */
    H5T_t       *dst=NULL;              /*destination data type desc    */
    hid_t       tmp_sid=-1, tmp_did=-1;/*temporary data type IDs        */
    H5T_path_t  *old_path=NULL;         /*existing conversion path      */
    H5T_path_t  *new_path=NULL;         /*new conversion path           */
    H5T_cdata_t cdata;                  /*temporary conversion data     */
    int nprint=0;               /*number of paths shut down     */
    int i;                      /*counter                       */
    herr_t      ret_value=FAIL;         /*return value                  */

    FUNC_ENTER(H5Tregister, FAIL);
    H5TRACE5("e","Tesiix",pers,name,src_id,dst_id,func);

    /* Check args */
    if (H5T_PERS_HARD!=pers && H5T_PERS_SOFT!=pers) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "invalid function persistence");
    }
    if (!name || !*name) {
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                     "conversion must have a name for debugging");
    }
    if (H5I_DATATYPE!=H5I_get_type(src_id) ||
        NULL==(src=H5I_object(src_id)) ||
        H5I_DATATYPE!=H5I_get_type(dst_id) ||
        NULL==(dst=H5I_object(dst_id))) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }
    if (!func) {
        HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                    "no conversion function specified");
    }

    if (H5T_PERS_HARD==pers) {
        /* Locate or create a new conversion path */
        if (NULL==(new_path=H5T_path_find(src, dst, name, func))) {
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                        "unable to locate/allocate conversion path");
        }
        
        /*
         * Notify all other functions to recalculate private data since some
         * functions might cache a list of conversion functions.  For
         * instance, the compound type converter caches a list of conversion
         * functions for the members, so adding a new function should cause
         * the list to be recalculated to use the new function.
         */
        for (i=0; i<H5T_g.npaths; i++) {
            if (new_path != H5T_g.path[i]) {
                H5T_g.path[i]->cdata.recalc = TRUE;
            }
        }
        
    } else {
        /* Add function to end of soft list */
        if (H5T_g.nsoft>=H5T_g.asoft) {
            size_t na = MAX(32, 2*H5T_g.asoft);
            H5T_soft_t *x = H5MM_realloc(H5T_g.soft, na*sizeof(H5T_soft_t));

            if (!x) {
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                    "memory allocation failed");
            }
            H5T_g.asoft = (int)na;
            H5T_g.soft = x;
        }
        HDstrncpy (H5T_g.soft[H5T_g.nsoft].name, name, H5T_NAMELEN);
        H5T_g.soft[H5T_g.nsoft].name[H5T_NAMELEN-1] = '\0';
        H5T_g.soft[H5T_g.nsoft].src = src->type;
        H5T_g.soft[H5T_g.nsoft].dst = dst->type;
        H5T_g.soft[H5T_g.nsoft].func = func;
        H5T_g.nsoft++;

        /*
         * Any existing path (except the no-op path) to which this new soft
         * conversion function applies should be replaced by a new path that
         * uses this function.
         */
        for (i=1; i<H5T_g.npaths; i++) {
            old_path = H5T_g.path[i];
            assert(old_path);

            /* Does the new soft conversion function apply to this path? */
            if (old_path->is_hard ||
                    old_path->src->type!=src->type ||
                    old_path->dst->type!=dst->type) {
                continue;
            }
            if ((tmp_sid = H5I_register(H5I_DATATYPE,
                        H5T_copy(old_path->src, H5T_COPY_ALL)))<0 ||
                    (tmp_did = H5I_register(H5I_DATATYPE,
                        H5T_copy(old_path->dst, H5T_COPY_ALL)))<0) {
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL,
                    "unable to register data types for conv query");
            }
            HDmemset(&cdata, 0, sizeof cdata);
            cdata.command = H5T_CONV_INIT;
            if ((func)(tmp_sid, tmp_did, &cdata, (hsize_t)0, 0, 0, NULL, NULL,
                       H5P_DEFAULT)<0) {
                H5I_dec_ref(tmp_sid);
                H5I_dec_ref(tmp_did);
                tmp_sid = tmp_did = -1;
                H5E_clear();
                continue;
            }

            /* Create a new conversion path */
            if (NULL==(new_path=H5FL_ALLOC(H5T_path_t,1))) {
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                    "memory allocation failed");
            }
            HDstrncpy(new_path->name, name, H5T_NAMELEN);
            new_path->name[H5T_NAMELEN-1] = '\0';
            if (NULL==(new_path->src=H5T_copy(old_path->src, H5T_COPY_ALL)) ||
                NULL==(new_path->dst=H5T_copy(old_path->dst, H5T_COPY_ALL))) {
                HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                    "unable to copy data types");
            }
            new_path->func = func;
            new_path->is_hard = FALSE;
            new_path->cdata = cdata;

            /* Replace previous path */
            H5T_g.path[i] = new_path;
            new_path = NULL; /*so we don't free it on error*/

            /* Free old path */
            H5T_print_stats(old_path, &nprint);
            old_path->cdata.command = H5T_CONV_FREE;
            if ((old_path->func)(tmp_sid, tmp_did, &(old_path->cdata),
                                 (hsize_t)0, 0, 0, NULL, NULL, H5P_DEFAULT)<0) {
#ifdef H5T_DEBUG
                if (H5DEBUG(T)) {
                    fprintf (H5DEBUG(T), "H5T: conversion function 0x%08lx "
                             "failed to free private data for %s (ignored)\n",
                             (unsigned long)(old_path->func), old_path->name);
                }
#endif
            }
            H5T_close(old_path->src);
            H5T_close(old_path->dst);
            H5FL_FREE(H5T_path_t,old_path);

            /* Release temporary atoms */
            H5I_dec_ref(tmp_sid);
            H5I_dec_ref(tmp_did);
            tmp_sid = tmp_did = -1;

            /* We don't care about any failures during the freeing process */
            H5E_clear();
        }
    }
    
    ret_value = SUCCEED;

 done:
    if (ret_value<0) {
        if (new_path) {
            if (new_path->src) H5T_close(new_path->src);
            if (new_path->dst) H5T_close(new_path->dst);
        H5FL_FREE(H5T_path_t,new_path);
        }
        if (tmp_sid>=0) H5I_dec_ref(tmp_sid);
        if (tmp_did>=0) H5I_dec_ref(tmp_did);
    }
    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5T_unregister
 *
 * Purpose:     Removes conversion paths that match the specified criteria.
 *              All arguments are optional. Missing arguments are wild cards.
 *              The special no-op path cannot be removed.
 *
 * Return:      Succeess:       non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, January 13, 1998
 *
 * Modifications:
 *      Adapted to non-API function - QAK, 11/17/99
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_unregister(H5T_pers_t pers, const char *name, H5T_t *src, H5T_t *dst,
              H5T_conv_t func)
{
    H5T_path_t  *path = NULL;           /*conversion path               */
    H5T_soft_t  *soft = NULL;           /*soft conversion information   */
    int nprint=0;               /*number of paths shut down     */
    int i;                      /*counter                       */

    FUNC_ENTER(H5T_unregister, FAIL);

    /* Remove matching entries from the soft list */
    if (H5T_PERS_DONTCARE==pers || H5T_PERS_SOFT==pers) {
        for (i=H5T_g.nsoft-1; i>=0; --i) {
            soft = H5T_g.soft+i;
            assert(soft);
            if (name && *name && HDstrcmp(name, soft->name)) continue;
            if (src && src->type!=soft->src) continue;
            if (dst && dst->type!=soft->dst) continue;
            if (func && func!=soft->func) continue;

            HDmemmove(H5T_g.soft+i, H5T_g.soft+i+1,
                  (H5T_g.nsoft-(i+1)) * sizeof(H5T_soft_t));
            --H5T_g.nsoft;
        }
    }

    /* Remove matching conversion paths, except no-op path */
    for (i=H5T_g.npaths-1; i>0; --i) {
        path = H5T_g.path[i];
        assert(path);
        if ((H5T_PERS_SOFT==pers && path->is_hard) ||
            (H5T_PERS_HARD==pers && !path->is_hard)) continue;
        if (name && *name && HDstrcmp(name, path->name)) continue;
        if (src && H5T_cmp(src, path->src)) continue;
        if (dst && H5T_cmp(dst, path->dst)) continue;
        if (func && func!=path->func) continue;
        
        /* Remove from table */
        HDmemmove(H5T_g.path+i, H5T_g.path+i+1,
              (H5T_g.npaths-(i+1))*sizeof(H5T_path_t*));
        --H5T_g.npaths;

        /* Shut down path */
        H5T_print_stats(path, &nprint);
        path->cdata.command = H5T_CONV_FREE;
        if ((path->func)(FAIL, FAIL, &(path->cdata), (hsize_t)0, 0, 0, NULL, NULL,
                         H5P_DEFAULT)<0) {
#ifdef H5T_DEBUG
            if (H5DEBUG(T)) {
                fprintf(H5DEBUG(T), "H5T: conversion function 0x%08lx failed "
                        "to free private data for %s (ignored)\n",
                        (unsigned long)(path->func), path->name);
            }
#endif
        }
        H5T_close(path->src);
        H5T_close(path->dst);
        H5FL_FREE(H5T_path_t,path);
        H5E_clear(); /*ignore all shutdown errors*/
    }

    FUNC_LEAVE(SUCCEED);
}   /* end H5T_unregister() */

/*-------------------------------------------------------------------------
 * Function:    H5Tunregister
 *
 * Purpose:     Removes conversion paths that match the specified criteria.
 *              All arguments are optional. Missing arguments are wild cards.
 *              The special no-op path cannot be removed.
 *
 * Return:      Succeess:       non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, January 13, 1998
 *
 * Modifications:
 *      Changed to use H5T_unregister wrapper function - QAK, 11/17/99
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tunregister(H5T_pers_t pers, const char *name, hid_t src_id, hid_t dst_id,
              H5T_conv_t func)
{
    H5T_t       *src=NULL, *dst=NULL;   /*data type descriptors         */

    FUNC_ENTER(H5Tunregister, FAIL);
    H5TRACE5("e","Tesiix",pers,name,src_id,dst_id,func);

    /* Check arguments */
    if (src_id>0 && (H5I_DATATYPE!=H5I_get_type(src_id) ||
            NULL==(src=H5I_object(src_id)))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "src is not a data type");
    }
    if (dst_id>0 && (H5I_DATATYPE!=H5I_get_type(dst_id) ||
            NULL==(dst=H5I_object(dst_id)))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "dst is not a data type");
    }

    if (H5T_unregister(pers,name,src,dst,func)<0)
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTDELETE, FAIL,
                      "internal unregister function failed");

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5Tfind
 *
 * Purpose:     Finds a conversion function that can handle a conversion from
 *              type SRC_ID to type DST_ID.  The PCDATA argument is a pointer
 *              to a pointer to type conversion data which was created and
 *              initialized by the type conversion function of this path
 *              when the conversion function was installed on the path.
 *
 * Return:      Success:        A pointer to a suitable conversion function.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, January 13, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_conv_t
H5Tfind(hid_t src_id, hid_t dst_id, H5T_cdata_t **pcdata)
{
    H5T_conv_t  ret_value = NULL;
    H5T_t       *src = NULL, *dst = NULL;
    H5T_path_t  *path = NULL;

    FUNC_ENTER(H5Tfind, NULL);
    H5TRACE3("x","iix",src_id,dst_id,pcdata);

    /* Check args */
    if (H5I_DATATYPE != H5I_get_type(src_id) ||
        NULL == (src = H5I_object(src_id)) ||
        H5I_DATATYPE != H5I_get_type(dst_id) ||
        NULL == (dst = H5I_object(dst_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, NULL, "not a data type");
    }
    if (!pcdata) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, NULL,
                       "no address to receive cdata pointer");
    }
    
    /* Find it */
    if (NULL==(path=H5T_path_find(src, dst, NULL, NULL))) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_NOTFOUND, NULL,
                      "conversion function not found");
    }
    if (pcdata) *pcdata = &(path->cdata);
    ret_value = path->func;
    
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tconvert
 *
 * Purpose:     Convert NELMTS elements from type SRC_ID to type DST_ID.  The
 *              source elements are packed in BUF and on return the
 *              destination will be packed in BUF.  That is, the conversion
 *              is performed in place.  The optional background buffer is an
 *              array of NELMTS values of destination type which are merged
 *              with the converted values to fill in cracks (for instance,
 *              BACKGROUND might be an array of structs with the `a' and `b'
 *              fields already initialized and the conversion of BUF supplies
 *              the `c' and `d' field values).  The PLIST_ID a dataset transfer
 *      property list which is passed to the conversion functions.  (It's
 *      currently only used to pass along the VL datatype custom allocation
 *      information -QAK 7/1/99)
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, June 10, 1998
 *
 * Modifications:
 *              Added xfer_parms argument to pass VL datatype custom allocation
 *              information down the chain.  - QAK, 7/1/99
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tconvert(hid_t src_id, hid_t dst_id, hsize_t nelmts, void *buf,
            void *background, hid_t plist_id)
{
    H5T_path_t          *tpath=NULL;            /*type conversion info  */
    H5T_t               *src=NULL, *dst=NULL;   /*unatomized types      */
    
    FUNC_ENTER (H5Tconvert, FAIL);
    H5TRACE6("e","iihxxi",src_id,dst_id,nelmts,buf,background,plist_id);

    /* Check args */
    if (H5I_DATATYPE!=H5I_get_type(src_id) || NULL==(src=H5I_object(src_id)) ||
        H5I_DATATYPE!=H5I_get_type(dst_id) || NULL==(dst=H5I_object(dst_id)) ||
        (H5P_DEFAULT!=plist_id && H5P_DATASET_XFER!=H5P_get_class(plist_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a data type");
    }

    /* Find the conversion function */
    if (NULL==(tpath=H5T_path_find(src, dst, NULL, NULL))) {
        HRETURN_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL,
                       "unable to convert between src and dst data types");
    }

    if (H5T_convert(tpath, src_id, dst_id, nelmts, 0, 0, buf, background,
                    plist_id)<0) {
        HRETURN_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL,
                       "data type conversion failed");
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tget_overflow
 *
 * Purpose:     Returns a pointer to the current global overflow function.
 *              This is an application-defined function that is called
 *              whenever a data type conversion causes an overflow.
 *
 * Return:      Success:        Ptr to an application-defined function.
 *
 *              Failure:        NULL (this can happen if no overflow handling
 *                              function is registered).
 *
 * Programmer:  Robb Matzke
 *              Tuesday, July  7, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_overflow_t
H5Tget_overflow(void)
{
    FUNC_ENTER(H5Tget_overflow, NULL);
    H5TRACE0("x","");

    if (NULL==H5T_overflow_g) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_UNINITIALIZED, NULL,
                      "no overflow handling function is registered");
    }

    FUNC_LEAVE(H5T_overflow_g);
}


/*-------------------------------------------------------------------------
 * Function:    H5Tset_overflow
 *
 * Purpose:     Sets the overflow handler to be the specified function.  FUNC
 *              will be called for all data type conversions that result in
 *              an overflow.  See the definition of `H5T_overflow_t' for
 *              documentation of arguments and return values.  The NULL
 *              pointer may be passed to remove the overflow handler.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, July  7, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tset_overflow(H5T_overflow_t func)
{
    FUNC_ENTER(H5Tset_overflow, FAIL);
    H5TRACE1("e","x",func);
    H5T_overflow_g = func;
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * API functions are above; library-private functions are below...
 *------------------------------------------------------------------------- 
 */

/*-------------------------------------------------------------------------
 * Function:    H5T_create
 *
 * Purpose:     Creates a new data type and initializes it to reasonable
 *              values.  The new data type is SIZE bytes and an instance of
 *              the class TYPE.
 *
 * Return:      Success:        Pointer to the new type.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, December  5, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_create(H5T_class_t type, size_t size)
{
    H5T_t       *dt = NULL;
    hid_t       subtype;

    FUNC_ENTER(H5T_create, NULL);

    assert(size > 0);

    switch (type) {
        case H5T_INTEGER:
        case H5T_FLOAT:
        case H5T_TIME:
        case H5T_STRING:
        case H5T_BITFIELD:
            HRETURN_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, NULL,
                      "type class is not appropriate - use H5Tcopy()");

        case H5T_OPAQUE:
        case H5T_COMPOUND:
            if (NULL==(dt = H5FL_ALLOC(H5T_t,1))) {
                HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
            }
            dt->type = type;
            break;

        case H5T_ENUM:
            if (sizeof(char)==size) {
                subtype = H5T_NATIVE_SCHAR_g;
            } else if (sizeof(short)==size) {
                subtype = H5T_NATIVE_SHORT_g;
            } else if (sizeof(int)==size) {
                subtype = H5T_NATIVE_INT_g;
            } else if (sizeof(long)==size) {
                subtype = H5T_NATIVE_LONG_g;
            } else if (sizeof(long_long)==size) {
                subtype = H5T_NATIVE_LLONG_g;
            } else {
                HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL,
                      "no applicable native integer type");
            }
            if (NULL==(dt = H5FL_ALLOC(H5T_t,1))) {
                HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                      "memory allocation failed");
            }
            dt->type = type;
            if (NULL==(dt->parent=H5T_copy(H5I_object(subtype),
                                  H5T_COPY_ALL))) {
                H5FL_FREE(H5T_t,dt);
                HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL,
                      "unable to copy base data type");
            }
        break;

        case H5T_VLEN:  /* Variable length datatype */
            HRETURN_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, NULL,
                  "base type required - use H5Tvlen_create()");

        case H5T_ARRAY:  /* Array datatype */
            HRETURN_ERROR(H5E_DATATYPE, H5E_UNSUPPORTED, NULL,
                  "base type required - use H5Tarray_create()");

        default:
            HRETURN_ERROR(H5E_INTERNAL, H5E_UNSUPPORTED, NULL,
                  "unknown data type class");
    }

    dt->ent.header = HADDR_UNDEF;
    dt->size = size;
    FUNC_LEAVE(dt);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_isa
 *
 * Purpose:     Determines if an object has the requisite messages for being
 *              a data type.
 *
 * Return:      Success:        TRUE if the required data type messages are
 *                              present; FALSE otherwise.
 *
 *              Failure:        FAIL if the existence of certain messages
 *                              cannot be determined.
 *
 * Programmer:  Robb Matzke
 *              Monday, November  2, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5T_isa(H5G_entry_t *ent)
{
    htri_t      exists;
    
    FUNC_ENTER(H5T_isa, FAIL);
    assert(ent);

    if ((exists=H5O_exists(ent, H5O_DTYPE, 0))<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "unable to read object header");
    }
    FUNC_LEAVE(exists);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_open
 *
 * Purpose:     Open a named data type.
 *
 * Return:      Success:        Ptr to a new data type.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Monday, June  1, 1998
 *
 * Modifications:
 *      Changed to use H5T_open_oid - QAK - 3/17/99
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_open (H5G_entry_t *loc, const char *name)
{
    H5T_t       *dt = NULL;
    H5G_entry_t ent;
    
    FUNC_ENTER (H5T_open, NULL);
    assert (loc);
    assert (name && *name);

    /*
     * Find the named data type object header and read the data type message
     * from it.
     */
    if (H5G_find (loc, name, NULL, &ent/*out*/)<0) {
        HRETURN_ERROR (H5E_DATATYPE, H5E_NOTFOUND, NULL, "not found");
    }
    /* Open the datatype object */
    if ((dt=H5T_open_oid(&ent)) ==NULL) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_NOTFOUND, NULL, "not found");
    }

    FUNC_LEAVE (dt);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_open_oid
 *
 * Purpose:     Open a named data type.
 *
 * Return:      Success:        Ptr to a new data type.
 *
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, March 17, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_open_oid (H5G_entry_t *ent)
{
    H5T_t       *dt = NULL;
    
    FUNC_ENTER (H5T_open_oid, NULL);
    assert (ent);

    if (H5O_open (ent)<0) {
        HRETURN_ERROR (H5E_DATATYPE, H5E_CANTOPENOBJ, NULL,
                       "unable to open named data type");
    }
    if (NULL==(dt=H5O_read (ent, H5O_DTYPE, 0, NULL))) {
        H5O_close(ent);
        HRETURN_ERROR (H5E_DATATYPE, H5E_CANTINIT, NULL,
                       "unable to load type message from object header");
    }

    /* Mark the type as named and open */
    dt->state = H5T_STATE_OPEN;
    dt->ent = *ent;

    FUNC_LEAVE (dt);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_copy
 *
 * Purpose:     Copies datatype OLD_DT.  The resulting data type is not
 *              locked and is a transient type.
 *
 * Return:      Success:        Pointer to a new copy of the OLD_DT argument.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Thursday, December  4, 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 4 Jun 1998
 *      Added the METHOD argument.  If it's H5T_COPY_TRANSIENT then the
 *      result will be an unlocked transient type.  Otherwise if it's
 *      H5T_COPY_ALL then the result is a named type if the original is a
 *      named type, but the result is not opened.  Finally, if it's
 *      H5T_COPY_REOPEN and the original type is a named type then the result
 *      is a named type and the type object header is opened again.  The
 *      H5T_COPY_REOPEN method is used when returning a named type to the
 *      application.
 *
 *      Robb Matzke, 22 Dec 1998
 *      Now able to copy enumeration data types.
 *
 *      Robb Matzke, 20 May 1999
 *      Now able to copy opaque types.
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_copy(const H5T_t *old_dt, H5T_copy_t method)
{
    H5T_t       *new_dt=NULL, *tmp=NULL;
    int i;
    char        *s;

    FUNC_ENTER(H5T_copy, NULL);

    /* check args */
    assert(old_dt);

    /* Allocate space */
    if (NULL==(new_dt = H5FL_ALLOC(H5T_t,0))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }

    /* Copy actual information */
    *new_dt = *old_dt;

    /* Copy parent information */
    if (new_dt->parent)
        new_dt->parent = H5T_copy(new_dt->parent, method);

    /* Check what sort of copy we are making */
    switch (method) {
        case H5T_COPY_TRANSIENT:
            /*
             * Return an unlocked transient type.
             */
            new_dt->state = H5T_STATE_TRANSIENT;
            HDmemset (&(new_dt->ent), 0, sizeof(new_dt->ent));
            new_dt->ent.header = HADDR_UNDEF;
            break;
        
        case H5T_COPY_ALL:
            /*
             * Return a transient type (locked or unlocked) or an unopened named
             * type.  Immutable transient types are degraded to read-only.
             */
            if (H5T_STATE_OPEN==new_dt->state) {
                new_dt->state = H5T_STATE_NAMED;
            } else if (H5T_STATE_IMMUTABLE==new_dt->state) {
                new_dt->state = H5T_STATE_RDONLY;
            }
            break;

        case H5T_COPY_REOPEN:
            /*
             * Return a transient type (locked or unlocked) or an opened named
             * type.
             */
            if (H5F_addr_defined(new_dt->ent.header)) {
                if (H5O_open (&(new_dt->ent))<0) {
                    H5FL_FREE (H5T_t,new_dt);
                    HRETURN_ERROR (H5E_DATATYPE, H5E_CANTOPENOBJ, NULL,
                           "unable to reopen named data type");
                }
                new_dt->state = H5T_STATE_OPEN;
            }
            break;
    } /* end switch */
    
    switch(new_dt->type) {
        case H5T_COMPOUND:
            {
            int accum_change=0;    /* Amount of change in the offset of the fields */

            /*
             * Copy all member fields to new type, then overwrite the
             * name and type fields of each new member with copied values.
             * That is, H5T_copy() is a deep copy.
             */
            new_dt->u.compnd.memb = H5MM_malloc(new_dt->u.compnd.nalloc *
                                sizeof(H5T_cmemb_t));
            if (NULL==new_dt->u.compnd.memb) {
                HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
            }

            HDmemcpy(new_dt->u.compnd.memb, old_dt->u.compnd.memb,
                 new_dt->u.compnd.nmembs * sizeof(H5T_cmemb_t));

            for (i=0; i<new_dt->u.compnd.nmembs; i++) {
                int     j;
                int    old_match;

                s = new_dt->u.compnd.memb[i].name;
                new_dt->u.compnd.memb[i].name = H5MM_xstrdup(s);
                tmp = H5T_copy (old_dt->u.compnd.memb[i].type, method);
                new_dt->u.compnd.memb[i].type = tmp;

                /* Apply the accumulated size change to the offset of the field */
                new_dt->u.compnd.memb[i].offset += accum_change;

                if(old_dt->u.compnd.sorted != H5T_SORT_VALUE) {
                    for (old_match=-1, j=0; j<old_dt->u.compnd.nmembs; j++) {
                        if(!HDstrcmp(new_dt->u.compnd.memb[i].name,old_dt->u.compnd.memb[j].name)) {
                            old_match=j;
                            break;
                        } /* end if */
                    } /* end for */

                    /* check if we couldn't find a match */
                    if(old_match<0)
                        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTCOPY, NULL, "fields in datatype corrupted");
                } /* end if */
                else {
                    old_match=i;
                } /* end else */

                /* If the field changed size, add that change to the accumulated size change */
                if(new_dt->u.compnd.memb[i].type->size != old_dt->u.compnd.memb[old_match].type->size) {
                    /* Adjust the size of the member */
                    new_dt->u.compnd.memb[i].size = (old_dt->u.compnd.memb[old_match].size*tmp->size)/old_dt->u.compnd.memb[old_match].type->size;

                    accum_change += (new_dt->u.compnd.memb[i].type->size - old_dt->u.compnd.memb[old_match].type->size);
                } /* end if */
            } /* end for */

            /* Apply the accumulated size change to the size of the compound struct */
            new_dt->size += accum_change;

            }
            break;

        case H5T_ENUM:
            /*
             * Copy all member fields to new type, then overwrite the name fields
             * of each new member with copied values. That is, H5T_copy() is a
             * deep copy.
             */
            new_dt->u.enumer.name = H5MM_malloc(new_dt->u.enumer.nalloc *
                                sizeof(char*));
            new_dt->u.enumer.value = H5MM_malloc(new_dt->u.enumer.nalloc *
                                 new_dt->size);
            if (NULL==new_dt->u.enumer.value) {
                HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                      "memory allocation failed");
            }
            HDmemcpy(new_dt->u.enumer.value, old_dt->u.enumer.value,
                 new_dt->u.enumer.nmembs * new_dt->size);
            for (i=0; i<new_dt->u.enumer.nmembs; i++) {
                s = old_dt->u.enumer.name[i];
                new_dt->u.enumer.name[i] = H5MM_xstrdup(s);
            } 
            break;

        case H5T_VLEN:
            if(method==H5T_COPY_TRANSIENT || method==H5T_COPY_REOPEN) {
                /* H5T_copy converts any VL type into a memory VL type */
                if (H5T_vlen_mark(new_dt, NULL, H5T_VLEN_MEMORY)<0) {
                    HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL, "invalid VL location");
                }
            }
            break;

        case H5T_OPAQUE:
            /*
             * Copy the tag name.
             */
            new_dt->u.opaque.tag = HDstrdup(new_dt->u.opaque.tag);
            break;

        case H5T_ARRAY:
            /* Re-compute the array's size, in case it's base type changed size */
            new_dt->size=new_dt->u.array.nelem*new_dt->parent->size;
            break;

        default:
            break;
    } /* end switch */
    
    FUNC_LEAVE(new_dt);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_commit
 *
 * Purpose:     Commit a type, giving it a name and causing it to become
 *              immutable.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, June  1, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_commit (H5G_entry_t *loc, const char *name, H5T_t *type)
{
    herr_t      ret_value = FAIL;
    H5F_t       *file = NULL;
    
    FUNC_ENTER (H5T_commit, FAIL);

    /*
     * Check arguments.  We cannot commit an immutable type because H5Tclose()
     * normally fails on such types (try H5Tclose(H5T_NATIVE_INT)) but closing
     * a named type should always succeed.
     */
    assert (loc);
    assert (name && *name);
    assert (type);
    if (H5T_STATE_NAMED==type->state || H5T_STATE_OPEN==type->state) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "data type is already committed");
    }
    if (H5T_STATE_IMMUTABLE==type->state) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "data type is immutable");
    }

    /* Find the insertion file */
    if (NULL==(file=H5G_insertion_file(loc, name))) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL,
                      "unable to find insertion point");
    }

    /*
     * Create the object header and open it for write access. Insert the data
     * type message and then give the object header a name.
     */
    if (H5O_create (file, 64, &(type->ent))<0) {
        HGOTO_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL,
                     "unable to create data type object header");
    }
    if (H5O_modify (&(type->ent), H5O_DTYPE, 0, H5O_FLAG_CONSTANT, type)<0) {
        HGOTO_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL,
                     "unable to update type header message");
    }
    if (H5G_insert (loc, name, &(type->ent))<0) {
        HGOTO_ERROR (H5E_DATATYPE, H5E_CANTINIT, FAIL,
                     "unable to name data type");
    }
    type->state = H5T_STATE_OPEN;
    ret_value = SUCCEED;

 done:
    if (ret_value<0) {
        if (H5F_addr_defined(type->ent.header)) {
            H5O_close(&(type->ent));
            type->ent.header = HADDR_UNDEF;
        }
    }
    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_lock
 *
 * Purpose:     Lock a transient data type making it read-only.  If IMMUTABLE
 *              is set then the type cannot be closed except when the library
 *              itself closes.
 *
 *              This function is a no-op if the type is not transient or if
 *              the type is already read-only or immutable.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, June  4, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_lock (H5T_t *dt, hbool_t immutable)
{
    FUNC_ENTER (H5T_lock, FAIL);
    assert (dt);

    switch (dt->state) {
    case H5T_STATE_TRANSIENT:
        dt->state = immutable ? H5T_STATE_IMMUTABLE : H5T_STATE_RDONLY;
        break;
    case H5T_STATE_RDONLY:
        if (immutable) dt->state = H5T_STATE_IMMUTABLE;
        break;
    case H5T_STATE_IMMUTABLE:
    case H5T_STATE_NAMED:
    case H5T_STATE_OPEN:
        /*void*/
        break;
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_close
 *
 * Purpose:     Frees a data type and all associated memory.  If the data
 *              type is locked then nothing happens.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, December  8, 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-04-27
 *              This function fails if the datatype state is IMMUTABLE.
 *
 *              Robb Matzke, 1999-05-20
 *              Closes opaque types also.
 *-------------------------------------------------------------------------
 */
herr_t
H5T_close(H5T_t *dt)
{
    int i;
    H5T_t       *parent = dt->parent;

    FUNC_ENTER(H5T_close, FAIL);
    assert(dt);

    /*
     * If a named type is being closed then close the object header also.
     */
    if (H5T_STATE_OPEN==dt->state) {
        assert (H5F_addr_defined(dt->ent.header));
        if (H5O_close(&(dt->ent))<0) {
            HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                          "unable to close data type object header");
        }
        dt->state = H5T_STATE_NAMED;
    }

    /*
     * Don't free locked datatypes.
     */
    if (H5T_STATE_IMMUTABLE==dt->state) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CLOSEERROR, FAIL,
                      "unable to close immutable datatype");
    }

    /* Close the datatype */
    switch (dt->type) {
        case H5T_COMPOUND:
            for (i=0; i<dt->u.compnd.nmembs; i++) {
                H5MM_xfree(dt->u.compnd.memb[i].name);
                H5T_close(dt->u.compnd.memb[i].type);
            }
            H5MM_xfree(dt->u.compnd.memb);
            break;

        case H5T_ENUM:
            for (i=0; i<dt->u.enumer.nmembs; i++)
                H5MM_xfree(dt->u.enumer.name[i]);
            H5MM_xfree(dt->u.enumer.name);
            H5MM_xfree(dt->u.enumer.value);
            break;

        case H5T_OPAQUE:
            H5MM_xfree(dt->u.opaque.tag);
            break;

        default:
            break;
    }

    /* Free the datatype struct */
    H5FL_FREE(H5T_t,dt);

    /* Close the parent */
    if (parent && H5T_close(parent)<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "unable to close parent data type");
    }
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_is_atomic
 *
 * Purpose:     Determines if a data type is an atomic type.
 *
 * Return:      Success:        TRUE, FALSE
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5T_is_atomic(const H5T_t *dt)
{
    htri_t      ret_value = FAIL;
    
    FUNC_ENTER(H5T_is_atomic, FAIL);

    assert(dt);
    if (H5T_COMPOUND!=dt->type && H5T_ENUM!=dt->type && H5T_VLEN!=dt->type && H5T_OPAQUE!=dt->type && H5T_ARRAY!=dt->type) {
        ret_value = TRUE;
    } else {
        ret_value = FALSE;
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_set_size
 *
 * Purpose:     Sets the total size in bytes for a data type (this operation
 *              is not permitted on compound data types).  If the size is
 *              decreased so that the significant bits of the data type
 *              extend beyond the edge of the new size, then the `offset'
 *              property is decreased toward zero.  If the `offset' becomes
 *              zero and the significant bits of the data type still hang
 *              over the edge of the new size, then the number of significant
 *              bits is decreased.
 *
 *              Adjusting the size of an H5T_STRING automatically sets the
 *              precision to 8*size.
 *
 *              All data types have a positive size.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        nagative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December 22, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works with derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_set_size(H5T_t *dt, size_t size)
{
    size_t      prec, offset;

    FUNC_ENTER(H5T_set_size, FAIL);

    /* Check args */
    assert(dt);
    assert(size!=0);
    assert(H5T_ENUM!=dt->type || 0==dt->u.enumer.nmembs);

    if (dt->parent) {
        if (H5T_set_size(dt->parent, size)<0) {
            HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                  "unable to set size for parent data type");
        }
        dt->size = dt->parent->size;
    } else {
        if (H5T_is_atomic(dt)) {
            offset = dt->u.atomic.offset;
            prec = dt->u.atomic.prec;

            /* Decrement the offset and precision if necessary */
            if (prec > 8*size)
                offset = 0;
            else
                if (offset+prec > 8*size)
                    offset = 8 * size - prec;
            if (prec > 8*size)
                prec = 8 * size;
        } else {
            prec = offset = 0;
        }

        switch (dt->type) {
            case H5T_COMPOUND:
            case H5T_ARRAY:
                HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                      "unable to set size of specified data type");

            case H5T_INTEGER:
            case H5T_TIME:
            case H5T_BITFIELD:
            case H5T_ENUM:
            case H5T_OPAQUE:
                /* nothing to check */
                break;

            case H5T_STRING:
                /* Convert string to variable-length datatype */
                if(size==H5T_VARIABLE) {
                    H5T_t       *base = NULL;           /* base data type */

                    /* Get a copy of unsigned char type as the base/parent type */
                    if (NULL==(base=H5I_object(H5T_NATIVE_UCHAR)))
                        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "invalid base datatype");
                    dt->parent=H5T_copy(base,H5T_COPY_ALL);

                    /* change this datatype into a VL string */
                    dt->type = H5T_VLEN;

                    /*
                     * Force conversions (i.e. memory to memory conversions should duplicate
                     * data, not point to the same VL strings)
                     */
                    dt->force_conv = TRUE;

                    /* This is a string, not a sequence */
                    dt->u.vlen.type = H5T_VLEN_STRING;

                    /* Set up VL information */
                    if (H5T_vlen_mark(dt, NULL, H5T_VLEN_MEMORY)<0)
                        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL, "invalid VL location");

                } else {
                    prec = 8 * size;
                    offset = 0;
                } /* end else */
                break;

            case H5T_FLOAT:
                /*
                 * The sign, mantissa, and exponent fields should be adjusted
                 * first when decreasing the size of a floating point type.
                 */
                if (dt->u.atomic.u.f.sign >= prec ||
                        dt->u.atomic.u.f.epos + dt->u.atomic.u.f.esize > prec ||
                        dt->u.atomic.u.f.mpos + dt->u.atomic.u.f.msize > prec) {
                    HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                          "adjust sign, mantissa, and exponent fields first");
                }
                break;

            default:
                assert("not implemented yet" && 0);
        }

        /* Commit */
        if(dt->type!=H5T_VLEN) {
            dt->size = size;
            if (H5T_is_atomic(dt)) {
                dt->u.atomic.offset = offset;
                dt->u.atomic.prec = prec;
            }
        }
    }
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_get_size
 *
 * Purpose:     Determines the total size of a data type in bytes.
 *
 * Return:      Success:        Size of the data type in bytes.  The size of
 *                              the data type is the size of an instance of
 *                              that data type.
 *
 *              Failure:        0 (valid data types are never zero size)
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December  9, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
size_t
H5T_get_size(const H5T_t *dt)
{
    FUNC_ENTER(H5T_get_size, 0);

    /* check args */
    assert(dt);

    FUNC_LEAVE(dt->size);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_set_precision
 *
 * Purpose:     Sets the precision of a data type.  The precision is
 *              the number of significant bits which, unless padding is
 *              present, is 8 times larger than the value returned by
 *              H5Tget_size().
 *
 *              If the precision is increased then the offset is decreased
 *              and then the size is increased to insure that significant
 *              bits do not "hang over" the edge of the data type.
 *
 *              The precision property of strings is read-only.
 *
 *              When decreasing the precision of a floating point type, set
 *              the locations and sizes of the sign, mantissa, and exponent
 *              fields first.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_set_precision(H5T_t *dt, size_t prec)
{
    size_t      offset, size;

    FUNC_ENTER(H5T_set_precision, FAIL);

    /* Check args */
    assert(dt);
    assert(prec>0);
    assert(H5T_ENUM!=dt->type || 0==dt->u.enumer.nmembs);

    if (dt->parent) {
        if (H5T_set_precision(dt->parent, prec)<0) {
            HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                          "unable to set precision for base type");
        }
        dt->size = dt->parent->size;
    } else {
        if (H5T_COMPOUND==dt->type || H5T_OPAQUE==dt->type || H5T_ARRAY==dt->type) {
            HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                          "operation not defined for specified data type");

        } else if (H5T_ENUM==dt->type) {
            /*nothing*/

        } else if (H5T_is_atomic(dt)) {
            /* Adjust the offset and size */
            offset = dt->u.atomic.offset;
            size = dt->size;
            if (prec > 8*size) offset = 0;
            else if (offset+prec > 8 * size) offset = 8 * size - prec;
            if (prec > 8*size) size = (prec+7) / 8;

            /* Check that things are still kosher */
            switch (dt->type) {
            case H5T_INTEGER:
            case H5T_TIME:
            case H5T_BITFIELD:
                /* nothing to check */
                break;

            case H5T_STRING:
                HRETURN_ERROR(H5E_ARGS, H5E_UNSUPPORTED, FAIL,
                              "precision for this type is read-only");

            case H5T_FLOAT:
                /*
                 * The sign, mantissa, and exponent fields should be adjusted
                 * first when decreasing the precision of a floating point
                 * type.
                 */
                if (dt->u.atomic.u.f.sign >= prec ||
                    dt->u.atomic.u.f.epos + dt->u.atomic.u.f.esize > prec ||
                    dt->u.atomic.u.f.mpos + dt->u.atomic.u.f.msize > prec) {
                    HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                                  "adjust sign, mantissa, and exponent fields "
                                  "first");
                }
                break;

            default:
                assert("not implemented yet" && 0);
            }

            /* Commit */
            dt->size = size;
            if (H5T_is_atomic(dt)) {
                dt->u.atomic.offset = offset;
                dt->u.atomic.prec = prec;
            }
        }
    }
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_set_offset
 *
 * Purpose:     Sets the bit offset of the first significant bit.  The
 *              signficant bits of an atomic datum can be offset from the
 *              beginning of the memory for that datum by an amount of
 *              padding. The `offset' property specifies the number of bits
 *              of padding that appear to the "right of" the value.  That is,
 *              if we have a 32-bit datum with 16-bits of precision having
 *              the value 0x1122 then it will be layed out in memory as (from
 *              small byte address toward larger byte addresses):
 *
 *                  Big      Big       Little   Little
 *                  Endian   Endian    Endian   Endian
 *                  offset=0 offset=16 offset=0 offset=16
 *
 *              0:  [ pad]   [0x11]    [0x22]   [ pad]
 *              1:  [ pad]   [0x22]    [0x11]   [ pad]
 *              2:  [0x11]   [ pad]    [ pad]   [0x22]
 *              3:  [0x22]   [ pad]    [ pad]   [0x11]
 *
 *              If the offset is incremented then the total size is
 *              incremented also if necessary to prevent significant bits of
 *              the value from hanging over the edge of the data type.
 *
 *              The offset of an H5T_STRING cannot be set to anything but
 *              zero. 
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Also works for derived data types.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_set_offset(H5T_t *dt, size_t offset)
{
    FUNC_ENTER(H5T_set_offset, FAIL);

    /* Check args */
    assert(dt);
    assert(H5T_STRING!=dt->type || 0==offset);
    assert(H5T_ENUM!=dt->type || 0==dt->u.enumer.nmembs);

    if (dt->parent) {
        if (H5T_set_offset(dt->parent, offset)<0) {
            HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                          "unable to set offset for base type");
        }
        dt->size = dt->parent->size;
    } else {
        if (H5T_COMPOUND==dt->type || H5T_OPAQUE==dt->type || H5T_ARRAY==dt->type) {
            HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                          "operation not defined for specified data type");
        } else if (H5T_ENUM==dt->type) {
            /*nothing*/
        } else {
            if (offset+dt->u.atomic.prec > 8*dt->size) {
                dt->size = (offset + dt->u.atomic.prec + 7) / 8;
            }
            dt->u.atomic.offset = offset;
        }
    }
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_insert
 *
 * Purpose:     Adds a new MEMBER to the compound data type PARENT.  The new
 *              member will have a NAME that is unique within PARENT and an
 *              instance of PARENT will have the member begin at byte offset
 *              OFFSET from the beginning.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, December  8, 1997
 *
 * Modifications:
 *  Took out arrayness parameters - QAK, 10/6/00
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_insert(H5T_t *parent, const char *name, size_t offset, const H5T_t *member)
{
    int         idx, i;
    size_t              total_size;
    
    FUNC_ENTER(H5T_insert, FAIL);

    /* check args */
    assert(parent && H5T_COMPOUND == parent->type);
    assert(H5T_STATE_TRANSIENT==parent->state);
    assert(member);
    assert(name && *name);

    /* Does NAME already exist in PARENT? */
    for (i=0; i<parent->u.compnd.nmembs; i++) {
        if (!HDstrcmp(parent->u.compnd.memb[i].name, name)) {
            HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINSERT, FAIL,
                          "member name is not unique");
        }
    }

    /* Does the new member overlap any existing member ? */
    total_size=member->size;
    for (i=0; i<parent->u.compnd.nmembs; i++) {
        if ((offset <= parent->u.compnd.memb[i].offset &&
             offset + total_size > parent->u.compnd.memb[i].offset) ||
            (parent->u.compnd.memb[i].offset <= offset &&
             parent->u.compnd.memb[i].offset +
             parent->u.compnd.memb[i].size > offset)) {
            HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINSERT, FAIL,
                          "member overlaps with another member");
        }
    }

    /* Does the new member overlap the end of the compound type? */
    if(offset+total_size>parent->size)
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINSERT, FAIL, "member extends past end of compound type");

    /* Increase member array if necessary */
    if (parent->u.compnd.nmembs >= parent->u.compnd.nalloc) {
        size_t na = parent->u.compnd.nalloc + H5T_COMPND_INC;
        H5T_cmemb_t *x = H5MM_realloc (parent->u.compnd.memb,
                           na * sizeof(H5T_cmemb_t));

        if (!x) {
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                   "memory allocation failed");
        }
        parent->u.compnd.nalloc = (int)na;
        parent->u.compnd.memb = x;
    }

    /* Add member to end of member array */
    idx = parent->u.compnd.nmembs;
    parent->u.compnd.memb[idx].name = H5MM_xstrdup(name);
    parent->u.compnd.memb[idx].offset = offset;
    parent->u.compnd.memb[idx].size = total_size;
    parent->u.compnd.memb[idx].type = H5T_copy (member, H5T_COPY_ALL);

    parent->u.compnd.sorted = H5T_SORT_NONE;
    parent->u.compnd.nmembs++;

    /*
     * Set the "force conversion" flag if VL datatype fields exist in this type
     * or any component types
     */
    if(member->type==H5T_VLEN || member->force_conv==TRUE)
        parent->force_conv=TRUE;

    /* Set the flag for this compound type, if the field is an array */
    if(member->type==H5T_ARRAY)
        parent->u.compnd.has_array=TRUE;

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_pack
 *
 * Purpose:     Recursively packs a compound data type by removing padding
 *              bytes. This is done in place (that is, destructively).
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_pack(H5T_t *dt)
{
    int         i;
    size_t      offset;

    FUNC_ENTER(H5T_pack, FAIL);

    assert(dt);

    if (H5T_COMPOUND == dt->type) {
        assert(H5T_STATE_TRANSIENT==dt->state);
        
        /* Recursively pack the members */
        for (i=0; i<dt->u.compnd.nmembs; i++) {
            if (H5T_pack(dt->u.compnd.memb[i].type) < 0) {
                HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                              "unable to pack part of a compound data type");
            }
        }

        /* Remove padding between members */
        H5T_sort_value(dt, NULL);
        for (i=0, offset=0; i<dt->u.compnd.nmembs; i++) {
            dt->u.compnd.memb[i].offset = offset;
            offset += dt->u.compnd.memb[i].size;
        }

        /* Change total size */
        dt->size = MAX(1, offset);
    }
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_sort_value
 *
 * Purpose:     Sorts the members of a compound data type by their offsets;
 *              sorts the members of an enum type by their values. This even
 *              works for locked data types since it doesn't change the value
 *              of the type.  MAP is an optional parallel integer array which
 *              is also swapped along with members of DT.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_sort_value(H5T_t *dt, int *map)
{
    int         i, j, nmembs;
    size_t      size;
    hbool_t     swapped;
    uint8_t     tbuf[32];

    FUNC_ENTER(H5T_sort_value, FAIL);

    /* Check args */
    assert(dt);
    assert(H5T_COMPOUND==dt->type || H5T_ENUM==dt->type);

    /* Use a bubble sort because we can short circuit */
    if (H5T_COMPOUND==dt->type) {
        if (H5T_SORT_VALUE!=dt->u.compnd.sorted) {
            dt->u.compnd.sorted = H5T_SORT_VALUE;
            nmembs = dt->u.compnd.nmembs;
            for (i=nmembs-1, swapped=TRUE; i>0 && swapped; --i) {
                for (j=0, swapped=FALSE; j<i; j++) {
                    if (dt->u.compnd.memb[j].offset >
                        dt->u.compnd.memb[j+1].offset) {
                        H5T_cmemb_t tmp = dt->u.compnd.memb[j];
                        dt->u.compnd.memb[j] = dt->u.compnd.memb[j+1];
                        dt->u.compnd.memb[j+1] = tmp;
                        if (map) {
                            int x = map[j];
                            map[j] = map[j+1];
                            map[j+1] = x;
                        }
                        swapped = TRUE;
                    }
                }
            }
#ifndef NDEBUG
            /* I never trust a sort :-) -RPM */
            for (i=0; i<nmembs-1; i++) {
                assert(dt->u.compnd.memb[i].offset <
                       dt->u.compnd.memb[i+1].offset);
            }
#endif
        }
    } else if (H5T_ENUM==dt->type) {
        if (H5T_SORT_VALUE!=dt->u.enumer.sorted) {
            dt->u.enumer.sorted = H5T_SORT_VALUE;
            nmembs = dt->u.enumer.nmembs;
            size = dt->size;
            assert(size<=sizeof(tbuf));
            for (i=nmembs-1, swapped=TRUE; i>0 && swapped; --i) {
                for (j=0, swapped=FALSE; j<i; j++) {
                    if (HDmemcmp(dt->u.enumer.value+j*size,
                                 dt->u.enumer.value+(j+1)*size,
                                 size)>0) {
                        /* Swap names */
                        char *tmp = dt->u.enumer.name[j];
                        dt->u.enumer.name[j] = dt->u.enumer.name[j+1];
                        dt->u.enumer.name[j+1] = tmp;

                        /* Swap values */
                        HDmemcpy(tbuf, dt->u.enumer.value+j*size, size);
                        HDmemcpy(dt->u.enumer.value+j*size,
                                 dt->u.enumer.value+(j+1)*size, size);
                        HDmemcpy(dt->u.enumer.value+(j+1)*size, tbuf, size);
                        
                        /* Swap map */
                        if (map) {
                            int x = map[j];
                            map[j] = map[j+1];
                            map[j+1] = x;
                        }

                        swapped = TRUE;
                    }
                }
            }
#ifndef NDEBUG
            /* I never trust a sort :-) -RPM */
            for (i=0; i<nmembs-1; i++) {
                assert(HDmemcmp(dt->u.enumer.value+i*size,
                                dt->u.enumer.value+(i+1)*size,
                                size)<0);
            }
#endif
        }
    }
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_sort_name
 *
 * Purpose:     Sorts members of a compound or enumeration data type by their
 *              names. This even works for locked data types since it doesn't
 *              change the value of the types.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, January  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_sort_name(H5T_t *dt, int *map)
{
    int         i, j, nmembs;
    size_t      size;
    hbool_t     swapped;
    uint8_t     tbuf[32];

    FUNC_ENTER(H5T_sort_name, FAIL);

    /* Check args */
    assert(dt);
    assert(H5T_COMPOUND==dt->type || H5T_ENUM==dt->type);

    /* Use a bubble sort because we can short circuit */
    if (H5T_COMPOUND==dt->type) {
        if (H5T_SORT_NAME!=dt->u.compnd.sorted) {
            dt->u.compnd.sorted = H5T_SORT_NAME;
            nmembs = dt->u.compnd.nmembs;
            for (i=nmembs-1, swapped=TRUE; i>0 && swapped; --i) {
                for (j=0, swapped=FALSE; j<i; j++) {
                    if (HDstrcmp(dt->u.compnd.memb[j].name,
                                 dt->u.compnd.memb[j+1].name)>0) {
                        H5T_cmemb_t tmp = dt->u.compnd.memb[j];
                        dt->u.compnd.memb[j] = dt->u.compnd.memb[j+1];
                        dt->u.compnd.memb[j+1] = tmp;
                        swapped = TRUE;
                        if (map) {
                            int x = map[j];
                            map[j] = map[j+1];
                            map[j+1] = x;
                        }
                    }
                }
            }
#ifndef NDEBUG
            /* I never trust a sort :-) -RPM */
            for (i=0; i<nmembs-1; i++) {
                assert(HDstrcmp(dt->u.compnd.memb[i].name, 
                                dt->u.compnd.memb[i+1].name)<0);
            }
#endif
        }
    } else if (H5T_ENUM==dt->type) {
        if (H5T_SORT_NAME!=dt->u.enumer.sorted) {
            dt->u.enumer.sorted = H5T_SORT_NAME;
            nmembs = dt->u.enumer.nmembs;
            size = dt->size;
            assert(size<=sizeof(tbuf));
            for (i=nmembs-1, swapped=TRUE; i>0 && swapped; --i) {
                for (j=0, swapped=FALSE; j<i; j++) {
                    if (HDstrcmp(dt->u.enumer.name[j],
                                 dt->u.enumer.name[j+1])>0) {
                        /* Swap names */
                        char *tmp = dt->u.enumer.name[j];
                        dt->u.enumer.name[j] = dt->u.enumer.name[j+1];
                        dt->u.enumer.name[j+1] = tmp;

                        /* Swap values */
                        HDmemcpy(tbuf, dt->u.enumer.value+j*size, size);
                        HDmemcpy(dt->u.enumer.value+j*size,
                                 dt->u.enumer.value+(j+1)*size, size);
                        HDmemcpy(dt->u.enumer.value+(j+1)*size, tbuf, size);

                        /* Swap map */
                        if (map) {
                            int x = map[j];
                            map[j] = map[j+1];
                            map[j+1] = x;
                        }

                        swapped = TRUE;
                    }
                }
            }
#ifndef NDEBUG
            /* I never trust a sort :-) -RPM */
            for (i=0; i<nmembs-1; i++) {
                assert(HDstrcmp(dt->u.enumer.name[i],
                                dt->u.enumer.name[i+1])<0);
            }
#endif
        }
    }
    
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_enum_insert
 *
 * Purpose:     Insert a new member having a NAME and VALUE into an
 *              enumeration data TYPE.  The NAME and VALUE must both be
 *              unique. The VALUE points to data of the data type defined for
 *              the enumeration base type.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, December 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_enum_insert(H5T_t *dt, const char *name, void *value)
{
    int         i;
    char        **names=NULL;
    uint8_t     *values=NULL;
    
    FUNC_ENTER(H5T_enum_insert, FAIL);
    assert(dt);
    assert(name && *name);
    assert(value);

    /* The name and value had better not already exist */
    for (i=0; i<dt->u.enumer.nmembs; i++) {
        if (!HDstrcmp(dt->u.enumer.name[i], name)) {
            HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                          "name redefinition");
        }
        if (!HDmemcmp(dt->u.enumer.value+i*dt->size, value, dt->size)) {
            HRETURN_ERROR(H5E_DATATYPE, H5E_CANTINIT, FAIL,
                          "value redefinition");
        }
    }

    /* Increase table sizes */
    if (dt->u.enumer.nmembs >= dt->u.enumer.nalloc) {
        int n = MAX(32, 2*dt->u.enumer.nalloc);
        if (NULL==(names=H5MM_realloc(dt->u.enumer.name, n*sizeof(char*)))) {
            HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                          "memory allocation failed");
        }
        dt->u.enumer.name = names;

        if (NULL==(values=H5MM_realloc(dt->u.enumer.value, n*dt->size))) {
            HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                          "memory allocation failed");
        }
        dt->u.enumer.value = values;
        dt->u.enumer.nalloc = n;
    }

    /* Insert new member at end of member arrays */
    dt->u.enumer.sorted = H5T_SORT_NONE;
    i = dt->u.enumer.nmembs++;
    dt->u.enumer.name[i] = H5MM_xstrdup(name);
    HDmemcpy(dt->u.enumer.value+i*dt->size, value, dt->size);

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_enum_nameof
 *
 * Purpose:     Finds the symbol name that corresponds the the specified
 *              VALUE of an enumeration data type DT. At most SIZE characters
 *              of the symbol name are copied into the NAME buffer. If the
 *              entire symbol name and null terminator do not fit in the NAME
 *              buffer then as many characters as possible are copied and the
 *              function returns failure.
 *
 *              If NAME is the null pointer and SIZE is zero then enough
 *              space is allocated to hold the result and a pointer to that
 *              memory is returned.
 *
 * Return:      Success:        Pointer to NAME
 *
 *              Failure:        NULL, name[0] is set to null.
 *
 * Programmer:  Robb Matzke
 *              Monday, January  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
char *
H5T_enum_nameof(H5T_t *dt, void *value, char *name/*out*/, size_t size)
{
    int lt, md, rt;             /*indices for binary search     */
    int cmp;                    /*comparison result             */
    
    FUNC_ENTER(H5T_enum_nameof, NULL);

    /* Check args */
    assert(dt && H5T_ENUM==dt->type);
    assert(value);
    assert(name || 0==size);
    if (name && size>0) *name = '\0';

    /* Do a binary search over the values to find the correct one */
    H5T_sort_value(dt, NULL);
    lt = 0;
    rt = dt->u.enumer.nmembs;
    md = -1;

    while (lt<rt) {
        md = (lt+rt)/2;
        cmp = HDmemcmp(value, dt->u.enumer.value+md*dt->size, dt->size);
        if (cmp<0) {
            rt = md;
        } else if (cmp>0) {
            lt = md+1;
        } else {
            break;
        }
    }
    if (md<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_NOTFOUND, NULL,
                      "value is not in the domain of the enumeration type");
    }

    /* Save result name */
    if (!name && NULL==(name=H5MM_malloc(strlen(dt->u.enumer.name[md])+1))) {
        HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                      "memory allocation failed");
    }
    HDstrncpy(name, dt->u.enumer.name[md], size);
    if (HDstrlen(dt->u.enumer.name[md])>=size) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_NOSPACE, NULL,
                      "name has been truncated");
    }
    FUNC_LEAVE(name);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_enum_valueof
 *
 * Purpose:     Finds the value that corresponds the the specified symbol
 *              NAME of an enumeration data type DT and copy it to the VALUE
 *              result buffer. The VALUE should be allocated by the caller to
 *              be large enough for the result.
 *
 * Return:      Success:        Non-negative, value stored in VALUE.
 *
 *              Failure:        Negative, VALUE is undefined.
 *
 * Programmer:  Robb Matzke
 *              Monday, January  4, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_enum_valueof(H5T_t *dt, const char *name, void *value/*out*/)
{
    int lt, md, rt;             /*indices for binary search     */
    int cmp;                    /*comparison result             */
    
    FUNC_ENTER(H5T_enum_nameof, FAIL);

    /* Check args */
    assert(dt && H5T_ENUM==dt->type);
    assert(name && *name);
    assert(value);

    /* Do a binary search over the names to find the correct one */
    H5T_sort_name(dt, NULL);
    lt = 0;
    rt = dt->u.enumer.nmembs;
    md = -1;

    while (lt<rt) {
        md = (lt+rt)/2;
        cmp = HDstrcmp(name, dt->u.enumer.name[md]);
        if (cmp<0) {
            rt = md;
        } else if (cmp>0) {
            lt = md+1;
        } else {
            break;
        }
    }
    if (md<0) {
        HRETURN_ERROR(H5E_DATATYPE, H5E_NOTFOUND, FAIL,
                      "string is not in the domain of the enumeration type");
    }

    HDmemcpy(value, dt->u.enumer.value+md*dt->size, dt->size);
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_cmp
 *
 * Purpose:     Compares two data types.
 *
 * Return:      Success:        0 if DT1 and DT2 are equal.
 *                              <0 if DT1 is less than DT2.
 *                              >0 if DT1 is greater than DT2.
 *
 *              Failure:        0, never fails
 *
 * Programmer:  Robb Matzke
 *              Wednesday, December 10, 1997
 *
 * Modifications:
 *      Robb Matzke, 22 Dec 1998
 *      Able to compare enumeration data types.
 *
 *      Robb Matzke, 20 May 1999
 *      Compares bitfields and opaque types.
 *-------------------------------------------------------------------------
 */
int
H5T_cmp(const H5T_t *dt1, const H5T_t *dt2)
{
    int *idx1 = NULL, *idx2 = NULL;
    int ret_value = 0;
    int i, j, tmp;
    hbool_t     swapped;
    size_t      base_size;

    FUNC_ENTER(H5T_cmp, 0);

    /* the easy case */
    if (dt1 == dt2) HGOTO_DONE(0);
    assert(dt1);
    assert(dt2);

    /* compare */
    if (dt1->type < dt2->type) HGOTO_DONE(-1);
    if (dt1->type > dt2->type) HGOTO_DONE(1);

    if (dt1->size < dt2->size) HGOTO_DONE(-1);
    if (dt1->size > dt2->size) HGOTO_DONE(1);

    if (dt1->parent && !dt2->parent) HGOTO_DONE(-1);
    if (!dt1->parent && dt2->parent) HGOTO_DONE(1);
    if (dt1->parent) {
        tmp = H5T_cmp(dt1->parent, dt2->parent);
        if (tmp<0) HGOTO_DONE(-1);
        if (tmp>0) HGOTO_DONE(1);
    }

    switch(dt1->type) {
        case H5T_COMPOUND:
            /*
             * Compound data types...
             */
            if (dt1->u.compnd.nmembs < dt2->u.compnd.nmembs)
                HGOTO_DONE(-1);
            if (dt1->u.compnd.nmembs > dt2->u.compnd.nmembs)
                HGOTO_DONE(1);

            /* Build an index for each type so the names are sorted */
            if (NULL==(idx1 = H5MM_malloc(dt1->u.compnd.nmembs * sizeof(int))) ||
                NULL==(idx2 = H5MM_malloc(dt1->u.compnd.nmembs * sizeof(int)))) {
                HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, 0,
                       "memory allocation failed");
            }
            for (i=0; i<dt1->u.compnd.nmembs; i++)
                idx1[i] = idx2[i] = i;
            for (i=dt1->u.compnd.nmembs-1, swapped=TRUE; swapped && i>=0; --i) {
                for (j=0, swapped=FALSE; j<i; j++) {
                    if (HDstrcmp(dt1->u.compnd.memb[idx1[j]].name,
                             dt1->u.compnd.memb[idx1[j+1]].name) > 0) {
                        tmp = idx1[j];
                        idx1[j] = idx1[j+1];
                        idx1[j+1] = tmp;
                        swapped = TRUE;
                    }
                }
            }
            for (i=dt2->u.compnd.nmembs-1, swapped=TRUE; swapped && i>=0; --i) {
                for (j=0, swapped=FALSE; j<i; j++) {
                    if (HDstrcmp(dt2->u.compnd.memb[idx2[j]].name,
                             dt2->u.compnd.memb[idx2[j+1]].name) > 0) {
                        tmp = idx2[j];
                        idx2[j] = idx2[j+1];
                        idx2[j+1] = tmp;
                        swapped = TRUE;
                    }
                }
            }

#ifdef H5T_DEBUG
            /* I don't quite trust the code above yet :-)  --RPM */
            for (i=0; i<dt1->u.compnd.nmembs-1; i++) {
                assert(HDstrcmp(dt1->u.compnd.memb[idx1[i]].name,
                        dt1->u.compnd.memb[idx1[i + 1]].name));
                assert(HDstrcmp(dt2->u.compnd.memb[idx2[i]].name,
                        dt2->u.compnd.memb[idx2[i + 1]].name));
            }
#endif

            /* Compare the members */
            for (i=0; i<dt1->u.compnd.nmembs; i++) {
                tmp = HDstrcmp(dt1->u.compnd.memb[idx1[i]].name,
                       dt2->u.compnd.memb[idx2[i]].name);
                if (tmp < 0)
                    HGOTO_DONE(-1);
                if (tmp > 0)
                    HGOTO_DONE(1);

                if (dt1->u.compnd.memb[idx1[i]].offset <
                dt2->u.compnd.memb[idx2[i]].offset) HGOTO_DONE(-1);
                if (dt1->u.compnd.memb[idx1[i]].offset >
                dt2->u.compnd.memb[idx2[i]].offset) HGOTO_DONE(1);

                if (dt1->u.compnd.memb[idx1[i]].size <
                dt2->u.compnd.memb[idx2[i]].size) HGOTO_DONE(-1);
                if (dt1->u.compnd.memb[idx1[i]].size >
                dt2->u.compnd.memb[idx2[i]].size) HGOTO_DONE(1);

                tmp = H5T_cmp(dt1->u.compnd.memb[idx1[i]].type,
                      dt2->u.compnd.memb[idx2[i]].type);
                if (tmp < 0) HGOTO_DONE(-1);
                if (tmp > 0) HGOTO_DONE(1);
            }
            break;

        case H5T_ENUM:
            /*
             * Enumeration data types...
             */
            if (dt1->u.enumer.nmembs < dt2->u.enumer.nmembs)
                HGOTO_DONE(-1);
            if (dt1->u.enumer.nmembs > dt2->u.enumer.nmembs)
                HGOTO_DONE(1);

            /* Build an index for each type so the names are sorted */
            if (NULL==(idx1 = H5MM_malloc(dt1->u.enumer.nmembs * sizeof(int))) ||
                NULL==(idx2 = H5MM_malloc(dt1->u.enumer.nmembs * sizeof(int)))) {
                HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, 0,
                       "memory allocation failed");
            }
            for (i=0; i<dt1->u.enumer.nmembs; i++)
                idx1[i] = idx2[i] = i;
            for (i=dt1->u.enumer.nmembs-1, swapped=TRUE; swapped && i>=0; --i) {
                for (j=0, swapped=FALSE; j<i; j++) {
                    if (HDstrcmp(dt1->u.enumer.name[idx1[j]],
                             dt1->u.enumer.name[idx1[j+1]]) > 0) {
                        tmp = idx1[j];
                        idx1[j] = idx1[j+1];
                        idx1[j+1] = tmp;
                        swapped = TRUE;
                    }
                }
            }
            for (i=dt2->u.enumer.nmembs-1, swapped=TRUE; swapped && i>=0; --i) {
                for (j=0, swapped=FALSE; j<i; j++) {
                    if (HDstrcmp(dt2->u.enumer.name[idx2[j]],
                             dt2->u.enumer.name[idx2[j+1]]) > 0) {
                        tmp = idx2[j];
                        idx2[j] = idx2[j+1];
                        idx2[j+1] = tmp;
                        swapped = TRUE;
                    }
                }
            }

#ifdef H5T_DEBUG
            /* I don't quite trust the code above yet :-)  --RPM */
            for (i=0; i<dt1->u.enumer.nmembs-1; i++) {
                assert(HDstrcmp(dt1->u.enumer.name[idx1[i]],
                        dt1->u.enumer.name[idx1[i+1]]));
                assert(HDstrcmp(dt2->u.enumer.name[idx2[i]],
                        dt2->u.enumer.name[idx2[i+1]]));
            }
#endif

            /* Compare the members */
            base_size = dt1->parent->size;
            for (i=0; i<dt1->u.enumer.nmembs; i++) {
                tmp = HDstrcmp(dt1->u.enumer.name[idx1[i]],
                       dt2->u.enumer.name[idx2[i]]);
                if (tmp<0) HGOTO_DONE(-1);
                if (tmp>0) HGOTO_DONE(1);

                tmp = HDmemcmp(dt1->u.enumer.value+idx1[i]*base_size,
                       dt2->u.enumer.value+idx2[i]*base_size,
                       base_size);
                if (tmp<0) HGOTO_DONE(-1);
                if (tmp>0) HGOTO_DONE(1);
            }
            break;

        case H5T_VLEN:
            assert(dt1->u.vlen.type>H5T_VLEN_BADTYPE && dt1->u.vlen.type<H5T_VLEN_MAXTYPE);
            assert(dt2->u.vlen.type>H5T_VLEN_BADTYPE && dt2->u.vlen.type<H5T_VLEN_MAXTYPE);
            assert(dt1->u.vlen.loc>H5T_VLEN_BADLOC && dt1->u.vlen.loc<H5T_VLEN_MAXLOC);
            assert(dt2->u.vlen.loc>H5T_VLEN_BADLOC && dt2->u.vlen.loc<H5T_VLEN_MAXLOC);

            /* Arbitrarily sort sequence VL datatypes before string VL datatypes */
            if (dt1->u.vlen.type==H5T_VLEN_SEQUENCE &&
                    dt2->u.vlen.type==H5T_VLEN_STRING) {
                HGOTO_DONE(-1);
            } else if (dt1->u.vlen.type==H5T_VLEN_STRING &&
                    dt2->u.vlen.type==H5T_VLEN_SEQUENCE) {
                HGOTO_DONE(1);
            }
            /* Arbitrarily sort VL datatypes in memory before disk */
            if (dt1->u.vlen.loc==H5T_VLEN_MEMORY &&
                    dt2->u.vlen.loc==H5T_VLEN_DISK) {
                HGOTO_DONE(-1);
            } else if (dt1->u.vlen.loc==H5T_VLEN_DISK &&
                    dt2->u.vlen.loc==H5T_VLEN_MEMORY) {
                HGOTO_DONE(1);
            }
            /* Don't allow VL types in different files to compare as equal */
            if (dt1->u.vlen.f < dt2->u.vlen.f)
                HGOTO_DONE(-1);
            if (dt1->u.vlen.f > dt2->u.vlen.f)
                HGOTO_DONE(1);
            break;

        case H5T_OPAQUE:
            HGOTO_DONE(HDstrcmp(dt1->u.opaque.tag,dt2->u.opaque.tag));

        case H5T_ARRAY:
            if (dt1->u.array.ndims < dt2->u.array.ndims)
                HGOTO_DONE(-1);
            if (dt1->u.array.ndims > dt2->u.array.ndims)
                HGOTO_DONE(1);

            for (j=0; j<dt1->u.array.ndims; j++) {
                if (dt1->u.array.dim[j] < dt2->u.array.dim[j])
                    HGOTO_DONE(-1);
                if (dt1->u.array.dim[j] > dt2->u.array.dim[j])
                    HGOTO_DONE(1);
            }

            for (j=0; j<dt1->u.array.ndims; j++) {
                if (dt1->u.array.perm[j] < dt2->u.array.perm[j])
                    HGOTO_DONE(-1);
                if (dt1->u.array.perm[j] > dt2->u.array.perm[j])
                    HGOTO_DONE(1);
            }

            tmp = H5T_cmp(dt1->parent, dt2->parent);
            if (tmp < 0)
                HGOTO_DONE(-1);
            if (tmp > 0)
                HGOTO_DONE(1);
            break;

        default:
            /*
             * Atomic datatypes...
             */
            if (dt1->u.atomic.order < dt2->u.atomic.order) HGOTO_DONE(-1);
            if (dt1->u.atomic.order > dt2->u.atomic.order) HGOTO_DONE(1);

            if (dt1->u.atomic.prec < dt2->u.atomic.prec) HGOTO_DONE(-1);
            if (dt1->u.atomic.prec > dt2->u.atomic.prec) HGOTO_DONE(1);

            if (dt1->u.atomic.offset < dt2->u.atomic.offset) HGOTO_DONE(-1);
            if (dt1->u.atomic.offset > dt2->u.atomic.offset) HGOTO_DONE(1);

            if (dt1->u.atomic.lsb_pad < dt2->u.atomic.lsb_pad) HGOTO_DONE(-1);
            if (dt1->u.atomic.lsb_pad > dt2->u.atomic.lsb_pad) HGOTO_DONE(1);

            if (dt1->u.atomic.msb_pad < dt2->u.atomic.msb_pad) HGOTO_DONE(-1);
            if (dt1->u.atomic.msb_pad > dt2->u.atomic.msb_pad) HGOTO_DONE(1);

            switch (dt1->type) {
                case H5T_INTEGER:
                    if (dt1->u.atomic.u.i.sign < dt2->u.atomic.u.i.sign) {
                    HGOTO_DONE(-1);
                    }
                    if (dt1->u.atomic.u.i.sign > dt2->u.atomic.u.i.sign) {
                    HGOTO_DONE(1);
                    }
                    break;

                case H5T_FLOAT:
                    if (dt1->u.atomic.u.f.sign < dt2->u.atomic.u.f.sign) {
                    HGOTO_DONE(-1);
                    }
                    if (dt1->u.atomic.u.f.sign > dt2->u.atomic.u.f.sign) {
                    HGOTO_DONE(1);
                    }

                    if (dt1->u.atomic.u.f.epos < dt2->u.atomic.u.f.epos) {
                    HGOTO_DONE(-1);
                    }
                    if (dt1->u.atomic.u.f.epos > dt2->u.atomic.u.f.epos) {
                    HGOTO_DONE(1);
                    }

                    if (dt1->u.atomic.u.f.esize <
                    dt2->u.atomic.u.f.esize) HGOTO_DONE(-1);
                    if (dt1->u.atomic.u.f.esize >
                    dt2->u.atomic.u.f.esize) HGOTO_DONE(1);

                    if (dt1->u.atomic.u.f.ebias <
                    dt2->u.atomic.u.f.ebias) HGOTO_DONE(-1);
                    if (dt1->u.atomic.u.f.ebias >
                    dt2->u.atomic.u.f.ebias) HGOTO_DONE(1);

                    if (dt1->u.atomic.u.f.mpos < dt2->u.atomic.u.f.mpos) {
                    HGOTO_DONE(-1);
                    }
                    if (dt1->u.atomic.u.f.mpos > dt2->u.atomic.u.f.mpos) {
                    HGOTO_DONE(1);
                    }

                    if (dt1->u.atomic.u.f.msize <
                    dt2->u.atomic.u.f.msize) HGOTO_DONE(-1);
                    if (dt1->u.atomic.u.f.msize >
                    dt2->u.atomic.u.f.msize) HGOTO_DONE(1);

                    if (dt1->u.atomic.u.f.norm < dt2->u.atomic.u.f.norm) {
                    HGOTO_DONE(-1);
                    }
                    if (dt1->u.atomic.u.f.norm > dt2->u.atomic.u.f.norm) {
                    HGOTO_DONE(1);
                    }

                    if (dt1->u.atomic.u.f.pad < dt2->u.atomic.u.f.pad) {
                    HGOTO_DONE(-1);
                    }
                    if (dt1->u.atomic.u.f.pad > dt2->u.atomic.u.f.pad) {
                    HGOTO_DONE(1);
                    }

                    break;

                case H5T_TIME:  /* order and precision are checked above */
                    /*void */
                    break;

                case H5T_STRING:
                    if (dt1->u.atomic.u.s.cset < dt2->u.atomic.u.s.cset) {
                    HGOTO_DONE(-1);
                    }
                    if (dt1->u.atomic.u.s.cset > dt2->u.atomic.u.s.cset) {
                    HGOTO_DONE(1);
                    }

                    if (dt1->u.atomic.u.s.pad < dt2->u.atomic.u.s.pad) {
                    HGOTO_DONE(-1);
                    }
                    if (dt1->u.atomic.u.s.pad > dt2->u.atomic.u.s.pad) {
                    HGOTO_DONE(1);
                    }

                    break;

                case H5T_BITFIELD:
                    /*void */
                    break;

                case H5T_REFERENCE:
                    if (dt1->u.atomic.u.r.rtype < dt2->u.atomic.u.r.rtype) {
                    HGOTO_DONE(-1);
                    }       
                    if (dt1->u.atomic.u.r.rtype > dt2->u.atomic.u.r.rtype) {
                    HGOTO_DONE(1);
                    }

                    switch(dt1->u.atomic.u.r.rtype) {
                        case H5R_OBJECT:
                        case H5R_DATASET_REGION:
                    /* Does this need more to distinguish it? -QAK 11/30/98 */
                            /*void */
                            break;

                        default:
                            assert("not implemented yet" && 0);
                    }
                    break;

                default:
                    assert("not implemented yet" && 0);
            }
        break;
    } /* end switch */

done:
    if(idx1!=NULL)
        H5MM_xfree(idx1);
    if(idx2!=NULL)
        H5MM_xfree(idx2);

    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5T_path_find
 *
 * Purpose:     Finds the path which converts type SRC_ID to type DST_ID,
 *              creating a new path if necessary.  If FUNC is non-zero then
 *              it is set as the hard conversion function for that path
 *              regardless of whether the path previously existed. Changing
 *              the conversion function of a path causes statistics to be
 *              reset to zero after printing them.  The NAME is used only
 *              when creating a new path and is just for debugging.
 *
 *              If SRC and DST are both null pointers then the special no-op
 *              conversion path is used.  This path is always stored as the
 *              first path in the path table.
 *
 * Return:      Success:        Pointer to the path, valid until the path
 *                              database is modified.
 *
 *              Failure:        NULL if the path does not exist and no
 *                              function can be found to apply to the new
 *                              path.
 *
 * Programmer:  Robb Matzke
 *              Tuesday, January 13, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_path_t *
H5T_path_find(const H5T_t *src, const H5T_t *dst, const char *name,
              H5T_conv_t func)
{
    int lt, rt;                 /*left and right edges          */
    int md;                     /*middle                        */
    int cmp;                    /*comparison result             */
    int old_npaths;             /* Previous number of paths in table */
    H5T_path_t  *table=NULL;            /*path existing in the table    */
    H5T_path_t  *path=NULL;             /*new path                      */
    H5T_path_t  *ret_value=NULL;        /*return value                  */
    hid_t       src_id=-1, dst_id=-1;   /*src and dst type identifiers  */
    int i;                      /*counter                       */
    int nprint=0;               /*lines of output printed       */

    FUNC_ENTER(H5T_path_find, NULL);
    assert((!src && !dst) || (src && dst));

    /*
     * Make sure the first entry in the table is the no-op conversion path.
     */
    if (0==H5T_g.npaths) {
        if (NULL==(H5T_g.path=H5MM_malloc(128*sizeof(H5T_path_t*)))) {
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                        "memory allocation failed for type conversion path "
                        "table");
        }
        H5T_g.apaths = 128;
        if (NULL==(H5T_g.path[0]=H5FL_ALLOC(H5T_path_t,1))) {
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                        "memory allocation failed for no-op conversion path");
        }
        HDstrcpy(H5T_g.path[0]->name, "no-op");
        H5T_g.path[0]->func = H5T_conv_noop;
        H5T_g.path[0]->cdata.command = H5T_CONV_INIT;
        if (H5T_conv_noop(FAIL, FAIL, &(H5T_g.path[0]->cdata), (hsize_t)0, 0, 0,
                          NULL, NULL, H5P_DEFAULT)<0) {
#ifdef H5T_DEBUG
            if (H5DEBUG(T)) {
                fprintf(H5DEBUG(T), "H5T: unable to initialize no-op "
                        "conversion function (ignored)\n");
            }
#endif
            H5E_clear(); /*ignore the error*/
        }
        H5T_g.npaths = 1;
    }

    /*
     * Find the conversion path.  If source and destination types are equal
     * then use entry[0], otherwise do a binary search over the
     * remaining entries.
     *
     * Quincey Koziol, 2 July, 1999
     * Only allow the no-op conversion to occur if no "force conversion" flags
     * are set
     */
    if (src->force_conv==FALSE && dst->force_conv==FALSE && 0==H5T_cmp(src, dst)) {
        table = H5T_g.path[0];
        cmp = 0;
        md = 0;
    } else {
        lt = md = 1;
        rt = H5T_g.npaths;
        cmp = -1;
        
        while (cmp && lt<rt) {
            md = (lt+rt) / 2;
            assert(H5T_g.path[md]);
            cmp = H5T_cmp(src, H5T_g.path[md]->src);
            if (0==cmp) cmp = H5T_cmp(dst, H5T_g.path[md]->dst);
            if (cmp<0) {
                rt = md;
            } else if (cmp>0) {
                lt = md+1;
            } else {
                table = H5T_g.path[md];
            }
        }
    }
    
    /* Keep a record of the number of paths in the table, in case one of the
     * initialization calls below (hard or soft) causes more entries to be
     * added to the table - QAK, 1/26/02
     */
    old_npaths=H5T_g.npaths;
    
    /*
     * If we didn't find the path or if the caller is specifying a new hard
     * conversion function then create a new path and add the new function to
     * the path.
     */
    if (!table || func) {
        if (NULL==(path=H5FL_ALLOC(H5T_path_t,1))) {
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL,
                        "memory allocation failed for type conversion path");
        }
        if (name && *name) {
            strncpy(path->name, name, H5T_NAMELEN);
            path->name[H5T_NAMELEN-1] = '\0';
        } else {
            strcpy(path->name, "NONAME");
        }
        if ((src && NULL==(path->src=H5T_copy(src, H5T_COPY_ALL))) ||
            (dst && NULL==(path->dst=H5T_copy(dst, H5T_COPY_ALL)))) {
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL,
                        "unable to copy data type for conversion path");
        }
    } else {
        path = table;
    }

    /*
     * If a hard conversion function is specified and none is defined for the
     * path then add it to the path and initialize its conversion data.
     */
    if (func) {
        assert(path!=table);
        assert(NULL==path->func);
        if (path->src && (src_id=H5I_register(H5I_DATATYPE,
                                              H5T_copy(path->src,
                                                       H5T_COPY_ALL)))<0) {
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, NULL,
                        "unable to register source conversion type for query");
        }
        if (path->dst && (dst_id=H5I_register(H5I_DATATYPE,
                                              H5T_copy(path->dst,
                                                       H5T_COPY_ALL)))<0) {
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, NULL,
                        "unable to register destination conversion type for "
                        "query");
        }
        path->cdata.command = H5T_CONV_INIT;
        if ((func)(src_id, dst_id, &(path->cdata), (hsize_t)0, 0, 0, NULL, NULL,
                   H5P_DEFAULT)<0) {
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL,
                        "unable to initialize conversion function");
        }
        if (src_id>=0) H5I_dec_ref(src_id);
        if (dst_id>=0) H5I_dec_ref(dst_id);
        src_id = dst_id = -1;
        path->func = func;
        path->is_hard = TRUE;
    }
    
    /*
     * If the path doesn't have a function by now (because it's a new path
     * and the caller didn't supply a hard function) then scan the soft list
     * for an applicable function and add it to the path.  This can't happen
     * for the no-op conversion path.
     */
    assert(path->func || (src && dst));
    for (i=H5T_g.nsoft-1; i>=0 && !path->func; --i) {
        if (src->type!=H5T_g.soft[i].src ||
            dst->type!=H5T_g.soft[i].dst) {
            continue;
        }
        if ((src_id=H5I_register(H5I_DATATYPE,
                                 H5T_copy(path->src, H5T_COPY_ALL)))<0 ||
            (dst_id=H5I_register(H5I_DATATYPE,
                                 H5T_copy(path->dst, H5T_COPY_ALL)))<0) {
            HGOTO_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, NULL,
                        "unable to register conversion types for query");
        }
        path->cdata.command = H5T_CONV_INIT;
        if ((H5T_g.soft[i].func) (src_id, dst_id, &(path->cdata),
                                  (hsize_t)0, 0, 0, NULL, NULL, H5P_DEFAULT)<0) {
            HDmemset (&(path->cdata), 0, sizeof(H5T_cdata_t));
            H5E_clear(); /*ignore the error*/
        } else {
            HDstrcpy (path->name, H5T_g.soft[i].name);
            path->func = H5T_g.soft[i].func;
            path->is_hard = FALSE;
        }
        H5I_dec_ref(src_id);
        H5I_dec_ref(dst_id);
        src_id = dst_id = -1;
    }
    if (!path->func) {
        HGOTO_ERROR(H5E_DATATYPE, H5E_CANTINIT, NULL,
                    "no appropriate function for conversion path");
    }

    /* Check if paths were inserted into the table through a recursive call
     * and re-compute the correct location for this path if so. - QAK, 1/26/02
     */
    if(old_npaths!=H5T_g.npaths) {
        lt = md = 1;
        rt = H5T_g.npaths;
        cmp = -1;
        
        while (cmp && lt<rt) {
            md = (lt+rt) / 2;
            assert(H5T_g.path[md]);
            cmp = H5T_cmp(src, H5T_g.path[md]->src);
            if (0==cmp) cmp = H5T_cmp(dst, H5T_g.path[md]->dst);
            if (cmp<0) {
                rt = md;
            } else if (cmp>0) {
                lt = md+1;
            } else {
                table = H5T_g.path[md];
            }
        }
    } /* end if */

    /* Replace an existing table entry or add a new entry */
    if (table && path!=table) {
        assert(table==H5T_g.path[md]);
        H5T_print_stats(table, &nprint/*in,out*/);
        table->cdata.command = H5T_CONV_FREE;
        if ((table->func)(FAIL, FAIL, &(table->cdata), (hsize_t)0, 0, 0, NULL, NULL,
                          H5P_DEFAULT)<0) {
#ifdef H5T_DEBUG
            if (H5DEBUG(T)) {
                fprintf(H5DEBUG(T), "H5T: conversion function 0x%08lx free "
                        "failed for %s (ignored)\n",
                        (unsigned long)(path->func), path->name);
            }
#endif
            H5E_clear(); /*ignore the failure*/
        }
        if (table->src) H5T_close(table->src);
        if (table->dst) H5T_close(table->dst);
    H5FL_FREE(H5T_path_t,table);
        table = path;
        H5T_g.path[md] = path;
    } else if (path!=table) {
        assert(cmp);
        if (H5T_g.npaths >= H5T_g.apaths) {
            size_t na = MAX(128, 2 * H5T_g.apaths);
            H5T_path_t **x = H5MM_realloc (H5T_g.path,
                                           na*sizeof(H5T_path_t*));
            if (!x) {
                HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                             "memory allocation failed");
            }
            H5T_g.apaths = (int)na;
            H5T_g.path = x;
        }
        if (cmp>0) md++;
        HDmemmove(H5T_g.path+md+1, H5T_g.path+md,
                  (H5T_g.npaths-md) * sizeof(H5T_path_t*));
        H5T_g.npaths++;
        H5T_g.path[md] = path;
        table = path;
    }
    ret_value = path;

 done:
    if (!ret_value && path && path!=table) {
        if (path->src) H5T_close(path->src);
        if (path->dst) H5T_close(path->dst);
    H5FL_FREE(H5T_path_t,path);
    }
    if (src_id>=0) H5I_dec_ref(src_id);
    if (dst_id>=0) H5I_dec_ref(dst_id);
    
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_convert
 *
 * Purpose:     Call a conversion function to convert from source to
 *              destination data type and accumulate timing statistics.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Tuesday, December 15, 1998
 *
 * Modifications:
 *              Robb Matzke, 1999-06-16
 *              The timers are updated only if H5T debugging is enabled at
 *              runtime in addition to compile time.
 *
 *              Robb Matzke, 1999-06-16
 *              Added support for non-zero strides. If BUF_STRIDE is non-zero
 *              then convert one value at each memory location advancing
 *              BUF_STRIDE bytes each time; otherwise assume both source and
 *              destination values are packed.
 *
 *              Quincey Koziol, 1999-07-01
 *              Added dataset transfer properties, to allow custom VL
 *              datatype allocation function to be passed down to VL
 *              conversion routine.
 *
 *              Robb Matzke, 2000-05-17
 *              Added the BKG_STRIDE argument which gets passed to all the
 *              conversion functions. If BUF_STRIDE is non-zero then each
 *              data element is at a multiple of BUF_STRIDE bytes in BUF
 *              (on both input and output). If BKG_STRIDE is also set then
 *              the BKG buffer is used in such a way that temporary space
 *              for each element is aligned on a BKG_STRIDE byte boundary.
 *              If either BUF_STRIDE or BKG_STRIDE are zero then the BKG
 *              buffer will be accessed as though it were a packed array
 *              of destination datatype.
 *-------------------------------------------------------------------------
 */
herr_t
H5T_convert(H5T_path_t *tpath, hid_t src_id, hid_t dst_id, hsize_t nelmts,
            size_t buf_stride, size_t bkg_stride, void *buf, void *bkg,
            hid_t dset_xfer_plist)
{
#ifdef H5T_DEBUG
    H5_timer_t          timer;
#endif

    FUNC_ENTER(H5T_convert, FAIL);

#ifdef H5T_DEBUG
    if (H5DEBUG(T)) H5_timer_begin(&timer);
#endif
    tpath->cdata.command = H5T_CONV_CONV;
    if ((tpath->func)(src_id, dst_id, &(tpath->cdata), nelmts, buf_stride,
                      bkg_stride, buf, bkg, dset_xfer_plist)<0) {
        HRETURN_ERROR(H5E_ATTR, H5E_CANTENCODE, FAIL,
                      "data type conversion failed");
    }
#ifdef H5T_DEBUG
    if (H5DEBUG(T)) {
        H5_timer_end(&(tpath->stats.timer), &timer);
        tpath->stats.ncalls++;
        tpath->stats.nelmts += nelmts;
    }
#endif

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5T_entof
 *
 * Purpose:     Returns a pointer to the entry for a named data type.
 *
 * Return:      Success:        Ptr directly into named data type
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, June  5, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5G_entry_t *
H5T_entof (H5T_t *dt)
{
    H5G_entry_t         *ret_value = NULL;
    
    FUNC_ENTER (H5T_entof, NULL);
    assert (dt);

    switch (dt->state) {
    case H5T_STATE_TRANSIENT:
    case H5T_STATE_RDONLY:
    case H5T_STATE_IMMUTABLE:
        HRETURN_ERROR (H5E_DATATYPE, H5E_CANTINIT, NULL,
                       "not a named data type");
    case H5T_STATE_NAMED:
    case H5T_STATE_OPEN:
        ret_value = &(dt->ent);
        break;
    }

    FUNC_LEAVE (ret_value);
}


/*--------------------------------------------------------------------------
 NAME
    H5T_get_ref_type
 PURPOSE
    Retrieves the type of reference for a datatype
 USAGE
    H5R_type_t H5Tget_ref_type(dt)
        H5T_t *dt;  IN: datatype pointer for the reference datatype
        
 RETURNS
    Success:    A reference type defined in H5Rpublic.h
    Failure:    H5R_BADTYPE
 DESCRIPTION
    Given a reference datatype object, this function returns the reference type
        of the datatype.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
H5R_type_t
H5T_get_ref_type(const H5T_t *dt)
{
    H5R_type_t ret_value = H5R_BADTYPE;

    FUNC_ENTER(H5T_get_ref_type, H5R_BADTYPE);

    assert(dt);

    if(dt->type==H5T_REFERENCE)
        ret_value=dt->u.atomic.u.r.rtype;

#ifdef LATER
done:
#endif /* LATER */
    FUNC_LEAVE(ret_value);
}   /* end H5T_get_ref_type() */


/*-------------------------------------------------------------------------
 * Function:    H5Tarray_create
 *
 * Purpose:     Create a new array data type based on the specified BASE_TYPE.
 *              The type is an array with NDIMS dimensionality and the size of the
 *      array is DIMS. The total member size should be relatively small.
 *      PERM is currently unimplemented and unused, but is designed to contain
 *      the dimension permutation from C order.
 *      Array datatypes are currently limited to H5S_MAX_RANK number of
 *      dimensions and must have the number of dimensions set greater than
 *      0. (i.e. 0 > ndims <= H5S_MAX_RANK)  All dimensions sizes must be greater
 *      than 0 also.
 *
 * Return:      Success:        ID of new array data type
 *
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, Oct 26, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Tarray_create(hid_t base_id, int ndims, const hsize_t dim[/* ndims */],
    const int UNUSED perm[/* ndims */])
{
    H5T_t       *base = NULL;           /* base data type       */
    H5T_t       *dt = NULL;                 /* new array data type      */
    int    i;                  /* local index variable */
    hid_t       ret_value = FAIL;       /* return value                 */
    
    FUNC_ENTER(H5Tarray_create, FAIL);
    H5TRACE4("i","iIs*h*Is",base_id,ndims,dim,perm);

    /* Check args */
    if (ndims<1 || ndims>H5S_MAX_RANK)
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "invalid dimensionality");
    if (ndims>0 && !dim)
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no dimensions specified");
    for(i=0; i<ndims; i++)
        if(!(dim[i]>0))
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "zero-sized dimension specified");
    if (H5I_DATATYPE!=H5I_get_type(base_id) || NULL==(base=H5I_object(base_id)))
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an valid base datatype");

    /* Create the actual array datatype */
    if ((dt=H5T_array_create(base,ndims,dim,perm))==NULL)
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to create datatype");

    /* Atomize the type */
    if ((ret_value=H5I_register(H5I_DATATYPE, dt))<0)
        HRETURN_ERROR(H5E_DATATYPE, H5E_CANTREGISTER, FAIL, "unable to register datatype");

    FUNC_LEAVE(ret_value);
}   /* end H5Tarray_create */


/*-------------------------------------------------------------------------
 * Function:    H5T_array_create
 *
 * Purpose:     Internal routine to create a new array data type based on the
 *      specified BASE_TYPE.  The type is an array with NDIMS dimensionality
 *      and the size of the array is DIMS.  PERM is currently unimplemented
 *      and unused, but is designed to contain the dimension permutation from
 *      C order.  Array datatypes are currently limited to H5S_MAX_RANK number
 *      of *      dimensions.
 *
 * Return:      Success:        ID of new array data type
 *
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, Oct 26, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5T_t *
H5T_array_create(H5T_t *base, int ndims, const hsize_t dim[/* ndims */],
    const int perm[/* ndims */])
{
    H5T_t       *ret_value = NULL;              /*new array data type   */
    int    i;                  /* local index variable */
    
    FUNC_ENTER(H5T_array_create, NULL);

    assert(base);
    assert(ndims>0 && ndims<=H5S_MAX_RANK);
    assert(dim);

    /* Build new type */
    if (NULL==(ret_value = H5FL_ALLOC(H5T_t,1)))
        HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    ret_value->ent.header = HADDR_UNDEF;
    ret_value->type = H5T_ARRAY;

    /* Copy the base type of the array */
    ret_value->parent = H5T_copy(base, H5T_COPY_ALL);

    /* Set the array parameters */
    ret_value->u.array.ndims = ndims;

    /* Copy the array dimensions & compute the # of elements in the array */
    for(i=0, ret_value->u.array.nelem=1; i<ndims; i++) {
        ret_value->u.array.dim[i] = dim[i];
        ret_value->u.array.nelem *= dim[i];
    } /* end for */

    /* Copy the dimension permutations */
    for(i=0; i<ndims; i++)
        ret_value->u.array.perm[i] = perm ? perm[i] : i;

    /* Set the array's size (number of elements * element datatype's size) */
    ret_value->size = ret_value->parent->size * ret_value->u.array.nelem;

    /*
     * Set the "force conversion" flag if a VL base datatype is used or
     * or if any components of the base datatype are VL types.
     */
    if(base->type==H5T_VLEN || base->force_conv==TRUE)
        ret_value->force_conv=TRUE;

    FUNC_LEAVE(ret_value);
}   /* end H5T_array_create */


/*-------------------------------------------------------------------------
 * Function:    H5Tget_array_ndims
 *
 * Purpose:     Query the number of dimensions for an array datatype.
 *
 * Return:      Success:        Number of dimensions of the array datatype
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Monday, November 6, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5Tget_array_ndims(hid_t type_id)
{
    H5T_t       *dt = NULL;                 /* pointer to array data type       */
    int ret_value = FAIL;           /* return value                     */
    
    FUNC_ENTER(H5Tget_array_ndims, FAIL);
    H5TRACE1("Is","i",type_id);

    /* Check args */
    if (H5I_DATATYPE!=H5I_get_type(type_id) || NULL==(dt=H5I_object(type_id)))
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object");
    if(dt->type!=H5T_ARRAY)
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an array datatype");

    /* Retrieve the number of dimensions */
    ret_value=dt->u.array.ndims;

    FUNC_LEAVE(ret_value);
}   /* end H5Tget_array_ndims */


/*-------------------------------------------------------------------------
 * Function:    H5Tget_array_dims
 *
 * Purpose:     Query the sizes of dimensions for an array datatype.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Monday, November 6, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Tget_array_dims(hid_t type_id, hsize_t dims[], int perm[])
{
    H5T_t       *dt = NULL;                 /* pointer to array data type       */
    herr_t      ret_value = SUCCEED;    /* return value                 */
    int    i;                  /* Local index variable */
    
    FUNC_ENTER(H5Tget_array_dims, FAIL);
    H5TRACE3("e","i*h*Is",type_id,dims,perm);

    /* Check args */
    if (H5I_DATATYPE!=H5I_get_type(type_id) || NULL==(dt=H5I_object(type_id)))
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a datatype object");
    if(dt->type!=H5T_ARRAY)
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not an array datatype");

    /* Retrieve the sizes of the dimensions */
    if(dims) 
        for(i=0; i<dt->u.array.ndims; i++)
            dims[i]=dt->u.array.dim[i];

    /* Retrieve the dimension permutations */
    if(perm) 
        for(i=0; i<dt->u.array.ndims; i++)
            perm[i]=dt->u.array.perm[i];

    FUNC_LEAVE(ret_value);
}   /* end H5Tget_array_dims */


/*-------------------------------------------------------------------------
 * Function:    H5T_print_stats
 *
 * Purpose:     Print statistics about a conversion path.  Statistics are
 *              printed only if all the following conditions are true:
 *
 *              1. The library was compiled with H5T_DEBUG defined.
 *              2. Data type debugging is turned on at run time.
 *              3. The path was called at least one time.
 *
 *              The optional NPRINT argument keeps track of the number of
 *              conversions paths for which statistics have been shown. If
 *              its value is zero then table headers are printed before the
 *              first line of output.
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Monday, December 14, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5T_print_stats(H5T_path_t UNUSED *path, int UNUSED *nprint/*in,out*/)
{
#ifdef H5T_DEBUG
    hsize_t     nbytes;
    char        bandwidth[32];
#endif

    FUNC_ENTER(H5T_print_stats, FAIL);
    
#ifdef H5T_DEBUG
    if (H5DEBUG(T) && path->stats.ncalls>0) {
        if (nprint && 0==(*nprint)++) {
            HDfprintf (H5DEBUG(T), "H5T: type conversion statistics:\n");
            HDfprintf (H5DEBUG(T), "   %-16s %10s %10s %8s %8s %8s %10s\n",
                       "Conversion", "Elmts", "Calls", "User",
                       "System", "Elapsed", "Bandwidth");
            HDfprintf (H5DEBUG(T), "   %-16s %10s %10s %8s %8s %8s %10s\n",
                       "----------", "-----", "-----", "----",
                       "------", "-------", "---------");
        }
        nbytes = MAX (H5T_get_size (path->src),
                      H5T_get_size (path->dst));
        nbytes *= path->stats.nelmts;
        H5_bandwidth(bandwidth, (double)nbytes,
                     path->stats.timer.etime);
        HDfprintf (H5DEBUG(T), "   %-16s %10Hd %10d %8.2f %8.2f %8.2f %10s\n",
                   path->name,
                   path->stats.nelmts,
                   path->stats.ncalls,
                   path->stats.timer.utime, 
                   path->stats.timer.stime, 
                   path->stats.timer.etime,
                   bandwidth);
    }
#endif
    FUNC_LEAVE(SUCCEED);
    path=0;
    nprint=0;
}

/*-------------------------------------------------------------------------
 * Function:    H5T_debug
 *
 * Purpose:     Prints information about a data type.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, January  7, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5T_debug(H5T_t *dt, FILE *stream)
{
    const char  *s1="", *s2="";
    int         i;
    size_t      k, base_size;
    uint64_t    tmp;

    FUNC_ENTER(H5T_debug, FAIL);

    /* Check args */
    assert(dt);
    assert(stream);

    switch (dt->type) {
    case H5T_INTEGER:
        s1 = "int";
        break;
    case H5T_FLOAT:
        s1 = "float";
        break;
    case H5T_TIME:
        s1 = "time";
        break;
    case H5T_STRING:
        s1 = "str";
        break;
    case H5T_BITFIELD:
        s1 = "bits";
        break;
    case H5T_OPAQUE:
        s1 = "opaque";
        break;
    case H5T_COMPOUND:
        s1 = "struct";
        break;
    case H5T_ENUM:
        s1 = "enum";
        break;
    default:
        s1 = "";
        break;
    }

    switch (dt->state) {
    case H5T_STATE_TRANSIENT:
        s2 = "[transient]";
        break;
    case H5T_STATE_RDONLY:
        s2 = "[constant]";
        break;
    case H5T_STATE_IMMUTABLE:
        s2 = "[predefined]";
        break;
    case H5T_STATE_NAMED:
        s2 = "[named,closed]";
        break;
    case H5T_STATE_OPEN:
        s2 = "[named,open]";
        break;
    }

    fprintf(stream, "%s%s {nbytes=%lu", s1, s2, (unsigned long)(dt->size));

    if (H5T_is_atomic(dt)) {
        switch (dt->u.atomic.order) {
        case H5T_ORDER_BE:
            s1 = "BE";
            break;
        case H5T_ORDER_LE:
            s1 = "LE";
            break;
        case H5T_ORDER_VAX:
            s1 = "VAX";
            break;
        case H5T_ORDER_NONE:
            s1 = "NONE";
            break;
        default:
            s1 = "order?";
            break;
        }
        fprintf(stream, ", %s", s1);

        if (dt->u.atomic.offset) {
            fprintf(stream, ", offset=%lu",
                    (unsigned long) (dt->u.atomic.offset));
        }
        if (dt->u.atomic.prec != 8 * dt->size) {
            fprintf(stream, ", prec=%lu",
                    (unsigned long) (dt->u.atomic.prec));
        }
        switch (dt->type) {
        case H5T_INTEGER:
            switch (dt->u.atomic.u.i.sign) {
            case H5T_SGN_NONE:
                s1 = "unsigned";
                break;
            case H5T_SGN_2:
                s1 = NULL;
                break;
            default:
                s1 = "sign?";
                break;
            }
            if (s1) fprintf(stream, ", %s", s1);
            break;

        case H5T_FLOAT:
            switch (dt->u.atomic.u.f.norm) {
            case H5T_NORM_IMPLIED:
                s1 = "implied";
                break;
            case H5T_NORM_MSBSET:
                s1 = "msbset";
                break;
            case H5T_NORM_NONE:
                s1 = "no-norm";
                break;
            default:
                s1 = "norm?";
                break;
            }
            fprintf(stream, ", sign=%lu+1",
                    (unsigned long) (dt->u.atomic.u.f.sign));
            fprintf(stream, ", mant=%lu+%lu (%s)",
                    (unsigned long) (dt->u.atomic.u.f.mpos),
                    (unsigned long) (dt->u.atomic.u.f.msize), s1);
            fprintf(stream, ", exp=%lu+%lu",
                    (unsigned long) (dt->u.atomic.u.f.epos),
                    (unsigned long) (dt->u.atomic.u.f.esize));
            tmp = dt->u.atomic.u.f.ebias >> 32;
            if (tmp) {
                size_t hi = tmp;
                size_t lo = dt->u.atomic.u.f.ebias & 0xffffffff;
                fprintf(stream, " bias=0x%08lx%08lx",
                        (unsigned long)hi, (unsigned long)lo);
            } else {
                size_t lo = dt->u.atomic.u.f.ebias & 0xffffffff;
                fprintf(stream, " bias=0x%08lx", (unsigned long)lo);
            }
            break;

        default:
            /* No additional info */
            break;
        }
        
    } else if (H5T_COMPOUND==dt->type) {
        /* Compound data type */
        for (i=0; i<dt->u.compnd.nmembs; i++) {
            fprintf(stream, "\n\"%s\" @%lu",
                    dt->u.compnd.memb[i].name,
                    (unsigned long) (dt->u.compnd.memb[i].offset));
#ifdef OLD_WAY
            if (dt->u.compnd.memb[i].ndims) {
                fprintf(stream, "[");
                for (j = 0; j < dt->u.compnd.memb[i].ndims; j++) {
                    fprintf(stream, "%s%lu", j ? ", " : "",
                            (unsigned long)(dt->u.compnd.memb[i].dim[j]));
                }
                fprintf(stream, "]");
            }
#endif /* OLD_WAY */
            fprintf(stream, " ");
            H5T_debug(dt->u.compnd.memb[i].type, stream);
        }
        fprintf(stream, "\n");
        
    } else if (H5T_ENUM==dt->type) {
        /* Enumeration data type */
        fprintf(stream, " ");
        H5T_debug(dt->parent, stream);
        base_size = dt->parent->size;
        for (i=0; i<dt->u.enumer.nmembs; i++) {
            fprintf(stream, "\n\"%s\" = 0x", dt->u.enumer.name[i]);
            for (k=0; k<base_size; k++) {
                fprintf(stream, "%02lx",
                        (unsigned long)(dt->u.enumer.value+i*base_size+k));
            }
        }
        fprintf(stream, "\n");
        
    } else if (H5T_OPAQUE==dt->type) {
        fprintf(stream, ", tag=\"%s\"", dt->u.opaque.tag);

    } else {
        /* Unknown */
        fprintf(stream, "unknown class %d\n", (int)(dt->type));
    }
    fprintf(stream, "}");

    FUNC_LEAVE(SUCCEED);
}

