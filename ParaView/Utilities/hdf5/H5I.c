/****************************************************************************
 * NCSA HDF                                                                 *
 * Software Development Group                                               *
 * National Center for Supercomputing Applications                          *
 * University of Illinois at Urbana-Champaign                               *
 * 605 E. Springfield, Champaign IL 61820                                   *
 *                                                                          *
 * For conditions of distribution and use, see the accompanying             *
 * hdf/COPYING file.                                                        *
 *                                                                          *
 ****************************************************************************/

/* Id */

/*
 * FILE:        H5I.c - Internal storage routines for handling "IDs"
 *     
 * REMARKS:     ID's which allow objects (void *'s currently) to be bundled
 *              into "groups" for more general storage.
 * 
 * DESIGN:      The groups are stored in an array of pointers to store each
 *              group in an element. Each "group" node contains a link to a
 *              hash table to manage the IDs in each group.  The allowed
 *              "groups" are stored in an enum (called group_t) in
 *              H5Ipublic.h.
 *              
 * AUTHOR:      Quincey Koziol
 *
 * MODIFICATIONS:
 *      1/3/96  - Starting writing specs & coding prototype
 *      1/7/96  - Finished coding prototype
 *      6/10/97 - Moved into HDF5 library
 */
#include "H5private.h"
#include "H5Iprivate.h"
#include "H5Eprivate.h"
#include "H5FLprivate.h"        /*Free Lists      */
#include "H5MMprivate.h"

/* Interface initialialization? */
#define PABLO_MASK      H5I_mask
static int interface_initialize_g = 0;
#define INTERFACE_INIT H5I_init_interface
static herr_t H5I_init_interface(void);

/*
 * Define the following macro for fast hash calculations (but limited
 * hash sizes)
 */
#define HASH_SIZE_POWER_2

/* Define the following macro for atom caching over all the atoms */
#define IDS_ARE_CACHED

/*-------------------- Locally scoped variables -----------------------------*/

#ifdef IDS_ARE_CACHED
#  define ID_CACHE_SIZE 4             /*# of previous atoms cached         */
#endif

/*
 * Number of bits to use for Group ID in each atom. Increase if H5I_NGROUPS
 * becomes too large (an assertion would fail in H5I_init_interface). This is
 * the only number that must be changed since all other bit field sizes and
 * masks are calculated from GROUP_BITS.
 */
#define GROUP_BITS      5
#define GROUP_MASK      ((1<<GROUP_BITS)-1)

/*
 * Number of bits to use for the Atom index in each atom (assumes 8-bit
 * bytes). We don't use the sign bit.
 */
#define ID_BITS         ((sizeof(hid_t)*8)-(GROUP_BITS+1))
#define ID_MASK         ((1<<ID_BITS)-1)

/* Map an atom to a Group number */
#define H5I_GROUP(a)    ((H5I_type_t)(((hid_t)(a)>>ID_BITS) & GROUP_MASK))


#ifdef HASH_SIZE_POWER_2
/*
 * Map an ID to a hash location (assumes s is a power of 2 and smaller
 * than the ID_MASK constant).
 */
#  define H5I_LOC(a,s)          ((hid_t)((size_t)(a)&((s)-1)))
#  define POWER_OF_TWO(n)       ((((n) - 1) & (n)) == 0 && (n) > 0)
#else
/*
 * Map an ID to a hash location.
 */
#  define H5I_LOC(a,s)  (((hid_t)(a)&ID_MASK)%(s))
#endif

/* Combine a Group number and an atom index into an atom */
#define H5I_MAKE(g,i)   ((((hid_t)(g)&GROUP_MASK)<<ID_BITS)|      \
                             ((hid_t)(i)&ID_MASK))

#ifdef IDS_ARE_CACHED
/* ID Cache */
static H5I_id_info_t *H5I_cache_g[ID_CACHE_SIZE];
#endif

/* Array of pointers to atomic groups */
static H5I_id_group_t *H5I_id_group_list_g[H5I_NGROUPS];

/* Declare a free list to manage the H5I_id_info_t struct */
H5FL_DEFINE_STATIC(H5I_id_info_t);

/*--------------------- Local function prototypes ---------------------------*/
static H5I_id_info_t *H5I_find_id(hid_t id);
#ifdef H5I_DEBUG_OUTPUT
static herr_t H5I_debug(H5I_type_t grp);
#endif /* H5I_DEBUG_OUTPUT */


