/*
 * Copyright (C) 1997 NCSA
 *                    All rights reserved.
 *
 * Programmer: Robb Matzke <matzke@llnl.gov>
 *             Friday, October 10, 1997
 */
#ifndef H5Vprivate_H
#define H5Vprivate_H

#include "H5private.h"

/* Vector comparison functions like Fortran66 comparison operators */
#define H5V_vector_eq_s(N,V1,V2) (H5V_vector_cmp_s (N, V1, V2)==0)
#define H5V_vector_lt_s(N,V1,V2) (H5V_vector_cmp_s (N, V1, V2)<0)
#define H5V_vector_gt_s(N,V1,V2) (H5V_vector_cmp_s (N, V1, V2)>0)
#define H5V_vector_le_s(N,V1,V2) (H5V_vector_cmp_s (N, V1, V2)<=0)
#define H5V_vector_ge_s(N,V1,V2) (H5V_vector_cmp_s (N, V1, V2)>=0)
#define H5V_vector_eq_u(N,V1,V2) (H5V_vector_cmp_u (N, V1, V2)==0)
#define H5V_vector_lt_u(N,V1,V2) (H5V_vector_cmp_u (N, V1, V2)<0)
#define H5V_vector_gt_u(N,V1,V2) (H5V_vector_cmp_u (N, V1, V2)>0)
#define H5V_vector_le_u(N,V1,V2) (H5V_vector_cmp_u (N, V1, V2)<=0)
#define H5V_vector_ge_u(N,V1,V2) (H5V_vector_cmp_u (N, V1, V2)>=0)

/* Other functions */
#define H5V_vector_cpy(N,DST,SRC) {                                           \
    assert (sizeof(*(DST))==sizeof(*(SRC)));                                              \
    if (SRC) HDmemcpy (DST, SRC, (N)*sizeof(*(DST)));                         \
    else HDmemset (DST, 0, (N)*sizeof(*(DST)));                               \
}

#define H5V_vector_zero(N,DST) HDmemset(DST,0,(N)*sizeof(*(DST)))

/* A null pointer is equivalent to a zero vector */
#define H5V_ZERO        NULL

__DLL__ hsize_t H5V_hyper_stride(unsigned n, const hsize_t *size,
                                 const hsize_t *total_size,
                                 const hssize_t *offset,
                                 hssize_t *stride);
__DLL__ htri_t H5V_hyper_disjointp(unsigned n, const hssize_t *offset1,
                                   const hsize_t *size1,
                                   const hssize_t *offset2,
                                   const hsize_t *size2);
__DLL__ htri_t H5V_hyper_eq(int n, const hssize_t *offset1,
                            const hsize_t *size1, const hssize_t *offset2,
                            const hsize_t *size2);
__DLL__ herr_t H5V_hyper_fill(unsigned n, const hsize_t *_size,
                              const hsize_t *total_size,
                              const hssize_t *offset, void *_dst,
                              unsigned fill_value);
__DLL__ herr_t H5V_hyper_copy(unsigned n, const hsize_t *size,
                              const hsize_t *dst_total_size,
                              const hssize_t *dst_offset, void *_dst,
                              const hsize_t *src_total_size,
                              const hssize_t *src_offset, const void *_src);
__DLL__ herr_t H5V_stride_fill(unsigned n, hsize_t elmt_size, const hsize_t *size,
                               const hssize_t *stride, void *_dst,
                               unsigned fill_value);
__DLL__ herr_t H5V_stride_copy(unsigned n, hsize_t elmt_size, const hsize_t *_size,
                               const hssize_t *dst_stride, void *_dst,
                               const hssize_t *src_stride, const void *_src);
__DLL__ herr_t H5V_stride_copy2(hsize_t nelmts, hsize_t elmt_size, int dst_n,
                                const hsize_t *dst_size,
                                const hssize_t *dst_stride, void *_dst,
                                int src_n, const hsize_t *src_size,
                                const hssize_t *src_stride, const void *_src);
