/****************************************************************************
 * NCSA HDF                                                                 *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 *                                                                          *
 * For conditions of distribution and use, see the accompanying             *
 * hdf/COPYING file.                                                        *
 *                                                                          *
 ****************************************************************************/

/*
 * This file contains public declarations for the H5E module.
 */
#ifndef _H5Epublic_H
#define _H5Epublic_H

/*THIS IS BAD: #undef _PROTOTYPES*/
#include <stdio.h>              /*FILE arg of H5Eprint()*/

/* Public headers needed by this file */
#include "H5public.h"
#include "H5Ipublic.h"

/*
 * One often needs to temporarily disable automatic error reporting when
 * trying something that's likely or expected to fail.  For instance, to
 * determine if an object exists one can call H5Gget_objinfo() which will fail if
 * the object doesn't exist.  The code to try can be nested between calls to
 * H5Eget_auto() and H5Eset_auto(), but it's easier just to use this macro
 * like:
 *      H5E_BEGIN_TRY {
 *          ...stuff here that's likely to fail...
 *      } H5E_END_TRY;
 *
 * Warning: don't break, return, or longjmp() from the body of the loop or
 *          the error reporting won't be properly restored!
 */
#define H5E_BEGIN_TRY {                                                       \
    herr_t (*H5E_saved_efunc)(void*);                                         \
    void *H5E_saved_edata;                                                    \
    H5Eget_auto (&H5E_saved_efunc, &H5E_saved_edata);                         \
    H5Eset_auto (NULL, NULL);

#define H5E_END_TRY                                                           \
    H5Eset_auto (H5E_saved_efunc, H5E_saved_edata);                           \
}

/*
 * Public API Convenience Macros for Error reporting - Documented
 */
/* Use the Standard C __FILE__ & __LINE__ macros instead of typing them in */
#define H5Epush_sim(func,maj,min,str) H5Epush(__FILE__,func,__LINE__,maj,min,str)

/*
 * Public API Convenience Macros for Error reporting - Undocumented
 */
/* Use the Standard C __FILE__ & __LINE__ macros instead of typing them in */
/*  And return after pushing error onto stack */
#define H5Epush_ret(func,maj,min,str,ret) {         \
    H5Epush(__FILE__,func,__LINE__,maj,min,str);    \
    return(ret);                                    \
}

/* Use the Standard C __FILE__ & __LINE__ macros instead of typing them in */
/*  And goto a label after pushing error onto stack */
#define H5Epush_goto(func,maj,min,str,label) {      \
    H5Epush(__FILE__,func,__LINE__,maj,min,str);    \
    goto label;                                   \
}

/*
 * Declare an enumerated type which holds all the valid major HDF error codes.
 */
typedef enum H5E_major_t {
    H5E_NONE_MAJOR       = 0,   /*special zero, no error                     */
    H5E_ARGS,                   /*invalid arguments to routine               */
    H5E_RESOURCE,               /*resource unavailable                       */
    H5E_INTERNAL,               /* Internal error (too specific to document
                                 * in detail)
                                 */
    H5E_FILE,                   /*file Accessability                         */
    H5E_IO,                     /*Low-level I/O                              */
    H5E_FUNC,                   /*function Entry/Exit                        */
    H5E_ATOM,                   /*object Atom                                */
    H5E_CACHE,                  /*object Cache                               */
    H5E_BTREE,                  /*B-Tree Node                                */
    H5E_SYM,                    /*symbol Table                               */
    H5E_HEAP,                   /*Heap                                       */
    H5E_OHDR,                   /*object Header                              */
    H5E_DATATYPE,               /*Datatype                                   */
    H5E_DATASPACE,              /*Dataspace                                  */
    H5E_DATASET,                /*Dataset                                    */
    H5E_STORAGE,                /*data storage                               */
    H5E_PLIST,                  /*Property lists                             */
    H5E_ATTR,                   /*Attribute                                  */
    H5E_PLINE,                  /*Data filters                               */
    H5E_EFL,                    /*External file list                         */
    H5E_REFERENCE,              /*References                                 */
    H5E_VFL,                    /*Virtual File Layer                         */
    H5E_TBBT                    /*Threaded, Balanced, Binary Trees           */
} H5E_major_t;

