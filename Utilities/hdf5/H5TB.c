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
 *	       Saturday, April 22, 2000
 *
 * Purpose: Routines for using threaded, balanced, binary trees.
 *      Extended from (added threads to) Knuth 6.2.3, Algorithm A (AVL trees)
 *      Basic tree structure by Adel'son-Vel'skii and Landis
 *
 * These routines are designed to allow use of a general-purpose balanced
 * tree implimentation.  These trees are appropriate for maintaining in
 * memory one or more lists of items, each list sorted according to key
 * values (key values must form a "completely ordered set") where no two
 * items in a single list can have the same key value.  The following
 * operations are supported:
 *
 *     Create an empty list
 *     Add an item to a list
 *     Look up an item in a list by key value
 *     Look up the Nth item in a list
 *     Delete an item from a list
 *     Find the first/last/next/previous item in a list
 *     Destroy a list
 *
 * Each of the above operations requires Order(log(N)) time where N is
 * the number of items in the list (except for list creation which
 * requires constant time and list destruction which requires Order(N)
 * time if the user- supplied free-data-item or free-key-value routines
 * require constant time).  Each of the above operations (except create
 * and destroy) can be performed on a subtree.
 *
 * Each node of a tree has associated with it a generic pointer (void *)
 * which is set to point to one such "item" and a generic pointer to
 * point to that item's "key value".  The structure of the items and key
 * values is up to the user to define.  The user must specify a method
 * for comparing key values.  This routine takes three arguments, two
 * pointers to key values and a third integer argument.  You can specify
 * a routine that expects pointers to "data items" rather than key values
 * in which case the pointer to the key value in each node will be set
 * equal to the pointer to the data item.
 *
 * Since the "data item" pointer is the first field of each tree node,
 * these routines may be used without this "tbbt.h" file.  For example,
 * assume "ITM" is the structre definition for the data items you want to
 * store in lists:
 *
 * ITM ***H5TB_dmake( int (*cmp)(void *,void *,int), int arg );
 * ITM **root= NULL;        (* How to create an empty tree w/o H5TB_dmake() *)
 * ITM **H5TB_dfind( ITM ***tree, void *key, ITM ***pp );
 * ITM **H5TB_find( ITM **root, void *key, int (*cmp)(), int arg, ITM ***pp );
 * ITM **H5TB_dless( ITM ***tree, void *key, ITM ***pp );
 * ITM **H5TB_less( ITM **root, void *key, int (*cmp)(), int arg, ITM ***pp );
 * ITM **H5TB_index( ITM **root, long indx );
 * ITM **H5TB_dins( ITM ***tree, ITM *item, void *key );
 * ITM **H5TB_ins( ITM ***root, ITM *item, void *key, int (*cmp)(), int arg );
 * ITM *H5TB_rem( ITM ***root, ITM **node, void **kp );
 * ITM **H5TB_first( ITM **root ), **H5TB_last( ITM **root );
 * ITM **H5TB_next( ITM **node ), **H5TB_prev( ITM **node );
 * ITM ***H5TB_dfree( ITM ***tree, void (*df)(ITM *), void (*kf)(void *) );
 * void H5TB_free( ITM ***root, void (*df)(ITM *), void (*kf)(void *) );
 */

/* Id */

#include "H5private.h"		/*library		  */
#include "H5Eprivate.h"		/*error handling	  */
#include "H5Fprivate.h"        /* File address macros */
#include "H5MMprivate.h"	/*Core memory management	  */
#include "H5FLprivate.h"	/*Free Lists	  */
#include "H5TBprivate.h"    /*Threaded, balanced, binary trees	  */

# define   KEYcmp(k1,k2,a) ((NULL!=compar) ? (*compar)( k1, k2, a) \
                 : HDmemcmp( k1, k2, 0<(a) ? ((size_t)a) : HDstrlen(k1) )  )

/* Return maximum of two scalar values (use arguments w/o side effects): */
#define   Max(a,b)  ( (a) > (b) ? (a) : (b) )

/* Local Function Prototypes */
static H5TB_NODE * H5TB_end(H5TB_NODE * root, int side);
static H5TB_NODE *H5TB_ffind(H5TB_NODE * root, const void * key, unsigned fast_compare,
    H5TB_NODE ** pp);
static herr_t H5TB_balance(H5TB_NODE ** root, H5TB_NODE * ptr, int side, int added);
static H5TB_NODE *H5TB_swapkid(H5TB_NODE ** root, H5TB_NODE * ptr, int side);
static H5TB_NODE *H5TB_nbr(H5TB_NODE * ptr, int side);

#ifdef H5TB_DEBUG
static herr_t H5TB_printNode(H5TB_NODE * node, void(*key_dump)(void *,void *));
static herr_t H5TB_dumpNode(H5TB_NODE *node, void (*key_dump)(void *,void *),
                          int method);
#endif /* H5TB_DEBUG */

/* Declare a free list to manage the H5TB_NODE struct */
H5FL_DEFINE_STATIC(H5TB_NODE);

/* Declare a free list to manage the H5TB_TREE struct */
H5FL_DEFINE_STATIC(H5TB_TREE);

#define PABLO_MASK	H5TB_mask
static int		interface_initialize_g = 0;
#define INTERFACE_INIT	NULL


