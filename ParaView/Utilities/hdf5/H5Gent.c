/*
 * Copyright (C) 1997 National Center for Supercomputing Applications.
 *                    All rights reserved.
 *
 * Programmer: Robb Matzke <matzke@llnl.gov>
 *             Friday, September 19, 1997
 */
#define H5G_PACKAGE
#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */

#include "H5private.h"
#include "H5Eprivate.h"
#include "H5Fpkg.h"
#include "H5Gpkg.h"
#include "H5HLprivate.h"
#include "H5MMprivate.h"

#define PABLO_MASK      H5G_ent_mask
static int              interface_initialize_g = 0;
#define INTERFACE_INIT  NULL


/*-------------------------------------------------------------------------
 * Function:    H5G_ent_cache
 *
 * Purpose:     Returns a pointer to the cache associated with the symbol
 *              table entry.  You should modify the cache directly, then call
 *              H5G_modified() with the new cache type (even if the type is
 *              still the same).
 *
 * Return:      Success:        Ptr to the cache in the symbol table entry.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, September 19, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5G_cache_t            *
H5G_ent_cache(H5G_entry_t *ent, H5G_type_t *cache_type)
{
    FUNC_ENTER(H5G_ent_cache, NULL);
    if (!ent) {
        HRETURN_ERROR(H5E_SYM, H5E_BADVALUE, NULL, "no entry");
    }
    if (cache_type)
        *cache_type = ent->type;

    FUNC_LEAVE(&(ent->cache));
}

/*-------------------------------------------------------------------------
 * Function:    H5G_ent_modified
 *
 * Purpose:     This function should be called after you make any
 *              modifications to a symbol table entry cache.  Supply the new
 *              type for the cache.  If CACHE_TYPE is the constant
 *              H5G_NO_CHANGE then the cache type isn't changed--just the
 *              dirty bit is set.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, September 19, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_ent_modified(H5G_entry_t *ent, H5G_type_t cache_type)
{
    FUNC_ENTER(H5G_ent_modified, FAIL);
    assert(ent);
    if (H5G_NO_CHANGE != ent->type) ent->type = cache_type;
    ent->dirty = TRUE;
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5G_ent_decode_vec
 *
 * Purpose:     Same as H5G_ent_decode() except it does it for an array of
 *              symbol table entries.
 *
 * Errors:
 *              SYM       CANTDECODE    Can't decode. 
 *
 * Return:      Success:        Non-negative, with *pp pointing to the first byte
 *                              after the last symbol.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 18 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_ent_decode_vec(H5F_t *f, const uint8_t **pp, H5G_entry_t *ent, int n)
{
    int                    i;

    FUNC_ENTER(H5G_ent_decode_vec, FAIL);

    /* check arguments */
    assert(f);
    assert(pp);
    assert(ent);
    assert(n >= 0);

    /* decode entries */
    for (i = 0; i < n; i++) {
        if (H5G_ent_decode(f, pp, ent + i) < 0) {
            HRETURN_ERROR(H5E_SYM, H5E_CANTDECODE, FAIL, "can't decode");
        }
    }

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5G_ent_decode
 *
 * Purpose:     Decodes a symbol table entry pointed to by `*pp'.
 *
 * Errors:
 *
 * Return:      Success:        Non-negative with *pp pointing to the first byte
 *                              following the symbol table entry.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 18 1997
 *
 * Modifications:
 *      Robb Matzke, 17 Jul 1998
 *      Added a 4-byte padding field for alignment and future expansion.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_ent_decode(H5F_t *f, const uint8_t **pp, H5G_entry_t *ent)
{
    const uint8_t       *p_ret = *pp;
    uint32_t            tmp;

    FUNC_ENTER(H5G_ent_decode, FAIL);

    /* check arguments */
    assert(f);
    assert(pp);
    assert(ent);

    ent->file = f;

    /* decode header */
    H5F_DECODE_LENGTH(f, *pp, ent->name_off);
    H5F_addr_decode(f, pp, &(ent->header));
    UINT32DECODE(*pp, tmp);
    *pp += 4; /*reserved*/
    ent->type=(H5G_type_t)tmp;

    /* decode scratch-pad */
    switch (ent->type) {
    case H5G_NOTHING_CACHED:
        break;

    case H5G_CACHED_STAB:
        assert(2 * H5F_SIZEOF_ADDR(f) <= H5G_SIZEOF_SCRATCH);
        H5F_addr_decode(f, pp, &(ent->cache.stab.btree_addr));
        H5F_addr_decode(f, pp, &(ent->cache.stab.heap_addr));
        break;

    case H5G_CACHED_SLINK:
        UINT32DECODE (*pp, ent->cache.slink.lval_offset);
        break;

    default:
        HDabort();
    }

    *pp = p_ret + H5G_SIZEOF_ENTRY(f);
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5G_ent_encode_vec
 *
 * Purpose:     Same as H5G_ent_encode() except it does it for an array of
 *              symbol table entries.
 *
 * Errors:
 *              SYM       CANTENCODE    Can't encode. 
 *
 * Return:      Success:        Non-negative, with *pp pointing to the first byte
 *                              after the last symbol.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 18 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_ent_encode_vec(H5F_t *f, uint8_t **pp, const H5G_entry_t *ent, int n)
{
    int                    i;

    FUNC_ENTER(H5G_ent_encode_vec, FAIL);

    /* check arguments */
    assert(f);
    assert(pp);
    assert(ent);
    assert(n >= 0);

    /* encode entries */
    for (i = 0; i < n; i++) {
        if (H5G_ent_encode(f, pp, ent + i) < 0) {
            HRETURN_ERROR(H5E_SYM, H5E_CANTENCODE, FAIL, "can't encode");
        }
    }

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5G_ent_encode
 *
 * Purpose:     Encodes the specified symbol table entry into the buffer
 *              pointed to by *pp.
 *
 * Errors:
 *
 * Return:      Success:        Non-negative, with *pp pointing to the first byte
 *                              after the symbol table entry.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul 18 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 8 Aug 1997
 *      Writes zeros for the bytes that aren't used so the file doesn't
 *      contain junk.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_ent_encode(H5F_t *f, uint8_t **pp, const H5G_entry_t *ent)
{
    uint8_t             *p_ret = *pp + H5G_SIZEOF_ENTRY(f);

    FUNC_ENTER(H5G_ent_encode, FAIL);

    /* check arguments */
    assert(f);
    assert(pp);

    if (ent) {
        /* encode header */
        H5F_ENCODE_LENGTH(f, *pp, ent->name_off);
        H5F_addr_encode(f, pp, ent->header);
        UINT32ENCODE(*pp, ent->type);
        UINT32ENCODE(*pp, 0); /*reserved*/

        /* encode scratch-pad */
        switch (ent->type) {
        case H5G_NOTHING_CACHED:
            break;

        case H5G_CACHED_STAB:
            assert(2 * H5F_SIZEOF_ADDR(f) <= H5G_SIZEOF_SCRATCH);
            H5F_addr_encode(f, pp, ent->cache.stab.btree_addr);
            H5F_addr_encode(f, pp, ent->cache.stab.heap_addr);
            break;

        case H5G_CACHED_SLINK:
            UINT32ENCODE (*pp, ent->cache.slink.lval_offset);
            break;

        default:
            HDabort();
        }
    } else {
        H5F_ENCODE_LENGTH(f, *pp, 0);
        H5F_addr_encode(f, pp, HADDR_UNDEF);
        UINT32ENCODE(*pp, H5G_NOTHING_CACHED);
        UINT32ENCODE(*pp, 0); /*reserved*/
    }

    /* fill with zero */
    while (*pp < p_ret) *(*pp)++ = 0;
    *pp = p_ret;
    
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5G_ent_debug
 *
 * Purpose:     Prints debugging information about a symbol table entry.
 *
 * Errors:
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 29 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The HEAP argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5G_ent_debug(H5F_t UNUSED *f, const H5G_entry_t *ent, FILE * stream,
              int indent, int fwidth, haddr_t heap)
{
    const char          *lval = NULL;
    
    FUNC_ENTER(H5G_ent_debug, FAIL);

    HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
              "Name offset into private heap:",
              (unsigned long) (ent->name_off));

    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
              "Object header address:", ent->header);

    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
              "Dirty:",
              ent->dirty ? "Yes" : "No");
    HDfprintf(stream, "%*s%-*s ", indent, "", fwidth,
              "Symbol type:");
    switch (ent->type) {
    case H5G_NOTHING_CACHED:
        HDfprintf(stream, "Nothing Cached\n");
        break;

    case H5G_CACHED_STAB:
        HDfprintf(stream, "Symbol Table\n");

        HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
                  "B-tree address:", ent->cache.stab.btree_addr);

        HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
                  "Heap address:", ent->cache.stab.heap_addr);
        break;

    case H5G_CACHED_SLINK:
        HDfprintf (stream, "Symbolic Link\n");
        HDfprintf (stream, "%*s%-*s %lu\n", indent, "", fwidth,
                   "Link value offset:",
                   (unsigned long)(ent->cache.slink.lval_offset));
        if (H5F_addr_defined(heap)) {
            lval = H5HL_peek (ent->file, heap, ent->cache.slink.lval_offset);
            HDfprintf (stream, "%*s%-*s %s\n", indent, "", fwidth,
                       "Link value:",
                       lval);
        }
        break;
        
    default:
        HDfprintf(stream, "*** Unknown symbol type %d\n", ent->type);
        break;
    }

    FUNC_LEAVE(SUCCEED);

    f = 0;
}
