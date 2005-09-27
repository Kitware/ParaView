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

#define H5Z_PACKAGE		/*suppress error about including H5Zpkg	  */

#include "H5private.h"		/* Generic Functions			*/
#include "H5Eprivate.h"		/* Error handling		  	*/
#include "H5MMprivate.h"	/* Memory management			*/
#include "H5Ppublic.h"		/* Property lists			*/
#include "H5Tpublic.h"		/* Datatype functions			*/
#include "H5Zpkg.h"		/* Data filters				*/

#ifdef H5_HAVE_FILTER_SHUFFLE

/* Interface initialization */
#define PABLO_MASK	H5Z_shuffle_mask
#define INTERFACE_INIT	NULL
static int interface_initialize_g = 0;

/* Local function prototypes */
static herr_t H5Z_set_local_shuffle(hid_t dcpl_id, hid_t type_id, hid_t space_id);
static size_t H5Z_filter_shuffle(unsigned flags, size_t cd_nelmts,
    const unsigned cd_values[], size_t nbytes, size_t *buf_size, void **buf);

/* This message derives from H5Z */
const H5Z_class_t H5Z_SHUFFLE[1] = {{
    H5Z_FILTER_SHUFFLE,		/* Filter id number		*/
    "shuffle",			/* Filter name for debugging	*/
    NULL,                       /* The "can apply" callback     */
    H5Z_set_local_shuffle,      /* The "set local" callback     */
    H5Z_filter_shuffle,		/* The actual filter function	*/
}};

/* Local macros */
#define H5Z_SHUFFLE_USER_NPARMS    0       /* Number of parameters that users can set */
#define H5Z_SHUFFLE_TOTAL_NPARMS   1       /* Total number of parameters for filter */
#define H5Z_SHUFFLE_PARM_SIZE      0       /* "Local" parameter for shuffling size */


/*-------------------------------------------------------------------------
 * Function:	H5Z_set_local_shuffle
 *
 * Purpose:	Set the "local" dataset parameter for data shuffling to be
 *              the size of the datatype.
 *
 * Return:	Success: Non-negative
 *		Failure: Negative
 *
 * Programmer:	Quincey Koziol
 *              Monday, April  7, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5Z_set_local_shuffle(hid_t dcpl_id, hid_t type_id, hid_t UNUSED space_id)
{
    unsigned flags;         /* Filter flags */
    size_t cd_nelmts=H5Z_SHUFFLE_USER_NPARMS;     /* Number of filter parameters */
    unsigned cd_values[H5Z_SHUFFLE_TOTAL_NPARMS];  /* Filter parameters */
    herr_t ret_value=SUCCEED;   /* Return value */

    FUNC_ENTER_NOAPI(H5Z_set_local_shuffle, FAIL);

    /* Get the filter's current parameters */
    if(H5Pget_filter_by_id(dcpl_id,H5Z_FILTER_SHUFFLE,&flags,&cd_nelmts, cd_values,0,NULL)<0)
	HGOTO_ERROR(H5E_PLINE, H5E_CANTGET, FAIL, "can't get shuffle parameters");

    /* Check that no parameters are currently set */
    if(cd_nelmts!=H5Z_SHUFFLE_USER_NPARMS)
	HGOTO_ERROR(H5E_PLINE, H5E_BADVALUE, FAIL, "incorrect # of shuffle parameters");

    /* Set "local" parameter for this dataset */
    if((cd_values[H5Z_SHUFFLE_PARM_SIZE]=(unsigned)H5Tget_size(type_id))==0)
	HGOTO_ERROR(H5E_PLINE, H5E_BADTYPE, FAIL, "bad datatype size");

    /* Modify the filter's parameters for this dataset */
    if(H5Pmodify_filter(dcpl_id, H5Z_FILTER_SHUFFLE, flags, H5Z_SHUFFLE_TOTAL_NPARMS, cd_values)<0)
	HGOTO_ERROR(H5E_PLINE, H5E_CANTSET, FAIL, "can't set local shuffle parameters");

done:
    FUNC_LEAVE_NOAPI(ret_value);
} /* end H5Z_set_local_shuffle() */


