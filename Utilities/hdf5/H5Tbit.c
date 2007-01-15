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
 * Module Info:  Operations on bit vectors.  A bit vector is an array of bytes
 *    with the least-significant bits in the first byte.  That is,
 *    the bytes are in little-endian order.
 */

#define H5T_PACKAGE    /*suppress error about including H5Tpkg    */


#include "H5private.h"    /*generic functions        */
#include "H5Eprivate.h"    /*error handling        */
#include "H5Iprivate.h"    /*ID functions             */
#include "H5Tpkg.h"    /*data-type functions        */


/*-------------------------------------------------------------------------
 * Function:  H5T_bit_copy
 *
 * Purpose:  Copies bits from one vector to another.
 *
 * Return:  void
 *
 * Programmer:  Robb Matzke
 *              Wednesday, June 10, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5T_bit_copy (uint8_t *dst, size_t dst_offset, const uint8_t *src,
        size_t src_offset, size_t size)
{
    int  shift;
    unsigned  mask_lo, mask_hi;
    int  s_idx, d_idx;

    /*
     * Normalize the offset to be a byte number and a bit offset within that
     * byte.
     */
    s_idx = (int)src_offset / 8;
    d_idx = (int)dst_offset / 8;
    src_offset %= 8;
    dst_offset %= 8;

    /*
     * Get things rolling. This means copying bits until we're aligned on a
     * source byte.  This the following example, five bits are copied to the
     * destination.
     *
     *                      src[s_idx]
     *   +---------------+---------------+
     *   |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
     *   +---------------+---------------+
     *      ... : : : : : | | | | |
     *      ... v v v v v V V V V V
     *      ...+---------------+---------------+
     *      ...|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
     *      ...+---------------+---------------+
     *           dst[d_idx+1]      dst[d_idx]
     */
    while (src_offset && size>0) {
  unsigned nbits = (unsigned)MIN3 (size, 8-dst_offset, 8-src_offset);
  unsigned mask = (1<<nbits) - 1;

  dst[d_idx] &= ~(mask<<dst_offset);
  dst[d_idx] |= ((src[s_idx]>>src_offset)&mask) << dst_offset;

  src_offset += nbits;
  if (src_offset>=8) {
      s_idx++;
      src_offset %= 8;
  }
  dst_offset += nbits;
  if (dst_offset>=8) {
      d_idx++;
      dst_offset %= 8;
  }
  size -= nbits;
    }

    /*
     * The middle bits. We are aligned on a source byte which needs to be
     * copied to two (or one in the degenerate case) destination bytes.
     *
     *           src[s_idx]
     *        +---------------+
     *       |7 6 5 4 3 2 1 0|
     *        +---------------+
     *              | | | | | | | |
     *         V V V V V V V V
     *   +---------------+---------------+
     *   |7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|
     *   +---------------+---------------+
     *     dst[d_idx+1]      dst[d_idx]
     *
     *
     * Calculate shifts and masks.  See diagrams below.  MASK_LO in this
     * example is 0x1f (the low five bits) and MASK_HI is 0xe0 (the high three
     * bits). SHIFT is three since the source must be shifted right three bits
     * to line up with the destination.
     */
    shift = (int)dst_offset;
    mask_lo = (1<<(8-shift))-1;
    mask_hi = (~mask_lo) & 0xff;

    for (/*void*/; size>8; size-=8, d_idx++, s_idx++) {
  if (shift) {
      dst[d_idx+0] &= ~(mask_lo<<shift);
      dst[d_idx+0] |= (src[s_idx] & mask_lo) << shift;
      dst[d_idx+1] &= ~(mask_hi>>(8-shift));
      dst[d_idx+1] |= (src[s_idx] & mask_hi) >> (8-shift);
  } else {
      dst[d_idx] = src[s_idx];
  }
    }

    /* Finish up */
    while (size>0) {
  unsigned nbits = (unsigned)MIN3 (size, 8-dst_offset, 8-src_offset);
  unsigned mask = (1<<nbits) - 1;

  dst[d_idx] &= ~(mask<<dst_offset);
  dst[d_idx] |= ((src[s_idx]>>src_offset)&mask) << dst_offset;

  src_offset += nbits;
  if (src_offset>=8) {
      s_idx++;
      src_offset %= 8;
  }
  dst_offset += nbits;
  if (dst_offset>=8) {
      d_idx++;
      dst_offset %= 8;
  }
  size -= nbits;
    }
}


