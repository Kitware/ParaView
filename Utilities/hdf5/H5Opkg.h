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

#ifndef H5O_PACKAGE
#error "Do not include this file outside the H5O package!"
#endif

#ifndef _H5Opkg_H
#define _H5Opkg_H

/* Include private header file */
#include "H5Oprivate.h"          /* Object header functions                */

/*
 * Align messages on 8-byte boundaries because we would like to copy the
 * object header chunks directly into memory and operate on them there, even
 * on 64-bit architectures.  This allows us to reduce the number of disk I/O
 * requests with a minimum amount of mem-to-mem copies.
 */
#define H5O_ALIGN(X)		(8*(((X)+8-1)/8))

/* Object header macros */
#define H5O_NMESGS	32		/*initial number of messages	     */
#define H5O_NCHUNKS	8		/*initial number of chunks	     */
#define H5O_ALL		(-1)		/*delete all messages of type	     */

/* Version of object header structure */
#define H5O_VERSION		1

/*
 * Size of object header header.
 */
#define H5O_SIZEOF_HDR(F)						      \
    H5O_ALIGN(1 +		/*version number	*/		      \
	      1 +		/*alignment		*/		      \
	      2 +		/*number of messages	*/		      \
	      4 +		/*reference count	*/		      \
	      4)		/*header data size	*/

/*
 * Size of message header
 */
#define H5O_SIZEOF_MSGHDR(F)						      \
     H5O_ALIGN(2 +	/*message type		*/			      \
	       2 +	/*sizeof message data	*/			      \
	       4)	/*reserved		*/

typedef struct H5O_class_t {
    int	id;				 /*message type ID on disk   */
    const char	*name;				 /*for debugging             */
    size_t	native_size;			 /*size of native message    */
    void	*(*decode)(H5F_t*, hid_t, const uint8_t*, struct H5O_shared_t*);
    herr_t	(*encode)(H5F_t*, uint8_t*, const void*);
    void	*(*copy)(const void*, void*);    /*copy native value         */
    size_t	(*raw_size)(H5F_t*, const void*);/*sizeof raw val	     */
    herr_t	(*reset)(void *);		 /*free nested data structs  */
    herr_t	(*free)(void *);		 /*free main data struct  */
    herr_t	(*del)(H5F_t *, hid_t, const void *); /* Delete space in file referenced by this message */
    herr_t	(*link)(H5F_t *, hid_t, const void *); /* Increment any links in file reference by this message */
    herr_t	(*get_share)(H5F_t*, const void*, struct H5O_shared_t*);    /* Get shared information */
    herr_t	(*set_share)(H5F_t*, void*, const struct H5O_shared_t*);    /* Set shared information */
    herr_t	(*debug)(H5F_t*, hid_t, const void*, FILE*, int, int);
} H5O_class_t;

typedef struct H5O_mesg_t {
    const H5O_class_t	*type;		/*type of message		     */
    hbool_t		dirty;		/*raw out of date wrt native	     */
    uint8_t		flags;		/*message flags			     */
    void		*native;	/*native format message		     */
    uint8_t		*raw;		/*ptr to raw data		     */
    size_t		raw_size;	/*size with alignment		     */
    unsigned		chunkno;	/*chunk number for this mesg	     */
} H5O_mesg_t;

typedef struct H5O_chunk_t {
    hbool_t	dirty;			/*dirty flag			     */
    haddr_t	addr;			/*chunk file address		     */
    size_t	size;			/*chunk size			     */
    uint8_t	*image;			/*image of file			     */
} H5O_chunk_t;

typedef struct H5O_t {
    H5AC_info_t cache_info; /* Information for H5AC cache functions, _must_ be */
                            /* first field in structure */
    int		version;		/*version number		     */
    int		nlink;			/*link count			     */
    unsigned	nmesgs;			/*number of messages		     */
    unsigned	alloc_nmesgs;		/*number of message slots	     */
    H5O_mesg_t	*mesg;			/*array of messages		     */
    unsigned	nchunks;		/*number of chunks		     */
    unsigned	alloc_nchunks;		/*chunks allocated		     */
    H5O_chunk_t *chunk;			/*array of chunks		     */
} H5O_t;

/*
 * Null Message.
 */
H5_DLLVAR const H5O_class_t H5O_NULL[1];

/*
 * Simple Data Space Message.
 */
H5_DLLVAR const H5O_class_t H5O_SDSPACE[1];

/*
 * Data Type Message.
 */
H5_DLLVAR const H5O_class_t H5O_DTYPE[1];

/*
 * Old Fill Value Message.
 */
H5_DLLVAR const H5O_class_t H5O_FILL[1];

/*
 * New Fill Value Message.  The new fill value message is fill value plus 
 * space allocation time and fill value writing time and whether fill 
 * value is defined.
 */
H5_DLLVAR const H5O_class_t H5O_FILL_NEW[1];

/*
 * External File List Message
 */
H5_DLLVAR const H5O_class_t H5O_EFL[1];

/*
 * Data Layout Message.
 */
H5_DLLVAR const H5O_class_t H5O_LAYOUT[1];

#ifdef H5O_ENABLE_BOGUS
/*
 * "Bogus" Message.
 */
H5_DLLVAR const H5O_class_t H5O_BOGUS[1];
#endif /* H5O_ENABLE_BOGUS */

/*
 * Filter pipeline message.
 */
H5_DLLVAR const H5O_class_t H5O_PLINE[1];

/*
 * Attribute Message.
 */
H5_DLLVAR const H5O_class_t H5O_ATTR[1];

/*
 * Object name message.
 */
H5_DLLVAR const H5O_class_t H5O_NAME[1];

/*
 * Modification time message.  The message is just a `time_t'.
 * (See also the "new" modification time message)
 */
H5_DLLVAR const H5O_class_t H5O_MTIME[1];

/*
 * Shared object message.  This message ID never really appears in an object
 * header.  Instead, bit 2 of the `Flags' field will be set and the ID field
 * will be the ID of the pointed-to message.
 */
H5_DLLVAR const H5O_class_t H5O_SHARED[1];

/*
 * Object header continuation message.
 */
H5_DLLVAR const H5O_class_t H5O_CONT[1];

/*
 * Symbol table message.
 */
H5_DLLVAR const H5O_class_t H5O_STAB[1];

/*
 * New Modification time message.  The message is just a `time_t'.
 */
H5_DLLVAR const H5O_class_t H5O_MTIME_NEW[1];

/*
 * Generic property list message.
 */
H5_DLLVAR const H5O_class_t H5O_PLIST[1];

/* Package-local function prototypes */
H5_DLL void * H5O_read_real(H5G_entry_t *ent, const H5O_class_t *type,
        int sequence, void *mesg, hid_t dxpl_id);
H5_DLL void * H5O_free_real(const H5O_class_t *type, void *mesg);

/* Shared object operators */
H5_DLL void * H5O_shared_read(H5F_t *f, hid_t dxpl_id, H5O_shared_t *shared,
    const H5O_class_t *type, void *mesg);

/* Symbol table operators */
H5_DLL void *H5O_stab_fast(const H5G_cache_t *cache, const struct H5O_class_t *type,
			    void *_mesg);

#endif /* _H5Opkg_H */
