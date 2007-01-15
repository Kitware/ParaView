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
 * Programmer: Quincey Koziol <koziol@ncsa.uiuc.edu>
 *         Thursday, September 30, 2004
 */

#define H5D_PACKAGE    /*suppress error about including H5Dpkg    */

#include "H5private.h"    /* Generic Functions      */
#include "H5Dpkg.h"    /* Datasets        */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5Fprivate.h"    /* Files        */

/* PRIVATE PROTOTYPES */
static herr_t H5D_efl_read (const H5O_efl_t *efl, haddr_t addr, size_t size,
    uint8_t *buf);
static herr_t H5D_efl_write(const H5O_efl_t *efl, haddr_t addr, size_t size,
    const uint8_t *buf);


/*-------------------------------------------------------------------------
 * Function:  H5D_efl_read
 *
 * Purpose:  Reads data from an external file list.  It is an error to
 *    read past the logical end of file, but reading past the end
 *    of any particular member of the external file list results in
 *    zeros.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, March  4, 1998
 *
 * Modifications:
 *    Robb Matzke, 1999-07-28
 *    The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_efl_read (const H5O_efl_t *efl, haddr_t addr, size_t size, uint8_t *buf)
{
    int    fd=-1;
    size_t  to_read;
#ifndef NDEBUG
    hsize_t     tempto_read;
#endif /* NDEBUG */
    hsize_t     skip=0;
    haddr_t     cur;
    ssize_t  n;
    size_t      u;                      /* Local index variable */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_efl_read)

    /* Check args */
    assert (efl && efl->nused>0);
    assert (H5F_addr_defined (addr));
    assert (size < SIZET_MAX);
    assert (buf || 0==size);

    /* Find the first efl member from which to read */
    for (u=0, cur=0; u<efl->nused; u++) {
  if (H5O_EFL_UNLIMITED==efl->slot[u].size || addr < cur+efl->slot[u].size) {
      skip = addr - cur;
      break;
  }
    cur += efl->slot[u].size;
    }

    /* Read the data */
    while (size) {
        assert(buf);
  if (u>=efl->nused)
      HGOTO_ERROR (H5E_EFL, H5E_OVERFLOW, FAIL, "read past logical end of file")
  if (H5F_OVERFLOW_HSIZET2OFFT (efl->slot[u].offset+skip))
      HGOTO_ERROR (H5E_EFL, H5E_OVERFLOW, FAIL, "external file address overflowed")
  if ((fd=HDopen (efl->slot[u].name, O_RDONLY, 0))<0)
      HGOTO_ERROR (H5E_EFL, H5E_CANTOPENFILE, FAIL, "unable to open external raw data file")
  if (HDlseek (fd, (off_t)(efl->slot[u].offset+skip), SEEK_SET)<0)
      HGOTO_ERROR (H5E_EFL, H5E_SEEKERROR, FAIL, "unable to seek in external raw data file")
#ifndef NDEBUG
  tempto_read = MIN(efl->slot[u].size-skip,(hsize_t)size);
        H5_CHECK_OVERFLOW(tempto_read,hsize_t,size_t);
  to_read = (size_t)tempto_read;
#else /* NDEBUG */
  to_read = MIN((size_t)(efl->slot[u].size-skip), size);
#endif /* NDEBUG */
  if ((n=HDread (fd, buf, to_read))<0) {
      HGOTO_ERROR (H5E_EFL, H5E_READERROR, FAIL, "read error in external raw data file")
  } else if ((size_t)n<to_read) {
      HDmemset (buf+n, 0, to_read-n);
  }
  HDclose (fd);
  fd = -1;
  size -= to_read;
  buf += to_read;
  skip = 0;
  u++;
    }

done:
    if (fd>=0)
        HDclose (fd);

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_efl_write
 *
 * Purpose:  Writes data to an external file list.  It is an error to
 *    write past the logical end of file, but writing past the end
 *    of any particular member of the external file list just
 *    extends that file.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, March  4, 1998
 *
 * Modifications:
 *    Robb Matzke, 1999-07-28
 *    The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