/* Declare an enumerated type which holds all the valid minor HDF error codes */
typedef enum H5E_minor_t {
    H5E_NONE_MINOR       = 0,   /*special zero, no error                     */

    /* Argument errors */
    H5E_UNINITIALIZED,          /*information is unitialized                 */
    H5E_UNSUPPORTED,            /*feature is unsupported                     */
    H5E_BADTYPE,                /*incorrect type found                       */
    H5E_BADRANGE,               /*argument out of range                      */
    H5E_BADVALUE,               /*bad value for argument                     */

    /* Resource errors */
    H5E_NOSPACE,                /*no space available for allocation          */
    H5E_CANTCOPY,               /*unable to copy object                      */
    H5E_CANTFREE,               /*unable to free object                      */

    /* File accessability errors */
    H5E_FILEEXISTS,             /*file already exists                        */
    H5E_FILEOPEN,               /*file already open                          */
    H5E_CANTCREATE,             /*Can't create file                          */
    H5E_CANTOPENFILE,           /*Can't open file                            */
    H5E_CANTCLOSEFILE,          /*Can't close file                           */
    H5E_NOTHDF5,                /*not an HDF5 format file                    */
    H5E_BADFILE,                /*bad file ID accessed                       */
    H5E_TRUNCATED,              /*file has been truncated                    */
    H5E_MOUNT,                  /*file mount error                           */

    /* Generic low-level file I/O errors */
    H5E_SEEKERROR,              /*seek failed                                */
    H5E_READERROR,              /*read failed                                */
    H5E_WRITEERROR,             /*write failed                               */
    H5E_CLOSEERROR,             /*close failed                               */
    H5E_OVERFLOW,               /*address overflowed                         */

    /* Function entry/exit interface errors */
    H5E_CANTINIT,               /*Can't initialize                           */
    H5E_ALREADYINIT,            /*object already initialized                 */

    /* Object atom related errors */
    H5E_BADATOM,                /*Can't find atom information                */
    H5E_CANTREGISTER,           /*Can't register new atom                    */

    /* Cache related errors */
    H5E_CANTFLUSH,              /*Can't flush object from cache              */
    H5E_CANTLOAD,               /*Can't load object into cache               */
    H5E_PROTECT,                /*protected object error                     */
    H5E_NOTCACHED,              /*object not currently cached                */

    /* B-tree related errors */
    H5E_NOTFOUND,               /*object not found                           */
    H5E_EXISTS,                 /*object already exists                      */
    H5E_CANTENCODE,             /*Can't encode value                         */
    H5E_CANTDECODE,             /*Can't decode value                         */
    H5E_CANTSPLIT,              /*Can't split node                           */
    H5E_CANTINSERT,             /*Can't insert object                        */
    H5E_CANTLIST,               /*Can't list node                            */

    /* Object header related errors */
    H5E_LINKCOUNT,              /*bad object header link count               */
    H5E_VERSION,                /*wrong version number                       */
    H5E_ALIGNMENT,              /*alignment error                            */
    H5E_BADMESG,                /*unrecognized message                       */
    H5E_CANTDELETE,             /* Can't delete message                      */

    /* Group related errors */
    H5E_CANTOPENOBJ,            /*Can't open object                          */
    H5E_COMPLEN,                /*name component is too long                 */
    H5E_CWG,                    /*problem with current working group         */
    H5E_LINK,                   /*link count failure                         */
    H5E_SLINK,                  /*symbolic link error                        */

    /* Datatype conversion errors */
    H5E_CANTCONVERT,            /*Can't convert datatypes */

    /* Parallel errors */
    H5E_MPI                     /*some MPI function failed                   */
} H5E_minor_t;

/* Information about an error */
typedef struct H5E_error_t {
    H5E_major_t maj_num;                /*major error number                 */
    H5E_minor_t min_num;                /*minor error number                 */
    const char  *func_name;             /*function in which error occurred   */
    const char  *file_name;             /*file in which error occurred       */
    unsigned    line;                   /*line in file where error occurs    */
    const char  *desc;                  /*optional supplied description      */
} H5E_error_t;

/* Error stack traversal direction */
typedef enum H5E_direction_t {
    H5E_WALK_UPWARD     = 0,            /*begin deep, end at API function    */
    H5E_WALK_DOWNWARD   = 1             /*begin at API function, end deep    */
} H5E_direction_t;

/* Error stack traversal callback function */
typedef herr_t (*H5E_walk_t)(int n, H5E_error_t *err_desc, void *client_data);
typedef herr_t (*H5E_auto_t)(void *client_data);

#ifdef __cplusplus
extern "C" {
#endif

__DLL__ herr_t H5Eset_auto (H5E_auto_t func, void *client_data);
__DLL__ herr_t H5Eget_auto (H5E_auto_t *func, void **client_data);
__DLL__ herr_t H5Eclear (void);
__DLL__ herr_t H5Eprint (FILE *stream);
__DLL__ herr_t H5Ewalk (H5E_direction_t direction, H5E_walk_t func,
                        void *client_data);
__DLL__ herr_t H5Ewalk_cb (int n, H5E_error_t *err_desc, void *client_data);
__DLL__ const char *H5Eget_major (H5E_major_t major_number);
__DLL__ const char *H5Eget_minor (H5E_minor_t minor_number);
__DLL__ herr_t H5Epush(const char *file, const char *func,
            unsigned line, H5E_major_t maj, H5E_minor_t min, const char *str);

#ifdef __cplusplus
}
#endif
#endif
