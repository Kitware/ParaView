/*
 * tkGrid.c --
 *
 *	Grid based geometry manager.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"

/*
 * Convenience Macros
 */

#ifdef MAX
#   undef MAX
#endif
#define MAX(x,y)	((x) > (y) ? (x) : (y))
#ifdef MIN
#   undef MIN
#endif
#define MIN(x,y)	((x) > (y) ? (y) : (x))

#define COLUMN	(1)		/* working on column offsets */
#define ROW	(2)		/* working on row offsets */

#define CHECK_ONLY	(1)	/* check max slot constraint */
#define CHECK_SPACE	(2)	/* alloc more space, don't change max */

/*
 * Pre-allocate enough row and column slots for "typical" sized tables
 * this value should be chosen so by the time the extra malloc's are
 * required, the layout calculations overwehlm them. [A "slot" contains
 * information for either a row or column, depending upon the context.]
 */

#define TYPICAL_SIZE	25  /* (arbitrary guess) */
#define PREALLOC	10  /* extra slots to allocate */

/* 
 * Data structures are allocated dynamically to support arbitrary sized tables.
 * However, the space is proportional to the highest numbered slot with
 * some non-default property.  This limit is used to head off mistakes and
 * denial of service attacks by limiting the amount of storage required.
 */

#define MAX_ELEMENT	10000

/*
 * Special characters to support relative layouts.
 */

#define REL_SKIP	'x'	/* Skip this column. */
#define REL_HORIZ	'-'	/* Extend previous widget horizontally. */
#define REL_VERT	'^'	/* Extend widget from row above. */

/*
 *  Structure to hold information for grid masters.  A slot is either
 *  a row or column.
 */

typedef struct SlotInfo {
	int minSize;		/* The minimum size of this slot (in pixels).
				 * It is set via the rowconfigure or
				 * columnconfigure commands. */
	int weight;		/* The resize weight of this slot. (0) means
				 * this slot doesn't resize. Extra space in
				 * the layout is given distributed among slots
				 * inproportion to their weights. */
	int pad;		/* Extra padding, in pixels, required for
				 * this slot.  This amount is "added" to the
				 * largest slave in the slot. */
	int offset;		/* This is a cached value used for
				 * introspection.  It is the pixel
				 * offset of the right or bottom edge
				 * of this slot from the beginning of the
				 * layout. */
     	int temp;		/* This is a temporary value used for
     				 * calculating adjusted weights when
     				 * shrinking the layout below its
     				 * nominal size. */
} SlotInfo;

/*
 * Structure to hold information during layout calculations.  There
 * is one of these for each slot, an array for each of the rows or columns.
 */

typedef struct GridLayout {
    struct Gridder *binNextPtr;	/* The next slave window in this bin.
    				 * Each bin contains a list of all
    				 * slaves whose spans are >1 and whose
    				 * right edges fall in this slot. */
    int minSize;		/* Minimum size needed for this slot,
    				 * in pixels.  This is the space required
    				 * to hold any slaves contained entirely
    				 * in this slot, adjusted for any slot
    				 * constrants, such as size or padding. */
    int pad;			/* Padding needed for this slot */
    int weight;			/* Slot weight, controls resizing. */
    int minOffset;		/* The minimum offset, in pixels, from
    				 * the beginning of the layout to the
    				 * right/bottom edge of the slot calculated
    				 * from top/left to bottom/right. */
    int maxOffset;		/* The maximum offset, in pixels, from
    				 * the beginning of the layout to the
    				 * right-or-bottom edge of the slot calculated
    				 * from bottom-or-right to top-or-left. */
} GridLayout;

/*
 * Keep one of these for each geometry master.
 */

typedef struct {
    SlotInfo *columnPtr;	/* Pointer to array of column constraints. */
    SlotInfo *rowPtr;		/* Pointer to array of row constraints. */
    int columnEnd;		/* The last column occupied by any slave. */
    int columnMax;		/* The number of columns with constraints. */
    int columnSpace;		/* The number of slots currently allocated for
    				 * column constraints. */
    int rowEnd;			/* The last row occupied by any slave. */
    int rowMax;			/* The number of rows with constraints. */
    int rowSpace;		/* The number of slots currently allocated
    				 * for row constraints. */
    int startX;			/* Pixel offset of this layout within its
    				 * parent. */
    int startY;			/* Pixel offset of this layout within its
    				 * parent. */
} GridMaster;

/*
 * For each window that the grid cares about (either because
 * the window is managed by the grid or because the window
 * has slaves that are managed by the grid), there is a
 * structure of the following type:
 */

typedef struct Gridder {
    Tk_Window tkwin;		/* Tk token for window.  NULL means that
				 * the window has been deleted, but the
				 * gridder hasn't had a chance to clean up
				 * yet because the structure is still in
				 * use. */
    struct Gridder *masterPtr;	/* Master window within which this window
				 * is managed (NULL means this window
				 * isn't managed by the gridder). */
    struct Gridder *nextPtr;	/* Next window managed within same
				 * parent.  List order doesn't matter. */
    struct Gridder *slavePtr;	/* First in list of slaves managed
				 * inside this window (NULL means
				 * no grid slaves). */
    GridMaster *masterDataPtr;	/* Additional data for geometry master. */
    int column, row;		/* Location in the grid (starting
				 * from zero). */
    int numCols, numRows;	/* Number of columns or rows this slave spans.
				 * Should be at least 1. */
    int padX, padY;		/* Total additional pixels to leave around the
				 * window (half of this space is left on each
				 * side).  This is space *outside* the window:
				 * we'll allocate extra space in frame but
				 * won't enlarge window). */
    int iPadX, iPadY;		/* Total extra pixels to allocate inside the
				 * window (half this amount will appear on
				 * each side). */
    int sticky;			/* which sides of its cavity this window
				 * sticks to. See below for definitions */
    int doubleBw;		/* Twice the window's last known border
				 * width.  If this changes, the window
				 * must be re-arranged within its parent. */
    int *abortPtr;		/* If non-NULL, it means that there is a nested
				 * call to ArrangeGrid already working on
				 * this window.  *abortPtr may be set to 1 to
				 * abort that nested call.  This happens, for
				 * example, if tkwin or any of its slaves
				 * is deleted. */
    int flags;			/* Miscellaneous flags;  see below
				 * for definitions. */

    /*
     * These fields are used temporarily for layout calculations only.
     */

    struct Gridder *binNextPtr;	/* Link to next span>1 slave in this bin. */
    int size;			/* Nominal size (width or height) in pixels
    				 * of the slave.  This includes the padding. */
} Gridder;

/* Flag values for "sticky"ness  The 16 combinations subsume the packer's
 * notion of anchor and fill.
 *
 * STICK_NORTH  	This window sticks to the top of its cavity.
 * STICK_EAST		This window sticks to the right edge of its cavity.
 * STICK_SOUTH		This window sticks to the bottom of its cavity.
 * STICK_WEST		This window sticks to the left edge of its cavity.
 */

#define STICK_NORTH		1
#define STICK_EAST		2
#define STICK_SOUTH		4
#define STICK_WEST		8

/*
 * Flag values for Grid structures:
 *
 * REQUESTED_RELAYOUT:		1 means a Tcl_DoWhenIdle request
 *				has already been made to re-arrange
 *				all the slaves of this window.
 *
 * DONT_PROPAGATE:		1 means don't set this window's requested
 *				size.  0 means if this window is a master
 *				then Tk will set its requested size to fit
 *				the needs of its slaves.
 */

#define REQUESTED_RELAYOUT	1
#define DONT_PROPAGATE		2

/*
 * Prototypes for procedures used only in this file:
 */

static void	AdjustForSticky _ANSI_ARGS_((Gridder *slavePtr, int *xPtr, 
		    int *yPtr, int *widthPtr, int *heightPtr));
static int	AdjustOffsets _ANSI_ARGS_((int width,
			int elements, SlotInfo *slotPtr));
static void	ArrangeGrid _ANSI_ARGS_((ClientData clientData));
static int	CheckSlotData _ANSI_ARGS_((Gridder *masterPtr, int slot,
			int slotType, int checkOnly));
static int	ConfigureSlaves _ANSI_ARGS_((Tcl_Interp *interp,
			Tk_Window tkwin, int argc, char *argv[]));
static void	DestroyGrid _ANSI_ARGS_((char *memPtr));
static Gridder *GetGrid _ANSI_ARGS_((Tk_Window tkwin));
static void	GridStructureProc _ANSI_ARGS_((
			ClientData clientData, XEvent *eventPtr));
static void	GridLostSlaveProc _ANSI_ARGS_((ClientData clientData,
			Tk_Window tkwin));
static void	GridReqProc _ANSI_ARGS_((ClientData clientData,
			Tk_Window tkwin));
static void 	InitMasterData _ANSI_ARGS_((Gridder *masterPtr));
static int	ResolveConstraints _ANSI_ARGS_((Gridder *gridPtr,
			int rowOrColumn, int maxOffset));
static void	SetGridSize _ANSI_ARGS_((Gridder *gridPtr));
static void	StickyToString _ANSI_ARGS_((int flags, char *result));
static int	StringToSticky _ANSI_ARGS_((char *string));
static void	Unlink _ANSI_ARGS_((Gridder *gridPtr));

static Tk_GeomMgr gridMgrType = {
    "grid",			/* name */
    GridReqProc,		/* requestProc */
    GridLostSlaveProc,		/* lostSlaveProc */
};

