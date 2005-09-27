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
 * Programmer:  Quincey Koziol
 *              Monday, Apr 20
 *
 * Purpose:     This file contains declarations which are visible only within
 *              the H5A package.  Source files outside the H5A package should
 *              include H5Aprivate.h instead.
 */
#ifndef H5A_PACKAGE
#error "Do not include this file outside the H5A package!"
#endif

#ifndef _H5Apkg_H
#define _H5Apkg_H

/*
 * Define this to enable debugging.
 */
#ifdef NDEBUG
#  undef H5A_DEBUG
#endif

/* Get package's private header */
#include "H5Aprivate.h"

/* Other private headers needed by this file */
#include "H5HGprivate.h"	/* Global heaps				*/
#include "H5Sprivate.h"		/* Dataspace				*/

struct H5A_t {
    unsigned       initialized;/* Indicate whether the attribute has been modified */
    unsigned       ent_opened; /* Object header entry opened? */
    H5G_entry_t ent;        /* Object Header entry (for both datasets & groups) */
    char        *name;      /* Attribute's name */
    H5T_t       *dt;        /* Attribute's datatype */
    size_t      dt_size;    /* Size of datatype on disk */
    H5S_t       *ds;        /* Attribute's dataspace */
    size_t      ds_size;    /* Size of dataspace on disk */
    void        *data;      /* Attribute data (on a temporary basis) */
    size_t      data_size;  /* Size of data on disk */
    H5HG_t	sh_heap;    /*if defined, attribute is in global heap	     */
    H5F_t	*sh_file;   /*file pointer if this is a shared attribute    */
};

/* Function prototypes for H5A package scope */
H5_DLL H5A_t       *H5A_copy(const H5A_t *old_attr);
H5_DLL herr_t      H5A_close(H5A_t *attr);

#endif
