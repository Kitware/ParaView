/*
 * Copyright (C) 2000 NCSA
 *                    All rights reserved.
 *
 * Programmer: Quincey Koziol <koziol@ncsa.uiuc.edu>
 *             Thursday, March 23, 2000
 *
 * Purpose: Manage priority queues of free-lists (of blocks of bytes).
 *      These are used in various places in the library which allocate and
 *      free differently blocks of bytes repeatedly.  Usually the same size
 *      of block is allocated and freed repeatly in a loop, while writing out
 *      chunked data for example, but the blocks may also be of different sizes
 *      from different datasets and an attempt is made to optimize access to
 *      the proper free list of blocks by using these priority queues to
 *      move frequently accessed free lists to the head of the queue.
 */

/* #define H5FL_DEBUG */

#include "H5private.h"          /*library                 */
#include "H5Eprivate.h"         /*error handling          */
#include "H5MMprivate.h"        /*Core memory management          */
#include "H5FLprivate.h"        /*Priority queues         */

#define PABLO_MASK      H5FL_mask
static int              interface_initialize_g = 0;
#define INTERFACE_INIT  NULL

/*
 * Private type definitions
 */

/*
    Default limits on how much memory can accumulate on each free list before
    it is garbage collected.
 */
static size_t H5FL_reg_glb_mem_lim=1*16*65536;/* Default to 1MB limit on all regular free lists */
static size_t H5FL_reg_lst_mem_lim=1*65536;   /* Default to 64KB limit on each regular free list */
static size_t H5FL_arr_glb_mem_lim=4*16*65536;/* Default to 4MB limit on all array free lists */
static size_t H5FL_arr_lst_mem_lim=4*65536;   /* Default to 256KB limit on each array free list */
static size_t H5FL_blk_glb_mem_lim=16*16*65536; /* Default to 16MB limit on all block free lists */
static size_t H5FL_blk_lst_mem_lim=16*65536;  /* Default to 1024KB (1MB) limit on each block free list */

/* A garbage collection node for regular free lists */
typedef struct H5FL_reg_gc_node_t {
    H5FL_reg_head_t *list;              /* Pointer to the head of the list to garbage collect */
    struct H5FL_reg_gc_node_t *next;    /* Pointer to the next node in the list of things to garbage collect */
} H5FL_reg_gc_node_t;

/* The garbage collection head for regular free lists */
typedef struct H5FL_reg_gc_list_t {
    size_t mem_freed;               /* Amount of free memory on list */
    struct H5FL_reg_gc_node_t *first;   /* Pointer to the first node in the list of things to garbage collect */
} H5FL_reg_gc_list_t;

/* The head of the list of things to garbage collect */
static H5FL_reg_gc_list_t H5FL_reg_gc_head={0,NULL};

/* A garbage collection node for array free lists */
typedef struct H5FL_gc_arr_node_t {
    H5FL_arr_head_t *list;              /* Pointer to the head of the list to garbage collect */
    struct H5FL_gc_arr_node_t *next;    /* Pointer to the next node in the list of things to garbage collect */
} H5FL_gc_arr_node_t;

/* The garbage collection head for array free lists */
typedef struct H5FL_gc_arr_list_t {
    size_t mem_freed;                    /* Amount of free memory on list */
    struct H5FL_gc_arr_node_t *first;    /* Pointer to the first node in the list of things to garbage collect */
} H5FL_gc_arr_list_t;

/* The head of the list of array things to garbage collect */
static H5FL_gc_arr_list_t H5FL_arr_gc_head={0,NULL};

/* A garbage collection node for blocks */
typedef struct H5FL_blk_gc_node_t {
    H5FL_blk_head_t *pq;                /* Pointer to the head of the PQ to garbage collect */
    struct H5FL_blk_gc_node_t *next;    /* Pointer to the next node in the list of things to garbage collect */
} H5FL_blk_gc_node_t;

/* The garbage collection head for blocks */
typedef struct H5FL_blk_gc_list_t {
    size_t mem_freed;                   /* Amount of free memory on list */
    struct H5FL_blk_gc_node_t *first;   /* Pointer to the first node in the list of things to garbage collect */
} H5FL_blk_gc_list_t;

/* The head of the list of PQs to garbage collect */
static H5FL_blk_gc_list_t H5FL_blk_gc_head={0,NULL};

/* Macros for turning off free lists in the library */
/* #define NO_FREE_LISTS */
#ifdef NO_FREE_LISTS
#define NO_REG_FREE_LISTS
#define NO_ARR_FREE_LISTS
#define NO_BLK_FREE_LISTS
#endif /* NO_FREE_LISTS */

/* Forward declarations of local static functions */
static herr_t H5FL_reg_gc(void);
static herr_t H5FL_reg_gc_list(H5FL_reg_head_t *head);
static herr_t H5FL_arr_gc(void);
static herr_t H5FL_arr_gc_list(H5FL_arr_head_t *head);
static herr_t H5FL_blk_gc(void);
static herr_t H5FL_blk_gc_list(H5FL_blk_head_t *head);

/* Declare a free list to manage the H5FL_blk_node_t struct */
H5FL_DEFINE(H5FL_blk_node_t);


/*-------------------------------------------------------------------------
 * Function:    H5FL_malloc
 *
 * Purpose:     Attempt to allocate space using malloc.  If malloc fails, garbage
 *      collect and try again.  If malloc fails again, then return NULL.
 *
 * Return:      Success:        non-NULL
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, August 1, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
static void *
H5FL_malloc(hsize_t mem_size)
{
    void *ret_value=NULL;   /* return value*/

    FUNC_ENTER (H5FL_malloc, NULL);

    /* Attempt to allocate the memory requested */
    assert(mem_size==(hsize_t)((size_t)mem_size)); /*check for overflow*/
    if(NULL==(ret_value=H5MM_malloc((size_t)mem_size))) {
        /* If we can't allocate the memory now, try garbage collecting first */
        H5FL_garbage_coll();

        /* Now try allocating the memory again */
        if(NULL==(ret_value=H5MM_malloc((size_t)mem_size)))
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for chunk");
    } /* end if */

