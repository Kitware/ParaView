/* 
 * tkMacMenu.c --
 *
 *	This module implements the Mac-platform specific features of menus.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkMacInt.h"
#include "tkMenuButton.h"
#include "tkMenu.h"
#include "tkColor.h"
#include "tkMacInt.h"
#undef Status
#include <Menus.h>
#include <OSUtils.h>
#include <Palettes.h>
#include <Resources.h>
#include <string.h>
#include <ToolUtils.h>
#include <Balloons.h>
#include <Appearance.h>
#include <Devices.h>

typedef struct MacMenu {
    MenuHandle menuHdl;		/* The Menu Manager data structure. */
    Rect menuRect;		/* The rectangle as calculated in the
    				 * MDEF. This is used to figure ou the
    				 * clipping rgn before we push
    				 * the <<MenuSelect>> virtual binding
    				 * through. */
} MacMenu;

typedef struct MenuEntryUserData {
    Drawable mdefDrawable;
    TkMenuEntry *mePtr;
    Tk_Font tkfont;
    Tk_FontMetrics *fmPtr;
} MenuEntryUserData;
/*
 * Various geometry definitions:
 */

#define CASCADE_ARROW_HEIGHT 	10
#define CASCADE_ARROW_WIDTH 	8
#define DECORATION_BORDER_WIDTH 2
#define MAC_MARGIN_WIDTH 	8

/*
 * The following are constants relating to the SICNs used for drawing the MDEF.
 */

#define SICN_RESOURCE_NUMBER	128

#define SICN_HEIGHT 		16
#define SICN_ROWS 		2
#define CASCADE_ICON_WIDTH	7
#define	SHIFT_ICON_WIDTH	10
#define	OPTION_ICON_WIDTH	16
#define CONTROL_ICON_WIDTH	12
#define COMMAND_ICON_WIDTH	10

#define CASCADE_ARROW		0
#define SHIFT_ICON		1
#define OPTION_ICON		2
#define CONTROL_ICON		3
#define COMMAND_ICON		4
#define DOWN_ARROW		5
#define UP_ARROW		6

/*
 * Platform specific flags for menu entries
 *
 * ENTRY_COMMAND_ACCEL		Indicates the entry has the command key
 *				in its accelerator string.
 * ENTRY_OPTION_ACCEL		Indicates the entry has the option key
 *				in its accelerator string.
 * ENTRY_SHIFT_ACCEL		Indicates the entry has the shift key
 *				in its accelerator string.
 * ENTRY_CONTROL_ACCEL		Indicates the entry has the control key
 *				in its accelerator string.
 */

#define ENTRY_COMMAND_ACCEL	ENTRY_PLATFORM_FLAG1
#define ENTRY_OPTION_ACCEL	ENTRY_PLATFORM_FLAG2
#define ENTRY_SHIFT_ACCEL	ENTRY_PLATFORM_FLAG3
#define ENTRY_CONTROL_ACCEL	ENTRY_PLATFORM_FLAG4
#define ENTRY_ACCEL_MASK	(ENTRY_COMMAND_ACCEL | ENTRY_OPTION_ACCEL \
				| ENTRY_SHIFT_ACCEL | ENTRY_CONTROL_ACCEL)

/*
 * This structure is used to keep track of subfields within Macintosh menu
 * items.
 */

typedef struct EntryGeometry {
    int accelTextStart;		/* Offset into the accel string where
    				 * the text starts. Everything before
    				 * this is modifier key descriptions.
    				 */
    int modifierWidth;		/* Width of modifier symbols. */
    int accelTextWidth;		/* Width of the text after the modifier 
    				 * keys. */
    int nonAccelMargin;		/* The width of the margin for entries
    				 * without accelerators. */
} EntryGeometry;

/*
 * Structure to keep track of toplevel windows and their menubars.
 */

typedef struct TopLevelMenubarList {
    struct TopLevelMenubarList *nextPtr;
    				/* The next window in the list. */
    Tk_Window tkwin;		/* The toplevel window. */
    TkMenu *menuPtr;		/* The menu associated with this
    				 * toplevel. */
} TopLevelMenubarList;

/*
 * Platform-specific flags for menus.
 *
 * MENU_APPLE_MENU		0 indicates a custom Apple menu has
 *				not been installed; 1 a custom Apple
 *				menu has been installed.
 * MENU_HELP_MENU		0 indicates a custom Help menu has
 *				not been installed; 1 a custom Help
 *				menu has been installed.
 * MENU_RECONFIGURE_PENDING	1 indicates that an idle handler has
 *				been scheduled to reconfigure the
 *				Macintosh MenuHandle.
 */

#define MENU_APPLE_MENU			MENU_PLATFORM_FLAG1
#define MENU_HELP_MENU			MENU_PLATFORM_FLAG2
#define MENU_RECONFIGURE_PENDING	MENU_PLATFORM_FLAG3

#define CASCADE_CMD (0x1b)    	
				/* The special command char for cascade
			         * menus. */
#define SEPARATOR_TEXT "\p(-"
				/* The text for a menu separator. */

#define MENUBAR_REDRAW_PENDING 1

static int gNoTkMenus = 0;      /* This is used by Tk_MacTurnOffMenus as the
                                 * flag that Tk is not to draw any menus. */
RgnHandle tkMenuCascadeRgn = NULL;
				/* The region to clip drawing to when the
				 * MDEF is up. */
int tkUseMenuCascadeRgn = 0;	/* If this is 1, clipping code
				 * should intersect tkMenuCascadeRgn
				 * before drawing occurs.
				 * tkMenuCascadeRgn will only
				 * be valid when the value of this
				 * variable is 1. */

static Tcl_HashTable commandTable;
				/* The list of menuInstancePtrs associated with
				 * menu ids */
static short currentAppleMenuID;
				/* The id of the current Apple menu. 0 for
				 * none. */
static short currentHelpMenuID; /* The id of the current Help menu. 0 for
				 * none. */
static Tcl_Interp *currentMenuBarInterp;
				/* The interpreter of the window that owns
				 * the current menubar. */
static char *currentMenuBarName;
				/* Malloced. Name of current menu in menu bar.
				 * NULL if no menu set. TO DO: make this a
				 * DString. */
static Tk_Window currentMenuBarOwner;
				/* Which window owns the current menu bar. */
static char elipsisString[TCL_UTF_MAX + 1];
				/* The UTF representation of the elipsis (ƒ) 
				 * character. */
static int helpItemCount;	/* The number of items in the help menu. 
				 * -1 means that the help menu is
				 * unavailable. This does not include
				 * the automatically generated separator. */
static int inPostMenu;		/* We cannot be re-entrant like X
				 * windows. */
static short lastMenuID;	/* To pass to NewMenu; need to figure out
				 * a good way to do this. */
static unsigned char lastCascadeID;
				/* Cascades have to have ids that are
				 * less than 256. */
static MacDrawable macMDEFDrawable;
				/* Drawable for use by MDEF code */
static MDEFScrollFlag = 0;	/* Used so that popups don't scroll too soon. */
static int menuBarFlags;	/* Used for whether the menu bar needs
				 * redrawing or not. */
static TkMenuDefUPP menuDefProc = NULL ;
                                /* The routine descriptor to the MDEF proc.
				 * The MDEF is needed to draw menus with
				 * non-standard attributes and to support
				 * tearoff menus. */
static struct TearoffSelect {
    TkMenu *menuPtr;		/* The menu that is torn off */
    Point point;		/* The point to place the new menu */
    Rect excludeRect;		/* We don't want to drag tearoff highlights
    				 * when we are in this menu */
} tearoffStruct;

static RgnHandle totalMenuRgn = NULL;
				/* Used to update windows which have been
				 * obscured by menus. */
static RgnHandle utilRgn = NULL;/* Used when creating the region that is to
				 * be clipped out while the MDEF is active. */

static TopLevelMenubarList *windowListPtr;
				/* A list of windows that have menubars set. */
static MenuItemDrawingUPP tkThemeMenuItemDrawingUPP; 
				/* Points to the UPP for theme Item drawing. */

				
/*
 * Forward declarations for procedures defined later in this file:
 */
 
static void		CompleteIdlers _ANSI_ARGS_((TkMenu *menuPtr));
static void		DrawMenuBarWhenIdle _ANSI_ARGS_((
			    ClientData clientData));
static void 		DrawMenuBackground _ANSI_ARGS_((
    			    Rect *menuRectPtr, Drawable d, ThemeMenuType type));
static void		DrawMenuEntryAccelerator _ANSI_ARGS_((
			    TkMenu *menuPtr, TkMenuEntry *mePtr, 
			    Drawable d, GC gc, Tk_Font tkfont,
			    CONST Tk_FontMetrics *fmPtr,
			    Tk_3DBorder activeBorder, int x, int y,
			    int width, int height, int drawArrow));
static void		DrawMenuEntryBackground _ANSI_ARGS_((
			    TkMenu *menuPtr, TkMenuEntry *mePtr,
			    Drawable d, Tk_3DBorder activeBorder,
			    Tk_3DBorder bgBorder, int x, int y,
			    int width, int heigth));
static void		DrawMenuEntryIndicator _ANSI_ARGS_((
			    TkMenu *menuPtr, TkMenuEntry *mePtr,
			    Drawable d, GC gc, GC indicatorGC, 
			    Tk_Font tkfont,
			    CONST Tk_FontMetrics *fmPtr, int x, int y,
			    int width, int height));
static void		DrawMenuEntryLabel _ANSI_ARGS_((
			    TkMenu * menuPtr, TkMenuEntry *mePtr, Drawable d,
			    GC gc, Tk_Font tkfont,
			    CONST Tk_FontMetrics *fmPtr, int x, int y,
			    int width, int height));
static void		DrawMenuSeparator _ANSI_ARGS_((TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Drawable d, GC gc, 
			    Tk_Font tkfont, CONST Tk_FontMetrics *fmPtr, 
			    int x, int y, int width, int height));
static void		DrawTearoffEntry _ANSI_ARGS_((TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Drawable d, GC gc, 
			    Tk_Font tkfont, CONST Tk_FontMetrics *fmPtr, 
			    int x, int y, int width, int height));
static Handle		FixMDEF _ANSI_ARGS_((void));
static void		GetEntryText _ANSI_ARGS_((TkMenuEntry *mePtr,
			    Tcl_DString *dStringPtr));
static void		GetMenuAccelGeometry _ANSI_ARGS_((TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Tk_Font tkfont,
			    CONST Tk_FontMetrics *fmPtr, int *modWidthPtr,
			    int *textWidthPtr, int *heightPtr));
static void		GetMenuLabelGeometry _ANSI_ARGS_((TkMenuEntry *mePtr,
			    Tk_Font tkfont, CONST Tk_FontMetrics *fmPtr,
			    int *widthPtr, int *heightPtr));
static void		GetMenuIndicatorGeometry _ANSI_ARGS_((
			    TkMenu *menuPtr, TkMenuEntry *mePtr, 
			    Tk_Font tkfont, CONST Tk_FontMetrics *fmPtr, 
			    int *widthPtr, int *heightPtr));
static void		GetMenuSeparatorGeometry _ANSI_ARGS_((
			    TkMenu *menuPtr, TkMenuEntry *mePtr,
			    Tk_Font tkfont, CONST Tk_FontMetrics *fmPtr,
			    int *widthPtr, int *heightPtr));
static void		GetTearoffEntryGeometry _ANSI_ARGS_((TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Tk_Font tkfont,
			    CONST Tk_FontMetrics *fmPtr, int *widthPtr,
			    int *heightPtr));
static int		GetNewID _ANSI_ARGS_((Tcl_Interp *interp,
			    TkMenu *menuInstPtr, int cascade, 
			    short *menuIDPtr));
static char		FindMarkCharacter _ANSI_ARGS_((TkMenuEntry *mePtr));
static void		FreeID _ANSI_ARGS_((short menuID));
static void		InvalidateMDEFRgns _ANSI_ARGS_((void));
static void		MenuDefProc _ANSI_ARGS_((short message,
			    MenuHandle menu, Rect *menuRectPtr,
			    Point hitPt, short *whichItem,
			    TkMenuLowMemGlobals *globalsPtr));
static void		MenuSelectEvent _ANSI_ARGS_((TkMenu *menuPtr));
static void		ReconfigureIndividualMenu _ANSI_ARGS_((
    			    TkMenu *menuPtr, MenuHandle macMenuHdl, 
    			    int base));
static void		ReconfigureMacintoshMenu _ANSI_ARGS_ ((
			    ClientData clientData));
static void		RecursivelyClearActiveMenu _ANSI_ARGS_((
			    TkMenu *menuPtr));
static void		RecursivelyDeleteMenu _ANSI_ARGS_((
			    TkMenu *menuPtr));
static void		RecursivelyInsertMenu _ANSI_ARGS_((
			    TkMenu *menuPtr));
static void		SetDefaultMenubar _ANSI_ARGS_((void));
static int		SetMenuCascade _ANSI_ARGS_((TkMenu *menuPtr));
static void		SetMenuIndicator _ANSI_ARGS_((TkMenuEntry *mePtr));
static void		SetMenuTitle _ANSI_ARGS_((MenuHandle menuHdl,
			    Tcl_Obj *titlePtr));
static void		AppearanceEntryDrawWrapper _ANSI_ARGS_((TkMenuEntry *mePtr, 
			    Rect * menuRectPtr, TkMenuLowMemGlobals *globalsPtr,     
			    Drawable d, Tk_FontMetrics *fmPtr, Tk_Font tkfont,
			    int x, int y, int width, int height));
pascal void 		tkThemeMenuItemDrawingProc _ANSI_ARGS_ ((const Rect *inBounds,
			    SInt16 inDepth, Boolean inIsColorDevice, 
			    SInt32 inUserData));


/*
 *----------------------------------------------------------------------
 *
 * TkMacUseID --
 *
 *	Take the ID out of the available list for new menus. Used by the
 *	default menu bar's menus so that they do not get created at the tk
 *	level. See GetNewID for more information.
 *
 * Results:
 *	Returns TCL_OK if the id was not in use. Returns TCL_ERROR if the
 *	id was in use.
 *
 * Side effects:
 *	A hash table entry in the command table is created with a NULL
 *	value.
 *
 *----------------------------------------------------------------------
 */