static herr_t
H5D_efl_write (const H5O_efl_t *efl, haddr_t addr, size_t size, const uint8_t *buf)
{
    int    fd=-1;
    size_t  to_write;
#ifndef NDEBUG
    hsize_t  tempto_write;
#endif /* NDEBUG */
    haddr_t     cur;
    hsize_t     skip=0;
    size_t  u;                      /* Local index variable */
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5D_efl_write)

    /* Check args */
    assert (efl && efl->nused>0);
    assert (H5F_addr_defined (addr));
    assert (size < SIZET_MAX);
    assert (buf || 0==size);

    /* Find the first efl member in which to write */
    for (u=0, cur=0; u<efl->nused; u++) {
  if (H5O_EFL_UNLIMITED==efl->slot[u].size || addr < cur+efl->slot[u].size) {
      skip = addr - cur;
      break;
  }
  cur += efl->slot[u].size;
    }

    /* Write the data */
    while (size) {
        assert(buf);
  if (u>=efl->nused)
      HGOTO_ERROR (H5E_EFL, H5E_OVERFLOW, FAIL, "write past logical end of file")
  if (H5F_OVERFLOW_HSIZET2OFFT (efl->slot[u].offset+skip))
      HGOTO_ERROR (H5E_EFL, H5E_OVERFLOW, FAIL, "external file address overflowed")
  if ((fd=HDopen (efl->slot[u].name, O_CREAT|O_RDWR, 0666))<0) {
      if (HDaccess (efl->slot[u].name, F_OK)<0) {
    HGOTO_ERROR (H5E_EFL, H5E_CANTOPENFILE, FAIL, "external raw data file does not exist")
      } else {
    HGOTO_ERROR (H5E_EFL, H5E_CANTOPENFILE, FAIL, "unable to open external raw data file")
      }
  }
  if (HDlseek (fd, (off_t)(efl->slot[u].offset+skip), SEEK_SET)<0)
      HGOTO_ERROR (H5E_EFL, H5E_SEEKERROR, FAIL, "unable to seek in external raw data file")
#ifndef NDEBUG
  tempto_write = MIN(efl->slot[u].size-skip,(hsize_t)size);
        H5_CHECK_OVERFLOW(tempto_write,hsize_t,size_t);
        to_write = (size_t)tempto_write;
#else /* NDEBUG */
  to_write = MIN((size_t)(efl->slot[u].size-skip), size);
#endif /* NDEBUG */
  if ((size_t)HDwrite (fd, buf, to_write)!=to_write)
      HGOTO_ERROR (H5E_EFL, H5E_READERROR, FAIL, "write error in external raw data file")
  HDclose (fd);
  fd = -1;
  size -= to_write;
  buf += to_write;
  skip = 0;
  u++;
    }