/*-------------------------------------------------------------------------
 * Function:	H5Z_filter_shuffle
 *
 * Purpose:	Implement an I/O filter which "de-interlaces" a block of data
 *              by putting all the bytes in a byte-position for each element
 *              together in the block.  For example, for 4-byte elements stored
 *              as: 012301230123, shuffling will store them as: 000111222333
 *              Usually, the bytes in each byte position are more related to
 *              each other and putting them together will increase compression.
 *
 * Return:	Success: Size of buffer filtered
 *		Failure: 0	
 *
 * Programmer:	Kent Yang
 *              Wednesday, November 13, 2002
 *
 * Modifications:
 *              Quincey Koziol, November 13, 2002
 *              Cleaned up code.
 *
 *-------------------------------------------------------------------------
 */
static size_t
H5Z_filter_shuffle(unsigned flags, size_t cd_nelmts, const unsigned cd_values[], 
                   size_t nbytes, size_t *buf_size, void **buf)
{
    void *dest = NULL;          /* Buffer to deposit [un]shuffled bytes into */
    unsigned char *_src=NULL;   /* Alias for source buffer */
    unsigned char *_dest=NULL;  /* Alias for destination buffer */
    unsigned bytesoftype;       /* Number of bytes per element */
    size_t numofelements;       /* Number of elements in buffer */
    size_t i,j;                 /* Local index variables */
    size_t leftover;            /* Extra bytes at end of buffer */
    size_t ret_value;           /* Return value */

    FUNC_ENTER_NOAPI(H5Z_filter_shuffle, 0);

    /* Check arguments */
    if (cd_nelmts!=H5Z_SHUFFLE_TOTAL_NPARMS || cd_values[H5Z_SHUFFLE_PARM_SIZE]==0)
	HGOTO_ERROR(H5E_ARGS, H5E_BADVALUE, 0, "invalid shuffle parameters");

    /* Get the number of bytes per element from the parameter block */
    bytesoftype=cd_values[H5Z_SHUFFLE_PARM_SIZE];

    /* Don't do anything for 1-byte elements */
    if(bytesoftype>1) {
        /* Compute the number of elements in buffer */
        numofelements=nbytes/bytesoftype;

        /* Compute the leftover bytes if there are any */
        leftover = nbytes%bytesoftype;

        /* Allocate the destination buffer */
        if (NULL==(dest = H5MM_malloc(nbytes)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, 0, "memory allocation failed for shuffle buffer");

        if(flags & H5Z_FLAG_REVERSE) {
            /* Get the pointer to the source buffer */
            _src =(unsigned char *)(*buf);

            /* Input; unshuffle */
            for(i=0; i<bytesoftype; i++) {
                _dest=((unsigned char *)dest)+i;
                for(j=0; j<numofelements; j++) {
                    *_dest=*_src++;
                    _dest+=bytesoftype;
                } /* end for */
            } /* end for */

            /* Add leftover to the end of data */ 
            if(leftover>0) {
                /* Adjust back to end of shuffled bytes */
                _dest -= (bytesoftype - 1);
                HDmemcpy((void*)_dest, (void*)_src, leftover);
            }
        } /* end if */
        else {
            /* Get the pointer to the destination buffer */
            _dest =(unsigned char *)dest;

            /* Output; shuffle */
            for(i=0; i<bytesoftype; i++) {
                _src=((unsigned char *)(*buf))+i;
                for(j=0; j<numofelements; j++) {
                    *_dest++=*_src;
                    _src+=bytesoftype;
                } /* end for */
            } /* end for */

            /* Add leftover to the end of data */ 
            if(leftover>0) {
                /* Adjust back to end of shuffled bytes */
                _src -= (bytesoftype - 1);
                HDmemcpy((void*)_dest, (void*)_src, leftover);
            }
        } /* end else */

        /* Free the input buffer */
        H5MM_xfree(*buf);

        /* Set the buffer information to return */
        *buf = dest;
        *buf_size=nbytes;
    } /* end else */

    /* Set the return value */
    ret_value = nbytes;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}
#endif /*H5_HAVE_FILTER_SHUFFLE */

