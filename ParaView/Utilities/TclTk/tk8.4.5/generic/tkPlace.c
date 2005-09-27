/* 
 * tkPlace.c --
 *
 *	This file contains code to implement a simple geometry manager
 *	for Tk based on absolute placement or "rubber-sheet" placement.
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkPort.h"
#include "tkInt.h"


/*
 * Border modes for relative placement:
 *
 * BM_INSIDE:		relative distances computed using area inside
 *			all borders of master window.
 * BM_OUTSIDE:		relative distances computed using outside area
 *			that includes all borders of master.
 * BM_IGNORE:		border issues are ignored:  place relative to
 *			master's actual window size.
 */

static char *borderModeStrings[] = {
    "inside", "outside", "ignore", (char *) NULL
};

typedef enum {BM_INSIDE, BM_OUTSIDE, BM_IGNORE} BorderMode;

/*
 * For each window whose geometry is managed by the placer there is
 * a structure of the following type:
 */

typedef struct Slave {
    Tk_Window tkwin;		/* Tk's token for window. */
    Tk_Window inTkwin;		/* Token for the -in window. */
    struct Master *masterPtr;	/* Pointer to information for window
				 * relative to which tkwin is placed.
				 * This isn't necessarily the logical
				 * parent of tkwin.  NULL means the
				 * master was deleted or never assigned. */
    struct Slave *nextPtr;	/* Next in list of windows placed relative
				 * to same master (NULL for end of list). */
    /*
     * Geometry information for window;  where there are both relative
     * and absolute values for the same attribute (e.g. x and relX) only
     * one of them is actually used, depending on flags.
     */

    int x, y;			/* X and Y pixel coordinates for tkwin. */
    Tcl_Obj *xPtr, *yPtr;	/* Tcl_Obj rep's of x, y coords, to keep
				 * pixel spec. information */
    double relX, relY;		/* X and Y coordinates relative to size of
				 * master. */
    int width, height;		/* Absolute dimensions for tkwin. */
    Tcl_Obj *widthPtr;		/* Tcl_Obj rep of width, to keep pixel spec */
    Tcl_Obj *heightPtr;		/* Tcl_Obj rep of height, to keep pixel spec */
    double relWidth, relHeight;	/* Dimensions for tkwin relative to size of
				 * master. */
    Tcl_Obj *relWidthPtr;
    Tcl_Obj *relHeightPtr;
    Tk_Anchor anchor;		/* Which point on tkwin is placed at the
				 * given position. */
    BorderMode borderMode;	/* How to treat borders of master window. */
    int flags;			/* Various flags;  see below for bit
				 * definitions. */
} Slave;

/*
 * Type masks for options:
 */
#define IN_MASK		1

static Tk_OptionSpec optionSpecs[] = {
    {TK_OPTION_ANCHOR, "-anchor", NULL, NULL, "nw", -1,
	 Tk_Offset(Slave, anchor), 0, 0, 0},
    {TK_OPTION_STRING_TABLE, "-bordermode", NULL, NULL, "inside", -1,
	 Tk_Offset(Slave, borderMode), 0, (ClientData) borderModeStrings, 0},
    {TK_OPTION_PIXELS, "-height", NULL, NULL, "", Tk_Offset(Slave, heightPtr),
	 Tk_Offset(Slave, height), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_WINDOW, "-in", NULL, NULL, "", -1, Tk_Offset(Slave, inTkwin),
	 0, 0, IN_MASK},
    {TK_OPTION_DOUBLE, "-relheight", NULL, NULL, "",
	 Tk_Offset(Slave, relHeightPtr), Tk_Offset(Slave, relHeight),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_DOUBLE, "-relwidth", NULL, NULL, "",
	 Tk_Offset(Slave, relWidthPtr), Tk_Offset(Slave, relWidth),
	 TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_DOUBLE, "-relx", NULL, NULL, "0", -1,
	 Tk_Offset(Slave, relX), 0, 0, 0},
    {TK_OPTION_DOUBLE, "-rely", NULL, NULL, "0", -1,
	 Tk_Offset(Slave, relY), 0, 0, 0},
    {TK_OPTION_PIXELS, "-width", NULL, NULL, "", Tk_Offset(Slave, widthPtr),
	 Tk_Offset(Slave, width), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-x", NULL, NULL, "0", Tk_Offset(Slave, xPtr),
	 Tk_Offset(Slave, x), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_PIXELS, "-y", NULL, NULL, "0", Tk_Offset(Slave, yPtr),
	 Tk_Offset(Slave, y), TK_OPTION_NULL_OK, 0, 0},
    {TK_OPTION_END, (char *) NULL, (char *) NULL, (char *) NULL,
	 (char *) NULL, 0, -1, 0, 0, 0}
};
	