int
TkMacUseMenuID(
    short macID)		/* The id to take out of the table */
{
    Tcl_HashEntry *commandEntryPtr;
    int newEntry;
    
    TkMenuInit();
    commandEntryPtr = Tcl_CreateHashEntry(&commandTable, (char *) macID,
    	    &newEntry);
    if (newEntry == 1) {
    	Tcl_SetHashValue(commandEntryPtr, NULL);
    	return TCL_OK;
    } else {
    	return TCL_ERROR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetNewID --
 *
 *	Allocates a new menu id and marks it in use. Each menu on the
 *	mac must be designated by a unique id, which is a short. In
 *	addition, some ids are reserved by the system. Since Tk uses
 *	mostly dynamic menus, we must allocate and free these ids on
 *	the fly. We use the id as a key into a hash table; if there
 *	is no hash entry, we know that we can use the id.
 *
 * Results:
 *	Returns TCL_OK if succesful; TCL_ERROR if there are no more
 *	ids of the appropriate type to allocate. menuIDPtr contains
 *	the new id if succesful.
 *
 * Side effects:
 *	An entry is created for the menu in the command hash table,
 *	and the hash entry is stored in the appropriate field in the
 *	menu data structure.
 *
 *----------------------------------------------------------------------
 */

static int
GetNewID(
    Tcl_Interp *interp,		/* Used for error reporting */
    TkMenu *menuPtr,		/* The menu we are working with */
    int cascade,		/* 0 if we are working with a normal menu;
    				   1 if we are working with a cascade */
    short *menuIDPtr)		/* The resulting id */
{
    int found = 0;
    int newEntry;
    Tcl_HashEntry *commandEntryPtr;
    short returnID = *menuIDPtr;

    /*
     * The following code relies on shorts and unsigned chars wrapping
     * when the highest value is incremented. Also, the values between
     * 236 and 255 inclusive are reserved for DA's by the Mac OS.
     */
    
    if (!cascade) {
    	short curID = lastMenuID + 1;
        if (curID == 236) {
    	    curID = 256;
    	}

    	while (curID != lastMenuID) {
    	    commandEntryPtr = Tcl_CreateHashEntry(&commandTable,
		    (char *) curID, &newEntry);
    	    if (newEntry == 1) {
    	        found = 1;
    	        lastMenuID = returnID = curID;
    	        break;
    	    }
    	    curID++;
    	    if (curID == 236) {
    	    	curID = 256;
    	    }
    	}
    } else {
    
    	/*
    	 * Cascade ids must be between 0 and 235 only, so they must be
    	 * dealt with separately.
    	 */
    
    	unsigned char curID = lastCascadeID + 1;
        if (curID == 236) {
    	    curID = 0;
    	}
    	
    	while (curID != lastCascadeID) {
    	    commandEntryPtr = Tcl_CreateHashEntry(&commandTable,
		    (char *) curID, &newEntry);
    	    if (newEntry == 1) {
    	    	found = 1;
    	    	lastCascadeID = returnID = curID;
    	    	break;
    	    }
    	    curID++;
    	    if (curID == 236) {
    	    	curID = 0;
    	    }
    	}
    }

    if (found) {
    	Tcl_SetHashValue(commandEntryPtr, (char *) menuPtr);
    	*menuIDPtr = returnID;
    	return TCL_OK;
    } else {
    	Tcl_ResetResult(interp);
        Tcl_AppendResult(interp, "No more menus can be allocated.", 
        	(char *) NULL);
    	return TCL_ERROR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FreeID --
 *
 *	Marks the id as free.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The hash table entry for the ID is cleared.
 *
 *----------------------------------------------------------------------
 */

static void
FreeID(
    short menuID)			/* The id to free */
{
    Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&commandTable,
	    (char *) menuID);
    
    if (entryPtr != NULL) {
    	 Tcl_DeleteHashEntry(entryPtr);
    }
    if (menuID == currentAppleMenuID) {
    	currentAppleMenuID = 0;
    }
    if (menuID == currentHelpMenuID) {
    	currentHelpMenuID = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpNewMenu --
 *
 *	Gets a new blank menu. Only the platform specific options are filled
 *	in.
 *
 * Results:
 *	Returns a standard TCL error.
 *
 * Side effects:
 *	Allocates a Macintosh menu handle and puts in the platformData
 *	field of the menuPtr.
 *
 *----------------------------------------------------------------------
 */

int
TkpNewMenu(
    TkMenu *menuPtr)		/* The common structure we are making the
    				 * platform structure for. */
{
    short menuID;
    Str255 itemText;
    int length;
    MenuHandle macMenuHdl;
    int error = TCL_OK;
    
    error = GetNewID(menuPtr->interp, menuPtr, 0, &menuID);
    if (error != TCL_OK) {
    	return error;
    }
    length = strlen(Tk_PathName(menuPtr->tkwin));
    memmove(&itemText[1], Tk_PathName(menuPtr->tkwin), 
    	    (length > 230) ? 230 : length);
    itemText[0] = (length > 230) ? 230 : length;
    macMenuHdl = NewMenu(menuID, itemText);
#ifdef GENERATINGCFM
    {
        Handle mdefProc = FixMDEF();
        if ((mdefProc != NULL)) {
    	    (*macMenuHdl)->menuProc = mdefProc;
    	}
    }
#endif
    menuPtr->platformData = (TkMenuPlatformData) ckalloc(sizeof(MacMenu));
    ((MacMenu *) menuPtr->platformData)->menuHdl = macMenuHdl;
    SetRect(&((MacMenu *) menuPtr->platformData)->menuRect, 0, 0, 0, 0);

    if ((currentMenuBarInterp == menuPtr->interp)
    	    && (currentMenuBarName != NULL)) {
    	Tk_Window parentWin = Tk_Parent(menuPtr->tkwin);
    	
    	if (strcmp(currentMenuBarName, Tk_PathName(parentWin)) == 0) {
    	    if ((strcmp(Tk_PathName(menuPtr->tkwin)
    	    	    + strlen(Tk_PathName(parentWin)), ".apple") == 0)
    	    	    || (strcmp(Tk_PathName(menuPtr->tkwin)
    	    	    + strlen(Tk_PathName(parentWin)), ".help") == 0)) {
	    	if (!(menuBarFlags & MENUBAR_REDRAW_PENDING)) {
	    	    Tcl_DoWhenIdle(DrawMenuBarWhenIdle, (ClientData *) NULL);
	    	    menuBarFlags |= MENUBAR_REDRAW_PENDING;
	    	}
	    }   	    		
    	}
    }
    
    menuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
    Tcl_DoWhenIdle(ReconfigureMacintoshMenu, (ClientData) menuPtr);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyMenu --
 *
 *	Destroys platform-specific menu structures.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	All platform-specific allocations are freed up.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyMenu(
    TkMenu *menuPtr)		/* The common menu structure */
{
    MenuHandle macMenuHdl = ((MacMenu *) menuPtr->platformData)->menuHdl;

    if (menuPtr->menuFlags & MENU_RECONFIGURE_PENDING) {
    	Tcl_CancelIdleCall(ReconfigureMacintoshMenu, (ClientData) menuPtr);
    	menuPtr->menuFlags &= ~MENU_RECONFIGURE_PENDING;
    }

    if ((*macMenuHdl)->menuID == currentHelpMenuID) {
    	MenuHandle helpMenuHdl;
    	
    	if ((HMGetHelpMenuHandle(&helpMenuHdl) == noErr) 
    		&& (helpMenuHdl != NULL)) {
    	    int i, count = CountMItems(helpMenuHdl);
    	    
    	    for (i = helpItemCount; i <= count; i++) {
    	    	DeleteMenuItem(helpMenuHdl, helpItemCount);
    	    }
    	}
    	currentHelpMenuID = 0;
    }

    if (menuPtr->platformData != NULL) {
	DeleteMenu((*macMenuHdl)->menuID);
	FreeID((*macMenuHdl)->menuID);
	DisposeMenu(macMenuHdl);
	ckfree((char *) menuPtr->platformData);
	menuPtr->platformData = NULL;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SetMenuCascade --
 *
 *	Does any cleanup to change a menu from a normal to a cascade.
 *
 * Results:
 *	Standard Tcl error.
 *
 * Side effects:
 *	The mac menu id is reset.
 *
 *----------------------------------------------------------------------
 */

static int
SetMenuCascade(
    TkMenu* menuPtr)		/* The menu we are setting up to be a
				 * cascade. */
{
    MenuHandle macMenuHdl = ((MacMenu *) menuPtr->platformData)->menuHdl;
    short newMenuID, menuID = (*macMenuHdl)->menuID;
    int error = TCL_OK;
    
    if (menuID >= 256) {
    	error = GetNewID(menuPtr->interp, menuPtr, 1, &newMenuID);
    	if (error == TCL_OK) {
    	    FreeID(menuID);
    	    (*macMenuHdl)->menuID = newMenuID;
    	}
    }
    return error;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDestroyMenuEntry --
 *
 *	Cleans up platform-specific menu entry items.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	All platform-specific allocations are freed up.
 *
 *----------------------------------------------------------------------
 */

void
TkpDestroyMenuEntry(
    TkMenuEntry *mePtr)		/* The common structure for the menu 
    				 * entry. */
{
    TkMenu *menuPtr = mePtr->menuPtr;    
  
    ckfree((char *) mePtr->platformEntryData);
    if ((menuPtr->platformData != NULL) 
    	    && !(menuPtr->menuFlags & MENU_RECONFIGURE_PENDING)) {
    	menuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
    	Tcl_DoWhenIdle(ReconfigureMacintoshMenu, (ClientData) menuPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetEntryText --
 *
 *	Given a menu entry, gives back the text that should go in it.
 *	Separators should be done by the caller, as they have to be
 *	handled specially. This is primarily used to do a substitution
 *	between "..." and "ƒ".
 *
 * Results:
 *	itemText points to the new text for the item.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetEntryText(
    TkMenuEntry *mePtr,		/* A pointer to the menu entry. */
    Tcl_DString *dStringPtr)	/* The DString to put the text into. This
    				 * will be initialized by this routine. */
{
    Tcl_DStringInit(dStringPtr);
    if (mePtr->type == TEAROFF_ENTRY) {
    	Tcl_DStringAppend(dStringPtr, "(Tear-off)", -1);
    } else if (mePtr->imagePtr != NULL) {
    	Tcl_DStringAppend(dStringPtr, "(Image)", -1);
    } else if (mePtr->bitmapPtr != NULL) {
    	Tcl_DStringAppend(dStringPtr, "(Pixmap)", -1);
    } else if (mePtr->labelPtr == NULL || mePtr->labelLength == 0) {
    
	/*
	 * The Mac menu manager does not like null strings.
	 */
	 
	Tcl_DStringAppend(dStringPtr, " ", -1);
    } else {
    	int length;
    	char *text = Tcl_GetStringFromObj(mePtr->labelPtr, &length);
    	char *dStringText;
    	int i;
    	
	for (i = 0; i < length; text++, i++) {
    	    if ((*text == '.')
    	    	    && (*(text + 1) != '\0') && (*(text + 1) == '.')
    	    	    && (*(text + 2) != '\0') && (*(text + 2) == '.')) {
    	    	Tcl_DStringAppend(dStringPtr, elipsisString, -1);
    	    	i += strlen(elipsisString) - 1;
   	    } else {
    	    	Tcl_DStringSetLength(dStringPtr,
			Tcl_DStringLength(dStringPtr) + 1);
    	    	dStringText = Tcl_DStringValue(dStringPtr);
    	    	dStringText[i] = *text;
    	    }
    	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FindMarkCharacter --
 *
 *	Finds the Macintosh mark character based on the font of the
 *	item. We calculate a good mark character based on the font
 * 	that this item is rendered in.
 *
 * 	We try the following special mac characters. If none of them
 * 	are present, just use the check mark.
 * 	'' - Check mark character
 * 	'´' - Bullet character
 * 	'' - Filled diamond
 * 	'×' - Hollow diamond
 * 	'„' = Long dash ("em dash")
 * 	'-' = short dash (minus, "en dash");
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	New item is added to platform menu
 *
 *----------------------------------------------------------------------
 */

static char
FindMarkCharacter(
    TkMenuEntry *mePtr)		/* The entry we are finding the character
    				 * for. */
{
    char markChar;
    Tk_Font tkfont;

    tkfont = Tk_GetFontFromObj(mePtr->menuPtr->tkwin,
    	    (mePtr->fontPtr == NULL) ? mePtr->menuPtr->fontPtr
	    : mePtr->fontPtr);
    	    
    if (!TkMacIsCharacterMissing(tkfont, '')) {
    	markChar = '';
    } else if (!TkMacIsCharacterMissing(tkfont, '´')) {
    	markChar = '´';
    } else if (!TkMacIsCharacterMissing(tkfont, '')) {
    	markChar = '';
    } else if (!TkMacIsCharacterMissing(tkfont, '×')) {
    	markChar = '×';
    } else if (!TkMacIsCharacterMissing(tkfont, '„')) {
    	markChar = '„';
    } else if (!TkMacIsCharacterMissing(tkfont, '-')) {
    	markChar = '-';
    } else {
    	markChar = '';
    }
    return markChar;
}

/*
 *----------------------------------------------------------------------
 *
 * SetMenuIndicator --
 *
 *	Sets the Macintosh mark character based on the font of the
 *	item.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	New item is added to platform menu
 *
 *----------------------------------------------------------------------
 */

static void
SetMenuIndicator(
    TkMenuEntry *mePtr)		/* The entry we are setting */
{
    TkMenu *menuPtr = mePtr->menuPtr;
    MenuHandle macMenuHdl = ((MacMenu *) menuPtr->platformData)->menuHdl;
    char markChar;

    /*
     * There can be no indicators on menus that are not checkbuttons
     * or radiobuttons. However, we should go ahead and set them
     * so that menus look right when they are displayed. We should
     * not set cascade entries, however, as the mark character
     * means something different for cascade items on the Mac.
     * Also, we do reflect the tearOff menu items in the Mac menu
     * handle, so we ignore them.
     */

    if (mePtr->type == CASCADE_ENTRY) {
    	return;
    }
    
    markChar = 0;
    if ((mePtr->type == RADIO_BUTTON_ENTRY) 
    	    || (mePtr->type == CHECK_BUTTON_ENTRY)) {
    	if (mePtr->indicatorOn && (mePtr->entryFlags & ENTRY_SELECTED)) {
    	    markChar = FindMarkCharacter(mePtr);
    	}
    }
    SetItemMark(macMenuHdl, mePtr->index + 1, markChar);
}

/*
 *----------------------------------------------------------------------
 *
 * SetMenuTitle --
 *
 *	Sets title of menu so that the text displays correctly in menubar.
 *	This code directly manipulates menu handle data. This code
 *	was originally part of an ancient Apple Developer Response mail.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menu handle will change size depending on the length of the
 *	title
 *
 *----------------------------------------------------------------------
 */

static void
SetMenuTitle(
    MenuHandle menuHdl,		/* The menu we are setting the title of. */
    Tcl_Obj *titlePtr)	/* The C string to set the title to. */
{
    int oldLength, newLength, oldHandleSize, dataLength;
    Ptr menuDataPtr;
    char *title = (titlePtr == NULL) ? ""
    	    : Tcl_GetStringFromObj(titlePtr, NULL);
 
    menuDataPtr = (Ptr) (*menuHdl)->menuData;

    if (strncmp(title, menuDataPtr + 1, menuDataPtr[0]) != 0) {    
    	newLength = strlen(title) + 1;
    	oldLength = menuDataPtr[0] + 1;
    	oldHandleSize = GetHandleSize((Handle) menuHdl);
    	dataLength = oldHandleSize - (sizeof(MenuInfo) - sizeof(Str255)) 
    		- oldLength;
    	if (newLength > oldLength) {
    	    SetHandleSize((Handle) menuHdl, oldHandleSize + (newLength 
    	    	    - oldLength));
    	    menuDataPtr = (Ptr) (*menuHdl)->menuData;
    	}
    
    	BlockMove(menuDataPtr + oldLength, menuDataPtr + newLength, 
    		dataLength);
    	BlockMove(title, menuDataPtr + 1, newLength - 1);
    	menuDataPtr[0] = newLength - 1;
    
    	if (newLength < oldLength) {
    	    SetHandleSize((Handle) menuHdl, oldHandleSize + (newLength 
    	    	    - oldLength));
    	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpConfigureMenuEntry --
 *
 *	Processes configurations for menu entries.
 *
 * Results:
 *	Returns standard TCL result. If TCL_ERROR is returned, then
 *	the interp's result contains an error message.
 *
 * Side effects:
 *	Configuration information get set for mePtr; old resources
 *	get freed, if any need it.
 *
 *----------------------------------------------------------------------
 */

int
TkpConfigureMenuEntry(
    register TkMenuEntry *mePtr)	/* Information about menu entry;  may
					 * or may not already have values for
					 * some fields. */
{
    TkMenu *menuPtr = mePtr->menuPtr;
    int index = mePtr->index;
    MenuHandle macMenuHdl = ((MacMenu *) menuPtr->platformData)->menuHdl;
    MenuHandle helpMenuHdl = NULL;

    /*
     * Cascade menus have to have menu IDs of less than 256. So
     * we need to change the child menu if this has been configured
     * for a cascade item.
     */
    
    if (mePtr->type == CASCADE_ENTRY) {
    	if ((mePtr->childMenuRefPtr != NULL)
    		&& (mePtr->childMenuRefPtr->menuPtr != NULL)) {
    	    MenuHandle childMenuHdl = ((MacMenu *) mePtr
    	    	    ->childMenuRefPtr->menuPtr->platformData)->menuHdl;
    	    
    	    if (childMenuHdl != NULL) {
    	    	int error = SetMenuCascade(mePtr->childMenuRefPtr->menuPtr);
    	    	
    	    	if (error != TCL_OK) {
    	    	    return error;
    	    	}
    	    	
    	    	if (menuPtr->menuType == MENUBAR) {
    	    	    SetMenuTitle(childMenuHdl, mePtr->labelPtr);
    	    	}
    	    }
    	}
    }
	
    /*
     * We need to parse the accelerator string. If it has the strings
     * for Command, Control, Shift or Option, we need to flag it
     * so we can draw the symbols for it. We also need to precalcuate
     * the position of the first real character we are drawing.
     */
	
    if (0 == mePtr->accelLength) {
    	((EntryGeometry *)mePtr->platformEntryData)->accelTextStart = -1;
    } else {
	char *accelString = (mePtr->accelPtr == NULL) ? ""
		: Tcl_GetStringFromObj(mePtr->accelPtr, NULL);
	char *accel = accelString;
	mePtr->entryFlags |= ~ENTRY_ACCEL_MASK;
	    
	while (1) {
	    if ((0 == strncasecmp("Control", accelString, 6))
	    	    && (('-' == accelString[6]) || ('+' == accelString[6]))) {
	  	mePtr->entryFlags |= ENTRY_CONTROL_ACCEL;
	  	accelString += 7;
	    } else if ((0 == strncasecmp("Ctrl", accelString, 4))
	    	    && (('-' == accelString[4]) || ('+' == accelString[4]))) {
	  	mePtr->entryFlags |= ENTRY_CONTROL_ACCEL;
	  	accelString += 5;
	    } else if ((0 == strncasecmp("Shift", accelString, 5))
	    	    && (('-' == accelString[5]) || ('+' == accelString[5]))) {
	  	mePtr->entryFlags |= ENTRY_SHIFT_ACCEL;
	  	accelString += 6;
	    } else if ((0 == strncasecmp("Option", accelString, 6))
	    	    && (('-' == accelString[6]) || ('+' == accelString[6]))) {
	  	mePtr->entryFlags |= ENTRY_OPTION_ACCEL;
	  	accelString += 7;
	    } else if ((0 == strncasecmp("Opt", accelString, 3))
	    	    && (('-' == accelString[3]) || ('+' == accelString[3]))) {
	  	mePtr->entryFlags |= ENTRY_OPTION_ACCEL;
	  	accelString += 4;
	    } else if ((0 == strncasecmp("Command", accelString, 7))
	    	    && (('-' == accelString[7]) || ('+' == accelString[7]))) {
	  	mePtr->entryFlags |= ENTRY_COMMAND_ACCEL;
	  	accelString += 8;
	    } else if ((0 == strncasecmp("Cmd", accelString, 3))
	    	    && (('-' == accelString[3]) || ('+' == accelString[3]))) {
	  	mePtr->entryFlags |= ENTRY_COMMAND_ACCEL;
	  	accelString += 4;
	    } else if ((0 == strncasecmp("Alt", accelString, 3))
	    	    && (('-' == accelString[3]) || ('+' == accelString[3]))) {
	  	mePtr->entryFlags |= ENTRY_OPTION_ACCEL;
	  	accelString += 4;
	    } else if ((0 == strncasecmp("Meta", accelString, 4))
	    	    && (('-' == accelString[4]) || ('+' == accelString[4]))) {
	  	mePtr->entryFlags |= ENTRY_COMMAND_ACCEL;
	  	accelString += 5;
	    } else {
	  	break;
	    }
	}
	    
	((EntryGeometry *)mePtr->platformEntryData)->accelTextStart 
		= ((long) accelString - (long) accel);
    }
    
    if (!(menuPtr->menuFlags & MENU_RECONFIGURE_PENDING)) {
    	menuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
    	Tcl_DoWhenIdle(ReconfigureMacintoshMenu, (ClientData) menuPtr);
    }
    
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ReconfigureIndividualMenu --
 *
 *	This routine redoes the guts of the menu. It works from
 *	a base item and offset, so that a regular menu will
 *	just have all of its items added, but the help menu will
 *	have all of its items appended after the apple-defined
 *	items.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Macintosh menu handle is updated
 *
 *----------------------------------------------------------------------
 */

static void
ReconfigureIndividualMenu(
    TkMenu *menuPtr,		/* The menu we are affecting. */
    MenuHandle macMenuHdl,	/* The macintosh menu we are affecting.
    				 * Will not necessarily be
    				 * menuPtr->platformData because this could
    				 * be the help menu. */
    int base)			/* The last index that we do not want
    				 * touched. 0 for normal menus;
    				 * helpMenuItemCount for help menus. */
{
    int count;
    int index;
    TkMenuEntry *mePtr;
    Str255 itemText;
    int parentDisabled = 0;
    
    for (mePtr = menuPtr->menuRefPtr->parentEntryPtr; mePtr != NULL;
    	    mePtr = mePtr->nextCascadePtr) {
    	char *name = (mePtr->namePtr == NULL) ? ""
    		: Tcl_GetStringFromObj(mePtr->namePtr, NULL);
    	
    	if (strcmp(Tk_PathName(menuPtr->tkwin), name) == 0) {
    	    if (mePtr->state == ENTRY_DISABLED) {
    	    	parentDisabled = 1;
    	    }
    	    break;
    	}
    }
    
    /*
     * First, we get rid of all of the old items.
     */
    
    count = CountMItems(macMenuHdl);
    for (index = base; index < count; index++) {
    	DeleteMenuItem(macMenuHdl, base + 1);
    }

    count = menuPtr->numEntries;
    
    for (index = 1; index <= count; index++) {
    	mePtr = menuPtr->entries[index - 1];
    
    	/*
    	 * We have to do separators separately because SetMenuItemText
    	 * does not parse meta-characters.
    	 */
    
    	if (mePtr->type == SEPARATOR_ENTRY) {
    	    AppendMenu(macMenuHdl, SEPARATOR_TEXT);
    	} else {
    	    Tcl_DString itemTextDString;
    	    int destWrote;
    	    
	    GetEntryText(mePtr, &itemTextDString);
	    Tcl_UtfToExternal(NULL, NULL, Tcl_DStringValue(&itemTextDString),
	    	    Tcl_DStringLength(&itemTextDString), 0, NULL, 
	    	    (char *) &itemText[1],
	    	    231, NULL, &destWrote, NULL);
	    itemText[0] = destWrote;
	    
	    AppendMenu(macMenuHdl, "\px");
	    SetMenuItemText(macMenuHdl, base + index, itemText);
	    Tcl_DStringFree(&itemTextDString);
	
    	    /*
    	     * Set enabling and disabling correctly.
    	     */

	    if (parentDisabled || (mePtr->state == ENTRY_DISABLED)) {
	    	DisableItem(macMenuHdl, base + index);
	    } else {
	    	EnableItem(macMenuHdl, base + index);
	    }
    	
    	    /*
    	     * Set the check mark for check entries and radio entries.
    	     */
	
	    SetItemMark(macMenuHdl, base + index, 0);		
	    if ((mePtr->type == CHECK_BUTTON_ENTRY)
		    || (mePtr->type == RADIO_BUTTON_ENTRY)) {
	    	CheckItem(macMenuHdl, base + index, (mePtr->entryFlags
		    	& ENTRY_SELECTED) && mePtr->indicatorOn);
		if (mePtr->indicatorOn
			&& (mePtr->entryFlags & ENTRY_SELECTED)) {
		    SetItemMark(macMenuHdl, base + index,
		    	    FindMarkCharacter(mePtr));
	    	}
	    }
	
	    if (mePtr->type == CASCADE_ENTRY) {
	    	if ((mePtr->childMenuRefPtr != NULL) 
	    	    	&& (mePtr->childMenuRefPtr->menuPtr != NULL)) {
	    	    MenuHandle childMenuHdl = 
	    	    	    ((MacMenu *) mePtr->childMenuRefPtr
			    ->menuPtr->platformData)->menuHdl;

		    if (childMenuHdl == NULL) {
		        childMenuHdl = ((MacMenu *) mePtr->childMenuRefPtr
			    	->menuPtr->platformData)->menuHdl;
		    }
		    if (childMenuHdl != NULL) {
		        if (TkMacHaveAppearance() > 1) {
		            SetMenuItemHierarchicalID(macMenuHdl, base + index,
				    (*childMenuHdl)->menuID);
		        } else {
	    	    	SetItemMark(macMenuHdl, base + index,
				(*childMenuHdl)->menuID);
	    	    	SetItemCmd(macMenuHdl, base + index, CASCADE_CMD);
	    	    }
	    	    }
	    	    /*
	    	     * If we changed the highligthing of this menu, its
	    	     * children all have to be reconfigured so that
	    	     * their state will be reflected in the menubar.
	    	     */
	    
	    	    if (!(mePtr->childMenuRefPtr->menuPtr->menuFlags 
	    	    	    	& MENU_RECONFIGURE_PENDING)) {
	    	    	mePtr->childMenuRefPtr->menuPtr->menuFlags
	    	    		|= MENU_RECONFIGURE_PENDING;
	    	    	Tcl_DoWhenIdle(ReconfigureMacintoshMenu, 
	    	    		(ClientData) mePtr->childMenuRefPtr->menuPtr);
	    	    }
	    	}
	    }
	    
    	    if ((mePtr->type != CASCADE_ENTRY) 
    	    	    && (ENTRY_COMMAND_ACCEL 
    	    	    == (mePtr->entryFlags & ENTRY_ACCEL_MASK))) {
    	    	char *accel = Tcl_GetStringFromObj(mePtr->accelPtr, NULL);
	    	SetItemCmd(macMenuHdl, index, accel[((EntryGeometry *)
	    		mePtr->platformEntryData)->accelTextStart]);
	    }
    	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * ReconfigureMacintoshMenu --
 *
 *	Rebuilds the Macintosh MenuHandle items from the menu. Called
 *	usually as an idle handler, but can be called synchronously
 *	if the menu is about to be posted.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Configuration information get set for mePtr; old resources
 *	get freed, if any need it.
 *
 *----------------------------------------------------------------------
 */

static void
ReconfigureMacintoshMenu(
    ClientData clientData)		/* Information about menu entry;  may
					 * or may not already have values for
					 * some fields. */
{
    TkMenu *menuPtr = (TkMenu *) clientData;
    MenuHandle macMenuHdl = ((MacMenu *) menuPtr->platformData)->menuHdl;
    MenuHandle helpMenuHdl = NULL;

    menuPtr->menuFlags &= ~MENU_RECONFIGURE_PENDING;

    if (NULL == macMenuHdl) {
    	return;
    }

    ReconfigureIndividualMenu(menuPtr, macMenuHdl, 0);

    if (menuPtr->menuFlags & MENU_APPLE_MENU) {
    	AddResMenu(macMenuHdl, 'DRVR');
    }

    if ((*macMenuHdl)->menuID == currentHelpMenuID) {
    	HMGetHelpMenuHandle(&helpMenuHdl);
    	if (helpMenuHdl != NULL) {
    	    ReconfigureIndividualMenu(menuPtr, helpMenuHdl, helpItemCount);
    	}
    }

    if (menuPtr->menuType == MENUBAR) {
        if (!(menuBarFlags & MENUBAR_REDRAW_PENDING)) {
    	    Tcl_DoWhenIdle(DrawMenuBarWhenIdle, (ClientData *) NULL);
    	    menuBarFlags |= MENUBAR_REDRAW_PENDING;
    	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * CompleteIdlers --
 *
 *	Completes all idle handling so that the menus are in sync when
 *	the user invokes them with the mouse.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The Macintosh menu handles are flushed out.
 *
 *----------------------------------------------------------------------
 */

static void
CompleteIdlers(
    TkMenu *menuPtr)			/* The menu we are completing. */
{
    int i;

    if (menuPtr->menuFlags & MENU_RECONFIGURE_PENDING) {
    	Tcl_CancelIdleCall(ReconfigureMacintoshMenu, (ClientData) menuPtr);
    	ReconfigureMacintoshMenu((ClientData) menuPtr);
    }
    
    for (i = 0; i < menuPtr->numEntries; i++) {
        if (menuPtr->entries[i]->type == CASCADE_ENTRY) {
            if ((menuPtr->entries[i]->childMenuRefPtr != NULL)
            	    && (menuPtr->entries[i]->childMenuRefPtr->menuPtr
		    != NULL)) {
		CompleteIdlers(menuPtr->entries[i]->childMenuRefPtr
			->menuPtr);
	    }
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpPostMenu --
 *
 *	Posts a menu on the screen
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menu is posted and handled.
 *
 *----------------------------------------------------------------------
 */

int
TkpPostMenu(
    Tcl_Interp *interp,		/* The interpreter this menu lives in */
    TkMenu *menuPtr,		/* The menu we are posting */
    int x,			/* The global x-coordinate of the top, left-
    				 * hand corner of where the menu is supposed
    				 * to be posted. */
    int y)			/* The global y-coordinate */
{
    MenuHandle macMenuHdl = ((MacMenu *) menuPtr->platformData)->menuHdl;
    long popUpResult;
    int result;
    int oldMode;

    if (inPostMenu) {
        Tcl_AppendResult(interp,
		"Cannot call post menu while already posting menu",
		(char *) NULL);
    	result = TCL_ERROR;
    } else {
    	Window dummyWin;
    	unsigned int state;
    	int dummy, mouseX, mouseY;
    	short menuID;
	Window window;
	int oldWidth = menuPtr->totalWidth;
	Tk_Window parentWindow = Tk_Parent(menuPtr->tkwin);
    
    	inPostMenu++;
    	
    	result = TkPreprocessMenu(menuPtr);
    	if (result != TCL_OK) {
    	    inPostMenu--;
    	    return result;
    	}

    	/*
    	 * The post commands could have deleted the menu, which means
    	 * we are dead and should go away.
    	 */
    	
    	if (menuPtr->tkwin == NULL) {
    	    inPostMenu--;
    	    return TCL_OK;
    	}

    	CompleteIdlers(menuPtr);
    	if (menuBarFlags & MENUBAR_REDRAW_PENDING) {
    	    Tcl_CancelIdleCall(DrawMenuBarWhenIdle, (ClientData *) NULL);
    	    DrawMenuBarWhenIdle((ClientData *) NULL);
        }
    	
	if (NULL == parentWindow) {
	    tearoffStruct.excludeRect.top = tearoffStruct.excludeRect.left
	    	    = tearoffStruct.excludeRect.bottom
		    = tearoffStruct.excludeRect.right = SHRT_MAX;
	} else {
	    int left, top;
	
	    Tk_GetRootCoords(parentWindow, &left, &top);
	    tearoffStruct.excludeRect.left = left;
	    tearoffStruct.excludeRect.top = top;
	    tearoffStruct.excludeRect.right = left + Tk_Width(parentWindow);
	    tearoffStruct.excludeRect.bottom = top + Tk_Height(parentWindow);
	    if (Tk_Class(parentWindow) == Tk_GetUid("Menubutton")) {
	    	TkWindow *parentWinPtr = (TkWindow *) parentWindow;
	    	TkMenuButton *mbPtr = 
	    		(TkMenuButton *) parentWinPtr->instanceData;
	    	int menuButtonWidth = Tk_Width(parentWindow)
	    		- 2 * (mbPtr->highlightWidth + mbPtr->borderWidth + 1);
	    	menuPtr->totalWidth = menuButtonWidth > menuPtr->totalWidth
	    		? menuButtonWidth : menuPtr->totalWidth;
	    }
	}
    	 
    	InsertMenu(macMenuHdl, -1);
    	RecursivelyInsertMenu(menuPtr);
    	CountMItems(macMenuHdl);
    	
	FixMDEF();
	oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
	popUpResult = PopUpMenuSelect(macMenuHdl, y, x, menuPtr->active);
	Tcl_SetServiceMode(oldMode);

	menuPtr->totalWidth = oldWidth;
	RecursivelyDeleteMenu(menuPtr);
	DeleteMenu((*macMenuHdl)->menuID);
	
	/*
	 * Simulate the mouse up.
	 */
	 
	XQueryPointer(NULL, None, &dummyWin, &dummyWin, &mouseX,
	    &mouseY, &dummy, &dummy, &state);
	window = Tk_WindowId(menuPtr->tkwin);
	TkGenerateButtonEvent(mouseX, mouseY, window, state);
	
	/*
	 * Dispatch the command.
	 */
	 
	menuID = HiWord(popUpResult);
	if (menuID != 0) {
	    result = TkMacDispatchMenuEvent(menuID, LoWord(popUpResult));
	} else {
	    TkMacHandleTearoffMenu();
	    result = TCL_OK;
	}
	InvalidateMDEFRgns();
	RecursivelyClearActiveMenu(menuPtr);
	
	inPostMenu--;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuNewEntry --
 *
 *	Adds a pointer to a new menu entry structure with the platform-
 *	specific fields filled in. The Macintosh uses the
 *	platformEntryData field of the TkMenuEntry record to store
 *	geometry information.
 *
 * Results:
 *	Standard TCL error.
 *
 * Side effects:
 *	Storage gets allocated. New menu entry data is put into the
 *	platformEntryData field of the mePtr.
 *
 *----------------------------------------------------------------------
 */

int
TkpMenuNewEntry(
    TkMenuEntry *mePtr)		/* The menu we are adding an entry to */
{
    EntryGeometry *geometryPtr =
	    (EntryGeometry *) ckalloc(sizeof(EntryGeometry));
    TkMenu *menuPtr = mePtr->menuPtr;
    
    geometryPtr->accelTextStart = 0;
    geometryPtr->accelTextWidth = 0;
    geometryPtr->nonAccelMargin = 0;
    geometryPtr->modifierWidth = 0;
    mePtr->platformEntryData = (TkMenuPlatformEntryData) geometryPtr;
    if (!(menuPtr->menuFlags & MENU_RECONFIGURE_PENDING)) {
    	menuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
    	Tcl_DoWhenIdle(ReconfigureMacintoshMenu, (ClientData) menuPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * 
 * Tk_MacTurnOffMenus --
 *
 *	Turns off all the menu drawing code.  This is more than just disabling
 *      the "menu" command, this means that Tk will NEVER touch the menubar.
 *      It is needed in the Plugin, where Tk does not own the menubar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	A flag is set which will disable all menu drawing.
 *
 *----------------------------------------------------------------------
 */

void
Tk_MacTurnOffMenus()
{
    gNoTkMenus = 1;
}

/*
 *----------------------------------------------------------------------
 *
 * 
 * DrawMenuBarWhenIdle --
 *
 *	Update the menu bar next time there is an idle event.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Menu bar is redrawn.
 *
 *----------------------------------------------------------------------
 */

static void
DrawMenuBarWhenIdle(
    ClientData clientData)	/* ignored here */
{
    TkMenuReferences *menuRefPtr;
    TkMenu *appleMenuPtr, *helpMenuPtr;
    MenuHandle macMenuHdl;
    Tcl_HashEntry *hashEntryPtr;
    
    /*
     * If we have been turned off, exit.
     */
     
    if (gNoTkMenus) {
        return;
    }
    
    /*
     * We need to clear the apple and help menus of any extra items.
     */
 
    if (currentAppleMenuID != 0) {
    	hashEntryPtr = Tcl_FindHashEntry(&commandTable,
    		(char *) currentAppleMenuID);
    	appleMenuPtr = (TkMenu *) Tcl_GetHashValue(hashEntryPtr);
    	TkpDestroyMenu(appleMenuPtr);
    	TkpNewMenu(appleMenuPtr);
    	appleMenuPtr->menuFlags &= ~MENU_APPLE_MENU;
    	appleMenuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
    	Tcl_DoWhenIdle(ReconfigureMacintoshMenu, 
    		(ClientData) appleMenuPtr);
    }

    if (currentHelpMenuID != 0) {
    	hashEntryPtr = Tcl_FindHashEntry(&commandTable,
    		(char *) currentHelpMenuID);
    	helpMenuPtr = (TkMenu *) Tcl_GetHashValue(hashEntryPtr);
    	TkpDestroyMenu(helpMenuPtr);
    	TkpNewMenu(helpMenuPtr);
    	helpMenuPtr->menuFlags &= ~MENU_HELP_MENU;
    	helpMenuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
    	Tcl_DoWhenIdle(ReconfigureMacintoshMenu,
    		(ClientData) helpMenuPtr);
    }
    
    /*
     * We need to find the clone of this menu that is the menubar.
     * Once we do that, for every cascade in the menu, we need to 
     * insert the Mac menu in the Mac menubar. Finally, we need
     * to redraw the menubar.
     */

    menuRefPtr = NULL;
    if (currentMenuBarName != NULL) {
    	menuRefPtr = TkFindMenuReferences(currentMenuBarInterp,
    		currentMenuBarName);
    }
    if (menuRefPtr != NULL) {
    	TkMenu *menuPtr, *menuBarPtr;
    	TkMenu *cascadeMenuPtr;
        char *appleMenuName, *helpMenuName;
        int appleIndex = -1, helpIndex = -1;
    	int i;
        
        menuPtr = menuRefPtr->menuPtr;
        if (menuPtr != NULL) {
            TkMenuReferences *specialMenuRefPtr;
            TkMenuEntry *specialEntryPtr;
            
            appleMenuName = ckalloc(strlen(currentMenuBarName)
            	    + 1 + strlen(".apple") + 1);
            sprintf(appleMenuName, "%s.apple", 
            	    Tk_PathName(menuPtr->tkwin));
            specialMenuRefPtr = TkFindMenuReferences(currentMenuBarInterp, 
            	    appleMenuName);
            if ((specialMenuRefPtr != NULL) 
            	    && (specialMenuRefPtr->menuPtr != NULL)) {
            	for (specialEntryPtr 
            		= specialMenuRefPtr->parentEntryPtr;
            		specialEntryPtr != NULL;
            		specialEntryPtr 
            		= specialEntryPtr->nextCascadePtr) {
		    if (specialEntryPtr->menuPtr == menuPtr) {
		    	appleIndex = specialEntryPtr->index;
		    	break;
		    }
		}
	    }	            	    	            
            ckfree(appleMenuName);
            
            helpMenuName = ckalloc(strlen(currentMenuBarName)
            	    + 1 + strlen(".help") + 1);
            sprintf(helpMenuName, "%s.help", 
            	    Tk_PathName(menuPtr->tkwin));
            specialMenuRefPtr = TkFindMenuReferences(currentMenuBarInterp, 
            	    helpMenuName);
            if ((specialMenuRefPtr != NULL)
            	    && (specialMenuRefPtr->menuPtr != NULL)) {
            	for (specialEntryPtr 
            		= specialMenuRefPtr->parentEntryPtr;
            		specialEntryPtr != NULL;
            		specialEntryPtr 
            		= specialEntryPtr->nextCascadePtr) {
		    if (specialEntryPtr->menuPtr == menuPtr) {
		    	helpIndex = specialEntryPtr->index;
		    	break;
		    }
		}
	    }
	    ckfree(helpMenuName);  
                
        }
        
        for (menuBarPtr = menuPtr; 
        	(menuBarPtr != NULL) 
        	&& (menuBarPtr->menuType != MENUBAR);
        	menuBarPtr = menuBarPtr->nextInstancePtr) {
        
            /*
             * Null loop body.
             */
             
        }
        
        if (menuBarPtr == NULL) {
            SetDefaultMenubar();
        } else {
	    if (menuBarPtr->tearoff != menuPtr->tearoff) {
	    	if (menuBarPtr->tearoff) {
	    	    appleIndex = (-1 == appleIndex) ? appleIndex
	    	    	    : appleIndex + 1;
	    	    helpIndex = (-1 == helpIndex) ? helpIndex
	    	    	    : helpIndex + 1;
	    	} else {
	    	    appleIndex = (-1 == appleIndex) ? appleIndex
	    	            : appleIndex - 1;
	    	    helpIndex = (-1 == helpIndex) ? helpIndex
	    	    	    : helpIndex - 1;
	    	}
	    }
	    ClearMenuBar();
	    
	    if (appleIndex == -1) {
	    	InsertMenu(tkAppleMenu, 0);
	    	currentAppleMenuID = 0;
	    } else {
    		short appleID;
    	    	appleMenuPtr = menuBarPtr->entries[appleIndex]
    	    	    	->childMenuRefPtr->menuPtr;
		TkpDestroyMenu(appleMenuPtr);
    		GetNewID(appleMenuPtr->interp, appleMenuPtr, 0, 
    			&appleID);
    		macMenuHdl = NewMenu(appleID, "\p\024");
    		appleMenuPtr->platformData = 
    			(TkMenuPlatformData) ckalloc(sizeof(MacMenu));
    		((MacMenu *)appleMenuPtr->platformData)->menuHdl
    			= macMenuHdl;
    		SetRect(&((MacMenu *) appleMenuPtr->platformData)->menuRect,
    			0, 0, 0, 0);
    	    	appleMenuPtr->menuFlags |= MENU_APPLE_MENU;
    	    	if (!(appleMenuPtr->menuFlags 
    	    		& MENU_RECONFIGURE_PENDING)) {
    	    	    appleMenuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
    	    	    Tcl_DoWhenIdle(ReconfigureMacintoshMenu,
    	    	    	    (ClientData) appleMenuPtr);
    	    	}
    	    	InsertMenu(macMenuHdl, 0);
    	    	RecursivelyInsertMenu(appleMenuPtr);
    	    	currentAppleMenuID = appleID;
	    }
	    if (helpIndex == -1) {
	    	currentHelpMenuID = 0;
	    }
	    
	    for (i = 0; i < menuBarPtr->numEntries; i++) {
	    	if (i == appleIndex) {
	    	    if (menuBarPtr->entries[i]->state == ENTRY_DISABLED) {
	    	    	DisableItem(((MacMenu *) menuBarPtr->entries[i]
	    	    		->childMenuRefPtr->menuPtr
	    	    		->platformData)->menuHdl,
	    	    		0);
	    	    } else {
	    	    	EnableItem(((MacMenu *) menuBarPtr->entries[i]
	    	    		->childMenuRefPtr->menuPtr
	    	    		->platformData)->menuHdl,
	    	    		0);
		    }	    	    	
	    	    continue;
	    	} else if (i == helpIndex) {
	    	    TkMenu *helpMenuPtr = menuBarPtr->entries[i]
	    	    	    ->childMenuRefPtr->menuPtr;
	    	    MenuHandle helpMenuHdl = NULL;
	    	    
	    	    if (helpMenuPtr == NULL) {
	    	    	continue;
	    	    }
	    	    helpMenuPtr->menuFlags |= MENU_HELP_MENU;
	    	    if (!(helpMenuPtr->menuFlags
	    	    	    & MENU_RECONFIGURE_PENDING)) {
	    	    	helpMenuPtr->menuFlags 
	    	    		|= MENU_RECONFIGURE_PENDING;
	    	    	Tcl_DoWhenIdle(ReconfigureMacintoshMenu,
	    	    		(ClientData) helpMenuPtr);
	    	    }
	    	    macMenuHdl = 
	    	    	    ((MacMenu *) helpMenuPtr->platformData)->menuHdl;
	    	    currentHelpMenuID = (*macMenuHdl)->menuID;
	    	} else if (menuBarPtr->entries[i]->type 
	    		== CASCADE_ENTRY) {
	    	    if ((menuBarPtr->entries[i]->childMenuRefPtr != NULL)
	    		    && menuBarPtr->entries[i]->childMenuRefPtr
			    ->menuPtr != NULL) {
	    	    	cascadeMenuPtr = menuBarPtr->entries[i]
			    	->childMenuRefPtr->menuPtr;
	    	    	macMenuHdl = ((MacMenu *) cascadeMenuPtr
	    	    		->platformData)->menuHdl;
		    	DeleteMenu((*macMenuHdl)->menuID);
	    	    	InsertMenu(macMenuHdl, 0);
	    	    	RecursivelyInsertMenu(cascadeMenuPtr);
	    	    	if (menuBarPtr->entries[i]->state == ENTRY_DISABLED) {
	    	    	    DisableItem(((MacMenu *) menuBarPtr->entries[i]
	    	    	    	    ->childMenuRefPtr->menuPtr
	    	    	    	    ->platformData)->menuHdl,
				    0);
	    	    	} else {
	    	    	    EnableItem(((MacMenu *) menuBarPtr->entries[i]
	    	    	    	    ->childMenuRefPtr->menuPtr
	    	    	    	    ->platformData)->menuHdl,
				    0);
	    	    	 }
	    	    }
	    	}
	    }
	}
    } else {
    	SetDefaultMenubar();
    }
    DrawMenuBar();
    menuBarFlags &= ~MENUBAR_REDRAW_PENDING;
}


/*
 *----------------------------------------------------------------------
 *
 * RecursivelyInsertMenu --
 *
 *	Puts all of the cascades of this menu in the Mac hierarchical list.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menubar is changed.
 *
 *----------------------------------------------------------------------
 */

static void
RecursivelyInsertMenu(
    TkMenu *menuPtr)		/* All of the cascade items in this menu
    				 * will be inserted into the mac menubar. */
{
    int i;
    TkMenu *cascadeMenuPtr;
    MenuHandle macMenuHdl;
    
    for (i = 0; i < menuPtr->numEntries; i++) {
        if (menuPtr->entries[i]->type == CASCADE_ENTRY) {
            if ((menuPtr->entries[i]->childMenuRefPtr != NULL)
            	    && (menuPtr->entries[i]->childMenuRefPtr->menuPtr
		    != NULL)) {
            	cascadeMenuPtr = menuPtr->entries[i]->childMenuRefPtr->menuPtr;
	    	macMenuHdl =
		        ((MacMenu *) cascadeMenuPtr->platformData)->menuHdl;
	    	InsertMenu(macMenuHdl, -1);
	    	RecursivelyInsertMenu(cascadeMenuPtr);
	    }
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * RecursivelyDeleteMenu --
 *
 *	Takes all of the cascades of this menu out of the Mac hierarchical
 *	list.
 *
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menubar is changed.
 *
 *----------------------------------------------------------------------
 */

static void
RecursivelyDeleteMenu(
    TkMenu *menuPtr)		/* All of the cascade items in this menu
    				 * will be inserted into the mac menubar. */
{
    int i;
    TkMenu *cascadeMenuPtr;
    MenuHandle macMenuHdl;
    
    for (i = 0; i < menuPtr->numEntries; i++) {
        if (menuPtr->entries[i]->type == CASCADE_ENTRY) {
            if ((menuPtr->entries[i]->childMenuRefPtr != NULL)
            	    && (menuPtr->entries[i]->childMenuRefPtr->menuPtr
		    != NULL)) {
            	cascadeMenuPtr = menuPtr->entries[i]->childMenuRefPtr->menuPtr;
	    	macMenuHdl =
		        ((MacMenu *) cascadeMenuPtr->platformData)->menuHdl;
	    	DeleteMenu((*macMenuHdl)->menuID);
	    	RecursivelyInsertMenu(cascadeMenuPtr);
	    }
        }
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SetDefaultMenubar --
 *
 *	Puts the Apple, File and Edit menus into the Macintosh menubar.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menubar is changed.
 *
 *----------------------------------------------------------------------
 */

static void
SetDefaultMenubar()
{
    if (currentMenuBarName != NULL) {
    	ckfree(currentMenuBarName);
    	currentMenuBarName = NULL;
    }
    currentMenuBarOwner = NULL;
    ClearMenuBar();
    InsertMenu(tkAppleMenu, 0);
    InsertMenu(tkFileMenu, 0);
    InsertMenu(tkEditMenu, 0);
    if (!(menuBarFlags & MENUBAR_REDRAW_PENDING)) {
    	Tcl_DoWhenIdle(DrawMenuBarWhenIdle, (ClientData *) NULL);
    	menuBarFlags |= MENUBAR_REDRAW_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSetMainMenubar --
 *
 *	Puts the menu associated with a window into the menubar. Should
 *	only be called when the window is in front.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The menubar is changed.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetMainMenubar(
    Tcl_Interp *interp,		/* The interpreter of the application */
    Tk_Window tkwin,		/* The frame we are setting up */
    char *menuName)		/* The name of the menu to put in front.
    				 * If NULL, use the default menu bar.
    				 */
{
    TkWindow *winPtr = (TkWindow *) tkwin;
    WindowRef macWindowPtr = (WindowRef) TkMacGetDrawablePort(winPtr->window);
    
    if ((macWindowPtr == NULL) || (macWindowPtr != FrontWindow())) {
    	return;
    }

    if ((currentMenuBarInterp != interp) 
            || (currentMenuBarOwner != tkwin) 
            || (currentMenuBarName == NULL)
            || (menuName == NULL) 
            || (strcmp(menuName, currentMenuBarName) != 0)) {        
	Tk_Window searchWindow;
    	TopLevelMenubarList *listPtr;
	    		    
        if (currentMenuBarName != NULL) {
            ckfree(currentMenuBarName);
        }

	if (menuName == NULL) {
	    searchWindow = tkwin;
	    if (strcmp(Tk_Class(searchWindow), "Menu") == 0) {
	    	TkMenuReferences *menuRefPtr;
	    	    
	    	menuRefPtr = TkFindMenuReferences(interp, Tk_PathName(tkwin));
	    	if (menuRefPtr != NULL) {
	    	    TkMenu *menuPtr = menuRefPtr->menuPtr;
	    	    if (menuPtr != NULL) {
	    	    	menuPtr = menuPtr->masterMenuPtr;
	    	    	searchWindow = menuPtr->tkwin;
	    	    }
	    	}
	    } 
	    for (; searchWindow != NULL;
		    searchWindow = Tk_Parent(searchWindow)) {
	    	
	    	for (listPtr = windowListPtr; listPtr != NULL;
	    		listPtr = listPtr->nextPtr) {
	    	    if (listPtr->tkwin == searchWindow) {
	    	    	break;
	    	    }
	    	}
	    	if (listPtr != NULL) {
	    	    menuName = Tk_PathName(listPtr->menuPtr->masterMenuPtr
			    ->tkwin);
	    	    break;
	    	}
	    }
	}
	
	if (menuName == NULL) {
	    currentMenuBarName = NULL;
	} else {            
            currentMenuBarName = ckalloc(strlen(menuName) + 1);
	    strcpy(currentMenuBarName, menuName);
        }
        currentMenuBarOwner = tkwin;
        currentMenuBarInterp = interp;
    }
    if (!(menuBarFlags & MENUBAR_REDRAW_PENDING)) {
    	Tcl_DoWhenIdle(DrawMenuBarWhenIdle, (ClientData *) NULL);
    	menuBarFlags |= MENUBAR_REDRAW_PENDING;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpSetWindowMenuBar --
 *
 *	Associates a given menu with a window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	On Windows and UNIX, associates the platform menu with the
 *	platform window.
 *
 *----------------------------------------------------------------------
 */

void
TkpSetWindowMenuBar(
    Tk_Window tkwin,		/* The window we are setting the menu in */
    TkMenu *menuPtr)		/* The menu we are setting */
{
    TopLevelMenubarList *listPtr, *prevPtr;
    
    /*
     * Remove any existing reference to this window.
     */
    
    for (prevPtr = NULL, listPtr = windowListPtr; 
    	    listPtr != NULL; 
    	    prevPtr = listPtr, listPtr = listPtr->nextPtr) {
	if (listPtr->tkwin == tkwin) {
	    break;
	}    	 
    }
    
    if (listPtr != NULL) {
    	if (prevPtr != NULL) {
    	    prevPtr->nextPtr = listPtr->nextPtr;
    	} else {
    	    windowListPtr = listPtr->nextPtr;
    	}
    	ckfree((char *) listPtr);
    }
    
    if (menuPtr != NULL) {
    	listPtr = (TopLevelMenubarList *) ckalloc(sizeof(TopLevelMenubarList));
    	listPtr->nextPtr = windowListPtr;
    	windowListPtr = listPtr;
    	listPtr->tkwin = tkwin;
    	listPtr->menuPtr = menuPtr;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacDispatchMenuEvent --
 *
 *	Given a menu id and an item, dispatches the command associated
 *	with it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands get executed.
 *
 *----------------------------------------------------------------------
 */

int
TkMacDispatchMenuEvent(
    int menuID,		/* The menu id of the menu we are invoking */
    int index)		/* The one-based index of the item that was 
                         * selected. */
{
    int result = TCL_OK;
    if (menuID != 0) {
    	if (menuID == kHMHelpMenuID) {
    	    if (currentMenuBarOwner != NULL) {
    	    	TkMenuReferences *helpMenuRef;
    	    	char *helpMenuName = ckalloc(strlen(currentMenuBarName)
    	    		+ strlen(".help") + 1);
    	    	sprintf(helpMenuName, "%s.help", currentMenuBarName);
    	    	helpMenuRef = TkFindMenuReferences(currentMenuBarInterp,
    	    		helpMenuName);
    	    	ckfree(helpMenuName);
    	    	if ((helpMenuRef != NULL) && (helpMenuRef->menuPtr != NULL)) {
    	    	    int newIndex = index - helpItemCount - 1;
    	    	    result = TkInvokeMenu(currentMenuBarInterp,
    	    	    	    helpMenuRef->menuPtr, newIndex);
    	    	}
    	    }
    	} else {
	    Tcl_HashEntry *commandEntryPtr = 
	    	    Tcl_FindHashEntry(&commandTable, (char *) menuID);
	    TkMenu *menuPtr = (TkMenu *) Tcl_GetHashValue(commandEntryPtr);
	    if ((currentAppleMenuID == menuID) 
	    	    && (index > menuPtr->numEntries + 1)) {
	    	Str255 itemText;
	    	
	    	GetMenuItemText(GetMenuHandle(menuID), index, itemText);
	    	OpenDeskAcc(itemText);
	    	result = TCL_OK;
	    } else {
	    	result = TkInvokeMenu(menuPtr->interp, menuPtr, index - 1);
	    }
	}
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * GetMenuIndicatorGeometry --
 *
 *	Gets the width and height of the indicator area of a menu.
 *
 * Results:
 *	widthPtr and heightPtr are set.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetMenuIndicatorGeometry (
    TkMenu *menuPtr,			/* The menu we are drawing */
    TkMenuEntry *mePtr,			/* The entry we are measuring */
    Tk_Font tkfont,			/* Precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* Precalculated font metrics */
    int *widthPtr,			/* The resulting width */
    int *heightPtr)			/* The resulting height */
{
    char markChar;
    
    *heightPtr = fmPtr->linespace;
 
    markChar = (char) FindMarkCharacter(mePtr);
    *widthPtr = Tk_TextWidth(tkfont, &markChar, 1) + 4;
}

/*
 *----------------------------------------------------------------------
 *
 * GetMenuAccelGeometry --
 *
 *	Gets the width and height of the accelerator area of a menu.
 *
 * Results:
 *	widthPtr and heightPtr are set.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetMenuAccelGeometry (
    TkMenu *menuPtr,			/* The menu we are measuring */
    TkMenuEntry *mePtr,			/* The entry we are measuring */
    Tk_Font tkfont,			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* The precalculated font metrics */
    int *modWidthPtr,			/* The width of all of the key
    					 * modifier symbols. */
    int *textWidthPtr,			/* The resulting width */
    int *heightPtr)			/* The resulting height */
{
    *heightPtr = fmPtr->linespace;
    *modWidthPtr = 0;
    if (mePtr->type == CASCADE_ENTRY) {
        *textWidthPtr = SICN_HEIGHT;
    	*modWidthPtr = Tk_TextWidth(tkfont, "W", 1);
    } else if (0 == mePtr->accelLength) {
    	*textWidthPtr = 0;
    } else {
    	char *accel = (mePtr->accelPtr == NULL) ? ""
    		: Tcl_GetStringFromObj(mePtr->accelPtr, NULL);
    	
    	if (NULL == GetResource('SICN', SICN_RESOURCE_NUMBER)) {
    	    *textWidthPtr = Tk_TextWidth(tkfont, accel, mePtr->accelLength);
    	} else {
    	    int emWidth = Tk_TextWidth(tkfont, "W", 1) + 1;
    	    if ((mePtr->entryFlags & ENTRY_ACCEL_MASK) == 0) {
    	    	int width = Tk_TextWidth(tkfont, accel,	mePtr->accelLength);
    	    	*textWidthPtr = emWidth;
    	    	if (width < emWidth) {
    	    	    *modWidthPtr = 0;
    	    	} else {
    	    	    *modWidthPtr = width - emWidth;
    	    	}   
    	    } else {
    	        int length = ((EntryGeometry *)mePtr->platformEntryData)
    	    	    	->accelTextStart;
    	    	if (mePtr->entryFlags & ENTRY_CONTROL_ACCEL) {
    	    	    *modWidthPtr += CONTROL_ICON_WIDTH;
    	    	}
    	    	if (mePtr->entryFlags & ENTRY_SHIFT_ACCEL) {
    	    	    *modWidthPtr += SHIFT_ICON_WIDTH;
    	    	}
    	    	if (mePtr->entryFlags & ENTRY_OPTION_ACCEL) {
    	    	    *modWidthPtr += OPTION_ICON_WIDTH;
    	    	}
    	    	if (mePtr->entryFlags & ENTRY_COMMAND_ACCEL) {
    	    	    *modWidthPtr += COMMAND_ICON_WIDTH;
    	    	}
    	    	if (1 == (mePtr->accelLength - length)) {
    	    	    *textWidthPtr = emWidth;
    	    	} else {
    	    	    *textWidthPtr += Tk_TextWidth(tkfont, accel 
    		    	    + length, mePtr->accelLength - length);
    		}
    	    }
    	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetTearoffEntryGeometry --
 *
 *	Gets the width and height of of a tearoff entry.
 *
 * Results:
 *	widthPtr and heightPtr are set.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetTearoffEntryGeometry (
    TkMenu *menuPtr,			/* The menu we are drawing */
    TkMenuEntry *mePtr,			/* The entry we are measuring */
    Tk_Font tkfont,			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* The precalculated font metrics */
    int *widthPtr,			/* The resulting width */
    int *heightPtr)			/* The resulting height */
{
    if ((GetResource('MDEF', 591) == NULL) &&
	    (menuPtr->menuType == MASTER_MENU)) {
    	*heightPtr = fmPtr->linespace;
    	*widthPtr = 0;
    } else {
	*widthPtr = *heightPtr = 0;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetMenuSeparatorGeometry --
 *
 *	Gets the width and height of menu separator.
 *
 * Results:
 *	widthPtr and heightPtr are set.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetMenuSeparatorGeometry(
    TkMenu *menuPtr,			/* The menu we are drawing */
    TkMenuEntry *mePtr,			/* The entry we are measuring */
    Tk_Font tkfont,			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* The precalcualted font metrics */
    int *widthPtr,			/* The resulting width */
    int *heightPtr)			/* The resulting height */
{
    if (TkMacHaveAppearance() > 1) {
        SInt16 outHeight;
        
        GetThemeMenuSeparatorHeight(&outHeight);
        *widthPtr = 0;
        *heightPtr = outHeight;
    } else {
        *widthPtr = 0;
        *heightPtr = fmPtr->linespace;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryIndicator --
 *
 *	This procedure draws the indicator part of a menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */

static void
DrawMenuEntryIndicator(
    TkMenu *menuPtr,			/* The menu we are drawing */
    TkMenuEntry *mePtr,			/* The entry we are drawing */
    Drawable d,				/* The drawable we are drawing */
    GC gc,				/* The GC we are drawing with */
    GC indicatorGC,			/* The GC to use for the indicator */
    Tk_Font tkfont,			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* The precalculated font metrics */
    int x,				/* topleft hand corner of entry */
    int y,				/* topleft hand corner of entry */
    int width,				/* width of entry */
    int height)				/* height of entry */
{
    if ((mePtr->type == CHECK_BUTTON_ENTRY) || 
    	    (mePtr->type == RADIO_BUTTON_ENTRY)) {
    	if (mePtr->indicatorOn
    	    	&& (mePtr->entryFlags & ENTRY_SELECTED)) {
	    int baseline;
	    short markShort;
    
    	    baseline = y + (height + fmPtr->ascent - fmPtr->descent) / 2;
    	    GetItemMark(((MacMenu *) menuPtr->platformData)->menuHdl,
    		    mePtr->index + 1, &markShort);
            if (markShort != 0) {
	    	char markChar;
	    	char markCharUTF[TCL_UTF_MAX + 1];
	    	int dstWrote;
	    	
            	markChar = (char) markShort;
            	Tcl_ExternalToUtf(NULL, NULL, &markChar, 1, 0, NULL,
			markCharUTF, TCL_UTF_MAX + 1, NULL, &dstWrote, NULL);
		Tk_DrawChars(menuPtr->display, d, gc, tkfont, markCharUTF,
			dstWrote, x + 2, baseline);
            }
	}
    }    
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuBackground --
 *
 *	If Appearance is present, draws the Appearance background
 *
 * Results:
 *	Nothing
 *
 * Side effects:
 *	Commands are output to X to display the menu in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */
static void
DrawMenuBackground(
    Rect     *menuRectPtr,	/* The menu rect */
    Drawable d,			/* What we are drawing into */
    ThemeMenuType type			/* Type of menu */    
    )
{
    if (!TkMacHaveAppearance()) {
    	return;
    } else {
	CGrafPtr saveWorld;
	GDHandle saveDevice;
	GWorldPtr destPort;

	destPort = TkMacGetDrawablePort(d);
	GetGWorld(&saveWorld, &saveDevice);
	SetGWorld(destPort, NULL);
	TkMacSetUpClippingRgn(d);
	DrawThemeMenuBackground (menuRectPtr, type);
	SetGWorld(saveWorld, saveDevice);    
    	return;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawSICN --
 *
 *	Given a resource id and an index, loads the appropriate SICN
 *	and draws it into a given drawable using the given gc.
 *
 * Results:
 *	Returns 1 if the SICN was found, 0 if not found.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */
static int
DrawSICN(
    int resourceID,		    /* The resource # of the SICN table */
    int index,			    /* The index into the SICN table of the
    				     * icon we want. */
    Drawable d,			    /* What we are drawing into */
    GC gc,			    /* The GC to draw with */
    int x,			    /* The left hand coord of the SICN */
    int y)			    /* The top coord of the SICN */
{
    Handle sicnHandle = (Handle) GetResource('SICN', SICN_RESOURCE_NUMBER);
    
    if (NULL == sicnHandle) {
    	return 0;
    } else {
    	BitMap sicnBitmap;
	Rect destRect;
	CGrafPtr saveWorld;
	GDHandle saveDevice;
	GWorldPtr destPort;
	BitMapPtr destBitMap;
	RGBColor origForeColor, origBackColor, foreColor, backColor;

	HLock(sicnHandle);
	destPort = TkMacGetDrawablePort(d);
	GetGWorld(&saveWorld, &saveDevice);
	SetGWorld(destPort, NULL);
	TkMacSetUpClippingRgn(d);
	TkMacSetUpGraphicsPort(gc);
	GetForeColor(&origForeColor);
	GetBackColor(&origBackColor);
	
	if (TkSetMacColor(gc->foreground, &foreColor)) {
	    RGBForeColor(&foreColor);
	}
	
	if (TkSetMacColor(gc->background, &backColor)) {
	    RGBBackColor(&backColor);
	}

	SetRect(&destRect, x, y, x + SICN_HEIGHT, y + SICN_HEIGHT);
	sicnBitmap.baseAddr = (Ptr) (*sicnHandle) + index * SICN_HEIGHT
	    * SICN_ROWS;
	sicnBitmap.rowBytes = SICN_ROWS;
	SetRect(&sicnBitmap.bounds, 0, 0, 16, 16);
	destBitMap = &((GrafPtr) destPort)->portBits;
	CopyBits(&sicnBitmap, destBitMap, &sicnBitmap.bounds, &destRect, 
	    destPort->txMode, NULL);
	HUnlock(sicnHandle);
	RGBForeColor(&origForeColor);
	RGBBackColor(&origBackColor);
	SetGWorld(saveWorld, saveDevice);    
    	return 1;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryAccelerator --
 *
 *	This procedure draws the accelerator part of a menu. We
 *	need to decide what to draw here. Should we replace strings
 *	like "Control", "Command", etc?
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */

static void
DrawMenuEntryAccelerator(
    TkMenu *menuPtr,		    /* The menu we are drawing */
    TkMenuEntry *mePtr,		    /* The entry we are drawing */
    Drawable d,			    /* The drawable we are drawing in */
    GC gc,			    /* The gc to draw into */
    Tk_Font tkfont,		    /* The precalculated font */
    CONST Tk_FontMetrics *fmPtr,    /* The precalculated font metrics */
    Tk_3DBorder activeBorder,	    /* border for menu background */
    int x,			    /* The left side of the entry */
    int y,			    /* The top of the entry */
    int width,			    /* The width of the entry */
    int height,			    /* The height of the entry */
    int drawArrow)		    /* Whether or not to draw cascade arrow */
{
    int activeBorderWidth;
    
    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->activeBorderWidthPtr,
    	    &activeBorderWidth);
    if (mePtr->type == CASCADE_ENTRY) {
        /*
         * Under Appearance, we let the Appearance Manager draw the icon
         */
         
        if (!TkMacHaveAppearance()) {
    	if (0 == DrawSICN(SICN_RESOURCE_NUMBER, CASCADE_ARROW, d, gc,
    		x + width - SICN_HEIGHT, (y + (height / 2))
    		- (SICN_HEIGHT / 2))) {
	    XPoint points[3];
	    Tk_Window tkwin = menuPtr->tkwin;

	    if (mePtr->type == CASCADE_ENTRY) {
		points[0].x = width - activeBorderWidth
			- MAC_MARGIN_WIDTH - CASCADE_ARROW_WIDTH;
		points[0].y = y + (height - CASCADE_ARROW_HEIGHT)/2;
		points[1].x = points[0].x;
		points[1].y = points[0].y + CASCADE_ARROW_HEIGHT;
		points[2].x = points[0].x + CASCADE_ARROW_WIDTH;
		points[2].y = points[0].y + CASCADE_ARROW_HEIGHT/2;
		Tk_Fill3DPolygon(menuPtr->tkwin, d, activeBorder, points, 
			3, DECORATION_BORDER_WIDTH, TK_RELIEF_FLAT);
	    }
	}
	}
    } else if (mePtr->accelLength != 0) {
    	int leftEdge = x + width;
    	int baseline = y + (height + fmPtr->ascent - fmPtr->descent) / 2;
    	char *accel;
    	
    	accel = Tcl_GetStringFromObj(mePtr->accelPtr, NULL);

	if (NULL == GetResource('SICN', SICN_RESOURCE_NUMBER)) {
	    leftEdge -= ((EntryGeometry *) mePtr->platformEntryData)
	    	    ->accelTextWidth;
	    Tk_DrawChars(menuPtr->display, d, gc, tkfont, accel,
	    	    mePtr->accelLength, leftEdge, baseline);
	} else {
	    EntryGeometry *geometryPtr = 
	    	    (EntryGeometry *) mePtr->platformEntryData;
	    int length = mePtr->accelLength - geometryPtr->accelTextStart;
	    
	    leftEdge -= geometryPtr->accelTextWidth;
	    if ((mePtr->entryFlags & ENTRY_ACCEL_MASK) == 0) {
	    	leftEdge -= geometryPtr->modifierWidth;
	    }
	    
	    Tk_DrawChars(menuPtr->display, d, gc, tkfont, accel 
		    + geometryPtr->accelTextStart, length, leftEdge, baseline);

	    if (mePtr->entryFlags & ENTRY_COMMAND_ACCEL) {
	    	leftEdge -= COMMAND_ICON_WIDTH;
	    	DrawSICN(SICN_RESOURCE_NUMBER, COMMAND_ICON, d, gc,
	    		leftEdge, (y + (height / 2)) - (SICN_HEIGHT / 2) - 1);
	    }

	    if (mePtr->entryFlags & ENTRY_OPTION_ACCEL) {
	    	leftEdge -= OPTION_ICON_WIDTH;
	    	DrawSICN(SICN_RESOURCE_NUMBER, OPTION_ICON, d, gc,
	    		leftEdge, (y + (height / 2)) - (SICN_HEIGHT / 2) - 1);
	    }

	    if (mePtr->entryFlags & ENTRY_SHIFT_ACCEL) {
	    	leftEdge -= SHIFT_ICON_WIDTH;
	    	DrawSICN(SICN_RESOURCE_NUMBER, SHIFT_ICON, d, gc,
	    		leftEdge, (y + (height / 2)) - (SICN_HEIGHT / 2) - 1);
	    }

	    if (mePtr->entryFlags & ENTRY_CONTROL_ACCEL) {
	    	leftEdge -= CONTROL_ICON_WIDTH;
	    	DrawSICN(SICN_RESOURCE_NUMBER, CONTROL_ICON, d, gc,
	    		leftEdge, (y + (height / 2)) - (SICN_HEIGHT / 2) - 1);
	    }
    	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuSeparator --
 *
 *	The menu separator is drawn.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */

static void
DrawMenuSeparator(
    TkMenu *menuPtr,			/* The menu we are drawing */
    TkMenuEntry *mePtr,			/* The entry we are drawing */
    Drawable d,				/* The drawable we are drawing into */
    GC gc,				/* The gc we are drawing with */
    Tk_Font tkfont,			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* The precalculated font metrics */
    int x,				/* left coordinate of entry */
    int y,				/* top coordinate of entry */
    int width,				/* width of entry */
    int height)				/* height of entry */
{
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
   
    destPort = TkMacGetDrawablePort(d);
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    TkMacSetUpClippingRgn(d);
    if (TkMacHaveAppearance() > 1) {
        Rect r;
        r.top = y;
        r.left = x;
        r.bottom = y + height;
        r.right = x + width;
         
        DrawThemeMenuSeparator(&r);
    } else {
    /*
     * We don't want to use the text GC for drawing the separator. It
     * needs to be the same color as disabled items.
     */
    
    TkMacSetUpGraphicsPort(mePtr->disabledGC != None ? mePtr->disabledGC
    	    : menuPtr->disabledGC);
    
    MoveTo(x, y + (height / 2));
    Line(width, 0);
    
    SetGWorld(saveWorld, saveDevice);
}
}

/*
 *----------------------------------------------------------------------
 *
 * MenuDefProc --
 *
 *	This routine is the MDEF handler for Tk. It receives all messages
 *	for the menu and dispatches them.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This routine causes menus to be drawn and will certainly allocate
 *	memory as a result. Also, the menu can scroll up and down, and
 *	various other interface actions can take place.
 *
 *----------------------------------------------------------------------
 */

static void
MenuDefProc(
    short message,			/* What action are we taking? */
    MenuHandle menu,			/* The menu we are working with */
    Rect *menuRectPtr,			/* A pointer to the rect for the
    					 * whole menu. */
    Point hitPt,			/* Where the mouse was clicked for
    					 * the appropriate messages. */
    short *whichItem,			/* Output result. Which item was
    					 * hit by the user? */
    TkMenuLowMemGlobals *globalsPtr)	/* The low mem globals we have
    					 * to change */
{
#define SCREEN_MARGIN 5
    TkMenu *menuPtr;
    TkMenuEntry *parentEntryPtr;
    Tcl_HashEntry *commandEntryPtr;
    GrafPtr windowMgrPort;
    Tk_Font tkfont, menuFont;
    Tk_FontMetrics fontMetrics, entryMetrics;
    Tk_FontMetrics *fmPtr;
    TkMenuEntry *mePtr;
    int i;
    int maxMenuHeight;
    int oldItem;
    int newItem = -1;
    GDHandle device;
    Rect itemRect;
    short windowPart;
    WindowRef whichWindow;
    RGBColor bgColor;
    RGBColor fgColor;
    RGBColor origFgColor;
    PenState origPenState;
    Rect dragRect;
    Rect scratchRect = {-32768, -32768, 32767, 32767};
    RgnHandle oldClipRgn;
    TkMenuReferences *menuRefPtr;
    TkMenu *searchMenuPtr;
    Rect menuClipRect;
    
    HLock((Handle) menu);
    commandEntryPtr = Tcl_FindHashEntry(&commandTable,
    	    (char *) (*menu)->menuID);
    HUnlock((Handle) menu);
    menuPtr = (TkMenu *) Tcl_GetHashValue(commandEntryPtr);

    switch (message) {
    	case mSizeMsg:
	    GetWMgrPort(&windowMgrPort);
    	    maxMenuHeight = windowMgrPort->portRect.bottom
    	    	    - windowMgrPort->portRect.top
    	    	    - GetMBarHeight() - SCREEN_MARGIN;
    	    (*menu)->menuWidth = menuPtr->totalWidth;
    	    (*menu)->menuHeight = maxMenuHeight < menuPtr->totalHeight ?
    	    	    maxMenuHeight : menuPtr->totalHeight;
       	    break;

    	case mDrawMsg:
    	
    	    /*
    	     * Store away the menu rectangle so we can keep track of the
    	     * different regions that the menu obscures.
    	     */
    	    
    	    ((MacMenu *) menuPtr->platformData)->menuRect = *menuRectPtr;
	    if (tkMenuCascadeRgn == NULL) {
		tkMenuCascadeRgn = NewRgn();
	    }
	    if (utilRgn == NULL) {
		utilRgn = NewRgn();
	    }
	    if (totalMenuRgn == NULL) {
	    	totalMenuRgn = NewRgn();
	    }
            SetEmptyRgn(tkMenuCascadeRgn);
	    for (searchMenuPtr = menuPtr; searchMenuPtr != NULL; ) {
		RectRgn(utilRgn, 
		    	&((MacMenu *) searchMenuPtr->platformData)->menuRect);
		InsetRgn(utilRgn, -1, -1);
		UnionRgn(tkMenuCascadeRgn, utilRgn, tkMenuCascadeRgn);
		OffsetRgn(utilRgn, 1, 1);
		UnionRgn(tkMenuCascadeRgn, utilRgn, tkMenuCascadeRgn);
		
		if (searchMenuPtr->menuRefPtr->parentEntryPtr != NULL) {
		    searchMenuPtr = searchMenuPtr->menuRefPtr
			    ->parentEntryPtr->menuPtr;
		} else {
		    break;
		}
		if (searchMenuPtr->menuType == MENUBAR) {
		    break;
		}
	    }
	    UnionRgn(totalMenuRgn, tkMenuCascadeRgn, totalMenuRgn);
	    SetEmptyRgn(utilRgn);
	    
	    /*
	     * Now draw the background if Appearance is present...
	     */
	     
    	    GetGWorld(&macMDEFDrawable.portPtr, &device);
	    if (TkMacHaveAppearance() > 1) {
	        ThemeMenuType menuType;
	        
	        if (menuPtr->menuRefPtr->topLevelListPtr != NULL) {
	            menuType = kThemeMenuTypePullDown;
	        } else if (menuPtr->menuRefPtr->parentEntryPtr != NULL) {
	            menuType = kThemeMenuTypeHierarchical;
	        } else {
	            menuType = kThemeMenuTypePopUp;
	        }
	            
	        DrawMenuBackground(menuRectPtr, (Drawable) &macMDEFDrawable, 
	        	menuType);
	    }
	    
	    /*
	     * Next, figure out scrolling information.
	     */
	    
	    menuClipRect = *menuRectPtr;
	    if ((menuClipRect.bottom - menuClipRect.top) 
	    	    < menuPtr->totalHeight) {
	 	if (globalsPtr->menuTop < menuRectPtr->top) {
	 	    DrawSICN(SICN_RESOURCE_NUMBER, UP_ARROW, 
	 	    	    (Drawable) &macMDEFDrawable,
	 	    	    menuPtr->textGC, 
	 	    	    menuRectPtr->left 
	 	    	    + menuPtr->entries[1]->indicatorSpace,
	 	    	    menuRectPtr->top);
	 	    menuClipRect.top += SICN_HEIGHT;
	 	}
	 	if ((globalsPtr->menuTop + menuPtr->totalHeight)
	 		> menuRectPtr->bottom) {
	 	    DrawSICN(SICN_RESOURCE_NUMBER, DOWN_ARROW,
	 	    	    (Drawable) &macMDEFDrawable,
	 	    	    menuPtr->textGC, 
	 	    	    menuRectPtr->left 
	 	    	    + menuPtr->entries[1]->indicatorSpace,
	 	    	    menuRectPtr->bottom - SICN_HEIGHT);
	 	    menuClipRect.bottom -= SICN_HEIGHT;
	 	}
	 	GetClip(utilRgn);
	    }
	    
	    /*
	     * Now, actually draw the menu. Don't draw entries that
	     * are higher than the top arrow, and don't draw entries
	     * that are lower than the bottom.
	     */
	    
	    menuFont = Tk_GetFontFromObj(menuPtr->tkwin, menuPtr->fontPtr);
	    Tk_GetFontMetrics(menuFont, &fontMetrics);    	    
    	    for (i = 0; i < menuPtr->numEntries; i++) {
    	        mePtr = menuPtr->entries[i];
    	    	if (globalsPtr->menuTop + mePtr->y + mePtr->height
    	    		< menuClipRect.top) {
    	    	    continue;
    	    	} else if (globalsPtr->menuTop + mePtr->y
    	    		> menuClipRect.bottom) {
    	    	    continue;
    	    	}
	 	/* ClipRect(&menuClipRect); */
    	    	if (mePtr->fontPtr == NULL) {
    	    	    fmPtr = &fontMetrics;
    	    	    tkfont = menuFont;
    	    	} else {
		    tkfont = Tk_GetFontFromObj(menuPtr->tkwin, mePtr->fontPtr);
    	    	    Tk_GetFontMetrics(tkfont, &entryMetrics);
    	    	    fmPtr = &entryMetrics;
    	    	}
    	    	AppearanceEntryDrawWrapper(mePtr, menuRectPtr, globalsPtr,
    	    		(Drawable) &macMDEFDrawable, fmPtr, tkfont, 
    	    		menuRectPtr->left + mePtr->x,
     	    		globalsPtr->menuTop + mePtr->y,
   	    		(mePtr->entryFlags & ENTRY_LAST_COLUMN) ?
	        	    menuPtr->totalWidth - mePtr->x : mePtr->width,
	        	menuPtr->entries[i]->height);
     	    }
     	    globalsPtr->menuBottom = globalsPtr->menuTop 
     	    	    + menuPtr->totalHeight;
	    if (!EmptyRgn(utilRgn)) {
	    	SetClip(utilRgn);
	    	SetEmptyRgn(utilRgn);
	    }
	    MDEFScrollFlag = 1;
    	    break;

    	case mChooseMsg: {
    	    int hasTopScroll, hasBottomScroll;
    	    enum {
    	    	DONT_SCROLL, DOWN_SCROLL, UP_SCROLL
    	    } scrollDirection;
    	    Rect updateRect;
    	    short scrollAmt;
    	    RGBColor origForeColor, origBackColor, foreColor, backColor;
    	    
 	    GetGWorld(&macMDEFDrawable.portPtr, &device);
 	    GetForeColor(&origForeColor);
 	    GetBackColor(&origBackColor);

	    if (TkSetMacColor(menuPtr->textGC->foreground, 
	    	    &foreColor)) {
	    	/* if (!TkMacHaveAppearance()) { */
	    	    RGBForeColor(&foreColor);
	    	/* } */
	    }
	    if (TkSetMacColor(menuPtr->textGC->background, 
	    	    &backColor)) {
	    	/* if (!TkMacHaveAppearance()) { */
	    	    RGBBackColor(&backColor);
	    	/* } */
	    }

	    /*
	     * Find out which item was hit. If it is the same as the old item,
	     * we don't need to do anything.
	     */

	    oldItem = *whichItem - 1;
	     
	    if (PtInRect(hitPt, menuRectPtr)) {
	    	for (i = 0; i < menuPtr->numEntries; i++) {
	    	    mePtr = menuPtr->entries[i];
	    	    itemRect.left = menuRectPtr->left + mePtr->x;
	    	    itemRect.top = globalsPtr->menuTop + mePtr->y;
	    	    if (mePtr->entryFlags & ENTRY_LAST_COLUMN) {
	    	    	itemRect.right = itemRect.left + menuPtr->totalWidth
	    	    		- mePtr->x;
	    	    } else {
	    	    	itemRect.right = itemRect.left + mePtr->width;
	    	    }
	    	    itemRect.bottom = itemRect.top
			    + menuPtr->entries[i]->height;
	    	    if (PtInRect(hitPt, &itemRect)) {
	    	        if ((mePtr->type == SEPARATOR_ENTRY)
	    	        	|| (mePtr->state == ENTRY_DISABLED)) {
	    	            newItem = -1;
	    	        } else {
	    	            TkMenuEntry *cascadeEntryPtr;
	    	            int parentDisabled = 0;
	    	            
	    	            for (cascadeEntryPtr
				    = menuPtr->menuRefPtr->parentEntryPtr;
	    	            	    cascadeEntryPtr != NULL;
	    	            	    cascadeEntryPtr 
	    	            	    = cascadeEntryPtr->nextCascadePtr) {
	    	            	char *name;
	    	            	
	    	            	name = Tcl_GetStringFromObj(
	    	            		cascadeEntryPtr->namePtr, NULL);
				if (strcmp(name, Tk_PathName(menuPtr->tkwin)) 
					== 0) {
				    if (cascadeEntryPtr->state == ENTRY_DISABLED) {
				    	parentDisabled = 1;
				    }
				    break;
				}
			    }
			    if (parentDisabled) {
			    	newItem = -1;
			    } else {         	    
	    	            	newItem = i;
	    	            	if ((mePtr->type == CASCADE_ENTRY) 
	    	            		&& (oldItem != newItem)) {
		    	    	    globalsPtr->itemRect = itemRect;
		    	    	}
		    	    }
	    	        }
	    	        break;
	    	    }
	    	}
	    }

	    /*
	     * Now we need to take care of scrolling the menu.
	     */
	    
	    hasTopScroll = globalsPtr->menuTop < menuRectPtr->top;
	    hasBottomScroll = globalsPtr->menuBottom > menuRectPtr->bottom;
	    scrollDirection = DONT_SCROLL;
	    if (hasTopScroll 
	            && (hitPt.v < menuRectPtr->top + SICN_HEIGHT)) {
		newItem = -1;
		scrollDirection = DOWN_SCROLL;
	    } else if (hasBottomScroll
		    && (hitPt.v > menuRectPtr->bottom - SICN_HEIGHT)) {
		newItem = -1;
		scrollDirection = UP_SCROLL;
	    }	        
	    menuClipRect = *menuRectPtr;
	    if (hasTopScroll) {
	    	menuClipRect.top += SICN_HEIGHT;
	    }
	    if (hasBottomScroll) {
	    	menuClipRect.bottom -= SICN_HEIGHT;
	    }
	    if (MDEFScrollFlag) {
	    	scrollDirection = DONT_SCROLL;
	    	MDEFScrollFlag = 0;
	    }
	    GetClip(utilRgn);
	    ClipRect(&menuClipRect);

	    if (oldItem != newItem) {
	        if (oldItem >= 0) {
		    mePtr = menuPtr->entries[oldItem];
		    if (mePtr->fontPtr == NULL) {
			tkfont = Tk_GetFontFromObj(menuPtr->tkwin, 
				menuPtr->fontPtr);
		    } else {
			tkfont = Tk_GetFontFromObj(menuPtr->tkwin,
				mePtr->fontPtr);
		    }
		    Tk_GetFontMetrics(tkfont, &fontMetrics);
    	    	    AppearanceEntryDrawWrapper(mePtr, menuRectPtr, globalsPtr,
    	    		(Drawable) &macMDEFDrawable, &fontMetrics, tkfont, 
    	    		menuRectPtr->left + mePtr->x,
     	    		globalsPtr->menuTop + mePtr->y,
   	    		(mePtr->entryFlags & ENTRY_LAST_COLUMN) ?
	        	    menuPtr->totalWidth - mePtr->x : mePtr->width,
	        	mePtr->height);
		}
		if (newItem != -1) {
		    int oldActiveItem = menuPtr->active;
		    
		    mePtr = menuPtr->entries[newItem];
		    if (mePtr->state != ENTRY_DISABLED) {
		    	TkActivateMenuEntry(menuPtr, newItem);
		    }
		    if (mePtr->fontPtr == NULL) {
			tkfont = Tk_GetFontFromObj(menuPtr->tkwin, 
				menuPtr->fontPtr);
		    } else {
			tkfont = Tk_GetFontFromObj(menuPtr->tkwin,
				mePtr->fontPtr);
		    }
		    Tk_GetFontMetrics(tkfont, &fontMetrics);
    	    	    AppearanceEntryDrawWrapper(mePtr, menuRectPtr, globalsPtr,
    	    		(Drawable) &macMDEFDrawable, &fontMetrics, tkfont, 
    	    		menuRectPtr->left + mePtr->x,
     	    		globalsPtr->menuTop + mePtr->y,
   	    		(mePtr->entryFlags & ENTRY_LAST_COLUMN) ?
	        	    menuPtr->totalWidth - mePtr->x : mePtr->width,
	        	mePtr->height);
		}

		tkUseMenuCascadeRgn = 1;
		MenuSelectEvent(menuPtr);
		Tcl_ServiceAll();
		tkUseMenuCascadeRgn = 0;
		if (mePtr->state != ENTRY_DISABLED) {
		    TkActivateMenuEntry(menuPtr, -1);
		}
	    	*whichItem = newItem + 1;
	    }
	    globalsPtr->menuDisable = ((*menu)->menuID << 16) | (newItem + 1);
	    
	    if (scrollDirection == UP_SCROLL) {
	    	scrollAmt = menuClipRect.bottom - hitPt.v;
	    	if (scrollAmt < menuRectPtr->bottom 
	    		- globalsPtr->menuBottom) {
	    	    scrollAmt = menuRectPtr->bottom - globalsPtr->menuBottom;
	    	}
	    	if (!hasTopScroll && ((globalsPtr->menuTop + scrollAmt)
			< menuRectPtr->top)) {
	    	    SetRect(&updateRect, menuRectPtr->left,
	    	    	    globalsPtr->menuTop, menuRectPtr->right,
	    	    	    globalsPtr->menuTop + SICN_HEIGHT);
	    	    EraseRect(&updateRect);
	    	    DrawSICN(SICN_RESOURCE_NUMBER, UP_ARROW,
	    	    	    (Drawable) &macMDEFDrawable,
	    	    	    menuPtr->textGC, menuRectPtr->left
	    	    	    + menuPtr->entries[1]->indicatorSpace,
	    	    	    menuRectPtr->top);
	    	    menuClipRect.top += SICN_HEIGHT;
	    	}
	    } else if (scrollDirection == DOWN_SCROLL) {
	    	scrollAmt = menuClipRect.top - hitPt.v;
	    	if (scrollAmt > menuRectPtr->top - globalsPtr->menuTop) {
	    	    scrollAmt = menuRectPtr->top - globalsPtr->menuTop;
	    	}
	    	if (!hasBottomScroll && ((globalsPtr->menuBottom + scrollAmt)
	    		> menuRectPtr->bottom)) {
	    	    SetRect(&updateRect, menuRectPtr->left, 
	    	    	    globalsPtr->menuBottom - SICN_HEIGHT,
	    	    	    menuRectPtr->right, globalsPtr->menuBottom);
	    	    EraseRect(&updateRect);
	    	    DrawSICN(SICN_RESOURCE_NUMBER, DOWN_ARROW,
	    	    	    (Drawable) &macMDEFDrawable,
	    	    	    menuPtr->textGC, menuRectPtr->left
	    	    	    + menuPtr->entries[1]->indicatorSpace,
	    	    	    menuRectPtr->bottom - SICN_HEIGHT);
	    	    menuClipRect.bottom -= SICN_HEIGHT;
	    	}
	    }
	    if (scrollDirection != DONT_SCROLL) {
	    	Tk_Font menuFont;
	    	RgnHandle updateRgn = NewRgn();
	    	ScrollRect(&menuClipRect, 0, scrollAmt, updateRgn);
	    	updateRect = (*updateRgn)->rgnBBox;
	    	DisposeRgn(updateRgn);
	    	globalsPtr->menuTop += scrollAmt;
	    	globalsPtr->menuBottom += scrollAmt;
	    	if (globalsPtr->menuTop == menuRectPtr->top) {
	    	    updateRect.top -= SICN_HEIGHT;
	    	}
	    	if (globalsPtr->menuBottom == menuRectPtr->bottom) {
	    	    updateRect.bottom += SICN_HEIGHT;
	    	}
		ClipRect(&updateRect);
		EraseRect(&updateRect);
		menuFont = Tk_GetFontFromObj(menuPtr->tkwin, menuPtr->fontPtr);
	    	Tk_GetFontMetrics(menuFont, &fontMetrics);    	    
    	    	for (i = 0; i < menuPtr->numEntries; i++) {
    	            mePtr = menuPtr->entries[i];
    	    	    if (globalsPtr->menuTop + mePtr->y + mePtr->height
    	    		    < updateRect.top) {
    	    	    	continue;
    	    	    } else if (globalsPtr->menuTop + mePtr->y
    	    		    > updateRect.bottom) {
    	    	    	continue;
    	    	    }
    	    	    if (mePtr->fontPtr == NULL) {
    	    	    	fmPtr = &fontMetrics;
    	    	    	tkfont = menuFont;
    	    	    } else {
			tkfont = Tk_GetFontFromObj(menuPtr->tkwin,
				mePtr->fontPtr);
    	    	    	Tk_GetFontMetrics(tkfont, &entryMetrics);
    	    	    	fmPtr = &entryMetrics;
    	    	    }
    	    	    AppearanceEntryDrawWrapper(mePtr, menuRectPtr, globalsPtr,
    	    		(Drawable) &macMDEFDrawable, fmPtr, tkfont, 
    	    		menuRectPtr->left + mePtr->x,
     	    		globalsPtr->menuTop + mePtr->y,
   	    		(mePtr->entryFlags & ENTRY_LAST_COLUMN) ?
	        	    menuPtr->totalWidth - mePtr->x : mePtr->width,
	        	menuPtr->entries[i]->height);
     	    	}	    	
	    }

	    SetClip(utilRgn);
	    SetEmptyRgn(utilRgn);
	    RGBForeColor(&origForeColor);
	    RGBBackColor(&origBackColor);

    	    /*
    	     * If the menu is a tearoff, and the mouse is outside the menu,
    	     * we need to draw the drag rectangle.
    	     *
    	     * In order for tearoffs to work properly, we need to set
    	     * the active member of the containing menubar.
    	     */
    	    
    	    menuRefPtr = TkFindMenuReferences(menuPtr->interp,
    	    	    Tk_PathName(menuPtr->tkwin));
    	    if ((menuRefPtr != NULL) && (menuRefPtr->parentEntryPtr != NULL)) {
    	    	char *name;
    	    	for (parentEntryPtr = menuRefPtr->parentEntryPtr;
    	    	        parentEntryPtr != NULL
    	    	    	; parentEntryPtr = parentEntryPtr->nextCascadePtr) {
		    name = Tcl_GetStringFromObj(parentEntryPtr->namePtr,
			    NULL);
		    if (strcmp(name, Tk_PathName(menuPtr->tkwin)) != 0) {
		        break;
		    }
    	    	}
    	    	if (parentEntryPtr != NULL) {
    	    	    TkActivateMenuEntry(parentEntryPtr->menuPtr,
    	    	    	    parentEntryPtr->index);
	 	}
    	    }
    	    
	    if (menuPtr->tearoff) {
   	    	scratchRect = *menuRectPtr;
		if (tearoffStruct.menuPtr == NULL) {
   	    	    scratchRect.top -= 10;
   	    	    scratchRect.bottom += 10;
   	    	    scratchRect.left -= 10;
   	    	    scratchRect.right += 10;
   	    	}

	    	windowPart = FindWindow(hitPt, &whichWindow);
	    	if ((windowPart != inMenuBar) && (newItem == -1)
	    		&& (hitPt.v != 0) && (hitPt.h != 0)
	    		&& (!PtInRect(hitPt, &scratchRect))
	    		&& (!PtInRect(hitPt, &tearoffStruct.excludeRect))) {
/*
 * This is the second argument to the Toolbox Delay function.  It changed
 * from long to unsigned long between Universal Headers 2.0 & 3.0
 */
#if !defined(UNIVERSAL_INTERFACES_VERSION) || (UNIVERSAL_INTERFACES_VERSION < 0x0300)
	    	    long dummy;
#else
                    unsigned long dummy;
#endif	    	    
	    	    oldClipRgn = NewRgn();
	    	    GetClip(oldClipRgn);
   	    	    GetForeColor(&origFgColor);
   	    	    GetPenState(&origPenState);
   	    	    GetForeColor(&fgColor);
   	    	    GetBackColor(&bgColor);
   	    	    GetGray(device, &bgColor, &fgColor);
   	    	    RGBForeColor(&fgColor);
   	    	    SetRect(&scratchRect, -32768, -32768, 32767, 32767);
   	    	    ClipRect(&scratchRect);
   	    	    
    	            dragRect = *menuRectPtr;
		    tearoffStruct.menuPtr = menuPtr;

	            PenMode(srcXor);
   	    	    dragRect = *menuRectPtr;
	            OffsetRect(&dragRect, -dragRect.left, -dragRect.top);
	            OffsetRect(&dragRect, tearoffStruct.point.h,
	        	    tearoffStruct.point.v);
	       	    if ((dragRect.top != 0) && (dragRect.left != 0)) {
	            	FrameRect(&dragRect);
	            	Delay(1, &dummy);
		    	FrameRect(&dragRect);
		    }
		    tearoffStruct.point = hitPt;

	    	    SetClip(oldClipRgn);
	    	    DisposeRgn(oldClipRgn);
	    	    RGBForeColor(&origFgColor);
	    	    SetPenState(&origPenState);    
		} else {
		    tearoffStruct.menuPtr = NULL;
		    tearoffStruct.point.h = tearoffStruct.point.v = 0;
		}
	    } else {
	    	tearoffStruct.menuPtr = NULL;
	    	tearoffStruct.point.h = tearoffStruct.point.v = 0;
	    }
	    
	    break;
	}

    	case mPopUpMsg:
    	
    	    /*
    	     * Note that for some oddball reason, h and v are reversed in the
    	     * point given to us by the MDEF.
    	     */
    	
	    oldItem = *whichItem;
	    if (oldItem >= menuPtr->numEntries) {
	        oldItem = -1;
	    }
	    GetWMgrPort(&windowMgrPort);
	    maxMenuHeight = windowMgrPort->portRect.bottom
	    	    - windowMgrPort->portRect.top
	    	    - GetMBarHeight() - SCREEN_MARGIN;
	    if (menuPtr->totalHeight > maxMenuHeight) {
	    	menuRectPtr->top = GetMBarHeight();
	    } else {
	    	int delta;
	    	menuRectPtr->top = hitPt.h;
	    	if (oldItem >= 0) {
	    	    menuRectPtr->top -= menuPtr->entries[oldItem]->y;
	    	}
	    	
	    	if (menuRectPtr->top < GetMBarHeight()) {
	    	    /* Displace downward if the menu would stick off the
	    	     * top of the screen.
	    	     */
	    	     
	    	    menuRectPtr->top = GetMBarHeight() + SCREEN_MARGIN;
	    	} else {
	    	    /*
	    	     * Or upward if the menu sticks off the 
	    	     * bottom end...
	    	     */
	    	     
	    	    delta = menuRectPtr->top + menuPtr->totalHeight 
	    	            - maxMenuHeight;
	    	    if (delta > 0) {
	    	        menuRectPtr->top -= delta;
	    	    }
	    	}
	    }
    	    menuRectPtr->left = hitPt.v;
    	    menuRectPtr->right = menuRectPtr->left + menuPtr->totalWidth;
    	    menuRectPtr->bottom = menuRectPtr->top + 
    	    	    ((maxMenuHeight < menuPtr->totalHeight) 
    	    	    ? maxMenuHeight : menuPtr->totalHeight);
    	    if (menuRectPtr->top == GetMBarHeight()) {
    	    	*whichItem = hitPt.h;
    	    } else {
	    	*whichItem = menuRectPtr->top;
	    }
	    globalsPtr->menuTop = *whichItem;
	    globalsPtr->menuBottom = menuRectPtr->bottom;
    	    break;
    }
}

/*
 *----------------------------------------------------------------------
 *
 *   AppearanceEntryDrawWrapper --
 *
 *	This routine wraps the TkpDrawMenuEntry function.  Under Appearance, 
 *      it routes to the Appearance Managers DrawThemeEntry, otherwise it
 *      just goes straight to TkpDrawMenuEntry.
 *
 * Results:
 *	A menu entry is drawn
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */
static void 
AppearanceEntryDrawWrapper(
    TkMenuEntry *mePtr,
    Rect *menuRectPtr,
    TkMenuLowMemGlobals *globalsPtr,     
    Drawable d,
    Tk_FontMetrics *fmPtr, 
    Tk_Font tkfont, 
    int x, 
    int y, 
    int width, 
    int height)
{
    if (TkMacHaveAppearance() > 1) {
        MenuEntryUserData meData;
        Rect itemRect;
        ThemeMenuState theState;
        ThemeMenuItemType theType;
    
        meData.mePtr = mePtr;
        meData.mdefDrawable = d;
        meData.fmPtr = fmPtr;
        meData.tkfont = tkfont;
    
        itemRect.top = y;
        itemRect.left = x;
        itemRect.bottom = itemRect.top + height;
        itemRect.right = itemRect.left + width;
    
        if (mePtr->state == ENTRY_ACTIVE) {
            theState = kThemeMenuSelected;
        } else if (mePtr->state == ENTRY_DISABLED) {
    	    theState = kThemeMenuDisabled;
        } else {
    	    theState = kThemeMenuActive;
        }
        
        if (mePtr->type == CASCADE_ENTRY) {
            theType = kThemeMenuItemHierarchical;
        } else {
            theType = kThemeMenuItemPlain;
        }
        
        DrawThemeMenuItem (menuRectPtr, &itemRect,
    	        globalsPtr->menuTop, globalsPtr->menuBottom, theState,
    	        theType, tkThemeMenuItemDrawingUPP, 
    	        (unsigned long) &meData);
    	
    } else {
        TkpDrawMenuEntry(mePtr, d, tkfont, fmPtr,
	        x, y, width, height, 0, 1);
    }
}

/*
 *----------------------------------------------------------------------
 *
 *  tkThemeMenuItemDrawingProc --
 *
 *	This routine is called from the Appearance DrawThemeMenuEntry
 *
 * Results:
 *	A menu entry is drawn
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */
pascal void
tkThemeMenuItemDrawingProc (
	const Rect *inBounds,
	SInt16 inDepth, 
	Boolean inIsColorDevice, 
	SInt32 inUserData)
{
    MenuEntryUserData *meData = (MenuEntryUserData *) inUserData;

    TkpDrawMenuEntry(meData->mePtr, meData->mdefDrawable,
    	 meData->tkfont, meData->fmPtr, inBounds->left, 
    	 inBounds->top, inBounds->right - inBounds->left,
    	 inBounds->bottom - inBounds->top, 0, 1);

}

/*
 *----------------------------------------------------------------------
 *
 * TkMacHandleTearoffMenu() --
 *
 *	This routine sees if the MDEF has set a menu and a mouse position
 *	for tearing off and makes a tearoff menu if it has.
 *
 * Results:
 *	menuPtr->interp will have the result of the tearoff command.
 *
 * Side effects:
 *	A new tearoff menu is created if it is supposed to be.
 *
 *----------------------------------------------------------------------
 */

void
TkMacHandleTearoffMenu(void)
{
    if (tearoffStruct.menuPtr != NULL) {
    	Tcl_DString tearoffCmdStr;
    	char intString[TCL_INTEGER_SPACE];
    	short windowPart;
    	WindowRef whichWindow;
    	
    	windowPart = FindWindow(tearoffStruct.point, &whichWindow);
    	
    	if (windowPart != inMenuBar) {
    	    Tcl_DStringInit(&tearoffCmdStr);
    	    Tcl_DStringAppendElement(&tearoffCmdStr, "tkTearOffMenu");
    	    Tcl_DStringAppendElement(&tearoffCmdStr, 
    		    Tk_PathName(tearoffStruct.menuPtr->tkwin));
	    sprintf(intString, "%d", tearoffStruct.point.h);
	    Tcl_DStringAppendElement(&tearoffCmdStr, intString);
	    sprintf(intString, "%d", tearoffStruct.point.v);
	    Tcl_DStringAppendElement(&tearoffCmdStr, intString);
	    Tcl_Eval(tearoffStruct.menuPtr->interp,
		    Tcl_DStringValue(&tearoffCmdStr));
	    Tcl_DStringFree(&tearoffCmdStr);
	    tearoffStruct.menuPtr = NULL;
	}
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkpInitializeMenuBindings --
 *
 *	For every interp, initializes the bindings for Windows
 *	menus. Does nothing on Mac or XWindows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	C-level bindings are setup for the interp which will
 *	handle Alt-key sequences for menus without beeping
 *	or interfering with user-defined Alt-key bindings.
 *
 *--------------------------------------------------------------
 */

void
TkpInitializeMenuBindings(interp, bindingTable)
    Tcl_Interp *interp;		    /* The interpreter to set. */
    Tk_BindingTable bindingTable;   /* The table to add to. */
{
    /*
     * Nothing to do.
     */
}

/*
 *--------------------------------------------------------------
 *
 * TkpComputeMenubarGeometry --
 *
 *	This procedure is invoked to recompute the size and
 *	layout of a menu that is a menubar clone.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fields of menu entries are changed to reflect their
 *	current positions, and the size of the menu window
 *	itself may be changed.
 *
 *--------------------------------------------------------------
 */

void
TkpComputeMenubarGeometry(menuPtr)
    TkMenu *menuPtr;		/* Structure describing menu. */
{
    TkpComputeStandardMenuGeometry(menuPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DrawTearoffEntry --
 *
 *	This procedure draws the background part of a menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */

void
DrawTearoffEntry(
    TkMenu *menuPtr,			/* The menu we are drawing */
    TkMenuEntry *mePtr,			/* The entry we are drawing */
    Drawable d,				/* The drawable we are drawing into */
    GC gc,				/* The gc we are drawing with */
    Tk_Font tkfont,			/* The font we are drawing with */
    CONST Tk_FontMetrics *fmPtr,	/* The metrics we are drawing with */
    int x,				/* Left edge of entry. */
    int y,				/* Top edge of entry. */
    int width,				/* Width of entry. */
    int height)				/* Height of entry. */
{
    XPoint points[2];
    int margin, segmentWidth, maxX;
    Tk_3DBorder border;

    if ((menuPtr->menuType != MASTER_MENU) || (FixMDEF() != NULL)) {
	return;
    }
    
    margin = (fmPtr->ascent + fmPtr->descent)/2;
    points[0].x = x;
    points[0].y = y + height/2;
    points[1].y = points[0].y;
    segmentWidth = 6;
    maxX  = width - 1;
    border = Tk_Get3DBorderFromObj(menuPtr->tkwin, menuPtr->borderPtr);

    while (points[0].x < maxX) {
	points[1].x = points[0].x + segmentWidth;
	if (points[1].x > maxX) {
	    points[1].x = maxX;
	}
	Tk_Draw3DPolygon(menuPtr->tkwin, d, border, points, 2, 1,
		TK_RELIEF_RAISED);
	points[0].x += 2*segmentWidth;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacSetHelpMenuItemCount --
 *
 *	Has to be called after the first call to InsertMenu. Sets
 *	up the global variable for the number of items in the
 *	unmodified help menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Sets the global helpItemCount.
 *
 *----------------------------------------------------------------------
 */

void 
TkMacSetHelpMenuItemCount()
{
    MenuHandle helpMenuHandle;
    
    if ((HMGetHelpMenuHandle(&helpMenuHandle) != noErr) 
    	    || (helpMenuHandle == NULL)) {
    	helpItemCount = -1;
    } else {
    	helpItemCount = CountMItems(helpMenuHandle);
        DeleteMenuItem(helpMenuHandle, helpItemCount);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacMenuClick --
 *
 *	Prepares a menubar for MenuSelect or MenuKey.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Any pending configurations of the menubar are completed.
 *
 *----------------------------------------------------------------------
 */

void
TkMacMenuClick()
{
    TkMenu *menuPtr;
    TkMenuReferences *menuRefPtr;
    
    if ((currentMenuBarInterp != NULL) && (currentMenuBarName != NULL)) {
    	menuRefPtr = TkFindMenuReferences(currentMenuBarInterp,
    		currentMenuBarName);
    	for (menuPtr = menuRefPtr->menuPtr->masterMenuPtr;
    		menuPtr != NULL; menuPtr = menuPtr->nextInstancePtr) {
    	    if (menuPtr->menuType == MENUBAR) {
    	        CompleteIdlers(menuPtr);
    	        break;
    	    }
    	}
    }
    
    if (menuBarFlags & MENUBAR_REDRAW_PENDING) {
    	Tcl_CancelIdleCall(DrawMenuBarWhenIdle, (ClientData *) NULL);
    	DrawMenuBarWhenIdle((ClientData *) NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpDrawMenuEntry --
 *
 *	Draws the given menu entry at the given coordinates with the
 *	given attributes.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	X Server commands are executed to display the menu entry.
 *
 *----------------------------------------------------------------------
 */

void
TkpDrawMenuEntry(
    TkMenuEntry *mePtr,		    /* The entry to draw */
    Drawable d,			    /* What to draw into */
    Tk_Font tkfont,		    /* Precalculated font for menu */
    CONST Tk_FontMetrics *menuMetricsPtr,
				    /* Precalculated metrics for menu */
    int x,			    /* X-coordinate of topleft of entry */
    int y,			    /* Y-coordinate of topleft of entry */
    int width,			    /* Width of the entry rectangle */
    int height,			    /* Height of the current rectangle */
    int strictMotif,		    /* Boolean flag */
    int drawArrow)		    /* Whether or not to draw the cascade
				     * arrow for cascade items. Only applies
				     * to Windows. */
{
    GC gc;
    TkMenu *menuPtr = mePtr->menuPtr;
    int padY = (menuPtr->menuType == MENUBAR) ? 3 : 0;
    GC indicatorGC;
    Tk_3DBorder bgBorder, activeBorder;
    const Tk_FontMetrics *fmPtr;
    Tk_FontMetrics entryMetrics;
    int adjustedY = y + padY;
    int adjustedHeight = height - 2 * padY;
    int state;

    /*
     * Choose the gc for drawing the foreground part of the entry.
     * Under Appearance, we pass a null (appearanceGC) to tell 
     * ourselves not to change whatever color the appearance manager has set.
     */

    if ((mePtr->state == ENTRY_ACTIVE) && !strictMotif) {
	gc = mePtr->activeGC;
	if (gc == NULL) {
	        gc = menuPtr->activeGC;
	}
    } else {
    	TkMenuEntry *cascadeEntryPtr;
    	int parentDisabled = 0;
    	
    	for (cascadeEntryPtr = menuPtr->menuRefPtr->parentEntryPtr;
    		cascadeEntryPtr != NULL;
    		cascadeEntryPtr = cascadeEntryPtr->nextCascadePtr) {
    	    char *name = (cascadeEntryPtr->namePtr == NULL) ? ""
    	    	    : Tcl_GetStringFromObj(cascadeEntryPtr->namePtr, NULL);
    	 
    	    if (strcmp(name, Tk_PathName(menuPtr->tkwin)) == 0) {
    	    	if (cascadeEntryPtr->state == ENTRY_DISABLED) {
    	    	    parentDisabled = 1;
    	    	}
    	    	break;
    	    }
    	}

	if (((parentDisabled || (state == ENTRY_DISABLED)))
		&& (menuPtr->disabledFgPtr != NULL)) {
	    gc = mePtr->disabledGC;
	    if (gc == NULL) {
		gc = menuPtr->disabledGC;
	    }
	} else {
	    gc = mePtr->textGC;
	    if (gc == NULL) {
		gc = menuPtr->textGC;
	    }
        }
    }
    
    indicatorGC = mePtr->indicatorGC;
    if (indicatorGC == NULL) {
	indicatorGC = menuPtr->indicatorGC;
    }

    bgBorder = Tk_Get3DBorderFromObj(menuPtr->tkwin,
	    (mePtr->borderPtr == NULL)
	    ? menuPtr->borderPtr : mePtr->borderPtr);
    if (strictMotif) {
	activeBorder = bgBorder;
    } else {
	activeBorder = Tk_Get3DBorderFromObj(menuPtr->tkwin,
	    (mePtr->activeBorderPtr == NULL)
	    ? menuPtr->activeBorderPtr : mePtr->activeBorderPtr);
    }

    if (mePtr->fontPtr == NULL) {
	fmPtr = menuMetricsPtr;
    } else {
	tkfont = Tk_GetFontFromObj(menuPtr->tkwin, mePtr->fontPtr);
	Tk_GetFontMetrics(tkfont, &entryMetrics);
	fmPtr = &entryMetrics;
    }

    /*
     * Need to draw the entire background, including padding. On Unix,
     * for menubars, we have to draw the rest of the entry taking
     * into account the padding.
     */
    
    DrawMenuEntryBackground(menuPtr, mePtr, d, activeBorder, 
	    bgBorder, x, y, width, height);
    
    if (mePtr->type == SEPARATOR_ENTRY) {
	DrawMenuSeparator(menuPtr, mePtr, d, gc, tkfont, 
		fmPtr, x, adjustedY, width, adjustedHeight);
    } else if (mePtr->type == TEAROFF_ENTRY) {
	DrawTearoffEntry(menuPtr, mePtr, d, gc, tkfont, fmPtr, x, adjustedY,
		width, adjustedHeight);
    } else {
	DrawMenuEntryLabel(menuPtr, mePtr, d, gc, tkfont, fmPtr, x, 
		adjustedY, width, adjustedHeight);
	DrawMenuEntryAccelerator(menuPtr, mePtr, d, gc, tkfont, fmPtr,
		activeBorder, x, adjustedY, width, adjustedHeight, drawArrow);
	if (!mePtr->hideMargin) {
	    DrawMenuEntryIndicator(menuPtr, mePtr, d, gc, indicatorGC, tkfont,
		    fmPtr, x, adjustedY, width, adjustedHeight);
	}
    
    }
}

/*
 *--------------------------------------------------------------
 *
 * TkpComputeStandardMenuGeometry --
 *
 *	This procedure is invoked to recompute the size and
 *	layout of a menu that is not a menubar clone.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Fields of menu entries are changed to reflect their
 *	current positions, and the size of the menu window
 *	itself may be changed.
 *
 *--------------------------------------------------------------
 */

void
TkpComputeStandardMenuGeometry(
    TkMenu *menuPtr)		/* Structure describing menu. */
{
    Tk_Font tkfont, menuFont;
    Tk_FontMetrics menuMetrics, entryMetrics, *fmPtr;
    int x, y, height, modifierWidth, labelWidth, indicatorSpace;
    int windowWidth, windowHeight, accelWidth, maxAccelTextWidth;
    int i, j, lastColumnBreak, maxModifierWidth, maxWidth, nonAccelMargin;
    int maxNonAccelMargin, maxEntryWithAccelWidth, maxEntryWithoutAccelWidth;
    int entryWidth, maxIndicatorSpace, borderWidth, activeBorderWidth;
    TkMenuEntry *mePtr, *columnEntryPtr;
    EntryGeometry *geometryPtr;
    
    if (menuPtr->tkwin == NULL) {
	return;
    }

    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->borderWidthPtr,
    	    &borderWidth);
    Tk_GetPixelsFromObj(NULL, menuPtr->tkwin, menuPtr->activeBorderWidthPtr,
    	    &activeBorderWidth);
    x = y = borderWidth;
    indicatorSpace = labelWidth = accelWidth = maxAccelTextWidth = 0;
    windowHeight = windowWidth = maxWidth = lastColumnBreak = 0;
    maxModifierWidth = nonAccelMargin = maxNonAccelMargin = 0;
    maxEntryWithAccelWidth = maxEntryWithoutAccelWidth = 0;
    maxIndicatorSpace = 0;

    /*
     * On the Mac especially, getting font metrics can be quite slow,
     * so we want to do it intelligently. We are going to precalculate
     * them and pass them down to all of the measuring and drawing
     * routines. We will measure the font metrics of the menu once.
     * If an entry does not have its own font set, then we give
     * the geometry/drawing routines the menu's font and metrics.
     * If an entry has its own font, we will measure that font and
     * give all of the geometry/drawing the entry's font and metrics.
     */

    menuFont = Tk_GetFontFromObj(menuPtr->tkwin, menuPtr->fontPtr);
    Tk_GetFontMetrics(menuFont, &menuMetrics);

    for (i = 0; i < menuPtr->numEntries; i++) {
    	mePtr = menuPtr->entries[i];
    	if (mePtr->fontPtr == NULL) {
	    tkfont = menuFont;
	    fmPtr = &menuMetrics;
    	} else {
	    tkfont = Tk_GetFontFromObj(menuPtr->tkwin, mePtr->fontPtr);
    	    Tk_GetFontMetrics(tkfont, &entryMetrics);
    	    fmPtr = &entryMetrics;
    	}
    	
	if ((i > 0) && mePtr->columnBreak) {
	    if (maxIndicatorSpace != 0) {
		maxIndicatorSpace += 2;
	    }
	    for (j = lastColumnBreak; j < i; j++) {
	    	columnEntryPtr = menuPtr->entries[j];
	    	geometryPtr =
		        (EntryGeometry *) columnEntryPtr->platformEntryData;
	    	
	    	columnEntryPtr->indicatorSpace = maxIndicatorSpace;
		columnEntryPtr->width = maxIndicatorSpace + maxWidth 
			+ 2 * activeBorderWidth;
		geometryPtr->accelTextWidth = maxAccelTextWidth;
		geometryPtr->modifierWidth = maxModifierWidth;
		columnEntryPtr->x = x;
		columnEntryPtr->entryFlags &= ~ENTRY_LAST_COLUMN;
		if (maxEntryWithoutAccelWidth > maxEntryWithAccelWidth) {
		    geometryPtr->nonAccelMargin = maxEntryWithoutAccelWidth
		    	    - maxEntryWithAccelWidth;
		    if (geometryPtr->nonAccelMargin > maxNonAccelMargin) {
		    	geometryPtr->nonAccelMargin = maxNonAccelMargin;
		    }
		} else {
		    geometryPtr->nonAccelMargin = 0;
		}		
	    }
	    x += maxIndicatorSpace + maxWidth + 2 * borderWidth;
	    windowWidth = x;
	    maxWidth = maxIndicatorSpace = maxAccelTextWidth = 0;
	    maxModifierWidth = maxNonAccelMargin = maxEntryWithAccelWidth = 0;
	    maxEntryWithoutAccelWidth = 0;
	    lastColumnBreak = i;
	    y = borderWidth;
	}

	if (mePtr->type == SEPARATOR_ENTRY) {
	    GetMenuSeparatorGeometry(menuPtr, mePtr, tkfont,
	    	    fmPtr, &entryWidth, &height);
	    mePtr->height = height;
	} else if (mePtr->type == TEAROFF_ENTRY) {
	    GetTearoffEntryGeometry(menuPtr, mePtr, tkfont, 
	    	    fmPtr, &entryWidth, &height);
	    mePtr->height = height;
	} else {
	    /*
	     * For each entry, compute the height required by that
	     * particular entry, plus three widths:  the width of the
	     * label, the width to allow for an indicator to be displayed
	     * to the left of the label (if any), and the width of the
	     * accelerator to be displayed to the right of the label
	     * (if any).  These sizes depend, of course, on the type
	     * of the entry.
	     */
	    
	    GetMenuLabelGeometry(mePtr, tkfont, fmPtr, &labelWidth,
	    	    &height);
	    mePtr->height = height;
	
	    if (mePtr->type == CASCADE_ENTRY) {
	    	GetMenuAccelGeometry(menuPtr, mePtr, tkfont, fmPtr,
	    		&modifierWidth, &accelWidth, &height);
	    	nonAccelMargin = 0;
	    } else if (mePtr->accelLength == 0) {
	    	nonAccelMargin = mePtr->hideMargin ? 0
		    : Tk_TextWidth(tkfont, "m", 1);
	    	accelWidth = modifierWidth = 0;
	    } else {
	    	labelWidth += Tk_TextWidth(tkfont, "m", 1);
	    	GetMenuAccelGeometry(menuPtr, mePtr, tkfont,
		    	fmPtr, &modifierWidth, &accelWidth, &height);
	        if (height > mePtr->height) {
	    	    mePtr->height = height;
	    	}
	    	nonAccelMargin = 0;
	    }

	    if (!(mePtr->hideMargin)) {
	    	GetMenuIndicatorGeometry(menuPtr, mePtr, tkfont, 
	    	    	fmPtr, &indicatorSpace, &height);
	    	if (height > mePtr->height) {
	    	    mePtr->height = height;
	    	}
	    } else {
	    	indicatorSpace = 0;
	    }

	    if (nonAccelMargin > maxNonAccelMargin) {
	    	maxNonAccelMargin = nonAccelMargin;
	    }
	    if (accelWidth > maxAccelTextWidth) {
	    	maxAccelTextWidth = accelWidth;
	    }
	    if (modifierWidth > maxModifierWidth) {
	    	maxModifierWidth = modifierWidth;
	    }
	    if (indicatorSpace > maxIndicatorSpace) {
	    	maxIndicatorSpace = indicatorSpace;
	    }

	    entryWidth = labelWidth + modifierWidth + accelWidth
		    + nonAccelMargin;

	    if (entryWidth > maxWidth) {
	    	maxWidth = entryWidth;
	    }
	    
	    if (mePtr->accelLength > 0) {
	    	if (entryWidth > maxEntryWithAccelWidth) {
	    	    maxEntryWithAccelWidth = entryWidth;
	    	}
	    } else {
	    	if (entryWidth > maxEntryWithoutAccelWidth) {
	    	    maxEntryWithoutAccelWidth = entryWidth;
	    	}
	    }
	    
	    mePtr->height += 2 * activeBorderWidth;
    	}
        mePtr->y = y;
	y += menuPtr->entries[i]->height + borderWidth;
	if (y > windowHeight) {
	    windowHeight = y;
	}
    }

    for (j = lastColumnBreak; j < menuPtr->numEntries; j++) {
    	columnEntryPtr = menuPtr->entries[j];
    	geometryPtr = (EntryGeometry *) columnEntryPtr->platformEntryData;
    	
    	columnEntryPtr->indicatorSpace = maxIndicatorSpace;
	columnEntryPtr->width = maxIndicatorSpace + maxWidth 
		+ 2 * activeBorderWidth;
	geometryPtr->accelTextWidth = maxAccelTextWidth;
	geometryPtr->modifierWidth = maxModifierWidth;
	columnEntryPtr->x = x;
	columnEntryPtr->entryFlags |= ENTRY_LAST_COLUMN;
	if (maxEntryWithoutAccelWidth > maxEntryWithAccelWidth) {
	    geometryPtr->nonAccelMargin = maxEntryWithoutAccelWidth
	    	    - maxEntryWithAccelWidth;
	    if (geometryPtr->nonAccelMargin > maxNonAccelMargin) {
	    	geometryPtr->nonAccelMargin = maxNonAccelMargin;
	    }
	} else {
	    geometryPtr->nonAccelMargin = 0;
	}		
    }
    windowWidth = x + maxIndicatorSpace + maxWidth
	    + 2 * activeBorderWidth + borderWidth;
    windowHeight += borderWidth;
    
    /*
     * The X server doesn't like zero dimensions, so round up to at least
     * 1 (a zero-sized menu should never really occur, anyway).
     */

    if (windowWidth <= 0) {
	windowWidth = 1;
    }
    if (windowHeight <= 0) {
	windowHeight = 1;
    }
    menuPtr->totalWidth = windowWidth;
    menuPtr->totalHeight = windowHeight;
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryLabel --
 *
 *	This procedure draws the label part of a menu.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */

static void
DrawMenuEntryLabel(
    TkMenu *menuPtr,			/* The menu we are drawing */
    TkMenuEntry *mePtr,			/* The entry we are drawing */
    Drawable d,				/* What we are drawing into */
    GC gc,				/* The gc we are drawing into */
    Tk_Font tkfont,			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* The precalculated font metrics */
    int x,				/* left edge */
    int y,				/* right edge */
    int width,				/* width of entry */
    int height)				/* height of entry */
{
    int baseline;
    int indicatorSpace =  mePtr->indicatorSpace;
    int leftEdge = x + indicatorSpace;
    int imageHeight, imageWidth;
    
    /*
     * Draw label or bitmap or image for entry.
     */

    baseline = y + (height + fmPtr->ascent - fmPtr->descent) / 2;
    if (mePtr->image != NULL) {
    	Tk_SizeOfImage(mePtr->image, &imageWidth, &imageHeight);
    	if ((mePtr->selectImage != NULL)
	    	&& (mePtr->entryFlags & ENTRY_SELECTED)) {
	    Tk_RedrawImage(mePtr->selectImage, 0, 0,
		    imageWidth, imageHeight, d, leftEdge,
	            (int) (y + (mePtr->height - imageHeight)/2));
    	} else {
	    Tk_RedrawImage(mePtr->image, 0, 0, imageWidth,
		    imageHeight, d, leftEdge,
		    (int) (y + (mePtr->height - imageHeight)/2));
    	}
    } else if (mePtr->bitmapPtr != NULL) {
    	int width, height;
    	Pixmap bitmap = Tk_GetBitmapFromObj(menuPtr->tkwin, mePtr->bitmapPtr);
        Tk_SizeOfBitmap(menuPtr->display,
	        bitmap, &width, &height);
    	XCopyPlane(menuPtr->display, bitmap, d, gc, 0, 0, 
    		(unsigned) width, (unsigned) height, leftEdge,
	    	(int) (y + (mePtr->height - height)/2), 1);
    } else {
    	if (mePtr->labelLength > 0) {
    	    Tcl_DString itemTextDString, convertedTextDString;
    	    
    	    GetEntryText(mePtr, &itemTextDString);
    	    
    	    /* Somehow DrawChars is changing the colors, it is odd, since
    	       it works for the Apple Platinum Appearance, but not for
    	       some Kaleidoscope Themes...  Untill I can figure out what
    	       exactly is going on, this will have to do: */
    	    
            TkMacSetUpGraphicsPort(gc);
	    MoveTo((short) leftEdge, (short) baseline);
	    Tcl_UtfToExternalDString(NULL, Tcl_DStringValue(&itemTextDString), 
	            Tcl_DStringLength(&itemTextDString), &convertedTextDString);
	    DrawText(Tcl_DStringValue(&convertedTextDString), 0, 
	            Tcl_DStringLength(&convertedTextDString));
	    
	    /* Tk_DrawChars(menuPtr->display, d, gc,
		    tkfont, Tcl_DStringValue(&itemTextDString), 
		    Tcl_DStringLength(&itemTextDString),
		    leftEdge, baseline); */
		    
	    Tcl_DStringFree(&itemTextDString);
    	}
    }

    if (mePtr->state == ENTRY_DISABLED) {
	if (menuPtr->disabledFgPtr == NULL) {
	    if (!TkMacHaveAppearance()) {
	        XFillRectangle(menuPtr->display, d, menuPtr->disabledGC, x, y,
		        (unsigned) width, (unsigned) height);
	    }
	} else if ((mePtr->image != NULL) 
		&& (menuPtr->disabledImageGC != None)) {
	    XFillRectangle(menuPtr->display, d, menuPtr->disabledImageGC,
		    leftEdge,
		    (int) (y + (mePtr->height - imageHeight)/2),
		    (unsigned) imageWidth, (unsigned) imageHeight);
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryBackground --
 *
 *	This procedure draws the background part of a menu entry.
 *      Under Appearance, we only draw the background if the entry's
 *      border is set, we DO NOT inherit it from the menu...
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands are output to X to display the menu in its
 *	current mode.
 *
 *----------------------------------------------------------------------
 */

static void
DrawMenuEntryBackground(
    TkMenu *menuPtr,			/* The menu we are drawing. */
    TkMenuEntry *mePtr,			/* The entry we are drawing. */
    Drawable d,				/* What we are drawing into */
    Tk_3DBorder activeBorder,		/* Border for active items */
    Tk_3DBorder bgBorder,		/* Border for the background */
    int x,				/* left edge */
    int y,				/* top edge */
    int width,				/* width of rectangle to draw */
    int height)				/* height of rectangle to draw */
{
    if (!TkMacHaveAppearance()
            || (menuPtr->menuType == TEAROFF_MENU)
            || ((mePtr->state == ENTRY_ACTIVE)
		    && (mePtr->activeBorderPtr != None)) 
            || ((mePtr->state != ENTRY_ACTIVE) && (mePtr->borderPtr != None))) {
        if (mePtr->state == ENTRY_ACTIVE) {
	    bgBorder = activeBorder;
        }
        Tk_Fill3DRectangle(menuPtr->tkwin, d, bgBorder,
    	        x, y, width, height, 0, TK_RELIEF_FLAT);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetMenuLabelGeometry --
 *
 *	Figures out the size of the label portion of a menu item.
 *
 * Results:
 *	widthPtr and heightPtr are filled in with the correct geometry
 *	information.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void
GetMenuLabelGeometry(
    TkMenuEntry *mePtr,			/* The entry we are computing */
    Tk_Font tkfont,			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* The precalculated metrics */
    int *widthPtr,			/* The resulting width of the label
					 * portion */
    int *heightPtr)			/* The resulting height of the label
					 * portion */
{
    TkMenu *menuPtr = mePtr->menuPtr;
 
    if (mePtr->image != NULL) {
    	Tk_SizeOfImage(mePtr->image, widthPtr, heightPtr);
    } else if (mePtr->bitmapPtr != NULL) {
    	Pixmap bitmap = Tk_GetBitmapFromObj(menuPtr->tkwin, mePtr->bitmapPtr);
    	Tk_SizeOfBitmap(menuPtr->display, bitmap, widthPtr, heightPtr);
    } else {
    	*heightPtr = fmPtr->linespace;
    	
    	if (mePtr->labelPtr != NULL) {
    	    Tcl_DString itemTextDString;
    	    
    	    GetEntryText(mePtr, &itemTextDString);
    	    *widthPtr = Tk_TextWidth(tkfont, 
    	    	    Tcl_DStringValue(&itemTextDString),
    	    	    Tcl_DStringLength(&itemTextDString));
    	    Tcl_DStringFree(&itemTextDString);
    	} else {
    	    *widthPtr = 0;
    	}
    }
    *heightPtr += 1;
}

/*
 *----------------------------------------------------------------------
 *
 * MenuSelectEvent --
 *
 *	Generates a "MenuSelect" virtual event. This can be used to
 *	do context-sensitive menu help.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Places a virtual event on the event queue.
 *
 *----------------------------------------------------------------------
 */

static void
MenuSelectEvent(
    TkMenu *menuPtr)		/* the menu we have selected. */
{
    XVirtualEvent event;
    Point where;
   
    event.type = VirtualEvent;
    event.serial = menuPtr->display->request;
    event.send_event = false;
    event.display = menuPtr->display;
    Tk_MakeWindowExist(menuPtr->tkwin);
    event.event = Tk_WindowId(menuPtr->tkwin);
    event.root = XRootWindow(menuPtr->display, 0);
    event.subwindow = None;
    event.time = TkpGetMS();
    
    GetMouse(&where);
    event.x_root = where.h;
    event.y_root = where.v;
    event.state = TkMacButtonKeyState();
    event.same_screen = true;
    event.name = Tk_GetUid("MenuSelect");
    Tk_QueueWindowEvent((XEvent *) &event, TCL_QUEUE_TAIL);
}

/*
 *----------------------------------------------------------------------
 *
 * RecursivelyClearActiveMenu --
 *
 *	Recursively clears the active entry in the menu's cascade hierarchy.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates <<MenuSelect>> virtual events.
 *
 *----------------------------------------------------------------------
 */

void
RecursivelyClearActiveMenu(
    TkMenu *menuPtr)		/* The menu to reset. */
{
    int i;
    TkMenuEntry *mePtr;
    
    TkActivateMenuEntry(menuPtr, -1);
    MenuSelectEvent(menuPtr);
    for (i = 0; i < menuPtr->numEntries; i++) {
    	mePtr = menuPtr->entries[i];
    	if (mePtr->type == CASCADE_ENTRY) {
    	    if ((mePtr->childMenuRefPtr != NULL)
    	    	    && (mePtr->childMenuRefPtr->menuPtr != NULL)) {
    	    	RecursivelyClearActiveMenu(mePtr->childMenuRefPtr->menuPtr);
    	    }
    	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * InvalidateMDEFRgns --
 *
 *	Invalidates the regions covered by menus that did redrawing and
 *	might be damaged.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates Mac update events for affected windows.
 *
 *----------------------------------------------------------------------
 */

void
InvalidateMDEFRgns(void) {
    GDHandle saveDevice;
    GWorldPtr saveWorld, destPort;
    Point scratch;
    MacDrawable *macDraw;
    TkMacWindowList *listPtr;
    
    if (totalMenuRgn == NULL) {
    	return;
    }
    
    GetGWorld(&saveWorld, &saveDevice);
    for (listPtr = tkMacWindowListPtr ; listPtr != NULL; 
    	    listPtr = listPtr->nextPtr) {
    	macDraw = (MacDrawable *) Tk_WindowId(listPtr->winPtr);
    	if (macDraw->flags & TK_DRAWN_UNDER_MENU) {
    	    destPort = TkMacGetDrawablePort(Tk_WindowId(listPtr->winPtr));
    	    SetGWorld(destPort, NULL);
    	    scratch.h = scratch.v = 0;
    	    GlobalToLocal(&scratch);
    	    OffsetRgn(totalMenuRgn, scratch.v, scratch.h);
    	    InvalRgn(totalMenuRgn);
    	    OffsetRgn(totalMenuRgn, -scratch.v, -scratch.h);
    	    macDraw->flags &= ~TK_DRAWN_UNDER_MENU;
    	}
    }
    
    SetGWorld(saveWorld, saveDevice);
    SetEmptyRgn(totalMenuRgn);
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacClearMenubarActive --
 *
 *	Recursively clears the active entry in the current menubar hierarchy.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Generates <<MenuSelect>> virtual events.
 *
 *----------------------------------------------------------------------
 */

void
TkMacClearMenubarActive(void) {
    TkMenuReferences *menuBarRefPtr;
    
    if (currentMenuBarName != NULL) {
    	menuBarRefPtr = TkFindMenuReferences(currentMenuBarInterp,
    		currentMenuBarName);
    	if ((menuBarRefPtr != NULL) && (menuBarRefPtr->menuPtr != NULL)) {
    	    TkMenu *menuPtr;
    	    
    	    for (menuPtr = menuBarRefPtr->menuPtr->masterMenuPtr; menuPtr != NULL;
    	    	    menuPtr = menuPtr->nextInstancePtr) {
    	    	if (menuPtr->menuType == MENUBAR) {
    	    	    RecursivelyClearActiveMenu(menuPtr);
    	    	}
    	    }
    	}
    }
    InvalidateMDEFRgns();
    FixMDEF();
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuNotifyToplevelCreate --
 *
 *	This routine reconfigures the menu and the clones indicated by
 *	menuName becuase a toplevel has been created and any system
 *	menus need to be created. Only applicable to Windows.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	An idle handler is set up to do the reconfiguration.
 *
 *----------------------------------------------------------------------
 */

void
TkpMenuNotifyToplevelCreate(
    Tcl_Interp *interp,			/* The interp the menu lives in. */
    char *menuName)			/* The name of the menu to 
					 * reconfigure. */
{
    /*
     * Nothing to do.
     */
}

/*
 *----------------------------------------------------------------------
 *
 * FixMDEF --
 *
 *	Loads the MDEF and blasts our routine descriptor into it.
 * 	We have to set up the MDEF. This is pretty slimy. The real MDEF
 * 	resource is 68K code. All this code does is call another procedure.
 * 	When the application in launched, a dummy value for the procedure
 * 	is compiled into the MDEF. We are going to replace that dummy
 * 	value with a routine descriptor. When the routine descriptor
 * 	is invoked, the globals and everything will be setup, and we
 * 	can do what we need. This will not work from 68K or CFM 68k
 * 	currently, so we will conditional compile this until we
 * 	figure it out. 
 *
 * Results:
 *	Returns the MDEF handle.
 *
 * Side effects:
 *	The MDEF is read in and massaged.
 *
 *----------------------------------------------------------------------
 */

static Handle
FixMDEF(void)
{
#ifdef GENERATINGCFM
    Handle MDEFHandle = GetResource('MDEF', 591);
    Handle SICNHandle = GetResource('SICN', SICN_RESOURCE_NUMBER);
    if ((MDEFHandle != NULL) && (SICNHandle != NULL)) {
        HLock(MDEFHandle);
    	HLock(SICNHandle);
	if (menuDefProc == NULL) {
    	    menuDefProc = TkNewMenuDefProc(MenuDefProc);
	}
    	memmove((void *) (((long) (*MDEFHandle)) + 0x24), &menuDefProc, 4);
        return MDEFHandle;
    } else {
        return NULL;
    }
#else
    return NULL;
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuInit --
 *
 *	Initializes Mac-specific menu data.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates a hash table.
 *
 *----------------------------------------------------------------------
 */

void
TkpMenuInit(void)
{
    lastMenuID = 256;
    Tcl_InitHashTable(&commandTable, TCL_ONE_WORD_KEYS);
    currentMenuBarOwner = NULL;
    tearoffStruct.menuPtr = NULL;
    currentAppleMenuID = 0;
    currentHelpMenuID = 0;
    currentMenuBarInterp = NULL;
    currentMenuBarName = NULL;
    windowListPtr = NULL;
    
    /*
     * Get the GC that we will use as the sign to the font
     * routines that they should not muck with the foreground color...
     */
    
    if (TkMacHaveAppearance() > 1) {
        XGCValues tmpValues;
        TkColor *tmpColorPtr;
        
        tmpColorPtr = TkpGetColor(NULL, "systemAppearanceColor");
        tmpValues.foreground = tmpColorPtr->color.pixel;
        tmpValues.background = tmpColorPtr->color.pixel;
        ckfree((char *) tmpColorPtr);
        
        tkThemeMenuItemDrawingUPP = NewMenuItemDrawingProc(tkThemeMenuItemDrawingProc);				
    }
    FixMDEF();

    
    Tcl_ExternalToUtf(NULL, NULL, "É", -1, 0, NULL, elipsisString,
	    TCL_UTF_MAX + 1, NULL, NULL, NULL);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuThreadInit --
 *
 *	Does platform-specific initialization of thread-specific
 *      menu state.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

void
TkpMenuThreadInit()
{
    /*
     * Nothing to do.
     */
}