/*-------------------------------------------------------------------------
 * Function:  H5T_bit_get_d
 *
 * Purpose:  Return a small bit sequence as a number.  Bit vector starts
 *              at OFFSET and is SIZE bits long.
 *
 * Return:  Success:  The bit sequence interpretted as an unsigned
 *        integer.
 *
 *    Failure:  0
 *
 * Programmer:  Robb Matzke
 *              Tuesday, June 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hsize_t
H5T_bit_get_d (uint8_t *buf, size_t offset, size_t size)
{
    hsize_t  val=0;
    size_t  i, hs;
    hsize_t  ret_value;      /* Return value */

    FUNC_ENTER_NOAPI_NOFUNC(H5T_bit_get_d);

    assert (8*sizeof(val)>=size);

    H5T_bit_copy ((uint8_t*)&val, 0, buf, offset, size);
    switch (H5T_native_order_g) {
        case H5T_ORDER_LE:
            break;

        case H5T_ORDER_BE:
            for (i=0, hs=sizeof(val)/2; i<hs; i++) {
                uint8_t tmp = ((uint8_t*)&val)[i];
                ((uint8_t*)&val)[i] = ((uint8_t*)&val)[sizeof(val)-(i+1)];
                ((uint8_t*)&val)[sizeof(val)-(i+1)] = tmp;
            }
            break;

        default:
            HDabort ();
    }

    /* Set return value */
    ret_value=val;

    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5T_bit_set_d
 *
 * Purpose:  Sets part of a bit vector to the specified unsigned value.
 *
 * Return:  void
 *
 * Programmer:  Robb Matzke
 *              Wednesday, June 24, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5T_bit_set_d (uint8_t *buf, size_t offset, size_t size, hsize_t val)
{
    size_t  i, hs;

    assert (8*sizeof(val)>=size);

    switch (H5T_native_order_g) {
        case H5T_ORDER_LE:
            break;

        case H5T_ORDER_BE:
            for (i=0, hs=sizeof(val)/2; i<hs; i++) {
                uint8_t tmp = ((uint8_t*)&val)[i];
                ((uint8_t*)&val)[i] = ((uint8_t*)&val)[sizeof(val)-(i+1)];
                ((uint8_t*)&val)[sizeof(val)-(i+1)] = tmp;
            }
            break;

        default:
            HDabort ();
    }

    H5T_bit_copy (buf, offset, (uint8_t*)&val, 0, size);
}


/*-------------------------------------------------------------------------
 * Function:  H5T_bit_set
 *
 * Purpose:  Sets or clears bits in a contiguous region of a vector
 *    beginning at bit OFFSET and continuing for SIZE bits.
 *
 * Return:  void
 *
 * Programmer:  Robb Matzke
 *              Wednesday, June 10, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void
H5T_bit_set (uint8_t *buf, size_t offset, size_t size, hbool_t value)
{
    int  idx;

    /* Normalize */
    idx = (int)offset / 8;
    offset %= 8;

    /* The first partial byte */
    if (size && offset%8) {
  size_t nbits = MIN (size, 8-offset);
  unsigned mask = (1<<nbits)-1;
  if (value) {
      buf[idx++] |= mask << offset;
  } else {
      buf[idx++] &= ~(mask << offset);
  }
  size -= nbits;
    }

    /* The middle bytes */
    while (size>=8) {
  buf[idx++] = value ? 0xff : 0x00;
  size -= 8;
    }

    /* The last partial byte */
    if (size) {
  if (value) {
      buf[idx] |= (1<<size)-1;
  } else {
      buf[idx] &= ~((1<<size)-1);
  }
    }
}