/*-------------------------------------------------------------------------
 * Function:    H5I_init_interface
 *
 * Purpose:     Initialize interface-specific information.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t 
H5I_init_interface(void)
{
    herr_t                  ret_value = SUCCEED;
    FUNC_ENTER(H5I_init_interface, FAIL);

    /*
     * Make certain the ID types don't overflow the number of bits allocated
     * for them in an ID.
     */
    assert(H5I_NGROUPS<=(1<<GROUP_BITS));

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_term_interface
 *
 * Purpose:     Terminate the H5I interface: release all memory, reset all
 *              global variables to initial values. This only happens if all
 *              groups have been destroyed from other interfaces.
 *
 * Return:      Success:        Positive if any action was taken that might
 *                              affect some other interface; zero otherwise.
 *
 *              Failure:        Negative.
 *
 * Programmer:  
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5I_term_interface(void)
{
    H5I_id_group_t      *grp_ptr;
    H5I_type_t          grp;
    int         n=0;

    if (interface_initialize_g) {
        /* How many groups are still being used? */
        for (grp=(H5I_type_t)0; grp<H5I_NGROUPS; grp++) {
            if ((grp_ptr=H5I_id_group_list_g[grp]) && grp_ptr->id_list)
                n++;
        }

        /* If no groups are used then clean  up */
        if (0==n) {
            for (grp=(H5I_type_t)0; grp<H5I_NGROUPS; grp++) {
                grp_ptr = H5I_id_group_list_g[grp];
                H5MM_xfree(grp_ptr);
                H5I_id_group_list_g[grp] = NULL;
            }
        }

        /* Mark interface closed */
        interface_initialize_g = 0;
    }
    return n;
}


/*-------------------------------------------------------------------------
 * Function:    H5I_init_group
 *
 * Purpose:     Initialize an ID group whose ID number is specified by GRP,
 *              If the group has already been initialized, this routine just
 *              increments the count of number of initializations and returns
 *              without trying to change the size of the hash table.  A
 *              specific number (RESERVED) of group entries may be reserved
 *              to enable "constant" values to be handed out which are valid
 *              IDs in the group, but which do not map to any data structures
 *              and are not allocated dynamicly later. HASH_SIZE is the
 *              minimum hash table size to use for the group. FREE_FUNC is
 *              called with an object pointer when the object is removed from
 *              the group.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Friday, February 19, 1999
 *
 * Modifications:
 *              Bill Wendling, 2000-05-05
 *              Instead of the ugly test of whether hash_size is a power of
 *              two, I placed it in a macro POWER_OF_TWO which uses the fact
 *              that a number that is a power of two has only 1 bit set.
 *
 *              Bill Wendling, 2000-05-09
 *              Changed POWER_OF_TWO macro to allow 1 as a valid power of two.
 *              Changed test below accordingly.
 *
 *-------------------------------------------------------------------------
 */
