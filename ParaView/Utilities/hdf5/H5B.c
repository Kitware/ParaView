/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:             hdf5btree.c
 *                      Jul 10 1997
 *                      Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:             Implements balanced, sibling-linked, N-ary trees
 *                      capable of storing any type of data with unique key
 *                      values.
 *
 *                      A B-link-tree is a balanced tree where each node has
 *                      a pointer to its left and right siblings.  A
 *                      B-link-tree is a rooted tree having the following
 *                      properties:
 *
 *                      1. Every node, x, has the following fields:
 *
 *                         a. level[x], the level in the tree at which node
 *                            x appears.  Leaf nodes are at level zero.
 *
 *                         b. n[x], the number of children pointed to by the
 *                            node.  Internal nodes point to subtrees while
 *                            leaf nodes point to arbitrary data.
 *
 *                         c. The child pointers themselves, child[x,i] such
 *                            that 0 <= i < n[x].
 *
 *                         d. n[x]+1 key values stored in increasing
 *                            order:
 *
 *                              key[x,0] < key[x,1] < ... < key[x,n[x]].
 *
 *                         e. left[x] is a pointer to the node's left sibling
 *                            or the null pointer if this is the left-most
 *                            node at this level in the tree.
 *                            
 *                         f. right[x] is a pointer to the node's right
 *                            sibling or the null pointer if this is the
 *                            right-most node at this level in the tree.
 *
 *                      3. The keys key[x,i] partition the key spaces of the
 *                         children of x:
 *
 *                            key[x,i] <= key[child[x,i],j] <= key[x,i+1]
 *
 *                         for any valid combination of i and j.
 *
 *                      4. There are lower and upper bounds on the number of
 *                         child pointers a node can contain.  These bounds
 *                         can be expressed in terms of a fixed integer k>=2
 *                         called the `minimum degree' of the B-tree.
 *
 *                         a. Every node other than the root must have at least
 *                            k child pointers and k+1 keys.  If the tree is
 *                            nonempty, the root must have at least one child
 *                            pointer and two keys.
 *
 *                         b. Every node can contain at most 2k child pointers
 *                            and 2k+1 keys.  A node is `full' if it contains
 *                            exactly 2k child pointers and 2k+1 keys.
 *
 *                      5. When searching for a particular value, V, and
 *                         key[V] = key[x,i] for some node x and entry i,
 *                         then:
 *
 *                         a. If i=0 the child[0] is followed.
 *
 *                         b. If i=n[x] the child[n[x]-1] is followed.
 *
 *                         c. Otherwise, the child that is followed
 *                            (either child[x,i-1] or child[x,i]) is
 *                            determined by the type of object to which the
 *                            leaf nodes of the tree point and is controlled
 *                            by the key comparison function registered for
 *                            that type of B-tree.
 *
 *
 * Modifications:
 *
 *      Robb Matzke, 4 Aug 1997
 *      Added calls to H5E.
 *
 *-------------------------------------------------------------------------
 */

#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */

/* private headers */
#include "H5private.h"          /*library                               */
#include "H5ACprivate.h"        /*cache                                 */
#include "H5Bprivate.h"         /*B-link trees                          */
#include "H5Eprivate.h"         /*error handling                        */
#include "H5Fpkg.h"             /*file access                           */
#include "H5FLprivate.h"        /*Free Lists                            */
#include "H5MFprivate.h"        /*file memory management                */
#include "H5MMprivate.h"        /*core memory management                */
#include "H5Pprivate.h"         /*property lists                        */

#include "H5FDmpio.h"           /*for H5FD_mpio_tas_allsame()           */

#define PABLO_MASK      H5B_mask

#define BOUND(MIN,X,MAX) ((X)<(MIN)?(MIN):((X)>(MAX)?(MAX):(X)))

/* PRIVATE PROTOTYPES */
static H5B_ins_t H5B_insert_helper(H5F_t *f, haddr_t addr,
                                   const H5B_class_t *type,
                                   const double split_ratios[],
                                   uint8_t *lt_key,
                                   hbool_t *lt_key_changed,
                                   uint8_t *md_key, void *udata,
                                   uint8_t *rt_key,
                                   hbool_t *rt_key_changed,
                                   haddr_t *retval);
static herr_t H5B_insert_child(H5F_t *f, const H5B_class_t *type,
                               H5B_t *bt, int idx, haddr_t child,
                               H5B_ins_t anchor, void *md_key);
static herr_t H5B_flush(H5F_t *f, hbool_t destroy, haddr_t addr, H5B_t *b);
static H5B_t *H5B_load(H5F_t *f, haddr_t addr, const void *_type, void *udata);
static herr_t H5B_decode_key(H5F_t *f, H5B_t *bt, int idx);
static herr_t H5B_decode_keys(H5F_t *f, H5B_t *bt, int idx);
static hsize_t H5B_nodesize(H5F_t *f, const H5B_class_t *type,
                           hsize_t *total_nkey_size, size_t sizeof_rkey);
static herr_t H5B_split(H5F_t *f, const H5B_class_t *type, H5B_t *old_bt,
                        haddr_t old_addr, int idx,
                        const double split_ratios[], void *udata,
                        haddr_t *new_addr/*out*/);
static H5B_t * H5B_copy(H5F_t *f, const H5B_t *old_bt);
#ifdef H5B_DEBUG
static herr_t H5B_assert(H5F_t *f, haddr_t addr, const H5B_class_t *type,
                         void *udata);
#endif

/* H5B inherits cache-like properties from H5AC */
static const H5AC_class_t H5AC_BT[1] = {{
    H5AC_BT_ID,
    (void *(*)(H5F_t*, haddr_t, const void*, void*))H5B_load,
    (herr_t (*)(H5F_t*, hbool_t, haddr_t, void*))H5B_flush,
}};

/* Interface initialization? */
#define INTERFACE_INIT NULL
static int interface_initialize_g = 0;

/* Declare a free list to manage the page information */
H5FL_BLK_DEFINE_STATIC(page);

/* Declare a PQ free list to manage the native block information */
H5FL_BLK_DEFINE_STATIC(native_block);

/* Declare a free list to manage the H5B_key_t array information */
H5FL_ARR_DEFINE_STATIC(H5B_key_t,-1);

/* Declare a free list to manage the haddr_t array information */
H5FL_ARR_DEFINE_STATIC(haddr_t,-1);

/* Declare a free list to manage the H5B_t struct */
H5FL_DEFINE_STATIC(H5B_t);


/*-------------------------------------------------------------------------
 * Function:    H5B_create
 *
 * Purpose:     Creates a new empty B-tree leaf node.  The UDATA pointer is
 *              passed as an argument to the sizeof_rkey() method for the
 *              B-tree.
 *
 * Return:      Success:        Non-negative, and the address of new node is
 *                              returned through the ADDR_P argument.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jun 23 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              Changed the name of the ADDR argument to ADDR_P to make it
 *              obvious that the address is passed by reference unlike most
 *              other functions that take addresses.
 *-------------------------------------------------------------------------
 */
herr_t
H5B_create(H5F_t *f, const H5B_class_t *type, void *udata,
           haddr_t *addr_p/*out*/)
{
    H5B_t               *bt = NULL;
    size_t              sizeof_rkey;
    hsize_t             size;
    hsize_t             total_native_keysize;
    size_t              offset;
    int         i;
    herr_t              ret_value = FAIL;

    FUNC_ENTER(H5B_create, FAIL);

    /*
     * Check arguments.
     */
    assert(f);
    assert(type);
    assert(addr_p);

    /*
     * Allocate file and memory data structures.
     */
    sizeof_rkey = (type->get_sizeof_rkey) (f, udata);
    size = H5B_nodesize(f, type, &total_native_keysize, sizeof_rkey);
    if (HADDR_UNDEF==(*addr_p=H5MF_alloc(f, H5FD_MEM_BTREE, size))) {
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                    "file allocation failed for B-tree root node");
    }
    if (NULL==(bt = H5FL_ALLOC(H5B_t,1))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "memory allocation failed for B-tree root node");
    }
    bt->type = type;
    bt->sizeof_rkey = sizeof_rkey;
    bt->dirty = TRUE;
    bt->ndirty = 0;
    bt->level = 0;
    bt->left = HADDR_UNDEF;
    bt->right = HADDR_UNDEF;
    bt->nchildren = 0;
    if (NULL==(bt->page=H5FL_BLK_ALLOC(page,size,1)) ||
        NULL==(bt->native=H5FL_BLK_ALLOC(native_block,total_native_keysize,0)) ||
        NULL==(bt->child=H5FL_ARR_ALLOC(haddr_t,(hsize_t)(2*H5B_K(f,type)),0)) ||
        NULL==(bt->key=H5FL_ARR_ALLOC(H5B_key_t,(hsize_t)(2*H5B_K(f,type)+1),0))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                     "memory allocation failed for B-tree root node");
    }

    /*
     * Initialize each entry's raw child and key pointers to point into the
     * `page' buffer.  Each native key pointer should be null until the key is
     * translated to native format.
     */
    for (i = 0, offset = H5B_SIZEOF_HDR(f);
         i < 2 * H5B_K(f, type);
         i++, offset += bt->sizeof_rkey + H5F_SIZEOF_ADDR(f)) {

        bt->key[i].dirty = FALSE;
        bt->key[i].rkey = bt->page + offset;
        bt->key[i].nkey = NULL;
        bt->child[i] = HADDR_UNDEF;
    }

    /*
     * The last possible key...
     */
    bt->key[2 * H5B_K(f, type)].dirty = FALSE;
    bt->key[2 * H5B_K(f, type)].rkey = bt->page + offset;
    bt->key[2 * H5B_K(f, type)].nkey = NULL;

    /*
     * Cache the new B-tree node.
     */
    if (H5AC_set(f, H5AC_BT, *addr_p, bt) < 0) {
        HRETURN_ERROR(H5E_BTREE, H5E_CANTINIT, FAIL,
                      "can't add B-tree root node to cache");
    }