done:
    FUNC_LEAVE (ret_value);
}   /* end H5FL_malloc() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_reg_init
 *
 * Purpose:     Initialize a free list for a certain type.  Right now, this just
 *      adds the free list to the list of things to garbage collect.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, March 24, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
static herr_t
H5FL_reg_init(H5FL_reg_head_t *head)
{
    H5FL_reg_gc_node_t *new_node;   /* Pointer to the node for the new list to garbage collect */
    herr_t ret_value=SUCCEED;   /* return value*/

    FUNC_ENTER (H5FL_reg_init, FAIL);

    /* Allocate a new garbage collection node */
    if (NULL==(new_node = H5MM_malloc(sizeof(H5FL_reg_gc_node_t))))
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");

    /* Initialize the new garbage collection node */
    new_node->list=head;

    /* Link in to the garbage collection list */
    new_node->next=H5FL_reg_gc_head.first;
    H5FL_reg_gc_head.first=new_node;

    /* Indicate that the free list is initialized */
    head->init=1;

    FUNC_LEAVE (ret_value);
}   /* end H5FL_reg_init() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_reg_free
 *
 * Purpose:     Release an object & put on free list
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, March 24, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
void *
H5FL_reg_free(H5FL_reg_head_t *head, void *obj)
{
    H5FL_reg_node_t *temp;      /* Temp. ptr to the new free list node allocated */

    FUNC_ENTER (H5FL_reg_free, NULL);

    /* Double check parameters */
    assert(head);
    assert(obj);

#ifdef H5FL_DEBUG
    HDmemset(obj,255,head->size);
#endif /* H5FL_DEBUG */

#ifdef NO_REG_FREE_LISTS
    H5MM_xfree(obj);
#else /* NO_REG_FREE_LISTS */
    /* Make certain that the free list is initialized */
    assert(head->init);

    /* Get the pointer to the info header in front of the block to free */
    temp=(H5FL_reg_node_t *)((unsigned char *)obj-sizeof(H5FL_reg_node_t));

#ifdef H5FL_DEBUG
    assert(temp->inuse);
    temp->inuse=0;
#endif /* H5FL_DEBUG */

    /* Link into the free list */
    temp->next=head->list;

    /* Point free list at the node freed */
    head->list=temp;

    /* Increment the number of blocks & memory on free list */
    head->onlist++;
    head->list_mem+=head->size;

    /* Increment the amount of "regular" freed memory globally */
    H5FL_reg_gc_head.mem_freed+=head->size;

    /* Check for exceeding free list memory use limits */
    /* First check this particular list */
    if(head->list_mem>H5FL_reg_lst_mem_lim)
        H5FL_reg_gc_list(head);

    /* Then check the global amount memory on regular free lists */
    if(H5FL_reg_gc_head.mem_freed>H5FL_reg_glb_mem_lim)
        H5FL_reg_gc();

#endif /* NO_REG_FREE_LISTS */

    FUNC_LEAVE(NULL);
}   /* end H5FL_reg_free() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_reg_alloc
 *
 * Purpose:     Allocate a block on a free list
 *
 * Return:      Success:        Pointer to a valid object
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              Friday, March 24, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
void *
H5FL_reg_alloc(H5FL_reg_head_t *head, unsigned clear)
{
    H5FL_reg_node_t *new_obj;   /* Pointer to the new free list node allocated */
    void *ret_value;        /* Pointer to object to return */

    FUNC_ENTER (H5FL_reg_alloc, NULL);

    /* Double check parameters */
    assert(head);

#ifdef NO_REG_FREE_LISTS
    if(clear)
        ret_value=H5MM_calloc(head->size);
    else
        ret_value=H5MM_malloc(head->size);
#else /* NO_REG_FREE_LISTS */
    /* Make certain the list is initialized first */
    if(!head->init)
        H5FL_reg_init(head);

    /* Check for nodes available on the free list first */
    if(head->list!=NULL) {
        /* Get a pointer to the block on the free list */
        ret_value=((char *)(head->list))+sizeof(H5FL_reg_node_t);

#ifdef H5FL_DEBUG
        head->list->inuse=1;
#endif /* H5FL_DEBUG */

        /* Remove node from free list */
        head->list=head->list->next;

        /* Decrement the number of blocks & memory on free list */
        head->onlist--;
        head->list_mem-=head->size;

        /* Decrement the amount of global "regular" free list memory in use */
        H5FL_reg_gc_head.mem_freed-=(head->size);

    } /* end if */
    /* Otherwise allocate a node */
    else {
        if (NULL==(new_obj = H5FL_malloc(sizeof(H5FL_reg_node_t)+head->size)))
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

#ifdef H5FL_DEBUG
        new_obj->inuse=1;
#endif /* H5FL_DEBUG */

        /* Increment the number of blocks allocated in list */
        head->allocated++;

        /* Get a pointer to the new block */
        ret_value=((char *)new_obj)+sizeof(H5FL_reg_node_t);
    } /* end else */

    /* Clear to zeros, if asked */
    if(clear) {
        assert(head->size==(hsize_t)((size_t)head->size)); /*check for overflow*/
        HDmemset(ret_value,0,(size_t)head->size);
    } /* end if */
#endif /* NO_REG_FREE_LISTS */

    FUNC_LEAVE (ret_value);
}   /* end H5FL_reg_alloc() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_reg_gc_list
 *
 * Purpose:     Garbage collect on a particular object free list
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, July 25, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
static herr_t
H5FL_reg_gc_list(H5FL_reg_head_t *head)
{
    H5FL_reg_node_t *free_list; /* Pointer to nodes in free list being garbage collected */
    void *tmp;          /* Temporary node pointer */
    size_t total_mem;   /* Total memory used on list */
    
    /* FUNC_ENTER_INIT() should not be called, it causes an infinite loop at library termination */

    /* Calculate the total memory used on this list */
    total_mem=head->onlist*head->size;

    /* For each free list being garbage collected, walk through the nodes and free them */
    free_list=head->list;
    while(free_list!=NULL) {
        tmp=free_list->next;

        /* Decrement the count of nodes allocated and free the node */
        head->allocated--;

        /* Decrement count of free memory on this list */
        head->list_mem-=head->size;

#ifdef H5FL_DEBUG
        assert(!free_list->inuse);
#endif /* H5FL_DEBUG */

        H5MM_xfree(free_list);

        free_list=tmp;
    } /* end while */

    /* Double check that all the memory on this list is recycled */
    assert(head->list_mem==0);

    /* Indicate no free nodes on the free list */
    head->list=NULL;
    head->onlist=0;

    /* Decrement global count of free memory on "regular" lists */
    H5FL_reg_gc_head.mem_freed-=total_mem;

    return(SUCCEED);
}   /* end H5FL_reg_gc_list() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_reg_gc
 *
 * Purpose:     Garbage collect on all the object free lists
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, March 24, 2000
 *
 * Modifications:
 *  Broke into two parts, one for looping over all the free lists and
 *      another for freeing each list - QAK 7/25/00
 *      
 *-------------------------------------------------------------------------
 */