/*
 *--------------------------------------------------------------
 *
 * Tk_GridCmd --
 *
 *	This procedure is invoked to process the "grid" Tcl command.
 *	See the user documentation for details on what it does.
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
Tk_GridCmd(clientData, interp, argc, argv)
    ClientData clientData;	/* Main window associated with
				 * interpreter. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int argc;			/* Number of arguments. */
    char **argv;		/* Argument strings. */
{
    Tk_Window tkwin = (Tk_Window) clientData;
    Gridder *masterPtr;		/* master grid record */
    GridMaster *gridPtr;	/* pointer to grid data */
    size_t length;		/* streing length of argument */
    char c;			/* 1st character of argument */
  
    if ((argc >= 2) && ((argv[1][0] == '.') || (argv[1][0] == REL_SKIP) ||
    		(argv[1][0] == REL_VERT))) {
	return ConfigureSlaves(interp, tkwin, argc-1, argv+1);
    }
    if (argc < 3) {
	Tcl_AppendResult(interp, "wrong # args: should be \"",
		argv[0], " option arg ?arg ...?\"", (char *) NULL);
	return TCL_ERROR;
    }
    c = argv[1][0];
    length = strlen(argv[1]);
  
    if ((c == 'b') && (strncmp(argv[1], "bbox", length) == 0)) {
	Tk_Window master;
	int row, column;	/* origin for bounding box */
	int row2, column2;	/* end of bounding box */
	int endX, endY;		/* last column/row in the layout */
	int x=0, y=0;		/* starting pixels for this bounding box */
	int width, height;	/* size of the bounding box */
	char buf[TCL_INTEGER_SPACE * 4];

	if (argc!=3 && argc != 5 && argc != 7) {
	    Tcl_AppendResult(interp, "wrong number of arguments: ",
		    "must be \"",argv[0],
		    " bbox master ?column row ?column row??\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
        
	master = Tk_NameToWindow(interp, argv[2], tkwin);
	if (master == NULL) {
	    return TCL_ERROR;
	}
	masterPtr = GetGrid(master);

	if (argc >= 5) {
	    if (Tcl_GetInt(interp, argv[3], &column) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (Tcl_GetInt(interp, argv[4], &row) != TCL_OK) {
		return TCL_ERROR;
	    }
	    column2 = column;
	    row2 = row;
	}

	if (argc == 7) {
	    if (Tcl_GetInt(interp, argv[5], &column2) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (Tcl_GetInt(interp, argv[6], &row2) != TCL_OK) {
		return TCL_ERROR;
	    }
	}

	gridPtr = masterPtr->masterDataPtr;
	if (gridPtr == NULL) {
	    Tcl_SetResult(interp, "0 0 0 0", TCL_STATIC);
	    return(TCL_OK);
	}

	SetGridSize(masterPtr);
	endX = MAX(gridPtr->columnEnd, gridPtr->columnMax);
	endY = MAX(gridPtr->rowEnd, gridPtr->rowMax);

	if ((endX == 0) || (endY == 0)) {
	    Tcl_SetResult(interp, "0 0 0 0", TCL_STATIC);
	    return(TCL_OK);
	}
	if (argc == 3) {
	    row = column = 0;
	    row2 = endY;
	    column2 = endX;
	}

	if (column > column2) {
	    int temp = column;
	    column = column2, column2 = temp;
	}
	if (row > row2) {
	    int temp = row;
	    row = row2, row2 = temp;
	}

	if (column > 0 && column < endX) {
	    x = gridPtr->columnPtr[column-1].offset;
	} else if  (column > 0) {
	    x = gridPtr->columnPtr[endX-1].offset;
	}

	if (row > 0 && row < endY) {
	    y = gridPtr->rowPtr[row-1].offset;
	} else if (row > 0) {
	    y = gridPtr->rowPtr[endY-1].offset;
	}

	if (column2 < 0) {
	    width = 0;
	} else if (column2 >= endX) {
	    width = gridPtr->columnPtr[endX-1].offset - x;
	} else {
	    width = gridPtr->columnPtr[column2].offset - x;
	} 

	if (row2 < 0) {
	    height = 0;
	} else if (row2 >= endY) {
	    height = gridPtr->rowPtr[endY-1].offset - y;
	} else {
	    height = gridPtr->rowPtr[row2].offset - y;
	} 

	sprintf(buf, "%d %d %d %d", x + gridPtr->startX, y + gridPtr->startY,
		width, height);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
    } else if ((c == 'c') && (strncmp(argv[1], "configure", length) == 0)) {
	if (argv[2][0] != '.') {
	    Tcl_AppendResult(interp, "bad argument \"", argv[2],
		    "\": must be name of window", (char *) NULL);
	    return TCL_ERROR;
	}
	return ConfigureSlaves(interp, tkwin, argc-2, argv+2);
    } else if (((c == 'f') && (strncmp(argv[1], "forget", length) == 0))  || 
	    ((c == 'r') && (strncmp(argv[1], "remove", length) == 0))) {
	Tk_Window slave;
	Gridder *slavePtr;
	int i;
    
	for (i = 2; i < argc; i++) {
	    slave = Tk_NameToWindow(interp, argv[i], tkwin);
	    if (slave == NULL) {
		return TCL_ERROR;
	    }
	    slavePtr = GetGrid(slave);
	    if (slavePtr->masterPtr != NULL) {

	    	/*
	    	 * For "forget", reset all the settings to their defaults
	    	 */

	    	if (c == 'f') {
		    slavePtr->column = slavePtr->row = -1;
		    slavePtr->numCols = 1;
		    slavePtr->numRows = 1;
		    slavePtr->padX = slavePtr->padY = 0;
		    slavePtr->iPadX = slavePtr->iPadY = 0;
		    slavePtr->doubleBw = 2*Tk_Changes(tkwin)->border_width;
		    if (slavePtr->flags & REQUESTED_RELAYOUT) {
			Tcl_CancelIdleCall(ArrangeGrid, (ClientData) slavePtr);
		    }
		    slavePtr->flags = 0;
		    slavePtr->sticky = 0;
	    	}
		Tk_ManageGeometry(slave, (Tk_GeomMgr *) NULL,
			(ClientData) NULL);
		if (slavePtr->masterPtr->tkwin != Tk_Parent(slavePtr->tkwin)) {
		    Tk_UnmaintainGeometry(slavePtr->tkwin,
			    slavePtr->masterPtr->tkwin);
		}
		Unlink(slavePtr);
		Tk_UnmapWindow(slavePtr->tkwin);
	    }
	}
    } else if ((c == 'i') && (strncmp(argv[1], "info", length) == 0)) {
	register Gridder *slavePtr;
	Tk_Window slave;
	char buffer[64 + TCL_INTEGER_SPACE * 4];
    
	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " info window\"", (char *) NULL);
	    return TCL_ERROR;
	}
	slave = Tk_NameToWindow(interp, argv[2], tkwin);
	if (slave == NULL) {
	    return TCL_ERROR;
	}
	slavePtr = GetGrid(slave);
	if (slavePtr->masterPtr == NULL) {
	    Tcl_ResetResult(interp);
	    return TCL_OK;
	}
    
	Tcl_AppendElement(interp, "-in");
	Tcl_AppendElement(interp, Tk_PathName(slavePtr->masterPtr->tkwin));
	sprintf(buffer, " -column %d -row %d -columnspan %d -rowspan %d",
		slavePtr->column, slavePtr->row,
		slavePtr->numCols, slavePtr->numRows);
	Tcl_AppendResult(interp, buffer, (char *) NULL);
	sprintf(buffer, " -ipadx %d -ipady %d -padx %d -pady %d",
		slavePtr->iPadX/2, slavePtr->iPadY/2, slavePtr->padX/2,
		slavePtr->padY/2);
	Tcl_AppendResult(interp, buffer, (char *) NULL);
	StickyToString(slavePtr->sticky,buffer);
	Tcl_AppendResult(interp, " -sticky ", buffer, (char *) NULL);
    } else if((c == 'l') && (strncmp(argv[1], "location", length) == 0)) {
	Tk_Window master;
	register SlotInfo *slotPtr;
	int x, y;		/* Offset in pixels, from edge of parent. */
	int i, j;		/* Corresponding column and row indeces. */
	int endX, endY;		/* end of grid */
	char buf[TCL_INTEGER_SPACE * 2];

	if (argc != 5) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " location master x y\"", (char *)NULL);
	    return TCL_ERROR;
	}

	master = Tk_NameToWindow(interp, argv[2], tkwin);
	if (master == NULL) {
	    return TCL_ERROR;
	}

	if (Tk_GetPixels(interp, master, argv[3], &x) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (Tk_GetPixels(interp, master, argv[4], &y) != TCL_OK) {
	    return TCL_ERROR;
	}

	masterPtr = GetGrid(master);
	if (masterPtr->masterDataPtr == NULL) {
	    Tcl_SetResult(interp, "-1 -1", TCL_STATIC);
	    return TCL_OK;
	}
	gridPtr = masterPtr->masterDataPtr;

	/* 
	 * Update any pending requests.  This is not always the
	 * steady state value, as more configure events could be in
	 * the pipeline, but its as close as its easy to get.
	 */

	while (masterPtr->flags & REQUESTED_RELAYOUT) {
	    Tcl_CancelIdleCall(ArrangeGrid, (ClientData) masterPtr);
	    ArrangeGrid ((ClientData) masterPtr);
	}
	SetGridSize(masterPtr);
	endX = MAX(gridPtr->columnEnd, gridPtr->columnMax);
	endY = MAX(gridPtr->rowEnd, gridPtr->rowMax);

	slotPtr  = masterPtr->masterDataPtr->columnPtr;
	if (x < masterPtr->masterDataPtr->startX) {
	    i = -1;
	} else {
	    x -= masterPtr->masterDataPtr->startX;
	    for (i=0;slotPtr[i].offset < x && i < endX; i++) {
		/* null body */
	    }
	}

	slotPtr  = masterPtr->masterDataPtr->rowPtr;
	if (y < masterPtr->masterDataPtr->startY) {
	    j = -1;
	} else {
	    y -= masterPtr->masterDataPtr->startY;
	    for (j=0;slotPtr[j].offset < y && j < endY; j++) {
		/* null body */
	    }
	}

	sprintf(buf, "%d %d", i, j);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
    } else if ((c == 'p') && (strncmp(argv[1], "propagate", length) == 0)) {
	Tk_Window master;
	int propagate;
    
	if (argc > 4) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " propagate window ?boolean?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	master = Tk_NameToWindow(interp, argv[2], tkwin);
	if (master == NULL) {
	    return TCL_ERROR;
	}
	masterPtr = GetGrid(master);
	if (argc == 3) {
	    Tcl_SetResult(interp,
		    ((masterPtr->flags & DONT_PROPAGATE) ? "0" : "1"),
		    TCL_STATIC);
	    return TCL_OK;
	}
	if (Tcl_GetBoolean(interp, argv[3], &propagate) != TCL_OK) {
	    return TCL_ERROR;
	}
	if ((!propagate) ^ (masterPtr->flags&DONT_PROPAGATE)) {
	    masterPtr->flags  ^= DONT_PROPAGATE;
      
	    /*
	     * Re-arrange the master to allow new geometry information to
	     * propagate upwards to the master's master.
	     */
      
	    if (masterPtr->abortPtr != NULL) {
		*masterPtr->abortPtr = 1;
	    }
	    if (!(masterPtr->flags & REQUESTED_RELAYOUT)) {
		masterPtr->flags |= REQUESTED_RELAYOUT;
		Tcl_DoWhenIdle(ArrangeGrid, (ClientData) masterPtr);
	    }
	}
    } else if ((c == 's') && (strncmp(argv[1], "size", length) == 0)
	    && (length > 1)) {
	Tk_Window master;

	if (argc != 3) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " size window\"", (char *) NULL);
	    return TCL_ERROR;
	}
	master = Tk_NameToWindow(interp, argv[2], tkwin);
	if (master == NULL) {
	    return TCL_ERROR;
	}
	masterPtr = GetGrid(master);

	if (masterPtr->masterDataPtr != NULL) {
	    char buf[TCL_INTEGER_SPACE * 2];

	    SetGridSize(masterPtr);
	    gridPtr = masterPtr->masterDataPtr;
	    sprintf(buf, "%d %d",
		    MAX(gridPtr->columnEnd, gridPtr->columnMax),
		    MAX(gridPtr->rowEnd, gridPtr->rowMax));
	    Tcl_SetResult(interp, buf, TCL_VOLATILE);
	} else {
	    Tcl_SetResult(interp, "0 0", TCL_STATIC);
	}
    } else if ((c == 's') && (strncmp(argv[1], "slaves", length) == 0)
	    && (length > 1)) {
	Tk_Window master;
	Gridder *slavePtr;
	int i, value;
	int row = -1, column = -1;
 
	if ((argc < 3) || ((argc%2) == 0)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"",
		    argv[0], " slaves window ?-option value...?\"",
		    (char *) NULL);
	    return TCL_ERROR;
	}

	for (i=3; i<argc; i+=2) {
	    length = strlen(argv[i]);
	    if ((*argv[i] != '-') || (length < 2)) {
		Tcl_AppendResult(interp, "invalid args: should be \"",
			argv[0], " slaves window ?-option value...?\"",
			(char *) NULL);
		return TCL_ERROR;
	    }
	    if (Tcl_GetInt(interp, argv[i+1], &value) != TCL_OK) {
		return TCL_ERROR;
	    }
	    if (value < 0) {
		Tcl_AppendResult(interp, argv[i],
			" is an invalid value: should NOT be < 0",
			(char *) NULL);
		return TCL_ERROR;
	    }
	    if (strncmp(argv[i], "-column", length) == 0) {
		column = value;
	    } else if (strncmp(argv[i], "-row", length) == 0) {
		row = value;
	    } else {
		Tcl_AppendResult(interp, argv[i],
			" is an invalid option: should be \"",
			"-row, -column\"",
			(char *) NULL);
		return TCL_ERROR;
	    }
	}
	master = Tk_NameToWindow(interp, argv[2], tkwin);
	if (master == NULL) {
	    return TCL_ERROR;
	}
	masterPtr = GetGrid(master);

	for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
					     slavePtr = slavePtr->nextPtr) {
	    if (column>=0 && (slavePtr->column > column
		    || slavePtr->column+slavePtr->numCols-1 < column)) {
		continue;
	    }
	    if (row>=0 && (slavePtr->row > row ||
		    slavePtr->row+slavePtr->numRows-1 < row)) {
		continue;
	    }
	    Tcl_AppendElement(interp, Tk_PathName(slavePtr->tkwin));
	}

    /*
     * Sample argument combinations:
     *  grid columnconfigure <master> <index> -option
     *  grid columnconfigure <master> <index> -option value -option value
     *  grid rowconfigure <master> <index>
     *  grid rowconfigure <master> <index> -option
     *  grid rowconfigure <master> <index> -option value -option value.
     */
   
    } else if(((c == 'c') && (strncmp(argv[1], "columnconfigure", length) == 0)
	    && (length >= 3)) ||
            ((c == 'r') && (strncmp(argv[1], "rowconfigure", length) == 0) 
            && (length >=2))) {
	Tk_Window master;
	SlotInfo *slotPtr = NULL;
	int slot;		/* the column or row number */
	size_t length;		/* the # of chars in the "-option" string */
	int slotType;		/* COLUMN or ROW */
	int size;		/* the configuration value */
	int checkOnly;		/* check the size only */
	int argcPtr;		/* Number of items in index list */
	char **argvPtr;		/* array of indeces */
	char **indexP;		/* String value of current index list item. */
	int ok;			/* temporary TCL result code */
	int i;

	if (((argc%2 != 0) && (argc>6)) || (argc < 4)) {
	    Tcl_AppendResult(interp, "wrong # args: should be \"", argv[0],
		    " ", argv[1], " master index ?-option value...?\"",
		    (char *)NULL);
	    return TCL_ERROR;
	}

	master = Tk_NameToWindow(interp, argv[2], tkwin);
	if (master == NULL) {
	    return TCL_ERROR;
	}

	if (Tcl_SplitList(interp, argv[3], &argcPtr, &argvPtr) != TCL_OK) {
	    return TCL_ERROR;
	}

	checkOnly = ((argc == 4) || (argc == 5));
	masterPtr = GetGrid(master);
	slotType = (c == 'c') ? COLUMN : ROW;
	if (checkOnly && argcPtr > 1) {
	    Tcl_AppendResult(interp, argv[3],
		    " must be a single element.", (char *) NULL);
	    Tcl_Free((char *)argvPtr);
	    return TCL_ERROR;
	}
	for (indexP=argvPtr; *indexP != NULL; indexP++) {
	    if (Tcl_GetInt(interp, *indexP, &slot) != TCL_OK) {
		Tcl_Free((char *)argvPtr);
		return TCL_ERROR;
	    }
	    ok = CheckSlotData(masterPtr, slot, slotType, checkOnly);
	    if ((ok!=TCL_OK) && ((argc<4) || (argc>5))) {
		Tcl_AppendResult(interp, argv[0],
			" ", argv[1], ": \"", *argvPtr,"\" is out of range",
			(char *) NULL);
		Tcl_Free((char *)argvPtr);
		return TCL_ERROR;
	    } else if (ok == TCL_OK) {
		slotPtr = (slotType == COLUMN) ?
			masterPtr->masterDataPtr->columnPtr :
			masterPtr->masterDataPtr->rowPtr;
	    }

	    /*
	     * Return all of the options for this row or column.  If the
	     * request is out of range, return all 0's.
	     */

	    if (argc == 4) {
		Tcl_Free((char *)argvPtr);
	    }
	    if ((argc == 4) && (ok == TCL_OK)) {
		char buf[64 + TCL_INTEGER_SPACE * 3];
		
		sprintf(buf, "-minsize %d -pad %d -weight %d",
			slotPtr[slot].minSize,slotPtr[slot].pad,
			slotPtr[slot].weight);
		Tcl_SetResult(interp, buf, TCL_VOLATILE);
		return (TCL_OK);
	    } else if (argc == 4) {
		Tcl_SetResult(interp, "-minsize 0 -pad 0 -weight 0",
			TCL_STATIC);
		return (TCL_OK);
	    }

	    /*
	     * Loop through each option value pair, setting the values as required.
	     * If only one option is given, with no value, the current value is
	     * returned.
	     */

	    for (i=4; i<argc; i+=2) {
		length = strlen(argv[i]);
		if ((*argv[i] != '-') || length < 2) {
		    Tcl_AppendResult(interp, "invalid arg \"",
			    argv[i], "\" :expecting -minsize, -pad, or -weight.",
			    (char *) NULL);
		    Tcl_Free((char *)argvPtr);
		    return TCL_ERROR;
		}
		if (strncmp(argv[i], "-minsize", length) == 0) {
		    if (argc == 5) {
			char buf[TCL_INTEGER_SPACE];
		    	int value;

			value = (ok == TCL_OK) ? slotPtr[slot].minSize : 0;
			sprintf(buf, "%d", value);
			Tcl_SetResult(interp, buf, TCL_VOLATILE);
		    } else if (Tk_GetPixels(interp, master, argv[i+1], &size)
			    != TCL_OK) {
			Tcl_Free((char *)argvPtr);
			return TCL_ERROR;
		    } else {
			slotPtr[slot].minSize = size;
		    }
		}
		else if (strncmp(argv[i], "-weight", length) == 0) {
		    int wt;
		    if (argc == 5) {
			char buf[TCL_INTEGER_SPACE];
		    	int value;

			value = (ok == TCL_OK) ? slotPtr[slot].weight : 0;
			sprintf(buf, "%d", value);
			Tcl_SetResult(interp, buf, TCL_VOLATILE);
		    } else if (Tcl_GetInt(interp, argv[i+1], &wt) != TCL_OK) {
			Tcl_Free((char *)argvPtr);
			return TCL_ERROR;
		    } else if (wt < 0) {
			Tcl_AppendResult(interp, "invalid arg \"", argv[i],
				"\": should be non-negative", (char *) NULL);
			Tcl_Free((char *)argvPtr);
			return TCL_ERROR;
		    } else {
			slotPtr[slot].weight = wt;
		    }
		}
		else if (strncmp(argv[i], "-pad", length) == 0) {
		    if (argc == 5) {
			char buf[TCL_INTEGER_SPACE];
		    	int value;

			value = (ok == TCL_OK) ? slotPtr[slot].pad : 0;
			sprintf(buf, "%d", value);
			Tcl_SetResult(interp, buf, TCL_VOLATILE);
		    } else if (Tk_GetPixels(interp, master, argv[i+1], &size)
			    != TCL_OK) {
			Tcl_Free((char *)argvPtr);
			return TCL_ERROR;
		    } else if (size < 0) {
			Tcl_AppendResult(interp, "invalid arg \"", argv[i],
				"\": should be non-negative", (char *) NULL);
			Tcl_Free((char *)argvPtr);
			return TCL_ERROR;
		    } else {
			slotPtr[slot].pad = size;
		    }
		} else {
		    Tcl_AppendResult(interp, "invalid arg \"",
			    argv[i], "\": expecting -minsize, -pad, or -weight.",
			    (char *) NULL);
		    Tcl_Free((char *)argvPtr);
		    return TCL_ERROR;
		}
	    }
	}
	Tcl_Free((char *)argvPtr);

	/*
	 * If we changed a property, re-arrange the table,
	 * and check for constraint shrinkage.
	 */

	if (argc != 5) {
	    if (slotType == ROW) {
		int last = masterPtr->masterDataPtr->rowMax - 1;
		while ((last >= 0) && (slotPtr[last].weight == 0)
			&& (slotPtr[last].pad == 0)
			&& (slotPtr[last].minSize == 0)) {
		    last--;
		}
		masterPtr->masterDataPtr->rowMax = last+1;
	    } else {
		int last = masterPtr->masterDataPtr->columnMax - 1;
		while ((last >= 0) && (slotPtr[last].weight == 0)
			&& (slotPtr[last].pad == 0)
			&& (slotPtr[last].minSize == 0)) {
		    last--;
		}
		masterPtr->masterDataPtr->columnMax = last + 1;
	    }

	    if (masterPtr->abortPtr != NULL) {
		*masterPtr->abortPtr = 1;
	    }
	    if (!(masterPtr->flags & REQUESTED_RELAYOUT)) {
		masterPtr->flags |= REQUESTED_RELAYOUT;
		Tcl_DoWhenIdle(ArrangeGrid, (ClientData) masterPtr);
	    }
	}
    } else {
	Tcl_AppendResult(interp, "bad option \"", argv[1],
		"\":  must be bbox, columnconfigure, configure, forget, info, ",
		"location, propagate, remove, rowconfigure, size, or slaves.",
		(char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------
 *
 * GridReqProc --
 *
 *	This procedure is invoked by Tk_GeometryRequest for
 *	windows managed by the grid.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Arranges for tkwin, and all its managed siblings, to
 *	be re-arranged at the next idle point.
 *
 *--------------------------------------------------------------
 */

static void
GridReqProc(clientData, tkwin)
    ClientData clientData;	/* Grid's information about
				 * window that got new preferred
				 * geometry.  */
    Tk_Window tkwin;		/* Other Tk-related information
				 * about the window. */
{
    register Gridder *gridPtr = (Gridder *) clientData;

    gridPtr = gridPtr->masterPtr;
    if (!(gridPtr->flags & REQUESTED_RELAYOUT)) {
	gridPtr->flags |= REQUESTED_RELAYOUT;
	Tcl_DoWhenIdle(ArrangeGrid, (ClientData) gridPtr);
    }
}

/*
 *--------------------------------------------------------------
 *
 * GridLostSlaveProc --
 *
 *	This procedure is invoked by Tk whenever some other geometry
 *	claims control over a slave that used to be managed by us.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Forgets all grid-related information about the slave.
 *
 *--------------------------------------------------------------
 */

static void
GridLostSlaveProc(clientData, tkwin)
    ClientData clientData;	/* Grid structure for slave window that
				 * was stolen away. */
    Tk_Window tkwin;		/* Tk's handle for the slave window. */
{
    register Gridder *slavePtr = (Gridder *) clientData;

    if (slavePtr->masterPtr->tkwin != Tk_Parent(slavePtr->tkwin)) {
	Tk_UnmaintainGeometry(slavePtr->tkwin, slavePtr->masterPtr->tkwin);
    }
    Unlink(slavePtr);
    Tk_UnmapWindow(slavePtr->tkwin);
}

/*
 *--------------------------------------------------------------
 *
 * AdjustOffsets --
 *
 *	This procedure adjusts the size of the layout to fit in the
 *	space provided.  If it needs more space, the extra is added
 *	according to the weights.  If it needs less, the space is removed
 *	according to the weights, but at no time does the size drop below
 *	the minsize specified for that slot.
 *
 * Results:
 *	The initial offset of the layout,
 *	if all the weights are zero, else 0.
 *
 * Side effects:
 *	The slot offsets are modified to shrink the layout.
 *
 *--------------------------------------------------------------
 */

static int
AdjustOffsets(size, slots, slotPtr)
    int size;			/* The total layout size (in pixels). */
    int slots;			/* Number of slots. */
    register SlotInfo *slotPtr;	/* Pointer to slot array. */
{
    register int slot;		/* Current slot. */
    int diff;			/* Extra pixels needed to add to the layout. */
    int totalWeight = 0;	/* Sum of the weights for all the slots. */
    int weight = 0;		/* Sum of the weights so far. */
    int minSize = 0;		/* Minimum possible layout size. */
    int newDiff;		/* The most pixels that can be added on
    				 * the current pass. */

    diff = size - slotPtr[slots-1].offset;

    /*
     * The layout is already the correct size; all done.
     */

    if (diff == 0) {
	return(0);
    }

    /*
     * If all the weights are zero, center the layout in its parent if 
     * there is extra space, else clip on the bottom/right.
     */

    for (slot=0; slot < slots; slot++) {
	totalWeight += slotPtr[slot].weight;
    }

    if (totalWeight == 0 ) {
	return(diff > 0 ? diff/2 : 0);
    }

    /*
     * Add extra space according to the slot weights.  This is done
     * cumulatively to prevent round-off error accumulation.
     */

    if (diff > 0) {
	for (weight=slot=0; slot < slots; slot++) {
	    weight += slotPtr[slot].weight;
	    slotPtr[slot].offset += diff * weight / totalWeight;
	}
	return(0);
    }

    /*
     * The layout must shrink below its requested size.  Compute the
     * minimum possible size by looking at the slot minSizes.
     */

    for (slot=0; slot < slots; slot++) {
    	if (slotPtr[slot].weight > 0) {
	    minSize += slotPtr[slot].minSize;
	} else if (slot > 0) {
	    minSize += slotPtr[slot].offset - slotPtr[slot-1].offset;
	} else {
	    minSize += slotPtr[slot].offset;
	}
    }

    /*
     * If the requested size is less than the minimum required size,
     * set the slot sizes to their minimum values, then clip on the 
     * bottom/right.
     */

    if (size <= minSize) {
    	int offset = 0;
	for (slot=0; slot < slots; slot++) {
	    if (slotPtr[slot].weight > 0) {
		offset += slotPtr[slot].minSize;
	    } else if (slot > 0) {
		offset += slotPtr[slot].offset - slotPtr[slot-1].offset;
	    } else {
		offset += slotPtr[slot].offset;
	    }
	    slotPtr[slot].offset = offset;
	}
	return(0);
    }

    /*
     * Remove space from slots according to their weights.  The weights
     * get renormalized anytime a slot shrinks to its minimum size.
     */

    while (diff < 0) {

	/*
	 * Find the total weight for the shrinkable slots.
	 */

	for (totalWeight=slot=0; slot < slots; slot++) {
	    int current = (slot == 0) ? slotPtr[slot].offset :
		    slotPtr[slot].offset - slotPtr[slot-1].offset;
	    if (current > slotPtr[slot].minSize) {
		totalWeight += slotPtr[slot].weight;
		slotPtr[slot].temp = slotPtr[slot].weight;
	    } else {
		slotPtr[slot].temp = 0;
	    }
	}
	if (totalWeight == 0) {
	    break;
	}

	/*
	 * Find the maximum amount of space we can distribute this pass.
	 */

	newDiff = diff;
	for (slot = 0; slot < slots; slot++) {
	    int current;		/* current size of this slot */
	    int maxDiff;		/* max diff that would cause
	    				 * this slot to equal its minsize */
	    if (slotPtr[slot].temp == 0) {
	    	continue;
	    }
	    current = (slot == 0) ? slotPtr[slot].offset :
		    slotPtr[slot].offset - slotPtr[slot-1].offset;
	    maxDiff = totalWeight * (slotPtr[slot].minSize - current)
		    / slotPtr[slot].temp;
	    if (maxDiff > newDiff) {
	    	newDiff = maxDiff;
	    }
	}

	/*
	 * Now distribute the space.
	 */

	for (weight=slot=0; slot < slots; slot++) {
	    weight += slotPtr[slot].temp;
	    slotPtr[slot].offset += newDiff * weight / totalWeight;
	}
    	diff -= newDiff;
    }
    return(0);
}

/*
 *--------------------------------------------------------------
 *
 * AdjustForSticky --
 *
 *	This procedure adjusts the size of a slave in its cavity based
 *	on its "sticky" flags.
 *
 * Results:
 *	The input x, y, width, and height are changed to represent the
 *	desired coordinates of the slave.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

static void
AdjustForSticky(slavePtr, xPtr, yPtr, widthPtr, heightPtr)
    Gridder *slavePtr;	/* Slave window to arrange in its cavity. */
    int *xPtr;		/* Pixel location of the left edge of the cavity. */
    int *yPtr;		/* Pixel location of the top edge of the cavity. */
    int *widthPtr;	/* Width of the cavity (in pixels). */
    int *heightPtr;	/* Height of the cavity (in pixels). */
{
    int diffx=0;	/* Cavity width - slave width. */
    int diffy=0;	/* Cavity hight - slave height. */
    int sticky = slavePtr->sticky;

    *xPtr += slavePtr->padX/2;
    *widthPtr -= slavePtr->padX;
    *yPtr += slavePtr->padY/2;
    *heightPtr -= slavePtr->padY;

    if (*widthPtr > (Tk_ReqWidth(slavePtr->tkwin) + slavePtr->iPadX)) {
	diffx = *widthPtr - (Tk_ReqWidth(slavePtr->tkwin) + slavePtr->iPadX);
	*widthPtr = Tk_ReqWidth(slavePtr->tkwin) + slavePtr->iPadX;
    }

    if (*heightPtr > (Tk_ReqHeight(slavePtr->tkwin) + slavePtr->iPadY)) {
	diffy = *heightPtr - (Tk_ReqHeight(slavePtr->tkwin) + slavePtr->iPadY);
	*heightPtr = Tk_ReqHeight(slavePtr->tkwin) + slavePtr->iPadY;
    }

    if (sticky&STICK_EAST && sticky&STICK_WEST) {
	*widthPtr += diffx;
    }
    if (sticky&STICK_NORTH && sticky&STICK_SOUTH) {
	*heightPtr += diffy;
    }
    if (!(sticky&STICK_WEST)) {
    	*xPtr += (sticky&STICK_EAST) ? diffx : diffx/2;
    }
    if (!(sticky&STICK_NORTH)) {
    	*yPtr += (sticky&STICK_SOUTH) ? diffy : diffy/2;
    }
}

/*
 *--------------------------------------------------------------
 *
 * ArrangeGrid --
 *
 *	This procedure is invoked (using the Tcl_DoWhenIdle
 *	mechanism) to re-layout a set of windows managed by
 *	the grid.  It is invoked at idle time so that a
 *	series of grid requests can be merged into a single
 *	layout operation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The slaves of masterPtr may get resized or moved.
 *
 *--------------------------------------------------------------
 */

static void
ArrangeGrid(clientData)
    ClientData clientData;	/* Structure describing parent whose slaves
				 * are to be re-layed out. */
{
    register Gridder *masterPtr = (Gridder *) clientData;
    register Gridder *slavePtr;	
    GridMaster *slotPtr = masterPtr->masterDataPtr;
    int abort;
    int width, height;		/* requested size of layout, in pixels */
    int realWidth, realHeight;	/* actual size layout should take-up */

    masterPtr->flags &= ~REQUESTED_RELAYOUT;

    /*
     * If the parent has no slaves anymore, then don't do anything
     * at all:  just leave the parent's size as-is.  Otherwise there is
     * no way to "relinquish" control over the parent so another geometry
     * manager can take over.
     */

    if (masterPtr->slavePtr == NULL) {
	return;
    }

    if (masterPtr->masterDataPtr == NULL) {
	return;
    }

    /*
     * Abort any nested call to ArrangeGrid for this window, since
     * we'll do everything necessary here, and set up so this call
     * can be aborted if necessary.  
     */

    if (masterPtr->abortPtr != NULL) {
	*masterPtr->abortPtr = 1;
    }
    masterPtr->abortPtr = &abort;
    abort = 0;
    Tcl_Preserve((ClientData) masterPtr);

    /*
     * Call the constraint engine to fill in the row and column offsets.
     */

    SetGridSize(masterPtr);
    width =  ResolveConstraints(masterPtr, COLUMN, 0);
    height = ResolveConstraints(masterPtr, ROW, 0);
    width += 2*Tk_InternalBorderWidth(masterPtr->tkwin);
    height += 2*Tk_InternalBorderWidth(masterPtr->tkwin);

    if (((width != Tk_ReqWidth(masterPtr->tkwin))
            || (height != Tk_ReqHeight(masterPtr->tkwin)))
            && !(masterPtr->flags & DONT_PROPAGATE)) {
        Tk_GeometryRequest(masterPtr->tkwin, width, height);
	if (width>1 && height>1) {
	    masterPtr->flags |= REQUESTED_RELAYOUT;
	    Tcl_DoWhenIdle(ArrangeGrid, (ClientData) masterPtr);
	}
	masterPtr->abortPtr = NULL;
	Tcl_Release((ClientData) masterPtr);
        return;
    }

    /*
     * If the currently requested layout size doesn't match the parent's
     * window size, then adjust the slot offsets according to the
     * weights.  If all of the weights are zero, center the layout in 
     * its parent.  I haven't decided what to do if the parent is smaller
     * than the requested size.
     */

    realWidth = Tk_Width(masterPtr->tkwin) -
	    2*Tk_InternalBorderWidth(masterPtr->tkwin);
    realHeight = Tk_Height(masterPtr->tkwin) -
	    2*Tk_InternalBorderWidth(masterPtr->tkwin);
    slotPtr->startX = AdjustOffsets(realWidth,
	    MAX(slotPtr->columnEnd,slotPtr->columnMax), slotPtr->columnPtr);
    slotPtr->startY = AdjustOffsets(realHeight,
	    MAX(slotPtr->rowEnd,slotPtr->rowMax), slotPtr->rowPtr);
    slotPtr->startX += Tk_InternalBorderWidth(masterPtr->tkwin);
    slotPtr->startY += Tk_InternalBorderWidth(masterPtr->tkwin);

    /*
     * Now adjust the actual size of the slave to its cavity by
     * computing the cavity size, and adjusting the widget according
     * to its stickyness.
     */

    for (slavePtr = masterPtr->slavePtr; slavePtr != NULL && !abort;
	    slavePtr = slavePtr->nextPtr) {
	int x, y;			/* top left coordinate */
	int width, height;		/* slot or slave size */
	int col = slavePtr->column;
	int row = slavePtr->row;

	x = (col>0) ? slotPtr->columnPtr[col-1].offset : 0;
	y = (row>0) ? slotPtr->rowPtr[row-1].offset : 0;

	width = slotPtr->columnPtr[slavePtr->numCols+col-1].offset - x;
	height = slotPtr->rowPtr[slavePtr->numRows+row-1].offset - y;

        x += slotPtr->startX;
        y += slotPtr->startY;

	AdjustForSticky(slavePtr, &x, &y, &width, &height);

	/*
	 * Now put the window in the proper spot.  (This was taken directly
	 * from tkPack.c.)  If the slave is a child of the master, then
         * do this here.  Otherwise let Tk_MaintainGeometry do the work.
         */

        if (masterPtr->tkwin == Tk_Parent(slavePtr->tkwin)) {
            if ((width <= 0) || (height <= 0)) {
                Tk_UnmapWindow(slavePtr->tkwin);
            } else {
                if ((x != Tk_X(slavePtr->tkwin))
                        || (y != Tk_Y(slavePtr->tkwin))
                        || (width != Tk_Width(slavePtr->tkwin))
                        || (height != Tk_Height(slavePtr->tkwin))) {
                    Tk_MoveResizeWindow(slavePtr->tkwin, x, y, width, height);
                }
                if (abort) {
                    break;
                }
 
                /*
                 * Don't map the slave if the master isn't mapped: wait
                 * until the master gets mapped later.
                 */
 
                if (Tk_IsMapped(masterPtr->tkwin)) {
                    Tk_MapWindow(slavePtr->tkwin);
                }
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

    masterPtr->abortPtr = NULL;
    Tcl_Release((ClientData) masterPtr);
}

/*
 *--------------------------------------------------------------
 *
 * ResolveConstraints --
 *
 *	Resolve all of the column and row boundaries.  Most of
 *	the calculations are identical for rows and columns, so this procedure
 *	is called twice, once for rows, and again for columns.
 *
 * Results:
 *	The offset (in pixels) from the left/top edge of this layout is
 *	returned.
 *
 * Side effects:
 *	The slot offsets are copied into the SlotInfo structure for the
 *	geometry master.
 *
 *--------------------------------------------------------------
 */

static int
ResolveConstraints(masterPtr, slotType, maxOffset) 
    Gridder *masterPtr;		/* The geometry master for this grid. */
    int slotType;		/* Either ROW or COLUMN. */
    int maxOffset;		/* The actual maximum size of this layout
    				 * in pixels,  or 0 (not currently used). */
{
    register SlotInfo *slotPtr;	/* Pointer to row/col constraints. */
    register Gridder *slavePtr;	/* List of slave windows in this grid. */
    int constraintCount;	/* Count of rows or columns that have
    				 * constraints. */
    int slotCount;		/* Last occupied row or column. */
    int gridCount;		/* The larger of slotCount and constraintCount.
    				 */
    GridLayout *layoutPtr;	/* Temporary layout structure. */
    int requiredSize;		/* The natural size of the grid (pixels).
				 * This is the minimum size needed to
				 * accomodate all of the slaves at their
				 * requested sizes. */
    int offset;			/* The pixel offset of the right edge of the
    				 * current slot from the beginning of the
    				 * layout. */
    int slot;			/* The current slot. */
    int start;			/* The first slot of a contiguous set whose
    				 * constraints are not yet fully resolved. */
    int end;			/* The Last slot of a contiguous set whose
				 * constraints are not yet fully resolved. */

    /*
     * For typical sized tables, we'll use stack space for the layout data
     * to avoid the overhead of a malloc and free for every layout.
     */

    GridLayout layoutData[TYPICAL_SIZE + 1];

    if (slotType == COLUMN) {
	constraintCount = masterPtr->masterDataPtr->columnMax;
	slotCount = masterPtr->masterDataPtr->columnEnd;
	slotPtr  = masterPtr->masterDataPtr->columnPtr;
    } else {
	constraintCount = masterPtr->masterDataPtr->rowMax;
	slotCount = masterPtr->masterDataPtr->rowEnd;
	slotPtr  = masterPtr->masterDataPtr->rowPtr;
    }

    /*
     * Make sure there is enough memory for the layout.
     */

    gridCount = MAX(constraintCount,slotCount);
    if (gridCount >= TYPICAL_SIZE) {
	layoutPtr = (GridLayout *) Tcl_Alloc(sizeof(GridLayout) * (1+gridCount));
    } else {
	layoutPtr = layoutData;
    }

    /*
     * Allocate an extra layout slot to represent the left/top edge of
     * the 0th slot to make it easier to calculate slot widths from
     * offsets without special case code.
     * Initialize the "dummy" slot to the left/top of the table.
     * This slot avoids special casing the first slot.
     */

    layoutPtr->minOffset = 0;
    layoutPtr->maxOffset = 0;
    layoutPtr++;

    /*
     * Step 1.
     * Copy the slot constraints into the layout structure,
     * and initialize the rest of the fields.
     */

    for (slot=0; slot < constraintCount; slot++) {
        layoutPtr[slot].minSize = slotPtr[slot].minSize;
        layoutPtr[slot].weight =  slotPtr[slot].weight;
        layoutPtr[slot].pad =  slotPtr[slot].pad;
        layoutPtr[slot].binNextPtr = NULL;
    }
    for(;slot<gridCount;slot++) {
        layoutPtr[slot].minSize = 0;
        layoutPtr[slot].weight = 0;
        layoutPtr[slot].pad = 0;
        layoutPtr[slot].binNextPtr = NULL;
    }

    /*
     * Step 2.
     * Slaves with a span of 1 are used to determine the minimum size of
     * each slot.  Slaves whose span is two or more slots don't
     * contribute to the minimum size of each slot directly, but can cause
     * slots to grow if their size exceeds the the sizes of the slots they
     * span.
     * 
     * Bin all slaves whose spans are > 1 by their right edges.  This
     * allows the computation on minimum and maximum possible layout
     * sizes at each slot boundary, without the need to re-sort the slaves.
     */
 
    switch (slotType) {
    	case COLUMN:
	    for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
			slavePtr = slavePtr->nextPtr) {
		int rightEdge = slavePtr->column + slavePtr->numCols - 1;
		slavePtr->size = Tk_ReqWidth(slavePtr->tkwin) +
			slavePtr->padX + slavePtr->iPadX + slavePtr->doubleBw;
		if (slavePtr->numCols > 1) {
		    slavePtr->binNextPtr = layoutPtr[rightEdge].binNextPtr;
		    layoutPtr[rightEdge].binNextPtr = slavePtr;
		} else {
		    int size = slavePtr->size + layoutPtr[rightEdge].pad;
		    if (size > layoutPtr[rightEdge].minSize) {
			layoutPtr[rightEdge].minSize = size;
		    }
		}
	    }
	    break;
    	case ROW:
	    for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
			slavePtr = slavePtr->nextPtr) {
		int rightEdge = slavePtr->row + slavePtr->numRows - 1;
		slavePtr->size = Tk_ReqHeight(slavePtr->tkwin) +
			slavePtr->padY + slavePtr->iPadY + slavePtr->doubleBw;
		if (slavePtr->numRows > 1) {
		    slavePtr->binNextPtr = layoutPtr[rightEdge].binNextPtr;
		    layoutPtr[rightEdge].binNextPtr = slavePtr;
		} else {
		    int size = slavePtr->size + layoutPtr[rightEdge].pad;
		    if (size > layoutPtr[rightEdge].minSize) {
			layoutPtr[rightEdge].minSize = size;
		    }
		}
	    }
	    break;
	}

    /*
     * Step 3.
     * Determine the minimum slot offsets going from left to right
     * that would fit all of the slaves.  This determines the minimum
     */

    for (offset=slot=0; slot < gridCount; slot++) {
        layoutPtr[slot].minOffset = layoutPtr[slot].minSize + offset;
        for (slavePtr = layoutPtr[slot].binNextPtr; slavePtr != NULL;
                    slavePtr = slavePtr->binNextPtr) {
	    int span = (slotType == COLUMN) ? slavePtr->numCols : slavePtr->numRows;
            int required = slavePtr->size + layoutPtr[slot - span].minOffset;
            if (required > layoutPtr[slot].minOffset) {
                layoutPtr[slot].minOffset = required;
            }
        }
        offset = layoutPtr[slot].minOffset;
    }

    /*
     * At this point, we know the minimum required size of the entire layout.
     * It might be prudent to stop here if our "master" will resize itself
     * to this size.
     */

    requiredSize = offset;
    if (maxOffset > offset) {
    	offset=maxOffset;
    }

    /*
     * Step 4.
     * Determine the minimum slot offsets going from right to left,
     * bounding the pixel range of each slot boundary.
     * Pre-fill all of the right offsets with the actual size of the table;
     * they will be reduced as required.
     */

    for (slot=0; slot < gridCount; slot++) {
        layoutPtr[slot].maxOffset = offset;
    }
    for (slot=gridCount-1; slot > 0;) {
        for (slavePtr = layoutPtr[slot].binNextPtr; slavePtr != NULL;
                    slavePtr = slavePtr->binNextPtr) {
	    int span = (slotType == COLUMN) ? slavePtr->numCols : slavePtr->numRows;
            int require = offset - slavePtr->size;
            int startSlot  = slot - span;
            if (startSlot >=0 && require < layoutPtr[startSlot].maxOffset) {
                layoutPtr[startSlot].maxOffset = require;
            }
	}
	offset -= layoutPtr[slot].minSize;
	slot--;
	if (layoutPtr[slot].maxOffset < offset) {
	    offset = layoutPtr[slot].maxOffset;
	} else {
	    layoutPtr[slot].maxOffset = offset;
	}
    }

    /*
     * Step 5.
     * At this point, each slot boundary has a range of values that
     * will satisfy the overall layout size.
     * Make repeated passes over the layout structure looking for
     * spans of slot boundaries where the minOffsets are less than
     * the maxOffsets, and adjust the offsets according to the slot
     * weights.  At each pass, at least one slot boundary will have
     * its range of possible values fixed at a single value.
     */

    for (start=0; start < gridCount;) {
    	int totalWeight = 0;	/* Sum of the weights for all of the
    				 * slots in this span. */
    	int need = 0;		/* The minimum space needed to layout
    				 * this span. */
    	int have;		/* The actual amount of space that will
    				 * be taken up by this span. */
    	int weight;		/* Cumulative weights of the columns in 
    				 * this span. */
    	int noWeights = 0;	/* True if the span has no weights. */

    	/*
    	 * Find a span by identifying ranges of slots whose edges are
    	 * already constrained at fixed offsets, but whose internal
    	 * slot boundaries have a range of possible positions.
    	 */

    	if (layoutPtr[start].minOffset == layoutPtr[start].maxOffset) {
	    start++;
	    continue;
	}

	for (end=start+1; end<gridCount; end++) {
	    if (layoutPtr[end].minOffset == layoutPtr[end].maxOffset) {
		break;
	    }
	}

	/*
	 * We found a span.  Compute the total weight, minumum space required,
	 * for this span, and the actual amount of space the span should
	 * use.
	 */

	for (slot=start; slot<=end; slot++) {
	    totalWeight += layoutPtr[slot].weight;
	    need += layoutPtr[slot].minSize;
	}
	have = layoutPtr[end].maxOffset - layoutPtr[start-1].minOffset;

	/*
	 * If all the weights in the span are zero, then distribute the
	 * extra space evenly.
	 */

	if (totalWeight == 0) {
	    noWeights++;
	    totalWeight = end - start + 1;
	}

	/*
	 * It might not be possible to give the span all of the space
	 * available on this pass without violating the size constraints 
	 * of one or more of the internal slot boundaries.
	 * Determine the maximum amount of space that when added to the
	 * entire span, would cause a slot boundary to have its possible
	 * range reduced to one value, and reduce the amount of extra
	 * space allocated on this pass accordingly.
	 * 
	 * The calculation is done cumulatively to avoid accumulating
	 * roundoff errors.
	 */

	for (weight=0,slot=start; slot<end; slot++) {
	    int diff = layoutPtr[slot].maxOffset - layoutPtr[slot].minOffset;
	    weight += noWeights ? 1 : layoutPtr[slot].weight;
	    if ((noWeights || layoutPtr[slot].weight>0) &&
		    (diff*totalWeight/weight) < (have-need)) {
		have = diff * totalWeight / weight + need;
	    }
	}

	/*
	 * Now distribute the extra space among the slots by
	 * adjusting the minSizes and minOffsets.
	 */

	for (weight=0,slot=start; slot<end; slot++) {
	    weight += noWeights ? 1 : layoutPtr[slot].weight;
	    layoutPtr[slot].minOffset +=
		(int)((double) (have-need) * weight/totalWeight + 0.5);
	    layoutPtr[slot].minSize = layoutPtr[slot].minOffset 
		    - layoutPtr[slot-1].minOffset;
	}
	layoutPtr[slot].minSize = layoutPtr[slot].minOffset 
		- layoutPtr[slot-1].minOffset;

	/*
	 * Having pushed the top/left boundaries of the slots to
	 * take up extra space, the bottom/right space is recalculated
	 * to propagate the new space allocation.
	 */

	for (slot=end; slot > start; slot--) {
	    layoutPtr[slot-1].maxOffset = 
		    layoutPtr[slot].maxOffset-layoutPtr[slot].minSize;
	}
    }


    /*
     * Step 6.
     * All of the space has been apportioned; copy the
     * layout information back into the master.
     */

    for (slot=0; slot < gridCount; slot++) {
        slotPtr[slot].offset = layoutPtr[slot].minOffset;
    }

    --layoutPtr;
    if (layoutPtr != layoutData) {
	Tcl_Free((char *)layoutPtr);
    }
    return requiredSize;
}

/*
 *--------------------------------------------------------------
 *
 * GetGrid --
 *
 *	This internal procedure is used to locate a Grid
 *	structure for a given window, creating one if one
 *	doesn't exist already.
 *
 * Results:
 *	The return value is a pointer to the Grid structure
 *	corresponding to tkwin.
 *
 * Side effects:
 *	A new grid structure may be created.  If so, then
 *	a callback is set up to clean things up when the
 *	window is deleted.
 *
 *--------------------------------------------------------------
 */

static Gridder *
GetGrid(tkwin)
    Tk_Window tkwin;		/* Token for window for which
				 * grid structure is desired. */
{
    register Gridder *gridPtr;
    Tcl_HashEntry *hPtr;
    int new;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (!dispPtr->gridInit) {
	Tcl_InitHashTable(&dispPtr->gridHashTable, TCL_ONE_WORD_KEYS);
	dispPtr->gridInit = 1;
    }

    /*
     * See if there's already grid for this window.  If not,
     * then create a new one.
     */

    hPtr = Tcl_CreateHashEntry(&dispPtr->gridHashTable, (char *) tkwin, &new);
    if (!new) {
	return (Gridder *) Tcl_GetHashValue(hPtr);
    }
    gridPtr = (Gridder *) Tcl_Alloc(sizeof(Gridder));
    gridPtr->tkwin = tkwin;
    gridPtr->masterPtr = NULL;
    gridPtr->masterDataPtr = NULL;
    gridPtr->nextPtr = NULL;
    gridPtr->slavePtr = NULL;
    gridPtr->binNextPtr = NULL;

    gridPtr->column = gridPtr->row = -1;
    gridPtr->numCols = 1;
    gridPtr->numRows = 1;

    gridPtr->padX = gridPtr->padY = 0;
    gridPtr->iPadX = gridPtr->iPadY = 0;
    gridPtr->doubleBw = 2*Tk_Changes(tkwin)->border_width;
    gridPtr->abortPtr = NULL;
    gridPtr->flags = 0;
    gridPtr->sticky = 0;
    gridPtr->size = 0;
    gridPtr->masterDataPtr = NULL;
    Tcl_SetHashValue(hPtr, gridPtr);
    Tk_CreateEventHandler(tkwin, StructureNotifyMask,
	    GridStructureProc, (ClientData) gridPtr);
    return gridPtr;
}

/*
 *--------------------------------------------------------------
 *
 * SetGridSize --
 *
 *	This internal procedure sets the size of the grid occupied
 *	by slaves.
 *
 * Results:
 *	none
 *
 * Side effects:
 *	The width and height arguments are filled in the master data structure.
 *	Additional space is allocated for the constraints to accomodate
 *	the offsets.
 *
 *--------------------------------------------------------------
 */

static void
SetGridSize(masterPtr)
    Gridder *masterPtr;			/* The geometry master for this grid. */
{
    register Gridder *slavePtr;		/* Current slave window. */
    int maxX = 0, maxY = 0;

    for (slavePtr = masterPtr->slavePtr; slavePtr != NULL;
		slavePtr = slavePtr->nextPtr) {
	maxX = MAX(maxX,slavePtr->numCols + slavePtr->column);
	maxY = MAX(maxY,slavePtr->numRows + slavePtr->row);
    }
    masterPtr->masterDataPtr->columnEnd = maxX;
    masterPtr->masterDataPtr->rowEnd = maxY;
    CheckSlotData(masterPtr, maxX, COLUMN, CHECK_SPACE);
    CheckSlotData(masterPtr, maxY, ROW, CHECK_SPACE);
}

/*
 *--------------------------------------------------------------
 *
 * CheckSlotData --
 *
 *	This internal procedure is used to manage the storage for
 *	row and column (slot) constraints.
 *
 * Results:
 *	TRUE if the index is OK, False otherwise.
 *
 * Side effects:
 *	A new master grid structure may be created.  If so, then
 *	it is initialized.  In addition, additional storage for
 *	a row or column constraints may be allocated, and the constraint
 *	maximums are adjusted.
 *
 *--------------------------------------------------------------
 */

static int
CheckSlotData(masterPtr, slot, slotType, checkOnly)
    Gridder *masterPtr;	/* the geometry master for this grid */
    int slot;		/* which slot to look at */
    int slotType;	/* ROW or COLUMN */
    int checkOnly;	/* don't allocate new space if true */
{
    int numSlot;        /* number of slots already allocated (Space) */
    int end;	        /* last used constraint */

    /*
     * If slot is out of bounds, return immediately.
     */

    if (slot < 0 || slot >= MAX_ELEMENT) {
	return TCL_ERROR;
    }

    if ((checkOnly == CHECK_ONLY) && (masterPtr->masterDataPtr == NULL)) {
	return TCL_ERROR;
    }

    /*
     * If we need to allocate more space, allocate a little extra to avoid
     * repeated re-alloc's for large tables.  We need enough space to
     * hold all of the offsets as well.
     */

    InitMasterData(masterPtr);
    end = (slotType == ROW) ? masterPtr->masterDataPtr->rowMax :
	    masterPtr->masterDataPtr->columnMax;
    if (checkOnly == CHECK_ONLY) {
    	return  (end < slot) ? TCL_ERROR : TCL_OK;
    } else {
    	numSlot = (slotType == ROW) ? masterPtr->masterDataPtr->rowSpace 
	                            : masterPtr->masterDataPtr->columnSpace;
    	if (slot >= numSlot) {
	    int      newNumSlot = slot + PREALLOC ;
	    size_t   oldSize = numSlot    * sizeof(SlotInfo) ;
	    size_t   newSize = newNumSlot * sizeof(SlotInfo) ;
	    SlotInfo *new = (SlotInfo *) Tcl_Alloc(newSize);
	    SlotInfo *old = (slotType == ROW) ?
		    masterPtr->masterDataPtr->rowPtr :
		    masterPtr->masterDataPtr->columnPtr;
	    memcpy((VOID *) new, (VOID *) old, oldSize );
	    memset((VOID *) (new+numSlot), 0, newSize - oldSize );
	    Tcl_Free((char *) old);
	    if (slotType == ROW) {
	 	masterPtr->masterDataPtr->rowPtr = new ;
	    	masterPtr->masterDataPtr->rowSpace = newNumSlot ;
	    } else {
	    	masterPtr->masterDataPtr->columnPtr = new;
	    	masterPtr->masterDataPtr->columnSpace = newNumSlot ;
	    }
	}
	if (slot >= end && checkOnly != CHECK_SPACE) {
	    if (slotType == ROW) {
		masterPtr->masterDataPtr->rowMax = slot+1;
	    } else {
		masterPtr->masterDataPtr->columnMax = slot+1;
	    }
	}
    	return TCL_OK;
    }
}

/*
 *--------------------------------------------------------------
 *
 * InitMasterData --
 *
 *	This internal procedure is used to allocate and initialize
 *	the data for a geometry master, if the data
 *	doesn't exist already.
 *
 * Results:
 *	none
 *
 * Side effects:
 *	A new master grid structure may be created.  If so, then
 *	it is initialized.
 *
 *--------------------------------------------------------------
 */

static void
InitMasterData(masterPtr)
    Gridder *masterPtr;
{
    size_t size;
    if (masterPtr->masterDataPtr == NULL) {
	GridMaster *gridPtr = masterPtr->masterDataPtr =
		(GridMaster *) Tcl_Alloc(sizeof(GridMaster));
	size = sizeof(SlotInfo) * TYPICAL_SIZE;

	gridPtr->columnEnd = 0;
	gridPtr->columnMax = 0;
	gridPtr->columnPtr = (SlotInfo *) Tcl_Alloc(size);
	gridPtr->columnSpace = 0;
	gridPtr->columnSpace = TYPICAL_SIZE;
	gridPtr->rowEnd = 0;
	gridPtr->rowMax = 0;
	gridPtr->rowPtr = (SlotInfo *) Tcl_Alloc(size);
	gridPtr->rowSpace = 0;
	gridPtr->rowSpace = TYPICAL_SIZE;

	memset((VOID *) gridPtr->columnPtr, 0, size);
	memset((VOID *) gridPtr->rowPtr, 0, size);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Unlink --
 *
 *	Remove a grid from its parent's list of slaves.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The parent will be scheduled for re-arranging, and the size of the
 *	grid will be adjusted accordingly
 *
 *----------------------------------------------------------------------
 */

static void
Unlink(slavePtr)
    register Gridder *slavePtr;		/* Window to unlink. */
{
    register Gridder *masterPtr, *slavePtr2;
    GridMaster *gridPtr;	/* pointer to grid data */

    masterPtr = slavePtr->masterPtr;
    if (masterPtr == NULL) {
	return;
    }

    gridPtr = masterPtr->masterDataPtr;
    if (masterPtr->slavePtr == slavePtr) {
	masterPtr->slavePtr = slavePtr->nextPtr;
    }
    else {
	for (slavePtr2 = masterPtr->slavePtr; ; slavePtr2 = slavePtr2->nextPtr) {
	    if (slavePtr2 == NULL) {
		panic("Unlink couldn't find previous window");
	    }
	    if (slavePtr2->nextPtr == slavePtr) {
		slavePtr2->nextPtr = slavePtr->nextPtr;
		break;
	    }
	}
    }
    if (!(masterPtr->flags & REQUESTED_RELAYOUT)) {
	masterPtr->flags |= REQUESTED_RELAYOUT;
	Tcl_DoWhenIdle(ArrangeGrid, (ClientData) masterPtr);
    }
    if (masterPtr->abortPtr != NULL) {
	*masterPtr->abortPtr = 1;
    }

    if ((slavePtr->numCols+slavePtr->column == gridPtr->columnMax)
	    || (slavePtr->numRows+slavePtr->row == gridPtr->rowMax)) {
    }
    slavePtr->masterPtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DestroyGrid --
 *
 *	This procedure is invoked by Tcl_EventuallyFree or Tcl_Release
 *	to clean up the internal structure of a grid at a safe time
 *	(when no-one is using it anymore).   Cleaning up the grid involves
 *	freeing the main structure for all windows. and the master structure
 *	for geometry managers.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Everything associated with the grid is freed up.
 *
 *----------------------------------------------------------------------
 */

static void
DestroyGrid(memPtr)
    char *memPtr;		/* Info about window that is now dead. */
{
    register Gridder *gridPtr = (Gridder *) memPtr;

    if (gridPtr->masterDataPtr != NULL) {
	if (gridPtr->masterDataPtr->rowPtr != NULL) {
	    Tcl_Free((char *) gridPtr->masterDataPtr -> rowPtr);
	}
	if (gridPtr->masterDataPtr->columnPtr != NULL) {
	    Tcl_Free((char *) gridPtr->masterDataPtr -> columnPtr);
	}
	Tcl_Free((char *) gridPtr->masterDataPtr);
    }
    Tcl_Free((char *) gridPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * GridStructureProc --
 *
 *	This procedure is invoked by the Tk event dispatcher in response
 *	to StructureNotify events.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If a window was just deleted, clean up all its grid-related
 *	information.  If it was just resized, re-configure its slaves, if
 *	any.
 *
 *----------------------------------------------------------------------
 */

static void
GridStructureProc(clientData, eventPtr)
    ClientData clientData;		/* Our information about window
					 * referred to by eventPtr. */
    XEvent *eventPtr;			/* Describes what just happened. */
{
    register Gridder *gridPtr = (Gridder *) clientData;
    TkDisplay *dispPtr = ((TkWindow *) gridPtr->tkwin)->dispPtr;

    if (eventPtr->type == ConfigureNotify) {
	if (!(gridPtr->flags & REQUESTED_RELAYOUT)) {
	    gridPtr->flags |= REQUESTED_RELAYOUT;
	    Tcl_DoWhenIdle(ArrangeGrid, (ClientData) gridPtr);
	}
	if (gridPtr->doubleBw != 2*Tk_Changes(gridPtr->tkwin)->border_width) {
	    if ((gridPtr->masterPtr != NULL) &&
		    !(gridPtr->masterPtr->flags & REQUESTED_RELAYOUT)) {
		gridPtr->doubleBw = 2*Tk_Changes(gridPtr->tkwin)->border_width;
		gridPtr->masterPtr->flags |= REQUESTED_RELAYOUT;
		Tcl_DoWhenIdle(ArrangeGrid, (ClientData) gridPtr->masterPtr);
	    }
	}
    } else if (eventPtr->type == DestroyNotify) {
	register Gridder *gridPtr2, *nextPtr;

	if (gridPtr->masterPtr != NULL) {
	    Unlink(gridPtr);
	}
	for (gridPtr2 = gridPtr->slavePtr; gridPtr2 != NULL;
					   gridPtr2 = nextPtr) {
	    Tk_UnmapWindow(gridPtr2->tkwin);
	    gridPtr2->masterPtr = NULL;
	    nextPtr = gridPtr2->nextPtr;
	    gridPtr2->nextPtr = NULL;
	}
	Tcl_DeleteHashEntry(Tcl_FindHashEntry(&dispPtr->gridHashTable,
		(char *) gridPtr->tkwin));
	if (gridPtr->flags & REQUESTED_RELAYOUT) {
	    Tcl_CancelIdleCall(ArrangeGrid, (ClientData) gridPtr);
	}
	gridPtr->tkwin = NULL;
	Tcl_EventuallyFree((ClientData) gridPtr, DestroyGrid);
    } else if (eventPtr->type == MapNotify) {
	if (!(gridPtr->flags & REQUESTED_RELAYOUT)) {
	    gridPtr->flags |= REQUESTED_RELAYOUT;
	    Tcl_DoWhenIdle(ArrangeGrid, (ClientData) gridPtr);
	}
    } else if (eventPtr->type == UnmapNotify) {
	register Gridder *gridPtr2;

	for (gridPtr2 = gridPtr->slavePtr; gridPtr2 != NULL;
					   gridPtr2 = gridPtr2->nextPtr) {
	    Tk_UnmapWindow(gridPtr2->tkwin);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ConfigureSlaves --
 *
 *	This implements the guts of the "grid configure" command.  Given
 *	a list of slaves and configuration options, it arranges for the
 *	grid to manage the slaves and sets the specified options.
 *	arguments consist of windows or window shortcuts followed by
 *	"-option value" pairs.
 *
 * Results:
 *	TCL_OK is returned if all went well.  Otherwise, TCL_ERROR is
 *	returned and the interp's result is set to contain an error message.
 *
 * Side effects:
 *	Slave windows get taken over by the grid.
 *
 *----------------------------------------------------------------------
 */

static int
ConfigureSlaves(interp, tkwin, argc, argv)
    Tcl_Interp *interp;		/* Interpreter for error reporting. */
    Tk_Window tkwin;		/* Any window in application containing
				 * slaves.  Used to look up slave names. */
    int argc;			/* Number of elements in argv. */
    char *argv[];		/* Argument strings:  contains one or more
				 * window names followed by any number
				 * of "option value" pairs.  Caller must
				 * make sure that there is at least one
				 * window name. */
{
    Gridder *masterPtr;
    Gridder *slavePtr;
    Tk_Window other, slave, parent, ancestor;
    int i, j, c, tmp;
    size_t length;
    int numWindows;
    int width;
    int defaultColumn = 0;	/* default column number */
    int defaultColumnSpan = 1;	/* default number of columns */
    char *lastWindow;		/* use this window to base current
				 * Row/col on */

    /*
     * Count the number of windows, or window short-cuts.
     */

    for(numWindows=i=0;i<argc;i++) {
    	char firstChar = *argv[i];
	if (firstChar == '.') {
	    numWindows++;
	    continue;
    	}
    	length = strlen(argv[i]);
    	if (length > 1 && firstChar == '-') {
	    break;
	}
	if (length > 1) {
	    Tcl_AppendResult(interp, "unexpected parameter, \"",
		    argv[i], "\", in configure list. ",
		    "Should be window name or option", (char *) NULL);
	    return TCL_ERROR;
	}

	if ((firstChar == REL_HORIZ) && ((numWindows == 0) ||
		(*argv[i-1] == REL_SKIP) || (*argv[i-1] == REL_VERT))) {
	    Tcl_AppendResult(interp,
		    "Must specify window before shortcut '-'.",
		    (char *) NULL);
	    return TCL_ERROR;
	}

	if ((firstChar == REL_VERT) || (firstChar == REL_SKIP)
		|| (firstChar == REL_HORIZ)) {
	    continue;
	}

	Tcl_AppendResult(interp, "invalid window shortcut, \"",
		argv[i], "\" should be '-', 'x', or '^'", (char *) NULL);
	return TCL_ERROR;
    }
    numWindows = i;

    if ((argc-numWindows)&1) {
	Tcl_AppendResult(interp, "extra option or",
		" option with no value", (char *) NULL);
	return TCL_ERROR;
    }

    /*
     * Iterate over all of the slave windows and short-cuts, parsing
     * options for each slave.  It's a bit wasteful to re-parse the
     * options for each slave, but things get too messy if we try to
     * parse the arguments just once at the beginning.  For example,
     * if a slave already is managed we want to just change a few
     * existing values without resetting everything.  If there are
     * multiple windows, the -in option only gets processed for the
     * first window.
     */

    masterPtr = NULL;
    for (j = 0; j < numWindows; j++) {
    	char firstChar = *argv[j];

	/*
	 * '^' and 'x' cause us to skip a column.  '-' is processed
	 * as part of its preceeding slave.
	 */

	if ((firstChar == REL_VERT) || (firstChar == REL_SKIP)) {
	    defaultColumn++;
	    continue;
	}
	if (firstChar == REL_HORIZ) {
	    continue;
	}

	for (defaultColumnSpan=1;
		j + defaultColumnSpan < numWindows &&
		(*argv[j+defaultColumnSpan] == REL_HORIZ);
		defaultColumnSpan++) {
	    /* null body */
	}

	slave = Tk_NameToWindow(interp, argv[j], tkwin);
	if (slave == NULL) {
	    return TCL_ERROR;
	}
	if (Tk_IsTopLevel(slave)) {
	    Tcl_AppendResult(interp, "can't manage \"", argv[j],
		    "\": it's a top-level window", (char *) NULL);
	    return TCL_ERROR;
	}
	slavePtr = GetGrid(slave);

	/*
	 * The following statement is taken from tkPack.c:
	 *
	 * "If the slave isn't currently managed, reset all of its
	 * configuration information to default values (there could
	 * be old values left from a previous packer)."
	 *
	 * I [D.S.] disagree with this statement.  If a slave is disabled (using
	 * "forget") and then re-enabled, I submit that 90% of the time the
	 * programmer will want it to retain its old configuration information.
	 * If the programmer doesn't want this behavior, then the
	 * defaults can be reestablished by hand, without having to worry
	 * about keeping track of the old state. 
	 */

	for (i = numWindows; i < argc; i+=2) {
	    length = strlen(argv[i]);
	    c = argv[i][1];

	    if (length < 2) {
		Tcl_AppendResult(interp, "unknown or ambiguous option \"",
			argv[i], "\": must be ",
			"-column, -columnspan, -in, -ipadx, -ipady, ",
			"-padx, -pady, -row, -rowspan, or -sticky",
			(char *) NULL);
		return TCL_ERROR;
	    }
	    if ((c == 'c') && (strncmp(argv[i], "-column", length) == 0)) {
		if (Tcl_GetInt(interp, argv[i+1], &tmp) != TCL_OK || tmp<0) {
		    Tcl_ResetResult(interp);
		    Tcl_AppendResult(interp, "bad column value \"", argv[i+1],
			    "\": must be a non-negative integer", (char *)NULL);
		    return TCL_ERROR;
		}
		slavePtr->column = tmp;
	    } else if ((c == 'c')
		    && (strncmp(argv[i], "-columnspan", length) == 0)) {
		if (Tcl_GetInt(interp, argv[i+1], &tmp) != TCL_OK || tmp <= 0) {
		    Tcl_ResetResult(interp);
		    Tcl_AppendResult(interp, "bad columnspan value \"", argv[i+1],
			    "\": must be a positive integer", (char *)NULL);
		    return TCL_ERROR;
		}
		slavePtr->numCols = tmp;
	    } else if ((c == 'i') && (strncmp(argv[i], "-in", length) == 0)) {
		other = Tk_NameToWindow(interp, argv[i+1], tkwin);
		if (other == NULL) {
		    return TCL_ERROR;
		}
		if (other == slave) {
		    Tcl_SetResult(interp, "Window can't be managed in itself",
			    TCL_STATIC);
		    return TCL_ERROR;
		}
		masterPtr = GetGrid(other);
		InitMasterData(masterPtr);
	    } else if ((c == 'i')
		    && (strncmp(argv[i], "-ipadx", length) == 0)) {
		if ((Tk_GetPixels(interp, slave, argv[i+1], &tmp) != TCL_OK)
			|| (tmp < 0)) {
		    Tcl_ResetResult(interp);
		    Tcl_AppendResult(interp, "bad ipadx value \"", argv[i+1],
			    "\": must be positive screen distance",
			    (char *) NULL);
		    return TCL_ERROR;
		}
		slavePtr->iPadX = tmp*2;
	    } else if ((c == 'i')
		    && (strncmp(argv[i], "-ipady", length) == 0)) {
		if ((Tk_GetPixels(interp, slave, argv[i+1], &tmp) != TCL_OK)
			|| (tmp< 0)) {
		    Tcl_ResetResult(interp);
		    Tcl_AppendResult(interp, "bad ipady value \"", argv[i+1],
			    "\": must be positive screen distance",
			    (char *) NULL);
		    return TCL_ERROR;
		}
		slavePtr->iPadY = tmp*2;
	    } else if ((c == 'p')
		    && (strncmp(argv[i], "-padx", length) == 0)) {
		if ((Tk_GetPixels(interp, slave, argv[i+1], &tmp) != TCL_OK)
			|| (tmp< 0)) {
		    Tcl_ResetResult(interp);
		    Tcl_AppendResult(interp, "bad padx value \"", argv[i+1],
			    "\": must be positive screen distance",
			    (char *) NULL);
		    return TCL_ERROR;
		}
		slavePtr->padX = tmp*2;
	    } else if ((c == 'p')
		    && (strncmp(argv[i], "-pady", length) == 0)) {
		if ((Tk_GetPixels(interp, slave, argv[i+1], &tmp) != TCL_OK)
			|| (tmp< 0)) {
		    Tcl_ResetResult(interp);
		    Tcl_AppendResult(interp, "bad pady value \"", argv[i+1],
			    "\": must be positive screen distance",
			    (char *) NULL);
		    return TCL_ERROR;
		}
		slavePtr->padY = tmp*2;
	    } else if ((c == 'r') && (strncmp(argv[i], "-row", length) == 0)) {
		if (Tcl_GetInt(interp, argv[i+1], &tmp) != TCL_OK || tmp<0) {
		    Tcl_ResetResult(interp);
		    Tcl_AppendResult(interp, "bad grid value \"", argv[i+1],
			    "\": must be a non-negative integer", (char *)NULL);
		    return TCL_ERROR;
		}
		slavePtr->row = tmp;
	    } else if ((c == 'r')
		    && (strncmp(argv[i], "-rowspan", length) == 0)) {
		if ((Tcl_GetInt(interp, argv[i+1], &tmp) != TCL_OK) || tmp<=0) {
		    Tcl_ResetResult(interp);
		    Tcl_AppendResult(interp, "bad rowspan value \"", argv[i+1],
			    "\": must be a positive integer", (char *)NULL);
		    return TCL_ERROR;
		}
		slavePtr->numRows = tmp;
	    } else if ((c == 's')
		    && strncmp(argv[i], "-sticky", length) == 0) {
		int sticky = StringToSticky(argv[i+1]);
		if (sticky == -1) {
		    Tcl_AppendResult(interp, "bad stickyness value \"", argv[i+1],
			    "\": must be a string containing n, e, s, and/or w",
			    (char *)NULL);
		    return TCL_ERROR;
		}
		slavePtr->sticky = sticky;
	    } else {
		Tcl_AppendResult(interp, "unknown or ambiguous option \"",
			argv[i], "\": must be ",
			"-column, -columnspan, -in, -ipadx, -ipady, ",
			"-padx, -pady, -row, -rowspan, or -sticky",
			(char *) NULL);
		return TCL_ERROR;
	    }
	}

	/*
	 * Make sure we have a geometry master.  We look at:
	 *  1)   the -in flag
	 *  2)   the geometry master of the first slave (if specified)
	 *  3)   the parent of the first slave.
	 */
    
    	if (masterPtr == NULL) {
	    masterPtr = slavePtr->masterPtr;
    	}
	parent = Tk_Parent(slave);
    	if (masterPtr == NULL) {
	    masterPtr = GetGrid(parent);
	    InitMasterData(masterPtr);
    	}

	if (slavePtr->masterPtr != NULL && slavePtr->masterPtr != masterPtr) {
	    Unlink(slavePtr);
	    slavePtr->masterPtr = NULL;
	}

	if (slavePtr->masterPtr == NULL) {
	    Gridder *tempPtr = masterPtr->slavePtr;
	    slavePtr->masterPtr = masterPtr;
	    masterPtr->slavePtr = slavePtr;
	    slavePtr->nextPtr = tempPtr;
	}

	/*
	 * Make sure that the slave's parent is either the master or
	 * an ancestor of the master, and that the master and slave
	 * aren't the same.
	 */

	for (ancestor = masterPtr->tkwin; ; ancestor = Tk_Parent(ancestor)) {
	    if (ancestor == parent) {
		break;
	    }
	    if (Tk_IsTopLevel(ancestor)) {
		Tcl_AppendResult(interp, "can't put ", argv[j],
			" inside ", Tk_PathName(masterPtr->tkwin),
			(char *) NULL);
		Unlink(slavePtr);
		return TCL_ERROR;
	    }
	}

	/*
	 * Try to make sure our master isn't managed by us.
	 */

     	if (masterPtr->masterPtr == slavePtr) {
	    Tcl_AppendResult(interp, "can't put ", argv[j],
		    " inside ", Tk_PathName(masterPtr->tkwin),
		    ", would cause management loop.", 
		    (char *) NULL);
	    Unlink(slavePtr);
	    return TCL_ERROR;
     	}

	Tk_ManageGeometry(slave, &gridMgrType, (ClientData) slavePtr);

	/*
	 * Assign default position information.
	 */

	if (slavePtr->column == -1) {
	    slavePtr->column = defaultColumn;
	}
	slavePtr->numCols += defaultColumnSpan - 1;
	if (slavePtr->row == -1) {
	    if (masterPtr->masterDataPtr == NULL) {
	    	slavePtr->row = 0;
	    } else {
	    	slavePtr->row = masterPtr->masterDataPtr->rowEnd;
	    }
	}
	defaultColumn += slavePtr->numCols;
	defaultColumnSpan = 1;

	/*
	 * Arrange for the parent to be re-arranged at the first
	 * idle moment.
	 */

	if (masterPtr->abortPtr != NULL) {
	    *masterPtr->abortPtr = 1;
	}
	if (!(masterPtr->flags & REQUESTED_RELAYOUT)) {
	    masterPtr->flags |= REQUESTED_RELAYOUT;
	    Tcl_DoWhenIdle(ArrangeGrid, (ClientData) masterPtr);
	}
    }

    /* Now look for all the "^"'s. */

    lastWindow = NULL;
    for (j = 0; j < numWindows; j++) {
	struct Gridder *otherPtr;
	int match;			/* found a match for the ^ */
	int lastRow, lastColumn;		/* implied end of table */

    	if (*argv[j] == '.') {
	    lastWindow = argv[j];
	}
	if (*argv[j] != REL_VERT) {
	    continue;
	}

	if (masterPtr == NULL) {
	    Tcl_AppendResult(interp, "can't use '^', cant find master",
		    (char *) NULL);
	    return TCL_ERROR;
	}

	for (width=1; width+j < numWindows && *argv[j+width] == REL_VERT;
		width++) {
	    /* Null Body */
	}

	/*
	 * Find the implied grid location of the ^
	 */

	if (lastWindow == NULL) { 
	    if (masterPtr->masterDataPtr != NULL) {
		SetGridSize(masterPtr);
		lastRow = masterPtr->masterDataPtr->rowEnd - 1;
	    } else {
		lastRow = 0;
	    }
	    lastColumn = 0;
	} else {
	    other = Tk_NameToWindow(interp, lastWindow, tkwin);
	    otherPtr = GetGrid(other);
	    lastRow = otherPtr->row;
	    lastColumn = otherPtr->column + otherPtr->numCols;
	}

	for (match=0, slavePtr = masterPtr->slavePtr; slavePtr != NULL;
					 slavePtr = slavePtr->nextPtr) {

	    if (slavePtr->numCols == width
		    && slavePtr->column == lastColumn
		    && slavePtr->row + slavePtr->numRows == lastRow) {
		slavePtr->numRows++;
		match++;
	    }
	    lastWindow = Tk_PathName(slavePtr->tkwin);
	}
	if (!match) {
	    Tcl_AppendResult(interp, "can't find slave to extend with \"^\".",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	j += width - 1;
    }

    if (masterPtr == NULL) {
	Tcl_AppendResult(interp, "can't determine master window",
		(char *) NULL);
	return TCL_ERROR;
    }
    SetGridSize(masterPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * StickyToString
 *
 *	Converts the internal boolean combination of "sticky" bits onto
 *	a TCL list element containing zero or mor of n, s, e, or w.
 *
 * Results:
 *	A string is placed into the "result" pointer.
 *
 * Side effects:
 *	none.
 *
 *----------------------------------------------------------------------
 */

static void
StickyToString(flags, result)
    int flags;		/* the sticky flags */
    char *result;	/* where to put the result */
{
    int count = 0;
    if (flags&STICK_NORTH) {
    	result[count++] = 'n';
    }
    if (flags&STICK_EAST) {
    	result[count++] = 'e';
    }
    if (flags&STICK_SOUTH) {
    	result[count++] = 's';
    }
    if (flags&STICK_WEST) {
    	result[count++] = 'w';
    }
    if (count) {
	result[count] = '\0';
    } else {
	sprintf(result,"{}");
    }
}

/*
 *----------------------------------------------------------------------
 *
 * StringToSticky --
 *
 *	Converts an ascii string representing a widgets stickyness
 *	into the boolean result.
 *
 * Results:
 *	The boolean combination of the "sticky" bits is retuned.  If an
 *	error occurs, such as an invalid character, -1 is returned instead.
 *
 * Side effects:
 *	none
 *
 *----------------------------------------------------------------------
 */

static int
StringToSticky(string)
    char *string;
{
    int sticky = 0;
    char c;

    while ((c = *string++) != '\0') {
	switch (c) {
	    case 'n': case 'N': sticky |= STICK_NORTH; break;
	    case 'e': case 'E': sticky |= STICK_EAST;  break;
	    case 's': case 'S': sticky |= STICK_SOUTH; break;
	    case 'w': case 'W': sticky |= STICK_WEST;  break;
	    case ' ': case ',': case '\t': case '\r': case '\n': break;
	    default: return -1;
	}
    }
    return sticky;
}		