/*
 * Flag definitions for Slave structures:
 *
 * CHILD_WIDTH -		1 means -width was specified;
 * CHILD_REL_WIDTH -		1 means -relwidth was specified.
 * CHILD_HEIGHT -		1 means -height was specified;
 * CHILD_REL_HEIGHT -		1 means -relheight was specified.
 */

#define CHILD_WIDTH		1
#define CHILD_REL_WIDTH		2
#define CHILD_HEIGHT		4
#define CHILD_REL_HEIGHT	8

/*
 * For each master window that has a slave managed by the placer there
 * is a structure of the following form:
 */

typedef struct Master {
    Tk_Window tkwin;		/* Tk's token for master window. */
    struct Slave *slavePtr;	/* First in linked list of slaves
				 * placed relative to this master. */
    int flags;			/* See below for bit definitions. */
} Master;

/*
 * Flag definitions for masters:
 *
 * PARENT_RECONFIG_PENDING -	1 means that a call to RecomputePlacement
 *				is already pending via a Do_When_Idle handler.
 */

#define PARENT_RECONFIG_PENDING	1

/*
 * The following structure is the official type record for the
 * placer:
 */

static void		PlaceRequestProc _ANSI_ARGS_((ClientData clientData,
			    Tk_Window tkwin));
static void		PlaceLostSlaveProc _ANSI_ARGS_((ClientData clientData,
			    Tk_Window tkwin));

static Tk_GeomMgr placerType = {
    "place",				/* name */
    PlaceRequestProc,			/* requestProc */
    PlaceLostSlaveProc,			/* lostSlaveProc */
};

/*
 * Forward declarations for procedures defined later in this file:
 */

static void		SlaveStructureProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static int		ConfigureSlave _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin, Tk_OptionTable table,
			    int objc, Tcl_Obj *CONST objv[]));
static int		PlaceInfoCommand _ANSI_ARGS_((Tcl_Interp *interp,
			    Tk_Window tkwin));
static Slave *		CreateSlave _ANSI_ARGS_((Tk_Window tkwin));
static Slave *		FindSlave _ANSI_ARGS_((Tk_Window tkwin));
static Master *		CreateMaster _ANSI_ARGS_((Tk_Window tkwin));
static Master *		FindMaster _ANSI_ARGS_((Tk_Window tkwin));
static void		MasterStructureProc _ANSI_ARGS_((ClientData clientData,
			    XEvent *eventPtr));
static void		RecomputePlacement _ANSI_ARGS_((ClientData clientData));
static void		UnlinkSlave _ANSI_ARGS_((Slave *slavePtr));

/*
 *--------------------------------------------------------------
 *
 * Tk_PlaceObjCmd --
 *
 *	This procedure is invoked to process the "place" Tcl
 *	commands.  See the user documentation for details on
 *	what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *--------------------------------------------------------------
 */

