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

#define H5F_PACKAGE    /*suppress error about including H5Fpkg    */

/* Interface initialization */
#define H5_INTERFACE_INIT_FUNC  H5F_init_super_interface


/* Packages needed by this file... */
#include "H5private.h"    /* Generic Functions      */
#include "H5Eprivate.h"    /* Error handling        */
#include "H5Fpkg.h"             /* File access        */
#include "H5FDprivate.h"  /* File drivers        */
#include "H5Iprivate.h"    /* IDs            */
#include "H5Pprivate.h"    /* Property lists      */

/* PRIVATE PROTOTYPES */


/*--------------------------------------------------------------------------
NAME
   H5F_init_super_interface -- Initialize interface-specific information
USAGE
    herr_t H5T_init_super_interface()

RETURNS
    Non-negative on success/Negative on failure
DESCRIPTION
    Initializes any interface-specific data or routines.  (Just calls
    H5F_init_iterface currently).

--------------------------------------------------------------------------*/
static herr_t
H5F_init_super_interface(void)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5F_init_super_interface)

    FUNC_LEAVE_NOAPI(H5F_init())
} /* H5F_init_super_interface() */


/*-------------------------------------------------------------------------
 * Function:    H5F_read_superblock
 *
 * Purpose:     Reads the superblock from the file or from the BUF. If
 *              ADDR is a valid address, then it reads it from the file.
 *              If not, then BUF must be non-NULL for it to read from the
 *              BUF.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  Bill Wendling
 *              wendling@ncsa.uiuc.edu
 *              Sept 12, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_read_superblock(H5F_t *f, hid_t dxpl_id, H5G_entry_t *root_ent)
{
    haddr_t             stored_eoa;         /*relative end-of-addr in file  */
    haddr_t             eof;                /*end of file address           */
    uint8_t            *q;                  /*ptr into temp I/O buffer      */
    size_t              sizeof_addr = 0;
    size_t              sizeof_size = 0;
    const size_t        fixed_size = 24;    /*fixed sizeof superblock       */
    unsigned            sym_leaf_k = 0;
    size_t              variable_size;      /*variable sizeof superblock    */
    unsigned            btree_k[H5B_NUM_BTREE_ID];  /* B-tree internal node 'K' values */
    H5F_file_t         *shared = NULL;      /* shared part of `file'        */
    H5FD_t             *lf = NULL;          /* file driver part of `shared' */
    uint8_t            *p;                  /* Temporary pointer into encoding buffers */
    unsigned            i;                  /* Index variable               */
    unsigned            chksum;             /* Checksum temporary variable  */
    size_t              driver_size;        /* Size of driver info block, in bytes */
    char                driver_name[9];     /* Name of driver, for driver info block */
    unsigned            super_vers;         /* Super block version          */
    unsigned            freespace_vers;     /* Freespace info version       */
    unsigned            obj_dir_vers;       /* Object header info version   */
    unsigned            share_head_vers;    /* Shared header info version   */
    uint8_t             buf[H5F_SUPERBLOCK_SIZE];     /* Local buffer                 */
    H5P_genplist_t     *c_plist;            /* File creation property list  */
    herr_t              ret_value = SUCCEED;

    /* Decoding */
    FUNC_ENTER_NOAPI(H5F_read_superblock, FAIL)

    /* Short cuts */
    shared = f->shared;
    lf = shared->lf;

    /* Get the shared file creation property list */
    if (NULL == (c_plist = H5I_object(shared->fcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "can't get property list")

    /* Read the superblock if it hasn't been read before. */
    if (HADDR_UNDEF == (shared->super_addr=H5F_locate_signature(lf,dxpl_id)))
        HGOTO_ERROR(H5E_FILE, H5E_NOTHDF5, FAIL, "unable to find file signature")
    if (H5FD_set_eoa(lf, shared->super_addr + fixed_size) < 0 ||
            H5FD_read(lf, H5FD_MEM_SUPER, dxpl_id, shared->super_addr, fixed_size, buf) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_READERROR, FAIL, "unable to read superblock")

    /* Signature, already checked */
    p = buf + H5F_SIGNATURE_LEN;

    /* Superblock version */
    super_vers = *p++;
    if (super_vers > HDF5_SUPERBLOCK_VERSION_MAX)
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "bad superblock version number")
    if (H5P_set(c_plist, H5F_CRT_SUPER_VERS_NAME, &super_vers) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set superblock version")

    /* Freespace version */
    freespace_vers = *p++;
    if (HDF5_FREESPACE_VERSION != freespace_vers)
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "bad free space version number")
    if (H5P_set(c_plist, H5F_CRT_FREESPACE_VERS_NAME, &freespace_vers) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to free space version")

    /* Root group version number */
    obj_dir_vers = *p++;
    if (HDF5_OBJECTDIR_VERSION != obj_dir_vers)
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "bad object directory version number")
    if (H5P_set(c_plist, H5F_CRT_OBJ_DIR_VERS_NAME, &obj_dir_vers) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set object directory version")

    /* Skip over reserved byte */
    p++;

    /* Shared header version number */
    share_head_vers = *p++;
    if (HDF5_SHAREDHEADER_VERSION != share_head_vers)
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "bad shared-header format version number")
    if (H5P_set(c_plist, H5F_CRT_SHARE_HEAD_VERS_NAME, &share_head_vers) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set shared-header format version")

    /* Size of file addresses */
    sizeof_addr = *p++;
    if (sizeof_addr != 2 && sizeof_addr != 4 &&
            sizeof_addr != 8 && sizeof_addr != 16 && sizeof_addr != 32)
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "bad byte number in an address")
    if (H5P_set(c_plist, H5F_CRT_ADDR_BYTE_NUM_NAME,&sizeof_addr) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set byte number in an address")
    shared->sizeof_addr = sizeof_addr;  /* Keep a local copy also */

    /* Size of file sizes */
    sizeof_size = *p++;
    if (sizeof_size != 2 && sizeof_size != 4 &&
            sizeof_size != 8 && sizeof_size != 16 && sizeof_size != 32)
        HGOTO_ERROR(H5E_FILE, H5E_BADVALUE, FAIL, "bad byte number for object size")
    if (H5P_set(c_plist, H5F_CRT_OBJ_BYTE_NUM_NAME, &sizeof_size) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set byte number for object size")
    shared->sizeof_size = sizeof_size;  /* Keep a local copy also */

    /* Skip over reserved byte */
    p++;

    /* Various B-tree sizes */
    UINT16DECODE(p, sym_leaf_k);
    if (sym_leaf_k == 0)
        HGOTO_ERROR(H5E_FILE, H5E_BADRANGE, FAIL, "bad symbol table leaf node 1/2 rank")
    if (H5P_set(c_plist, H5F_CRT_SYM_LEAF_NAME, &sym_leaf_k) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set rank for symbol table leaf nodes")
    shared->sym_leaf_k = sym_leaf_k;    /* Keep a local copy also */

    /* Need 'get' call to set other array values */
    if (H5P_get(c_plist, H5F_CRT_BTREE_RANK_NAME, btree_k) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get rank for btree internal nodes")
    UINT16DECODE(p, btree_k[H5B_SNODE_ID]);
    if (btree_k[H5B_SNODE_ID] == 0)
        HGOTO_ERROR(H5E_FILE, H5E_BADRANGE, FAIL, "bad 1/2 rank for btree internal nodes")
    /*
     * Delay setting the value in the property list until we've checked
     * for the indexed storage B-tree internal 'K' value later.
     */

    /* File consistency flags. Not really used yet */
    UINT32DECODE(p, shared->consist_flags);
    assert(((size_t)(p - buf)) == fixed_size);

    /* Decode the variable-length part of the superblock... */
    variable_size = (super_vers>0 ? 4 : 0) +    /* Potential indexed storage B-tree internal 'K' value */
                    H5F_SIZEOF_ADDR(f) +        /*base addr*/
                    H5F_SIZEOF_ADDR(f) +        /*global free list*/
                    H5F_SIZEOF_ADDR(f) +        /*end-of-address*/
                    H5F_SIZEOF_ADDR(f) +        /*reserved address*/
                    H5G_SIZEOF_ENTRY(f);        /*root group ptr*/
    assert(fixed_size + variable_size <= sizeof(buf));
    if (H5FD_set_eoa(lf, shared->super_addr + fixed_size+variable_size) < 0 ||
            H5FD_read(lf, H5FD_MEM_SUPER, dxpl_id, shared->super_addr + fixed_size, variable_size, p) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to read superblock")

    /*
     * If the superblock version # is greater than 0, read in the indexed
     * storage B-tree internal 'K' value
     */
    if (super_vers > 0) {
        UINT16DECODE(p, btree_k[H5B_ISTORE_ID]);
        p += 2;   /* reserved */
    }
    else
        btree_k[H5B_ISTORE_ID] = HDF5_BTREE_ISTORE_IK_DEF;

    /* Set the B-tree internal node values, etc */
    if (H5P_set(c_plist, H5F_CRT_BTREE_RANK_NAME, btree_k) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set rank for btree internal nodes")
    HDmemcpy(shared->btree_k, btree_k, sizeof(unsigned) * (size_t)H5B_NUM_BTREE_ID);    /* Keep a local copy also */

    H5F_addr_decode(f, (const uint8_t **)&p, &shared->base_addr/*out*/);
    H5F_addr_decode(f, (const uint8_t **)&p, &shared->freespace_addr/*out*/);
    H5F_addr_decode(f, (const uint8_t **)&p, &stored_eoa/*out*/);
    H5F_addr_decode(f, (const uint8_t **)&p, &shared->driver_addr/*out*/);
    if (H5G_ent_decode(f, (const uint8_t **)&p, root_ent/*out*/) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to read root symbol entry")

    /*
     * Check if superblock address is different from base address and
     * adjust base address and "end of address" address if so.
     */
    if (!H5F_addr_eq(shared->super_addr,shared->base_addr)) {
        /* Check if the superblock moved earlier in the file */
        if (H5F_addr_lt(shared->super_addr, shared->base_addr))
            stored_eoa -= (shared->base_addr - shared->super_addr);
        else
            /* The superblock moved later in the file */
            stored_eoa += (shared->super_addr - shared->base_addr);

        shared->base_addr = shared->super_addr;
    } /* end if */

    /* Compute super block checksum */
    assert(sizeof(chksum) == sizeof(shared->super_chksum));
    for (q = (uint8_t *)&chksum, chksum = 0, i = 0; i < fixed_size + variable_size; ++i)
        q[i % sizeof(shared->super_chksum)] ^= buf[i];

    /* Set the super block checksum */
    shared->super_chksum = chksum;

    /* Decode the optional driver information block */
    if (H5F_addr_defined(shared->driver_addr)) {
        haddr_t drv_addr = shared->base_addr + shared->driver_addr;
        uint8_t dbuf[H5F_DRVINFOBLOCK_SIZE];     /* Local buffer                 */

        if (H5FD_set_eoa(lf, drv_addr + 16) < 0 ||
                H5FD_read(lf, H5FD_MEM_SUPER, dxpl_id, drv_addr, (size_t)16, dbuf) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to read driver information block")
        p = dbuf;

        /* Version number */
        if (HDF5_DRIVERINFO_VERSION != *p++)
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "bad driver information block version number")

        p += 3; /* reserved */

        /* Driver info size */
        UINT32DECODE(p, driver_size);

        /* Driver name and/or version */
        HDstrncpy(driver_name, (const char *)p, (size_t)8);
        driver_name[8] = '\0';
        p += 8; /* advance past name/version */

        /* Read driver information and decode */
        assert((driver_size + 16) <= sizeof(dbuf));
        if (H5FD_set_eoa(lf, drv_addr + 16 + driver_size) < 0 ||
                H5FD_read(lf, H5FD_MEM_SUPER, dxpl_id, drv_addr+16, driver_size, p) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to read file driver information")

        if (H5FD_sb_decode(lf, driver_name, p) < 0)
            HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to decode driver information")

        /* Compute driver info block checksum */
        assert(sizeof(chksum) == sizeof(shared->drvr_chksum));
        for (q = (uint8_t *)&chksum, chksum = 0, i = 0; i < (driver_size + 16); ++i)
            q[i % sizeof(shared->drvr_chksum)] ^= dbuf[i];

        /* Set the driver info block checksum */
        shared->drvr_chksum = chksum;
    } /* end if */

    /*
     * The user-defined data is the area of the file before the base
     * address.
     */
    if (H5P_set(c_plist, H5F_CRT_USER_BLOCK_NAME, &shared->base_addr) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTSET, FAIL, "unable to set usr block size")

    /*
     * Make sure that the data is not truncated. One case where this is
     * possible is if the first file of a family of files was opened
     * individually.
     */
    if (HADDR_UNDEF == (eof = H5FD_get_eof(lf)))
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to determine file size")
    if (eof < stored_eoa)
        HGOTO_ERROR(H5E_FILE, H5E_TRUNCATED, FAIL, "truncated file")

    /*
     * Tell the file driver how much address space has already been
     * allocated so that it knows how to allocate additional memory.
     */
    if (H5FD_set_eoa(lf, stored_eoa) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTOPENFILE, FAIL, "unable to set end-of-address marker for file")

done:
    FUNC_LEAVE_NOAPI(ret_value)
}


/*-------------------------------------------------------------------------
 * Function:    H5F_init_superblock
 *
 * Purpose:     Allocates the superblock for the file and initializes
 *              information about the superblock in memory.  Does not write
 *              any superblock information to the file.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              Sept 15, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_init_superblock(const H5F_t *f, hid_t dxpl_id)
{
    hsize_t         userblock_size = 0;         /* Size of userblock, in bytes */
    size_t          superblock_size;            /* Size of superblock, in bytes     */
    size_t          driver_size;                /* Size of driver info block (bytes)*/
    unsigned        super_vers;                 /* Super block version              */
    haddr_t         addr;                       /* Address of superblock            */
    H5P_genplist_t *plist;                      /* Property list                    */
    herr_t          ret_value=SUCCEED;

    /* Encoding */
    FUNC_ENTER_NOAPI(H5F_init_superblock, FAIL)

    /* Get the shared file creation property list */
    if (NULL == (plist = H5I_object(f->shared->fcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    /*
     * The superblock starts immediately after the user-defined
     * header, which we have already insured is a proper size. The
     * base address is set to the same thing as the superblock for
     * now.
     */
    if(H5P_get(plist, H5F_CRT_USER_BLOCK_NAME, &userblock_size) < 0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTGET, FAIL, "unable to get user block size")
    f->shared->super_addr = userblock_size;
    f->shared->base_addr = f->shared->super_addr;
    f->shared->consist_flags = 0x03;

    /* Grab superblock version from property list */
    if (H5P_get(plist, H5F_CRT_SUPER_VERS_NAME, &super_vers) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get super block version")

    /* Compute the size of the superblock */
    superblock_size=H5F_SIGNATURE_LEN   /* Signature length (8 bytes) */
        + 16                            /* Length of required fixed-size portion */
        + ((super_vers>0) ? 4 : 0)      /* Version specific fixed-size portion */
        + 4 * H5F_sizeof_addr(f)        /* Variable-sized addresses */
        + H5G_SIZEOF_ENTRY(f);          /* Size of root group symbol table entry */

    /* Compute the size of the driver information block. */
    H5_ASSIGN_OVERFLOW(driver_size, H5FD_sb_size(f->shared->lf), hsize_t, size_t);
    if (driver_size > 0)
  driver_size += 16; /* Driver block header */

    /*
     * Allocate space for the userblock, superblock, and driver info
     * block. We do it with one allocation request because the
     * userblock and superblock need to be at the beginning of the
     * file and only the first allocation request is required to
     * return memory at format address zero.
     */

    H5_CHECK_OVERFLOW(f->shared->base_addr, haddr_t, hsize_t);
    addr = H5FD_alloc(f->shared->lf, H5FD_MEM_SUPER, dxpl_id,
                      ((hsize_t)f->shared->base_addr + superblock_size + driver_size));

    if (HADDR_UNDEF == addr)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL,
                    "unable to allocate file space for userblock and/or superblock")

    if (0 != addr)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL,
                    "file driver failed to allocate userblock and/or superblock at address zero")

    /*
     * The file driver information block begins immediately after the
     * superblock.
     */
    if (driver_size > 0)
        f->shared->driver_addr = superblock_size;

done:
    FUNC_LEAVE_NOAPI(ret_value)
} /* end H5F_init_superblock() */


/*-------------------------------------------------------------------------
 * Function:    H5F_write_superblock
 *
 * Purpose:     Writes (and optionally allocates) the superblock for the file.
 *              If BUF is non-NULL, then write the serialized superblock
 *              information into it. It should be a buffer of size
 *              H5F_SUPERBLOCK_SIZE + H5F_DRVINFOBLOCK_SIZE or larger.
 *
 * Return:      Success:        SUCCEED
 *              Failure:        FAIL
 *
 * Programmer:  Bill Wendling
 *              wendling@ncsa.uiuc.edu
 *              Sept 12, 2003
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5F_write_superblock(H5F_t *f, hid_t dxpl_id)
{
    uint8_t         sbuf[H5F_SUPERBLOCK_SIZE];  /* Superblock encoding buffer       */
    uint8_t         dbuf[H5F_DRVINFOBLOCK_SIZE];/* Driver info block encoding buffer*/
    uint8_t        *p = NULL;                   /* Ptr into encoding buffers        */
    unsigned        i;                          /* Index variable                   */
    unsigned        chksum;                     /* Checksum temporary variable      */
    size_t          superblock_size;            /* Size of superblock, in bytes     */
    size_t          driver_size;                /* Size of driver info block (bytes)*/
    char            driver_name[9];             /* Name of driver, for driver info block */
    unsigned        super_vers;                 /* Super block version              */
    unsigned        freespace_vers;             /* Freespace info version           */
    unsigned        obj_dir_vers;               /* Object header info version       */
    unsigned        share_head_vers;            /* Shared header info version       */
    H5P_genplist_t *plist;                      /* Property list                    */
    herr_t          ret_value = SUCCEED;

    /* Encoding */
    FUNC_ENTER_NOAPI(H5F_write_superblock, FAIL)

    /* Get the shared file creation property list */
    if (NULL == (plist = H5I_object(f->shared->fcpl_id)))
        HGOTO_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a property list")

    /* Grab values from property list */
    if (H5P_get(plist, H5F_CRT_SUPER_VERS_NAME, &super_vers) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get super block version")
    if (H5P_get(plist, H5F_CRT_FREESPACE_VERS_NAME, &freespace_vers) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get free space version")
    if (H5P_get(plist, H5F_CRT_OBJ_DIR_VERS_NAME, &obj_dir_vers) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get object directory version")
    if (H5P_get(plist, H5F_CRT_SHARE_HEAD_VERS_NAME, &share_head_vers) < 0)
        HGOTO_ERROR(H5E_PLIST, H5E_CANTGET, FAIL, "unable to get shared-header format version")

    /* Encode the file super block */
    p = sbuf;
    HDmemcpy(p, H5F_SIGNATURE, (size_t)H5F_SIGNATURE_LEN);
    p += H5F_SIGNATURE_LEN;
    *p++ = (uint8_t)super_vers;
    *p++ = (uint8_t)freespace_vers;
    *p++ = (uint8_t)obj_dir_vers;
    *p++ = 0;   /* reserved*/
    *p++ = (uint8_t)share_head_vers;
    assert (H5F_SIZEOF_ADDR(f) <= 255);
    *p++ = (uint8_t)H5F_SIZEOF_ADDR(f);
    assert (H5F_SIZEOF_SIZE(f) <= 255);
    *p++ = (uint8_t)H5F_SIZEOF_SIZE(f);
    *p++ = 0;   /* reserved */
    UINT16ENCODE(p, f->shared->sym_leaf_k);
    UINT16ENCODE(p, f->shared->btree_k[H5B_SNODE_ID]);
    UINT32ENCODE(p, f->shared->consist_flags);

    /*
     * Versions of the superblock >0 have the indexed storage B-tree
     * internal 'K' value stored
     */
    if (super_vers > 0) {
        UINT16ENCODE(p, f->shared->btree_k[H5B_ISTORE_ID]);
        *p++ = 0;   /*reserved */
        *p++ = 0;   /*reserved */
    }

    H5F_addr_encode(f, &p, f->shared->base_addr);
    H5F_addr_encode(f, &p, f->shared->freespace_addr);
    H5F_addr_encode(f, &p, H5FD_get_eoa(f->shared->lf));
    H5F_addr_encode(f, &p, f->shared->driver_addr);
    if(H5G_ent_encode(f, &p, H5G_entof(f->shared->root_grp))<0)
        HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to encode root group information")

    H5_ASSIGN_OVERFLOW(superblock_size, p - sbuf, int, size_t);

    /* Double check we didn't overrun the block (unlikely) */
    assert(superblock_size <= sizeof(sbuf));

    /* Encode the driver information block. */
    H5_ASSIGN_OVERFLOW(driver_size, H5FD_sb_size(f->shared->lf), hsize_t, size_t);

    if (driver_size > 0) {
  driver_size += 16; /* Driver block header */

        /* Double check we didn't overrun the block (unlikely) */
  assert(driver_size <= sizeof(dbuf));

        /* Encode the driver information block */
  p = dbuf;

  *p++ = HDF5_DRIVERINFO_VERSION; /* Version */
  *p++ = 0; /* reserved */
  *p++ = 0; /* reserved */
  *p++ = 0; /* reserved */

  /* Driver info size, excluding header */
  UINT32ENCODE(p, driver_size - 16);

  /* Encode driver-specific data */
  if (H5FD_sb_encode(f->shared->lf, driver_name, dbuf + 16) < 0)
      HGOTO_ERROR(H5E_FILE, H5E_CANTINIT, FAIL, "unable to encode driver information")

  /* Driver name */
  HDmemcpy(dbuf + 8, driver_name, (size_t)8);
    } /* end if */

    /* Compute super block checksum */
    assert(sizeof(chksum) == sizeof(f->shared->super_chksum));

    for (p = (uint8_t *)&chksum, chksum = 0, i = 0; i < superblock_size; ++i)
        p[i % sizeof(f->shared->super_chksum)] ^= sbuf[i];

    /* Compare with current checksums */
    if (chksum != f->shared->super_chksum) {
        /* Write superblock */
        if (H5FD_write(f->shared->lf, H5FD_MEM_SUPER, dxpl_id,
                       f->shared->super_addr, superblock_size, sbuf) < 0)
            HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to write superblock")

        /* Update checksum information if different */
        f->shared->super_chksum = chksum;
    } /* end if */

    /* Check for driver info block */
    if (HADDR_UNDEF != f->shared->driver_addr) {
        /* Compute driver info block checksum */
        assert(sizeof(chksum) == sizeof(f->shared->drvr_chksum));

        for (p = (uint8_t *)&chksum, chksum = 0, i = 0; i < driver_size; ++i)
            p[i % sizeof(f->shared->drvr_chksum)] ^= dbuf[i];

        /* Compare with current checksums */
        if (chksum != f->shared->drvr_chksum) {
            /* Write driver information block */
            if (H5FD_write(f->shared->lf, H5FD_MEM_SUPER, dxpl_id,
                           f->shared->base_addr + f->shared->driver_addr, driver_size, dbuf) < 0)
                HGOTO_ERROR(H5E_IO, H5E_WRITEERROR, FAIL, "unable to write driver information block")

            /* Update checksum information if different */
            f->shared->drvr_chksum = chksum;
        } /* end if */
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value)
}