int 
H5I_init_group(H5I_type_t grp, size_t hash_size, unsigned reserved,
               H5I_free_t free_func)
{
    H5I_id_group_t      *grp_ptr = NULL;        /*ptr to the atomic group*/
    int         ret_value = SUCCEED;    /*return value          */

    FUNC_ENTER(H5I_init_group, FAIL);

    /* Check arguments */
    if ((grp <= H5I_BADID || grp >= H5I_NGROUPS) && hash_size > 0) {
        HGOTO_DONE(FAIL);
    }
#ifdef HASH_SIZE_POWER_2
    if (!POWER_OF_TWO(hash_size) || hash_size == 1)
        HGOTO_DONE(FAIL);
#endif /* HASH_SIZE_POWER_2 */

    if (H5I_id_group_list_g[grp] == NULL) {
        /* Allocate the group information for new group */
        if (NULL==(grp_ptr = H5MM_calloc(sizeof(H5I_id_group_t)))) {
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                         "memory allocation failed");
        }
        H5I_id_group_list_g[grp] = grp_ptr;
    } else {
        /* Get the pointer to the existing group */
        grp_ptr = H5I_id_group_list_g[grp];
    }

    if (grp_ptr->count == 0) {
        /* Initialize the ID group structure for new groups */
        grp_ptr->hash_size = hash_size;
        grp_ptr->reserved = reserved;
        grp_ptr->wrapped = 0;
        grp_ptr->ids = 0;
        grp_ptr->nextid = reserved;
        grp_ptr->free_func = free_func;
        grp_ptr->id_list = H5MM_calloc(hash_size*sizeof(H5I_id_info_t *));
        if (NULL==grp_ptr->id_list) {
            HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                         "memory allocation failed");
        }
    }
    
    /* Increment the count of the times this group has been initialized */
    grp_ptr->count++;

  done:
    if (ret_value<0) {
        /* Error condition cleanup */
        if (grp_ptr != NULL) {
            H5MM_xfree(grp_ptr->id_list);
            H5MM_xfree(grp_ptr);
        }
    }

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_nmembers
 *
 * Purpose:     Returns the number of members in a group.
 *
 * Return:      Success:        Number of members; zero if the group is empty
 *                              or has been deleted.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, March 24, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5I_nmembers(H5I_type_t grp)
{
    H5I_id_group_t      *grp_ptr = NULL;
    H5I_id_info_t       *cur=NULL;
    int         n=0;
    unsigned            i;

    FUNC_ENTER(H5I_nmembers, FAIL);

    if (grp<=H5I_BADID || grp>=H5I_NGROUPS) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADRANGE, FAIL, "invalid group number");
    }
    if (NULL==(grp_ptr=H5I_id_group_list_g[grp]) || grp_ptr->count<=0) {
        HRETURN(0);
    }

    for (i=0; i<grp_ptr->hash_size; i++) {
        for (cur=grp_ptr->id_list[i]; cur; cur=cur->next) {
            n++;
        }
    }
    FUNC_LEAVE(n);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_clear_group
 *
 * Purpose:     Removes all objects from the group, calling the free
 *              function for each object regardless of the reference count.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, March 24, 1999
 *
 * Modifications:
 *              Robb Matzke, 1999-04-27
 *              If FORCE is zero then any item for which the free callback
 *              failed is not removed.  This function returns failure if
 *              items could not be removed.
 *
 *              Robb Matzke, 1999-08-17
 *              If the object reference count is larger than one then it must
 *              be because the library is using the object internally. This
 *              happens for instance for file driver ID's which are stored in
 *              things like property lists, files, etc.  Objects that have a
 *              reference count larger than one are not affected unless FORCE
 *              is non-zero.
 *-------------------------------------------------------------------------
 */