#ifdef H5B_DEBUG
    H5B_assert(f, *addr_p, type, udata);
#endif
    ret_value = SUCCEED;
    
 done:
    if (ret_value<0) {
        H5MF_xfree(f, H5FD_MEM_BTREE, *addr_p, size);
        if (bt) {
            H5FL_BLK_FREE (page,bt->page);
            H5FL_BLK_FREE (native_block,bt->native);
            H5FL_ARR_FREE (haddr_t,bt->child);
            H5FL_ARR_FREE (H5B_key_t,bt->key);
            H5FL_FREE (H5B_t,bt);
        }
    }
    
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5B_load
 *
 * Purpose:     Loads a B-tree node from the disk.
 *
 * Return:      Success:        Pointer to a new B-tree node.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jun 23 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
static H5B_t *
H5B_load(H5F_t *f, haddr_t addr, const void *_type, void *udata)
{
    const H5B_class_t   *type = (const H5B_class_t *) _type;
    hsize_t             total_nkey_size;
    hsize_t             size;
    H5B_t               *bt = NULL;
    int         i;
    uint8_t             *p;
    H5B_t               *ret_value = NULL;

    FUNC_ENTER(H5B_load, NULL);

    /* Check arguments */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(type);
    assert(type->get_sizeof_rkey);

    if (NULL==(bt = H5FL_ALLOC(H5B_t,1))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                     "memory allocation failed");
    }
    bt->sizeof_rkey = (type->get_sizeof_rkey) (f, udata);
    size = H5B_nodesize(f, type, &total_nkey_size, bt->sizeof_rkey);
    bt->type = type;
    bt->dirty = FALSE;
    bt->ndirty = 0;
    if (NULL==(bt->page=H5FL_BLK_ALLOC(page,size,0)) ||
        NULL==(bt->native=H5FL_BLK_ALLOC(native_block,total_nkey_size,0)) ||
        NULL==(bt->key=H5FL_ARR_ALLOC(H5B_key_t,(hsize_t)(2*H5B_K(f,type)+1),0)) ||
        NULL==(bt->child=H5FL_ARR_ALLOC(haddr_t,(hsize_t)(2*H5B_K(f,type)),0))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                     "memory allocation failed");
    }
    if (H5F_block_read(f, H5FD_MEM_BTREE, addr, (hsize_t)size, H5P_DEFAULT, bt->page)<0) {
        HGOTO_ERROR(H5E_BTREE, H5E_READERROR, NULL,
                      "can't read B-tree node");
    }
    p = bt->page;

    /* magic number */
    if (HDmemcmp(p, H5B_MAGIC, H5B_SIZEOF_MAGIC)) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, NULL,
                    "wrong B-tree signature");
    }
    p += 4;

    /* node type and level */
    if (*p++ != type->id) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, NULL,
                    "incorrect B-tree node level");
    }
    bt->level = *p++;

    /* entries used */
    UINT16DECODE(p, bt->nchildren);

    /* sibling pointers */
    H5F_addr_decode(f, (const uint8_t **) &p, &(bt->left));
    H5F_addr_decode(f, (const uint8_t **) &p, &(bt->right));

    /* the child/key pairs */
    for (i = 0; i < 2 * H5B_K(f, type); i++) {

        bt->key[i].dirty = FALSE;
        bt->key[i].rkey = p;
        p += bt->sizeof_rkey;
        bt->key[i].nkey = NULL;

        if (i < bt->nchildren) {
            H5F_addr_decode(f, (const uint8_t **) &p, bt->child + i);
        } else {
            bt->child[i] = HADDR_UNDEF;
            p += H5F_SIZEOF_ADDR(f);
        }
    }

    bt->key[2 * H5B_K(f, type)].dirty = FALSE;
    bt->key[2 * H5B_K(f, type)].rkey = p;
    bt->key[2 * H5B_K(f, type)].nkey = NULL;
    ret_value = bt;

  done:
    if (!ret_value && bt) {
        H5FL_ARR_FREE(haddr_t,bt->child);
        H5FL_ARR_FREE(H5B_key_t,bt->key);
        H5FL_BLK_FREE(page,bt->page);
        H5FL_BLK_FREE(native_block,bt->native);
        H5FL_FREE(H5B_t,bt);
    }
    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5B_flush
 *
 * Purpose:     Flushes a dirty B-tree node to disk.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jun 23 1997
 *
 * Modifications:
 *              rky 980828
 *              Only p0 writes metadata to disk.
 *
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
static herr_t
H5B_flush(H5F_t *f, hbool_t destroy, haddr_t addr, H5B_t *bt)
{
    int i;
    hsize_t     size = 0;
    uint8_t     *p = bt->page;

    FUNC_ENTER(H5B_flush, FAIL);

    /*
     * Check arguments.
     */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(bt);
    assert(bt->type);
    assert(bt->type->encode);

    size = H5B_nodesize(f, bt->type, NULL, bt->sizeof_rkey);

    if (bt->dirty) {

        /* magic number */
        HDmemcpy(p, H5B_MAGIC, H5B_SIZEOF_MAGIC);
        p += 4;

        /* node type and level */
        *p++ = bt->type->id;
        *p++ = bt->level;

        /* entries used */
        UINT16ENCODE(p, bt->nchildren);

        /* sibling pointers */
        H5F_addr_encode(f, &p, bt->left);
        H5F_addr_encode(f, &p, bt->right);

        /* child keys and pointers */
        for (i=0; i<=bt->nchildren; i++) {

            /* encode the key */
            assert(bt->key[i].rkey == p);
            if (bt->key[i].dirty) {
                if (bt->key[i].nkey) {
                    if ((bt->type->encode) (f, bt, bt->key[i].rkey,
                                            bt->key[i].nkey) < 0) {
                        HRETURN_ERROR(H5E_BTREE, H5E_CANTENCODE, FAIL,
                                      "unable to encode B-tree key");
                    }
                }
                bt->key[i].dirty = FALSE;
            }
            p += bt->sizeof_rkey;

            /* encode the child address */
            if (i < bt->ndirty) {
                H5F_addr_encode(f, &p, bt->child[i]);
            } else {
                p += H5F_SIZEOF_ADDR(f);
            }
        }

        /*
         * Write the disk page.  We always write the header, but we don't
         * bother writing data for the child entries that don't exist or
         * for the final unchanged children.
         */
#ifdef H5_HAVE_PARALLEL
        if (IS_H5FD_MPIO(f))
            H5FD_mpio_tas_allsame(f->shared->lf, TRUE); /* only p0 will write */
#endif /* H5_HAVE_PARALLEL */
        if (H5F_block_write(f, H5FD_MEM_BTREE, addr, (hsize_t)size, H5P_DEFAULT, bt->page)<0) {
            HRETURN_ERROR(H5E_BTREE, H5E_CANTFLUSH, FAIL,
                          "unable to save B-tree node to disk");
        }
        bt->dirty = FALSE;
        bt->ndirty = 0;
    }
    if (destroy) {
        H5FL_ARR_FREE(haddr_t,bt->child);
        H5FL_ARR_FREE(H5B_key_t,bt->key);
        H5FL_BLK_FREE(page,bt->page);
        H5FL_BLK_FREE(native_block,bt->native);
        H5FL_FREE(H5B_t,bt);
    }
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5B_find
 *
 * Purpose:     Locate the specified information in a B-tree and return
 *              that information by filling in fields of the caller-supplied
 *              UDATA pointer depending on the type of leaf node
 *              requested.  The UDATA can point to additional data passed
 *              to the key comparison function.
 *
 * Note:        This function does not follow the left/right sibling
 *              pointers since it assumes that all nodes can be reached
 *              from the parent node.
 *
 * Return:      Non-negative on success (if found, values returned through the
 *              UDATA argument). Negative on failure (if not found, UDATA is
 *              undefined).
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jun 23 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5B_find(H5F_t *f, const H5B_class_t *type, haddr_t addr, void *udata)
{
    H5B_t       *bt = NULL;
    int idx = -1, lt = 0, rt, cmp = 1;
    int         ret_value = FAIL;

    FUNC_ENTER(H5B_find, FAIL);

    /*
     * Check arguments.
     */
    assert(f);
    assert(type);
    assert(type->decode);
    assert(type->cmp3);
    assert(type->found);
    assert(H5F_addr_defined(addr));

    /*
     * Perform a binary search to locate the child which contains
     * the thing for which we're searching.
     */
    if (NULL == (bt = H5AC_protect(f, H5AC_BT, addr, type, udata))) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL,
                    "unable to load B-tree node");
    }
    rt = bt->nchildren;

    while (lt < rt && cmp) {
        idx = (lt + rt) / 2;
        if (H5B_decode_keys(f, bt, idx) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, FAIL,
                        "unable to decode B-tree key(s)");
        }
        /* compare */
        if ((cmp = (type->cmp3) (f, bt->key[idx].nkey, udata,
                                 bt->key[idx+1].nkey)) < 0) {
            rt = idx;
        } else {
            lt = idx+1;
        }
    }
    if (cmp) {
        HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL,
                    "B-tree key not found");
    }
    
    /*
     * Follow the link to the subtree or to the data node.
     */
    assert(idx >= 0 && idx < bt->nchildren);
    if (bt->level > 0) {
        if ((ret_value = H5B_find(f, type, bt->child[idx], udata)) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL,
                        "key not found in subtree");
        }
    } else {
        ret_value = (type->found) (f, bt->child[idx], bt->key[idx].nkey,
                                   udata, bt->key[idx+1].nkey);
        if (ret_value < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, FAIL,
                        "key not found in leaf node");
        }
    }

  done:
    if (bt && H5AC_unprotect(f, H5AC_BT, addr, bt) < 0) {
        HRETURN_ERROR(H5E_BTREE, H5E_PROTECT, FAIL,
                      "unable to release node");
    }
    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5B_split
 *
 * Purpose:     Split a single node into two nodes.  The old node will
 *              contain the left children and the new node will contain the
 *              right children.
 *
 *              The UDATA pointer is passed to the sizeof_rkey() method but is
 *              otherwise unused.
 *
 *              The OLD_BT argument is a pointer to a protected B-tree
 *              node.
 *
 * Return:      Non-negative on success (The address of the new node is
 *              returned through the NEW_ADDR argument). Negative on failure.
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  3 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The OLD_ADDR argument is passed by value. The NEW_ADDR
 *              argument has been renamed to NEW_ADDR_P
 *-------------------------------------------------------------------------
 */