int
Tk_PlaceObjCmd(clientData, interp, objc, objv)
    ClientData clientData;	/* NULL. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    Tk_Window tkwin;
    Slave *slavePtr;
    char *string;
    TkDisplay *dispPtr;
    Tk_OptionTable optionTable;
    static CONST char *optionStrings[] = {
	"configure", "forget", "info", "slaves", (char *) NULL
    };
    enum options { PLACE_CONFIGURE, PLACE_FORGET, PLACE_INFO, PLACE_SLAVES };
    int index;
    
    
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 1, objv, "option|pathName args");
	return TCL_ERROR;
    }

    /*
     * Create the option table for this widget class.  If it has already
     * been created, the cached pointer will be returned.
     */

    optionTable = Tk_CreateOptionTable(interp, optionSpecs);

    /*
     * Handle special shortcut where window name is first argument.
     */

    string = Tcl_GetString(objv[1]);
    if (string[0] == '.') {
	tkwin = Tk_NameToWindow(interp, string,	Tk_MainWindow(interp));
	if (tkwin == NULL) {
	    return TCL_ERROR;
	}

	/*
	 * Initialize, if that hasn't been done yet.
	 */

	dispPtr = ((TkWindow *) tkwin)->dispPtr;
	if (!dispPtr->placeInit) {
	    Tcl_InitHashTable(&dispPtr->masterTable, TCL_ONE_WORD_KEYS);
	    Tcl_InitHashTable(&dispPtr->slaveTable, TCL_ONE_WORD_KEYS);
	    dispPtr->placeInit = 1;
	}

	return ConfigureSlave(interp, tkwin, optionTable, objc-2, objv+2);
    }

    /*
     * Handle more general case of option followed by window name followed
     * by possible additional arguments.
     */

    tkwin = Tk_NameToWindow(interp, Tcl_GetString(objv[2]),
	    Tk_MainWindow(interp));
    if (tkwin == NULL) {
	return TCL_ERROR;
    }

    /*
     * Initialize, if that hasn't been done yet.
     */

    dispPtr = ((TkWindow *) tkwin)->dispPtr;
    if (!dispPtr->placeInit) {
	Tcl_InitHashTable(&dispPtr->masterTable, TCL_ONE_WORD_KEYS);
	Tcl_InitHashTable(&dispPtr->slaveTable, TCL_ONE_WORD_KEYS);
	dispPtr->placeInit = 1;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], optionStrings, "option", 0,
	    &index) != TCL_OK) {
	return TCL_ERROR;
    }

    switch ((enum options) index) {
	case PLACE_CONFIGURE: {
	    Tcl_Obj *objPtr;
	    if (objc == 3 || objc == 4) {
		slavePtr = FindSlave(tkwin);
		if (slavePtr == NULL) {
		    return TCL_OK;
		}
		objPtr = Tk_GetOptionInfo(interp, (char *) slavePtr,
			optionTable,
			(objc == 4) ? objv[3] : (Tcl_Obj *) NULL, tkwin);
		if (objPtr == NULL) {
		    return TCL_ERROR;
		} else {
		    Tcl_SetObjResult(interp, objPtr);
		    return TCL_OK;
		}
	    } else {
		return ConfigureSlave(interp, tkwin, optionTable, objc-3,
			objv+3);
	    }
	}
	
	case PLACE_FORGET: {
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "pathName");
		return TCL_ERROR;
	    }
	    slavePtr = FindSlave(tkwin);
	    if (slavePtr == NULL) {
		return TCL_OK;
	    }
	    if ((slavePtr->masterPtr != NULL) &&
		    (slavePtr->masterPtr->tkwin !=
			    Tk_Parent(slavePtr->tkwin))) {
		Tk_UnmaintainGeometry(slavePtr->tkwin,
			slavePtr->masterPtr->tkwin);
	    }
	    UnlinkSlave(slavePtr);
	    Tcl_DeleteHashEntry(Tcl_FindHashEntry(&dispPtr->slaveTable,
		    (char *) tkwin));
	    Tk_DeleteEventHandler(tkwin, StructureNotifyMask,
		    SlaveStructureProc,	(ClientData) slavePtr);
	    Tk_ManageGeometry(tkwin, (Tk_GeomMgr *) NULL, (ClientData) NULL);
	    Tk_UnmapWindow(tkwin);
	    ckfree((char *) slavePtr);
	    break;
	}
	
	case PLACE_INFO: {
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "pathName");
		return TCL_ERROR;
	    }
	    return PlaceInfoCommand(interp, tkwin);
	}

	case PLACE_SLAVES: {
	    Master *masterPtr;
	    Tcl_Obj *listPtr;
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "pathName");
		return TCL_ERROR;
	    }
	    masterPtr = FindMaster(tkwin);
	    if (masterPtr != NULL) {
		listPtr = Tcl_NewObj();
		for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
		     slavePtr = slavePtr->nextPtr) {
		    Tcl_ListObjAppendElement(interp, listPtr,
			    Tcl_NewStringObj(Tk_PathName(slavePtr->tkwin),-1));
		}
		Tcl_SetObjResult(interp, listPtr);
	    }
	    break;
	}
    }

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateSlave --
 *
 *	Given a Tk_Window token, find the Slave structure corresponding
 *	to that token, creating a new one if necessary.
 *
 * Results:
 *	Pointer to the Slave structure.
 *
 * Side effects:
 *	A new Slave structure may be created.
 *
 *----------------------------------------------------------------------
 */

