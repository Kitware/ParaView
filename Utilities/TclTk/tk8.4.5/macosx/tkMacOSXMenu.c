/* 
 * tkMacOSXMenu.c --
 *
 *	This module implements the Mac-platform specific features of menus.
 *
 * Copyright (c) 1996-1997 by Sun Microsystems, Inc.
 * Copyright 2001, Apple Computer, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */
#include "tkMacOSXInt.h"
#include "tkMenuButton.h"
#include "tkMenu.h"
#include "tkColor.h"
#include "tkMacOSXInt.h"
#undef Status

#define USE_TK_MDEF
//#define USE_ATSU

#include <Carbon/Carbon.h>
#include "tkMacOSXDebug.h"
#include <CoreFoundation/CFString.h>

typedef struct MacMenu {
    MenuRef menuHdl;		/* The Menu Manager data structure. */
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
#define SCREEN_MARGIN 5

static int gNoTkMenus = 0;      /* This is used by Tk_MacOSXTurnOffMenus as the
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
				/* The UTF representation of the elipsis (...) 
				 * character. */
static int inPostMenu;		/* We cannot be re-entrant like X
				 * windows. */
static short lastMenuID;	/* To pass to NewMenu; need to figure out
				 * a good way to do this. */
static short lastCascadeID;
				/* Cascades have to have ids that are
				 * less than 256. */
static MacDrawable macMDEFDrawable;
				/* Drawable for use by MDEF code */
static int MDEFScrollFlag = 0;	/* Used so that popups don't scroll too soon. */
static int menuBarFlags;	/* Used for whether the menu bar needs
				 * redrawing or not. */
                                 
static struct TearoffSelect {
    TkMenu *menuPtr;		/* The menu that is torn off */
    Point point;		/* The point to place the new menu */
    Rect excludeRect;		/* We don't want to drag tearoff highlights
    				 * when we are in this menu */
} tearoffStruct;

struct MenuCommandHandlerData { /* This is the ClientData we pass to */
    TkMenu *menuPtr;            /* Tcl_DoWhenIdle to move handling */
    int index;                  /* menu commands to the event loop. */
};

static RgnHandle totalMenuRgn = NULL;
				/* Used to update windows which have been
				 * obscured by menus. */
static RgnHandle utilRgn = NULL;/* Used when creating the region that is to
				 * be clipped out while the MDEF is active. */

static TopLevelMenubarList *windowListPtr;
				/* A list of windows that have menubars set. */
static MenuItemDrawingUPP tkThemeMenuItemDrawingUPP; 
				/* Points to the UPP for theme Item drawing. */
static Tcl_Obj *useMDEFVar;
				
/*
 * Forward declarations for procedures defined later in this file:
 */

int TkMacOSXGetNewMenuID _ANSI_ARGS_((Tcl_Interp *interp, 
        TkMenu *menuInstPtr, 
        int cascade, 
        short *menuIDPtr));
void TkMacOSXFreeMenuID _ANSI_ARGS_((short menuID));
 
static void CompleteIdlers _ANSI_ARGS_((TkMenu *menuPtr));
static void DrawMenuBarWhenIdle _ANSI_ARGS_((
    ClientData clientData));
static void  DrawMenuBackground _ANSI_ARGS_((
    Rect *menuRectPtr, Drawable d, ThemeMenuType type));
static void DrawMenuEntryAccelerator _ANSI_ARGS_((
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
static void 	EventuallyInvokeMenu (ClientData data);
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
static char		FindMarkCharacter _ANSI_ARGS_((TkMenuEntry *mePtr));
static void		InvalidateMDEFRgns _ANSI_ARGS_((void));

static void             MenuDefProc _ANSI_ARGS_((short message,
                            MenuHandle menu, Rect *menuRectPtr,
                            Point hitPt, short *whichItem ));
static void             HandleMenuHiliteMsg (MenuRef menu, 
                            Rect *menuRectPtr, 
                            Point hitPt, 
                            SInt16 *whichItem,
                            TkMenu *menuPtr);
static void             HandleMenuDrawMsg (MenuRef menu, 
                            Rect *menuRectPtr, 
                            Point hitPt, 
                            SInt16 *whichItem,
                            TkMenu *menuPtr);
static void             HandleMenuFindItemsMsg (MenuRef menu, 
                            Rect *menuRectPtr, 
                            Point hitPt, 
                            SInt16 *whichItem,
                            TkMenu *menuPtr);
static void             HandleMenuPopUpMsg (MenuRef menu, 
                            Rect *menuRectPtr, 
                            Point hitPt, 
                            SInt16 *whichItem,
                            TkMenu *menuPtr);
static void             HandleMenuCalcItemMsg (MenuRef menu, 
                            Rect *menuRectPtr, 
                            Point hitPt, 
                            SInt16 *whichItem,
                            TkMenu *menuPtr);

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
static void		mySetMenuTitle _ANSI_ARGS_((MenuHandle menuHdl,
			    Tcl_Obj *titlePtr));
static void		AppearanceEntryDrawWrapper _ANSI_ARGS_((TkMenuEntry *mePtr, 
			    Rect * menuRectPtr, MenuTrackingData *mtdPtr,     
			    Drawable d, Tk_FontMetrics *fmPtr, Tk_Font tkfont,
			    int x, int y, int width, int height));
pascal void 		tkThemeMenuItemDrawingProc _ANSI_ARGS_ ((const Rect *inBounds,
			    SInt16 inDepth, Boolean inIsColorDevice, 
			    SInt32 inUserData));


/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXUseID --
 *
 *	Take the ID out of the available list for new menus. Used by the
 *	default menu bar's menus so that they do not get created at the tk
 *	level. See TkMacOSXGetNewMenuID for more information.
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
TkMacOSXUseMenuID(
    short macID)		/* The id to take out of the table */
{
    Tcl_HashEntry *commandEntryPtr;
    int newEntry;
    int iMacID = macID; /* Do this to remove compiler warning */
    
    TkMenuInit();
    commandEntryPtr = Tcl_CreateHashEntry(&commandTable, (char *) iMacID,
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
 * TkMacOSXGetNewMenuID --
 *
 *	Allocates a new menu id and marks it in use. Each menu on the
 *	mac must be designated by a unique id, which is a short. In
 *	addition, some ids are reserved by the system. Since Tk uses
 *	mostly dynamic menus, we must allocate and free these ids on
 *	the fly. We use the id as a key into a hash table; if there
 *	is no hash entry, we know that we can use the id.
 *
 *	Carbon allows a much larger number of menus than the old APIs.
 *	I believe this is 32768, but am not sure.  This code just uses
 *	2000 as the upper limit.  Unfortunately tk leaks menus when
 *	cloning, under some circumstances (see bug on sourceforge).
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
int
 TkMacOSXGetNewMenuID(
    Tcl_Interp *interp,		/* Used for error reporting */
    TkMenu *menuPtr,		/* The menu we are working with */
    int cascade,		/* 0 if we are working with a normal menu;
    				   1 if we are working with a cascade */
    short *menuIDPtr)		/* The resulting id */
{
    int found = 0;
    int newEntry;
    Tcl_HashEntry *commandEntryPtr = NULL;
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
            int iCurID = curID;
    	    commandEntryPtr = Tcl_CreateHashEntry(&commandTable,
		    (char *) iCurID, &newEntry);
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
    
    	short curID = lastCascadeID + 1;
        if (curID == 2000) {
    	    curID = 0;
    	}
    	
    	while (curID != lastCascadeID) {
            int iCurID = curID;
    	    commandEntryPtr = Tcl_CreateHashEntry(&commandTable,
		    (char *) iCurID, &newEntry);
    	    if (newEntry == 1) {
    	    	found = 1;
    	    	lastCascadeID = returnID = curID;
    	    	break;
    	    }
    	    curID++;
    	    if (curID == 2000) {
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
 * TkMacOSXFreeMenuID --
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

void
TkMacOSXFreeMenuID(
    short menuID)			/* The id to free */
{
    Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&commandTable,
	    (char *) ((int)menuID));
    
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
    MenuRef macMenuHdl;
    MenuDefSpec menuDefSpec;
    Tcl_Obj *useMDEFObjPtr;
    int useMDEF;
    int error = TCL_OK;
    int err;

    
    error = TkMacOSXGetNewMenuID(menuPtr->interp, menuPtr, 0, &menuID);
    if (error != TCL_OK) {
    	return error;
    }
    length = strlen(Tk_PathName(menuPtr->tkwin));
    memmove(&itemText[1], Tk_PathName(menuPtr->tkwin), 
    	    (length > 230) ? 230 : length);
    itemText[0] = (length > 230) ? 230 : length;
    macMenuHdl = NewMenu(menuID, itemText);
    
    /*
     * Check whether we want to use the custom mdef or not.  For now
     * the default is to use it unless the variable is explicitly
     * set to no.
     */
     
    useMDEFObjPtr = Tcl_ObjGetVar2(menuPtr->interp, useMDEFVar, NULL, TCL_GLOBAL_ONLY);
    if (useMDEFObjPtr == NULL
            || Tcl_GetBooleanFromObj(NULL, useMDEFObjPtr, &useMDEF) == TCL_ERROR
            || useMDEF) { 
        menuDefSpec.defType = kMenuDefProcPtr;
        menuDefSpec.u.defProc = MenuDefProc;
        if ((err = SetMenuDefinition(macMenuHdl, &menuDefSpec)) != noErr) {
            fprintf(stderr, "SetMenuDefinition failed %d\n", err );
        }
    }
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
    MenuRef macMenuHdl = ((MacMenu *) menuPtr->platformData)->menuHdl;

    if (menuPtr->menuFlags & MENU_RECONFIGURE_PENDING) {
    	Tcl_CancelIdleCall(ReconfigureMacintoshMenu, (ClientData) menuPtr);
    	menuPtr->menuFlags &= ~MENU_RECONFIGURE_PENDING;
    }
    if (GetMenuID(macMenuHdl) == currentHelpMenuID) {
    	MenuRef helpMenuHdl;
        MenuItemIndex helpIndex;
    	
    	if ((HMGetHelpMenu(&helpMenuHdl,&helpIndex) == noErr) 
    		&& (helpMenuHdl != NULL)) {
    	    int i, count = CountMenuItems(helpMenuHdl);
    	    
    	    for (i = helpIndex; i <= count; i++) {
    	    	DeleteMenuItem(helpMenuHdl, helpIndex);
    	    }
    	}
    	currentHelpMenuID = 0;
    }
    if (menuPtr->platformData != NULL) {
        MenuID menuID;
        menuID = GetMenuID(macMenuHdl);
        DeleteMenu(menuID);
        TkMacOSXFreeMenuID(menuID);
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
    MenuID newMenuID, menuID = GetMenuID(macMenuHdl);
    int error = TCL_OK;
    if (menuID >= 256) {
    	error = TkMacOSXGetNewMenuID(menuPtr->interp, menuPtr, 1, &newMenuID);
    	if (error == TCL_OK) {
    	    TkMacOSXFreeMenuID(menuID);
    	    SetMenuID (macMenuHdl,newMenuID);
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
 *	between "..." and the ellipsis character which looks nicer.
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

	for (i = 0; *text; text++, i++) {
    	    if ((*text == '.')
    	    	    && (*(text + 1) != '\0') && (*(text + 1) == '.')
    	    	    && (*(text + 2) != '\0') && (*(text + 2) == '.')) {
    	    	Tcl_DStringAppend(dStringPtr, elipsisString, -1);
    	    	i += strlen(elipsisString) - 1;
		text += 2;
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
 * 	'' - Check mark character		(\022)
 * 	'¥' - Mac Bullet character		(\245)
 * 	'' - Filled diamond			(\023)
 * 	'' - Hollow diamond			(\327)
 * 	'' = Mac Long dash ("em dash")	(\321)
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
    	    
    if (!TkMacOSXIsCharacterMissing(tkfont, '\022')) {
    	markChar = '\022';	/* Check mark */
    } else if (!TkMacOSXIsCharacterMissing(tkfont, '\245')) {
    	markChar = '\245';	/* Bullet */
    } else if (!TkMacOSXIsCharacterMissing(tkfont, '\023')) {
    	markChar = '\023';	/* Filled Diamond */
    } else if (!TkMacOSXIsCharacterMissing(tkfont, '\327')) {
    	markChar = '\327';	/* Hollow Diamond */
    } else if (!TkMacOSXIsCharacterMissing(tkfont, '\321')) {
    	markChar = '\321';	/* Long Dash */
    } else if (!TkMacOSXIsCharacterMissing(tkfont, '-')) {
    	markChar = '-';		/* Short Dash */
    } else {
    	markChar = '\022';	/* Check mark */
    }
    return markChar;
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
mySetMenuTitle(
    MenuRef menuHdl,		/* The menu we are setting the title of. */
    Tcl_Obj *titlePtr)	        /* The C string to set the title to. */
{
    char *title = (titlePtr == NULL) ? ""
	    : Tcl_GetStringFromObj(titlePtr, NULL);
    CFStringRef cf = CFStringCreateWithCString(NULL,
			    title, kCFStringEncodingUTF8);

    SetMenuTitleWithCFString(menuHdl, cf);
    CFRelease(cf);
}

static int ParseAccelerators(char **accelStringPtr) {
    char *accelString = *accelStringPtr;
    int flags = 0;
    while (1) {
	if ((0 == strncasecmp("Control", accelString, 6))
		&& (('-' == accelString[6]) || ('+' == accelString[6]))) {
	    flags |= ENTRY_CONTROL_ACCEL;
	    accelString += 7;
	} else if ((0 == strncasecmp("Ctrl", accelString, 4))
		&& (('-' == accelString[4]) || ('+' == accelString[4]))) {
	    flags |= ENTRY_CONTROL_ACCEL;
	    accelString += 5;
	} else if ((0 == strncasecmp("Shift", accelString, 5))
		&& (('-' == accelString[5]) || ('+' == accelString[5]))) {
	    flags |= ENTRY_SHIFT_ACCEL;
	    accelString += 6;
	} else if ((0 == strncasecmp("Option", accelString, 6))
		&& (('-' == accelString[6]) || ('+' == accelString[6]))) {
	    flags |= ENTRY_OPTION_ACCEL;
	    accelString += 7;
	} else if ((0 == strncasecmp("Opt", accelString, 3))
		&& (('-' == accelString[3]) || ('+' == accelString[3]))) {
	    flags |= ENTRY_OPTION_ACCEL;
	    accelString += 4;
	} else if ((0 == strncasecmp("Command", accelString, 7))
		&& (('-' == accelString[7]) || ('+' == accelString[7]))) {
	    flags |= ENTRY_COMMAND_ACCEL;
	    accelString += 8;
	} else if ((0 == strncasecmp("Cmd", accelString, 3))
		&& (('-' == accelString[3]) || ('+' == accelString[3]))) {
	    flags |= ENTRY_COMMAND_ACCEL;
	    accelString += 4;
	} else if ((0 == strncasecmp("Alt", accelString, 3))
		&& (('-' == accelString[3]) || ('+' == accelString[3]))) {
	    flags |= ENTRY_OPTION_ACCEL;
	    accelString += 4;
	} else if ((0 == strncasecmp("Meta", accelString, 4))
		&& (('-' == accelString[4]) || ('+' == accelString[4]))) {
	    flags |= ENTRY_COMMAND_ACCEL;
	    accelString += 5;
	} else {
	    break;
	}
    }
    *accelStringPtr = accelString;
    return flags;
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
    TkMenuEntry *mePtr)	/* Information about menu entry;  may
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
    	    	    mySetMenuTitle(childMenuHdl, mePtr->labelPtr);
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
	    
	mePtr->entryFlags |= ParseAccelerators(&accelString);
	
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
    				 * # of system help menu items
    				 * for help menus. */
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
    
    count = CountMenuItems(macMenuHdl);
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
            CFStringRef cf;    	    
	    GetEntryText(mePtr, &itemTextDString);
            cf = CFStringCreateWithCString(NULL,
                  Tcl_DStringValue(&itemTextDString), kCFStringEncodingUTF8);
	    AppendMenu(macMenuHdl, "\px");
	    if (cf != NULL) {
	      SetMenuItemTextWithCFString(macMenuHdl, base + index, cf);
	      CFRelease(cf);
	    } else {
	      cf = CFSTR ("<Error>");
	      SetMenuItemTextWithCFString(macMenuHdl, base + index, cf);
	    }
	    Tcl_DStringFree(&itemTextDString);
	
    	    /*
    	     * Set enabling and disabling correctly.
    	     */

	    if (parentDisabled || (mePtr->state == ENTRY_DISABLED)) {
	    	DisableMenuItem(macMenuHdl, base + index);
	    } else {
	    	EnableMenuItem(macMenuHdl, base + index);
	    }
    	
    	    /*
    	     * Set the check mark for check entries and radio entries.
    	     */
	
	    SetItemMark(macMenuHdl, base + index, 0);		
	    if ((mePtr->type == CHECK_BUTTON_ENTRY)
		    || (mePtr->type == RADIO_BUTTON_ENTRY)) {
	    	CheckMenuItem(macMenuHdl, base + index, (mePtr->entryFlags
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
		        {
		            SetMenuItemHierarchicalID(macMenuHdl, base + index,
				    GetMenuID(childMenuHdl));
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
	    
    	    if ((mePtr->type != CASCADE_ENTRY) && (mePtr->accelPtr != NULL)) {
                int accelLen;
		int modifiers = 0;
                int hasCmd = 0;
		int offset = ((EntryGeometry *)mePtr->platformEntryData)->accelTextStart;
    	    	char *accel = Tcl_GetStringFromObj(mePtr->accelPtr, &accelLen);
                accelLen -= offset;
		accel+= offset;
		
		if (mePtr->entryFlags & ENTRY_OPTION_ACCEL) {
		    modifiers |= kMenuOptionModifier;
		}
		if (mePtr->entryFlags & ENTRY_SHIFT_ACCEL) {
		    modifiers |= kMenuShiftModifier;
		}
		if (mePtr->entryFlags & ENTRY_CONTROL_ACCEL) {
		    modifiers |= kMenuControlModifier;
		}
		if (mePtr->entryFlags & ENTRY_COMMAND_ACCEL) {
		    hasCmd = 1;
		}
                if (accelLen == 1) {
		    if (hasCmd || (modifiers != 0 && modifiers != kMenuShiftModifier)) {
                        SetItemCmd(macMenuHdl, base + index, accel[0]);
			if (!hasCmd) {
			    modifiers |= kMenuNoCommandModifier;
			}
		    }
                } else {
		    /* 
		     * Now we need to convert from various textual names
		     * to Carbon codes
		     */
		    char glyph = 0x0;
                    char first = UCHAR(accel[0]);
		    if (first == 'F' && (accel[1] > '0' && accel[1] <= '9')) {
			int fkey = accel[1] - '0';
			if (accel[2] > '0' && accel[2] <= '9') {
			    fkey = 10*fkey + (accel[2] - '0');
			}
			if (fkey > 0 && fkey < 16) {
			    glyph = kMenuF1Glyph + fkey - 1;
			}
		    } else if (first == 'P' && 0 ==strcasecmp(accel,"pageup")) {
                        glyph = kMenuPageUpGlyph;
		    } else if (first == 'P' && 0 ==strcasecmp(accel,"pagedown")) {
                        glyph = kMenuPageDownGlyph;
		    } else if (first == 'L' && 0 ==strcasecmp(accel,"left")) {
                        glyph = kMenuLeftArrowGlyph;
		    } else if (first == 'R' && 0 ==strcasecmp(accel,"right")) {
                        glyph = kMenuRightArrowGlyph;
		    } else if (first == 'U' && 0 ==strcasecmp(accel,"up")) {
                        glyph = kMenuUpArrowGlyph;
		    } else if (first == 'D' && 0 ==strcasecmp(accel,"down")) {
                        glyph = kMenuDownArrowGlyph;
		    } else if (first == 'E' && 0 ==strcasecmp(accel,"escape")) {
                        glyph = kMenuEscapeGlyph;
		    } else if (first == 'C' && 0 ==strcasecmp(accel,"clear")) {
                        glyph = kMenuClearGlyph;
		    } else if (first == 'E' && 0 ==strcasecmp(accel,"enter")) {
                        glyph = kMenuEnterGlyph;
		    } else if (first == 'D' && 0 ==strcasecmp(accel,"backspace")) {
                        glyph = kMenuDeleteLeftGlyph;
		    } else if (first == 'S' && 0 ==strcasecmp(accel,"space")) {
                        glyph = kMenuSpaceGlyph;
		    } else if (first == 'T' && 0 ==strcasecmp(accel,"tab")) {
                        glyph = kMenuTabRightGlyph;
		    } else if (first == 'F' && 0 ==strcasecmp(accel,"delete")) {
                        glyph = kMenuDeleteRightGlyph;
		    } else if (first == 'H' && 0 ==strcasecmp(accel,"home")) {
                        glyph = kMenuNorthwestArrowGlyph;
		    } else if (first == 'R' && 0 ==strcasecmp(accel,"return")) {
                        glyph = kMenuReturnGlyph;
		    } else if (first == 'H' && 0 ==strcasecmp(accel,"help")) {
                        glyph = kMenuHelpGlyph;
		    } else if (first == 'P' && 0 ==strcasecmp(accel,"power")) {
                        glyph = kMenuPowerGlyph;
                    }
		    if (glyph != 0x0) {
			SetMenuItemKeyGlyph(macMenuHdl, base + index, glyph);
			if (!hasCmd) {
			    modifiers |= kMenuNoCommandModifier;
			}
		    }
                }
		
		SetMenuItemModifiers(macMenuHdl, base + index, modifiers);
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
    	AppendResMenu(macMenuHdl, 'DRVR');
    }
    if (GetMenuID(macMenuHdl) == currentHelpMenuID) {
        MenuItemIndex helpIndex;
    	HMGetHelpMenu(&helpMenuHdl,&helpIndex);
    	if (helpMenuHdl != NULL) {
    	    ReconfigureIndividualMenu(menuPtr, helpMenuHdl, 
	            helpIndex - 1);
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
    	CountMenuItems(macMenuHdl);
    	
	oldMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
	popUpResult = PopUpMenuSelect(macMenuHdl, y, x, menuPtr->active);
	Tcl_SetServiceMode(oldMode);

	menuPtr->totalWidth = oldWidth;
	RecursivelyDeleteMenu(menuPtr);
	DeleteMenu(GetMenuID(macMenuHdl));
	
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
	    result = TkMacOSXDispatchMenuEvent(menuID, LoWord(popUpResult));
	} else {
	    TkMacOSXHandleTearoffMenu();
	    result = TCL_OK;
	}

        /*
         * Be careful, here.  The command executed in handling the menu event
         * could destroy the window.  Don't try to do anything with it then.
         */
        
        if (menuPtr->tkwin) {
	    InvalidateMDEFRgns();
	    RecursivelyClearActiveMenu(menuPtr);
        }
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
 * Tk_MacOSXTurnOffMenus --
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
Tk_MacOSXTurnOffMenus()
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
    		(char *) ((int)currentAppleMenuID));
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
    		(char *) ((int)currentHelpMenuID));
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
    		TkMacOSXGetNewMenuID(appleMenuPtr->interp, appleMenuPtr, 0, 
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
	    	    	DisableMenuItem(((MacMenu *) menuBarPtr->entries[i]
	    	    		->childMenuRefPtr->menuPtr
	    	    		->platformData)->menuHdl,
	    	    		0);
	    	    } else {
	    	    	EnableMenuItem(((MacMenu *) menuBarPtr->entries[i]
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
	    	    currentHelpMenuID = GetMenuID(macMenuHdl);
	    	} else if (menuBarPtr->entries[i]->type 
	    		== CASCADE_ENTRY) {
	    	    if ((menuBarPtr->entries[i]->childMenuRefPtr != NULL)
	    		    && menuBarPtr->entries[i]->childMenuRefPtr
			    ->menuPtr != NULL) {
	    	    	cascadeMenuPtr = menuBarPtr->entries[i]
			    	->childMenuRefPtr->menuPtr;
	    	    	macMenuHdl = ((MacMenu *) cascadeMenuPtr
	    	    		->platformData)->menuHdl;
		    	DeleteMenu(GetMenuID(macMenuHdl));
	    	    	InsertMenu(macMenuHdl, 0);
	    	    	RecursivelyInsertMenu(cascadeMenuPtr);
	    	    	if (menuBarPtr->entries[i]->state == ENTRY_DISABLED) {
	    	    	    DisableMenuItem(((MacMenu *) menuBarPtr->entries[i]
	    	    	    	    ->childMenuRefPtr->menuPtr
	    	    	    	    ->platformData)->menuHdl,
				    0);
	    	    	} else {
	    	    	    EnableMenuItem(((MacMenu *) menuBarPtr->entries[i]
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
	    	DeleteMenu(GetMenuID(macMenuHdl));
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
    CGrafPtr  winPort;
    WindowRef macWindowPtr;
    WindowRef frontNonFloating;

    winPort=TkMacOSXGetDrawablePort(winPtr->window);
    if (!winPort) {
        return;
    }
    macWindowPtr = GetWindowFromPort(winPort);
    
    frontNonFloating = FrontNonFloatingWindow();    
    if ((macWindowPtr == NULL) || (macWindowPtr != frontNonFloating)) {
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

static void 
/*
 *----------------------------------------------------------------------
 *
 * EventuallyInvokeMenu --
 *
 *	This IdleTime callback actually invokes the menu command
 *      scheduled in TkMacOSXDispatchMenuEvent.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands get executed.
 *
 *----------------------------------------------------------------------
 */

EventuallyInvokeMenu (ClientData data)
{
    struct MenuCommandHandlerData *realData
            = (struct MenuCommandHandlerData *) data;

    Tcl_Release(realData->menuPtr->interp);
    Tcl_Release(realData->menuPtr);
    TkInvokeMenu(realData->menuPtr->interp, realData->menuPtr,
            realData->index);
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXDispatchMenuEvent --
 *
 *	Given a menu id and an item, dispatches the command associated
 *	with it.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Commands for the event are scheduled for execution at idle time.
 *
 *----------------------------------------------------------------------
 */

int
TkMacOSXDispatchMenuEvent(
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
    	    	    MenuRef outHelpMenu;
    	    	    MenuItemIndex itemIndex;
    	    	    int newIndex;
    	    	    HMGetHelpMenu(&outHelpMenu, &itemIndex);
    	    	    newIndex = index - itemIndex;
    	    	    result = TkInvokeMenu(currentMenuBarInterp,
    	    	    	    helpMenuRef->menuPtr, newIndex);
    	    	}
    	    }
    	} else {
	    Tcl_HashEntry *commandEntryPtr = 
	    	    Tcl_FindHashEntry(&commandTable, (char *) ((int)menuID));
            if (commandEntryPtr != NULL) {
                TkMenu *menuPtr = (TkMenu *) Tcl_GetHashValue(commandEntryPtr);
                if ((currentAppleMenuID == menuID)
                    && (index > menuPtr->numEntries + 1)) {
                    Str255 itemText;

                    GetMenuItemText(GetMenuHandle(menuID), index, itemText);
                    result = TCL_OK;
                } else {
                    struct MenuCommandHandlerData *data
                            = (struct MenuCommandHandlerData *)
                            ckalloc(sizeof(struct MenuCommandHandlerData));
                    Tcl_Preserve(menuPtr->interp);
                    Tcl_Preserve(menuPtr);
                    data->menuPtr = menuPtr;
                    data->index = index - 1;
                    Tcl_DoWhenIdle (EventuallyInvokeMenu,
                            (ClientData) data);
                    /* result = TkInvokeMenu(menuPtr->interp, menuPtr, index - 1); */
                }
            } else {
                return TCL_ERROR;
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
        SInt16 outHeight;
        
        GetThemeMenuSeparatorHeight(&outHeight);
        *widthPtr = 0;
        *heightPtr = outHeight;
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
                /* 
                 * Not sure if this is the correct encoding, but this function
                 * doesn't appear to be used at all in, since the Carbon Menus
                 * draw themselves
                 */
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
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;

    destPort = TkMacOSXGetDrawablePort(d);
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    TkMacOSXSetUpClippingRgn(d);
    DrawThemeMenuBackground (menuRectPtr, type);
    SetGWorld(saveWorld, saveDevice);    
    return;
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
	const BitMap *destBitMap;
	RGBColor origForeColor, origBackColor, foreColor, backColor;

	HLock(sicnHandle);
	destPort = TkMacOSXGetDrawablePort(d);
	GetGWorld(&saveWorld, &saveDevice);
	SetGWorld(destPort, NULL);
	TkMacOSXSetUpClippingRgn(d);
	TkMacOSXSetUpGraphicsPort(gc, destPort);
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
	destBitMap = GetPortBitMapForCopyBits(destPort);
	CopyBits(&sicnBitmap, destBitMap, &sicnBitmap.bounds, &destRect, GetPortTextMode(destPort), NULL);
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
    Rect r;
   
    destPort = TkMacOSXGetDrawablePort(d);
    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    TkMacOSXSetUpClippingRgn(d);
    r.top = y;
    r.left = x;
    r.bottom = y + height;
    r.right = x + width;
         
    DrawThemeMenuSeparator(&r);
}

/*
 *----------------------------------------------------------------------
 *
 *   AppearanceEntryDrawWrapper --
 *
 *      It routes to the Appearance Managers DrawThemeEntry, which will
 *      then call us back after setting up the drawing context.
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
    MenuTrackingData *mtdPtr,     
    Drawable d,
    Tk_FontMetrics *fmPtr, 
    Tk_Font tkfont, 
    int x, 
    int y, 
    int width, 
    int height)
{
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
        mtdPtr->virtualMenuTop, mtdPtr->virtualMenuBottom, theState,
        theType, tkThemeMenuItemDrawingUPP, 
        (unsigned long) &meData);
    
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
 * TkMacOSXHandleTearoffMenu() --
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
TkMacOSXHandleTearoffMenu(void)
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

    if (menuPtr->menuType != MASTER_MENU ) {
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
 * TkMacOSXSetHelpMenuItemCount --
 *
 *	Has to be called after the first call to InsertMenu. Sets
 *	up the global variable for the number of items in the
 *	unmodified help menu.
 *      NB. Nobody uses this any more, since you can get the number
 *      of system help items from HMGetHelpMenu trivially.
 *      But it is in the stubs table...
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Nothing.
 *
 *----------------------------------------------------------------------
 */

void 
TkMacOSXSetHelpMenuItemCount()
{
}

/*
 *----------------------------------------------------------------------
 *
 * TkMacOSXMenuClick --
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
TkMacOSXMenuClick()
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

	if (((parentDisabled || (mePtr->state == ENTRY_DISABLED)))
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
            CGrafPtr saveWorld;
            GDHandle saveDevice;
            GWorldPtr destPort;
#ifdef USE_ATSU
            int runLengths;
            CFStringRef stringRef;
            ATSUTextLayout textLayout;
            UniCharCount runLength;
            ATSUStyle        style;
            int              length;
            int err;
            Str255 fontName;
            SInt16 fontSize;
            Style  fontStyle;
            ATSUAttributeValuePtr valuePtr;
            ByteCount valueSize;
            Fixed fixedSize;
            short iFONDNumber;
            ATSUFontID fontID;
            ATSUAttributeTag tag;
            
            GetThemeFont (kThemeMenuItemFont, smSystemScript, fontName, &fontSize, &fontStyle);
            if ((err = ATSUCreateStyle(&style)) != noErr) {
                fprintf(stderr,"ATSUCreateStyle failed, %d\n", err);
                return;
            }
            fixedSize = fontSize<<16;
            tag = kATSUSizeTag;
            valueSize = sizeof(fixedSize);
            valuePtr = &fixedSize;
            if ((err=ATSUSetAttributes(style, 1, &tag, &valueSize, &valuePtr))!= noErr) {
                fprintf(stderr,"ATSUSetAttributes failed,%d\n", err );
            }

	    GetFNum(fontName, &iFONDNumber);
            ATSUFONDtoFontID(iFONDNumber, NULL, &fontID);
            tag = kATSUFontTag;
            valueSize = sizeof(fontID);
            valuePtr = &fontID;
            if ((err=ATSUSetAttributes(style, 1, &tag, &valueSize, &valuePtr))!= noErr) {
                fprintf(stderr,"ATSUSetAttributes failed,%d\n", err );
            }

#endif
    	    
    	    GetEntryText(mePtr, &itemTextDString);
#ifdef USE_ATSU
            runLengths = 1;
            length = Tcl_DStringLength(&itemTextDString);
            stringRef = CFStringCreateWithCString(NULL, Tcl_DStringValue(&itemTextDString), GetApplicationTextEncoding());
            if (!stringRef) {
                fprintf(stderr,"CFStringCreateWithCString failed\n");
            }
            if ((err=ATSUCreateTextLayoutWithTextPtr(CFStringGetCharactersPtr(stringRef), 0, length, length,
                    1, &runLengths, &style, &textLayout)) != noErr) {
                fprintf(stderr,"ATSUCreateTextLayoutWithTextPtr failed, %d\n", err);
                return;
            }
#endif
    	    
    	    /* Somehow DrawChars is changing the colors, it is odd, since
    	       it works for the Apple Platinum Appearance, but not for
    	       some Kaleidoscope Themes...  Untill I can figure out what
    	       exactly is going on, this will have to do: */

            destPort = TkMacOSXGetDrawablePort(d);
            GetGWorld(&saveWorld, &saveDevice);
            SetGWorld(destPort, NULL);
            TkMacOSXSetUpGraphicsPort(gc, destPort);

	    MoveTo((short) leftEdge, (short) baseline);
	    Tcl_UtfToExternalDString(NULL, Tcl_DStringValue(&itemTextDString), 
	            Tcl_DStringLength(&itemTextDString), &convertedTextDString);
#ifdef USE_ATSU
            xLocation = leftEdge<<16;
            yLocation = baseline<<16;
            ATSUDrawText(textLayout,kATSUFromTextBeginning, kATSUToTextEnd, xLocation, yLocation);
            ATSUDisposeTextLayout(textLayout);
            CFRelease(stringRef);
#else
	    DrawText(Tcl_DStringValue(&convertedTextDString), 0, 
	            Tcl_DStringLength(&convertedTextDString));
#endif
           
	    /* Tk_DrawChars(menuPtr->display, d, gc,
		    tkfont, Tcl_DStringValue(&itemTextDString), 
		    Tcl_DStringLength(&itemTextDString),
		    leftEdge, baseline); */
		    
	    Tcl_DStringFree(&itemTextDString);
    	}
    }

    if (mePtr->state == ENTRY_DISABLED) {
	if (menuPtr->disabledFgPtr == NULL) {
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
    if ((menuPtr->menuType == TEAROFF_MENU)
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
    CGrafPtr port;
    Rect     bounds;
   
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
    GetPort(&port);
    GetPortBounds(port,&bounds);
    event.x_root = where.h + bounds.left;
    event.y_root = where.v + bounds.top;
    event.state = TkMacOSXButtonKeyState();
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
InvalidateMDEFRgns(void)
{
    GDHandle saveDevice;
    GWorldPtr saveWorld, destPort;
    Point scratch;
    MacDrawable *macDraw;
    TkMacOSXWindowList *listPtr;
    
    if (totalMenuRgn == NULL) {
    	return;
    }
    
    GetGWorld(&saveWorld, &saveDevice);
    for (listPtr = tkMacOSXWindowListPtr ; listPtr != NULL; 
    	    listPtr = listPtr->nextPtr) {
    	macDraw = (MacDrawable *) Tk_WindowId(listPtr->winPtr);
    	if (macDraw->flags & TK_DRAWN_UNDER_MENU) {
    	    destPort = TkMacOSXGetDrawablePort(Tk_WindowId(listPtr->winPtr));
    	    SetGWorld(destPort, NULL);
    	    scratch.h = scratch.v = 0;
    	    GlobalToLocal(&scratch);
    	    OffsetRgn(totalMenuRgn, scratch.v, scratch.h);
    	    InvalWindowRgn(GetWindowFromPort(destPort),totalMenuRgn);
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
 * TkMacOSXClearMenubarActive --
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
TkMacOSXClearMenubarActive(void)
{
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

    tkThemeMenuItemDrawingUPP 
            = NewMenuItemDrawingUPP(tkThemeMenuItemDrawingProc);
            				
    /* 
     * We should just hardcode the utf-8 ellipsis character into 
     * 'elipsisString' here 
     */
    Tcl_ExternalToUtf(NULL, Tcl_GetEncoding(NULL, "macRoman"), 
		      "\311", /* ellipsis character */
		      -1, 0, NULL, elipsisString,
		      TCL_UTF_MAX + 1, NULL, NULL, NULL);
        
    useMDEFVar = Tcl_NewStringObj("::tk::mac::useCustomMDEF", -1);
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

/*
 *----------------------------------------------------------------------
 *
 * TkpPreprocessMacMenu --
 *
 *    Handle preprocessing of menubar if it exists.
 *
 * Results:
 *    None.
 *
 * Side effects:
 *    All post commands for the current menubar get executed.
 *
 *----------------------------------------------------------------------
 */

void
TkMacOSXPreprocessMenu()
{
    TkMenuReferences *mbRefPtr;
    int code;

    if ((currentMenuBarName != NULL) && (currentMenuBarInterp != NULL)) {
        mbRefPtr = TkFindMenuReferences(currentMenuBarInterp,
		currentMenuBarName);
        if ((mbRefPtr != NULL) && (mbRefPtr->menuPtr != NULL)) {
	    Tcl_Preserve((ClientData)currentMenuBarInterp);
	    code = TkPreprocessMenu(mbRefPtr->menuPtr->masterMenuPtr);
	    if ((code != TCL_OK) && (code != TCL_CONTINUE)
		    && (code != TCL_BREAK)) {
		Tcl_AddErrorInfo(currentMenuBarInterp,
			"\n    (menu preprocess)");
		Tcl_BackgroundError(currentMenuBarInterp);
	    }
	    Tcl_Release((ClientData)currentMenuBarInterp);
        }
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
    SInt16 message,	/* What action are we taking? */
    MenuRef menu,	/* The menu we are working with */
    Rect *menuRectPtr,	/* A pointer to the rect for the
    			* whole menu. */
    Point hitPt,	/* Where the mouse was clicked for
    			* the appropriate messages. */
    SInt16 *whichItem)	/* Output result. Which item was
			 * hit by the user? */
{
    TkMenu *menuPtr;
    Tcl_HashEntry *commandEntryPtr;
    int maxMenuHeight;
    MenuID menuID;
    BitMap screenBits;
    
    menuID = GetMenuID(menu);
    commandEntryPtr = Tcl_FindHashEntry(&commandTable, (char *) ((int)menuID));
    
    if (commandEntryPtr) {
        menuPtr = (TkMenu *) Tcl_GetHashValue(commandEntryPtr);
    } else {
        menuPtr = NULL;
    }

    switch (message) {
        case kMenuInitMsg:
            *whichItem = noErr;
            break;
        case kMenuDisposeMsg:
            break;
        case kMenuHiliteItemMsg: {
            HandleMenuHiliteMsg (menu, menuRectPtr, hitPt, whichItem, menuPtr);
            break;
        }
        case kMenuCalcItemMsg:
            HandleMenuCalcItemMsg (menu, menuRectPtr, hitPt, whichItem, menuPtr);
            break;
        case kMenuDrawItemsMsg: {
            /*
             * We do nothing  here, because we don't support the Menu Managers
             * dynamic item groups
             */
             
            break;
        }
        case kMenuThemeSavvyMsg:
            *whichItem = kThemeSavvyMenuResponse;
            break;
    	case kMenuSizeMsg:
            GetQDGlobalsScreenBits(&screenBits);
    	    maxMenuHeight = screenBits.bounds.bottom
    	    	    - screenBits.bounds.top
    	    	    - GetMBarHeight() - SCREEN_MARGIN;
    	    SetMenuWidth(menu, menuPtr->totalWidth );
    	    SetMenuHeight(menu,maxMenuHeight < menuPtr->totalHeight ? maxMenuHeight : menuPtr->totalHeight );
       	    break;
    	case kMenuDrawMsg:
            HandleMenuDrawMsg (menu, menuRectPtr, hitPt, whichItem, menuPtr);
            break;
    	case kMenuFindItemMsg:
            HandleMenuFindItemsMsg (menu, menuRectPtr, hitPt, whichItem, menuPtr);
            break;
    	case kMenuPopUpMsg:
            HandleMenuPopUpMsg (menu, menuRectPtr, hitPt, whichItem, menuPtr);
    	    break;
    }
}

void
HandleMenuHiliteMsg (MenuRef menu, 
        Rect *menuRectPtr, 
        Point hitPt, 
        SInt16 *whichItem,
        TkMenu *menuPtr)
{
    TkMenuEntry *mePtr = NULL;
    Tk_Font tkfont;
    Tk_FontMetrics fontMetrics;
    int oldItem;
    int newItem = -1;
    MDEFHiliteItemData * hidPtr = ( MDEFHiliteItemData *)whichItem;
    MenuTrackingData   mtd, *mtdPtr = &mtd;
    int err;
    oldItem = hidPtr->previousItem - 1;
    newItem = hidPtr->newItem - 1;
    
    err = GetMenuTrackingData(menu, mtdPtr);
    if (err !=noErr) {
        fprintf(stderr,"GetMenuTrackingData failed : %d\n", err );
        return;
    }
    
    if (oldItem >= 0) {
        Rect oldItemRect;
        int width;
        
        mePtr = menuPtr->entries[oldItem];
        if (mePtr->fontPtr == NULL) {
                tkfont = Tk_GetFontFromObj(menuPtr->tkwin, 
                        menuPtr->fontPtr);
        } else {
                tkfont = Tk_GetFontFromObj(menuPtr->tkwin,
                        mePtr->fontPtr);
        }
        Tk_GetFontMetrics(tkfont, &fontMetrics);
        
        width = (mePtr->entryFlags & ENTRY_LAST_COLUMN)
                ? menuPtr->totalWidth - mePtr->x : mePtr->width;
                
        /*
            * In Aqua, have to call EraseMenuBackground when you overdraw
            * a previously selected menu item, otherwise you will see the 
            * old select highlight under the transparency of the new menu item.
            */
            
        oldItemRect.left = menuRectPtr->left + mePtr->x;
        oldItemRect.right = oldItemRect.left +width; 
        oldItemRect.top = mtdPtr->virtualMenuTop + mePtr->y;
        oldItemRect.bottom = oldItemRect.top + mePtr->height;
        
        EraseMenuBackground(menu, & oldItemRect, NULL);
        
        AppearanceEntryDrawWrapper(mePtr, menuRectPtr, mtdPtr,
                (Drawable) &macMDEFDrawable, &fontMetrics, tkfont, 
                oldItemRect.left,
                oldItemRect.top,
                width,
                mePtr->height);
    }
    if (newItem != -1) {        
        mePtr = menuPtr->entries[newItem];
        if (mePtr->state != ENTRY_DISABLED) {
            TkActivateMenuEntry(menuPtr, newItem);
        }
        if (mePtr->fontPtr == NULL) {  
            tkfont = Tk_GetFontFromObj(menuPtr->tkwin, menuPtr->fontPtr);
        } else {
            tkfont = Tk_GetFontFromObj(menuPtr->tkwin, mePtr->fontPtr);
        }
        Tk_GetFontMetrics(tkfont, &fontMetrics);
        AppearanceEntryDrawWrapper(mePtr, menuRectPtr, mtdPtr,
            (Drawable) &macMDEFDrawable, &fontMetrics, tkfont,
            menuRectPtr->left + mePtr->x,
            mtdPtr->virtualMenuTop + mePtr->y,
            (mePtr->entryFlags & ENTRY_LAST_COLUMN) ?
            menuPtr->totalWidth - mePtr->x : mePtr->width,
            mePtr->height);
    }
    tkUseMenuCascadeRgn = 1;
    MenuSelectEvent(menuPtr);
    Tcl_ServiceAll();
    tkUseMenuCascadeRgn = 0;
    if (newItem!=-1 && mePtr->state != ENTRY_DISABLED) {
        TkActivateMenuEntry(menuPtr, -1);
    }

}

/*
 *----------------------------------------------------------------------
 *
 *   HandleMenuDrawMsg --
 *
 *      It handles the MenuDefProc's draw message.
 *
 * Results:
 *	A menu entry is drawn
 *
 * Side effects:
 *	None
 *
 *----------------------------------------------------------------------
 */
void
HandleMenuDrawMsg(MenuRef menu, 
        Rect *menuRectPtr, 
        Point hitPt, 
        SInt16 *whichItem,
        TkMenu *menuPtr)
{
    Tk_Font tkfont, menuFont;
    Tk_FontMetrics fontMetrics, entryMetrics;
    Tk_FontMetrics *fmPtr;
    TkMenuEntry *mePtr;
    int i;
    GDHandle device;
    TkMenu *searchMenuPtr;
    Rect menuClipRect;
    ThemeMenuType menuType;
    MenuTrackingData * mtdPtr = (MenuTrackingData *)whichItem;
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
        
    GetGWorld(&macMDEFDrawable.grafPtr, &device);
        
    if (menuPtr->menuRefPtr->topLevelListPtr != NULL) {
        menuType = kThemeMenuTypePullDown;
    } else if (menuPtr->menuRefPtr->parentEntryPtr != NULL) {
        menuType = kThemeMenuTypeHierarchical;
    } else {
        menuType = kThemeMenuTypePopUp;
    }
            
    DrawMenuBackground(menuRectPtr, (Drawable) &macMDEFDrawable, menuType);
    
    /*
        * Next, figure out scrolling information.
        */
    
    menuClipRect = *menuRectPtr;
    if ((menuClipRect.bottom - menuClipRect.top) 
            < menuPtr->totalHeight) {
        if (mtdPtr->virtualMenuTop < menuRectPtr->top) {
            DrawSICN(SICN_RESOURCE_NUMBER, UP_ARROW, 
                (Drawable) &macMDEFDrawable,
                menuPtr->textGC, 
                menuRectPtr->left + menuPtr->entries[1]->indicatorSpace,
                menuRectPtr->top);
            menuClipRect.top += SICN_HEIGHT;
        }
        if ((mtdPtr->virtualMenuTop + menuPtr->totalHeight)
                > menuRectPtr->bottom) {
            DrawSICN(SICN_RESOURCE_NUMBER, DOWN_ARROW,
                (Drawable) &macMDEFDrawable,
                menuPtr->textGC, 
                menuRectPtr->left + menuPtr->entries[1]->indicatorSpace,
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
        if (mtdPtr->virtualMenuTop + mePtr->y + mePtr->height
                < menuClipRect.top) {
            continue;
        } else if (mtdPtr->virtualMenuTop + mePtr->y
                > menuClipRect.bottom) {
            continue;
        }
        ClipRect(&menuClipRect);
        if (mePtr->fontPtr == NULL) {
            fmPtr = &fontMetrics;
            tkfont = menuFont;
        } else {
            tkfont = Tk_GetFontFromObj(menuPtr->tkwin, mePtr->fontPtr);
            Tk_GetFontMetrics(tkfont, &entryMetrics);
            fmPtr = &entryMetrics;
        }
        AppearanceEntryDrawWrapper(mePtr, menuRectPtr, mtdPtr,
            (Drawable) &macMDEFDrawable, fmPtr, tkfont, 
            menuRectPtr->left + mePtr->x,
            mtdPtr->virtualMenuTop + mePtr->y,
            (mePtr->entryFlags & ENTRY_LAST_COLUMN) ?
                menuPtr->totalWidth - mePtr->x : mePtr->width,
            menuPtr->entries[i]->height);
    }
    mtdPtr->virtualMenuBottom = mtdPtr->virtualMenuTop 
            + menuPtr->totalHeight;
    if (!EmptyRgn(utilRgn)) {
        SetClip(utilRgn);
        SetEmptyRgn(utilRgn);
    }
    MDEFScrollFlag = 1;
}

/*
 *----------------------------------------------------------------------
 *
 *   HandleMenuFindItemsMsg --
 *
 *      It handles the MenuDefProc's FindItems message.  We have to
 *      respond by filling in the itemSelected, itemUnderMouse and
 *      itemRect fields.  This is also the time to scroll the menu if
 *      it is too long to fit on the screen.
 *
 * Results:
 *	The Menu system is informed of the selected item & the item
 *      under the mouse.
 *
 * Side effects:
 *	The menu might get scrolled.
 *
 *----------------------------------------------------------------------
 */
void
HandleMenuFindItemsMsg (MenuRef menu, 
        Rect *menuRectPtr, 
        Point hitPt, 
        SInt16 *whichItem,
        TkMenu *menuPtr)
{
    TkMenuEntry *parentEntryPtr;
    Tk_Font tkfont;
    Tk_FontMetrics fontMetrics, entryMetrics;
    Tk_FontMetrics *fmPtr;
    TkMenuEntry *mePtr;
    int i;
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
    Rect menuClipRect;

    int hasTopScroll, hasBottomScroll;
    MenuTrackingData * mtdPtr = (MenuTrackingData *)whichItem;
    int                itemUnderMouse = -1;
    enum {
        DONT_SCROLL, DOWN_SCROLL, UP_SCROLL
    } scrollDirection;
    Rect updateRect;
    short scrollAmt = 0;
    RGBColor origForeColor, origBackColor;
    
    /*
     * Find out which item was hit. If it is the same as the old item,
     * we don't need to do anything.
     */

    if (PtInRect(hitPt, menuRectPtr)) {
        for (i = 0; i < menuPtr->numEntries; i++) {
            mePtr = menuPtr->entries[i];
            itemRect.left = menuRectPtr->left + mePtr->x;
            itemRect.top = mtdPtr->virtualMenuTop + mePtr->y;
            if (mePtr->entryFlags & ENTRY_LAST_COLUMN) {
                itemRect.right = itemRect.left + menuPtr->totalWidth
                        - mePtr->x;
            } else {
                itemRect.right = itemRect.left + mePtr->width;
            }
            itemRect.bottom = itemRect.top
                    + mePtr->height;
            if (PtInRect(hitPt, &itemRect)) {
                if ((mePtr->type == SEPARATOR_ENTRY)
                        || (mePtr->state == ENTRY_DISABLED)) {
                    newItem = -1;
                    itemUnderMouse = i;
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
                        itemUnderMouse = i;
                    } else {         	    
                        newItem = i;
                        itemUnderMouse = i;
                    }
                }
                break;
            }
        }
    } else {
    }

    /*
     * Now we need to take care of scrolling the menu.
     */
    
    hasTopScroll = mtdPtr->virtualMenuTop < menuRectPtr->top;
    hasBottomScroll = mtdPtr->virtualMenuBottom > menuRectPtr->bottom;
    scrollDirection = DONT_SCROLL;
    if (hasTopScroll && (hitPt.v < menuRectPtr->top + SICN_HEIGHT)) {
        newItem = -1;
        scrollDirection = DOWN_SCROLL;
    } else if (hasBottomScroll && (hitPt.v > (menuRectPtr->bottom - SICN_HEIGHT))) {
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

    mtdPtr->itemSelected = newItem + 1;
    mtdPtr->itemUnderMouse = itemUnderMouse + 1;
    mtdPtr->itemRect = itemRect;
    
    GetGWorld(&macMDEFDrawable.grafPtr, &device);
    GetForeColor(&origForeColor);
    GetBackColor(&origBackColor);

    if (scrollDirection == UP_SCROLL) {
        scrollAmt = menuClipRect.bottom - hitPt.v;
        if (scrollAmt < menuRectPtr->bottom 
                - mtdPtr->virtualMenuBottom) {
            scrollAmt = menuRectPtr->bottom - mtdPtr->virtualMenuBottom;
        }
        if (!hasTopScroll && ((mtdPtr->virtualMenuTop + scrollAmt)
                < menuRectPtr->top)) {
            SetRect(&updateRect, menuRectPtr->left,
                    mtdPtr->virtualMenuTop, menuRectPtr->right,
                    mtdPtr->virtualMenuTop + SICN_HEIGHT);
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
        if (scrollAmt > menuRectPtr->top - mtdPtr->virtualMenuTop) {
            scrollAmt = menuRectPtr->top - mtdPtr->virtualMenuTop;
        }

        if (!hasBottomScroll && ((mtdPtr->virtualMenuBottom + scrollAmt)
                > menuRectPtr->bottom)) {
            SetRect(&updateRect, menuRectPtr->left, 
                    mtdPtr->virtualMenuBottom - SICN_HEIGHT,
                    menuRectPtr->right, mtdPtr->virtualMenuBottom);
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
        
        ScrollMenuImage(menu, menuRectPtr, 0, scrollAmt, NULL);
        mtdPtr->virtualMenuTop += scrollAmt;
        mtdPtr->virtualMenuBottom += scrollAmt;
#if 0
        GetRegionBounds(updateRgn,&updateRect);
        DisposeRgn(updateRgn);
        if (mtdPtr->virtualMenuTop == menuRectPtr->top) {
            updateRect.top -= SICN_HEIGHT;
        }
        if (mtdPtr->virtualMenuBottom == menuRectPtr->bottom) {
            updateRect.bottom += SICN_HEIGHT;
        }
        ClipRect(&updateRect);
        EraseRect(&updateRect);
        menuFont = Tk_GetFontFromObj(menuPtr->tkwin, menuPtr->fontPtr);
        Tk_GetFontMetrics(menuFont, &fontMetrics);    	    
        for (i = 0; i < menuPtr->numEntries; i++) {
            mePtr = menuPtr->entries[i];
            if (mtdPtr->virtualMenuTop + mePtr->y + mePtr->height
                    < updateRect.top) {
                continue;
            } else if (mtdPtr->virtualMenuTop + mePtr->y
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
            AppearanceEntryDrawWrapper(mePtr, menuRectPtr, mtdPtr,
                (Drawable) &macMDEFDrawable, fmPtr, tkfont, 
                menuRectPtr->left + mePtr->x,
                mtdPtr->virtualMenuTop + mePtr->y,
                (mePtr->entryFlags & ENTRY_LAST_COLUMN) ?
                    menuPtr->totalWidth - mePtr->x : mePtr->width,
                menuPtr->entries[i]->height);
        }	    	
#endif
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
            unsigned long dummy;
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
}

/*
 *----------------------------------------------------------------------
 *
 *   HandleMenuPopUpMsg --
 *
 *      It handles the MenuDefProc's PopUp message.  The menu is
 *	posted with the selected item at the point given in hitPt.
 *
 * Results:
 *	A menu is posted.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
HandleMenuPopUpMsg (MenuRef menu, 
        Rect *menuRectPtr, 
        Point hitPt, 
        SInt16 *whichItem,
        TkMenu *menuPtr)
{
    int maxMenuHeight;
    int oldItem;
    Rect portRect;
    BitMap screenBits;

    /*
        * Note that for some oddball reason, h and v are reversed in the
        * point given to us by the MDEF.
        */
    GetQDGlobalsScreenBits(&screenBits);

    oldItem = *whichItem;
    if (oldItem >= menuPtr->numEntries) {
        oldItem = -1;
    }
    portRect.top = 0;
    portRect.bottom = 1280;
    maxMenuHeight = screenBits.bounds.bottom
            - screenBits.bounds.top
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
}

/*
 *----------------------------------------------------------------------
 *
 *   HandleMenuCalcItemMsg --
 *
 *      It handles the MenuDefProc's CalcItem message.  It is supposed
 *      to calculate the Rect of the menu entry in whichItem in the
 *      menu, and put that in menuRectPtr.  I assume this works, but I
 *      have never seen the MenuManager send this message.
 *
 * Results:
 *	The Menu Manager is informed of the bounding rect of a 
 *	menu rect.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
void
HandleMenuCalcItemMsg(MenuRef menu, 
        Rect *menuRectPtr, 
        Point hitPt, 
        SInt16 *whichItem,
        TkMenu *menuPtr)
{
    TkMenuEntry *mePtr;
    MenuTrackingData   mtd, *mtdPtr = &mtd;
    int err, virtualTop;
    
    err = GetMenuTrackingData(menu, mtdPtr);
    if (err == noErr) {
        virtualTop = mtdPtr->virtualMenuTop;
    } else {
        virtualTop = 0;
    }
    
    mePtr = menuPtr->entries[*whichItem];
    menuRectPtr->left = mePtr->x;
    menuRectPtr->top = mePtr->y - virtualTop;
    if (mePtr->entryFlags & ENTRY_LAST_COLUMN) {
        menuRectPtr->right = menuPtr->totalWidth;
    } else {
        menuRectPtr->right = mePtr->x + mePtr->width;
    }
    menuRectPtr->bottom = menuRectPtr->top
            + mePtr->height;
}