static herr_t
H5B_split(H5F_t *f, const H5B_class_t *type, H5B_t *old_bt, haddr_t old_addr,
          int idx, const double split_ratios[], void *udata,
          haddr_t *new_addr_p/*out*/)
{
    H5B_t       *new_bt = NULL, *tmp_bt = NULL;
    herr_t      ret_value = FAIL;
    int i, k, nleft, nright;
    size_t      recsize = 0;

    FUNC_ENTER(H5B_split, FAIL);

    /*
     * Check arguments.
     */
    assert(f);
    assert(type);
    assert(H5F_addr_defined(old_addr));

    /*
     * Initialize variables.
     */
    assert(old_bt->nchildren == 2 * H5B_K(f, type));
    recsize = old_bt->sizeof_rkey + H5F_SIZEOF_ADDR(f);
    k = H5B_K(f, type);

#ifdef H5B_DEBUG
    if (H5DEBUG(B)) {
        const char *side;
        if (!H5F_addr_defined(old_bt->left) &&
            !H5F_addr_defined(old_bt->right)) {
            side = "ONLY";
        } else if (!H5F_addr_defined(old_bt->right)) {
            side = "RIGHT";
        } else if (!H5F_addr_defined(old_bt->left)) {
            side = "LEFT";
        } else {
            side = "MIDDLE";
        }
        fprintf(H5DEBUG(B), "H5B_split: %3d {%5.3f,%5.3f,%5.3f} %6s",
                2*k, split_ratios[0], split_ratios[1], split_ratios[2], side);
    }
#endif

    /*
     * Decide how to split the children of the old node among the old node
     * and the new node.
     */
    if (!H5F_addr_defined(old_bt->right)) {
        nleft = 2 * k * split_ratios[2];        /*right*/
    } else if (!H5F_addr_defined(old_bt->left)) {
        nleft = 2 * k * split_ratios[0];        /*left*/
    } else {
        nleft = 2 * k * split_ratios[1];        /*middle*/
    }

    /*
     * Keep the new child in the same node as the child that split.  This can
     * result in nodes that have an unused child when data is written
     * sequentially, but it simplifies stuff below.
     */
    if (idx<nleft && nleft==2*k) {
        --nleft;
    } else if (idx>=nleft && 0==nleft) {
        nleft++;
    }
    nright = 2*k - nleft;
#ifdef H5B_DEBUG
    if (H5DEBUG(B)) {
        fprintf(H5DEBUG(B), " split %3d/%-3d\n", nleft, nright);
    }
#endif
    
    /*
     * Create the new B-tree node.
     */
    if (H5B_create(f, type, udata, new_addr_p/*out*/) < 0) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, FAIL,
                    "unable to create B-tree");
    }
    if (NULL==(new_bt=H5AC_protect(f, H5AC_BT, *new_addr_p, type, udata))) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL,
                    "unable to protect B-tree");
    }
    new_bt->level = old_bt->level;

    /*
     * Copy data from the old node to the new node.
     */
    HDmemcpy(new_bt->page + H5B_SIZEOF_HDR(f),
             old_bt->page + H5B_SIZEOF_HDR(f) + nleft * recsize,
             nright * recsize + new_bt->sizeof_rkey);
    HDmemcpy(new_bt->native,
             old_bt->native + nleft * type->sizeof_nkey,
             (nright+1) * type->sizeof_nkey);

    for (i=0; i<=nright; i++) {
        /* key */
        new_bt->key[i].dirty = old_bt->key[nleft+i].dirty;
        if (old_bt->key[nleft+i].nkey) {
            new_bt->key[i].nkey = new_bt->native + i * type->sizeof_nkey;
        }
        /* child */
        if (i < nright) {
            new_bt->child[i] = old_bt->child[nleft+i];
        }
    }
    new_bt->ndirty = new_bt->nchildren = nright;

    /*
     * Truncate the old node.
     */
    old_bt->dirty = TRUE;
    old_bt->nchildren = nleft;
    old_bt->ndirty = MIN(old_bt->ndirty, old_bt->nchildren);
    
    /*
     * Update sibling pointers.
     */
    new_bt->left = old_addr;
    new_bt->right = old_bt->right;

    if (H5F_addr_defined(old_bt->right)) {
        if (NULL == (tmp_bt = H5AC_find(f, H5AC_BT, old_bt->right, type,
                                        udata))) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL,
                        "unable to load right sibling");
        }
        tmp_bt->dirty = TRUE;
        tmp_bt->left = *new_addr_p;
    }
    old_bt->right = *new_addr_p;

    HGOTO_DONE(SUCCEED);

  done:
    {
        if (new_bt && H5AC_unprotect(f, H5AC_BT, *new_addr_p, new_bt) < 0) {
            HRETURN_ERROR(H5E_BTREE, H5E_PROTECT, FAIL,
                          "unable to release B-tree node");
        }
    }
    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5B_decode_key
 *
 * Purpose:     Decode the specified key into native format.  Do not call
 *              this function if the key is already decoded since it my
 *              decode a stale raw key into the native key.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  8 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B_decode_key(H5F_t *f, H5B_t *bt, int idx)
{
    FUNC_ENTER(H5B_decode_key, FAIL);

    bt->key[idx].nkey = bt->native + idx * bt->type->sizeof_nkey;
    if ((bt->type->decode) (f, bt, bt->key[idx].rkey,
                            bt->key[idx].nkey) < 0) {
        HRETURN_ERROR(H5E_BTREE, H5E_CANTDECODE, FAIL,
                      "unable to decode key");
    }
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5B_decode_keys
 *
 * Purpose:     Decode keys on either side of the specified branch.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Tuesday, October 14, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5B_decode_keys(H5F_t *f, H5B_t *bt, int idx)
{
    FUNC_ENTER(H5B_decode_keys, FAIL);

    assert(f);
    assert(bt);
    assert(idx >= 0 && idx < bt->nchildren);

    if (!bt->key[idx].nkey && H5B_decode_key(f, bt, idx) < 0) {
        HRETURN_ERROR(H5E_BTREE, H5E_CANTDECODE, FAIL,
                      "unable to decode key");
    }
    if (!bt->key[idx+1].nkey && H5B_decode_key(f, bt, idx+1) < 0) {
        HRETURN_ERROR(H5E_BTREE, H5E_CANTDECODE, FAIL,
                      "unable to decode key");
    }
    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5B_insert
 *
 * Purpose:     Adds a new item to the B-tree.  If the root node of
 *              the B-tree splits then the B-tree gets a new address.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jun 23 1997
 *
 * Modifications:
 *      Robb Matzke, 28 Sep 1998
 *      The optional SPLIT_RATIOS[] indicates what percent of the child
 *      pointers should go in the left node when a node splits.  There are
 *      three possibilities and a separate split ratio can be specified for
 *      each: [0] The node that split is the left-most node at its level of
 *      the tree, [1] the node that split has left and right siblings, [2]
 *      the node that split is the right-most node at its level of the tree.
 *      When a node is an only node at its level then we use the right-most
 *      rule.  If SPLIT_RATIOS is null then default values are used.
 *
 *      Robb Matzke, 1999-07-28
 *      The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5B_insert(H5F_t *f, const H5B_class_t *type, haddr_t addr,
           const double split_ratios[], void *udata)
{
    /*
     * These are defined this way to satisfy alignment constraints.
     */
    uint64_t    _lt_key[128], _md_key[128], _rt_key[128];
    uint8_t     *lt_key=(uint8_t*)_lt_key;
    uint8_t     *md_key=(uint8_t*)_md_key;
    uint8_t     *rt_key=(uint8_t*)_rt_key;

    hbool_t     lt_key_changed = FALSE, rt_key_changed = FALSE;
    haddr_t     child, old_root;
    int level;
    H5B_t       *bt;
    hsize_t     size;
    H5B_ins_t   my_ins = H5B_INS_ERROR;
    herr_t      ret_value = FAIL;

    FUNC_ENTER(H5B_insert, FAIL);

    /*
     * Check arguments.
     */
    assert(f);
    assert(type);
    assert(type->sizeof_nkey <= sizeof _lt_key);
    assert(H5F_addr_defined(addr));

    if ((my_ins = H5B_insert_helper(f, addr, type, split_ratios, lt_key,
                                    &lt_key_changed, md_key, udata, rt_key,
                                    &rt_key_changed, &child/*out*/))<0 ||
        my_ins<0) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, FAIL,
                    "unable to insert key");
    }
    if (H5B_INS_NOOP == my_ins) HRETURN(SUCCEED);
    assert(H5B_INS_RIGHT == my_ins);

    /* the current root */
    if (NULL == (bt = H5AC_find(f, H5AC_BT, addr, type, udata))) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL,
                    "unable to locate root of B-tree");
    }
    level = bt->level;
    if (!lt_key_changed) {
        if (!bt->key[0].nkey && H5B_decode_key(f, bt, 0) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, FAIL,
                        "unable to decode key");
        }
        HDmemcpy(lt_key, bt->key[0].nkey, type->sizeof_nkey);
    }
    
    /* the new node */
    if (NULL == (bt = H5AC_find(f, H5AC_BT, child, type, udata))) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL,
                    "unable to load new node");
    }
    if (!rt_key_changed) {
        if (!bt->key[bt->nchildren].nkey &&
            H5B_decode_key(f, bt, bt->nchildren) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, FAIL,
                        "unable to decode key");
        }
        HDmemcpy(rt_key, bt->key[bt->nchildren].nkey, type->sizeof_nkey);
    }
    
    /*
     * Copy the old root node to some other file location and make the new
     * root at the old root's previous address.  This prevents the B-tree
     * from "moving".
     */
    size = H5B_nodesize(f, type, NULL, bt->sizeof_rkey);
    if (HADDR_UNDEF==(old_root=H5MF_alloc(f, H5FD_MEM_BTREE, size))) {
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                    "unable to allocate file space to move root");
    }

    /* update the new child's left pointer */
    if (NULL == (bt = H5AC_find(f, H5AC_BT, child, type, udata))) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL,
                    "unable to load new child");
    }
    bt->dirty = TRUE;
    bt->left = old_root;

    /*
     * Move the node to the new location by checking it out & checking it in
     * at the new location -QAK
     */
    /* Bring the old root into the cache if it's not already */
    if (NULL == (bt = H5AC_find(f, H5AC_BT, addr, type, udata))) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL,
                    "unable to load new child");
    }

    /* Make certain the old root info is marked as dirty before moving it, */
    /* so it is certain to be written out at the new location */
    bt->dirty = TRUE;

    /* Make a copy of the old root information */
    if (NULL == (bt = H5B_copy(f, bt))) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL,
                    "unable to copy old root");
    }

    /* Move the location on the disk */
    if (H5AC_rename(f, H5AC_BT, addr, old_root) < 0) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTSPLIT, FAIL,
                    "unable to move B-tree root node");
    }

    /* Insert the copy of the old root into the file again */
    if (H5AC_set(f, H5AC_BT, addr, bt) < 0) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTFLUSH, FAIL,
                    "unable to flush old B-tree root node");
    }

    /* clear the old root info at the old address (we already copied it) */
    bt->dirty = TRUE;
    bt->left = HADDR_UNDEF;
    bt->right = HADDR_UNDEF;

    /* Set the new information for the copy */
    bt->ndirty = 2;
    bt->level = level + 1;
    bt->nchildren = 2;

    bt->child[0] = old_root;
    bt->key[0].dirty = TRUE;
    bt->key[0].nkey = bt->native;
    HDmemcpy(bt->key[0].nkey, lt_key, type->sizeof_nkey);

    bt->child[1] = child;
    bt->key[1].dirty = TRUE;
    bt->key[1].nkey = bt->native + type->sizeof_nkey;
    HDmemcpy(bt->key[1].nkey, md_key, type->sizeof_nkey);

    bt->key[2].dirty = TRUE;
    bt->key[2].nkey = bt->native + 2 * type->sizeof_nkey;
    HDmemcpy(bt->key[2].nkey, rt_key, type->sizeof_nkey);