herr_t
H5I_clear_group(H5I_type_t grp, hbool_t force)
{
    H5I_id_group_t      *grp_ptr = NULL;        /* ptr to the atomic group */
    H5I_id_info_t       *cur=NULL, *next=NULL, *prev=NULL;
    int         ret_value = SUCCEED;
    unsigned       deleted;                /* Flag to indicate objects have been removed from a linked list */
    unsigned            i;

    FUNC_ENTER(H5I_clear_group, FAIL);

    if (grp <= H5I_BADID || grp >= H5I_NGROUPS) {
        HGOTO_DONE(FAIL);
    }
    
    grp_ptr = H5I_id_group_list_g[grp];
    if (grp_ptr == NULL || grp_ptr->count <= 0) {
        HGOTO_DONE(FAIL);
    }

#ifdef IDS_ARE_CACHED
    /*
     * Remove atoms from the global atom cache.
     */
    for (i=0; i<ID_CACHE_SIZE; i++) {
        if (H5I_cache_g[i] && H5I_GROUP(H5I_cache_g[i]->id) == grp) {
            H5I_cache_g[i] = NULL;
        }
    }
#endif /* IDS_ARE_CACHED */

    /*
     * Call free method for all objects in group regardless of their reference
     * counts. Ignore the return value from from the free method and remove
     * object from group regardless if FORCE is non-zero.
     */
    for (i=0; i<grp_ptr->hash_size; i++) {
        /* Reset the "deleted an object from this list" flag */
        deleted=0;

        for (cur=grp_ptr->id_list[i]; cur; cur=next) {
            /*
             * Do nothing to the object if the reference count is larger than
             * one and forcing is off.
             */
            if (!force && cur->count>1)
                continue;

            /* Flag this list as having deleted objects */
            deleted=1;

            /* Free the object regardless of reference count */
            if (grp_ptr->free_func && (grp_ptr->free_func)(cur->obj_ptr)<0) {
                if (force) {
#if H5I_DEBUG
                    if (H5DEBUG(I)) {
                    fprintf(H5DEBUG(I), "H5I: free grp=%d obj=0x%08lx "
                        "failure ignored\n", (int)grp,
                        (unsigned long)(cur->obj_ptr));
                    }
#endif /*H5I_DEBUG*/
                    /* Decrement the number of IDs in the group */
                    (grp_ptr->ids)--;

                    /* Add ID struct to free list */
                    next = cur->next;
                    H5FL_FREE(H5I_id_info_t,cur);
                } else {
                    if (prev)
                        prev->next = cur;
                    else
                        grp_ptr->id_list[i] = cur;
                    prev = cur;
                }
            } else {
                /* Decrement the number of IDs in the group */
                (grp_ptr->ids)--;

                /* Add ID struct to free list */
                next = cur->next;
                H5FL_FREE(H5I_id_info_t,cur);
            }
        }
        /* 'prev' flag is only valid to check when we've actually deleted objects */
        if (deleted && !prev)
            grp_ptr->id_list[i]=NULL;
    }
    
  done:
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_destroy_group
 *
 * Purpose:     Decrements the reference count on an entire group of IDs.
 *              If the group reference count becomes zero then the group is
 *              destroyed along with all atoms in that group regardless of
 *              their reference counts.  Destroying IDs involves calling
 *              the free-func for each ID's object and then adding the ID
 *              struct to the ID free list.
 *              
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Unknown
 *
 * Modifications:
 *
 *      Robb Matzke, 25 Feb 1998
 *      IDs are freed when a group is destroyed.
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5I_destroy_group(H5I_type_t grp)
{
    H5I_id_group_t      *grp_ptr = NULL;        /* ptr to the atomic group */
    int         ret_value = SUCCEED;

    FUNC_ENTER(H5I_destroy_group, FAIL);

    if (grp <= H5I_BADID || grp >= H5I_NGROUPS) {
        HGOTO_DONE(FAIL);
    }
    
    grp_ptr = H5I_id_group_list_g[grp];
    if (grp_ptr == NULL || grp_ptr->count <= 0) {
        HGOTO_DONE(FAIL);
    }

    /*
     * Decrement the number of users of the atomic group.  If this is the
     * last user of the group then release all atoms from the group.  The
     * free function is invoked for each atom being freed.
     */
    if (1==grp_ptr->count) {
        H5I_clear_group(grp, TRUE);
        H5E_clear(); /*don't care about errors*/
        H5MM_xfree(grp_ptr->id_list);
        HDmemset (grp_ptr, 0, sizeof(*grp_ptr));
    } else {
        --(grp_ptr->count);
    }
    
  done:
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_register
 *
 * Purpose:     Registers an OBJECT in a GROUP and returns an ID for it.
 *              This routine does _not_ check for unique-ness of the objects,
 *              if you register an object twice, you will get two different
 *              IDs for it.  This routine does make certain that each ID in a
 *              group is unique.  IDs are created by getting a unique number
 *              for the group the ID is in and incorporating the group into
 *              the ID which is returned to the user.
 *
 * Return:      Success:        New object id.
 *
 *              Failure:        Negative
 *
 * Programmer:  Unknown
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t 
H5I_register(H5I_type_t grp, void *object)
{
    H5I_id_group_t      *grp_ptr=NULL;  /*ptr to the group              */
    H5I_id_info_t       *id_ptr=NULL;   /*ptr to the new ID information */
    hid_t               new_id;         /*new ID                        */
    unsigned            hash_loc;       /*new item's hash table location*/
    hid_t               next_id;        /*next ID to check              */
    hid_t               ret_value=SUCCEED; /*return value               */
    H5I_id_info_t       *curr_id;       /*ptr to the current atom       */
    unsigned            i;              /*counter                       */
    
    FUNC_ENTER(H5I_register, FAIL);

    /* Check arguments */
    if (grp <= H5I_BADID || grp >= H5I_NGROUPS) {
        HGOTO_DONE(FAIL);
    }
    grp_ptr = H5I_id_group_list_g[grp];
    if (grp_ptr == NULL || grp_ptr->count <= 0) {
        HGOTO_DONE(FAIL);
    }
    if ((id_ptr = H5FL_ALLOC(H5I_id_info_t,0)) == NULL) {
        HGOTO_DONE(FAIL);
    }

    /* Create the struct & it's ID */
    new_id = H5I_MAKE(grp, grp_ptr->nextid);
    id_ptr->id = new_id;
    id_ptr->count = 1; /*initial reference count*/
    id_ptr->obj_ptr = object;
    id_ptr->next = NULL;

    /* hash bucket already full, prepend to front of chain */
    hash_loc = grp_ptr->nextid % (unsigned) grp_ptr->hash_size;
    if (grp_ptr->id_list[hash_loc] != NULL) {
        id_ptr->next = grp_ptr->id_list[hash_loc];
    }

    /* Insert into the group */
    grp_ptr->id_list[hash_loc] = id_ptr;
    grp_ptr->ids++;
    grp_ptr->nextid++;

    /*
     * This next section of code checks for the 'nextid' getting too large and
     * wrapping around, thus necessitating checking for duplicate IDs being
     * handed out.
     */
    if (grp_ptr->nextid > (unsigned)ID_MASK) {
        grp_ptr->wrapped = 1;
        grp_ptr->nextid = grp_ptr->reserved;
    }

    /*
     * If we've wrapped around then we need to check for duplicate id's being
     * handed out.
     */
    if (grp_ptr->wrapped) {
        /*
         * Make sure we check all available ID's.  If we're about at the end
         * of the range then wrap around and check the beginning values.  If
         * we check all possible values and didn't find any free ones *then*
         * we can fail.
         */
        for (i=grp_ptr->reserved; i<ID_MASK; i++) {
            /* Handle end of range by wrapping to beginning */
            if (grp_ptr->nextid>(unsigned)ID_MASK) {
                grp_ptr->nextid = grp_ptr->reserved;
            }

            /* new ID to check for */
            next_id = H5I_MAKE(grp, grp_ptr->nextid);
            hash_loc = H5I_LOC (grp_ptr->nextid, grp_ptr->hash_size);
            curr_id = grp_ptr->id_list[hash_loc];
            if (curr_id == NULL) break; /* Ha! this is not likely... */

            while (curr_id) {
                if (curr_id->id == next_id) break;
                curr_id = curr_id->next;
            }
            if (!curr_id) break; /* must not have found a match */
            grp_ptr->nextid++;
        }

        if (i>=(unsigned)ID_MASK) {
            /* All the IDs are gone! */
            HGOTO_DONE(FAIL);
        }
    }
    ret_value = new_id;

  done:
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_object
 *
 * Purpose:     Find an object pointer for the specified ID.
 *
 * Return:      Success:        Non-null object pointer associated with the
 *                              specified ID.
 *
 *              Failure:        NULL
 *
 * Programmer:  
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void * 
H5I_object(hid_t id)
{
    H5I_id_info_t       *id_ptr = NULL;         /*ptr to the new atom   */
    void                *ret_value = NULL;      /*return value          */

    FUNC_ENTER(H5I_object, NULL);

    /* General lookup of the ID */
    if (NULL==(id_ptr = H5I_find_id(id))) HGOTO_DONE(NULL);

    /* Check if we've found the correct ID */
    if (id_ptr) ret_value = id_ptr->obj_ptr;

  done:
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_get_type
 *
 * Purpose:     Given an object ID return the group (type) to which it
 *              belongs.  The ID need not be the ID of an object which
 *              currently exists because the group number (type) is encoded
 *              in the object ID.
 *
 * Return:      Success:        A valid group number (type)
 *
 *              Failure:        H5I_BADID, a negative value.
 *
 * Programmer:  Robb Matzke
 *              Friday, February 19, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5I_type_t 
H5I_get_type(hid_t id)
{
    H5I_type_t          ret_value = H5I_BADID;

    FUNC_ENTER(H5I_get_type, H5I_BADID);

    if (id>0) ret_value = H5I_GROUP(id);
    assert(ret_value>=H5I_BADID && ret_value<H5I_NGROUPS);

    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Iget_type
 *
 * Purpose:     The public version of H5I_get_type(), obtains a group number
 *              (type) when given an ID.  The ID need not be the ID of an
 *              object which currently exists because the group number is
 *              encoded as part of the ID.
 *
 * Return:      Success:        Group number (type)
 *
 *              Failure:        H5I_BADID, a negative value
 *
 * Programmer:  
 *
 * Modifications:
 *              Robb Matzke, 1999-08-23
 *              Also fails if the ID has a valid group but no longer exists
 *              in the ID tables.
 *-------------------------------------------------------------------------
 */
H5I_type_t
H5Iget_type(hid_t id)
{
    H5I_type_t          ret_value = H5I_BADID;

    FUNC_ENTER(H5Iget_type, H5I_BADID);
    H5TRACE1("It","i",id);

    ret_value = H5I_get_type(id);

    if (ret_value <= H5I_BADID || ret_value >= H5I_NGROUPS ||
        NULL==H5I_object(id)) {
        HGOTO_DONE(H5I_BADID);
    }

done:
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_remove
 *
 * Purpose:     Removes the specified ID from its group.
 *
 * Return:      Success:        A pointer to the object that was removed, the
 *                              same pointer which would have been found by
 *                              calling H5I_object().
 *
 *              Failure:        NULL
 *
 * Programmer:  
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void * 
H5I_remove(hid_t id)
{
    H5I_id_group_t      *grp_ptr = NULL;/*ptr to the atomic group       */
    H5I_id_info_t       *curr_id;       /*ptr to the current atom       */
    H5I_id_info_t       *last_id;       /*ptr to the last atom          */
    H5I_type_t          grp;            /*atom's atomic group           */
    unsigned            hash_loc;       /*atom's hash table location    */
#ifdef IDS_ARE_CACHED
    unsigned            i;              /*local counting variable       */
#endif
    void *            ret_value = NULL; /*return value                  */

    FUNC_ENTER(H5I_remove, NULL);

    /* Check arguments */
    grp = H5I_GROUP(id);
    if (grp <= H5I_BADID || grp >= H5I_NGROUPS)
        HGOTO_DONE(NULL);
    grp_ptr = H5I_id_group_list_g[grp];
    if (grp_ptr == NULL || grp_ptr->count <= 0)
        HGOTO_DONE(NULL);

    /* Get the bucket in which the ID is located */
    hash_loc = (unsigned) H5I_LOC(id, grp_ptr->hash_size);
    curr_id = grp_ptr->id_list[hash_loc];
    if (curr_id == NULL)
        HGOTO_DONE(NULL);

    last_id = NULL;
    while (curr_id != NULL) {
        if (curr_id->id == id)
            break;
        last_id = curr_id;
        curr_id = curr_id->next;
    }

#ifdef IDS_ARE_CACHED
    /* Delete object from cache */
    for (i = 0; i < ID_CACHE_SIZE; i++)
        if (H5I_cache_g[i] && H5I_cache_g[i]->id == id) {
            H5I_cache_g[i] = NULL;
            break; /* we assume there is only one instance in the cache */
        }
#endif /* IDS_ARE_CACHED */

    if (curr_id != NULL) {
        if (last_id == NULL) {
            /* ID is the first in the chain */
            grp_ptr->id_list[hash_loc] = curr_id->next;
        } else {
            last_id->next = curr_id->next;
        }
        ret_value = curr_id->obj_ptr;
        H5FL_FREE(H5I_id_info_t,curr_id);
    } else {
        /* couldn't find the ID in the proper place */
        HGOTO_DONE(NULL);
    }

    /* Decrement the number of IDs in the group */
    (grp_ptr->ids)--;

  done:
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_dec_ref
 *
 * Purpose:     Decrements the number of references outstanding for an ID.
 *              This will fail if the group is not a reference counted group.
 *              The ID group's 'free' function will be called for the ID
 *              if the reference count for the ID reaches 0 and a free
 *              function has been defined at group creation time.
 *
 * Return:      Success:        New reference count.
 *
 *              Failure:        Negative
 *
 * Programmer:  Unknown
 *
 * Modifications:
 *
 *      Robb Matzke, 19 Feb 1998
 *      It is no longer an error when the reference count of an item reaches
 *      zero and no `free' function has been defined.  The object is still
 *      removed from the list.
 *
 *      Robb Matzke, 30 Dec 1998
 *      Fixed a bug where the return value was always zero instead of the new
 *      reference count.
 *
 *      Robb Matzke, 19 Feb 1999
 *      If the free method is defined and fails then the object is not
 *      removed from the group and its reference count is not decremented.
 *      The group number is now passed to the free method.
 *
 *-------------------------------------------------------------------------
 */
int
H5I_dec_ref(hid_t id)
{
    H5I_type_t          grp = H5I_GROUP(id);    /*group the object is in*/
    H5I_id_group_t      *grp_ptr = NULL;        /*ptr to the group      */
    H5I_id_info_t       *id_ptr = NULL;         /*ptr to the new ID     */
    int         ret_value = FAIL;       /*return value          */

    FUNC_ENTER(H5I_dec_ref, FAIL);

    /* Check arguments */
    grp_ptr = H5I_id_group_list_g[grp];
    if (grp_ptr == NULL || grp_ptr->count <= 0) {
        HRETURN(FAIL);
    }
    
    /* General lookup of the ID */
    if ((id_ptr=H5I_find_id(id))) {
        /*
         * If this is the last reference to the object then invoke the group's
         * free method on the object. If the free method is undefined or
         * successful then remove the object from the group; otherwise leave
         * the object in the group without decrementing the reference
         * count. If the reference count is more than one then decrement the
         * reference count without calling the free method.
         *
         * Beware: the free method may call other H5I functions.
         */
        if (1==id_ptr->count) {
            if (!grp_ptr->free_func ||
                (grp_ptr->free_func)(id_ptr->obj_ptr)>=0) {
                H5I_remove(id);
                ret_value = 0;
            } else {
                ret_value = 1;
            }
        } else {
            ret_value = --(id_ptr->count);
        }
    }
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_inc_ref
 *
 * Purpose:     Increment the reference count for an object.
 *
 * Return:      Success:        The new reference count.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Thursday, July 29, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5I_inc_ref(hid_t id)
{
    H5I_type_t          grp = H5I_GROUP(id);    /*group the object is in*/
    H5I_id_group_t      *grp_ptr = NULL;        /*ptr to the group      */
    H5I_id_info_t       *id_ptr = NULL;         /*ptr to the ID         */

    FUNC_ENTER(H5I_inc_ref, FAIL);

    /* Check arguments */
    if (id<0) HRETURN(FAIL);
    grp_ptr = H5I_id_group_list_g[grp];
    if (!grp_ptr || grp_ptr->count<=0) HRETURN(FAIL);

    /* General lookup of the ID */
    if (NULL==(id_ptr=H5I_find_id(id))) HRETURN(FAIL);
    id_ptr->count++;

    FUNC_LEAVE(id_ptr->count);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_search
 *
 * Purpose:     Apply function FUNC to each member of group GRP and return a
 *              pointer to the first object for which FUNC returns non-zero.
 *              The FUNC should take a pointer to the object and the KEY as
 *              arguments and return non-zero to terminate the search (zero
 *              to continue).
 *
 * Limitation:  Currently there is no way to start searching from where a
 *              previous search left off.
 *
 * Return:      Success:        The first object in the group for which FUNC
 *                              returns non-zero. NULL if FUNC returned zero
 *                              for every object in the group.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Friday, February 19, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
void *
H5I_search(H5I_type_t grp, H5I_search_func_t func, const void *key)
{
    H5I_id_group_t      *grp_ptr = NULL;        /*ptr to the group      */
    H5I_id_info_t       *id_ptr = NULL;         /*ptr to the new ID     */
    unsigned            i;                      /*counter               */
    void                *ret_value = NULL;      /*return value          */

    FUNC_ENTER(H5I_search, NULL);

    /* Check arguments */
    if (grp <= H5I_BADID || grp >= H5I_NGROUPS) {
        HGOTO_DONE(NULL);
    }
    grp_ptr = H5I_id_group_list_g[grp];
    if (grp_ptr == NULL || grp_ptr->count <= 0) {
        HGOTO_DONE(NULL);
    }

    /* Start at the beginning of the array */
    for (i=0; i<grp_ptr->hash_size; i++) {
        id_ptr = grp_ptr->id_list[i];
        while (id_ptr) {
            if ((*func)(id_ptr->obj_ptr, key)) {
                HGOTO_DONE(id_ptr->obj_ptr);    /*found the item*/
            }
            id_ptr = id_ptr->next;
        }
    }

  done:
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_find_id
 *
 * Purpose:     Given an object ID find the info struct that describes the
 *              object.
 *
 * Return:      Success:        Ptr to the object's info struct.
 *
 *              Failure:        NULL
 *
 * Programmer:  
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static H5I_id_info_t *
H5I_find_id(hid_t id)
{
    H5I_id_group_t      *grp_ptr = NULL;        /*ptr to the group      */
    H5I_id_info_t       *id_ptr = NULL;         /*ptr to the new ID     */
    H5I_type_t          grp;                    /*ID's group            */
    unsigned            hash_loc;               /*bucket pointer        */
    H5I_id_info_t       *ret_value = NULL;      /*return value          */
#ifdef IDS_ARE_CACHED
    int         i;
#endif

    FUNC_ENTER(H5I_find_id, NULL);

    /* Check arguments */
    grp = H5I_GROUP(id);
    if (grp <= H5I_BADID || grp >= H5I_NGROUPS) {
        HGOTO_DONE(NULL);
    }
    grp_ptr = H5I_id_group_list_g[grp];
    if (grp_ptr == NULL || grp_ptr->count <= 0) {
        HGOTO_DONE(NULL);
    }

#ifdef IDS_ARE_CACHED
    /*
     * Look for the ID in the cache first. Implement a simple "move
     * forward" caching scheme by swapping the found cache item with the
     * previous cache item.  This gradually migrates used cache items toward
     * the front of the cache and unused items toward the end.  For instance,
     * finding `e' in the cache results in:
     *
     * Before: a b c d e f g h i j
     *         | | |  X  | | | | |
     * After:  a b c e d f g h i j
     */
    for (i=0; i<ID_CACHE_SIZE; i++)
        if (H5I_cache_g[i] && H5I_cache_g[i]->id == id) {
            ret_value = H5I_cache_g[i];
            if (i > 0) {
                H5I_id_info_t *tmp = H5I_cache_g[i-1];
                H5I_cache_g[i-1] = H5I_cache_g[i];
                H5I_cache_g[i] = tmp;
            }
            HGOTO_DONE(ret_value);
        }
#endif /* IDS_ARE_CACHED */

    /* Get the bucket in which the ID is located */
    hash_loc = (unsigned)H5I_LOC(id, grp_ptr->hash_size);
    id_ptr = grp_ptr->id_list[hash_loc];
    if (id_ptr == NULL) {
        HGOTO_DONE(NULL);
    }

    /* Scan the bucket's linked list for a match */
    while (id_ptr) {
        if (id_ptr->id == id) break;
        id_ptr = id_ptr->next;
    }
    ret_value = id_ptr;

#ifdef IDS_ARE_CACHED
    /* Add id to the end of the cache */
    H5I_cache_g[ID_CACHE_SIZE-1] = id_ptr;
#endif /* IDS_ARE_CACHED */

  done:
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5I_debug
 *
 * Purpose:     Dump the contents of a group to stderr for debugging.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Friday, February 19, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
#ifdef H5I_DEBUG_OUTPUT
static herr_t
H5I_debug(H5I_type_t grp)
{
    H5I_id_group_t      *grp_ptr;
    H5I_id_info_t       *cur;
    int                 is, js;
    unsigned int        iu;

    FUNC_ENTER(H5I_debug, FAIL);
    
    fprintf(stderr, "Dumping group %d\n", (int)grp);
    grp_ptr = H5I_id_group_list_g[grp];

    /* Header */
    fprintf(stderr, "    count     = %u\n", grp_ptr->count);
    fprintf(stderr, "    reserved  = %u\n", grp_ptr->reserved);
    fprintf(stderr, "    wrapped   = %u\n", grp_ptr->wrapped);
    fprintf(stderr, "    hash_size = %lu\n",
            (unsigned long)grp_ptr->hash_size);
    fprintf(stderr, "    ids       = %u\n", grp_ptr->ids);
    fprintf(stderr, "    nextid    = %u\n", grp_ptr->nextid);

    /* Cache */
    fprintf(stderr, "    Cache:\n");
    for (is=0; is<ID_CACHE_SIZE; is++) {
        if (H5I_cache_g[is] && H5I_GROUP(H5I_cache_g[is]->id)==grp) {
            fprintf(stderr, "        Entry-%d, ID=%lu\n",
                    is, (unsigned long)(H5I_cache_g[is]->id));
        }
    }

    /* List */
    fprintf(stderr, "    List:\n");
    for (iu=0; iu<grp_ptr->hash_size; iu++) {
        for (js=0, cur=grp_ptr->id_list[iu]; cur; cur=cur->next, js++) {
            fprintf(stderr, "        #%u.%d\n", iu, js);
            fprintf(stderr, "            id = %lu\n",
                    (unsigned long)(cur->id));
            fprintf(stderr, "            count = %u\n", cur->count);
            fprintf(stderr, "            obj   = 0x%08lx\n",
                    (unsigned long)(cur->obj_ptr));
        }
    }

    FUNC_LEAVE(SUCCEED);
}
#endif /* H5I_DEBUG_OUTPUT */

