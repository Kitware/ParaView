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
 * This file contains private information about the H5S module
 */
#ifndef _H5Sprivate_H
#define _H5Sprivate_H

#include "H5Spublic.h"

/* Private headers needed by this file */
#include "H5private.h"		/* Generic Functions			*/
#include "H5Dprivate.h"		/* Dataset functions			*/
#include "H5Fprivate.h"		/* Files				*/
#include "H5Gprivate.h"		/* Groups				*/
#include "H5Oprivate.h"		/* Object headers		  	*/
#include "H5Pprivate.h"		/* Property lists			*/

/* Flags for H5S_find */
#define H5S_CONV_PAR_IO_POSSIBLE        0x0001
/* The storage options are mutually exclusive */
/* (2-bits reserved for storage type currently) */
#define H5S_CONV_STORAGE_COMPACT        0x0000  /* i.e. '0' */
#define H5S_CONV_STORAGE_CONTIGUOUS     0x0002  /* i.e. '1' */
#define H5S_CONV_STORAGE_CHUNKED        0x0004  /* i.e. '2' */
#define H5S_CONV_STORAGE_MASK           0x0006

/* Forward references of package typedefs */
typedef struct H5S_t H5S_t;
typedef struct H5S_pnt_node_t H5S_pnt_node_t;
typedef struct H5S_hyper_span_t H5S_hyper_span_t;
typedef struct H5S_hyper_span_info_t H5S_hyper_span_info_t;
typedef struct H5S_hyper_dim_t H5S_hyper_dim_t;

/* Point selection iteration container */
typedef struct {
    H5S_pnt_node_t *curr;   /* Pointer to next node to output */
} H5S_point_iter_t;

/* Hyperslab selection iteration container */
typedef struct {
    /* Common fields for all hyperslab selections */
    hssize_t *off;          /* Offset in span node (used as position for regular hyperslabs) */
    unsigned iter_rank;     /* Rank of iterator information */
                            /* (This should always be the same as the dataspace
                             * rank, except for regular hyperslab selections in
                             * which there are contiguous regions in the lower
                             * dimensions which have been "flattened" out
                             */

    /* "Flattened" regular hyperslab selection fields */
    H5S_hyper_dim_t *diminfo;   /* "Flattened" regular selection information */
    hsize_t *size;          /* "Flattened" dataspace extent information */
    hssize_t *sel_off;      /* "Flattened" selection offset information */

    /* Irregular hyperslab selection fields */
    H5S_hyper_span_info_t *spans;  /* Pointer to copy of the span tree */
    H5S_hyper_span_t **span;/* Array of pointers to span nodes */
} H5S_hyper_iter_t;

/* "All" selection iteration container */
typedef struct {
    hsize_t offset;         /* Next element to output */
} H5S_all_iter_t;

typedef struct H5S_sel_iter_t H5S_sel_iter_t;

/* Method to retrieve the current coordinates of iterator for current selection */
typedef herr_t (*H5S_sel_iter_coords_func_t)(const H5S_sel_iter_t *iter, hssize_t *coords);
/* Method to retrieve the current block of iterator for current selection */
typedef herr_t (*H5S_sel_iter_block_func_t)(const H5S_sel_iter_t *iter, hssize_t *start, hssize_t *end);
/* Method to determine number of elements left in iterator for current selection */
typedef hsize_t (*H5S_sel_iter_nelmts_func_t)(const H5S_sel_iter_t *iter);
/* Method to determine if there are more blocks left in the current selection */
typedef htri_t (*H5S_sel_iter_has_next_block_func_t)(const H5S_sel_iter_t *iter);
/* Method to move selection iterator to the next element in the selection */
typedef herr_t (*H5S_sel_iter_next_func_t)(H5S_sel_iter_t *iter, size_t nelem);
/* Method to move selection iterator to the next block in the selection */
typedef herr_t (*H5S_sel_iter_next_block_func_t)(H5S_sel_iter_t *iter);
/* Method to release iterator for current selection */
typedef herr_t (*H5S_sel_iter_release_func_t)(H5S_sel_iter_t *iter);

/* Selection iteration container */
struct H5S_sel_iter_t {
    /* Information common to all iterators */
    unsigned rank;              /* Rank of dataspace the selection iterator is operating on */
    hsize_t *dims;              /* Dimensions of dataspace the selection is operating on */
    hsize_t elmt_left;          /* Number of elements left to iterate over */

    /* Information specific to each type of iterator */
    union {
        H5S_point_iter_t pnt;   /* Point selection iteration information */
        H5S_hyper_iter_t hyp;   /* New Hyperslab selection iteration information */
        H5S_all_iter_t all;     /* "All" selection iteration information */
    } u;