static herr_t
H5FL_reg_gc(void)
{
    H5FL_reg_gc_node_t *gc_node;    /* Pointer into the list of things to garbage collect */
    
    /* FUNC_ENTER_INIT() should not be called, it causes an infinite loop at library termination */

    /* Walk through all the free lists, free()'ing the nodes */
    gc_node=H5FL_reg_gc_head.first;
    while(gc_node!=NULL) {
        /* Release the free nodes on the list */
        H5FL_reg_gc_list(gc_node->list);

        /* Go on to the next free list to garbage collect */
        gc_node=gc_node->next;
    } /* end while */

    /* Double check that all the memory on the free lists is recycled */
    assert(H5FL_reg_gc_head.mem_freed==0);

    return(SUCCEED);
}   /* end H5FL_reg_gc() */


/*--------------------------------------------------------------------------
 NAME
    H5FL_reg_term
 PURPOSE
    Terminate various H5FL object free lists
 USAGE
    int H5FL_term()
 RETURNS
    Success:    Positive if any action might have caused a change in some
                other interface; zero otherwise.
        Failure:        Negative
 DESCRIPTION
    Release any resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
        Robb Matzke, 2000-04-25
        If a list cannot be freed because something is using it then return
        zero (failure to free a list doesn't affect any other part of the
        library). If some other layer frees something during its termination
        it will return non-zero, which will cause this function to get called
        again to reclaim this layer's memory.
--------------------------------------------------------------------------*/
static int
H5FL_reg_term(void)
{
    H5FL_reg_gc_node_t *left;   /* pointer to garbage collection lists with work left */
    H5FL_reg_gc_node_t *tmp;    /* Temporary pointer to a garbage collection node */

    if (interface_initialize_g) {
        /* Free the nodes on the garbage collection list, keeping nodes with allocations outstanding */
        left=NULL;
        while(H5FL_reg_gc_head.first!=NULL) {
            tmp=H5FL_reg_gc_head.first->next;

#ifdef H5FL_DEBUG
            printf("H5FL_reg_term: head->name=%s, head->allocated=%d\n", H5FL_reg_gc_head.first->list->name,(int)H5FL_reg_gc_head.first->list->allocated);
#endif /* H5FL_DEBUG */
            /* Check if the list has allocations outstanding */
            if(H5FL_reg_gc_head.first->list->allocated>0) {
                /* Add free list to the list of nodes with allocations open still */
                H5FL_reg_gc_head.first->next=left;
                left=H5FL_reg_gc_head.first;
            } /* end if */
            /* No allocations left open for list, get rid of it */
            else {
                /* Reset the "initialized" flag, in case we restart this list somehow (I don't know how..) */
                H5FL_reg_gc_head.first->list->init=0;

                /* Free the node from the garbage collection list */
                H5MM_xfree(H5FL_reg_gc_head.first);
            } /* end else */

            H5FL_reg_gc_head.first=tmp;
        } /* end while */

        /* Point to the list of nodes left with allocations open, if any */
        H5FL_reg_gc_head.first=left;
        if (!left)
            interface_initialize_g = 0; /*this layer has reached its initial state*/
    }

    /* Terminating this layer never affects other layers; rather, other layers affect
     * the termination of this layer. */
    return(0);
}   /* end H5FL_reg_term() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_blk_find_list
 *
 * Purpose:     Finds the free list for blocks of a given size.  Also moves that
 *      free list node to the head of the priority queue (if it isn't there
 *      already).  This routine does not manage the actual free list, it just
 *      works with the priority queue.
 *
 * Return:      Success:        valid pointer to the free list node
 *
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              Thursday, March  23, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5FL_blk_node_t *
H5FL_blk_find_list(H5FL_blk_node_t **head, hsize_t size)
{
    H5FL_blk_node_t *temp;  /* Temp. pointer to node in the native list */
    H5FL_blk_node_t *ret_value=NULL;

    FUNC_ENTER(H5FL_blk_find_list, NULL);

    /* Find the correct free list */
    temp=*head;
    while(temp!=NULL && temp->size!=size)
        temp=temp->next;

    /* If the free list exists, move it to the front of the queue, if it's not there already */
    if(temp!=NULL && temp!=*head) {
        /* Take the node found out of it's current position */
        if(temp->next==NULL) {
            temp->prev->next=NULL;
        } /* end if */
        else {
            temp->prev->next=temp->next;
            temp->next->prev=temp->prev;
        } /* end else */

        /* Move the found node to the head of the list */
        temp->prev=NULL;
        temp->next=*head;
        (*head)->prev=temp;
        *head=temp;
    } /* end if */
    
    ret_value=temp;

    FUNC_LEAVE(ret_value);
} /* end H5FL_blk_find_list() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_blk_create_list
 *
 * Purpose:     Creates a new free list for blocks of the given size at the
 *      head of the priority queue.
 *
 * Return:      Success:        valid pointer to the free list node
 *
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              Thursday, March  23, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5FL_blk_node_t *
H5FL_blk_create_list(H5FL_blk_node_t **head, hsize_t size)
{
    H5FL_blk_node_t *temp;  /* Temp. pointer to node in the list */
    H5FL_blk_node_t *ret_value=NULL;

    FUNC_ENTER(H5FL_blk_create_list, NULL);

    /* Allocate room for the new free list node */
    if(NULL==(temp=H5FL_ALLOC(H5FL_blk_node_t,0)))
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for chunk info");
    
    /* Set the correct values for the new free list */
    temp->size=size;
    temp->list=NULL;

    /* Attach to head of priority queue */
    if(*head==NULL) {
        *head=temp;
        temp->next=temp->prev=NULL;
    } /* end if */
    else {
        temp->next=*head;
        (*head)->prev=temp;
        temp->prev=NULL;
        *head=temp;
    } /* end else */

    ret_value=temp;