/*-------------------------------------------------------------------------
 * Function:  H5T_bit_find
 *
 * Purpose:  Finds the first bit with the specified VALUE within a region
 *    of a bit vector.  The region begins at OFFSET and continues
 *    for SIZE bits, but the region can be searched from the least
 *    significat end toward the most significant end(H5T_BIT_LSB
 *    as DIRECTION), or from the most significant end to the least
 *    significant end(H5T_BIT_MSB as DIRECTION).
 *
 * Return:  Success:  The position of the bit found, relative to
 *        the offset.
 *
 *    Failure:  -1
 *
 * Programmer:  Robb Matzke
 *              Wednesday, June 10, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5T_bit_find (uint8_t *buf, size_t offset, size_t size, H5T_sdir_t direction,
        hbool_t value)
{
    ssize_t  base=(ssize_t)offset;
    ssize_t  idx, i;
    size_t  iu;
    ssize_t     ret_value=(-1);         /* Return value */

    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_bit_find);

    /* Some functions call this with value=TRUE */
    assert (TRUE==1);

    switch (direction) {
    case H5T_BIT_LSB:
  /* Calculate index */
  idx = (ssize_t)(offset / 8);
  offset %= 8;

  /* Beginning */
  if (offset) {
      for (iu=offset; iu<8 && size>0; iu++, size--) {
    if (value==(hbool_t)((buf[idx]>>iu) & 0x01))
        HGOTO_DONE(8*idx+(ssize_t)iu - base);
      }
      offset = 0;
      idx++;
  }
  /* Middle */
  while (size>=8) {
      if ((value?0x00:0xff)!=buf[idx]) {
    for (i=0; i<8; i++) {
        if (value==(hbool_t)((buf[idx]>>i) & 0x01))
      HGOTO_DONE(8*idx+i - base);
    }
      }
      size -= 8;
      idx++;
  }
  /* End */
  for (i=0; i<(ssize_t)size; i++) {
      if (value==(hbool_t)((buf[idx]>>i) & 0x01))
    HGOTO_DONE(8*idx+i - base);
  }
  break;

    case H5T_BIT_MSB:
  /* Calculate index */
  idx = (ssize_t)((offset+size-1) / 8);
  offset %= 8;

  /* Beginning */
  if (size>8-offset && (offset+size)%8) {
      for (iu=(offset+size)%8; iu>0; --iu, --size) {
    if (value==(hbool_t)((buf[idx]>>(iu-1)) & 0x01))
        HGOTO_DONE(8*idx+(ssize_t)(iu-1) - base);
      }
      --idx;
  }
  /* Middle */
  while (size>=8) {
      if ((value?0x00:0xff)!=buf[idx]) {
    for (i=7; i>=0; --i) {
        if (value==(hbool_t)((buf[idx]>>i) & 0x01))
      HGOTO_DONE(8*idx+i - base);
    }
      }
      size -= 8;
      --idx;
  }
  /* End */
  if (size>0) {
      for (iu=offset+size; iu>offset; --iu) {
    if (value==(hbool_t)((buf[idx]>>(iu-1)) & 0x01))
        HGOTO_DONE(8*idx+(ssize_t)(iu-1) - base);
      }
  }
  break;
    }

done:
    FUNC_LEAVE_NOAPI(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:  H5T_bit_inc
 *
 * Purpose:  Increment part of a bit field by adding 1.  The bit field
 *              starts with bit position START and is SIZE bits long.
 *
 * Return:  Success:        The carry-out value.  One if overflows,
 *                              zero otherwise.
 *
 *    Failure:  Negative
 *
 * Programmer:  Robb Matzke
 *              Friday, June 26, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5T_bit_inc(uint8_t *buf, size_t start, size_t size)
{
    size_t  idx = start / 8;
    unsigned  carry = 1;
    unsigned  acc, mask;

    /* Use FUNC_ENTER_NOAPI_NOINIT_NOFUNC here to avoid performance issues */
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5T_bit_inc);

    assert(buf);
    start %= 8;

    /* The first partial byte */
    if (start) {
  if (size+start<8) mask = (1<<size)-1;
  else mask = (1<<(8-start))-1;
  acc = (buf[idx]>>start) & mask;
  acc += 1;
  carry = acc & (1<<MIN(size, 8-start));
  buf[idx] &= ~(mask<<start);
  buf[idx] |= (acc & mask) << start;
  size -= MIN(size, 8-start);
  start=0;
  idx++;
    }

    /* The middle */
    while (carry && size>=8) {
  acc = buf[idx];
  acc += 1;
  carry = acc & 0x100;
  buf[idx] = acc & 0xff;
  idx++;
  size -= 8;
    }

    /* The last bits */
    if (carry && size>0) {
  mask = (1<<size)-1;
  acc = buf[idx] & mask;
  acc += 1;
  carry = acc & (1<<size);
  buf[idx] &= ~mask;
  buf[idx] |= acc & mask;
    }

    FUNC_LEAVE_NOAPI(carry ? TRUE : FALSE);
}