    /* Methods on selections */
    H5S_sel_iter_coords_func_t iter_coords;     /* Method to retrieve the current coordinates of iterator for current selection */
    H5S_sel_iter_block_func_t iter_block;       /* Method to retrieve the current block of iterator for current selection */
    H5S_sel_iter_nelmts_func_t iter_nelmts;     /* Method to determine number of elements left in iterator for current selection */
    H5S_sel_iter_has_next_block_func_t iter_has_next_block;         /* Method to query if there is another block left in the selection */
    H5S_sel_iter_next_func_t iter_next;         /* Method to move selection iterator to the next element in the selection */
    H5S_sel_iter_next_block_func_t iter_next_block;     /* Method to move selection iterator to the next block in the selection */
    H5S_sel_iter_release_func_t iter_release;   /* Method to release iterator for current selection */
};

typedef struct H5S_conv_t {
    H5S_sel_type	ftype;
    H5S_sel_type	mtype;

    /*
     * If there is no data type conversion then it might be possible to
     * transfer data points between application memory and the file in one
     * step without going through the data type conversion buffer.
     */
    
    /* Read from file to application w/o intermediate scratch buffer */
    herr_t (*read)(H5F_t *f, const struct H5O_layout_t *layout,
           H5P_genplist_t *dc_plist, const H5D_storage_t *store,
           size_t elmt_size, const H5S_t *file_space,
           const H5S_t *mem_space, hid_t dxpl_id, void *buf/*out*/);


    /* Write directly from app buffer to file */
    herr_t (*write)(H5F_t *f, struct H5O_layout_t *layout,
           H5P_genplist_t *dc_plist, const union H5D_storage_t *store, 
           size_t elmt_size, const H5S_t *file_space,
           const H5S_t *mem_space, hid_t dxpl_id, const void *buf);
    
#ifdef H5S_DEBUG
    struct {
	H5_timer_t	scat_timer;		/*time spent scattering	*/
	hsize_t		scat_nbytes;		/*scatter throughput	*/
	hsize_t		scat_ncalls;		/*number of calls	*/
	H5_timer_t	gath_timer;		/*time spent gathering	*/
	hsize_t		gath_nbytes;		/*gather throughput	*/
	hsize_t		gath_ncalls;		/*number of calls	*/
	H5_timer_t	bkg_timer;		/*time for background	*/
	hsize_t		bkg_nbytes;		/*background throughput	*/
	hsize_t		bkg_ncalls;		/*number of calls	*/
	H5_timer_t	read_timer;		/*time for read calls	*/
	hsize_t		read_nbytes;		/*total bytes read	*/
	hsize_t		read_ncalls;		/*number of calls	*/
	H5_timer_t	write_timer;		/*time for write calls	*/
	hsize_t		write_nbytes;		/*total bytes written	*/
	hsize_t		write_ncalls;		/*number of calls	*/
    } stats[2];		/* 0=output, 1=input */
#endif
} H5S_conv_t;

/* Operations on dataspaces */
H5_DLL H5S_t *H5S_copy(const H5S_t *src);
H5_DLL herr_t H5S_close(H5S_t *ds);
H5_DLL H5S_conv_t *H5S_find(const H5S_t *mem_space, const H5S_t *file_space,
                unsigned flags, hbool_t *use_par_opt_io);
H5_DLL H5S_class_t H5S_get_simple_extent_type(const H5S_t *ds);
H5_DLL hssize_t H5S_get_simple_extent_npoints(const H5S_t *ds);
H5_DLL hsize_t H5S_get_npoints_max(const H5S_t *ds);
H5_DLL int H5S_get_simple_extent_ndims(const H5S_t *ds);
H5_DLL int H5S_get_simple_extent_dims(const H5S_t *ds, hsize_t dims[]/*out*/,
					hsize_t max_dims[]/*out*/);
H5_DLL herr_t H5S_modify(struct H5G_entry_t *ent, const H5S_t *space,
        hbool_t update_time, hid_t dxpl_id);
H5_DLL herr_t H5S_append(H5F_t *f, hid_t dxpl_id, struct H5O_t *oh, const H5S_t *ds);
H5_DLL H5S_t *H5S_read(struct H5G_entry_t *ent, hid_t dxpl_id);
H5_DLL int H5S_extend(H5S_t *space, const hsize_t *size);
H5_DLL int H5S_set_extent(H5S_t *space, const hsize_t *size);
H5_DLL herr_t H5S_set_extent_real(H5S_t *space, const hsize_t *size);
H5_DLL H5S_t *H5S_create_simple(unsigned rank, const hsize_t dims[/*rank*/],
		  const hsize_t maxdims[/*rank*/]);
H5_DLL herr_t H5S_debug(H5F_t *f, hid_t dxpl_id, const void *_mesg, FILE *stream,
			 int indent, int fwidth);

/* Operations on selections */
H5_DLL herr_t H5S_select_deserialize(H5S_t *space, const uint8_t *buf);
H5_DLL H5S_sel_type H5S_get_select_type(const H5S_t *space);
H5_DLL herr_t H5S_select_iterate(void *buf, hid_t type_id, const H5S_t *space,
				H5D_operator_t op, void *operator_data);
H5_DLL herr_t H5S_select_fill(void *fill, size_t fill_size,
                                const H5S_t *space, void *buf);