done:
    FUNC_LEAVE(ret_value);
} /* end H5FL_blk_create_list() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_blk_init
 *
 * Purpose:     Initialize a priority queue of a certain type.  Right now, this just
 *      adds the PQ to the list of things to garbage collect.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 25, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
static herr_t
H5FL_blk_init(H5FL_blk_head_t *head)
{
    H5FL_blk_gc_node_t *new_node;   /* Pointer to the node for the new list to garbage collect */
    herr_t ret_value=SUCCEED;       /* return value*/

    FUNC_ENTER (H5FL_blk_init, FAIL);

    /* Allocate a new garbage collection node */
    if (NULL==(new_node = H5MM_malloc(sizeof(H5FL_blk_gc_node_t))))
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");

    /* Initialize the new garbage collection node */
    new_node->pq=head;

    /* Link in to the garbage collection list */
    new_node->next=H5FL_blk_gc_head.first;
    H5FL_blk_gc_head.first=new_node;

    /* Indicate that the PQ is initialized */
    head->init=1;

    FUNC_LEAVE (ret_value);
}   /* end H5FL_blk_init() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_blk_alloc
 *
 * Purpose:     Allocates memory for a block.  This routine is used
 *      instead of malloc because the block can be kept on a free list so
 *      they don't thrash malloc/free as much.
 *
 * Return:      Success:        valid pointer to the block
 *
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              Thursday, March  23, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5FL_blk_alloc(H5FL_blk_head_t *head, hsize_t size, unsigned clear)
{
    H5FL_blk_node_t *free_list;  /* The free list of nodes of correct size */
    H5FL_blk_list_t *temp;  /* Temp. ptr to the new native list allocated */
    void *ret_value;    /* Pointer to the block to return to the user */

    FUNC_ENTER(H5FL_blk_alloc, NULL);

    /* Double check parameters */
    assert(head);

#ifdef NO_BLK_FREE_LISTS
    if(clear)
        ret_value=H5MM_calloc(size);
    else
        ret_value=H5MM_malloc(size);
#else /* NO_BLK_FREE_LISTS */
    /* Make certain the list is initialized first */
    if(!head->init)
        H5FL_blk_init(head);

    /* check if there is a free list for blocks of this size */
    /* and if there are any blocks available on the list */
    if((free_list=H5FL_blk_find_list(&(head->head),size))!=NULL && free_list->list!=NULL) {
        /* Remove the first node from the list and return it */
        ret_value=((char *)(free_list->list))+sizeof(H5FL_blk_list_t);
        free_list->list=free_list->list->next;

        /* Decrement the number of blocks & memory used on free list */
        head->onlist--;
        head->list_mem-=size;

        /* Decrement the amount of global "block" free list memory in use */
        H5FL_blk_gc_head.mem_freed-=size;

    } /* end if */
    /* No free list available, or there are no nodes on the list, allocate a new node to give to the user */
    else { 
        /* Allocate new node, with room for the page info header and the actual page data */
                if(NULL==(temp=H5FL_malloc(sizeof(H5FL_blk_list_t)+size)))
                    HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for chunk");

        /* Increment the number of blocks allocated */
        head->allocated++;

        /* Initialize the block allocated */
        temp->size=size;
        temp->next=NULL;

        /* Set the return value to the block itself */
        ret_value=((char *)temp)+sizeof(H5FL_blk_list_t);
    } /* end else */

    /* Clear the block to zeros, if requested */
    if(clear) {
        assert(size==(hsize_t)((size_t)size)); /*check for overflow*/
        HDmemset(ret_value,0,(size_t)size);
    } /* end if */
#endif /* NO_BLK_FREE_LISTS */

done:
    FUNC_LEAVE(ret_value);
} /* end H5FL_blk_alloc() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_blk_free
 *
 * Purpose:     Releases memory for a block.  This routine is used
 *      instead of free because the blocks can be kept on a free list so
 *      they don't thrash malloc/free as much.
 *
 * Return:      Success:        NULL
 *
 *              Failure:        never fails
 *
 * Programmer:  Quincey Koziol
 *              Thursday, March  23, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5FL_blk_free(H5FL_blk_head_t *head, void *block)
{
    H5FL_blk_node_t *free_list;      /* The free list of nodes of correct size */
    H5FL_blk_list_t *temp;      /* Temp. ptr to the new free list node allocated */

    FUNC_ENTER(H5FL_blk_free, NULL);

    /* Double check parameters */
    assert(head);
    assert(block);

#ifdef NO_BLK_FREE_LISTS
    H5MM_xfree(block);
#else /* NO_BLK_FREE_LISTS */
    /* Get the pointer to the native block info header in front of the native block to free */
    temp=(H5FL_blk_list_t *)((unsigned char *)block-sizeof(H5FL_blk_list_t));

    /* check if there is a free list for native blocks of this size */
    if((free_list=H5FL_blk_find_list(&(head->head),temp->size))==NULL) {
        /* No free list available, create a new list node and insert it to the queue */
        free_list=H5FL_blk_create_list(&(head->head),temp->size);
    } /* end if */

    /* Prepend the free'd native block to the front of the free list */
    if(free_list!=NULL) {
        temp->next=free_list->list;
        free_list->list=temp;
    } /* end if */

    /* Increment the number of blocks on free list */
    head->onlist++;
    head->list_mem+=temp->size;

    /* Increment the amount of "block" freed memory globally */
    H5FL_blk_gc_head.mem_freed+=temp->size;

    /* Check for exceeding free list memory use limits */
    /* First check this particular list */
    if(head->list_mem>H5FL_blk_lst_mem_lim) {
#ifdef QAK
printf("%s: temp->size=%u, head->name=%s, head->list_mem=%u, H5FL_blk_gc_head.mem_freed=%u, garbage collecting list\n",FUNC,(unsigned)temp->size,head->name,(unsigned)head->list_mem,(unsigned)H5FL_blk_gc_head.mem_freed);
#endif /* QAK */
        H5FL_blk_gc_list(head);
    } /* end if */

    /* Then check the global amount memory on block free lists */
    if(H5FL_blk_gc_head.mem_freed>H5FL_blk_glb_mem_lim) {
#ifdef QAK
printf("%s: head->name=%s, garbage collecting all block lists\n",FUNC,head->name);
#endif /* QAK */
        H5FL_blk_gc();
    } /* end if */