#ifdef H5B_DEBUG
    H5B_assert(f, addr, type, udata);
#endif
    ret_value = SUCCEED;
    
 done:
    FUNC_LEAVE(ret_value);
}
    

/*-------------------------------------------------------------------------
 * Function:    H5B_insert_child
 *
 * Purpose:     Insert a child to the left or right of child[IDX] depending
 *              on whether ANCHOR is H5B_INS_LEFT or H5B_INS_RIGHT. The BT
 *              argument is a pointer to a protected B-tree node.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  8 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The CHILD argument is passed by value.
 *-------------------------------------------------------------------------
 */
static herr_t
H5B_insert_child(H5F_t *f, const H5B_class_t *type, H5B_t *bt,
                 int idx, haddr_t child, H5B_ins_t anchor, void *md_key)
{
    size_t      recsize;
    int i;

    FUNC_ENTER(H5B_insert_child, FAIL);
    assert(bt);
    assert(bt->nchildren<2*H5B_K(f, type));

    bt->dirty = TRUE;
    recsize = bt->sizeof_rkey + H5F_SIZEOF_ADDR(f);

    if (H5B_INS_RIGHT == anchor) {
        /*
         * The MD_KEY is the left key of the new node.
         */
        idx++;
        
        HDmemmove(bt->page + H5B_SIZEOF_HDR(f) + (idx+1) * recsize,
                  bt->page + H5B_SIZEOF_HDR(f) + idx * recsize,
                  (bt->nchildren - idx) * recsize + bt->sizeof_rkey);

        HDmemmove(bt->native + (idx+1) * type->sizeof_nkey,
                  bt->native + idx * type->sizeof_nkey,
                  ((bt->nchildren - idx) + 1) * type->sizeof_nkey);

        for (i=bt->nchildren; i>=idx; --i) {
            bt->key[i+1].dirty = bt->key[i].dirty;
            if (bt->key[i].nkey) {
                bt->key[i+1].nkey = bt->native + (i+1) * type->sizeof_nkey;
            } else {
                bt->key[i+1].nkey = NULL;
            }
        }
        bt->key[idx].dirty = TRUE;
        bt->key[idx].nkey = bt->native + idx * type->sizeof_nkey;
        HDmemcpy(bt->key[idx].nkey, md_key, type->sizeof_nkey);

    } else {
        /*
         * The MD_KEY is the right key of the new node.
         */
        HDmemmove(bt->page + (H5B_SIZEOF_HDR(f) +
                              (idx+1) * recsize + bt->sizeof_rkey),
                  bt->page + (H5B_SIZEOF_HDR(f) +
                              idx * recsize + bt->sizeof_rkey),
                  (bt->nchildren - idx) * recsize);

        HDmemmove(bt->native + (idx+2) * type->sizeof_nkey,
                  bt->native + (idx+1) * type->sizeof_nkey,
                  (bt->nchildren - idx) * type->sizeof_nkey);

        for (i = bt->nchildren; i > idx; --i) {
            bt->key[i+1].dirty = bt->key[i].dirty;
            if (bt->key[i].nkey) {
                bt->key[i+1].nkey = bt->native + (i+1) * type->sizeof_nkey;
            } else {
                bt->key[i+1].nkey = NULL;
            }
        }
        bt->key[idx+1].dirty = TRUE;
        bt->key[idx+1].nkey = bt->native + (idx+1) * type->sizeof_nkey;
        HDmemcpy(bt->key[idx+1].nkey, md_key, type->sizeof_nkey);
    }

    HDmemmove(bt->child + idx + 1,
              bt->child + idx,
              (bt->nchildren - idx) * sizeof(haddr_t));

    bt->child[idx] = child;
    bt->nchildren += 1;
    bt->ndirty = bt->nchildren;

    FUNC_LEAVE(SUCCEED);
}

/*-------------------------------------------------------------------------
 * Function:    H5B_insert_helper
 *
 * Purpose:     Inserts the item UDATA into the tree rooted at ADDR and having
 *              the specified type.
 *
 *              On return, if LT_KEY_CHANGED is non-zero, then LT_KEY is
 *              the new native left key.  Similarily for RT_KEY_CHANGED
 *              and RT_KEY.
 *
 *              If the node splits, then MD_KEY contains the key that
 *              was split between the two nodes (that is, the key that
 *              appears as the max key in the left node and the min key
 *              in the right node).
 *
 * Return:      Success:        A B-tree operation.  The address of the new
 *                              node, if the node splits, is returned through
 *                              the NEW_NODE_P argument. The new node is always
 *                              to the right of the previous node.  This
 *                              function is called recursively and the return
 *                              value influences the behavior of the caller.
 *                              See also, declaration of H5B_ins_t.
 *
 *              Failure:        H5B_INS_ERROR
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  9 1997
 *
 * Modifications:
 *
 *      Robb Matzke, 28 Sep 1998
 *      The optional SPLIT_RATIOS[] indicates what percent of the child
 *      pointers should go in the left node when a node splits.  There are
 *      three possibilities and a separate split ratio can be specified for
 *      each: [0] The node that split is the left-most node at its level of
 *      the tree, [1] the node that split has left and right siblings, [2]
 *      the node that split is the right-most node at its level of the tree.
 *      When a node is an only node at its level then we use the right-most
 *      rule.  If SPLIT_RATIOS is null then default values are used.
 *
 *      Robb Matzke, 1999-07-28
 *      The ADDR argument is passed by value. The NEW_NODE argument is
 *      renamed NEW_NODE_P
 *-------------------------------------------------------------------------
 */