__DLL__ herr_t H5V_stride_optimize1(unsigned *np, hsize_t *elmt_size,
                                    hsize_t *size, hssize_t *stride1);
__DLL__ herr_t H5V_stride_optimize2(unsigned *np, hsize_t *elmt_size,
                                    hsize_t *size, hssize_t *stride1,
                                    hssize_t *stride2);
__DLL__ herr_t H5V_array_fill(void *_dst, const void *src, size_t size,
                              size_t count);
__DLL__ hsize_t H5V_array_offset(unsigned n, const hsize_t *total_size,
                                 const hssize_t *offset);


/*-------------------------------------------------------------------------
 * Function:    H5V_vector_reduce_product
 *
 * Purpose:     Product reduction of a vector.  Vector elements and return
 *              value are size_t because we usually want the number of
 *              elements in an array and array dimensions are always of type
 *              size_t.
 *
 * Return:      Success:        Product of elements
 *
 *              Failure:        1 if N is zero
 *
 * Programmer:  Robb Matzke
 *              Friday, October 10, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
extern H5_inline hsize_t UNUSED
H5V_vector_reduce_product(unsigned n, const hsize_t *v);

/*-------------------------------------------------------------------------
 * Function:    H5V_vector_zerop_u
 *
 * Purpose:     Determines if all elements of a vector are zero.
 *
 * Return:      Success:        TRUE if all elements are zero,
 *                              FALSE otherwise
 *
 *              Failure:        TRUE if N is zero
 *
 * Programmer:  Robb Matzke
 *              Friday, October 10, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
extern H5_inline htri_t UNUSED
H5V_vector_zerop_u(int n, const hsize_t *v);

/*-------------------------------------------------------------------------
 * Function:    H5V_vector_zerop_s
 *
 * Purpose:     Determines if all elements of a vector are zero.
 *
 * Return:      Success:        TRUE if all elements are zero,
 *                              FALSE otherwise
 *
 *              Failure:        TRUE if N is zero
 *
 * Programmer:  Robb Matzke
 *              Friday, October 10, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
extern H5_inline htri_t UNUSED
H5V_vector_zerop_s(int n, const hssize_t *v);

/*-------------------------------------------------------------------------
 * Function:    H5V_vector_cmp_u
 *
 * Purpose:     Compares two vectors of the same size and determines if V1 is
 *              lexicographically less than, equal, or greater than V2.
 *
 * Return:      Success:        -1 if V1 is less than V2
 *                              0 if they are equal
 *                              1 if V1 is greater than V2
 *
 *              Failure:        0 if N is zero
 *
 * Programmer:  Robb Matzke
 *              Friday, October 10, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
extern H5_inline int UNUSED
H5V_vector_cmp_u (int n, const hsize_t *v1, const hsize_t *v2);


/*-------------------------------------------------------------------------
 * Function:    H5V_vector_cmp_s
 *
 * Purpose:     Compares two vectors of the same size and determines if V1 is
 *              lexicographically less than, equal, or greater than V2.
 *
 * Return:      Success:        -1 if V1 is less than V2
 *                              0 if they are equal
 *                              1 if V1 is greater than V2
 *
 *              Failure:        0 if N is zero
 *
 * Programmer:  Robb Matzke
 *              Wednesday, April  8, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
extern H5_inline int UNUSED
H5V_vector_cmp_s (unsigned n, const hssize_t *v1, const hssize_t *v2);


/*-------------------------------------------------------------------------
 * Function:    H5V_vector_inc
 *
 * Purpose:     Increments V1 by V2
 *
 * Return:      void
 *
 * Programmer:  Robb Matzke
 *              Monday, October 13, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
extern H5_inline void UNUSED
H5V_vector_inc(int n, hsize_t *v1, const hsize_t *v2);

#endif