#endif /* NO_BLK_FREE_LISTS */

    FUNC_LEAVE(NULL);
} /* end H5FL_blk_free() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_blk_realloc
 *
 * Purpose:     Resizes a block.  This does things the straightforward, simple way,
 *      not actually using realloc.
 *
 * Return:      Success:        NULL
 *
 *              Failure:        never fails
 *
 * Programmer:  Quincey Koziol
 *              Thursday, March  23, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5FL_blk_realloc(H5FL_blk_head_t *head, void *block, hsize_t new_size)
{
    hsize_t blk_size;           /* Temporary block size */
    H5FL_blk_list_t *temp;      /* Temp. ptr to the new block node allocated */
    void *ret_value=NULL;       /* Return value */

    FUNC_ENTER(H5FL_blk_realloc, NULL);

    /* Double check parameters */
    assert(head);

#ifdef NO_BLK_FREE_LISTS
    ret_value=H5MM_realloc(block,new_size);
#else /* NO_BLK_FREE_LISTS */
    /* Check if we are actually re-allocating a block */
    if(block!=NULL) {
        /* Get the pointer to the chunk info header in front of the chunk to free */
        temp=(H5FL_blk_list_t *)((unsigned char *)block-sizeof(H5FL_blk_list_t));

        /* check if we are actually changing the size of the buffer */
        if(new_size!=temp->size) {
            if((ret_value=H5FL_blk_alloc(head,new_size,0))==NULL)
                HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed for block");
            blk_size=MIN(new_size,temp->size);
            assert(blk_size==(hsize_t)((size_t)blk_size)); /*check for overflow*/
            HDmemcpy(ret_value,block,(size_t)blk_size);
            H5FL_blk_free(head,block);
        } /* end if */
        else
            ret_value=block;
    } /* end if */
    /* Not re-allocating, just allocate a fresh block */
    else
        ret_value=H5FL_blk_alloc(head,new_size,0);
#endif /* NO_BLK_FREE_LISTS */

done:
    FUNC_LEAVE(ret_value);
} /* end H5FL_blk_realloc() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_blk_gc_list
 *
 * Purpose:     Garbage collect a priority queue
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Thursday, March 23, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
static herr_t
H5FL_blk_gc_list(H5FL_blk_head_t *head)
{
    H5FL_blk_list_t *list; /* The free list of native nodes of a particular size */
    void *next;     /* Temp. ptr to the free list list node */
    void *temp;     /* Temp. ptr to the free list page node */
    
    /* FUNC_ENTER_INIT() should not be called, it causes an infinite loop at library termination */

    /* Loop through all the nodes in the block free list queue */
    while(head->head!=NULL) {
        temp=head->head->next;

        /* Loop through all the blocks in the free list, freeing them */
        list=head->head->list;
        while(list!=NULL) {
            next=list->next;

            /* Decrement the number of blocks & memory allocated from this PQ */
            head->allocated--;
            head->list_mem-=list->size;

            /* Decrement global count of free memory on "block" lists */
            H5FL_blk_gc_head.mem_freed-=list->size;

            /* Free the block */
            H5MM_xfree(list);

            list=next;
        } /* end while */

        /* Free the free list node */
        H5FL_FREE(H5FL_blk_node_t,head->head);

        /* Advance to the next free list */
        head->head=temp;
    } /* end while */

    /* Indicate no free nodes on the free list */
    head->head=NULL;
    head->onlist=0;

    /* Double check that all the memory on this list is recycled */
    assert(head->list_mem==0);

    return(SUCCEED);
}   /* end H5FL_blk_gc_list() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_blk_gc
 *
 * Purpose:     Garbage collect on all the priority queues
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 25, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
static herr_t
H5FL_blk_gc(void)
{
    H5FL_blk_gc_node_t *gc_node;    /* Pointer into the list of things to garbage collect */
    
    /* FUNC_ENTER_INIT() should not be called, it causes an infinite loop at library termination */

    /* Walk through all the free lists, free()'ing the nodes */
    gc_node=H5FL_blk_gc_head.first;
    while(gc_node!=NULL) {
        /* For each free list being garbage collected, walk through the nodes and free them */
        H5FL_blk_gc_list(gc_node->pq);

        /* Go on to the next free list to garbage collect */
        gc_node=gc_node->next;
    } /* end while */

    /* Double check that all the memory on the free lists are recycled */
    assert(H5FL_blk_gc_head.mem_freed==0);

    return(SUCCEED);
}   /* end H5FL_blk_gc() */


/*--------------------------------------------------------------------------
 NAME
    H5FL_blk_term
 PURPOSE
    Terminate various H5FL_blk objects
 USAGE
    void H5FL_blk_term()
 RETURNS
    Success:    Positive if any action might have caused a change in some
                other interface; zero otherwise.
        Failure:        Negative
 DESCRIPTION
    Release any resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5FL_blk_term(void)
{
    H5FL_blk_gc_node_t *left;   /* pointer to garbage collection lists with work left */
    H5FL_blk_gc_node_t *tmp;    /* Temporary pointer to a garbage collection node */
    
    /* Free the nodes on the garbage collection list, keeping nodes with allocations outstanding */
    left=NULL;
    while(H5FL_blk_gc_head.first!=NULL) {
        tmp=H5FL_blk_gc_head.first->next;

#ifdef H5FL_DEBUG
printf("H5FL_blk_term: head->name=%s, head->allocated=%d\n", H5FL_blk_gc_head.first->pq->name,(int)H5FL_blk_gc_head.first->pq->allocated);
#endif /* H5FL_DEBUG */

        /* Check if the list has allocations outstanding */
        if(H5FL_blk_gc_head.first->pq->allocated>0) {
            /* Add free list to the list of nodes with allocations open still */
            H5FL_blk_gc_head.first->next=left;
            left=H5FL_blk_gc_head.first;
        } /* end if */
        /* No allocations left open for list, get rid of it */
        else {
            /* Reset the "initialized" flag, in case we restart this list somehow (I don't know how..) */
            H5FL_blk_gc_head.first->pq->init=0;

            /* Free the node from the garbage collection list */
            H5MM_xfree(H5FL_blk_gc_head.first);
        } /* end else */

        H5FL_blk_gc_head.first=tmp;
    } /* end while */

    /* Point to the list of nodes left with allocations open, if any */
    H5FL_blk_gc_head.first=left;
    
    return (H5FL_blk_gc_head.first!=NULL ? 1 : 0);
}   /* end H5FL_blk_term() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_arr_init
 *
 * Purpose:     Initialize a free list for a arrays of certain type.  Right now,
 *      this just adds the free list to the list of things to garbage collect.
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 25, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
static herr_t
H5FL_arr_init(H5FL_arr_head_t *head)
{
    H5FL_gc_arr_node_t *new_node;   /* Pointer to the node for the new list to garbage collect */
    herr_t ret_value=SUCCEED;       /* return value*/

    FUNC_ENTER (H5FL_arr_init, FAIL);

    /* Allocate a new garbage collection node */
    if (NULL==(new_node = H5MM_malloc(sizeof(H5FL_gc_arr_node_t))))
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");

    /* Initialize the new garbage collection node */
    new_node->list=head;

    /* Link in to the garbage collection list */
    new_node->next=H5FL_arr_gc_head.first;
    H5FL_arr_gc_head.first=new_node;

    /* Allocate room for the free lists, if the arrays have a maximum size */
    if(head->maxelem>0) {
        if (NULL==(head->u.list_arr = H5MM_calloc(head->maxelem*sizeof(H5FL_arr_node_t *))))
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");
        if (NULL==(head->onlist = H5MM_calloc(head->maxelem*sizeof(unsigned))))
            HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL, "memory allocation failed");
    } /* end if */
    else {
        head->u.queue.init=0;
        head->u.queue.allocated=0;
        head->u.queue.onlist=0;
        head->u.queue.list_mem=0;
        head->u.queue.name=head->name;
        head->u.queue.head=NULL;
    } /* end else */
    
    /* Indicate that the free list is initialized */
    head->init=1;

    FUNC_LEAVE (ret_value);
}   /* end H5FL_arr_init() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_arr_free
 *
 * Purpose:     Release an array of objects & put on free list
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, March 24, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
void *
H5FL_arr_free(H5FL_arr_head_t *head, void *obj)
{
    H5FL_arr_node_t *temp;  /* Temp. ptr to the new free list node allocated */
    size_t mem_size;        /* Size of memory being freed */

    FUNC_ENTER (H5FL_arr_free, NULL);

