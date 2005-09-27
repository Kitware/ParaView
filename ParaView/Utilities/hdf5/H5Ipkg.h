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
 * Programmer:	Quincey Koziol <koziol@ncsa.uiuc.edu>
 *		Thursday, May 15, 2003
 *
 * Purpose:	This file contains declarations which are visible only within
 *		the H5I package.  Source files outside the H5I package should
 *		include H5Iprivate.h instead.
 */
#ifndef H5I_PACKAGE
#error "Do not include this file outside the H5I package!"
#endif

#ifndef _H5Ipkg_H
#define _H5Ipkg_H

/* Get package's private header */
#include "H5Iprivate.h"

/* Other private headers needed by this file */

/**************************/
/* Package Private Macros */
/**************************/

/*
 * Number of bits to use for Group ID in each atom. Increase if H5I_NGROUPS
 * becomes too large (an assertion would fail in H5I_init_interface). This is
 * the only number that must be changed since all other bit field sizes and
 * masks are calculated from GROUP_BITS.
 */
#define GROUP_BITS	5
#define GROUP_MASK	((1<<GROUP_BITS)-1)

/*
 * Number of bits to use for the Atom index in each atom (assumes 8-bit
 * bytes). We don't use the sign bit.
 */
#define ID_BITS		((sizeof(hid_t)*8)-(GROUP_BITS+1))
#define ID_MASK		((1<<ID_BITS)-1)

/* Map an atom to a Group number */
#define H5I_GROUP(a)	((H5I_type_t)(((hid_t)(a)>>ID_BITS) & GROUP_MASK))


/****************************/
/* Package Private Typedefs */
/****************************/

/******************************/
/* Package Private Prototypes */
/******************************/

#endif /*_H5Ipkg_H*/
