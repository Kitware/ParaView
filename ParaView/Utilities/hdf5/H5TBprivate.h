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

/*-------------------------------------------------------------------------
 *
 * Created:		H5TBprivate.h
 *			Apr 22 2000
 *			Quincey Koziol <koziol@ncsa.uiuc.edu>
 *
 * Purpose:		Private non-prototype header.
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifndef _H5TBprivate_H
#define _H5TBprivate_H

/* Public headers needed by this file */
#ifdef LATER
#include "H5TBpublic.h"		/*Public API prototypes */
#endif /* LATER */

/* Typedef for key comparison function */
typedef int (*H5TB_cmp_t)(const void *k1, const void *k2, int cmparg);

/* Shortcut macros for links */
# define   PARENT  0
# define   LEFT    1
# define   RIGHT   2

# define  Parent    link[PARENT]
# define  Lchild    link[LEFT]
# define  Rchild    link[RIGHT]

/* Tree-balancing flags */
# define  H5TB_HEAVY(s) s       /* If the `s' sub-tree is deeper than the other */
# define  H5TB_DOUBLE   4       /* If "heavy" sub-tree is two levels deeper */
# define  H5TB_INTERN   8       /* If node is internal (has two children) */
# define  H5TB_UNBAL    ( H5TB_HEAVY(LEFT) | H5TB_HEAVY(RIGHT) )
# define  H5TB_FLAGS    ( H5TB_UNBAL | H5TB_INTERN | H5TB_DOUBLE )
# define  H5TB_CHILD(s) ( H5TB_INTERN | H5TB_HEAVY(s) )

/* Internal macros */
# define  LeftCnt(node) ( (node)->lcnt )    /* Left descendants */
# define  RightCnt(node) ( (node)->rcnt )   /* Right descendants */
# define  Cnt(node,s)   ( LEFT==(s) ? LeftCnt(node) : RightCnt(node) )
# define  HasChild(n,s) ( Cnt(n,s)>0 )
# define  Heavy(n,s)    ( (s) & (LeftCnt(n)>RightCnt(n) ? LEFT : \
                 LeftCnt(n)==RightCnt(n) ? 0 : RIGHT))
# define  Intern(n)     ( LeftCnt(n) && RightCnt(n) )
# define  UnBal(n)      ( LeftCnt(n)>RightCnt(n) ? LEFT : \
                 LeftCnt(n)==RightCnt(n) ? 0 : RIGHT)
# define  Double(n)     ( H5TB_DOUBLE & (n)->flags )
# define  Other(side)   ( LEFT + RIGHT - (side) )
# define  Delta(n,s)    (  ( Heavy(n,s) ? 1 : -1 )                          \
                            *  ( Double(n) ? 2 : UnBal(n) ? 1 : 0 )  )
# define  SetFlags(n,s,b,i)   (  ( -2<(b) && (b)<2 ? 0 : H5TB_DOUBLE )   \
    |  ( 0>(b) ? H5TB_HEAVY(s) : (b)>0 ? H5TB_HEAVY(Other(s)) : 0 )        \
    |  ( (i) ? H5TB_INTERN : 0 )  )

/* Internal types for flags & leaf counts */
typedef unsigned long H5TB_flag;
typedef unsigned long H5TB_leaf;

/* Threaded node structure */
typedef struct H5TB_node
{
    void *       data;          /* Pointer to user data to be associated with node */
    void *       key;           /* Field to sort nodes on */

    struct H5TB_node *link[3];  /* Pointers to parent, left child, and right child */
    H5TB_flag flags;        /* Combination of the bit fields */
    H5TB_leaf lcnt;         /* count of left children */
    H5TB_leaf rcnt;         /* count of right children */
} H5TB_NODE;

/* Threaded tree structure */
typedef struct H5TB_tree
{
    H5TB_NODE  *root;       /* Pointer to actual root of tbbt tree */
    unsigned long count;    /* The number of nodes in the tree currently */
    unsigned fast_compare;  /* use a faster in-line compare (with casts) instead of function call */
    H5TB_cmp_t  compar;     /* Key comparison function */
    int cmparg;
} H5TB_TREE;

/* Define the "fast compare" values */
#define H5TB_FAST_HADDR_COMPARE    1
#define H5TB_FAST_INTN_COMPARE     2
#define H5TB_FAST_STR_COMPARE      3
#define H5TB_FAST_HSIZE_COMPARE    4

/* Define an access macro for getting a node's data */
#define H5TB_NODE_DATA(n)       ((n)->data)

#if defined c_plusplus || defined __cplusplus
extern      "C"
{
#endif                          /* c_plusplus || __cplusplus */

H5_DLL H5TB_TREE  *H5TB_dmake (H5TB_cmp_t cmp, int arg, unsigned fast_compare);
H5_DLL H5TB_TREE  *H5TB_fast_dmake (unsigned fast_compare);
H5_DLL H5TB_NODE  *H5TB_dfind (H5TB_TREE * tree, const void * key, H5TB_NODE ** pp);
H5_DLL H5TB_NODE  *H5TB_find(H5TB_NODE * root, const void * key, H5TB_cmp_t cmp,
                 int arg, H5TB_NODE ** pp);
H5_DLL H5TB_NODE  *H5TB_dless (H5TB_TREE * tree, void * key, H5TB_NODE ** pp);
H5_DLL H5TB_NODE  *H5TB_less (H5TB_NODE * root, void * key, H5TB_cmp_t cmp,
                 int arg, H5TB_NODE ** pp);
H5_DLL H5TB_NODE  *H5TB_index (H5TB_NODE * root, unsigned indx);
H5_DLL H5TB_NODE  *H5TB_dins (H5TB_TREE * tree, void * item, void * key);
H5_DLL H5TB_NODE  *H5TB_ins (H5TB_NODE ** root, void * item, void * key, H5TB_cmp_t cmp, int arg);
H5_DLL void *H5TB_rem(H5TB_NODE ** root, H5TB_NODE * node, void * *kp);
H5_DLL H5TB_NODE  *H5TB_first (H5TB_NODE * root);
H5_DLL H5TB_NODE  *H5TB_last (H5TB_NODE * root);
H5_DLL H5TB_NODE  *H5TB_next (H5TB_NODE * node);
H5_DLL H5TB_NODE  *H5TB_prev (H5TB_NODE * node);
H5_DLL H5TB_TREE  *H5TB_dfree (H5TB_TREE * tree, void(*fd) (void *), void(*fk) (void *));
H5_DLL void       *H5TB_free (H5TB_NODE ** root, void(*fd) (void *), void(*fk) (void *));
H5_DLL long        H5TB_count (H5TB_TREE * tree);

#ifdef H5TB_DEBUG
H5_DLL herr_t      H5TB_dump(H5TB_TREE *ptree, void (*key_dump)(void *,void *), int method);
#endif /* H5TB_DEBUG */

#if defined c_plusplus || defined __cplusplus
}
#endif                          /* c_plusplus || __cplusplus */

#endif  /* _H5TBprivate_H */