#ifdef NO_ARR_FREE_LISTS
    H5MM_xfree(obj);
#else /* NO_ARR_FREE_LISTS */
    /* The H5MM_xfree code allows obj to null */
    if (!obj)
        HRETURN(NULL);

    /* Double check parameters */
    assert(head);

    /* Make certain that the free list is initialized */
    assert(head->init);

    /* Check if there are a maximum number of elements in list */
    if(head->maxelem>0) {
        /* Get the pointer to the info header in front of the block to free */
        temp=(H5FL_arr_node_t *)((unsigned char *)obj-sizeof(H5FL_arr_node_t));

        /* Double-check that there is enough room for arrays of this size */
        assert((int)temp->nelem<=head->maxelem);

        /* Link into the free list */
        temp->next=head->u.list_arr[temp->nelem];

        /* Point free list at the node freed */
        head->u.list_arr[temp->nelem]=temp;

        /* Set the amount of memory being freed */
        mem_size=temp->nelem*head->size;

        /* Increment the number of blocks & memory used on free list */
        head->onlist[temp->nelem]++;
        head->list_mem+=mem_size;

        /* Increment the amount of "array" freed memory globally */
        H5FL_arr_gc_head.mem_freed+=mem_size;

        /* Check for exceeding free list memory use limits */
        /* First check this particular list */
        if(head->list_mem>H5FL_arr_lst_mem_lim)
            H5FL_arr_gc_list(head);

        /* Then check the global amount memory on array free lists */
        if(H5FL_arr_gc_head.mem_freed>H5FL_arr_glb_mem_lim)
            H5FL_arr_gc();

    } /* end if */
    /* No maximum number of elements, use block routine */
    else {
        H5FL_blk_free(&(head->u.queue),obj);
    } /* end else */
#endif /* NO_ARR_FREE_LISTS */

    FUNC_LEAVE(NULL);
}   /* end H5FL_arr_free() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_arr_alloc
 *
 * Purpose:     Allocate an array of objects
 *
 * Return:      Success:        Pointer to a valid array object
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 25, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
void *
H5FL_arr_alloc(H5FL_arr_head_t *head, hsize_t elem, unsigned clear)
{
    H5FL_arr_node_t *new_obj;   /* Pointer to the new free list node allocated */
    void *ret_value;        /* Pointer to object to return */
    hsize_t mem_size;        /* Size of memory block being recycled */

    FUNC_ENTER (H5FL_arr_alloc, NULL);

    /* Double check parameters */
    assert(head);

#ifdef NO_ARR_FREE_LISTS
    if(clear)
        ret_value=H5MM_calloc(elem*head->size);
    else
        ret_value=H5MM_malloc(elem*head->size);
#else /* NO_ARR_FREE_LISTS */
    /* Make certain the list is initialized first */
    if(!head->init)
        H5FL_arr_init(head);

    /* Set the set of the memory block */
    mem_size=head->size*elem;

    /* Check if there is a maximum number of elements in array */
    if(head->maxelem>0) {
        /* Check for nodes available on the free list first */
        if(head->u.list_arr[elem]!=NULL) {
            /* Get a pointer to the block on the free list */
            ret_value=((char *)(head->u.list_arr[elem]))+sizeof(H5FL_arr_node_t);

            /* Remove node from free list */
            head->u.list_arr[elem]=head->u.list_arr[elem]->next;

            /* Decrement the number of blocks & memory used on free list */
            head->onlist[elem]--;
            head->list_mem-=mem_size;

            /* Decrement the amount of global "array" free list memory in use */
            H5FL_arr_gc_head.mem_freed-=mem_size;

        } /* end if */
        /* Otherwise allocate a node */
        else {
            if (NULL==(new_obj = H5FL_malloc(sizeof(H5FL_arr_node_t)+mem_size)))
                HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL, "memory allocation failed");

            /* Increment the number of blocks allocated in list */
            head->allocated++;

            /* Initialize the new object */
            new_obj->nelem=elem;
            new_obj->next=NULL;

            /* Get a pointer to the new block */
            ret_value=((char *)new_obj)+sizeof(H5FL_arr_node_t);
        } /* end else */

        /* Clear to zeros, if asked */
        if(clear) {
            assert(mem_size==(hsize_t)((size_t)mem_size)); /*check for overflow*/
            HDmemset(ret_value,0,(size_t)mem_size);
        } /* end if */
    } /* end if */
    /* No fixed number of elements, use PQ routine */
    else {
        ret_value=H5FL_blk_alloc(&(head->u.queue),mem_size,clear);
    } /* end else */
