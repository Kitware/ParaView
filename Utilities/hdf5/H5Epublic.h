/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by the Board of Trustees of the University of Illinois.         *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the files COPYING and Copyright.html.  COPYING can be found at the root   *
 * of the source code distribution tree; Copyright.html can be found at the  *
 * root level of an installed copy of the electronic HDF5 document set and   *
 * is linked from the top-level documents page.  It can also be found at     *
 * http://hdf.ncsa.uiuc.edu/HDF5/doc/Copyright.html.  If you do not have     *
 * access to either file, you may request a copy from hdfhelp@ncsa.uiuc.edu. *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * This file contains public declarations for the H5E module.
 */
#ifndef _H5Epublic_H
#define _H5Epublic_H

#include <stdio.h>              /*FILE arg of H5Eprint()                     */

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
 *   H5E_BEGIN_TRY {
 *      ...stuff here that's likely to fail...
 *      } H5E_END_TRY;
 *
 * Warning: don't break, return, or longjmp() from the body of the loop or
 *      the error reporting won't be properly restored!
 */
#define H5E_BEGIN_TRY {                    \
    H5E_auto_t H5E_saved_efunc;                  \
    void *H5E_saved_edata;                  \
    H5Eget_auto (&H5E_saved_efunc, &H5E_saved_edata);            \
    H5Eset_auto (NULL, NULL);

#define H5E_END_TRY                    \
    H5Eset_auto (H5E_saved_efunc, H5E_saved_edata);            \
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
    H5E_VFL,      /*Virtual File Layer           */
    H5E_TBBT,             /*Threaded, Balanced, Binary Trees           */
    H5E_TST,             /*Ternary Search Trees                       */
    H5E_RS,              /*Reference Counted Strings                  */
    H5E_ERROR,      /*Error API                                  */
    H5E_SLIST              /*Skip Lists                                 */
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
    H5E_ALREADYEXISTS,          /*Object already exists */
    H5E_CANTLOCK,               /*Unable to lock object                      */
    H5E_CANTUNLOCK,             /*Unable to unlock object                    */
    H5E_CANTGC,                 /*Unable to garbage collect                  */
    H5E_CANTGETSIZE,            /*Unable to compute size                     */

    /* File accessability errors */
    H5E_FILEEXISTS,             /*file already exists                        */
    H5E_FILEOPEN,               /*file already open                          */
    H5E_CANTCREATE,             /*Can't create file                          */
    H5E_CANTOPENFILE,           /*Can't open file                            */
    H5E_CANTCLOSEFILE,          /*Can't close file           */
    H5E_NOTHDF5,                /*not an HDF5 format file                    */
    H5E_BADFILE,                /*bad file ID accessed                       */
    H5E_TRUNCATED,              /*file has been truncated                    */
    H5E_MOUNT,      /*file mount error           */

    /* Generic low-level file I/O errors */
    H5E_SEEKERROR,              /*seek failed                                */
    H5E_READERROR,              /*read failed                                */
    H5E_WRITEERROR,             /*write failed                               */
    H5E_CLOSEERROR,             /*close failed                               */
    H5E_OVERFLOW,    /*address overflowed           */
    H5E_FCNTL,                  /*file fcntl failed                          */

    /* Function entry/exit interface errors */
    H5E_CANTINIT,               /*Can't initialize object                    */
    H5E_ALREADYINIT,            /*object already initialized                 */
    H5E_CANTRELEASE,            /*Can't release object                       */

    /* Object atom related errors */
    H5E_BADATOM,                /*Can't find atom information                */
    H5E_BADGROUP,               /*Can't find group information               */
    H5E_CANTREGISTER,           /*Can't register new atom                    */
    H5E_CANTINC,                /*Can't increment reference count            */
    H5E_CANTDEC,                /*Can't decrement reference count            */
    H5E_NOIDS,                  /*Out of IDs for group                       */

    /* Cache related errors */
    H5E_CANTFLUSH,              /*Can't flush object from cache              */
    H5E_CANTSERIALIZE,          /*Unable to serialize data from cache        */
    H5E_CANTLOAD,               /*Can't load object into cache               */
    H5E_PROTECT,                /*protected object error                     */
    H5E_NOTCACHED,              /*object not currently cached                */
    H5E_SYSTEM,                 /*Internal error detected                    */
    H5E_CANTINS,                /*Unable to insert metadata into cache       */
    H5E_CANTRENAME,             /*Unable to rename metadata                  */
    H5E_CANTPROTECT,            /*Unable to protect metadata                 */
    H5E_CANTUNPROTECT,          /*Unable to unprotect metadata               */

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
    H5E_BADITER,                /* Iteration failed                          */

    /* Group related errors */
    H5E_CANTOPENOBJ,            /*Can't open object                          */
    H5E_CANTCLOSEOBJ,           /*Can't close object                         */
    H5E_COMPLEN,                /*name component is too long                 */
    H5E_LINK,                   /*link count failure                         */
    H5E_SLINK,      /*symbolic link error           */
    H5E_PATH,      /*Problem with path to object         */

    /* Datatype conversion errors */
    H5E_CANTCONVERT,            /*Can't convert datatypes                    */
    H5E_BADSIZE,                /*Bad size for object                        */

    /* Dataspace errors */
    H5E_CANTCLIP,               /*Can't clip hyperslab region                */
    H5E_CANTCOUNT,              /*Can't count elements                       */
    H5E_CANTSELECT,             /*Can't select hyperslab                     */
    H5E_CANTNEXT,               /*Can't move to next iterator location       */
    H5E_BADSELECT,              /*Invalid selection                          */
    H5E_CANTCOMPARE,            /*Can't compare objects                      */

    /* Property list errors */
    H5E_CANTGET,                /*Can't get value                            */
    H5E_CANTSET,                /*Can't set value                            */
    H5E_DUPCLASS,               /*Duplicate class name in parent class       */

    /* Parallel errors */
    H5E_MPI,      /*some MPI function failed         */
    H5E_MPIERRSTR,    /*MPI Error String            */

    /* Heap errors */
    H5E_CANTRESTORE,    /*Can't restore condition                    */

    /* TBBT errors */
    H5E_CANTMAKETREE,    /*Can't create TBBT tree                     */

    /* I/O pipeline errors */
    H5E_NOFILTER,               /*requested filter is not available          */
    H5E_CALLBACK,               /*callback failed                            */
    H5E_CANAPPLY,               /*error from filter "can apply" callback     */
    H5E_SETLOCAL,               /*error from filter "set local" callback     */
    H5E_NOENCODER,              /* Filter present, but encoding disabled     */

    /* System level errors */
    H5E_SYSERRSTR               /* System error message           */
} H5E_minor_t;