static Slave *
CreateSlave(tkwin)
    Tk_Window tkwin;		/* Token for desired slave. */
{
    Tcl_HashEntry *hPtr;
    register Slave *slavePtr;
    int new;
    TkDisplay * dispPtr = ((TkWindow *) tkwin)->dispPtr;

    hPtr = Tcl_CreateHashEntry(&dispPtr->slaveTable, (char *) tkwin, &new);
    if (new) {
	slavePtr = (Slave *) ckalloc(sizeof(Slave));
	memset(slavePtr, 0, sizeof(Slave));
	slavePtr->tkwin		= tkwin;
	slavePtr->inTkwin	= None;
	slavePtr->anchor	= TK_ANCHOR_NW;
	slavePtr->borderMode	= BM_INSIDE;
	Tcl_SetHashValue(hPtr, slavePtr);
	Tk_CreateEventHandler(tkwin, StructureNotifyMask, SlaveStructureProc,
		(ClientData) slavePtr);
	Tk_ManageGeometry(tkwin, &placerType, (ClientData) slavePtr);
    } else {
	slavePtr = (Slave *) Tcl_GetHashValue(hPtr);
    }
    return slavePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FindSlave --
 *
 *	Given a Tk_Window token, find the Slave structure corresponding
 *	to that token.  This is purely a lookup function; it will not
 *	create a record if one does not yet exist.
 *
 * Results:
 *	Pointer to Slave structure; NULL if none exists.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Slave *
FindSlave(tkwin)
    Tk_Window tkwin;		/* Token for desired slave. */
{
    Tcl_HashEntry *hPtr;
    register Slave *slavePtr;
    TkDisplay * dispPtr = ((TkWindow *) tkwin)->dispPtr;

    hPtr = Tcl_FindHashEntry(&dispPtr->slaveTable, (char *) tkwin);
    if (hPtr == NULL) {
	return NULL;
    }
    slavePtr = (Slave *) Tcl_GetHashValue(hPtr);
    return slavePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * UnlinkSlave --
 *
 *	This procedure removes a slave window from the chain of slaves
 *	in its master.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The slave list of slavePtr's master changes.
 *
 *----------------------------------------------------------------------
 */

static void
UnlinkSlave(slavePtr)
    Slave *slavePtr;		/* Slave structure to be unlinked. */
{
    register Master *masterPtr;
    register Slave *prevPtr;

    masterPtr = slavePtr->masterPtr;
    if (masterPtr == NULL) {
	return;
    }
    if (masterPtr->slavePtr == slavePtr) {
	masterPtr->slavePtr = slavePtr->nextPtr;
    } else {
	for (prevPtr = masterPtr->slavePtr; ;
		prevPtr = prevPtr->nextPtr) {
	    if (prevPtr == NULL) {
		panic("UnlinkSlave couldn't find slave to unlink");
	    }
	    if (prevPtr->nextPtr == slavePtr) {
		prevPtr->nextPtr = slavePtr->nextPtr;
		break;
	    }
	}
    }
    slavePtr->masterPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * CreateMaster --
 *
 *	Given a Tk_Window token, find the Master structure corresponding
 *	to that token, creating a new one if necessary.
 *
 * Results:
 *	Pointer to the Master structure.
 *
 * Side effects:
 *	A new Master structure may be created.
 *
 *----------------------------------------------------------------------
 */

static Master *
CreateMaster(tkwin)
    Tk_Window tkwin;		/* Token for desired master. */
{
    Tcl_HashEntry *hPtr;
    register Master *masterPtr;
    int new;
    TkDisplay * dispPtr = ((TkWindow *) tkwin)->dispPtr;

    hPtr = Tcl_CreateHashEntry(&dispPtr->masterTable, (char *) tkwin, &new);
    if (new) {
	masterPtr = (Master *) ckalloc(sizeof(Master));
	masterPtr->tkwin	= tkwin;
	masterPtr->slavePtr	= NULL;
	masterPtr->flags	= 0;
	Tcl_SetHashValue(hPtr, masterPtr);
	Tk_CreateEventHandler(masterPtr->tkwin, StructureNotifyMask,
		MasterStructureProc, (ClientData) masterPtr);
    } else {
	masterPtr = (Master *) Tcl_GetHashValue(hPtr);
    }
    return masterPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FindMaster --
 *
 *	Given a Tk_Window token, find the Master structure corresponding
 *	to that token.  This is simply a lookup procedure; a new record
 *	will not be created if one does not already exist.
 *
 * Results:
 *	Pointer to the Master structure; NULL if one does not exist for
 *	the given Tk_Window token.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Master *
FindMaster(tkwin)
    Tk_Window tkwin;		/* Token for desired master. */
{
    Tcl_HashEntry *hPtr;
    register Master *masterPtr;
    TkDisplay * dispPtr = ((TkWindow *) tkwin)->dispPtr;

    hPtr = Tcl_FindHashEntry(&dispPtr->masterTable, (char *) tkwin);
    if (hPtr == NULL) {
	return NULL;
    }
    masterPtr = (Master *) Tcl_GetHashValue(hPtr);
    return masterPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureSlave --
 *
 *	This procedure is called to process an argv/argc list to
 *	reconfigure the placement of a window.
 *
 * Results:
 *	A standard Tcl result.  If an error occurs then a message is
 *	left in the interp's result.
 *
 * Side effects:
 *	Information in slavePtr may change, and slavePtr's master is
 *	scheduled for reconfiguration.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureSlave(interp, tkwin, table, objc, objv)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tk_Window tkwin;		/* Token for the window to manipulate. */
    Tk_OptionTable table;	/* Token for option table. */
    int objc;			/* Number of config arguments. */
    Tcl_Obj *CONST objv[];	/* Object values for arguments. */
{
    register Master *masterPtr;
    Tk_SavedOptions savedOptions;
    int mask;
    int result = TCL_OK;
    Slave *slavePtr;
    
    if (Tk_TopWinHierarchy(tkwin)) {
	Tcl_AppendResult(interp, "can't use placer on top-level window \"",
		Tk_PathName(tkwin), "\"; use wm command instead",
		(char *) NULL);
	return TCL_ERROR;
    }

    slavePtr = CreateSlave(tkwin);
    
    if (Tk_SetOptions(interp, (char *)slavePtr, table, objc, objv,
	    slavePtr->tkwin, &savedOptions, &mask) != TCL_OK) {
	Tk_RestoreSavedOptions(&savedOptions);
	result = TCL_ERROR;
	goto done;
    }

    if (mask & IN_MASK) {
	/* -in changed */
	Tk_Window tkwin;
	Tk_Window ancestor;
	
	tkwin = slavePtr->inTkwin;
	
	/*
	 * Make sure that the new master is either the logical parent
	 * of the slave or a descendant of that window, and that the
	 * master and slave aren't the same.
	 */
	
	for (ancestor = tkwin; ; ancestor = Tk_Parent(ancestor)) {
	    if (ancestor == Tk_Parent(slavePtr->tkwin)) {
		break;
	    }
	    if (Tk_TopWinHierarchy(ancestor)) {
		Tcl_AppendResult(interp, "can't place ",
			Tk_PathName(slavePtr->tkwin), " relative to ",
			Tk_PathName(tkwin), (char *) NULL);
		result = TCL_ERROR;
		Tk_RestoreSavedOptions(&savedOptions);
		goto done;
	    }
	}
	if (slavePtr->tkwin == tkwin) {
	    Tcl_AppendResult(interp, "can't place ",
		    Tk_PathName(slavePtr->tkwin), " relative to itself",
		    (char *) NULL);
	    result = TCL_ERROR;
	    Tk_RestoreSavedOptions(&savedOptions);
	    goto done;
	}
	if ((slavePtr->masterPtr != NULL)
		&& (slavePtr->masterPtr->tkwin == tkwin)) {
	    /*
	     * Re-using same old master.  Nothing to do.
	     */
	} else {
	    if ((slavePtr->masterPtr != NULL)
		    && (slavePtr->masterPtr->tkwin
			    != Tk_Parent(slavePtr->tkwin))) {
		Tk_UnmaintainGeometry(slavePtr->tkwin,
			slavePtr->masterPtr->tkwin);
	    }
	    UnlinkSlave(slavePtr);
	    slavePtr->masterPtr = CreateMaster(tkwin);
	    slavePtr->nextPtr = slavePtr->masterPtr->slavePtr;
	    slavePtr->masterPtr->slavePtr = slavePtr;
	}
    }

    /*
     * Set slave flags.  First clear the field, then add bits as needed.
     */

    slavePtr->flags = 0;
    if (slavePtr->heightPtr) {
	slavePtr->flags |= CHILD_HEIGHT;
    }

    if (slavePtr->relHeightPtr) {
	slavePtr->flags |= CHILD_REL_HEIGHT;
    }

    if (slavePtr->relWidthPtr) {
	slavePtr->flags |= CHILD_REL_WIDTH;
    }

    if (slavePtr->widthPtr) {
	slavePtr->flags |= CHILD_WIDTH;
    }

    /*
     * If there's no master specified for this slave, use its Tk_Parent.
     * Then arrange for a placement recalculation in the master.
     */

    Tk_FreeSavedOptions(&savedOptions);
    done:
    masterPtr = slavePtr->masterPtr;
    if (masterPtr == NULL) {
	masterPtr = CreateMaster(Tk_Parent(slavePtr->tkwin));
	slavePtr->masterPtr = masterPtr;
	slavePtr->nextPtr = masterPtr->slavePtr;
	masterPtr->slavePtr = slavePtr;
    }
    slavePtr->inTkwin = masterPtr->tkwin;
    if (!(masterPtr->flags & PARENT_RECONFIG_PENDING)) {
	masterPtr->flags |= PARENT_RECONFIG_PENDING;
	Tcl_DoWhenIdle(RecomputePlacement, (ClientData) masterPtr);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * PlaceInfoCommand --
 *
 *	Implementation of the [place info] subcommand.  See the user
 *	documentation for information on what it does.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	If the given tkwin is managed by the placer, this function will
 *	put information about that placement in the interp's result.
 *
 *----------------------------------------------------------------------
 */

static int
PlaceInfoCommand(interp, tkwin)
    Tcl_Interp *interp;		/* Interp into which to place result. */
    Tk_Window tkwin;		/* Token for the window to get info on. */
{
    char buffer[32 + TCL_INTEGER_SPACE];
    Slave *slavePtr;
    
    slavePtr = FindSlave(tkwin);
    if (slavePtr == NULL) {
	return TCL_OK;
    }
    if (slavePtr->masterPtr != NULL) {
	Tcl_AppendElement(interp, "-in");
	Tcl_AppendElement(interp, Tk_PathName(slavePtr->masterPtr->tkwin));
    }
    sprintf(buffer, " -x %d", slavePtr->x);
    Tcl_AppendResult(interp, buffer, (char *) NULL);
    sprintf(buffer, " -relx %.4g", slavePtr->relX);
    Tcl_AppendResult(interp, buffer, (char *) NULL);
    sprintf(buffer, " -y %d", slavePtr->y);
    Tcl_AppendResult(interp, buffer, (char *) NULL);
    sprintf(buffer, " -rely %.4g", slavePtr->relY);
    Tcl_AppendResult(interp, buffer, (char *) NULL);
    if (slavePtr->flags & CHILD_WIDTH) {
	sprintf(buffer, " -width %d", slavePtr->width);
	Tcl_AppendResult(interp, buffer, (char *) NULL);
    } else {
	Tcl_AppendResult(interp, " -width {}", (char *) NULL);
    }
    if (slavePtr->flags & CHILD_REL_WIDTH) {
	sprintf(buffer, " -relwidth %.4g", slavePtr->relWidth);
	Tcl_AppendResult(interp, buffer, (char *) NULL);
    } else {
	Tcl_AppendResult(interp, " -relwidth {}", (char *) NULL);
    }
    if (slavePtr->flags & CHILD_HEIGHT) {
	sprintf(buffer, " -height %d", slavePtr->height);
	Tcl_AppendResult(interp, buffer, (char *) NULL);
    } else {
	Tcl_AppendResult(interp, " -height {}", (char *) NULL);
    }
    if (slavePtr->flags & CHILD_REL_HEIGHT) {
	sprintf(buffer, " -relheight %.4g", slavePtr->relHeight);
	Tcl_AppendResult(interp, buffer, (char *) NULL);
    } else {
	Tcl_AppendResult(interp, " -relheight {}", (char *) NULL);
    }
    
    Tcl_AppendElement(interp, "-anchor");
    Tcl_AppendElement(interp, Tk_NameOfAnchor(slavePtr->anchor));
    Tcl_AppendElement(interp, "-bordermode");
    Tcl_AppendElement(interp, borderModeStrings[slavePtr->borderMode]);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * RecomputePlacement --
 *
 *	This procedure is called as a when-idle handler.  It recomputes
 *	the geometries of all the slaves of a given master.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Windows may change size or shape.
 *
 *----------------------------------------------------------------------
 */

static void
RecomputePlacement(clientData)
    ClientData clientData;	/* Pointer to Master record. */
{
    register Master *masterPtr = (Master *) clientData;
    register Slave *slavePtr;
    int x, y, width, height, tmp;
    int masterWidth, masterHeight, masterX, masterY;
    double x1, y1, x2, y2;

    masterPtr->flags &= ~PARENT_RECONFIG_PENDING;

    /*
     * Iterate over all the slaves for the master.  Each slave's
     * geometry can be computed independently of the other slaves.
     */

    for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
	    slavePtr = slavePtr->nextPtr) {
	/*
	 * Step 1: compute size and borderwidth of master, taking into
	 * account desired border mode.
	 */

	masterX = masterY = 0;
	masterWidth = Tk_Width(masterPtr->tkwin);
	masterHeight = Tk_Height(masterPtr->tkwin);
	if (slavePtr->borderMode == BM_INSIDE) {
	    masterX = Tk_InternalBorderLeft(masterPtr->tkwin);
	    masterY = Tk_InternalBorderTop(masterPtr->tkwin);
            masterWidth -= masterX + Tk_InternalBorderRight(masterPtr->tkwin);
            masterHeight -= masterY +
		    Tk_InternalBorderBottom(masterPtr->tkwin);
	} else if (slavePtr->borderMode == BM_OUTSIDE) {
	    masterX = masterY = -Tk_Changes(masterPtr->tkwin)->border_width;
            masterWidth -= 2 * masterX;
            masterHeight -= 2 * masterY;
	}

	/*
	 * Step 2:  compute size of slave (outside dimensions including
	 * border) and location of anchor point within master.
	 */

	x1 = slavePtr->x + masterX + (slavePtr->relX*masterWidth);
	x = (int) (x1 + ((x1 > 0) ? 0.5 : -0.5));
	y1 = slavePtr->y + masterY + (slavePtr->relY*masterHeight);
	y = (int) (y1 + ((y1 > 0) ? 0.5 : -0.5));
	if (slavePtr->flags & (CHILD_WIDTH|CHILD_REL_WIDTH)) {
	    width = 0;
	    if (slavePtr->flags & CHILD_WIDTH) {
		width += slavePtr->width;
	    }
	    if (slavePtr->flags & CHILD_REL_WIDTH) {
		/*
		 * The code below is a bit tricky.  In order to round
		 * correctly when both relX and relWidth are specified,
		 * compute the location of the right edge and round that,
		 * then compute width.  If we compute the width and round
		 * it, rounding errors in relX and relWidth accumulate.
		 */

		x2 = x1 + (slavePtr->relWidth*masterWidth);
		tmp = (int) (x2 + ((x2 > 0) ? 0.5 : -0.5));
		width += tmp - x;
	    }
	} else {
	    width = Tk_ReqWidth(slavePtr->tkwin)
		    + 2*Tk_Changes(slavePtr->tkwin)->border_width;
	}
	if (slavePtr->flags & (CHILD_HEIGHT|CHILD_REL_HEIGHT)) {
	    height = 0;
	    if (slavePtr->flags & CHILD_HEIGHT) {
		height += slavePtr->height;
	    }
	    if (slavePtr->flags & CHILD_REL_HEIGHT) {
		/* 
		 * See note above for rounding errors in width computation.
		 */

		y2 = y1 + (slavePtr->relHeight*masterHeight);
		tmp = (int) (y2 + ((y2 > 0) ? 0.5 : -0.5));
		height += tmp - y;
	    }
	} else {
	    height = Tk_ReqHeight(slavePtr->tkwin)
		    + 2*Tk_Changes(slavePtr->tkwin)->border_width;
	}

	/*
	 * Step 3: adjust the x and y positions so that the desired
	 * anchor point on the slave appears at that position.  Also
	 * adjust for the border mode and master's border.
	 */

	switch (slavePtr->anchor) {
	    case TK_ANCHOR_N:
		x -= width/2;
		break;
	    case TK_ANCHOR_NE:
		x -= width;
		break;
	    case TK_ANCHOR_E:
		x -= width;
		y -= height/2;
		break;
	    case TK_ANCHOR_SE:
		x -= width;
		y -= height;
		break;
	    case TK_ANCHOR_S:
		x -= width/2;
		y -= height;
		break;
	    case TK_ANCHOR_SW:
		y -= height;
		break;
	    case TK_ANCHOR_W:
		y -= height/2;
		break;
	    case TK_ANCHOR_NW:
		break;
	    case TK_ANCHOR_CENTER:
		x -= width/2;
		y -= height/2;
		break;
	}

	/*
	 * Step 4: adjust width and height again to reflect inside dimensions
	 * of window rather than outside.  Also make sure that the width and
	 * height aren't zero.
	 */

	width -= 2*Tk_Changes(slavePtr->tkwin)->border_width;
	height -= 2*Tk_Changes(slavePtr->tkwin)->border_width;
	if (width <= 0) {
	    width = 1;
	}
	if (height <= 0) {
	    height = 1;
	}

	/*
	 * Step 5: reconfigure the window and map it if needed.  If the
	 * slave is a child of the master, we do this ourselves.  If the
	 * slave isn't a child of the master, let Tk_MaintainGeometry do
	 * the work (it will re-adjust things as relevant windows map,
	 * unmap, and move).
	 */

	if (masterPtr->tkwin == Tk_Parent(slavePtr->tkwin)) {
	    if ((x != Tk_X(slavePtr->tkwin))
		    || (y != Tk_Y(slavePtr->tkwin))
		    || (width != Tk_Width(slavePtr->tkwin))
		    || (height != Tk_Height(slavePtr->tkwin))) {
		Tk_MoveResizeWindow(slavePtr->tkwin, x, y, width, height);
	    }

	    /*
	     * Don't map the slave unless the master is mapped: the slave
	     * will get mapped later, when the master is mapped.
	     */

	    if (Tk_IsMapped(masterPtr->tkwin)) {
		Tk_MapWindow(slavePtr->tkwin);
	    }
	} else {
	    if ((width <= 0) || (height <= 0)) {
		Tk_UnmaintainGeometry(slavePtr->tkwin, masterPtr->tkwin);
		Tk_UnmapWindow(slavePtr->tkwin);
	    } else {
		Tk_MaintainGeometry(slavePtr->tkwin, masterPtr->tkwin,
			x, y, width, height);
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MasterStructureProc --
 *
 *	This procedure is invoked by the Tk event handler when
 *	StructureNotify events occur for a master window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Structures get cleaned up if the window was deleted.  If the
 *	window was resized then slave geometries get recomputed.
 *
 *----------------------------------------------------------------------
 */

static void
MasterStructureProc(clientData, eventPtr)
    ClientData clientData;	/* Pointer to Master structure for window
				 * referred to by eventPtr. */
    XEvent *eventPtr;		/* Describes what just happened. */
{
    register Master *masterPtr = (Master *) clientData;
    register Slave *slavePtr, *nextPtr;
    TkDisplay *dispPtr = ((TkWindow *) masterPtr->tkwin)->dispPtr;

    if (eventPtr->type == ConfigureNotify) {
	if ((masterPtr->slavePtr != NULL)
		&& !(masterPtr->flags & PARENT_RECONFIG_PENDING)) {
	    masterPtr->flags |= PARENT_RECONFIG_PENDING;
	    Tcl_DoWhenIdle(RecomputePlacement, (ClientData) masterPtr);
	}
    } else if (eventPtr->type == DestroyNotify) {
	for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
		slavePtr = nextPtr) {
	    slavePtr->masterPtr = NULL;
	    nextPtr = slavePtr->nextPtr;
	    slavePtr->nextPtr = NULL;
	}
	Tcl_DeleteHashEntry(Tcl_FindHashEntry(&dispPtr->masterTable,
		(char *) masterPtr->tkwin));
	if (masterPtr->flags & PARENT_RECONFIG_PENDING) {
	    Tcl_CancelIdleCall(RecomputePlacement, (ClientData) masterPtr);
	}
	masterPtr->tkwin = NULL;
	ckfree((char *) masterPtr);
    } else if (eventPtr->type == MapNotify) {
	/*
	 * When a master gets mapped, must redo the geometry computation
	 * so that all of its slaves get remapped.
	 */

	if ((masterPtr->slavePtr != NULL)
		&& !(masterPtr->flags & PARENT_RECONFIG_PENDING)) {
	    masterPtr->flags |= PARENT_RECONFIG_PENDING;
	    Tcl_DoWhenIdle(RecomputePlacement, (ClientData) masterPtr);
	}
    } else if (eventPtr->type == UnmapNotify) {
	/*
	 * Unmap all of the slaves when the master gets unmapped,
	 * so that they don't keep redisplaying themselves.
	 */

	for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
		slavePtr = slavePtr->nextPtr) {
	    Tk_UnmapWindow(slavePtr->tkwin);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SlaveStructureProc --
 *
 *	This procedure is invoked by the Tk event handler when
 *	StructureNotify events occur for a slave window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Structures get cleaned up if the window was deleted.
 *
 *----------------------------------------------------------------------
 */

static void
SlaveStructureProc(clientData, eventPtr)
    ClientData clientData;	/* Pointer to Slave structure for window
				 * referred to by eventPtr. */
    XEvent *eventPtr;		/* Describes what just happened. */
{
    register Slave *slavePtr = (Slave *) clientData;
    TkDisplay * dispPtr = ((TkWindow *) slavePtr->tkwin)->dispPtr;

    if (eventPtr->type == DestroyNotify) {
	UnlinkSlave(slavePtr);
	Tcl_DeleteHashEntry(Tcl_FindHashEntry(&dispPtr->slaveTable,
		(char *) slavePtr->tkwin));
	ckfree((char *) slavePtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * PlaceRequestProc --
 *
 *	This procedure is invoked by Tk whenever a slave managed by us
 *	changes its requested geometry.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The window will get relayed out, if its requested size has
 *	anything to do with its actual size.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
static void
PlaceRequestProc(clientData, tkwin)
    ClientData clientData;		/* Pointer to our record for slave. */
    Tk_Window tkwin;			/* Window that changed its desired
					 * size. */
{
    Slave *slavePtr = (Slave *) clientData;
    Master *masterPtr;

    if (((slavePtr->flags & (CHILD_WIDTH|CHILD_REL_WIDTH)) != 0)
	    && ((slavePtr->flags & (CHILD_HEIGHT|CHILD_REL_HEIGHT)) != 0)) {
	return;
    }
    masterPtr = slavePtr->masterPtr;
    if (masterPtr == NULL) {
	return;
    }
    if (!(masterPtr->flags & PARENT_RECONFIG_PENDING)) {
	masterPtr->flags |= PARENT_RECONFIG_PENDING;
	Tcl_DoWhenIdle(RecomputePlacement, (ClientData) masterPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * PlaceLostSlaveProc --
 *
 *	This procedure is invoked by Tk whenever some other geometry
 *	claims control over a slave that used to be managed by us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Forgets all placer-related information about the slave.
 *
 *--------------------------------------------------------------
 */

	/* ARGSUSED */
static void
PlaceLostSlaveProc(clientData, tkwin)
    ClientData clientData;	/* Slave structure for slave window that
				 * was stolen away. */
    Tk_Window tkwin;		/* Tk's handle for the slave window. */
{
    register Slave *slavePtr = (Slave *) clientData;
    TkDisplay * dispPtr = ((TkWindow *) slavePtr->tkwin)->dispPtr;

    if (slavePtr->masterPtr->tkwin != Tk_Parent(slavePtr->tkwin)) {
	Tk_UnmaintainGeometry(slavePtr->tkwin, slavePtr->masterPtr->tkwin);
    }
    Tk_UnmapWindow(tkwin);
    UnlinkSlave(slavePtr);
    Tcl_DeleteHashEntry(Tcl_FindHashEntry(&dispPtr->slaveTable, 
            (char *) tkwin));
    Tk_DeleteEventHandler(tkwin, StructureNotifyMask, SlaveStructureProc,
	    (ClientData) slavePtr);
    ckfree((char *) slavePtr);
}
