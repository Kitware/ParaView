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

/*-------------------------------------------------------------------------
 *
 * Created:		H5ACprivate.h
 *			Jul  9 1997
 *			Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:		Constants and typedefs available to the rest of the
 *			library.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */

#ifndef _H5ACprivate_H
#define _H5ACprivate_H

#include "H5ACpublic.h"		/*public prototypes			     */

/* Pivate headers needed by this header */
#include "H5private.h"		/* Generic Functions			*/
#include "H5Fprivate.h"		/* File access				*/

/*
 * Feature: Define H5AC_DEBUG on the compiler command line if you want to
 *	    debug H5AC_protect() and H5AC_unprotect() by insuring that
 *	    nothing  accesses protected objects.  NDEBUG must not be defined
 *	    in order for this to have any effect.
 */
#ifdef NDEBUG
#  undef H5AC_DEBUG
#endif

/*
 * Class methods pertaining to caching.	 Each type of cached object will
 * have a constant variable with permanent life-span that describes how
 * to cache the object.	 That variable will be of type H5AC_class_t and
 * have the following required fields...
 *
 * LOAD:	Loads an object from disk to memory.  The function
 *		should allocate some data structure and return it.
 *
 * FLUSH:	Writes some data structure back to disk.  It would be
 *		wise for the data structure to include dirty flags to
 *		indicate whether it really needs to be written.	 This
 *		function is also responsible for freeing memory allocated
 *		by the LOAD method if the DEST argument is non-zero (by
 *              calling the DEST method).
 *
 * DEST:	Just frees memory allocated by the LOAD method.
 *
 * CLEAR:	Just marks object as non-dirty.
 */
typedef enum H5AC_subid_t {
    H5AC_BT_ID		= 0,	/*B-tree nodes				     */
    H5AC_SNODE_ID	= 1,	/*symbol table nodes			     */
    H5AC_LHEAP_ID	= 2,	/*local heap				     */
    H5AC_GHEAP_ID	= 3,	/*global heap				     */
    H5AC_OHDR_ID	= 4,	/*object header				     */
    H5AC_NTYPES		= 5	/*THIS MUST BE LAST!			     */
} H5AC_subid_t;

typedef void *(*H5AC_load_func_t)(H5F_t *f, hid_t dxpl_id, haddr_t addr, const void *udata1, void *udata2);
typedef herr_t (*H5AC_flush_func_t)(H5F_t *f, hid_t dxpl_id, hbool_t dest, haddr_t addr, void *thing);
typedef herr_t (*H5AC_dest_func_t)(H5F_t *f, void *thing);
typedef herr_t (*H5AC_clear_func_t)(void *thing);

typedef struct H5AC_class_t {
    H5AC_subid_t	id;
    H5AC_load_func_t    load;
    H5AC_flush_func_t   flush;
    H5AC_dest_func_t    dest;
    H5AC_clear_func_t   clear;
} H5AC_class_t;

/*
 * A cache has a certain number of entries.  Objects are mapped into a
 * cache entry by hashing the object's file address.  Each file has its
 * own cache, an array of slots.
 */
#define H5AC_NSLOTS	10330		/* The library "likes" this number... */

typedef struct H5AC_info_t {
    const H5AC_class_t	*type;		/*type of object stored here	     */
    haddr_t		addr;		/*file address for object	     */
    hbool_t             dirty;          /* 'Dirty' flag for cached object */
} H5AC_info_t;
typedef H5AC_info_t *H5AC_info_ptr_t;   /* Typedef for free lists */

/* Typedef for metadata cache (defined in H5AC.c) */
typedef struct H5AC_t H5AC_t;

/* Metadata specific properties for FAPL */
/* (Only used for parallel I/O) */
#ifdef H5_HAVE_PARALLEL
/* Definitions for "block before metadata write" property */
#define H5AC_BLOCK_BEFORE_META_WRITE_NAME       "H5AC_block_before_meta_write"
#define H5AC_BLOCK_BEFORE_META_WRITE_SIZE       sizeof(unsigned)
#define H5AC_BLOCK_BEFORE_META_WRITE_DEF        0

/* Definitions for "library internal" property */
#define H5AC_LIBRARY_INTERNAL_NAME       "H5AC_library_internal"
#define H5AC_LIBRARY_INTERNAL_SIZE       sizeof(unsigned)
#define H5AC_LIBRARY_INTERNAL_DEF        0
#endif /* H5_HAVE_PARALLEL */

/* Dataset transfer property list for flush calls */
/* (Collective set, "block before metadata write" set and "library internal" set) */
/* (Global variable declaration, definition is in H5AC.c) */
extern hid_t H5AC_dxpl_id;

/* Dataset transfer property list for independent metadata I/O calls */
/* (just "library internal" set - i.e. independent transfer mode) */
/* (Global variable declaration, definition is in H5AC.c) */
extern hid_t H5AC_ind_dxpl_id;

/*
 * Library prototypes.
 */
H5_DLL herr_t H5AC_init(void);
H5_DLL herr_t H5AC_create(H5F_t *f, int size_hint);
H5_DLL herr_t H5AC_set(H5F_t *f, hid_t dxpl_id, const H5AC_class_t *type, haddr_t addr,
			void *thing);
H5_DLL void *H5AC_protect(H5F_t *f, hid_t dxpl_id, const H5AC_class_t *type, haddr_t addr,
			   const void *udata1, void *udata2);
H5_DLL herr_t H5AC_unprotect(H5F_t *f, hid_t dxpl_id, const H5AC_class_t *type, haddr_t addr,
			      void *thing, hbool_t deleted);
H5_DLL void *H5AC_find(H5F_t *f, hid_t dxpl_id, const H5AC_class_t *type,
        haddr_t addr, const void *udata1, void *udata2);
H5_DLL herr_t H5AC_flush(H5F_t *f, hid_t dxpl_id, const H5AC_class_t *type, haddr_t addr,
			  unsigned flags);
H5_DLL herr_t H5AC_rename(H5F_t *f, hid_t dxpl_id, const H5AC_class_t *type,
			   haddr_t old_addr, haddr_t new_addr);
H5_DLL herr_t H5AC_dest(H5F_t *f, hid_t dxpl_id);
H5_DLL herr_t H5AC_debug(H5F_t *f);

#endif /* !_H5ACprivate_H */

