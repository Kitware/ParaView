/*-------------------------------------------------------------------------
 * Copyright (C) 1997   National Center for Supercomputing Applications.
 *                      All rights reserved.
 *
 *-------------------------------------------------------------------------
 *
 * Created:     H5G.c
 *              Jul 18 1997
 *              Robb Matzke <matzke@llnl.gov>
 *
 * Purpose:     Symbol table functions.  The functions that begin with
 *              `H5G_stab_' don't understand the naming system; they operate
 *              on a single symbol table at a time.
 *
 *              The functions that begin with `H5G_node_' operate on the leaf
 *              nodes of a symbol table B-tree.  They should be defined in
 *              the H5Gnode.c file. 
 *
 *              The remaining functions know how to traverse the group
 *              directed graph.
 *
 * Names:       Object names are a slash-separated list of components.  If
 *              the name begins with a slash then it's absolute, otherwise
 *              it's relative ("/foo/bar" is absolute while "foo/bar" is
 *              relative).  Multiple consecutive slashes are treated as
 *              single slashes and trailing slashes are ignored.  The special
 *              case `/' is the root group.  Every file has a root group.
 *
 *              API functions that look up names take a location ID and a
 *              name.  The location ID can be a file ID or a group ID and the
 *              name can be relative or absolute.
 *
 *              +--------------+----------- +--------------------------------+
 *              | Location ID  | Name       | Meaning                        |
 *              +--------------+------------+--------------------------------+
 *              | File ID      | "/foo/bar" | Find `foo' within `bar' within |
 *              |              |            | the root group of the specified|
 *              |              |            | file.                          |
 *              +--------------+------------+--------------------------------+
 *              | File ID      | "foo/bar"  | Find `foo' within `bar' within |
 *              |              |            | the current working group of   |
 *              |              |            | the specified file.            |
 *              +--------------+------------+--------------------------------+
 *              | File ID      | "/"        | The root group of the specified|
 *              |              |            | file.                          |
 *              +--------------+------------+--------------------------------+
 *              | File ID      | "."        | The current working group of   |
 *              |              |            | the specified file.            |
 *              +--------------+------------+--------------------------------+
 *              | Group ID     | "/foo/bar" | Find `foo' within `bar' within |
 *              |              |            | the root group of the file     |
 *              |              |            | containing the specified group.|
 *              +--------------+------------+--------------------------------+
 *              | Group ID     | "foo/bar"  | File `foo' within `bar' within |
 *              |              |            | the specified group.           |
 *              +--------------+------------+--------------------------------+
 *              | Group ID     | "/"        | The root group of the file     |
 *              |              |            | containing the specified group.|
 *              +--------------+------------+--------------------------------+
 *              | Group ID     | "."        | The specified group.           |
 *              +--------------+------------+--------------------------------+
 *
 *
 * Modifications:
 *
 *      Robb Matzke, 5 Aug 1997
 *      Added calls to H5E.
 *
 *      Robb Matzke, 30 Aug 1997
 *      Added `Errors:' field to function prologues.
 *
 *-------------------------------------------------------------------------
 */

#define H5G_PACKAGE /*suppress error message about including H5Gpkg.h */
#define H5F_PACKAGE             /*suppress error about including H5Fpkg   */


/* Packages needed by this file... */
#include "H5private.h"
#include "H5Aprivate.h"
#include "H5Bprivate.h"
#include "H5Dprivate.h"
#include "H5Eprivate.h"
#include "H5Fpkg.h"         /*file access                             */
#include "H5FLprivate.h"        /*Free Lists      */
#include "H5Gpkg.h"
#include "H5HLprivate.h"
#include "H5Iprivate.h"
#include "H5MMprivate.h"
#include "H5Oprivate.h"

#define H5G_INIT_HEAP           8192
#define H5G_RESERVED_ATOMS      0
#define PABLO_MASK              H5G_mask

/* Interface initialization */
static int interface_initialize_g = 0;
#define INTERFACE_INIT  H5G_init_interface
static herr_t H5G_init_interface(void);
static H5G_typeinfo_t *H5G_type_g = NULL;       /*object typing info    */
static size_t H5G_ntypes_g = 0;                 /*entries in type table */
static size_t H5G_atypes_g = 0;                 /*entries allocated     */

/* Declare a free list to manage the H5G_t struct */
H5FL_DEFINE(H5G_t);


/*-------------------------------------------------------------------------
 * Function:    H5Gcreate
 *
 * Purpose:     Creates a new group relative to LOC_ID and gives it the
 *              specified NAME.  The group is opened for write access
 *              and it's object ID is returned.
 *
 *              The optional SIZE_HINT specifies how much file space to
 *              reserve to store the names that will appear in this
 *              group. If a non-positive value is supplied for the SIZE_HINT
 *              then a default size is chosen.
 *
 * See also:    H5Gset(), H5Gpush(), H5Gpop()
 *
 * Errors:
 *
 * Return:      Success:        The object ID of a new, empty group open for
 *                              writing.  Call H5Gclose() when finished with
 *                              the group.
 *
 *              Failure:        FAIL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, September 24, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Gcreate(hid_t loc_id, const char *name, size_t size_hint)
{
    H5G_entry_t            *loc = NULL;
    H5G_t                  *grp = NULL;
    hid_t                   ret_value = FAIL;

    FUNC_ENTER(H5Gcreate, FAIL);
    H5TRACE3("i","isz",loc_id,name,size_hint);

    /* Check arguments */
    if (NULL==(loc=H5G_loc (loc_id))) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name given");
    }
    
    /* Create the group */
    if (NULL == (grp = H5G_create(loc, name, size_hint))) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to create group");
    }
    if ((ret_value = H5I_register(H5I_GROUP, grp)) < 0) {
        H5G_close(grp);
        HRETURN_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL,
                      "unable to register group");
    }
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Gopen
 *
 * Purpose:     Opens an existing group for modification.  When finished,
 *              call H5Gclose() to close it and release resources.
 *
 * Errors:
 *
 * Return:      Success:        Object ID of the group.
 *
 *              Failure:        FAIL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, December 31, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