H5_DLL herr_t H5S_select_fscat (H5F_t *f, struct H5O_layout_t *layout,
        H5P_genplist_t *dc_plist, const union H5D_storage_t *store, size_t elmt_size,
        const H5S_t *file_space, H5S_sel_iter_t *file_iter, hsize_t nelmts,
        hid_t dxpl_id, const void *_buf);
H5_DLL hsize_t H5S_select_fgath (H5F_t *f, const struct H5O_layout_t *layout,
        H5P_genplist_t *dc_plist, const union H5D_storage_t *store, size_t elmt_size,
        const H5S_t *file_space, H5S_sel_iter_t *file_iter, hsize_t nelmts,
        hid_t dxpl_id, void *buf);
H5_DLL herr_t H5S_select_mscat (const void *_tscat_buf, size_t elmt_size,
        const H5S_t *space, H5S_sel_iter_t *iter, hsize_t nelmts,
        hid_t dxpl_id, void *_buf/*out*/);
H5_DLL hsize_t H5S_select_mgath (const void *_buf, size_t elmt_size,
        const H5S_t *space, H5S_sel_iter_t *iter, hsize_t nelmts,
        hid_t dxpl_id, void *_tgath_buf/*out*/);
H5_DLL herr_t H5S_select_read(H5F_t *f, const struct H5O_layout_t *layout,
        H5P_genplist_t *dc_plist, const union H5D_storage_t *store, size_t elmt_size,
        const H5S_t *file_space, const H5S_t *mem_space, hid_t dxpl_id,
        void *buf/*out*/);
H5_DLL herr_t H5S_select_write(H5F_t *f, struct H5O_layout_t *layout,
        H5P_genplist_t *dc_plist, const union H5D_storage_t *store, size_t elmt_size,
        const H5S_t *file_space, const H5S_t *mem_space, hid_t dxpl_id,
        const void *buf/*out*/);
H5_DLL htri_t H5S_select_valid(const H5S_t *space);
H5_DLL hssize_t H5S_get_select_npoints(const H5S_t *space);
H5_DLL herr_t H5S_get_select_bounds(const H5S_t *space, hssize_t *start, hssize_t *end);
H5_DLL herr_t H5S_select_offset(H5S_t *space, const hssize_t *offset);
H5_DLL herr_t H5S_select_copy(H5S_t *dst, const H5S_t *src);
H5_DLL htri_t H5S_select_shape_same(const H5S_t *space1, const H5S_t *space2);
H5_DLL herr_t H5S_select_release(H5S_t *ds);

/* Operations on all selections */
H5_DLL herr_t H5S_select_all(H5S_t *space, unsigned rel_prev);

/* Operations on none selections */
H5_DLL herr_t H5S_select_none(H5S_t *space);

/* Operations on point selections */
H5_DLL herr_t H5S_select_elements (H5S_t *space, H5S_seloper_t op,
    size_t num_elem, const hssize_t **coord);

/* Operations on hyperslab selections */
H5_DLL herr_t H5S_select_hyperslab (H5S_t *space, H5S_seloper_t op, const hssize_t start[],
    const hsize_t *stride, const hsize_t count[], const hsize_t *block);
H5_DLL herr_t H5S_get_select_hyper_blocklist(H5S_t *space, hbool_t internal,
    hsize_t startblock, hsize_t numblocks, hsize_t *buf);
H5_DLL herr_t H5S_hyper_add_span_element(H5S_t *space, unsigned rank,
    hssize_t *coords);
H5_DLL herr_t H5S_hyper_reset_scratch(H5S_t *space);
H5_DLL herr_t H5S_hyper_convert(H5S_t *space);
H5_DLL htri_t H5S_hyper_intersect (H5S_t *space1, H5S_t *space2);
H5_DLL herr_t H5S_hyper_adjust(H5S_t *space, const hssize_t *offset);
H5_DLL herr_t H5S_hyper_move(H5S_t *space, const hssize_t *offset);
H5_DLL herr_t H5S_hyper_normalize_offset(H5S_t *space);

/* Operations on selection iterators */
H5_DLL herr_t H5S_select_iter_init(H5S_sel_iter_t *iter, const H5S_t *space, size_t elmt_size);
H5_DLL herr_t H5S_select_iter_coords(const H5S_sel_iter_t *sel_iter, hssize_t *coords);
H5_DLL hsize_t H5S_select_iter_nelmts(const H5S_sel_iter_t *sel_iter);
H5_DLL herr_t H5S_select_iter_next(H5S_sel_iter_t *sel_iter, size_t nelem);
H5_DLL herr_t H5S_select_iter_release(H5S_sel_iter_t *sel_iter);

#ifdef H5_HAVE_PARALLEL
#ifndef _H5S_IN_H5S_C
/* Global vars whose value comes from environment variable */
/* (Defined in H5S.c) */
H5_DLLVAR hbool_t		H5S_mpi_opt_types_g;
#endif /* _H5S_IN_H5S_C */
#endif /* H5_HAVE_PARALLEL */

#endif /* _H5Sprivate_H */