static H5B_ins_t
H5B_insert_helper(H5F_t *f, haddr_t addr, const H5B_class_t *type,
                  const double split_ratios[], uint8_t *lt_key,
                  hbool_t *lt_key_changed, uint8_t *md_key, void *udata,
                  uint8_t *rt_key, hbool_t *rt_key_changed,
                  haddr_t *new_node_p/*out*/)
{
    H5B_t       *bt = NULL, *twin = NULL, *tmp_bt = NULL;
    int lt = 0, idx = -1, rt, cmp = -1;
    haddr_t     child_addr;
    H5B_ins_t   my_ins = H5B_INS_ERROR;
    H5B_ins_t   ret_value = H5B_INS_ERROR;

    FUNC_ENTER(H5B_insert_helper, H5B_INS_ERROR);

    /*
     * Check arguments
     */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(type);
    assert(type->decode);
    assert(type->cmp3);
    assert(type->new_node);
    assert(lt_key);
    assert(lt_key_changed);
    assert(rt_key);
    assert(rt_key_changed);
    assert(new_node_p);

    *lt_key_changed = FALSE;
    *rt_key_changed = FALSE;

    /*
     * Use a binary search to find the child that will receive the new
     * data.  When the search completes IDX points to the child that
     * should get the new data.
     */
    if (NULL == (bt = H5AC_protect(f, H5AC_BT, addr, type, udata))) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, H5B_INS_ERROR,
                    "unable to load node");
    }
    rt = bt->nchildren;

    while (lt < rt && cmp) {
        idx = (lt + rt) / 2;
        if (H5B_decode_keys(f, bt, idx) < 0) {
            HRETURN_ERROR(H5E_BTREE, H5E_CANTDECODE, H5B_INS_ERROR,
                          "unable to decode key");
        }
        if ((cmp = (type->cmp3) (f, bt->key[idx].nkey, udata,
                                 bt->key[idx+1].nkey)) < 0) {
            rt = idx;
        } else {
            lt = idx + 1;
        }
    }

    if (0 == bt->nchildren) {
        /*
         * The value being inserted will be the only value in this tree. We
         * must necessarily be at level zero.
         */
        assert(0 == bt->level);
        bt->key[0].nkey = bt->native;
        bt->key[1].nkey = bt->native + type->sizeof_nkey;
        if ((type->new_node)(f, H5B_INS_FIRST, bt->key[0].nkey, udata,
                             bt->key[1].nkey, bt->child + 0/*out*/) < 0) {
            bt->key[0].nkey = bt->key[1].nkey = NULL;
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, H5B_INS_ERROR,
                        "unable to create leaf node");
        }
        bt->nchildren = 1;
        bt->dirty = TRUE;
        bt->ndirty = 1;
        bt->key[0].dirty = TRUE;
        bt->key[1].dirty = TRUE;
        idx = 0;

        if (type->follow_min) {
            if ((my_ins = (type->insert)(f, bt->child[idx], bt->key[idx].nkey,
                                         lt_key_changed, md_key, udata,
                                         bt->key[idx+1].nkey, rt_key_changed,
                                         &child_addr/*out*/)) < 0) {
                HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, H5B_INS_ERROR,
                            "unable to insert first leaf node");
            }
        } else {
            my_ins = H5B_INS_NOOP;
        }

    } else if (cmp < 0 && idx <= 0 && bt->level > 0) {
        /*
         * The value being inserted is less than any value in this tree.
         * Follow the minimum branch out of this node to a subtree.
         */
        idx = 0;
        if (H5B_decode_keys(f, bt, idx) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, H5B_INS_ERROR,
                        "unable to decode key");
        }
        if ((my_ins = H5B_insert_helper(f, bt->child[idx], type, split_ratios,
                                        bt->key[idx].nkey, lt_key_changed,
                                        md_key, udata, bt->key[idx+1].nkey,
                                        rt_key_changed,
                                        &child_addr/*out*/))<0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, H5B_INS_ERROR,
                        "can't insert minimum subtree");
        }
    } else if (cmp < 0 && idx <= 0 && type->follow_min) {
        /*
         * The value being inserted is less than any leaf node out of this
         * current node.  Follow the minimum branch to a leaf node and let the
         * subclass handle the problem.
         */
        idx = 0;
        if (H5B_decode_keys(f, bt, idx) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, H5B_INS_ERROR,
                        "unable to decode key");
        }
        if ((my_ins = (type->insert)(f, bt->child[idx], bt->key[idx].nkey,
                                     lt_key_changed, md_key, udata,
                                     bt->key[idx+1].nkey, rt_key_changed,
                                     &child_addr/*out*/)) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, H5B_INS_ERROR,
                        "can't insert minimum leaf node");
        }
    } else if (cmp < 0 && idx <= 0) {
        /*
         * The value being inserted is less than any leaf node out of the
         * current node. Create a new minimum leaf node out of this B-tree
         * node. This node is not empty (handled above).
         */
        idx = 0;
        if (H5B_decode_keys(f, bt, idx) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, H5B_INS_ERROR,
                        "unable to decode key");
        }
        my_ins = H5B_INS_LEFT;
        HDmemcpy(md_key, bt->key[idx].nkey, type->sizeof_nkey);
        if ((type->new_node)(f, H5B_INS_LEFT, bt->key[idx].nkey, udata,
                             md_key, &child_addr/*out*/) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, H5B_INS_ERROR,
                        "can't insert minimum leaf node");
        }
        *lt_key_changed = TRUE;

    } else if (cmp > 0 && idx + 1 >= bt->nchildren && bt->level > 0) {
        /*
         * The value being inserted is larger than any value in this tree.
         * Follow the maximum branch out of this node to a subtree.
         */
        idx = bt->nchildren - 1;
        if (H5B_decode_keys(f, bt, idx) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, H5B_INS_ERROR,
                        "unable to decode key");
        }
        if ((my_ins = H5B_insert_helper(f, bt->child[idx], type, split_ratios,
                                        bt->key[idx].nkey, lt_key_changed,
                                        md_key, udata, bt->key[idx+1].nkey,
                                        rt_key_changed,
                                        &child_addr/*out*/)) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, H5B_INS_ERROR,
                        "can't insert maximum subtree");
        }
    } else if (cmp > 0 && idx + 1 >= bt->nchildren && type->follow_max) {
        /*
         * The value being inserted is larger than any leaf node out of the
         * current node.  Follow the maximum branch to a leaf node and let the
         * subclass handle the problem.
         */
        idx = bt->nchildren - 1;
        if (H5B_decode_keys(f, bt, idx) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, H5B_INS_ERROR,
                        "unable to decode key");
        }
        if ((my_ins = (type->insert)(f, bt->child[idx], bt->key[idx].nkey,
                                     lt_key_changed, md_key, udata,
                                     bt->key[idx+1].nkey, rt_key_changed,
                                     &child_addr/*out*/)) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, H5B_INS_ERROR,
                        "can't insert maximum leaf node");
        }
    } else if (cmp > 0 && idx + 1 >= bt->nchildren) {
        /*
         * The value being inserted is larger than any leaf node out of the
         * current node.  Create a new maximum leaf node out of this B-tree
         * node.
         */
        idx = bt->nchildren - 1;
        if (H5B_decode_keys(f, bt, idx) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, H5B_INS_ERROR,
                        "unable to decode key");
        }
        my_ins = H5B_INS_RIGHT;
        HDmemcpy(md_key, bt->key[idx+1].nkey, type->sizeof_nkey);
        if ((type->new_node)(f, H5B_INS_RIGHT, md_key, udata,
                             bt->key[idx+1].nkey, &child_addr/*out*/) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, H5B_INS_ERROR,
                        "can't insert maximum leaf node");
        }
        *rt_key_changed = TRUE;

    } else if (cmp) {
        /*
         * We couldn't figure out which branch to follow out of this node. THIS
         * IS A MAJOR PROBLEM THAT NEEDS TO BE FIXED --rpm.
         */
        assert("INTERNAL HDF5 ERROR (contact rpm)" && 0);
        HDabort();

    } else if (bt->level > 0) {
        /*
         * Follow a branch out of this node to another subtree.
         */
        assert(idx >= 0 && idx < bt->nchildren);
        if ((my_ins = H5B_insert_helper(f, bt->child[idx], type, split_ratios,
                                        bt->key[idx].nkey, lt_key_changed,
                                        md_key, udata,
                                        bt->key[idx+1].nkey, rt_key_changed,
                                        &child_addr/*out*/)) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, H5B_INS_ERROR,
                        "can't insert subtree");
        }
    } else {
        /*
         * Follow a branch out of this node to a leaf node of some other type.
         */
        assert(idx >= 0 && idx < bt->nchildren);
        if ((my_ins = (type->insert)(f, bt->child[idx], bt->key[idx].nkey,
                                      lt_key_changed, md_key, udata,
                                      bt->key[idx+1].nkey, rt_key_changed,
                                      &child_addr/*out*/)) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, H5B_INS_ERROR,
                        "can't insert leaf node");
        }
    }
    assert(my_ins >= 0);

    /*
     * Update the left and right keys of the current node.
     */
    if (*lt_key_changed) {
        bt->dirty = TRUE;
        bt->key[idx].dirty = TRUE;
        if (idx > 0) {
            *lt_key_changed = FALSE;
        } else {
            HDmemcpy(lt_key, bt->key[idx].nkey, type->sizeof_nkey);
        }
    }
    if (*rt_key_changed) {
        bt->dirty = TRUE;
        bt->key[idx+1].dirty = TRUE;
        if (idx+1 < bt->nchildren) {
            *rt_key_changed = FALSE;
        } else {
            HDmemcpy(rt_key, bt->key[idx+1].nkey, type->sizeof_nkey);
        }
    }
    if (H5B_INS_CHANGE == my_ins) {
        /*
         * The insertion simply changed the address for the child.
         */
        bt->child[idx] = child_addr;
        bt->dirty = TRUE;
        bt->ndirty = MAX(bt->ndirty, idx+1);
        ret_value = H5B_INS_NOOP;

    } else if (H5B_INS_LEFT == my_ins || H5B_INS_RIGHT == my_ins) {
        /*
         * If this node is full then split it before inserting the new child.
         */
        if (bt->nchildren == 2 * H5B_K(f, type)) {
            if (H5B_split(f, type, bt, addr, idx, split_ratios, udata,
                          new_node_p/*out*/)<0) {
                HGOTO_ERROR(H5E_BTREE, H5E_CANTSPLIT, H5B_INS_ERROR,
                            "unable to split node");
            }
            if (NULL == (twin = H5AC_protect(f, H5AC_BT, *new_node_p, type,
                                             udata))) {
                HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, H5B_INS_ERROR,
                            "unable to load node");
            }
            if (idx<bt->nchildren) {
                tmp_bt = bt;
            } else {
                idx -= bt->nchildren;
                tmp_bt = twin;
            }
        } else {
            tmp_bt = bt;
        }

        /* Insert the child */
        if (H5B_insert_child(f, type, tmp_bt, idx, child_addr, my_ins,
                             md_key) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTINSERT, H5B_INS_ERROR,
                        "can't insert child");
        }
    }
    
    /*
     * If this node split, return the mid key (the one that is shared
     * by the left and right node).
     */
    if (twin) {
        if (!twin->key[0].nkey && H5B_decode_key(f, twin, 0) < 0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, H5B_INS_ERROR,
                        "unable to decode key");
        }
        HDmemcpy(md_key, twin->key[0].nkey, type->sizeof_nkey);
        ret_value = H5B_INS_RIGHT;