/*-------------------------------------------------------------------------
 * Function:	H5TB_strcmp
 *
 * Purpose:	Key comparison routine for TBBT routines
 *
 * Return:	same as strcmp()
 *
 * Programmer:	Quincey Koziol
 *              Wednesday, December 4, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5TB_strcmp(const void *k1, const void *k2, int UNUSED cmparg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5TB_strcmp);

    assert(k1);
    assert(k2);

    FUNC_LEAVE_NOAPI(HDstrcmp(k1,k2));
} /* end H5TB_strcmp() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_addr_cmp
 *
 * Purpose:	Key comparison routine for TBBT routines
 *
 * Return:	same as H5F_addr_cmp()
 *
 * Programmer:	Quincey Koziol
 *              Friday, December 20, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5TB_addr_cmp(const void *k1, const void *k2, int UNUSED cmparg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5TB_addr_cmp);

    assert(k1);
    assert(k2);

    FUNC_LEAVE_NOAPI(H5F_addr_cmp(*(const haddr_t *)k1,*(const haddr_t *)k2));
} /* end H5TB_addr_cmp() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_int_cmp
 *
 * Purpose:	Key comparison routine for TBBT routines
 *
 * Return:	same as comparing two integers
 *
 * Programmer:	Quincey Koziol
 *              Friday, December 20, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5TB_int_cmp(const void *k1, const void *k2, int UNUSED cmparg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5TB_int_cmp);

    assert(k1);
    assert(k2);

    FUNC_LEAVE_NOAPI(*(const int *)k1 - *(const int *)k2);
} /* end H5TB_int_cmp() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_hsize_cmp
 *
 * Purpose:	Key comparison routine for TBBT routines
 *
 * Return:	same as comparing two hsize_t's
 *
 * Programmer:	Quincey Koziol
 *              Friday, December 20, 2002
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static int
H5TB_hsize_cmp(const void *k1, const void *k2, int UNUSED cmparg)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5TB_hsize_cmp);

    assert(k1);
    assert(k2);

    FUNC_LEAVE_NOAPI(*(const hsize_t *)k1 - *(const hsize_t *)k2);
} /* end H5TB_hsize_cmp() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_fast_dmake
 *
 * Purpose:	Wrapper around H5TB_dmake for callers which want to use
 *              a "fast comparison" key.
 *
 * Return:	Success:	Pointer to a valid H5TB tree
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Friday, December 20, 2002
 *
 * Modifications:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_TREE  *
H5TB_fast_dmake(unsigned fast_compare)
{
    H5TB_cmp_t  compar;         /* Key comparison function */
    int cmparg;                 /* Key comparison value */
    H5TB_TREE  *ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(H5TB_fast_dmake, NULL);

    /* Get the corret fast comparison routine */
    switch(fast_compare) {
        case H5TB_FAST_HADDR_COMPARE:
            compar=H5TB_addr_cmp;
            cmparg=-1;
            break;

        case H5TB_FAST_INTN_COMPARE:
            compar=H5TB_int_cmp;
            cmparg=-1;
            break;

        case H5TB_FAST_STR_COMPARE:
            compar=H5TB_strcmp;
            cmparg=-1;
            break;

        case H5TB_FAST_HSIZE_COMPARE:
            compar=H5TB_hsize_cmp;
            cmparg=-1;
            break;

        default:
            HGOTO_ERROR (H5E_TBBT, H5E_BADVALUE, NULL, "invalid fast comparison type");
    } /* end switch */

    /* Set return value */
    if((ret_value=H5TB_dmake(compar,cmparg,fast_compare))==NULL)
        HGOTO_ERROR (H5E_TBBT, H5E_CANTCREATE, NULL, "can't create TBBT");

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_fast_dmake() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_dmake
 *
 * Purpose:	Allocates and initializes an empty threaded, balanced, binary tree
 * and returns a pointer to the control structure for it.  You can also create
 * empty trees without this function as long as you never use H5TB_d* routines
 * (H5TB_dfind, H5TB_dins, H5TB_dfree) on them.
 * Examples:
 *     int keycmp();
 *     H5TB_TREE *tree = H5TB_dmake( keycmp, (int)keysiz , 0);
 * or
 *     void *tree= H5TB_dmake( strcmp, 0 , 0);
 * or
 *     void *tree= H5TB_dmake( keycmp, (int)keysiz , H5TB_FAST_HADDR_COMPARE);
 * or
 *     H5TB_NODE *root= NULL;        (* Don't use H5TB_d* routines *)
 *
 * `cmp' is the routine to be used to compare two key values [in H5TB_dfind()
 * and H5TB_dins()].  The arguments to `cmp' are the two keys to compare
 * and `arg':  (*cmp)(k1,k2,arg).  `cmp' is expected to return 0 if its first
 * two arguments point to identical key values, -1 (or any integer less than 0)
 * if k1 points to a key value lower than that pointed to by k2, and 1 (or any
 * integer greater than 0) otherwise.  If `cmp' is NULL, memcmp is used.  If
 * `cmp' is NULL and `arg' is not greater than 0L, `1+strlen(key1)' is used in
 * place of `arg' to emulate strcmp():  memcmp( k1, k2, 1+strlen(k1) ).  You
 * can use strcmp() directly (as in the second example above) as long as your C
 * compiler does not assume strcmp() will always be passed exactly 2 arguments
 * (only newer, ANSI-influenced C compilers are likely to be able to make this
 * kind of assumption).  You can also use a key comparison routine that expects
 * pointers to data items rather than key values.
 *
 * The "fast compare" option is for keys of simple numeric types
 * (currently haddr_t and int) and avoids the function call for faster
 * searches in some cases.  The key comparison routine is still required
 * for some insertion routines which use it.
 *
 * Most of the other routines expect a pointer to a root node of a tree, not
 * a pointer to the tree's control structure (only H5TB_dfind(), H5TB_dins(),
 * and H5TB_dfree() expect pointers to control structures).  However H5TB_TREE
 * is just defined as "**H5TB_NODE" (unless you have defined H5TB_INTERNALS so
 * you have access to the internal structure of the nodes) so
 *     H5TB_TREE *tree1= H5TB_dmake( NULL, 0 );
 * is equivalent to
 *     H5TB_NODE **tree1= H5TB_dmake( NULL, 0 );
 * So could be used as:
 *     node= H5TB_dfind( tree1, key, NULL );
 *     node= H5TB_find( *tree1, key, compar, arg, NULL );
 *     node= H5TB_dless( tree1, key, NULL );
 *     node= H5TB_less( *tree1, key, compar, arg, NULL );
 *     node= H5TB_dins( tree1, item, key );
 *     node= H5TB_ins( tree1, item, key, compar, arg );
 *     item= H5TB_rem( tree1, H5TB_dfind(tree1,key,NULL), NULL );
 *     item= H5TB_rem( tree1, H5TB_find(*tree1,key,compar,arg,NULL), NULL );
 *     tree1= H5TB_dfree( tree1, free, NULL );       (* or whatever *)
 * while
 *     H5TB_NODE *root= NULL;
 * would be used like:
 *     node= H5TB_find( root, key );
 *     node= H5TB_ins( &root, item, key );
 *     node= H5TB_rem( &root, H5TB_find(root,key), NULL );
 *     H5TB_free( &root, free, NULL );               (* or whatever *)
 * Never use H5TB_free() on a tree allocated with H5TB_dmake() or on a sub-tree
 * of ANY tree.  Never use H5TB_dfree() except on a H5TB_dmake()d tree.
 *
 * Return:	Success:	Pointer to a valid H5TB tree
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Saturday, April 22, 2000
 *
 * Modifications:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_TREE  *