/* Information about an error */
typedef struct H5E_error_t {
    H5E_major_t maj_num;    /*major error number         */
    H5E_minor_t min_num;    /*minor error number         */
    const char  *func_name;       /*function in which error occurred   */
    const char  *file_name;    /*file in which error occurred       */
    unsigned  line;      /*line in file where error occurs    */
    const char  *desc;      /*optional supplied description      */
} H5E_error_t;

/* Error stack traversal direction */
typedef enum H5E_direction_t {
    H5E_WALK_UPWARD  = 0,    /*begin deep, end at API function    */
    H5E_WALK_DOWNWARD  = 1    /*begin at API function, end deep    */
} H5E_direction_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Error stack traversal callback function */
typedef herr_t (*H5E_walk_t)(int n, H5E_error_t *err_desc, void *client_data);
typedef herr_t (*H5E_auto_t)(void *client_data);

H5_DLL herr_t H5Eset_auto (H5E_auto_t func, void *client_data);
H5_DLL herr_t H5Eget_auto (H5E_auto_t *func, void **client_data);
H5_DLL herr_t H5Eclear (void);
H5_DLL herr_t H5Eprint (FILE *stream);
H5_DLL herr_t H5Ewalk (H5E_direction_t direction, H5E_walk_t func,
      void *client_data);
H5_DLL const char *H5Eget_major (H5E_major_t major_number);
H5_DLL const char *H5Eget_minor (H5E_minor_t minor_number);
H5_DLL herr_t H5Epush(const char *file, const char *func,
            unsigned line, H5E_major_t maj, H5E_minor_t min, const char *str);

#ifdef __cplusplus
}
#endif
#endif