#ifdef H5B_DEBUG
        /*
         * The max key in the original left node must be equal to the min key
         * in the new node.
         */
        if (!bt->key[bt->nchildren].nkey) {
            herr_t status = H5B_decode_key(f, bt, bt->nchildren);
            assert(status >= 0);
        }
        cmp = (type->cmp2) (f, bt->key[bt->nchildren].nkey, udata,
                            twin->key[0].nkey);
        assert(0 == cmp);
#endif
    } else {
        ret_value = H5B_INS_NOOP;
    }

  done:
    {
        herr_t e1 = (bt && H5AC_unprotect(f, H5AC_BT, addr, bt) < 0);
        herr_t e2 = (twin && H5AC_unprotect(f, H5AC_BT, *new_node_p, twin)<0);
        if (e1 || e2) { /*use vars to prevent short-circuit of side effects */
            HRETURN_ERROR(H5E_BTREE, H5E_PROTECT, H5B_INS_ERROR,
                          "unable to release node(s)");
        }
    }

    FUNC_LEAVE(ret_value);
}

/*-------------------------------------------------------------------------
 * Function:    H5B_iterate
 *
 * Purpose:     Calls the list callback for each leaf node of the
 *              B-tree, passing it the UDATA structure.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jun 23 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-04-21
 *              The key values are passed to the function which is called.
 *
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5B_iterate (H5F_t *f, const H5B_class_t *type, haddr_t addr, void *udata)
{
    H5B_t               *bt = NULL;
    haddr_t             next_addr;
    haddr_t             cur_addr = HADDR_UNDEF;
    haddr_t             *child = NULL;
    uint8_t             *key = NULL;
    int         i, nchildren;
    herr_t              ret_value = FAIL;

    FUNC_ENTER(H5B_iterate, FAIL);

    /*
     * Check arguments.
     */
    assert(f);
    assert(type);
    assert(type->list);
    assert(H5F_addr_defined(addr));
    assert(udata);

    if (NULL == (bt=H5AC_find(f, H5AC_BT, addr, type, udata))) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL,
                    "unable to load B-tree node");
    }
    if (bt->level > 0) {
        /* Keep following the left-most child until we reach a leaf node. */
        if ((ret_value=H5B_iterate(f, type, bt->child[0], udata))<0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTLIST, FAIL,
                        "unable to list B-tree node");
        }
    } else {
        /*
         * We've reached the left-most leaf.  Now follow the right-sibling
         * pointer from leaf to leaf until we've processed all leaves.
         */
        if (NULL==(child=H5FL_ARR_ALLOC(haddr_t,(hsize_t)(2*H5B_K(f,type)),0)) ||
            NULL==(key=H5MM_malloc((2*H5B_K(f, type)+1)*type->sizeof_nkey))) {
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                         "memory allocation failed");
        }
        for (cur_addr=addr, ret_value=0;
             H5F_addr_defined(cur_addr) && !ret_value;
             cur_addr=next_addr) {

            /*
             * Save all the child addresses and native keys since we can't
             * leave the B-tree node protected during an application
             * callback.
             */
            if (NULL==(bt=H5AC_find (f, H5AC_BT, cur_addr, type, udata))) {
                HGOTO_ERROR (H5E_BTREE, H5E_CANTLOAD, FAIL, "B-tree node");
            }
            for (i=0; i<bt->nchildren; i++) {
                child[i] = bt->child[i];
            }
            for (i=0; i<bt->nchildren+1; i++) {
                if (!bt->key[i].nkey) H5B_decode_key(f, bt, i);
                memcpy(key+i*type->sizeof_nkey, bt->key[i].nkey,
                       type->sizeof_nkey);
            }
            next_addr = bt->right;
            nchildren = bt->nchildren;
            bt = NULL;

            /*
             * Perform the iteration operator, which might invoke an
             * application  callback.
             */
            for (i=0, ret_value=0; i<nchildren && !ret_value; i++) {
                ret_value = (type->list)(f, key+i*type->sizeof_nkey,
                                         child[i], key+(i+1)*type->sizeof_nkey,
                                         udata);
                if (ret_value<0) {
                    HGOTO_ERROR(H5E_BTREE, H5E_CANTINIT, FAIL,
                                "iterator function failed");
                }
            }
        }
    }