done:
    if (fd>=0)
        HDclose (fd);

    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:  H5D_efl_readvv
 *
 * Purpose:  Reads data from an external file list.  It is an error to
 *    read past the logical end of file, but reading past the end
 *    of any particular member of the external file list results in
 *    zeros.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, May  7, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5D_efl_readvv(const H5D_io_info_t *io_info,
    size_t dset_max_nseq, size_t *dset_curr_seq, size_t dset_len_arr[], hsize_t dset_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[],
    void *_buf)
{
    const H5O_efl_t *efl=&(io_info->store->efl); /* Pointer to efl info */
    unsigned char *buf;         /* Pointer to buffer to write */
    haddr_t addr;               /* Actual address to read */
    size_t total_size=0;        /* Total size of sequence in bytes */
    size_t size;                /* Size of sequence in bytes */
    size_t u;                   /* Counting variable */
    size_t v;                   /* Counting variable */
    ssize_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5D_efl_readvv, FAIL)

    /* Check args */
    assert (efl && efl->nused>0);
    assert (_buf);

    /* Work through all the sequences */
    for(u=*dset_curr_seq, v=*mem_curr_seq; u<dset_max_nseq && v<mem_max_nseq; ) {
        /* Choose smallest buffer to write */
        if(mem_len_arr[v]<dset_len_arr[u])
            size=mem_len_arr[v];
        else
            size=dset_len_arr[u];

        /* Compute offset on disk */
        addr=dset_offset_arr[u];

        /* Compute offset in memory */
        buf = (unsigned char *)_buf + mem_offset_arr[v];

        /* Read data */
        if (H5D_efl_read(efl, addr, size, buf)<0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "block write failed")

        /* Update memory information */
        mem_len_arr[v]-=size;
        mem_offset_arr[v]+=size;
        if(mem_len_arr[v]==0)
            v++;

        /* Update file information */
        dset_len_arr[u]-=size;
        dset_offset_arr[u]+=size;
        if(dset_len_arr[u]==0)
            u++;

        /* Increment number of bytes copied */
        total_size+=size;
    } /* end for */

    /* Update current sequence vectors */
    *dset_curr_seq=u;
    *mem_curr_seq=v;

    /* Set return value */
    H5_ASSIGN_OVERFLOW(ret_value,total_size,size_t,ssize_t);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_efl_readvv() */


/*-------------------------------------------------------------------------
 * Function:  H5D_efl_writevv
 *
 * Purpose:  Writes data to an external file list.  It is an error to
 *    write past the logical end of file, but writing past the end
 *    of any particular member of the external file list just
 *    extends that file.
 *
 * Return:  Non-negative on success/Negative on failure
 *
 * Programmer:  Quincey Koziol
 *              Friday, May  2, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
ssize_t
H5D_efl_writevv(const H5D_io_info_t *io_info,
    size_t dset_max_nseq, size_t *dset_curr_seq, size_t dset_len_arr[], hsize_t dset_offset_arr[],
    size_t mem_max_nseq, size_t *mem_curr_seq, size_t mem_len_arr[], hsize_t mem_offset_arr[],
    const void *_buf)
{
    const H5O_efl_t *efl=&(io_info->store->efl); /* Pointer to efl info */
    const unsigned char *buf;   /* Pointer to buffer to write */
    haddr_t addr;               /* Actual address to read */
    size_t total_size=0;        /* Total size of sequence in bytes */
    size_t size;                /* Size of sequence in bytes */
    size_t u;                   /* Counting variable */
    size_t v;                   /* Counting variable */
    ssize_t ret_value;          /* Return value */

    FUNC_ENTER_NOAPI(H5D_efl_writevv, FAIL)

    /* Check args */
    assert (efl && efl->nused>0);
    assert (_buf);

    /* Work through all the sequences */
    for(u=*dset_curr_seq, v=*mem_curr_seq; u<dset_max_nseq && v<mem_max_nseq; ) {
        /* Choose smallest buffer to write */
        if(mem_len_arr[v]<dset_len_arr[u])
            size=mem_len_arr[v];
        else
            size=dset_len_arr[u];

        /* Compute offset on disk */
        addr=dset_offset_arr[u];

        /* Compute offset in memory */
        buf = (const unsigned char *)_buf + mem_offset_arr[v];

        /* Write data */
        if (H5D_efl_write(efl, addr, size, buf)<0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "block write failed")

        /* Update memory information */
        mem_len_arr[v]-=size;
        mem_offset_arr[v]+=size;
        if(mem_len_arr[v]==0)
            v++;

        /* Update file information */
        dset_len_arr[u]-=size;
        dset_offset_arr[u]+=size;
        if(dset_len_arr[u]==0)
            u++;

        /* Increment number of bytes copied */
        total_size+=size;
    } /* end for */

    /* Update current sequence vectors */
    *dset_curr_seq=u;
    *mem_curr_seq=v;

    /* Set return value */
    H5_ASSIGN_OVERFLOW(ret_value,total_size,size_t,ssize_t);

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5D_efl_writevv() */