hid_t
H5Gopen(hid_t loc_id, const char *name)
{
    hid_t       ret_value = FAIL;
    H5G_t       *grp = NULL;
    H5G_entry_t *loc = NULL;

    FUNC_ENTER(H5Gopen, FAIL);
    H5TRACE2("i","is",loc_id,name);

    /* Check args */
    if (NULL==(loc=H5G_loc(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name");
    }
    
    /* Open the group */
    if (NULL == (grp = H5G_open(loc, name))) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTOPENOBJ, FAIL, "unable to open group");
    }
    /* Register an atom for the group */
    if ((ret_value = H5I_register(H5I_GROUP, grp)) < 0) {
        H5G_close(grp);
        HRETURN_ERROR(H5E_ATOM, H5E_CANTREGISTER, FAIL,
                      "unable to register group");
    }
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Gclose
 *
 * Purpose:     Closes the specified group.  The group ID will no longer be
 *              valid for accessing the group.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Wednesday, December 31, 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gclose(hid_t group_id)
{
    FUNC_ENTER(H5Gclose, FAIL);
    H5TRACE1("e","i",group_id);

    /* Check args */
    if (H5I_GROUP != H5I_get_type(group_id) ||
        NULL == H5I_object(group_id)) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a group");
    }
    /*
     * Decrement the counter on the group atom.  It will be freed if the count
     * reaches zero.
     */
    if (H5I_dec_ref(group_id) < 0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to close group");
    }
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Giterate
 *
 * Purpose:     Iterates over the entries of a group.  The LOC_ID and NAME
 *              identify the group over which to iterate and IDX indicates
 *              where to start iterating (zero means at the beginning).  The
 *              OPERATOR is called for each member and the iteration
 *              continues until the operator returns non-zero or all members
 *              are processed. The operator is passed a group ID for the
 *              group being iterated, a member name, and OP_DATA for each
 *              member.
 *
 * Return:      Success:        The return value of the first operator that
 *                              returns non-zero, or zero if all members were
 *                              processed with no operator returning non-zero.
 *
 *              Failure:        Negative if something goes wrong within the
 *                              library, or the negative value returned by one
 *                              of the operators.
 *
 * Programmer:  Robb Matzke
 *              Monday, March 23, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Giterate(hid_t loc_id, const char *name, int *idx,
            H5G_iterate_t op, void *op_data)
{
    int                 _idx = 0;
    H5G_bt_ud2_t        udata;
    herr_t              ret_value = FAIL;
    H5G_entry_t         *loc = NULL;
    
    FUNC_ENTER (H5Giterate, FAIL);
    H5TRACE5("e","is*Isxx",loc_id,name,idx,op,op_data);

    /* Check args */
    if (NULL==(loc=H5G_loc (loc_id))) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified");
    }
    if (!idx) idx = &_idx;
    if (!op) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "no operator specified");
    }

    /*
     * Open the group on which to operate.  We also create a group ID which
     * we can pass to the application-defined operator.
     */
    if (NULL==(udata.group = H5G_open (loc, name))) {
        HRETURN_ERROR (H5E_SYM, H5E_CANTINIT, FAIL, "unable to open group");
    }
    if ((udata.group_id=H5I_register (H5I_GROUP, udata.group))<0) {
        H5G_close(udata.group);
        HRETURN_ERROR (H5E_SYM, H5E_CANTINIT, FAIL,
                       "unable to register group");
    }
    
    /* Build udata to pass through H5B_iterate() to H5G_node_iterate() */
    udata.skip = *idx;
    udata.op = op;
    udata.op_data = op_data;

    /* Set the number of entries looked at to zero */
    udata.final_ent = 0;

    /* Iterate over the group members */
    if ((ret_value = H5B_iterate (H5G_fileof(udata.group), H5B_SNODE,
              udata.group->ent.cache.stab.btree_addr, &udata))<0) {
        HERROR (H5E_SYM, H5E_CANTINIT, "iteration operator failed");
    }

    /* Set the index we stopped at */
    *idx=udata.final_ent;

    H5I_dec_ref (udata.group_id); /*also closes udata.group*/
    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5Gmove
 *
 * Purpose:     Renames an object within an HDF5 file.  The original name SRC
 *              is unlinked from the group graph and the new name DST is
 *              inserted as an atomic operation.  Both names are interpreted
 *              relative to LOC_ID which is either a file ID or a group ID.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, April  6, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gmove(hid_t loc_id, const char *src, const char *dst)
{
    H5G_entry_t         *loc=NULL;
    
    FUNC_ENTER (H5Gmove, FAIL);
    H5TRACE3("e","iss",loc_id,src,dst);

    if (NULL==(loc=H5G_loc(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!src || !*src) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "no current name specified");
    }
    if (!dst || !*dst) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL,
                      "no new name specified");
    }

    if (H5G_move(loc, src, dst)<0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL,
                      "unable to change object name");
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Glink
 *
 * Purpose:     Creates a link of the specified type from NEW_NAME to
 *              CUR_NAME.
 *
 *              If TYPE is H5G_LINK_HARD then CUR_NAME must name an existing
 *              object and both names are interpreted relative to LOC_ID
 *              which is either a file ID or a group ID.
 *
 *              If TYPE is H5G_LINK_SOFT then CUR_NAME can be anything and is
 *              interpreted at lookup time relative to the group which
 *              contains the final component of NEW_NAME.  For instance, if
 *              CUR_NAME is `./foo' and NEW_NAME is `./x/y/bar' and a request
 *              is made for `./x/y/bar' then the actual object looked up is
 *              `./x/y/./foo'.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, April  6, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Glink(hid_t loc_id, H5G_link_t type, const char *cur_name,
         const char *new_name)
{
    H5G_entry_t *loc = NULL;
    
    FUNC_ENTER (H5Glink, FAIL);
    H5TRACE4("e","iGlss",loc_id,type,cur_name,new_name);

    /* Check arguments */
    if (NULL==(loc=H5G_loc (loc_id))) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (type!=H5G_LINK_HARD && type!=H5G_LINK_SOFT) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "unrecognized link type");
    }
    if (!cur_name || !*cur_name) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "no current name specified");
    }
    if (!new_name || !*new_name) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL,
                       "no new name specified");
    }
    if (H5G_link (loc, type, cur_name, new_name, H5G_TARGET_NORMAL)<0) {
        HRETURN_ERROR (H5E_SYM, H5E_LINK, FAIL, "unable to create link");
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Gunlink
 *
 * Purpose:     Removes the specified NAME from the group graph and
 *              decrements the link count for the object to which NAME
 *              points.  If the link count reaches zero then all file-space
 *              associated with the object will be reclaimed (but if the
 *              object is open, then the reclamation of the file space is
 *              delayed until all handles to the object are closed).
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, April  6, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gunlink(hid_t loc_id, const char *name)
{
    H5G_entry_t *loc = NULL;
    
    FUNC_ENTER (H5Gunlink, FAIL);
    H5TRACE2("e","is",loc_id,name);

    /* Check arguments */
    if (NULL==(loc=H5G_loc(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name");
    }

    /* Unlink */
    if (H5G_unlink(loc, name)<0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to unlink object");
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Gget_objinfo
 *
 * Purpose:     Returns information about an object.  If FOLLOW_LINK is
 *              non-zero then all symbolic links are followed; otherwise all
 *              links except the last component of the name are followed.
 *
 * Return:      Non-negative on success, with the fields of STATBUF (if
 *              non-null) initialized. Negative on failure.
 *
 * Programmer:  Robb Matzke
 *              Monday, April 13, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gget_objinfo(hid_t loc_id, const char *name, hbool_t follow_link,
               H5G_stat_t *statbuf/*out*/)
{
    H5G_entry_t *loc = NULL;
    
    FUNC_ENTER (H5Gget_objinfo, FAIL);
    H5TRACE4("e","isbx",loc_id,name,follow_link,statbuf);

    /* Check arguments */
    if (NULL==(loc=H5G_loc (loc_id))) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified");
    }

    /* Get info */
    if (H5G_get_objinfo (loc, name, follow_link, statbuf)<0) {
        HRETURN_ERROR (H5E_ARGS, H5E_CANTINIT, FAIL, "cannot stat object");
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Gget_linkval
 *
 * Purpose:     Returns the value of a symbolic link whose name is NAME.  At
 *              most SIZE characters (counting the null terminator) are
 *              copied to the BUF result buffer.
 *
 * Return:      Success:        Non-negative with the link value in BUF.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, April 13, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gget_linkval(hid_t loc_id, const char *name, size_t size, char *buf/*out*/)
{
    H5G_entry_t *loc = NULL;
    
    FUNC_ENTER (H5Gget_linkval, FAIL);
    H5TRACE4("e","iszx",loc_id,name,size,buf);

    /* Check arguments */
    if (NULL==(loc=H5G_loc (loc_id))) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified");
    }

    /* Get the link value */
    if (H5G_linkval (loc, name, size, buf)<0) {
        HRETURN_ERROR (H5E_SYM, H5E_NOTFOUND, FAIL,
                       "unable to get link value");
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Gset_comment
 *
 * Purpose:     Gives the specified object a comment.  The COMMENT string
 *              should be a null terminated string.  An object can have only
 *              one comment at a time.  Passing NULL for the COMMENT argument
 *              will remove the comment property from the object.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, July 20, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5Gset_comment(hid_t loc_id, const char *name, const char *comment)
{
    H5G_entry_t *loc = NULL;
    
    FUNC_ENTER(H5Gset_comment, FAIL);
    H5TRACE3("e","iss",loc_id,name,comment);

    if (NULL==(loc=H5G_loc(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified");
    }

    if (H5G_set_comment(loc, name, comment)<0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL,
                      "unable to set comment value");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5Gget_comment
 *
 * Purpose:     Return at most BUFSIZE characters of the comment for the
 *              specified object.  If BUFSIZE is large enough to hold the
 *              entire comment then the comment string will be null
 *              terminated, otherwise it will not.  If the object does not
 *              have a comment value then no bytes are copied to the BUF
 *              buffer.
 *
 * Return:      Success:        Number of characters in the comment counting
 *                              the null terminator.  The value returned may
 *                              be larger than the BUFSIZE argument.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, July 20, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5Gget_comment(hid_t loc_id, const char *name, size_t bufsize, char *buf)
{
    H5G_entry_t *loc = NULL;
    int retval = FAIL;
    
    FUNC_ENTER(H5Gget_comment, FAIL);
    H5TRACE4("Is","iszs",loc_id,name,bufsize,buf);

    if (NULL==(loc=H5G_loc(loc_id))) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADTYPE, FAIL, "not a location");
    }
    if (!name || !*name) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no name specified");
    }
    if (bufsize>0 && !buf) {
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, FAIL, "no buffer specified");
    }

    if ((retval=H5G_get_comment(loc, name, bufsize, buf))<0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL,
                      "unable to get comment value");
    }

    FUNC_LEAVE(retval);
}

/*
 *-------------------------------------------------------------------------
 *-------------------------------------------------------------------------
 *   N O   A P I   F U N C T I O N S   B E Y O N D   T H I S   P O I N T
 *-------------------------------------------------------------------------
 *------------------------------------------------------------------------- 
 */

/*-------------------------------------------------------------------------
 * Function:    H5G_init_interface
 *
 * Purpose:     Initializes the H5G interface.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, January  5, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_init_interface(void)
{
    FUNC_ENTER(H5G_init_interface, FAIL);

    /* Initialize the atom group for the group IDs */
    if (H5I_init_group(H5I_GROUP, H5I_GROUPID_HASHSIZE, H5G_RESERVED_ATOMS,
                       (H5I_free_t)H5G_close) < 0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL,
                      "unable to initialize interface");
    }

    /*
     * Initialize the type info table.  Begin with the most general types and
     * end with the most specific. For instance, any object that has a data
     * type message is a data type but only some of them are datasets.
     */
    H5G_register_type(H5G_TYPE,    H5T_isa,  "data type");
    H5G_register_type(H5G_GROUP,   H5G_isa,  "group");
    H5G_register_type(H5G_DATASET, H5D_isa,  "dataset");

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_term_interface
 *
 * Purpose:     Terminates the H5G interface
 *
 * Return:      Success:        Positive if anything is done that might
 *                              affect other interfaces; zero otherwise.
 *
 *              Failure:        Negative.
 *
 * Programmer:  Robb Matzke
 *              Monday, January  5, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5G_term_interface(void)
{
    size_t      i;
    int n=0;
    
    if (interface_initialize_g) {
        if ((n=H5I_nmembers(H5I_GROUP))) {
            H5I_clear_group(H5I_GROUP, FALSE);
        } else {
            /* Empty the object type table */
            for (i=0; i<H5G_ntypes_g; i++) {
                H5MM_xfree(H5G_type_g[i].desc);
            }
            H5G_ntypes_g = H5G_atypes_g = 0;
            H5G_type_g = H5MM_xfree(H5G_type_g);
    
            /* Destroy the group object id group */
            H5I_destroy_group(H5I_GROUP);

            /* Mark closed */
            interface_initialize_g = 0;
            n = 1; /*H5I*/
        }
    }
    
    return n;
}


/*-------------------------------------------------------------------------
 * Function:    H5G_register_type
 *
 * Purpose:     Register a new object type so H5G_get_type() can detect it.
 *              One should always register a general type before a more
 *              specific type.  For instance, any object that has a data type
 *              message is a data type, but only some of those objects are
 *              datasets.
 *
 * Return:      Success:        Non-negative
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Wednesday, November  4, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_register_type(int type, htri_t(*isa)(H5G_entry_t*), const char *_desc)
{
    char        *desc = NULL;
    size_t      i;
    herr_t      ret_value = FAIL;
    
    FUNC_ENTER(H5G_register_type, FAIL);
    assert(type>=0);
    assert(isa);
    assert(_desc);

    /* Copy the description */
    if (NULL==(desc=H5MM_strdup(_desc))) {
        HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                    "memory allocation failed for object type description");
    }

    /*
     * If the type is already registered then just update its entry without
     * moving it to the end
     */
    for (i=0; i<H5G_ntypes_g; i++) {
        if (H5G_type_g[i].type==type) {
            H5G_type_g[i].isa = isa;
            H5MM_xfree(H5G_type_g[i].desc);
            H5G_type_g[i].desc = desc;
            ret_value = SUCCEED;
            goto done;
        }
    }

    /* Increase table size */
    if (H5G_ntypes_g>=H5G_atypes_g) {
        size_t n = MAX(32, 2*H5G_atypes_g);
        H5G_typeinfo_t *x = H5MM_realloc(H5G_type_g,
                                         n*sizeof(H5G_typeinfo_t));
        if (!x) {
            HGOTO_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                        "memory allocation failed for objec type table");
        }
        H5G_atypes_g = n;
        H5G_type_g = x;
    }
    
    /* Add a new entry */
    H5G_type_g[H5G_ntypes_g].type = type;
    H5G_type_g[H5G_ntypes_g].isa = isa;
    H5G_type_g[H5G_ntypes_g].desc = desc; /*already copied*/
    H5G_ntypes_g++;

    ret_value = SUCCEED;

 done:
    if (ret_value<0) H5MM_xfree(desc);
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_component
 *
 * Purpose:     Returns the pointer to the first component of the
 *              specified name by skipping leading slashes.  Returns
 *              the size in characters of the component through SIZE_P not
 *              counting leading slashes or the null terminator.
 *
 * Errors:
 *
 * Return:      Success:        Ptr into NAME.
 *
 *              Failure:        Ptr to the null terminator of NAME.
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 11 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static const char *
H5G_component(const char *name, size_t *size_p)
{
    assert(name);

    while ('/' == *name) name++;
    if (size_p) *size_p = HDstrcspn(name, "/");
    return name;
}


/*-------------------------------------------------------------------------
 * Function:    H5G_basename
 *
 * Purpose:     Returns a pointer to the last component of the specified
 *              name. The length of the component is returned through SIZE_P.
 *              The base name is followed by zero or more slashes and a null
 *              terminator, but SIZE_P does not count the slashes or the null
 *              terminator.
 *
 * Note:        The base name of the root directory is a single slash.
 *
 * Return:      Success:        Ptr to base name.
 *
 *              Failure:        Ptr to the null terminator.
 *
 * Programmer:  Robb Matzke
 *              Thursday, September 17, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static const char *
H5G_basename(const char *name, size_t *size_p)
{
    size_t      i;
    
    FUNC_ENTER(H5G_basename, NULL);

    /* Find the end of the base name */
    i = strlen(name);
    while (i>0 && '/'==name[i-1]) --i;

    /* Skip backward over base name */
    while (i>0 && '/'!=name[i-1]) --i;

    /* Watch out for root special case */
    if ('/'==name[i] && size_p) *size_p = 1;

    FUNC_LEAVE(name+i);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_namei
 *
 * Purpose:     Translates a name to a symbol table entry.
 *
 *              If the specified name can be fully resolved, then this
 *              function returns the symbol table entry for the named object
 *              through the OBJ_ENT argument. The symbol table entry for the
 *              group containing the named object is returned through the
 *              GRP_ENT argument if it is non-null.  However, if the name
 *              refers to the root object then the GRP_ENT will be
 *              initialized with an undefined object header address.  The
 *              REST argument, if present, will point to the null terminator
 *              of NAME.
 *
 *              If the specified name cannot be fully resolved, then OBJ_ENT
 *              is initialized with the undefined object header address. The
 *              REST argument will point into the NAME argument to the start
 *              of the component that could not be located.  The GRP_ENT will
 *              contain the entry for the symbol table that was being
 *              searched at the time of the failure and will have an
 *              undefined object header address if the search failed at the
 *              root object. For instance, if NAME is `/foo/bar/baz' and the
 *              root directory exists and contains an entry for `foo', and
 *              foo is a group that contains an entry for bar, but bar is not
 *              a group, then the results will be that REST points to `baz',
 *              OBJ_ENT has an undefined object header address, and GRP_ENT
 *              is the symbol table entry for `bar' in `/foo'.
 *
 *              Every file has a root group whose name is `/'.  Components of
 *              a name are separated from one another by one or more slashes
 *              (/).  Slashes at the end of a name are ignored.  If the name
 *              begins with a slash then the search begins at the root group
 *              of the file containing LOC_ENT. Otherwise it begins at
 *              LOC_ENT.  The component `.' is a no-op, but `..' is not
 *              understood by this function (unless it appears as an entry in
 *              the symbol table).
 *
 *              Symbolic links are followed automatically, but if TARGET
 *              includes the H5G_TARGET_SLINK bit and the last component of
 *              the name is a symbolic link then that link is not followed.
 *              The *NLINKS value is decremented each time a link is followed
 *              and link traversal fails if the value would become negative.
 *              If NLINKS is the null pointer then a default value is used.
 *
 *              Mounted files are handled by calling H5F_mountpoint() after
 *              each step of the translation.  If the input argument to that
 *              function is a mount point then the argument shall be replaced
 *              with information about the root group of the mounted file.
 *              But if TARGET includes the H5G_TARGET_MOUNT bit and the last
 *              component of the name is a mount point then H5F_mountpoint()
 *              is not called and information about the mount point itself is
 *              returned.
 *              
 * Errors:
 *
 * Return:      Success:        Non-negative if name can be fully resolved.
 *                              See above for values of REST, GRP_ENT, and
 *                              OBJ_ENT.  NLINKS has been decremented for
 *                              each symbolic link that was followed.
 *
 *              Failure:        Negative if the name could not be fully
 *                              resolved. See above for values of REST,
 *                              GRP_ENT, and OBJ_ENT.
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 11 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
static herr_t
H5G_namei(H5G_entry_t *loc_ent, const char *name, const char **rest/*out*/,
          H5G_entry_t *grp_ent/*out*/, H5G_entry_t *obj_ent/*out*/,
          unsigned target, int *nlinks)
{
    H5G_entry_t         _grp_ent;       /*entry for current group       */
    H5G_entry_t         _obj_ent;       /*entry found                   */
    size_t              nchars;         /*component name length         */
    char                comp[1024];     /*component name buffer         */
    int                 _nlinks = H5G_NLINKS;
    const char          *s = NULL;
    
    if (rest) *rest = name;
    if (!grp_ent) grp_ent = &_grp_ent;
    if (!obj_ent) obj_ent = &_obj_ent;
    if (!nlinks) nlinks = &_nlinks;
    
    FUNC_ENTER(H5G_namei, FAIL);

    /*
     * Where does the searching start?  For absolute names it starts at the
     * root of the file; for relative names it starts at CWG.
     */
    if (!name || !*name) {
        HRETURN_ERROR (H5E_SYM, H5E_NOTFOUND, FAIL, "no name given");
    } else if (!loc_ent) {
        HRETURN_ERROR (H5E_SYM, H5E_NOTFOUND, FAIL,
                       "no current working group");
    } else if ('/' == *name) {
        *obj_ent = H5G_rootof(loc_ent->file)->ent;
    } else {
        *obj_ent = *loc_ent;
    }
    HDmemset(grp_ent, 0, sizeof(H5G_entry_t));
    grp_ent->header = HADDR_UNDEF;

    /* traverse the name */
    while ((name = H5G_component(name, &nchars)) && *name) {
        if (rest) *rest = name;

        /*
         * Copy the component name into a null-terminated buffer so
         * we can pass it down to the other symbol table functions.
         */
        if (nchars+1 > sizeof(comp)) {
            HRETURN_ERROR (H5E_SYM, H5E_COMPLEN, FAIL,
                           "component is too long");
        }
        HDmemcpy(comp, name, nchars);
        comp[nchars] = '\0';

        /*
         * The special name `.' is a no-op.
         */
        if ('.' == comp[0] && !comp[1]) {
            name += nchars;
            continue;
        }

        /*
         * Advance to the next component of the name.
         */
        *grp_ent = *obj_ent;
        HDmemset(obj_ent, 0, sizeof(H5G_entry_t));
        obj_ent->header = HADDR_UNDEF;

        if (H5G_stab_find(grp_ent, comp, obj_ent/*out*/)<0) {
            /*
             * Component was not found in the current symbol table, possibly
             * because GRP_ENT isn't a symbol table.
             */
            HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "component not found");
        }

        /*
         * If we found a symbolic link then we should follow it.  But if this
         * is the last component of the name and the H5G_TARGET_SLINK bit of
         * TARGET is set then we don't follow it.
         */
        if (H5G_CACHED_SLINK==obj_ent->type &&
            (0==(target & H5G_TARGET_SLINK) ||
             ((s=H5G_component(name+nchars, NULL)) && *s))) {
            if ((*nlinks)-- <= 0) {
                HRETURN_ERROR (H5E_SYM, H5E_SLINK, FAIL,
                               "too many symbolic links");
            }
            if (H5G_traverse_slink (grp_ent, obj_ent, nlinks)<0) {
                HRETURN_ERROR (H5E_SYM, H5E_NOTFOUND, FAIL,
                               "symbolic link traversal failed");
            }
        }

        /*
         * Resolve mount points to the mounted group.  Do not do this step if
         * the H5G_TARGET_MOUNT bit of TARGET is set and this is the last
         * component of the name.
         */
        if (0==(target & H5G_TARGET_MOUNT) ||
            ((s=H5G_component(name+nchars, NULL)) && *s)) {
            H5F_mountpoint(obj_ent/*in,out*/);
        }
        
        /* next component */
        name += nchars;
    }
    if (rest) *rest = name; /*final null */

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_traverse_slink
 *
 * Purpose:     Traverses symbolic link.  The link head appears in the group
 *              whose entry is GRP_ENT and the link head entry is OBJ_ENT.
 *
 * Return:      Success:        Non-negative, OBJ_ENT will contain information
 *                              about the object to which the link points and
 *                              GRP_ENT will contain the information about
 *                              the group in which the link tail appears.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Friday, April 10, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_traverse_slink (H5G_entry_t *grp_ent/*in,out*/,
                    H5G_entry_t *obj_ent/*in,out*/,
                    int *nlinks/*in,out*/)
{
    H5O_stab_t          stab_mesg;              /*info about local heap */
    const char          *clv = NULL;            /*cached link value     */
    char                *linkval = NULL;        /*the copied link value */
    herr_t              ret_value = FAIL;       /*return value          */
    
    FUNC_ENTER (H5G_traverse_slink, FAIL);

    /* Get the link value */
    if (NULL==H5O_read (grp_ent, H5O_STAB, 0, &stab_mesg)) {
        HGOTO_ERROR (H5E_SYM, H5E_NOTFOUND, FAIL,
                     "unable to determine local heap address");
    }
    if (NULL==(clv=H5HL_peek (grp_ent->file, stab_mesg.heap_addr,
                              obj_ent->cache.slink.lval_offset))) {
        HGOTO_ERROR (H5E_SYM, H5E_NOTFOUND, FAIL,
                     "unable to read symbolic link value");
    }
    linkval = H5MM_xstrdup (clv);

    /* Traverse the link */
    if (H5G_namei (grp_ent, linkval, NULL, grp_ent, obj_ent, H5G_TARGET_NORMAL,
                   nlinks)) {
        HGOTO_ERROR (H5E_SYM, H5E_NOTFOUND, FAIL,
                     "unable to follow symbolic link");
    }
    ret_value = SUCCEED;

 done:
    H5MM_xfree (linkval);
    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_mkroot
 *
 * Purpose:     Creates a root group in an empty file and opens it.  If a
 *              root group is already open then this function immediately
 *              returns.   If ENT is non-null then it's the symbol table
 *              entry for an existing group which will be opened as the root
 *              group.  Otherwise a new root group is created and then
 *              opened.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 11 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_mkroot (H5F_t *f, H5G_entry_t *ent)
{
    H5G_entry_t new_root;               /*new root object               */
    H5O_stab_t  stab;                   /*symbol table message          */

    FUNC_ENTER(H5G_mkroot, FAIL);

    /* check args */
    assert(f);
    if (f->shared->root_grp) HRETURN(SUCCEED);

    /*
     * If there is no root object then create one. The root group always has
     * a hard link count of one since it's pointed to by the boot block.
     */
    if (!ent) {
        ent = &new_root;
        if (H5G_stab_create (f, 256, ent/*out*/)<0) {
            HRETURN_ERROR (H5E_SYM, H5E_CANTINIT, FAIL,
                           "unable to create root group");
        }
        if (1 != H5O_link (ent, 1)) {
            HRETURN_ERROR (H5E_SYM, H5E_LINK, FAIL,
                           "internal error (wrong link count)");
        }
    } else {
        /*
         * Open the root object as a group.
         */
        if (H5O_open (ent)<0) {
            HRETURN_ERROR (H5E_SYM, H5E_CANTOPENOBJ, FAIL,
                           "unable to open root group");
        }
        if (NULL==H5O_read (ent, H5O_STAB, 0, &stab)) {
            H5O_close(ent);
            HRETURN_ERROR (H5E_SYM, H5E_NOTFOUND, FAIL,
                           "root object is not a group");
        }
        H5O_reset (H5O_STAB, &stab);
    }

    /*
     * Create the group pointer.  Also decrement the open object count so we
     * don't count the root group as an open object.  The root group will
     * never be closed.
     */
    if (NULL==(f->shared->root_grp = H5FL_ALLOC (H5G_t,1))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, FAIL,
                       "memory allocation failed");
    }
    f->shared->root_grp->ent = *ent;
    f->shared->root_grp->nref = 1;
    assert (1==f->nopen_objs);
    f->nopen_objs = 0;

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_create
 *
 * Purpose:     Creates a new empty group with the specified name. The name
 *              is either an absolute name or is relative to LOC.
 *
 * Errors:
 *
 * Return:      Success:        A handle for the group.  The group is opened
 *                              and should eventually be close by calling
 *                              H5G_close().
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 11 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5G_t *
H5G_create(H5G_entry_t *loc, const char *name, size_t size_hint)
{
    const char  *rest = NULL;           /*the base name                 */
    H5G_entry_t grp_ent;                /*group containing new group    */
    char        _comp[1024];            /*name component                */
    size_t      nchars;                 /*number of characters in compon*/
    H5G_t       *grp = NULL;            /*new group                     */

    FUNC_ENTER(H5G_create, NULL);

    /* check args */
    assert(loc);
    assert(name && *name);

    /* lookup name */
    if (0 == H5G_namei(loc, name, &rest, &grp_ent, NULL, H5G_TARGET_NORMAL,
                       NULL)) {
        HRETURN_ERROR(H5E_SYM, H5E_EXISTS, NULL, "already exists");
    }
    H5E_clear(); /*it's OK that we didn't find it */
    assert(H5F_addr_defined(grp_ent.header));

    /* should be one null-terminated component left */
    rest = H5G_component(rest, &nchars);
    assert(rest && *rest);
    if (rest[nchars]) {
        const char *t = H5G_component(rest+nchars, NULL);
        if (t && *t) {
            HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "missing component");
        } else if (nchars+1 > sizeof _comp) {
            HRETURN_ERROR(H5E_SYM, H5E_COMPLEN, NULL, "component is too long");
        } else {
            /* null terminate */
            HDmemcpy(_comp, rest, nchars);
            _comp[nchars] = '\0';
            rest = _comp;
        }
    }
    
    /* create an open group */
    if (NULL==(grp = H5FL_ALLOC(H5G_t,1))) {
        HRETURN_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                       "memory allocation failed");
    }
    if (H5G_stab_create(grp_ent.file, size_hint, &(grp->ent)/*out*/) < 0) {
        grp = H5FL_FREE(H5G_t,grp);
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, NULL, "can't create grp");
    }
    
    /* insert child name into parent */
    if (1!=H5O_link(&(grp->ent), 1)) {
        HRETURN_ERROR(H5E_SYM, H5E_LINK, NULL, "link inc failure");
    }
    if (H5G_stab_insert(&grp_ent, rest, &(grp->ent)) < 0) {
        H5O_close(&(grp->ent));
        grp = H5FL_FREE(H5G_t,grp);
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, NULL, "can't insert");
    }
    grp->nref = 1;
    FUNC_LEAVE(grp);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_isa
 *
 * Purpose:     Determines if an object has the requisite messages for being
 *              a group.
 *
 * Return:      Success:        TRUE if the required group messages are
 *                              present; FALSE otherwise.
 *
 *              Failure:        FAIL if the existence of certain messages
 *                              cannot be determined.
 *
 * Programmer:  Robb Matzke
 *              Monday, November  2, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