done:
    if(child!=NULL)
        H5FL_ARR_FREE(haddr_t,child);
    if(key!=NULL)
        H5MM_xfree(key);
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5B_remove_helper
 *
 * Purpose:     The recursive part of removing an item from a B-tree.  The
 *              sub B-tree that is being considered is located at ADDR and
 *              the item to remove is described by UDATA.  If the removed
 *              item falls at the left or right end of the current level then
 *              it might be necessary to adjust the left and/or right keys
 *              (LT_KEY and/or RT_KEY) to to indicate that they changed by
 *              setting LT_KEY_CHANGED and/or RT_KEY_CHANGED.
 *
 * Return:      Success:        A B-tree operation, see comments for
 *                              H5B_ins_t declaration.  This function is
 *                              called recursively and the return value
 *                              influences the actions of the caller. It is
 *                              also called by H5B_remove().
 *
 *              Failure:        H5B_INS_ERROR, a negative value.
 *
 * Programmer:  Robb Matzke
 *              Wednesday, September 16, 1998
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
static H5B_ins_t
H5B_remove_helper(H5F_t *f, haddr_t addr, const H5B_class_t *type,
                  int level, uint8_t *lt_key/*out*/,
                  hbool_t *lt_key_changed/*out*/, void *udata,
                  uint8_t *rt_key/*out*/, hbool_t *rt_key_changed/*out*/)
{
    H5B_t       *bt = NULL, *sibling = NULL;
    H5B_ins_t   ret_value = H5B_INS_ERROR;
    int idx=-1, lt=0, rt, cmp=1, i;
    size_t      sizeof_rkey, sizeof_rec;
    hsize_t     sizeof_node;
    
    FUNC_ENTER(H5B_remove_helper, H5B_INS_ERROR);
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(type);
    assert(type->decode);
    assert(type->cmp3);
    assert(type->found);
    assert(lt_key && lt_key_changed);
    assert(udata);
    assert(rt_key && rt_key_changed);

    /*
     * Perform a binary search to locate the child which contains the thing
     * for which we're searching.
     */
    if (NULL==(bt=H5AC_protect(f, H5AC_BT, addr, type, udata))) {
        HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, H5B_INS_ERROR,
                    "unable to load B-tree node");
    }
    rt = bt->nchildren;
    while (lt<rt && cmp) {
        idx = (lt+rt)/2;
        if (H5B_decode_keys(f, bt, idx)<0) {
            HGOTO_ERROR(H5E_BTREE, H5E_CANTDECODE, H5B_INS_ERROR,
                        "unable to decode B-tree key(s)");
        }
        if ((cmp=(type->cmp3)(f, bt->key[idx].nkey, udata,
                              bt->key[idx+1].nkey))<0) {
            rt = idx;
        } else {
            lt = idx+1;
        }
    }
    if (cmp) {
        HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, H5B_INS_ERROR,
                    "B-tree key not found");
    }

    /*
     * Follow the link to the subtree or to the data node.  The return value
     * will be one of H5B_INS_ERROR, H5B_INS_NOOP, or H5B_INS_REMOVE.
     */
    assert(idx>=0 && idx<bt->nchildren);
    if (bt->level>0) {
        /* We're at an internal node -- call recursively */
        if ((ret_value=H5B_remove_helper(f,
                                         bt->child[idx],
                                         type,
                                         level+1,
                                         bt->key[idx].nkey/*out*/,
                                         lt_key_changed/*out*/,
                                         udata,
                                         bt->key[idx+1].nkey/*out*/,
                                         rt_key_changed/*out*/))<0) {
            HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, H5B_INS_ERROR,
                        "key not found in subtree");
        }
    } else if (type->remove) {
        /*
         * We're at a leave node but the leave node points to an object that
         * has a removal method.  Pass the removal request to the pointed-to
         * object and let it decide how to progress.
         */
        if ((ret_value=(type->remove)(f,
                                      bt->child[idx],
                                      bt->key[idx].nkey,
                                      lt_key_changed,
                                      udata,
                                      bt->key[idx+1].nkey,
                                      rt_key_changed))<0) {
            HGOTO_ERROR(H5E_BTREE, H5E_NOTFOUND, H5B_INS_ERROR,
                        "key not found in leaf node");
        }
    } else {
        /*
         * We're at a leaf node which points to an object that has no removal
         * method.  The best we can do is to leave the object alone but
         * remove the B-tree reference to the object.
         */
        *lt_key_changed = FALSE;
        *rt_key_changed = FALSE;
        ret_value = H5B_INS_REMOVE;
    }

    /*
     * Update left and right key dirty bits if the subtree indicates that they
     * have changed.  If the subtree's left key changed and the subtree is the
     * left-most child of the current node then we must update the key in our
     * parent and indicate that it changed.  Similarly, if the rigt subtree
     * key changed and it's the right most key of this node we must update
     * our right key and indicate that it changed.
     */
    if (*lt_key_changed) {
        bt->dirty = TRUE;
        bt->key[idx].dirty = TRUE;
        if (idx>0) {
            *lt_key_changed = FALSE;
        } else {
            HDmemcpy(lt_key, bt->key[idx].nkey, type->sizeof_nkey);
        }
    }
    if (*rt_key_changed) {
        bt->dirty = TRUE;
        bt->key[idx+1].dirty = TRUE;
        if (idx+1<bt->nchildren) {
            *rt_key_changed = FALSE;
        } else {
            HDmemcpy(rt_key, bt->key[idx+1].nkey, type->sizeof_nkey);
        }
    }

    /*
     * If the subtree returned H5B_INS_REMOVE then we should remove the
     * subtree entry from the current node.  There are four cases:
     */
    sizeof_rec = bt->sizeof_rkey + H5F_SIZEOF_ADDR(f);
    if (H5B_INS_REMOVE==ret_value && 1==bt->nchildren) {
        /*
         * The subtree is the only child of this node.  Discard both
         * keys and the subtree pointer. Free this node (unless it's the
         * root node) and return H5B_INS_REMOVE.
         */
        bt->dirty = TRUE;
        bt->nchildren = 0;
        bt->ndirty = 0;
        if (level>0) {
            if (H5F_addr_defined(bt->left)) {
                if (NULL==(sibling=H5AC_find(f, H5AC_BT, bt->left, type,
                                             udata))) {
                    HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, H5B_INS_ERROR,
                                "unable to unlink node from tree");
                }
                sibling->right = bt->right;
                sibling->dirty = TRUE;
            }
            if (H5F_addr_defined(bt->right)) {
                if (NULL==(sibling=H5AC_find(f, H5AC_BT, bt->right, type,
                                             udata))) {
                    HGOTO_ERROR(H5E_BTREE, H5E_CANTLOAD, H5B_INS_ERROR,
                                "unable to unlink node from tree");
                }
                sibling->left = bt->left;
                sibling->dirty = TRUE;
            }
            bt->left = HADDR_UNDEF;
            bt->right = HADDR_UNDEF;
            sizeof_rkey = (type->get_sizeof_rkey)(f, udata);
            sizeof_node = H5B_nodesize(f, type, NULL, sizeof_rkey);
            if (H5AC_unprotect(f, H5AC_BT, addr, bt)<0 ||
                H5AC_flush(f, H5AC_BT, addr, TRUE)<0 ||
                H5MF_xfree(f, H5FD_MEM_BTREE, addr, sizeof_node)<0) {
                bt = NULL;
                HGOTO_ERROR(H5E_BTREE, H5E_PROTECT, H5B_INS_ERROR,
                            "unable to free B-tree node");
            }
            bt = NULL;
        }

    } else if (H5B_INS_REMOVE==ret_value && 0==idx) {
        /*
         * The subtree is the left-most child of this node. We discard the
         * left-most key and the left-most child (the child has already been
         * freed) and shift everything down by one.  We copy the new left-most
         * key into lt_key and notify the caller that the left key has
         * changed.  Return H5B_INS_NOOP.
         */
        bt->dirty = TRUE;
        bt->nchildren -= 1;
        bt->ndirty = bt->nchildren;
        
        HDmemmove(bt->page+H5B_SIZEOF_HDR(f),
                  bt->page+H5B_SIZEOF_HDR(f)+sizeof_rec,
                  bt->nchildren*sizeof_rec + bt->sizeof_rkey);
        HDmemmove(bt->native,
                  bt->native + type->sizeof_nkey,
                  (bt->nchildren+1) * type->sizeof_nkey);
        HDmemmove(bt->child,
                  bt->child+1,
                  bt->nchildren * sizeof(haddr_t));
        for (i=0; i<bt->nchildren; i++) {
            bt->key[i].dirty = bt->key[i+1].dirty;
            if (bt->key[i+1].nkey) {
                bt->key[i].nkey = bt->native + i*type->sizeof_nkey;
            } else {
                bt->key[i].nkey = NULL;
            }
        }
        assert(bt->key[0].nkey);
        HDmemcpy(lt_key, bt->key[0].nkey, type->sizeof_nkey);
        *lt_key_changed = TRUE;
        ret_value = H5B_INS_NOOP;

    } else if (H5B_INS_REMOVE==ret_value && idx+1==bt->nchildren) {
        /*
         * The subtree is the right-most child of this node.  We discard the
         * right-most key and the right-most child (the child has already been
         * freed).  We copy the new right-most key into rt_key and notify the
         * caller that the right key has changed.  Return H5B_INS_NOOP.
         */
        bt->dirty = TRUE;
        bt->nchildren -= 1;
        bt->ndirty = MIN(bt->ndirty, bt->nchildren);
        assert(bt->key[bt->nchildren].nkey);
        HDmemcpy(rt_key, bt->key[bt->nchildren].nkey, type->sizeof_nkey);
        *rt_key_changed = TRUE;
        ret_value = H5B_INS_NOOP;

    } else if (H5B_INS_REMOVE==ret_value) {
        /*
         * There are subtrees out of this node to both the left and right of
         * the subtree being removed.  The key to the left of the subtree and
         * the subtree are removed from this node and all keys and nodes to
         * the right are shifted left by one place.  The subtree has already
         * been freed). Return H5B_INS_NOOP.
         */
        bt->dirty = TRUE;
        bt->nchildren -= 1;
        bt->ndirty = bt->nchildren;
        
        HDmemmove(bt->page+H5B_SIZEOF_HDR(f)+idx*sizeof_rec,
                  bt->page+H5B_SIZEOF_HDR(f)+(idx+1)*sizeof_rec,
                  (bt->nchildren-idx)*sizeof_rec + bt->sizeof_rkey);
        HDmemmove(bt->native + idx * type->sizeof_nkey,
                  bt->native + (idx+1) * type->sizeof_nkey,
                  (bt->nchildren+1-idx) * type->sizeof_nkey);
        HDmemmove(bt->child+idx,
                  bt->child+idx+1,
                  (bt->nchildren-idx) * sizeof(haddr_t));
        for (i=idx; i<bt->nchildren; i++) {
            bt->key[i].dirty = bt->key[i+1].dirty;
            if (bt->key[i+1].nkey) {
                bt->key[i].nkey = bt->native + i*type->sizeof_nkey;
            } else {
                bt->key[i].nkey = NULL;
            }
        }
        ret_value = H5B_INS_NOOP;
        
    } else {
        ret_value = H5B_INS_NOOP;
    }
    
    
 done:
    if (bt && H5AC_unprotect(f, H5AC_BT, addr, bt)<0) {
        HRETURN_ERROR(H5E_BTREE, H5E_PROTECT, H5B_INS_ERROR,
                      "unable to release node");
    }
    FUNC_LEAVE(ret_value);
}



/*-------------------------------------------------------------------------
 * Function:    H5B_remove
 *
 * Purpose:     Removes an item from a B-tree.
 *
 * Note:        The current version does not attempt to rebalance the tree.
 *
 * Return:      Non-negative on success/Negative on failure (failure includes
 *              not being able to find the object which is to be removed).
 *
 * Programmer:  Robb Matzke
 *              Wednesday, September 16, 1998
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5B_remove(H5F_t *f, const H5B_class_t *type, haddr_t addr, void *udata)
{
    /* These are defined this way to satisfy alignment constraints */
    uint64_t    _lt_key[128], _rt_key[128];
    uint8_t     *lt_key = (uint8_t*)_lt_key;    /*left key*/
    uint8_t     *rt_key = (uint8_t*)_rt_key;    /*right key*/
    hbool_t     lt_key_changed = FALSE;         /*left key changed?*/
    hbool_t     rt_key_changed = FALSE;         /*right key changed?*/
    H5B_t       *bt = NULL;                     /*btree node */
    
    
    FUNC_ENTER(H5B_remove, FAIL);

    /* Check args */
    assert(f);
    assert(type);
    assert(type->sizeof_nkey <= sizeof _lt_key);
    assert(H5F_addr_defined(addr));

    /* The actual removal */
    if (H5B_remove_helper(f, addr, type, 0, lt_key, &lt_key_changed,
                          udata, rt_key, &rt_key_changed)==H5B_INS_ERROR) {
        HRETURN_ERROR(H5E_BTREE, H5E_CANTINIT, FAIL,
                      "unable to remove entry from B-tree");
    }

    /*
     * If the B-tree is now empty then make sure we mark the root node as
     * being at level zero
     */
    if (NULL==(bt=H5AC_find(f, H5AC_BT, addr, type, udata))) {
        HRETURN_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL,
                      "unable to load B-tree root node");
    }
    if (0==bt->nchildren && 0!=bt->level) {
        bt->level = 0;
        bt->dirty = TRUE;
    }
    

#ifdef H5B_DEBUG
    H5B_assert(f, addr, type, udata);
#endif
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5B_nodesize
 *
 * Purpose:     Returns the number of bytes needed for this type of
 *              B-tree node.  The size is the size of the header plus
 *              enough space for 2t child pointers and 2t+1 keys.
 *
 *              If TOTAL_NKEY_SIZE is non-null, what it points to will
 *              be initialized with the total number of bytes required to
 *              hold all the key values in native order.
 *
 * Return:      Success:        Size of node in file.
 *
 *              Failure:        0
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Jul  3 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static hsize_t
H5B_nodesize(H5F_t *f, const H5B_class_t *type,
             hsize_t *total_nkey_size/*out*/, size_t sizeof_rkey)
{
    hsize_t     size;

    FUNC_ENTER(H5B_nodesize, (size_t) 0);

    /*
     * Check arguments.
     */
    assert(f);
    assert(type);
    assert(sizeof_rkey > 0);
    assert(H5B_K(f, type) > 0);

    /*
     * Total native key size.
     */
    if (total_nkey_size) {
        *total_nkey_size = (2 * H5B_K(f, type) + 1) * type->sizeof_nkey;
    }
    /*
     * Total node size.
     */
    size = (H5B_SIZEOF_HDR(f) + /*node header   */
            2 * H5B_K(f, type) * H5F_SIZEOF_ADDR(f) +   /*child pointers */
            (2 * H5B_K(f, type) + 1) * sizeof_rkey);    /*keys          */

    FUNC_LEAVE(size);
}