H5TB_dmake(H5TB_cmp_t cmp, int arg, unsigned fast_compare)
{
    H5TB_TREE  *tree;
    H5TB_TREE  *ret_value;

    FUNC_ENTER_NOAPI(H5TB_dmake, NULL);

    if (NULL == (tree = H5FL_MALLOC(H5TB_TREE)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

    tree->root = NULL;
    tree->count = 0;
    tree->fast_compare=fast_compare;
    tree->compar = cmp;
    tree->cmparg = arg;

    /* Set return value */
    ret_value=tree;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_dmake() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_dfind
 *
 * Purpose: Look up a node in a "described" tree based on a key value
 * Locate a node based on the key given.  A pointer to the node in the tree
 * with a key value matching `key' is returned.  If no such node exists, NULL
 * is returned.  Whether a node is found or not, if `pp' is not NULL, `*pp'
 * will be set to point to the parent of the node we are looking for (or that
 * node that would be the parent if the node is not found).  H5TB_dfind() is
 * used on trees created using H5TB_dmake() (so that `cmp' and `arg' don't have
 * to be passed).  [H5TB_find() can be used on the root or any subtree of a tree
 * create using H5TB_dmake() and is used on any tree (or subtree) created with-
 * out using H5TB_dmake().]
 *
 * Return:	Success:	Pointer to a valid H5TB node
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Thursday, May 5, 2000
 *
 * Modifications:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_NODE  *
H5TB_dfind(H5TB_TREE * tree, const void * key, H5TB_NODE ** pp)
{
    H5TB_NODE *ret_value;

    FUNC_ENTER_NOAPI(H5TB_dfind, NULL);

    assert(tree);

    if(tree->root)
        if(tree->fast_compare!=0)
            ret_value=H5TB_ffind(tree->root, key, tree->fast_compare, pp);
        else
            ret_value=H5TB_find(tree->root, key, tree->compar, tree->cmparg, pp);
    else {
        if (NULL != pp)
            *pp = NULL;
        ret_value=NULL;
    } /* end else */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_dfind() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_find
 *
 * Purpose: Look up a node in a "non-described" tree based on a key value
 * Locate a node based on the key given.  A pointer to the node in the tree
 * with a key value matching `key' is returned.  If no such node exists, NULL
 * is returned.  Whether a node is found or not, if `pp' is not NULL, `*pp'
 * will be set to point to the parent of the node we are looking for (or that
 * node that would be the parent if the node is not found).  H5TB_dfind() is
 * used on trees created using H5TB_dmake() (so that `cmp' and `arg' don't have
 * to be passed).  [H5TB_find() can be used on the root or any subtree of a tree
 * create using H5TB_dmake() and is used on any tree (or subtree) created with-
 * out using H5TB_dmake().]
 *
 * Return:	Success:	Pointer to a valid H5TB node
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Thursday, May 5, 2000
 *
 * Modifications:
 * 
 * Notes:
 *  H5TB_ffind is based on this routine - fix bugs in both places!
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_NODE  *
H5TB_find(H5TB_NODE * root, const void * key,
     H5TB_cmp_t compar, int arg, H5TB_NODE ** pp)
{
    H5TB_NODE  *ptr = root;
    H5TB_NODE  *parent = NULL;
    int        cmp = 1;
    int        side;
    H5TB_NODE  *ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(H5TB_find, NULL);


    if(ptr) {
        while (0 != (cmp = KEYcmp(key, ptr->key, arg))) {
            parent = ptr;
            side = (cmp < 0) ? LEFT : RIGHT;
            if (!HasChild(ptr, side))
                break;
            ptr = ptr->link[side];
          } /* end while */
    } /* end if */

    if (NULL != pp)
        *pp = parent;

    /* Set return value */
    ret_value= (0 == cmp) ? ptr : NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_find() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_dless
 *
 * Purpose: Look up a node in a "described" tree based on a key value.
 * Locate a node based on the key given.  A pointer to the node in the tree
 * with a key value less than or equal to `key' is returned.  If no such node
 * exists, NULL is returned.  Whether a node is found or not, if `pp' is not
 * NULL, `*pp' will be set to point to the parent of the node we are looking
 * for (or that node that would be the parent if the node is not found).
 * H5TB_dless() is used on trees created using H5TB_dmake() (so that `cmp' and
 * `arg' don't have to be passed).  [H5TB_less() can be used on the root or any
 * subtree of a tree create using H5TB_dmake() and is used on any tree (or
 * subtree) created with-out using H5TB_dmake().]
 *
 * Return:	Success:	Pointer to a valid H5TB node
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Thursday, May 5, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_NODE  *
H5TB_dless(H5TB_TREE * tree, void * key, H5TB_NODE ** pp)
{
    H5TB_NODE *ret_value;       /* Return value */

    FUNC_ENTER_NOAPI(H5TB_dless,NULL);

    assert(tree);

    /* Set return value */
    ret_value= H5TB_less(tree->root, key, tree->compar, tree->cmparg, pp);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_dless() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_less
 *
 * Purpose: Look up a node in a "non-described" tree based on a key value.
 * Locate a node based on the key given.  A pointer to the node in the tree
 * with a key value less than or equal to `key' is returned.  If no such node
 * exists, NULL is returned.  Whether a node is found or not, if `pp' is not
 * NULL, `*pp' will be set to point to the parent of the node we are looking
 * for (or that node that would be the parent if the node is not found).
 * H5TB_dless() is used on trees created using H5TB_dmake() (so that `cmp' and
 * `arg' don't have to be passed).  [H5TB_less() can be used on the root or any
 * subtree of a tree create using H5TB_dmake() and is used on any tree (or
 * subtree) created with-out using H5TB_dmake().]
 *
 * Return:	Success:	Pointer to a valid H5TB node
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Thursday, May 5, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_NODE  *
H5TB_less(H5TB_NODE * root, void * key, H5TB_cmp_t compar, int arg, H5TB_NODE ** pp)
{
    H5TB_NODE  *ptr = root;
    H5TB_NODE  *parent = NULL;
    int        cmp = 1;
    int        side;
    H5TB_NODE  *ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(H5TB_less,NULL);

    /* Try to find an exact match */
    if (ptr) {
        while (0 != (cmp = KEYcmp(key, ptr->key, arg))) {
            parent = ptr;
            side = (cmp < 0) ? LEFT : RIGHT;
            if (!HasChild(ptr, side))
                break;
            ptr = ptr->link[side];
        } /* end while */
    } /* end if */

	/* didn't find an exact match, search back up the tree until a node */
	/* is found with a key less than the key searched for */
    if(cmp!=0) {
        while((ptr=ptr->Parent)!=NULL) {
              cmp = KEYcmp(key, ptr->key, arg);
              if(cmp<0) /* found a node which is less than the search for one */
                  break;
          } /* end while */
        if(ptr==NULL) /* didn't find a node in the tree which was less */
            cmp=1;
        else /* reset this for cmp test below */
            cmp=0;
      } /* end if */

    if (NULL != pp)
        *pp = parent;

    /* Set return value */
    ret_value= (0 == cmp) ? ptr : NULL;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_less */


/*-------------------------------------------------------------------------
 * Function:	H5TB_index
 *
 * Purpose: Locate the node that has `indx' nodes with lesser key values.
 * This is like an array lookup with the first item in the list having index 0.
 * For large values of `indx', this call is much faster than H5TB_first()
 * followed by `indx' H5TB_next()s.  Thus `H5TB_index(&root,0L)' is equivalent to
 * (and almost as fast as) `H5TB_first(root)'.
 *
 * Return:	Success:	Pointer to a valid H5TB node
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_NODE  *
H5TB_index(H5TB_NODE * root, unsigned indx)
{
    H5TB_NODE  *ptr = root;
    H5TB_NODE  *ret_value;      /* Return value */

    FUNC_ENTER_NOAPI(H5TB_index,NULL);

    if (NULL != ptr) {
      /* Termination condition is if the index equals the number of children on
         out left plus the current node */
        while (ptr != NULL && indx != ((unsigned) LeftCnt(ptr)) ) {
            if (indx <= (unsigned) LeftCnt(ptr)) {
                ptr = ptr->Lchild;
              } /* end if */
            else if (HasChild(ptr, RIGHT)) {
                /* subtract children count from leftchild plus current node when
                   we descend into a right branch */
                indx -= (unsigned)(LeftCnt(ptr) + 1);  
                ptr = ptr->Rchild;
              } /* end if */
            else {
              /* Only `indx' or fewer nodes in tree */
              ptr=NULL;
              break;
            } /* end else */
        } /* end while */
    } /* end if */

    /* Set return value */
    ret_value=ptr;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_index() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_dins
 *
 * Purpose: Insert a new node into a "described" tree, having a key value of
 * `key' and a data pointer of `item'.  If a node already exists in the tree
 * with key value `key' or if malloc() fails, NULL is returned (no node is
 * inserted), otherwise a pointer to the inserted node is returned.  `cmp' and
 * `arg' are as for H5TB_find().
 *
 * Return:	Success:	Pointer to a valid H5TB node
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_NODE  *
H5TB_dins(H5TB_TREE * tree, void * item, void * key)
{
    H5TB_NODE  *ret_value;       /* the node to return */

    FUNC_ENTER_NOAPI(H5TB_dins,NULL);

    assert(tree);

    /* Try to insert the node */
    ret_value = H5TB_ins(&(tree->root), item, key, tree->compar, tree->cmparg);

    /* If we successfully inserted the node, increment the node count in the tree */
    if (ret_value != NULL)
        tree->count++;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_dins() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_ins
 *
 * Purpose: Insert a new node into a "non-described" tree, having a key value of
 * `key' and a data pointer of `item'.  If a node already exists in the tree
 * with key value `key' or if malloc() fails, NULL is returned (no node is
 * inserted), otherwise a pointer to the inserted node is returned.  `cmp' and
 * `arg' are as for H5TB_find().
 *
 * Return:	Success:	Pointer to a valid H5TB node
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_NODE  *
H5TB_ins(H5TB_NODE ** root, void * item, void * key, H5TB_cmp_t compar, int arg)
{
    int        cmp;
    H5TB_NODE  *ptr, *parent;
    H5TB_NODE  *ret_value;

    FUNC_ENTER_NOAPI(H5TB_ins,NULL);

    assert(root);
    assert(item);

    if (NULL != H5TB_find(*root, (key ? key : item), compar, arg, &parent))
        HGOTO_ERROR (H5E_TBBT, H5E_EXISTS, NULL, "node already in tree");
    if (NULL == (ptr = H5FL_MALLOC(H5TB_NODE)))
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");
    ptr->data = item;
    ptr->key = key ? key : item;
    ptr->Parent = parent;
    ptr->flags = 0L;    /* No children on either side */
    ptr->lcnt = 0;
    ptr->rcnt = 0;

    /* Adding first node to tree: */
    if (NULL == parent) {
          *root = ptr;
          ptr->Lchild = ptr->Rchild = NULL;
      }
    else {
        cmp = KEYcmp(ptr->key, parent->key, arg);
        if (cmp < 0) {
              ptr->Lchild = parent->Lchild;     /* Parent's thread now new node's */
              ptr->Rchild = parent;     /* New nodes right thread is parent */
              parent->Lchild = ptr;     /* Parent now has a left child */
          }
        else {
              ptr->Rchild = parent->Rchild;
              ptr->Lchild = parent;
              parent->Rchild = ptr;
          }
        H5TB_balance(root, parent, (cmp < 0) ? LEFT : RIGHT, 1);
    } /* end else */

    /* Set return value */
    ret_value=ptr;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_ins() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_rem
 *
 * Purpose: Remove a node from a tree.  You pass in the address of the
 * pointer to the root node of the tree along, a pointer to the node you wish
 * to remove, and optionally the address of a pointer to hold the address of
 * the key value of the deleted node.  The second argument is usually the
 * result from a lookup function call (H5TB_find, H5TB_dfind, or H5TB_index)
 * so if it is NULL, H5TB_rem returns NULL.  Otherwise H5TB_rem removes the
 * node from the tree and returns a pointer to the data item for that node and,
 * if the third argument is not NULL, the address of the key value for the
 * deleted node is placed in the buffer that it points to.
 *
 * Examples:
 *     data= H5TB_rem( tree, H5TB_dfind(tree,key), &kp );  free(data);  free(kp);
 *     data= H5TB_rem( &root, H5TB_find(root,key,compar,arg), NULL );
 *     data= H5TB_rem( &tree->root, H5TB_dfind(tree,key), NULL );
 *
 * Return:	Success:	Pointer to data item deleted
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
void *
H5TB_rem(H5TB_NODE ** root, H5TB_NODE * node, void * *kp)
{
    H5TB_NODE  *leaf;   /* Node with one or zero children */
    H5TB_NODE  *par;    /* Parent of `leaf' */
    H5TB_NODE  *next;   /* Next/prev node near `leaf' (`leaf's `side' thread) */
    int        side;   /* `leaf' is `side' child of `par' */
    void *      data;   /* Saved pointer to data item of deleted node */
    void *ret_value;    /* Return value */

    FUNC_ENTER_NOAPI(H5TB_rem, NULL);

    if (NULL == root || NULL == node)
        HGOTO_ERROR (H5E_ARGS, H5E_BADVALUE, NULL, "bad arguments to delete");

    data = node->data;  /* Save pointer to data item to be returned at end */
    if (NULL != kp)
        *kp = node->key;

    /* If the node to be removed is "internal" (children on both sides), we
     * replace it with it's previous (or next) node in the tree and delete that
     * previous (next) node (which has one or no children) instead. */

    /* Replace with a non-internal node: */
    if (Intern(node)) {
          /* Pick "near-leaf" node from the */
          if (Heavy(node, RIGHT)) {
                side = LEFT;    /* heavier of the sub-trees. */
            }
          else if (Heavy(node, LEFT)) {
                side = RIGHT;
            }
          /* If no sub-tree heavier, pick at "random" for "better balance" */
          else {
                side = (0x10 & *(short *) &node) ? LEFT : RIGHT;    /* balance" */
            }
          leaf = H5TB_nbr(next = node, Other(side));
          par = leaf->Parent;

          /* Case 2x: `node' had exactly 2 descendants */
          if (par == next) {
                side = Other(side);     /* Transform this to Case 2 */
                next = leaf->link[side];
            }
          node->data = leaf->data;
          node->key = leaf->key;
      } /* end if */
    /* Node has one or zero children: */
    else {
          leaf = node;  /* Simply remove THIS node */
          par = leaf->Parent;

          /* Case 3: Remove root (of 1- or 2-node tree) */
          if (NULL == par) {
                side = (int) UnBal(node);  /* Which side root has a child on */

                /* Case 3a: Remove root of 2-node tree: */
                if (side) {
                      *root = leaf = node->link[side];
                      leaf->Parent = leaf->link[Other(side)] = NULL;
                      leaf->flags = 0;  /* No left children, balanced, not internal */
                  }
                /* Case 3b: Remove last node of tree: */
                else {
                      *root = NULL;
                  }     /* end else */
                H5FL_FREE(H5TB_NODE,node);
                HGOTO_DONE(data);
            }
          side = (par->Rchild == leaf) ? RIGHT : LEFT;
          next = leaf->link[side];
      } /* end else */

    /* Now the deletion has been reduced to the following cases (and Case 3 has
     * been handled completely above and Case 2x has been transformed into
     * Case 2).  `leaf' is a node with one or zero children that we are going
     * to remove.  `next' points where the `side' thread of `leaf' points.
     * `par' is the parent of `leaf'.  The only posibilities (not counting
     * left/right reversals) are shown below:
     *       [Case 1]                  [Case 2]              [Case 2x]
     *            (next)                 (next)         ^         (next & par)
     *           /  ^   \               /  ^   \        |        /  ^         \
     *     . . .    |             . . .    |            |  (leaf)   /
     *   /          |           /          |            \_/      \_/
     * (par)        |         (par)        |             ^threads^
     *      \       |              \       |
     *     (leaf)   /             (leaf)   /            [Case 3a]    [Case 3b]
     *    /  ^   \_/<thread             \_/<thread       (root)
     * (n)   /                                                 \       (root)
     *    \_/<thread        --"side"-->                         (n)
     * Note that in Cases 1 and 2, `leaf's `side' thread can be NULL making
     * `next' NULL as well.  If you remove a node from a 2-node tree, removing
     * the root falls into Case 3a while removing the only leaf falls into
     * Case 2 (with `next' NULL and `par' the root node). */

    /* Case 2: `leaf' has no children: */
    if (!UnBal(leaf)) {
          par->link[side] = leaf->link[side];
          par->flags &= (H5TB_flag)(~(H5TB_INTERN | H5TB_HEAVY(side)));
      } /* end if */
    /* Case 1: `leaf' has one child: */
    else {
          H5TB_NODE  *n;

          /* two-in-a-row cases */
          if (HasChild(leaf, side)) {
                n = leaf->link[side];
                par->link[side] = n;
                n->Parent = par;
                if (HasChild(n, Other(side)))
                    while (HasChild(n, Other(side)))
                        n = n->link[Other(side)];
                n->link[Other(side)] = par;
            }   /* end if */
          /* zig-zag cases */
          else {
                n = leaf->link[Other(side)];
                par->link[side] = n;
                n->Parent = par;
                if (HasChild(n, side))
                    while (HasChild(n, side))
                        n = n->link[side];
                n->link[side] = next;
            }   /* end else */
      } /* end else */

    H5FL_FREE(H5TB_NODE,leaf);
    H5TB_balance(root, par, side, -1);

    /* Set return value */
    ret_value=data;

done:
    if(ret_value)
        ((H5TB_TREE *) root)->count--;

    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_rem() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_first
 *
 * Purpose: Retrieves a pointer to node from the tree with the lowest(first)
 * key value.  If the tree is empy NULL is returned.  Examples:
 *     node= H5TB_first(*tree);
 *     node= H5TB_first(root);
 *
 * Return:	Success:	Pointer to a valid H5TB node
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_NODE  *
H5TB_first(H5TB_NODE * root)
{
    H5TB_NODE *ret_value;               /* Return value */

    FUNC_ENTER_NOAPI(H5TB_first,NULL);

    /* Set return value */
    ret_value=H5TB_end(root, LEFT);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_first() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_last
 *
 * Purpose: Retrieves a pointer to node from the tree with the highest(last)
 * key value.  If the tree is empy NULL is returned.  Examples:
 *     node= H5TB_last(tree->root);
 *     node= H5TB_last(node);        (* Last node in a sub-tree *)
 *
 * Return:	Success:	Pointer to a valid H5TB node
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_NODE  *
H5TB_last(H5TB_NODE * root)
{
    H5TB_NODE *ret_value;               /* Return value */

    FUNC_ENTER_NOAPI(H5TB_last,NULL);

    /* Set return value */
    ret_value=H5TB_end(root, RIGHT);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_last() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_next
 *
 * Purpose: Returns a pointer the node from the tree with the next highest
 * key value relative to the node pointed to by `node'.  If `node' points the
 * last node of the tree, NULL is returned.
 *
 * Return:	Success:	Pointer to a valid H5TB node
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_NODE  *
H5TB_next(H5TB_NODE * node)
{
    H5TB_NODE *ret_value;               /* Return value */

    FUNC_ENTER_NOAPI(H5TB_next,NULL);

    /* Set return value */
    ret_value=H5TB_nbr(node, RIGHT);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_next() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_prev
 *
 * Purpose: Returns a pointer the node from the tree with the previous lowest
 * key value relative to the node pointed to by `node'.  If `node' points the
 * first node of the tree, NULL is returned.
 *
 * Return:	Success:	Pointer to a valid H5TB node
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_NODE  *
H5TB_prev(H5TB_NODE * node)
{
    H5TB_NODE *ret_value;               /* Return value */

    FUNC_ENTER_NOAPI(H5TB_prev,NULL);

    /* Set return value */
    ret_value=H5TB_nbr(node, LEFT);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_prev() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_dfree
 *
 * Purpose: Frees up an entire tree.  `fd' is a pointer to a function that
 * frees/destroys data items, and `fk' is the same for key values.
 *     void free();
 *       tree= tbbtdfree( tree, free, free );
 *       H5TB_free( &root, free, free );
 * is a typical usage, where keys and data are individually malloc()d.  If `fk'
 * is NULL, no action is done for the key values (they were allocated on the
 * stack, as a part of each data item, or together with one malloc() call, for
 * example) and likewise for `fd'.  H5TB_dfree() always returns NULL and
 * H5TB_free() always sets `root' to be NULL.
 *
 * Return:	Always returns NULL
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
H5TB_TREE  *
H5TB_dfree(H5TB_TREE * tree, void(*fd) (void * /* item */), void(*fk) (void * /* key */))
{
    H5TB_TREE *ret_value=NULL;  /* Return value */

    FUNC_ENTER_NOAPI(H5TB_dfree,NULL);

    if (tree != NULL) {
        /* Free the actual tree */
        H5TB_free(&tree->root, fd, fk);

        /* Free the tree root */
        H5FL_FREE(H5TB_TREE,tree);
    } /* end if */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_dfree() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_free
 *
 * Purpose: Frees up an entire tree.  `fd' is a pointer to a function that
 * frees/destroys data items, and `fk' is the same for key values.
 *     void free();
 *       tree= tbbtdfree( tree, free, free );
 *       H5TB_free( &root, free, free );
 * is a typical usage, where keys and data are individually malloc()d.  If `fk'
 * is NULL, no action is done for the key values (they were allocated on the
 * stack, as a part of each data item, or together with one malloc() call, for
 * example) and likewise for `fd'.  H5TB_dfree() always returns NULL and
 * H5TB_free() always sets `root' to be NULL.
 *
 * Return:	Always returns NULL
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
void *
H5TB_free(H5TB_NODE ** root, void(*fd) (void * /* item */), void(*fk) (void * /* key */))
{
    H5TB_NODE  *par, *node = *root;
    void *ret_value=NULL;                    /* Return value */

    FUNC_ENTER_NOAPI(H5TB_free,NULL);

    /* While nodes left to be free()d */
    while (NULL != *root) {
          /* First time at this node (just moved down a new leg of tree) */
          if (!HasChild(node, LEFT))
              node->Lchild = NULL;
          if (!HasChild(node, RIGHT))
              node->Rchild = NULL;
          do {
                par = NULL;     /* Assume we aren't ready to move up tree yet */
                if (NULL != node->Lchild)
                    node = node->Lchild;    /* Move down this leg next */
                else if (NULL != node->Rchild)
                    node = node->Rchild;    /* Move down this leg next */
                /* No children; free node an move up: */
                else {
                      par = node->Parent;   /* Move up tree (stay in loop) */
                      if (NULL != fd)
                          (*fd) (node->data);
                      if (NULL != fk)
                          (*fk) (node->key);
                      if (NULL == par)  /* Just free()d last node */
                          *root = NULL;     /* NULL=par & NULL=*root gets fully out */
                      else if (node == par->Lchild)
                          par->Lchild = NULL;   /* Now no longer has this child */
                      else
                          par->Rchild = NULL;   /* Ditto */

                      H5FL_FREE(H5TB_NODE,node);

                      node = par;   /* Move up tree; remember which node to do next */
                  } /* end else */
            } while (NULL != par);  /* While moving back up tree */
      } /* end while */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_free() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_count
 *
 * Purpose: Returns the number of nodes in a tree
 *
 * Return:	Success - Number of nodes in the tree
 *          Failure - Negative value
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
long
H5TB_count(H5TB_TREE * tree)
{
    long ret_value;     /* Return value */

    FUNC_ENTER_NOAPI(H5TB_count,FAIL);

    /* Set return value */
    ret_value= (tree==NULL) ? FAIL : (long)tree->count;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_count() */

#ifdef H5TB_DEBUG


/*-------------------------------------------------------------------------
 * Function:	H5TB_dump
 *
 * Purpose: Prints out information about an entire tree.
 * The 'method' variable determines which sort of traversal is used:
 *      -1 : Pre-Order Traversal
 *       1 : Post-Order Traversal
 *       0 : In-Order Traversal
 *
 * Return:	Shouldn't fail
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
herr_t
H5TB_dump(H5TB_TREE *tree, void (*key_dump)(void *,void *), int method)
{
    FUNC_ENTER_NOAPI(H5TB_dump,FAIL);

    printf("H5TB-tree dump  %p:\n",tree);
    printf("capacity = %ld\n\n",(long)tree->count);
    H5TB_dumpNode(tree->root,key_dump, method);

    FUNC_LEAVE_NOAPI(SUCCESS);
}   /* end H5TB_dump() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_printNode
 *
 * Purpose: Prints out information about a node in the tree
 *
 * Return:	Shouldn't fail
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
static herr_t
H5TB_printNode(H5TB_NODE * node, void(*key_dump)(void *,void *))
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5TB_printNode);

    if (node == NULL) {
        printf("ERROR:  null node pointer\n");
        HGOTO_DONE(FAIL);
    }

    printf("node=%p, key=%p, data=%p, flags=%x\n", node, node->key, node->data, (unsigned) node->flags);
    printf("Lcnt=%d, Rcnt=%d\n", (int) node->lcnt, (int) node->rcnt);
    printf("Lchild=%p, Rchild=%p, Parent=%p\n", node->Lchild, node->Rchild, node->Parent);
    if (key_dump != NULL)
        (*key_dump)(node->key,node->data);

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_printNode() */


/*-------------------------------------------------------------------------
 * Function:	H5TB_dumpNode
 *
 * Purpose: Internal routine to actually dump tree
 * The 'method' variable determines which sort of traversal is used:
 *      -1 : Pre-Order Traversal
 *       1 : Post-Order Traversal
 *       0 : In-Order Traversal
 *
 * Return:	Shouldn't fail
 *
 * Programmer:	Quincey Koziol
 *              Friday, May 6, 2000
 *
 * Modifications:
 * 
 * Notes:
 * 	
 *-------------------------------------------------------------------------
 */
static herr_t
H5TB_dumpNode(H5TB_NODE *node, void (*key_dump)(void *,void *),
                        int method)
{
    herr_t      ret_value=SUCCEED;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT(H5TB_dumpNode);

    if (node == NULL)
        HGOTO_DONE(FAIL);

    switch (method) {
          case -1:      /* Pre-Order Traversal */
              H5TB_printNode(node, key_dump);
              if (HasChild(node, LEFT))
                  H5TB_dumpNode(node->Lchild, key_dump, method);
              if (HasChild(node, RIGHT))
                  H5TB_dumpNode(node->Rchild, key_dump, method);
              break;

          case 1:   /* Post-Order Traversal */
              if (HasChild(node, LEFT))
                  H5TB_dumpNode(node->Lchild, key_dump, method);
              if (HasChild(node, RIGHT))
                  H5TB_dumpNode(node->Rchild, key_dump, method);
              H5TB_printNode(node, key_dump);
              break;

          case 0:   /* In-Order Traversal */
          default:
              if (HasChild(node, LEFT))
                  H5TB_dumpNode(node->Lchild, key_dump, method);
              H5TB_printNode(node, key_dump);
              if (HasChild(node, RIGHT))
                  H5TB_dumpNode(node->Rchild, key_dump, method);
              break;

      } /* end switch() */

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_dumpNode() */

#endif /* H5TB_DEBUG */



/*-------------------------------------------------------------------------
 * Function:	H5TB_end
 *
 * Purpose:	Returns pointer to end-most (to LEFT or RIGHT) node of tree:
 *
 * Return:	Success:	Valid pointer
 * 		Failure:	NULL
 *
 * Programmer:	Quincey Koziol
 *              Saturday, April 22, 2000
 *
 * Modifications:
 * 	
 *-------------------------------------------------------------------------
 */
static H5TB_NODE *
H5TB_end(H5TB_NODE * root, int side)
{
    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5TB_end);

    assert(root);
    assert(side==LEFT || side==RIGHT);

    while (HasChild(root, side))
      root = root->link[side];

    FUNC_LEAVE_NOAPI(root);
}   /* end H5TB_end() */

/* Returns pointer to neighboring node (to LEFT or RIGHT): */
static H5TB_NODE *
H5TB_nbr(H5TB_NODE * ptr, int side)
{
    H5TB_NODE *ret_value;       /* Return value */

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5TB_nbr);

    if (!HasChild(ptr, side))
        HGOTO_DONE (ptr->link[side]);
    ptr = ptr->link[side];
    if(ptr==NULL)
        HGOTO_DONE(NULL);
    while (HasChild(ptr, Other(side)))
        ptr = ptr->link[Other(side)];

    /* Set return value */
    ret_value=ptr;

done:
    FUNC_LEAVE_NOAPI(ret_value);
}   /* end H5TB_nbr() */

/* H5TB_ffind -- Look up a node in a tree based on a key value */
/* This routine is based on tbbtfind (fix bugs in both places!) */
/* Returns a pointer to the found node (or NULL) */
static H5TB_NODE  *
H5TB_ffind(H5TB_NODE * root, const void * key, unsigned fast_compare, H5TB_NODE ** pp)
{
    H5TB_NODE  *ptr = root;
    H5TB_NODE  *parent = NULL;
    int        side;
    int        cmp = 1;
    H5TB_NODE  *ret_value = NULL;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5TB_ffind);

    switch(fast_compare) {
        case H5TB_FAST_HADDR_COMPARE:
            if (ptr) {
                while (0 != (cmp = H5F_addr_cmp(*(const haddr_t *)key,*(haddr_t *)ptr->key))) {
                      parent = ptr;
                      side = (cmp < 0) ? LEFT : RIGHT;
                      if (!HasChild(ptr, side))
                          break;
                      ptr = ptr->link[side];
                  } /* end while */
              } /* end if */
            if (NULL != pp)
                *pp = parent;

            /* Set return value */
            ret_value= (0 == cmp) ? ptr : NULL;
            break;

        case H5TB_FAST_INTN_COMPARE:
            if (ptr) {
                while (0 != (cmp = (*(const int *)key - *(int *)ptr->key))) {
                      parent = ptr;
                      side = (cmp < 0) ? LEFT : RIGHT;
                      if (!HasChild(ptr, side))
                          break;
                      ptr = ptr->link[side];
                  } /* end while */
              } /* end if */
            if (NULL != pp)
                *pp = parent;

            /* Set return value */
            ret_value= (0 == cmp) ? ptr : NULL;
            break;

        case H5TB_FAST_STR_COMPARE:
            if (ptr) {
                while (0 != (cmp = HDstrcmp(key,ptr->key))) {
                      parent = ptr;
                      side = (cmp < 0) ? LEFT : RIGHT;
                      if (!HasChild(ptr, side))
                          break;
                      ptr = ptr->link[side];
                  } /* end while */
              } /* end if */
            if (NULL != pp)
                *pp = parent;

            /* Set return value */
            ret_value= (0 == cmp) ? ptr : NULL;
            break;

        case H5TB_FAST_HSIZE_COMPARE:
            if (ptr) {
                while (0 != (cmp = (int)(*(const hsize_t *)key - *(hsize_t *)ptr->key))) {
                      parent = ptr;
                      side = (cmp < 0) ? LEFT : RIGHT;
                      if (!HasChild(ptr, side))
                          break;
                      ptr = ptr->link[side];
                  } /* end while */
              } /* end if */
            if (NULL != pp)
                *pp = parent;

            /* Set return value */
            ret_value= (0 == cmp) ? ptr : NULL;
            break;

        default:
            break;
    } /* end switch */

    FUNC_LEAVE_NOAPI(ret_value);
} /* H5TB_ffind() */

/* swapkid -- Often refered to as "rotating" nodes.  ptr and ptr's `side'
 * child, kid, are swapped so ptr becomes kid's `Other(side)' child.
 * Here is how a single swap (rotate) works:
 *
 *           |           --side-->         |
 *         (ptr)                         (kid)
 *        /     \                       /     \
 *    +-A-+    (kid)                 (ptr)    +-C-+
 *    |   |   /     \               /     \   |   |
 *    |...| +-B-+  +-C-+         +-A-+  +-B-+ |...|
 *          |   |  |   |         |   |  |   |
 *          |...|  |...|         |...|  |...|
 * `deep' contains the relative depths of the subtrees so, since we set
 * `deep[1]' (the relative depth of subtree [B]) to 0, `deep[2]' is the depth
 * of [C] minus the depth of [B] (-1, 0, or 1 since `kid' is never too out of
 * balance) and `deep[0]' is the depth of [A] minus the depth of [B].  These
 * values are used to compute the balance levels after the rotation.  Note that
 * [A], [B], or [C] can have depth 0 so `link[]' contains threads rather than
 * pointers to children.
 */
static H5TB_NODE *
H5TB_swapkid(H5TB_NODE ** root, H5TB_NODE * ptr, int side)
{
    H5TB_NODE  *kid = ptr->link[side];  /* Sibling to be swapped with parent */
    int        deep[3];        /* Relative depths of three sub-trees involved. */
    /* 0:ptr->link[Other(side)], 1:kid->link[Other(side)], 2:kid->link[side] */
    H5TB_flag   ptrflg;         /* New value for ptr->flags (ptr->flags used after set) */
    H5TB_leaf   plcnt, prcnt,   /* current values of the ptr's and kid's leaf count */
                klcnt, krcnt;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5TB_swapkid);

    deep[2] = (deep[1] = 0) + Delta(kid, side);
    deep[0] = Max(0, deep[2]) + 1 - Delta(ptr, side);
    kid->Parent = ptr->Parent;
    ptrflg = (H5TB_flag)SetFlags(ptr, side, deep[0],
                  HasChild(ptr, Other(side)) && HasChild(kid, Other(side)));
    plcnt = LeftCnt(ptr);
    prcnt = RightCnt(ptr);
    klcnt = LeftCnt(kid);
    krcnt = RightCnt(kid);
    if (HasChild(kid, Other(side))) {
          ptr->link[side] = kid->link[Other(side)];     /* Real child */
          ptr->link[side]->Parent = ptr;
      }
    else {
          ptr->link[side] = kid;    /* Thread */
      }
    /* Update grand parent's pointer: */
    if (NULL == ptr->Parent) {
          *root = kid;
      }
    else if (ptr /*->Lchild*/  == ptr->Parent->Lchild) {
          ptr->Parent->Lchild = kid;
      }
    else {
          ptr->Parent->Rchild = kid;
      }
    ptr->Parent = kid;
    kid->link[Other(side)] = ptr;
    kid->flags = (H5TB_flag)SetFlags(kid, Other(side),
                        deep[2] - 1 - Max(deep[0], 0), HasChild(kid, side));

    /* update leaf counts */
    if (side == LEFT) {     /* kid's left count doesn't change, nor ptr's r-count */
          kid->rcnt = prcnt + krcnt + 1;    /* kid's leafs+former parent's leafs+parent */
          ptr->lcnt = krcnt;
      }     /* end if */
    else {     /* kid's right count doesn't change, nor ptr's l-count */
          kid->lcnt = plcnt + klcnt + 1;    /* kid's leafs+former parent's leafs+parent */
          ptr->rcnt = klcnt;
      }     /* end if */
    ptr->flags = ptrflg;

    FUNC_LEAVE_NOAPI(kid);
}   /* end H5TB_swapkid() */

/* balance -- Move up tree, incrimenting number of left children when needed
 * and looking for unbalanced ancestors.  Adjust all balance factors and re-
 * balance through "rotation"s when needed.
 */
/* Here is how rotatation rebalances a tree:
 * Either the deletion of a node shortened the sub-tree [A] (to length `h')
 * while [B] or [C] or both are length `h+1'  or  the addition of a node
 * lengthened [B] or [C] to length `h+1' while the other and [A] are both
 * length `h'.  Each case changes `ptr' from being "right heavy" to being
 * overly unbalanced.
 * This           |                      Becomes:      |
 * sub-tree:    (ptr)                                (kid)
 *             /     \          --side-->           /     \
 *         +-A-+    (kid)                        (ptr)   +-C-+
 *         |   |   /     \                      /     \  |   |
 *         | h | +-B-+  +-C-+               +-A-+  +-B-+ | h |
 *         |   | |   |  |   |               |   |  |   | |   |
 *         +---+ | h |  | h |               | h |  | h | +---+
 *         : - : |   |  |   |               |   |  |   | : 1 :
 *         `- -' +---+  +---+               +---+  +---+ + - +
 *               : 1 :  : 1 :                      : 1 :
 *               + - +  + - +                      + - +
 *
 * However, if [B] is long (h+1) while [C] is short (h), a double rotate is
 * required to rebalance.  In this case, [A] was shortened or [X] or [Y] was
 * lengthened so [A] is length `h' and one of [X] and [Y] is length `h' while
 * the other is length `h-1'.  Swap `kid' with `babe' then `ptr' with `babe'.
 * This          |                         Becomes:     |
 * sub-tree:   (ptr)                                  (babe)
 *            /     \             --side-->          /      \
 *       +-A-+       (kid)                      (ptr)       (kid)
 *       |   |      /     \                    /     \     /     \
 *       | h |    (babe)   +-C-+             +-A-+ +-X-+ +-Y-+ +-C-+
 *       |   |   /      \  |   |             |   | |h-1| |h-1| |   |
 *       +---+ +-X-+ +-Y-+ | h |             | h | +---+ +---+ | h |
 *       : - : |h-1| |h-1| |   |             |   | : 1 : : 1 : |   |
 *       `- -' +---+ +---+ +---+             +---+ + - + + - + +---+
 *             : 1 : : 1 :
 *             + - + + - +
 *
 * Note that in the node insertion cases total sub-tree length always increases
 * by one then decreases again so after the rotation(s) no more rebalancing is
 * required.  In the node removal cases, the single rotation reduces total sub-
 * tree length unless [B] is length `h+1' (`ptr' ends of "right heavy") while
 * the double rotation ALWAYS reduces total sub-tree length.  Thus removing a
 * single node can require log(N) rotations for rebalancing.  On average, only
 * are usually required.
 */
static      herr_t
H5TB_balance(H5TB_NODE ** root, H5TB_NODE * ptr, int side, int added)
{
    int        deeper = added; /* 1 if sub-tree got longer; -1 if got shorter */
    int        odelta;
    int        obal;

    FUNC_ENTER_NOAPI_NOINIT_NOFUNC(H5TB_balance);

    while (NULL != ptr) {
          odelta = Delta(ptr, side);    /* delta before the node was added */
          obal = UnBal(ptr);
          if (LEFT == side)     /* One more/fewer left child: */
              if (0 < added)
                  ptr->lcnt++;  /* LeftCnt(ptr)++ */
              else
                  ptr->lcnt--;  /* LeftCnt(ptr)-- */
          else if (0 < added)
              ptr->rcnt++;  /* RightCnt(ptr)++ */
          else
              ptr->rcnt--;  /* RightCnt(ptr)-- */
          if (0 != deeper)
            {   /* One leg got longer or shorter: */
                if ((deeper < 0 && odelta < 0) || (deeper > 0 && odelta > 0))
                  {     /* Became too unbalanced: */
                      H5TB_NODE  *kid;

                      ptr->flags |= H5TB_DOUBLE;    /* Mark node too unbalanced */
                      if (deeper < 0)   /* Just removed a node: */
                          side = Other(side);   /* Swap with child from other side. */
                      else
                          /* Just inserted a node: */ if (ptr->Parent && UnBal(ptr->Parent))
                        {
                            deeper = 0;     /* Fix will re-shorten sub-tree. */
                        }
                      kid = ptr->link[side];
                      if (Heavy(kid, Other(side)))
                        {   /* Double rotate needed: */
                            kid = H5TB_swapkid(root, kid, Other(side));
                            ptr = H5TB_swapkid(root, ptr, side);
                        }
                      else
                        {   /* Just rotate parent and kid: */
                            if (HasChild(kid, side))    /* In this case, sub-tree gets */
                                if (ptr->Parent && UnBal(ptr->Parent))
                                  {
                                      deeper = 0;   /* re-lengthened after a node removed. */
                                  }
                            ptr = H5TB_swapkid(root, ptr, side);
                        }
                  }
                else if (obal)
                  {     /* Just became balanced: */
                      ptr->flags &= ~H5TB_UNBAL;
                      if (0 < deeper)
                        {   /* Shorter of legs lengthened */
                            ptr->flags |= H5TB_INTERN;  /* Mark as internal node now */
                            deeper = 0;     /* so max length unchanged */
                        }   /* end if */
                  }
                else if (deeper < 0)
                  {     /* Just became unbalanced: */
                      if (ptr->link[Other(side)] != NULL && ptr->link[Other(side)]->Parent == ptr)
                        {
                            ptr->flags |= (H5TB_flag)H5TB_HEAVY(Other(side));  /* Other side longer */
                            if (ptr->Parent) {
                                if (ptr->Parent->Rchild == ptr) {
                                    /* we're the right child */
                                    if (Heavy(ptr->Parent, RIGHT) && LeftCnt(ptr->Parent) == 1) {
                                        deeper = 0;
                                    } else {
                                        /* we're the left child */
                                        if (Heavy(ptr->Parent, LEFT)) {
                                            if (ptr->Parent->Rchild && !UnBal(ptr->Parent->Rchild)) {
                                                deeper = 0;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                  }
                else
                  {     /* Just became unbalanced: */
                      ptr->flags |= (H5TB_flag)H5TB_HEAVY(side);   /* 0<deeper: Our side longer */
                  }     /* end else */
            }
          if (ptr->Parent)
            {
                if (ptr == (ptr->Parent->Rchild))
                    side = RIGHT;
                else
                    side = LEFT;
            }   /* end if */
          ptr = ptr->Parent;    /* Move up the tree */
      }
    /* total tree depth += deeper; */
    FUNC_LEAVE_NOAPI(SUCCEED);
}   /* end H5TB_balance() */