#endif /* NO_ARR_FREE_LISTS */

    FUNC_LEAVE (ret_value);
}   /* end H5FL_arr_alloc() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_arr_realloc
 *
 * Purpose:     Reallocate an array of objects
 *
 * Return:      Success:        Pointer to a valid array object
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 25, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
void *
H5FL_arr_realloc(H5FL_arr_head_t *head, void * obj, hsize_t new_elem)
{
    hsize_t blk_size;       /* Size of block */
    H5FL_arr_node_t *temp;  /* Temp. ptr to the new free list node allocated */
    void *ret_value;        /* Pointer to object to return */

    FUNC_ENTER (H5FL_arr_realloc, NULL);

    /* Double check parameters */
    assert(head);

#ifdef NO_ARR_FREE_LISTS
    ret_value=H5MM_realloc(obj,new_elem*head->size);
#else /* NO_ARR_FREE_LISTS */
    /* Check if we are really allocating the object */
    if(obj==NULL) {
        ret_value=H5FL_arr_alloc(head,new_elem,0);
    } /* end if */
    else {
        /* Check if there is a maximum number of elements in array */
        if(head->maxelem>0) {
            /* Get the pointer to the info header in front of the block to free */
            temp=(H5FL_arr_node_t *)((unsigned char *)obj-sizeof(H5FL_arr_node_t));

            /* Check if the size is really changing */
            if(temp->nelem!=new_elem) {
                /* Get the new array of objects */
                ret_value=H5FL_arr_alloc(head,new_elem,0);

                /* Copy the appropriate amount of elements */
                blk_size=head->size*MIN(temp->nelem,new_elem);
                assert(blk_size==(hsize_t)((size_t)blk_size)); /*check for overflow*/
                HDmemcpy(ret_value,obj,(size_t)blk_size);

                /* Free the old block */
                H5FL_arr_free(head,obj);
            } /* end if */
            else
                ret_value=obj;
        } /* end if */
        /* No fixed number of elements, use block routine */
        else {
            ret_value=H5FL_blk_realloc(&(head->u.queue),obj,head->size*new_elem);
        } /* end else */
    } /* end else */
#endif /* NO_ARR_FREE_LISTS */

    FUNC_LEAVE (ret_value);
}   /* end H5FL_arr_realloc() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_arr_gc_list
 *
 * Purpose:     Garbage collect on an array object free list
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Tuesday, July 25, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
static herr_t
H5FL_arr_gc_list(H5FL_arr_head_t *head)
{
    H5FL_arr_node_t *arr_free_list; /* Pointer to nodes in free list being garbage collected */
    void *tmp;      /* Temporary node pointer */
    int i;         /* Counter for array of free lists */
    size_t total_mem;   /* Total memory used on list */
    
    /* FUNC_ENTER_INIT() should not be called, it causes an infinite loop at library termination */

    /* Check if the array has a fixed maximum number of elements */
    if(head->maxelem>0) {
        /* Walk through the array of free lists */
        for(i=0; i<head->maxelem; i++) {
            if(head->onlist[i]>0) {
                /* Calculate the total memory used on this list */
                total_mem=head->onlist[i]*i*head->size;

                /* For each free list being garbage collected, walk through the nodes and free them */
                arr_free_list=head->u.list_arr[i];
                while(arr_free_list!=NULL) {
                    tmp=arr_free_list->next;

                    /* Decrement the count of nodes allocated and free the node */
                    head->allocated--;
                    H5MM_xfree(arr_free_list);

                    arr_free_list=tmp;
                } /* end while */

                /* Indicate no free nodes on the free list */
                head->u.list_arr[i]=NULL;
                head->onlist[i]=0;

                /* Decrement count of free memory on this "array" list */
                head->list_mem-=total_mem;

                /* Decrement global count of free memory on "array" lists */
                H5FL_arr_gc_head.mem_freed-=total_mem;
            } /* end if */

        } /* end for */

        /* Double check that all the memory on this list is recycled */
        assert(head->list_mem==0);

    } /* end if */
    /* No maximum number of elements, use the block call to garbage collect */
    else {
        H5FL_blk_gc_list(&(head->u.queue));
    } /* end else */

    return(SUCCEED);
}   /* end H5FL_arr_gc_list() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_arr_gc
 *
 * Purpose:     Garbage collect on all the array object free lists
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Saturday, March 25, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
static herr_t
H5FL_arr_gc(void)
{
    H5FL_gc_arr_node_t *gc_arr_node;    /* Pointer into the list of things to garbage collect */
    
    /* FUNC_ENTER_INIT() should not be called, it causes an infinite loop at library termination */

    /* Walk through all the free lists, free()'ing the nodes */
    gc_arr_node=H5FL_arr_gc_head.first;
    while(gc_arr_node!=NULL) {
        /* Release the free nodes on the list */
        H5FL_arr_gc_list(gc_arr_node->list);

        /* Go on to the next free list to garbage collect */
        gc_arr_node=gc_arr_node->next;
    } /* end while */

    /* Double check that all the memory on the free lists are recycled */
    assert(H5FL_arr_gc_head.mem_freed==0);

    return(SUCCEED);
}   /* end H5FL_arr_gc() */