/*-------------------------------------------------------------------------
 * Function:    H5B_copy
 *
 * Purpose:     Deep copies an existing H5B_t node.
 *
 * Return:      Success:        Pointer to H5B_t object.
 *
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              koziol@ncsa.uiuc.edu
 *              Apr 18 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5B_t *
H5B_copy(H5F_t *f, const H5B_t *old_bt)
{
    H5B_t               *ret_value = NULL;
    hsize_t             total_native_keysize;
    hsize_t             size;
    unsigned               nkeys;
    unsigned            u;

    FUNC_ENTER(H5B_copy, NULL);

    /*
     * Check arguments.
     */
    assert(f);
    assert(old_bt);

    /*
     * Get correct sizes 
     */
    size = H5B_nodesize(f, old_bt->type, &total_native_keysize, old_bt->sizeof_rkey);

    /* Allocate memory for the new H5B_t object */
    if (NULL==(ret_value = H5FL_ALLOC(H5B_t,0))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                     "memory allocation failed for B-tree root node");
    }

    /* Copy the main structure */
    HDmemcpy(ret_value,old_bt,sizeof(H5B_t));

    /* Compute the number of keys in this node */
    nkeys=2*H5B_K(f,old_bt->type);

    if (NULL==(ret_value->page=H5FL_BLK_ALLOC(page,size,0)) ||
            NULL==(ret_value->native=H5FL_BLK_ALLOC(native_block,total_native_keysize,0)) ||
            NULL==(ret_value->child=H5FL_ARR_ALLOC(haddr_t,nkeys,0)) ||
            NULL==(ret_value->key=H5FL_ARR_ALLOC(H5B_key_t,(nkeys+1),0))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                     "memory allocation failed for B-tree root node");
    }

    /* Copy the other structures */
    assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
    HDmemcpy(ret_value->page,old_bt->page,(size_t)size);
    assert(total_native_keysize==(hsize_t)((size_t)total_native_keysize)); /*check for overflow*/
    HDmemcpy(ret_value->native,old_bt->native,(size_t)total_native_keysize);
    assert((sizeof(haddr_t)*nkeys)==(hsize_t)((size_t)(sizeof(haddr_t)*nkeys))); /*check for overflow*/
    HDmemcpy(ret_value->child,old_bt->child,(size_t)(sizeof(haddr_t)*nkeys));
    assert((sizeof(H5B_key_t)*(nkeys+1))==(hsize_t)((size_t)(sizeof(H5B_key_t)*(nkeys+1)))); /*check for overflow*/
    HDmemcpy(ret_value->key,old_bt->key,(size_t)(sizeof(H5B_key_t)*(nkeys+1)));

    /*
     * Translate the keys from pointers into the old 'page' buffer into
     *  pointers into the new 'page' buffer.
     */
    for (u = 0; u < (nkeys+1); u++)
        ret_value->key[u].rkey = (old_bt->key[u].rkey - old_bt->page) + ret_value->page;

done:
    FUNC_LEAVE(ret_value);
}   /* H5B_copy */


/*-------------------------------------------------------------------------
 * Function:    H5B_debug
 *
 * Purpose:     Prints debugging info about a B-tree.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug  4 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
herr_t
H5B_debug(H5F_t *f, haddr_t addr, FILE *stream, int indent, int fwidth,
          const H5B_class_t *type, void *udata)
{
    H5B_t       *bt = NULL;
    int         i;

    FUNC_ENTER(H5B_debug, FAIL);

    /*
     * Check arguments.
     */
    assert(f);
    assert(H5F_addr_defined(addr));
    assert(stream);
    assert(indent >= 0);
    assert(fwidth >= 0);
    assert(type);

    /*
     * Load the tree node.
     */
    if (NULL == (bt = H5AC_find(f, H5AC_BT, addr, type, udata))) {
        HRETURN_ERROR(H5E_BTREE, H5E_CANTLOAD, FAIL,
                      "unable to load B-tree node");
    }
    /*
     * Print the values.
     */
    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
              "Tree type ID:",
              ((bt->type->id)==H5B_SNODE_ID ? "H5B_SNODE_ID" :
            ((bt->type->id)==H5B_ISTORE_ID ? "H5B_ISTORE_ID" : "Unknown!")));
    HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
              "Size of node:",
              (unsigned long) H5B_nodesize(f, bt->type, NULL, bt->sizeof_rkey));
    HDfprintf(stream, "%*s%-*s %lu\n", indent, "", fwidth,
              "Size of raw (disk) key:",
              (unsigned long) (bt->sizeof_rkey));
    HDfprintf(stream, "%*s%-*s %s\n", indent, "", fwidth,
              "Dirty flag:",
              bt->dirty ? "True" : "False");
    HDfprintf(stream, "%*s%-*s %d\n", indent, "", fwidth,
              "Number of initial dirty children:",
              (int) (bt->ndirty));
    HDfprintf(stream, "%*s%-*s %d\n", indent, "", fwidth,
              "Level:",
              (int) (bt->level));

    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
              "Address of left sibling:",
              bt->left);

    HDfprintf(stream, "%*s%-*s %a\n", indent, "", fwidth,
              "Address of right sibling:",
              bt->right);

    HDfprintf(stream, "%*s%-*s %d (%d)\n", indent, "", fwidth,
              "Number of children (max):",
              (int) (bt->nchildren),
              (int) (2 * H5B_K(f, type)));

    /*
     * Print the child addresses
     */
    for (i = 0; i < bt->nchildren; i++) {
        HDfprintf(stream, "%*sChild %d...\n", indent, "", i);
        HDfprintf(stream, "%*s%-*s %a\n", indent + 3, "", MAX(0, fwidth - 3),
                  "Address:", bt->child[i]);
        
        H5B_decode_key(f, bt, i);
        if (type->debug_key) {
            (type->debug_key)(stream, indent+3, MAX (0, fwidth-3),
                              bt->key[i].nkey, udata);
        }
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5B_assert
 *
 * Purpose:     Verifies that the tree is structured correctly.
 *
 * Return:      Success:        SUCCEED
 *
 *              Failure:        aborts if something is wrong.
 *
 * Programmer:  Robb Matzke
 *              Tuesday, November  4, 1997
 *
 * Modifications:
 *              Robb Matzke, 1999-07-28
 *              The ADDR argument is passed by value.
 *-------------------------------------------------------------------------
 */
#ifdef H5B_DEBUG
static herr_t
H5B_assert(H5F_t *f, haddr_t addr, const H5B_class_t *type, void *udata)
{
    H5B_t       *bt = NULL;
    int i, ncell, cmp;
    static int  ncalls = 0;
    herr_t      status;

    /* A queue of child data */
    struct child_t {
        haddr_t                 addr;
        int                     level;
        struct child_t         *next;
    } *head = NULL, *tail = NULL, *prev = NULL, *cur = NULL, *tmp = NULL;

    FUNC_ENTER(H5B_assert, FAIL);
    if (0==ncalls++) {
        if (H5DEBUG(B)) {
            fprintf(H5DEBUG(B), "H5B: debugging B-trees (expensive)\n");
        }
    }
    /* Initialize the queue */
    bt = H5AC_find(f, H5AC_BT, addr, type, udata);
    assert(bt);
    cur = H5MM_calloc(sizeof(struct child_t));
    assert (cur);
    cur->addr = addr;
    cur->level = bt->level;
    head = tail = cur;

    /*
     * Do a breadth-first search of the tree.  New nodes are added to the end
     * of the queue as the `cur' pointer is advanced toward the end.  We don't
     * remove any nodes from the queue because we need them in the uniqueness
     * test.
     */
    for (ncell = 0; cur; ncell++) {
        bt = H5AC_protect(f, H5AC_BT, cur->addr, type, udata);
        assert(bt);

        /* Check node header */
        assert(bt->ndirty >= 0 && bt->ndirty <= bt->nchildren);
        assert(bt->level == cur->level);
        if (cur->next && cur->next->level == bt->level) {
            assert(H5F_addr_eq(bt->right, cur->next->addr));
        } else {
            assert(!H5F_addr_defined(bt->right));
        }
        if (prev && prev->level == bt->level) {
            assert(H5F_addr_eq(bt->left, prev->addr));
        } else {
            assert(!H5F_addr_defined(bt->left));
        }

        if (cur->level > 0) {
            for (i = 0; i < bt->nchildren; i++) {

                /*
                 * Check that child nodes haven't already been seen.  If they
                 * have then the tree has a cycle.
                 */
                for (tmp = head; tmp; tmp = tmp->next) {
                    assert(H5F_addr_ne(tmp->addr, bt->child[i]));
                }

                /* Add the child node to the end of the queue */
                tmp = H5MM_calloc(sizeof(struct child_t));
                assert (tmp);
                tmp->addr = bt->child[i];
                tmp->level = bt->level - 1;
                tail->next = tmp;
                tail = tmp;

                /* Check that the keys are monotonically increasing */
                status = H5B_decode_keys(f, bt, i);
                assert(status >= 0);
                cmp = (type->cmp2) (f, bt->key[i].nkey, udata,
                                    bt->key[i+1].nkey);
                assert(cmp < 0);
            }
        }
        /* Release node */
        status = H5AC_unprotect(f, H5AC_BT, cur->addr, bt);
        assert(status >= 0);

        /* Advance current location in queue */
        prev = cur;
        cur = cur->next;
    }

    /* Free all entries from queue */
    while (head) {
        tmp = head->next;
        H5MM_xfree(head);
        head = tmp;
    }

    FUNC_LEAVE(SUCCEED);
}
#endif /* H5B_DEBUG */