htri_t
H5G_isa(H5G_entry_t *ent)
{
    htri_t      exists;
    
    FUNC_ENTER(H5G_isa, FAIL);
    assert(ent);

    if ((exists=H5O_exists(ent, H5O_STAB, 0))<0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL,
                      "unable to read object header");
    }
    FUNC_LEAVE(exists);
}
    

/*-------------------------------------------------------------------------
 * Function:    H5G_open
 *
 * Purpose:     Opens an existing group.  The group should eventually be
 *              closed by calling H5G_close().
 *
 * Return:      Success:        Ptr to a new group.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Monday, January  5, 1998
 *
 * Modifications:
 *      Modified to call H5G_open_oid - QAK - 3/17/99
 *
 *-------------------------------------------------------------------------
 */
H5G_t *
H5G_open(H5G_entry_t *loc, const char *name)
{
    H5G_t               *grp = NULL;
    H5G_t               *ret_value = NULL;
    H5G_entry_t ent;                    /*dataset symbol table entry    */

    FUNC_ENTER(H5G_open, NULL);

    /* Check args */
    assert(loc);
    assert(name && *name);

    /* Open the object, making sure it's a group */
    if (H5G_find(loc, name, NULL, &ent/*out*/) < 0) {
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "group not found");
    }
    /* Open the group object */
    if ((grp=H5G_open_oid(&ent)) ==NULL) {
        HGOTO_ERROR(H5E_SYM, H5E_NOTFOUND, NULL, "not found");
    }
    ret_value = grp;