/*--------------------------------------------------------------------------
 NAME
    H5FL_arr_term
 PURPOSE
    Terminate various H5FL array object free lists
 USAGE
    int H5FL_arr_term()
 RETURNS
    Success:    Positive if any action might have caused a change in some
                other interface; zero otherwise.
        Failure:        Negative
 DESCRIPTION
    Release any resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
static int
H5FL_arr_term(void)
{
    H5FL_gc_arr_node_t *left;   /* pointer to garbage collection lists with work left */
    H5FL_gc_arr_node_t *tmp;    /* Temporary pointer to a garbage collection node */
    
    /* Free the nodes on the garbage collection list, keeping nodes with allocations outstanding */
    left=NULL;
    while(H5FL_arr_gc_head.first!=NULL) {
        tmp=H5FL_arr_gc_head.first->next;

        /* Check if the array has a fixed maximum number of elements */
        if(H5FL_arr_gc_head.first->list->maxelem>0) {
            /* Check if the list has allocations outstanding */
#ifdef H5FL_DEBUG
printf("H5FL_arr_term: head->name=%s, head->allocated=%d\n", H5FL_arr_gc_head.first->list->name,(int)H5FL_arr_gc_head.first->list->allocated);
#endif /* H5FL_DEBUG */
            if(H5FL_arr_gc_head.first->list->allocated>0) {
                /* Add free list to the list of nodes with allocations open still */
                H5FL_arr_gc_head.first->next=left;
                left=H5FL_arr_gc_head.first;
            } /* end if */
            /* No allocations left open for list, get rid of it */
            else {
                /* Free the array of free lists */
                H5MM_xfree(H5FL_arr_gc_head.first->list->u.list_arr);

                /* Free the array of "onlist" counts */
                H5MM_xfree(H5FL_arr_gc_head.first->list->onlist);

                /* Reset the "initialized" flag, in case we restart this list somehow (I don't know how..) */
                H5FL_arr_gc_head.first->list->init=0;

                /* Free the node from the garbage collection list */
                H5MM_xfree(H5FL_arr_gc_head.first);
            } /* end else */
        } /* end if */
        /* No maximum number of elements, use the PQ information */
        else {
#ifdef H5FL_DEBUG
printf("H5FL_arr_term: head->name=%s, head->allocated=%d\n", H5FL_arr_gc_head->list->name,(int)H5FL_arr_gc_head->list->u.queue.allocated);
#endif /* H5FL_DEBUG */
            /* Check if the list has allocations outstanding */
            if(H5FL_arr_gc_head.first->list->u.queue.allocated>0) {
                /* Add free list to the list of nodes with allocations open still */
                H5FL_arr_gc_head.first->next=left;
                left=H5FL_arr_gc_head.first;
            } /* end if */
            /* No allocations left open for list, get rid of it */
            else {
                /* Reset the "initialized" flag, in case we restart this list somehow (I don't know how..) */
                H5FL_arr_gc_head.first->list->init=0;

                /* Free the node from the garbage collection list */
                H5MM_xfree(H5FL_arr_gc_head.first);
            } /* end else */
        } /* end else */

        H5FL_arr_gc_head.first=tmp;
    } /* end while */

    /* Point to the list of nodes left with allocations open, if any */
    H5FL_arr_gc_head.first=left;
    
    return (H5FL_arr_gc_head.first!=NULL ? 1 : 0);
}   /* end H5FL_arr_term() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_garbage_coll
 *
 * Purpose:     Garbage collect on all the free lists
 *
 * Return:      Success:        Non-negative
 *              Failure:        Negative
 *
 * Programmer:  Quincey Koziol
 *              Friday, March 24, 2000
 *
 * Modifications:
 *      
 *-------------------------------------------------------------------------
 */
herr_t
H5FL_garbage_coll(void)
{
    /* FUNC_ENTER_INIT() should not be called, it causes an infinite loop at library termination */

    /* Garbage collect the free lists for array objects */
    H5FL_arr_gc();

    /* Garbage collect free lists for blocks */
    H5FL_blk_gc();

    /* Garbage collect the free lists for regular objects */
    H5FL_reg_gc();

    return(SUCCEED);
}   /* end H5FL_garbage_coll() */


/*-------------------------------------------------------------------------
 * Function:    H5FL_set_free_list_limits
 *
 * Purpose:     Sets limits on the different kinds of free lists.  Setting a value
 *      of -1 for a limit means no limit of that type.  These limits are global
 *      for the entire library.  Each "global" limit only applies to free lists
 *      of that type, so if an application sets a limit of 1 MB on each of the
 *      global lists, up to 3 MB of total storage might be allocated (1MB on
 *      each of regular, array and block type lists).
 *
 * Parameters:
 *  int reg_global_lim;  IN: The limit on all "regular" free list memory used
 *  int reg_list_lim;    IN: The limit on memory used in each "regular" free list
 *  int arr_global_lim;  IN: The limit on all "array" free list memory used
 *  int arr_list_lim;    IN: The limit on memory used in each "array" free list
 *  int blk_global_lim;  IN: The limit on all "block" free list memory used
 *  int blk_list_lim;    IN: The limit on memory used in each "block" free list
 *
 * Return:      Success:        non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Quincey Koziol
 *              Wednesday, August 2, 2000
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t 
H5FL_set_free_list_limits(int reg_global_lim, int reg_list_lim, int arr_global_lim,
    int arr_list_lim, int blk_global_lim, int blk_list_lim)
{
    herr_t                  ret_value = SUCCEED;

    FUNC_ENTER(H5FL_set_free_list_limits, FAIL);

    /* Set the limit variables */
    /* limit on all regular free lists */
    H5FL_reg_glb_mem_lim=(reg_global_lim==-1 ? UINT_MAX : (size_t)reg_global_lim);
    /* limit on each regular free list */
    H5FL_reg_lst_mem_lim=(reg_list_lim==-1 ? UINT_MAX : (size_t)reg_list_lim);
    /* limit on all array free lists */
    H5FL_arr_glb_mem_lim=(arr_global_lim==-1 ? UINT_MAX : (size_t)arr_global_lim);
    /* limit on each array free list */
    H5FL_arr_lst_mem_lim=(arr_list_lim==-1 ? UINT_MAX : (size_t)arr_list_lim);
    /* limit on all block free lists */
    H5FL_blk_glb_mem_lim=(blk_global_lim==-1 ? UINT_MAX : (size_t)blk_global_lim);
    /* limit on each block free list */
    H5FL_blk_lst_mem_lim=(blk_list_lim==-1 ? UINT_MAX : (size_t)blk_list_lim);

    FUNC_LEAVE(ret_value);
}   /* end H5FL_set_free_list_limits() */


/*--------------------------------------------------------------------------
 NAME
    H5FL_term_interface
 PURPOSE
    Terminate various H5FL objects
 USAGE
    void H5FL_term_interface()
 RETURNS
    Success:    Positive if any action might have caused a change in some
                other interface; zero otherwise.
        Failure:        Negative
 DESCRIPTION
    Release any resources allocated.
 GLOBAL VARIABLES
 COMMENTS, BUGS, ASSUMPTIONS
     Can't report errors...
 EXAMPLES
 REVISION LOG
--------------------------------------------------------------------------*/
int
H5FL_term_interface(void)
{
    int ret_value=0;
    
    /* Garbage collect any nodes on the free lists */
    H5FL_garbage_coll();

    ret_value=H5FL_reg_term()+H5FL_arr_term()+H5FL_blk_term();

    return(ret_value);
}