done:
    if (!ret_value && grp) {
        H5FL_FREE(H5G_t,grp);
    }
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_open_oid
 *
 * Purpose:     Opens an existing group.  The group should eventually be
 *              closed by calling H5G_close().
 *
 * Return:      Success:        Ptr to a new group.
 *
 *              Failure:        NULL
 *
 * Programmer:  Quincey Koziol
 *          Wednesday, March    17, 1999
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5G_t *
H5G_open_oid(H5G_entry_t *ent)
{
    H5G_t               *grp = NULL;
    H5G_t               *ret_value = NULL;
    H5O_stab_t          mesg;

    FUNC_ENTER(H5G_open_oid, NULL);

    /* Check args */
    assert(ent);

    /* Open the object, making sure it's a group */
    if (NULL==(grp = H5FL_ALLOC(H5G_t,1))) {
        HGOTO_ERROR (H5E_RESOURCE, H5E_NOSPACE, NULL,
                     "memory allocation failed");
    }

    /* Copy over the symbol table information if it's provided */
    HDmemcpy(&(grp->ent),ent,sizeof(H5G_entry_t));

    /* Grab the object header */
    if (H5O_open(&(grp->ent)) < 0) {
        HGOTO_ERROR(H5E_SYM, H5E_CANTOPENOBJ, NULL, "unable to open group");
    }
    if (NULL==H5O_read (&(grp->ent), H5O_STAB, 0, &mesg)) {
        H5O_close(&(grp->ent));
        HGOTO_ERROR (H5E_SYM, H5E_CANTOPENOBJ, NULL, "not a group");
    }
    grp->nref = 1;
    ret_value = grp;

 done:
    if (!ret_value && grp) {
        H5FL_FREE(H5G_t,grp);
    }
    FUNC_LEAVE(ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_reopen
 *
 * Purpose:     Reopens a group by incrementing the open count.
 *
 * Return:      Success:        The GRP argument.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Monday, January  5, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5G_t *
H5G_reopen(H5G_t *grp)
{
    FUNC_ENTER(H5G_reopen, NULL);

    assert(grp);
    assert(grp->nref > 0);

    grp->nref++;

    FUNC_LEAVE(grp);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_close
 *
 * Purpose:     Closes the specified group.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, January  5, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_close(H5G_t *grp)
{
    FUNC_ENTER(H5G_close, FAIL);

    /* Check args */
    assert(grp);
    assert(grp->nref > 0);

    if (1 == grp->nref) {
        assert (grp!=H5G_rootof(H5G_fileof(grp)));
        if (H5O_close(&(grp->ent)) < 0) {
            HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to close");
        }
        grp->nref = 0;
        H5FL_FREE (H5G_t,grp);
    } else {
        --grp->nref;
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_rootof
 *
 * Purpose:     Return a pointer to the root group of the file.  If the file
 *              is part of a virtual file then the root group of the virtual
 *              file is returned.
 *
 * Return:      Success:        Ptr to the root group of the file.  Do not
 *                              free the pointer -- it points directly into
 *                              the file struct.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, October 13, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5G_t *
H5G_rootof(H5F_t *f)
{
    FUNC_ENTER(H5G_rootof, NULL);
    while (f->mtab.parent) f = f->mtab.parent;
    FUNC_LEAVE(f->shared->root_grp);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_insert
 *
 * Purpose:     Inserts a symbol table entry into the group graph.
 *
 * Errors:
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
H5G_insert(H5G_entry_t *loc, const char *name, H5G_entry_t *ent)
{
    const char  *rest = NULL;   /*part of name not existing yet */
    H5G_entry_t grp;            /*entry for group to contain obj */
    size_t      nchars;         /*number of characters in name  */
    char        _comp[1024];    /*name component                */

    FUNC_ENTER(H5G_insert, FAIL);

    /* Check args. */
    assert (loc);
    assert (name && *name);
    assert (ent);

    /*
     * Look up the name -- it shouldn't exist yet.
     */
    if (H5G_namei(loc, name, &rest, &grp, NULL, H5G_TARGET_NORMAL, NULL)>=0) {
        HRETURN_ERROR(H5E_SYM, H5E_EXISTS, FAIL, "already exists");
    }
    H5E_clear(); /*it's OK that we didn't find it */
    rest = H5G_component(rest, &nchars);

    /*
     * There should be one component left.  Make sure it's null
     * terminated.
     */
    if (rest[nchars]) {
        if (H5G_component(rest + nchars, NULL)) {
            HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "component not found");
        } else if (nchars + 1 > sizeof _comp) {
            HRETURN_ERROR(H5E_SYM, H5E_COMPLEN, FAIL, "component is too long");
        } else {
            /* null terminate */
            HDmemcpy(_comp, rest, nchars);
            _comp[nchars] = '\0';
            rest = _comp;
        }
    }

    /*
     * Insert the object into a symbol table.
     */
    if (H5O_link(ent, 1) < 0) {
        HRETURN_ERROR(H5E_SYM, H5E_LINK, FAIL,
                      "unable to increment hard link count");
    }
    if (H5G_stab_insert(&grp, rest, ent) < 0) {
        H5O_link(ent, -1);
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL, "unable to insert name");
    }
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_find
 *
 * Purpose:     Finds an object with the specified NAME at location LOC.  On
 *              successful return, GRP_ENT (if non-null) will be initialized
 *              with the symbol table information for the group in which the
 *              object appears (it will have an undefined object header
 *              address if the object is the root object) and OBJ_ENT will be
 *              initialized with the symbol table entry for the object
 *              (OBJ_ENT is optional when the caller is interested only in
 *              the existence of the object).
 *
 * Errors:
 *
 * Return:      Success:        Non-negative, see above for values of GRP_ENT
 *                              and OBJ_ENT.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              matzke@llnl.gov
 *              Aug 12 1997
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_find(H5G_entry_t *loc, const char *name,
         H5G_entry_t *grp_ent/*out*/, H5G_entry_t *obj_ent/*out*/)
{
    FUNC_ENTER(H5G_find, FAIL);

    /* check args */
    assert (loc);
    assert (name && *name);

    if (H5G_namei(loc, name, NULL, grp_ent, obj_ent, H5G_TARGET_NORMAL,
                  NULL)<0) {
        HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object not found");
    }
    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_entof
 *
 * Purpose:     Returns a pointer to the entry for a group.
 *
 * Return:      Success:        Ptr to group entry
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 24, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5G_entry_t *
H5G_entof (H5G_t *grp)
{
    return grp ? &(grp->ent) : NULL;
}


/*-------------------------------------------------------------------------
 * Function:    H5G_fileof
 *
 * Purpose:     Returns the file to which the specified group belongs.
 *
 * Return:      Success:        File pointer.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 24, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5F_t *
H5G_fileof (H5G_t *grp)
{
    assert (grp);
    return grp->ent.file;
}


/*-------------------------------------------------------------------------
 * Function:    H5G_loc
 *
 * Purpose:     Given an object ID return a symbol table entry for the
 *              object.
 *
 * Return:      Success:        Group pointer.
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Tuesday, March 24, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5G_entry_t *
H5G_loc (hid_t loc_id)
{
    H5F_t       *f;
    H5G_entry_t *ret_value = NULL;
    H5G_t       *group=NULL;
    H5T_t       *dt=NULL;
    H5D_t       *dset=NULL;
    H5A_t       *attr=NULL;

    FUNC_ENTER (H5G_loc, NULL);

    switch (H5I_get_type(loc_id)) {
    case H5I_FILE:
        if (NULL==(f=H5I_object (loc_id))) {
            HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, NULL, "invalid file ID");
        }
        if (NULL==(ret_value=H5G_entof(H5G_rootof(f)))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                          "unable to get symbol table entry for root group");
        }
        break;

    case H5I_TEMPLATE_0:
    case H5I_TEMPLATE_1:
    case H5I_TEMPLATE_2:
    case H5I_TEMPLATE_3:
    case H5I_TEMPLATE_4:
    case H5I_TEMPLATE_5:
    case H5I_TEMPLATE_6:
    case H5I_TEMPLATE_7:
    case H5I_TEMPLATE_MAX:
    case H5I_GENPROP_CLS:
    case H5I_GENPROP_LST:
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                      "unable to get symbol table entry of property list");

    case H5I_GROUP:
        if (NULL==(group=H5I_object (loc_id))) {
            HRETURN_ERROR (H5E_ARGS, H5E_BADVALUE, NULL, "invalid group ID");
        }
        if (NULL==(ret_value=H5G_entof(group))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                          "unable to get symbol table entry of group");
        }
        break;

    case H5I_DATATYPE:
        if (NULL==(dt=H5I_object(loc_id))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid type ID");
        }
        if (NULL==(ret_value=H5T_entof(dt))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                          "unable to get symbol table entry of data type");
        }
        break;

    case H5I_DATASPACE:
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                      "unable to get symbol table entry of data space");

    case H5I_DATASET:
        if (NULL==(dset=H5I_object(loc_id))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid data ID");
        }
        if (NULL==(ret_value=H5D_entof(dset))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                          "unable to get symbol table entry of dataset");
        }
        break;

    case H5I_ATTR:
        if (NULL==(attr=H5I_object(loc_id))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                          "invalid attribute ID");
        }
        if (NULL==(ret_value=H5A_entof(attr))) {
            HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                          "unable to get symbol table entry of attribute");
        }
        break;
            
    case H5I_TEMPBUF:
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL,
                      "unable to get symbol table entry of buffer");
    
    case H5I_NGROUPS:
    case H5I_BADID:
    case H5I_FILE_CLOSING:
    case H5I_REFERENCE:
    case H5I_VFL:
        HRETURN_ERROR(H5E_ARGS, H5E_BADVALUE, NULL, "invalid object ID");
    }

    FUNC_LEAVE (ret_value);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_link
 *
 * Purpose:     Creates a link from NEW_NAME to CUR_NAME.  See H5Glink() for
 *              full documentation.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, April  6, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_link (H5G_entry_t *loc, H5G_link_t type, const char *cur_name,
          const char *new_name, unsigned namei_flags)
{
    H5G_entry_t         cur_obj;        /*entry for the link tail       */
    H5G_entry_t         grp_ent;        /*ent for grp containing link hd*/
    H5O_stab_t          stab_mesg;      /*symbol table message          */
    const char          *rest = NULL;   /*last component of new name    */
    char                _comp[1024];    /*name component                */
    size_t              nchars;         /*characters in component       */
    size_t              offset;         /*offset to sym-link value      */
    
    FUNC_ENTER (H5G_link, FAIL);

    /* Check args */
    assert (loc);
    assert (cur_name && *cur_name);
    assert (new_name && *new_name);

    switch (type) {
    case H5G_LINK_SOFT:
        /*
         * Lookup the the new_name so we can get the group which will contain
         * the new entry.  The entry shouldn't exist yet.
         */
        if (H5G_namei (loc, new_name, &rest, &grp_ent, NULL, H5G_TARGET_NORMAL,
                       NULL)>=0) {
            HRETURN_ERROR (H5E_SYM, H5E_EXISTS, FAIL, "already exists");
        }
        H5E_clear (); /*it's okay that we didn't find it*/
        rest = H5G_component (rest, &nchars);

        /*
         * There should be one component left.  Make sure it's null
         * terminated and that `rest' points to it.
         */
        if (rest[nchars]) {
            if (H5G_component (rest+nchars, NULL)) {
                HRETURN_ERROR (H5E_SYM, H5E_NOTFOUND, FAIL,
                               "component not found");
            } else if (nchars+1 > sizeof _comp) {
                HRETURN_ERROR (H5E_SYM, H5E_COMPLEN, FAIL,
                               "name component is too long");
            } else {
                HDmemcpy (_comp, rest, nchars);
                _comp[nchars] = '\0';
                rest = _comp;
            }
        }

        /*
         * Add the link-value to the local heap for the symbol table which
         * will contain the link.
         */
        if (NULL==H5O_read (&grp_ent, H5O_STAB, 0, &stab_mesg)) {
            HRETURN_ERROR (H5E_SYM, H5E_CANTINIT, FAIL,
                           "unable to determine local heap address");
        }
        if ((size_t)(-1)==(offset=H5HL_insert (grp_ent.file,
                                               stab_mesg.heap_addr,
                                               HDstrlen(cur_name)+1,
                                               cur_name))) {
            HRETURN_ERROR (H5E_SYM, H5E_CANTINIT, FAIL,
                           "unable to write link value to local heap");
        }
        H5O_reset (H5O_STAB, &stab_mesg);

        /*
         * Create a symbol table entry for the link.  The object header is
         * undefined and the cache contains the link-value offset.
         */
        HDmemset (&cur_obj, 0, sizeof cur_obj);
        cur_obj.header = HADDR_UNDEF;
        cur_obj.file = grp_ent.file;
        cur_obj.type = H5G_CACHED_SLINK;
        cur_obj.cache.slink.lval_offset = offset;

        /*
         * Insert the link head in the symbol table.  This shouldn't ever
         * fail because we've already checked that the link head doesn't
         * exist and the file is writable (because the local heap is
         * writable).  But if it does, the only side effect is that the local
         * heap has some extra garbage in it.
         */
        if (H5G_stab_insert (&grp_ent, rest, &cur_obj)<0) {
            HRETURN_ERROR (H5E_SYM, H5E_CANTINIT, FAIL,
                           "unable to create new name/link for object");
        }
        break;

    case H5G_LINK_HARD:
        if (H5G_namei(loc, cur_name, NULL, NULL, &cur_obj, namei_flags,
                      NULL)<0) {
            HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL,
                          "source object not found");
        }
        if (H5G_insert (loc, new_name, &cur_obj)<0) {
            HRETURN_ERROR (H5E_SYM, H5E_CANTINIT, FAIL,
                           "unable to create new name/link for object");
        }
        break;

    default:
        HRETURN_ERROR (H5E_SYM, H5E_BADVALUE, FAIL,
                       "unrecognized link type");
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_get_type
 *
 * Purpose:     Returns the type of object pointed to by `ent'.
 *
 * Return:      Success:        An object type defined in H5Gpublic.h
 *
 *              Failure:        H5G_UNKNOWN
 *
 * Programmer:  Robb Matzke
 *              Wednesday, November  4, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5G_get_type(H5G_entry_t *ent)
{
    htri_t      isa;
    size_t      i;
    
    FUNC_ENTER(H5G_get_type, H5G_UNKNOWN);

    for (i=H5G_ntypes_g; i>0; --i) {
        if ((isa=(H5G_type_g[i-1].isa)(ent))<0) {
            HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, H5G_UNKNOWN,
                          "unable to determine object type");
        } else if (isa) {
            HRETURN(H5G_type_g[i-1].type);
        }
    }

    if (0==i) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, H5G_UNKNOWN,
                      "unable to determine object type");
    }
    FUNC_LEAVE(H5G_UNKNOWN);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_get_objinfo
 *
 * Purpose:     Returns information about an object.
 *
 * Return:      Success:        Non-negative with info about the object
 *                              returned through STATBUF if it isn't the null
 *                              pointer.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, April 13, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_get_objinfo (H5G_entry_t *loc, const char *name, hbool_t follow_link,
                 H5G_stat_t *statbuf/*out*/)
{
    H5O_stab_t          stab_mesg;
    H5G_entry_t         grp_ent, obj_ent;
    const char          *s = NULL;
    
    FUNC_ENTER (H5G_get_objinfo, FAIL);

    assert (loc);
    assert (name && *name);
    if (statbuf) HDmemset (statbuf, 0, sizeof *statbuf);

    /* Find the object's symbol table entry */
    if (H5G_namei (loc, name, NULL, &grp_ent/*out*/, &obj_ent/*out*/,
                   (unsigned)(follow_link?H5G_TARGET_NORMAL:H5G_TARGET_SLINK), NULL)<0) {
        HRETURN_ERROR (H5E_SYM, H5E_NOTFOUND, FAIL, "unable to stat object");
    }

    /*
     * Initialize the stat buf.  Symbolic links aren't normal objects and
     * therefore don't have much of the normal info.  However, the link value
     * length is specific to symbolic links.
     */
    if (statbuf) {
        if (H5G_CACHED_SLINK==obj_ent.type) {
            /* Named object is a symbolic link */
            if (NULL==H5O_read (&grp_ent, H5O_STAB, 0, &stab_mesg) ||
                NULL==(s=H5HL_peek (grp_ent.file, stab_mesg.heap_addr, 
                                    obj_ent.cache.slink.lval_offset))) {
                HRETURN_ERROR (H5E_SYM, H5E_CANTINIT, FAIL,
                               "unable to read symbolic link value");
            }
            statbuf->linklen = HDstrlen(s)+1; /*count the null terminator*/
            statbuf->objno[0] = statbuf->objno[1] = 0;
            statbuf->nlink = 0;
            statbuf->type = H5G_LINK;
            statbuf->mtime = 0;
            
        } else {
            /* Some other type of object */
            statbuf->objno[0] = (unsigned long)(obj_ent.header);
#if SIZEOF_UINT64_T>SIZEOF_LONG
            statbuf->objno[1] = (unsigned long)(obj_ent.header >>
                                                8*sizeof(long));
#else
            statbuf->objno[1] = 0;
#endif
            statbuf->nlink = H5O_link (&obj_ent, 0);
            statbuf->type = H5G_LINK;
            if (NULL==H5O_read(&obj_ent, H5O_MTIME, 0, &(statbuf->mtime))) {
                H5E_clear();
                statbuf->mtime = 0;
            }
            statbuf->type = H5G_get_type(&obj_ent);
            H5E_clear(); /*clear errors resulting from checking type*/
        }
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_linkval
 *
 * Purpose:     Returns the value of a symbolic link.
 *
 * Return:      Success:        Non-negative, with at most SIZE bytes of the
 *                              link value copied into the BUF buffer.  If the
 *                              link value is larger than SIZE characters
 *                              counting the null terminator then the BUF
 *                              result will not be null terminated.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, April 13, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_linkval (H5G_entry_t *loc, const char *name, size_t size, char *buf/*out*/)
{
    const char          *s = NULL;
    H5G_entry_t         grp_ent, obj_ent;
    H5O_stab_t          stab_mesg;
    
    FUNC_ENTER (H5G_linkval, FAIL);

    /*
     * Get the symbol table entry for the link head and the symbol table
     * entry for the group in which the link head appears.
     */
    if (H5G_namei (loc, name, NULL, &grp_ent/*out*/, &obj_ent/*out*/,
                   H5G_TARGET_SLINK, NULL)<0) {
        HRETURN_ERROR (H5E_SYM, H5E_NOTFOUND, FAIL,
                       "symbolic link was not found");
    }
    if (H5G_CACHED_SLINK!=obj_ent.type) {
        HRETURN_ERROR (H5E_SYM, H5E_NOTFOUND, FAIL,
                       "object is not a symbolic link");
    }

    /*
     * Get the address of the local heap for the link value and a pointer
     * into that local heap.
     */
    if (NULL==H5O_read (&grp_ent, H5O_STAB, 0, &stab_mesg)) {
        HRETURN_ERROR (H5E_SYM, H5E_CANTINIT, FAIL,
                       "unable to determine local heap address");
    }
    if (NULL==(s=H5HL_peek (grp_ent.file, stab_mesg.heap_addr,
                            obj_ent.cache.slink.lval_offset))) {
        HRETURN_ERROR (H5E_SYM, H5E_CANTINIT, FAIL,
                       "unable to read symbolic link value");
    }
    
    /* Copy to output buffer */
    if (size>0 && buf) {
        HDstrncpy (buf, s, size);
    }

    FUNC_LEAVE (SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_set_comment
 *
 * Purpose:     (Re)sets the comment for an object.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Monday, July 20, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_set_comment(H5G_entry_t *loc, const char *name, const char *buf)
{
    H5G_entry_t obj_ent;
    H5O_name_t  comment;
    
    FUNC_ENTER(H5G_set_comment, FAIL);

    /* Get the symbol table entry for the object */
    if (H5G_namei(loc, name, NULL, NULL, &obj_ent/*out*/, H5G_TARGET_NORMAL,
                  NULL)<0) {
        HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object not found");
    }

    /* Remove the previous comment message if any */
    if (H5O_remove(&obj_ent, H5O_NAME, 0)<0) H5E_clear();

    /* Add the new message */
    if (buf && *buf) {
        comment.s = H5MM_xstrdup(buf);
        if (H5O_modify(&obj_ent, H5O_NAME, H5O_NEW_MESG, 0, &comment)<0) {
            HRETURN_ERROR(H5E_OHDR, H5E_CANTINIT, FAIL,
                          "unable to set comment object header message");
        }
        H5O_reset(H5O_NAME, &comment);
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_get_comment
 *
 * Purpose:     Get the comment value for an object.
 *
 * Return:      Success:        Number of bytes in the comment including the
 *                              null terminator.  Zero if the object has no
 *                              comment.
 *
 *              Failure:        Negative
 *
 * Programmer:  Robb Matzke
 *              Monday, July 20, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
int
H5G_get_comment(H5G_entry_t *loc, const char *name, size_t bufsize, char *buf)
{
    H5O_name_t  comment;
    H5G_entry_t obj_ent;
    int retval = FAIL;
    
    FUNC_ENTER(H5G_get_comment, FAIL);

    /* Get the symbol table entry for the object */
    if (H5G_namei(loc, name, NULL, NULL, &obj_ent/*out*/, H5G_TARGET_NORMAL,
                  NULL)<0) {
        HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object not found");
    }

    /* Get the message */
    comment.s = NULL;
    if (NULL==H5O_read(&obj_ent, H5O_NAME, 0, &comment)) {
        if (buf && bufsize>0) buf[0] = '\0';
        retval = 0;
    } else {
        HDstrncpy(buf, comment.s, bufsize);
        retval = (int)HDstrlen(comment.s);
        H5O_reset(H5O_NAME, &comment);
    }

    FUNC_LEAVE(retval);
}
    

/*-------------------------------------------------------------------------
 * Function:    H5G_unlink
 *
 * Purpose:     Unlink a name from a group.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Thursday, September 17, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_unlink(H5G_entry_t *loc, const char *name)
{
    H5G_entry_t         grp_ent, obj_ent;
    size_t              len;
    const char          *base=NULL;
    
    FUNC_ENTER(H5G_unlink, FAIL);
    assert(loc);
    assert(name && *name);

    /* Get the entry for the group that contains the object to be unlinked */
    if (H5G_namei(loc, name, NULL, &grp_ent, &obj_ent,
                  H5G_TARGET_SLINK|H5G_TARGET_MOUNT, NULL)<0) {
        HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object not found");
    }
    if (!H5F_addr_defined(grp_ent.header)) {
        HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL,
                      "no containing group specified");
    }
    if (NULL==(base=H5G_basename(name, &len)) || '/'==*base) {
        HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL,
                      "problems obtaining object base name");
    }
    
    /* Remove the name from the symbol table */
    if (H5G_stab_remove(&grp_ent, base)<0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL,
                      "unable to unlink name from symbol table");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_move
 *
 * Purpose:     Atomically rename an object.
 *
 * Return:      Non-negative on success/Negative on failure
 *
 * Programmer:  Robb Matzke
 *              Friday, September 25, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
herr_t
H5G_move(H5G_entry_t *loc, const char *src_name, const char *dst_name)
{
    H5G_stat_t          sb;
    char                *linkval=NULL;
    size_t              lv_size=32;
    
    FUNC_ENTER(H5G_move, FAIL);
    assert(loc);
    assert(src_name && *src_name);
    assert(dst_name && *dst_name);

    if (H5G_get_objinfo(loc, src_name, FALSE, &sb)<0) {
        HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, FAIL, "object not found");
    }
    if (H5G_LINK==sb.type) {
        /*
         * When renaming a symbolic link we rename the link but don't change
         * the value of the link.
         */
        do {
            if (NULL==(linkval=H5MM_realloc(linkval, 2*lv_size))) {
                HRETURN_ERROR(H5E_RESOURCE, H5E_NOSPACE, FAIL,
                              "unable to allocate space for symbolic link "
                              "value");
            }
            linkval[lv_size-1] = '\0';
            if (H5G_linkval(loc, src_name, lv_size, linkval)<0) {
                HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL,
                              "unable to read symbolic link value");
            }
        } while (linkval[lv_size-1]);
        if (H5G_link(loc, H5G_LINK_SOFT, linkval, dst_name,
                     H5G_TARGET_NORMAL)<0) {
            HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL,
                          "unable to rename symbolic link");
        }
        H5MM_xfree(linkval);
        
    } else {
        /*
         * Rename the object.
         */
        if (H5G_link(loc, H5G_LINK_HARD, src_name, dst_name,
                     H5G_TARGET_MOUNT)<0) {
            HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL,
                          "unable to register new name for object");
        }
    }

    /* Remove the old name */
    if (H5G_unlink(loc, src_name)<0) {
        HRETURN_ERROR(H5E_SYM, H5E_CANTINIT, FAIL,
                      "unable to deregister old object name");
    }

    FUNC_LEAVE(SUCCEED);
}


/*-------------------------------------------------------------------------
 * Function:    H5G_insertion_file
 *
 * Purpose:     Given a location and name that specifies a not-yet-existing
 *              object return the file into which the object is about to be
 *              inserted.
 *
 * Return:      Success:        File pointer
 *
 *              Failure:        NULL
 *
 * Programmer:  Robb Matzke
 *              Wednesday, October 14, 1998
 *
 * Modifications:
 *
 *-------------------------------------------------------------------------
 */
H5F_t *
H5G_insertion_file(H5G_entry_t *loc, const char *name)
{
    const char  *rest;
    H5G_entry_t grp_ent;
    size_t      size;

    FUNC_ENTER(H5G_insertion_file, NULL);
    assert(loc);
    assert(name && *name);

    /*
     * Look up the name to get the containing group and to make sure the name
     * doesn't already exist.
     */
    if (H5G_namei(loc, name, &rest, &grp_ent, NULL, H5G_TARGET_NORMAL,
                  NULL)>=0) {
        HRETURN_ERROR(H5E_SYM, H5E_EXISTS, NULL, "name already exists");
    }
    H5E_clear();

    /* Make sure only the last component wasn't resolved */
    rest = H5G_component(rest, &size);
    assert(*rest && size>0);
    rest = H5G_component(rest+size, NULL);
    if (*rest) {
        HRETURN_ERROR(H5E_SYM, H5E_NOTFOUND, NULL,
                      "insertion point not found");
    }

    FUNC_LEAVE(grp_ent.file);
}
