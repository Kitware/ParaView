/* 
 * tkWinMenu.c --
 *
 *	This module implements the Windows platform-specific features of menus.
 *
 * Copyright (c) 1996-1998 by Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#define OEMRESOURCE
#include "tkWinInt.h"
#include "tkMenu.h"

#include <string.h>

/*
 * The class of the window for popup menus.
 */

#define MENU_CLASS_NAME "MenuWindowClass"

/*
 * Used to align a windows bitmap inside a rectangle
 */

#define ALIGN_BITMAP_LEFT   0x00000001
#define ALIGN_BITMAP_RIGHT  0x00000002
#define ALIGN_BITMAP_TOP    0x00000004
#define ALIGN_BITMAP_BOTTOM 0x00000008

/*
 * Platform-specific menu flags:
 *
 * MENU_SYSTEM_MENU	Non-zero means that the Windows menu handle
 *			was retrieved with GetSystemMenu and needs
 *			to be disposed of specially.
 * MENU_RECONFIGURE_PENDING
 *			Non-zero means that an idle handler has
 *			been set up to reconfigure the Windows menu
 *			handle for this menu.
 */

#define MENU_SYSTEM_MENU	    MENU_PLATFORM_FLAG1
#define MENU_RECONFIGURE_PENDING    MENU_PLATFORM_FLAG2

static int indicatorDimensions[2];
				/* The dimensions of the indicator space
				 * in a menu entry. Calculated at init
				 * time to save time. */

typedef struct ThreadSpecificData {
    Tcl_HashTable commandTable;
				/* A map of command ids to menu entries */
    int inPostMenu;		/* We cannot be re-entrant like X Windows. */
    WORD lastCommandID;	        /* The last command ID we allocated. */
    HWND menuHWND;		/* A window to service popup-menu messages
				 * in. */
    int oldServiceMode;	        /* Used while processing a menu; we need
				 * to set the event mode specially when we
				 * enter the menu processing modal loop
				 * and reset it when menus go away. */
    TkMenu *modalMenuPtr;	/* The menu we are processing inside the modal
				 * loop. We need this to reset all of the 
				 * active items when menus go away since
				 * Windows does not see fit to give this
				 * to us when it sends its WM_MENUSELECT. */
    Tcl_HashTable winMenuTable;
				/* Need this to map HMENUs back to menuPtrs */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

static OSVERSIONINFO versionInfo;
				/* So we don't have to keep doing this */

/*
 * The following are default menu value strings.
 */

static int defaultBorderWidth;	/* The windows default border width. */
static Tcl_DString menuFontDString;
				/* A buffer to store the default menu font
				 * string. */
TCL_DECLARE_MUTEX(winMenuMutex)

/*
 * Forward declarations for procedures defined later in this file:
 */

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
static void		DrawMenuUnderline _ANSI_ARGS_((TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Drawable d, GC gc,
			    Tk_Font tkfont, CONST Tk_FontMetrics *fmPtr, int x,
			    int y, int width, int height));
static void		DrawWindowsSystemBitmap _ANSI_ARGS_((
			    Display *display, Drawable drawable, 
			    GC gc, CONST RECT *rectPtr, int bitmapID,
			    int alignFlags));
static void		FreeID _ANSI_ARGS_((int commandID));
static TCHAR *		GetEntryText _ANSI_ARGS_((TkMenuEntry *mePtr));
static void		GetMenuAccelGeometry _ANSI_ARGS_((TkMenu *menuPtr,
			    TkMenuEntry *mePtr, Tk_Font tkfont,
			    CONST Tk_FontMetrics *fmPtr, int *widthPtr,
			    int *heightPtr));
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
static int		GetNewID _ANSI_ARGS_((TkMenuEntry *mePtr,
			    int *menuIDPtr));
static void		MenuExitProc _ANSI_ARGS_((ClientData clientData));
static int		MenuKeyBindProc _ANSI_ARGS_((
			    ClientData clientData, 
			    Tcl_Interp *interp, XEvent *eventPtr,
			    Tk_Window tkwin, KeySym keySym));
static void		MenuSelectEvent _ANSI_ARGS_((TkMenu *menuPtr));
static void		ReconfigureWindowsMenu _ANSI_ARGS_((
			    ClientData clientData));
static void		RecursivelyClearActiveMenu _ANSI_ARGS_((
			    TkMenu *menuPtr));
static void		SetDefaults _ANSI_ARGS_((int firstTime));
static LRESULT CALLBACK	TkWinMenuProc _ANSI_ARGS_((HWND hwnd,
			    UINT message, WPARAM wParam,
			    LPARAM lParam));



/*
 *----------------------------------------------------------------------
 *
 * GetNewID --
 *
 *	Allocates a new menu id and marks it in use.
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
GetNewID(mePtr, menuIDPtr)
    TkMenuEntry *mePtr;		/* The menu we are working with */
    int *menuIDPtr;		/* The resulting id */
{
    int found = 0;
    int newEntry;
    Tcl_HashEntry *commandEntryPtr;
    WORD returnID;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    WORD curID = tsdPtr->lastCommandID + 1;

    /*
     * The following code relies on WORD wrapping when the highest value is
     * incremented.
     */
    
    while (curID != tsdPtr->lastCommandID) {
    	commandEntryPtr = Tcl_CreateHashEntry(&tsdPtr->commandTable,
		(char *) curID, &newEntry);
    	if (newEntry == 1) {
    	    found = 1;
    	    returnID = curID;
    	    break;
    	}
    	curID++;
    }

    if (found) {
    	Tcl_SetHashValue(commandEntryPtr, (char *) mePtr);
    	*menuIDPtr = (int) returnID;
    	tsdPtr->lastCommandID = returnID;
    	return TCL_OK;
    } else {
    	return TCL_ERROR;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FreeID --
 *
 *	Marks the itemID as free.
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
FreeID(commandID)
    int commandID;
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    Tcl_HashEntry *entryPtr = Tcl_FindHashEntry(&tsdPtr->commandTable,
	    (char *) commandID);
    
    if (entryPtr != NULL) {
    	 Tcl_DeleteHashEntry(entryPtr);
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
 *	Standard TCL error.
 *
 * Side effects:
 *	Allocates a Windows menu handle and places it in the platformData
 *	field of the menuPtr.
 *
 *----------------------------------------------------------------------
 */

int
TkpNewMenu(menuPtr)
    TkMenu *menuPtr;	/* The common structure we are making the
			 * platform structure for. */
{
    HMENU winMenuHdl;
    Tcl_HashEntry *hashEntryPtr;
    int newEntry;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    winMenuHdl = CreatePopupMenu();
    
    if (winMenuHdl == NULL) {
    	Tcl_AppendResult(menuPtr->interp, "No more menus can be allocated.",
    		(char *) NULL);
    	return TCL_ERROR;
    }

    /*
     * We hash all of the HMENU's so that we can get their menu ptrs
     * back when dispatch messages.
     */

    hashEntryPtr = Tcl_CreateHashEntry(&tsdPtr->winMenuTable, (char *) winMenuHdl,
	    &newEntry);
    Tcl_SetHashValue(hashEntryPtr, (char *) menuPtr);

    menuPtr->platformData = (TkMenuPlatformData) winMenuHdl;
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
TkpDestroyMenu(menuPtr)
    TkMenu *menuPtr;	    /* The common menu structure */
{
    HMENU winMenuHdl = (HMENU) menuPtr->platformData;
    char *searchName;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (menuPtr->menuFlags & MENU_RECONFIGURE_PENDING) {
	Tcl_CancelIdleCall(ReconfigureWindowsMenu, (ClientData) menuPtr);
    }
    
    if (winMenuHdl == NULL) {
	return;
    }

    if (menuPtr->menuFlags & MENU_SYSTEM_MENU) {
	TkMenuEntry *searchEntryPtr;
	Tcl_HashTable *tablePtr = TkGetMenuHashTable(menuPtr->interp);
	char *menuName = Tcl_GetHashKey(tablePtr, 
		menuPtr->menuRefPtr->hashEntryPtr);

	/*
	 * Search for the menu in the menubar, if it is present, get the
	 * wrapper window associated with the toplevel and reset its
	 * system menu to the default menu.
	 */

	for (searchEntryPtr = menuPtr->menuRefPtr->parentEntryPtr;
	     searchEntryPtr != NULL;
	     searchEntryPtr = searchEntryPtr->nextCascadePtr) {
	    searchName = Tcl_GetStringFromObj(searchEntryPtr->namePtr, NULL);
	    if (strcmp(searchName, menuName) == 0) {
		Tk_Window parentTopLevelPtr = searchEntryPtr
		    ->menuPtr->parentTopLevelPtr;

		if (parentTopLevelPtr != NULL) {
		    GetSystemMenu(TkWinGetWrapperWindow(parentTopLevelPtr),
			    TRUE);
		}
		break;
	    }
	}
    } else {
	Tcl_HashEntry *hashEntryPtr;
 
	/*
	 * Remove the menu from the menu hash table, then destroy the handle.
	 */

	hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->winMenuTable, 
                (char *) winMenuHdl);
	if (hashEntryPtr != NULL) {
	    Tcl_DeleteHashEntry(hashEntryPtr);
	}
 	DestroyMenu(winMenuHdl);
    }
    menuPtr->platformData = NULL;

    if (menuPtr == tsdPtr->modalMenuPtr) {
	tsdPtr->modalMenuPtr = NULL;
    }
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
TkpDestroyMenuEntry(mePtr)
    TkMenuEntry *mePtr;		    /* The entry to destroy */
{
    TkMenu *menuPtr = mePtr->menuPtr;
    HMENU winMenuHdl = (HMENU) menuPtr->platformData;

    if (NULL != winMenuHdl) {
        if (!(menuPtr->menuFlags & MENU_RECONFIGURE_PENDING)) {
	    menuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
	    Tcl_DoWhenIdle(ReconfigureWindowsMenu, (ClientData) menuPtr);
	}
    }
    FreeID((int) mePtr->platformEntryData);
    mePtr->platformEntryData = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * GetEntryText --
 *
 *	Given a menu entry, gives back the text that should go in it.
 *	Separators should be done by the caller, as they have to be
 *	handled specially. Allocates the memory with alloc. The caller
 *	should free the memory.
 *
 * Results:
 *	itemText points to the new text for the item.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
GetEntryText(mePtr)
    TkMenuEntry *mePtr;		/* A pointer to the menu entry. */
{
    char *itemText;

    if (mePtr->type == TEAROFF_ENTRY) {
	itemText = ckalloc(sizeof("(Tear-off)"));
	strcpy(itemText, "(Tear-off)");
    } else if (mePtr->imagePtr != NULL) {
	itemText = ckalloc(sizeof("(Image)"));
	strcpy(itemText, "(Image)");
    } else if (mePtr->bitmapPtr != NULL) {
	itemText = ckalloc(sizeof("(Pixmap)"));
	strcpy(itemText, "(Pixmap)");
    } else if (mePtr->labelPtr == NULL || mePtr->labelLength == 0) {
	itemText = ckalloc(sizeof("( )"));
	strcpy(itemText, "( )");
    } else {
	int i;
	char *label = (mePtr->labelPtr == NULL) ? "" 
		: Tcl_GetStringFromObj(mePtr->labelPtr, NULL);
	char *accel = (mePtr->accelPtr == NULL) ? "" 
		: Tcl_GetStringFromObj(mePtr->accelPtr, NULL);
	char *p, *next;
	Tcl_DString itemString;

	/*
	 * We have to construct the string with an ampersand
	 * preceeding the underline character, and a tab seperating
	 * the text and the accel text. We have to be careful with
	 * ampersands in the string.
	 */

	Tcl_DStringInit(&itemString);

	for (p = label, i = 0; *p != '\0'; i++, p = next) {
	    if (i == mePtr->underline) {
		Tcl_DStringAppend(&itemString, "&", 1);
	    }
	    if (*p == '&') {
		Tcl_DStringAppend(&itemString, "&", 1);
	    }
	    next = Tcl_UtfNext(p);
	    Tcl_DStringAppend(&itemString, p, next - p);
	}
        if (mePtr->accelLength > 0) {
	    Tcl_DStringAppend(&itemString, "\t", 1);
	    for (p = accel, i = 0; *p != '\0'; i++, p = next) {
		if (*p == '&') {
		    Tcl_DStringAppend(&itemString, "&", 1);
		}
		next = Tcl_UtfNext(p);
		Tcl_DStringAppend(&itemString, p, next - p);
	    }
	} 	    

	itemText = ckalloc(Tcl_DStringLength(&itemString) + 1);
	strcpy(itemText, Tcl_DStringValue(&itemString));
	Tcl_DStringFree(&itemString);
    }
    return itemText;
}

/*
 *----------------------------------------------------------------------
 *
 * ReconfigureWindowsMenu --
 *
 *	Tears down and rebuilds the platform-specific part of this menu.
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
ReconfigureWindowsMenu(
    ClientData clientData)	    /* The menu we are rebuilding */
{
    TkMenu *menuPtr = (TkMenu *) clientData;
    TkMenuEntry *mePtr;
    HMENU winMenuHdl = (HMENU) menuPtr->platformData;
    TCHAR *itemText = NULL;
    const TCHAR *lpNewItem;
    UINT flags;
    UINT itemID;
    int i, count, systemMenu = 0, base;
    int width, height;
    Tcl_DString translatedText;
  
    if (NULL == winMenuHdl) {
    	return;
    }

    /*
     * Reconstruct the entire menu. Takes care of nasty system menu and index
     * problem.
     *
     */

    if ((menuPtr->menuType == MENUBAR)
	    && (menuPtr->parentTopLevelPtr != NULL)) {
	width = Tk_Width(menuPtr->parentTopLevelPtr);
	height = Tk_Height(menuPtr->parentTopLevelPtr);
    }

    base = (menuPtr->menuFlags & MENU_SYSTEM_MENU) ? 7 : 0;
    count = GetMenuItemCount(winMenuHdl);
    for (i = base; i < count; i++) {
	RemoveMenu(winMenuHdl, base, MF_BYPOSITION);
    }

    count = menuPtr->numEntries;
    for (i = 0; i < count; i++) {
	mePtr = menuPtr->entries[i];
	lpNewItem = NULL;
	flags = MF_BYPOSITION;
	itemID = 0;
	Tcl_DStringInit(&translatedText);

	if ((menuPtr->menuType == MENUBAR) && (mePtr->type == TEAROFF_ENTRY)) {
	    continue;
	}

	if (mePtr->type == SEPARATOR_ENTRY) {
	    flags |= MF_SEPARATOR;
	} else {
	    itemText = GetEntryText(mePtr);
	    if ((menuPtr->menuType == MENUBAR)
		    || (menuPtr->menuFlags & MENU_SYSTEM_MENU)) {
		Tcl_UtfToExternalDString(NULL, itemText, -1, &translatedText);
		lpNewItem = Tcl_DStringValue(&translatedText);
	    } else {
		lpNewItem = (LPCTSTR) mePtr;
		flags |= MF_OWNERDRAW;
	    }

    	    /*
    	     * Set enabling and disabling correctly.
    	     */

	    if (mePtr->state == ENTRY_DISABLED) {
		flags |= MF_DISABLED;
	    }
    	    
    	    /*
    	     * Set the check mark for check entries and radio entries.
    	     */
	    
	    if (((mePtr->type == CHECK_BUTTON_ENTRY)
		    || (mePtr->type == RADIO_BUTTON_ENTRY))
		    && (mePtr->entryFlags & ENTRY_SELECTED)) {
		flags |= MF_CHECKED;
	    }

	    if (mePtr->columnBreak) {
		flags |= MF_MENUBREAK;
	    }

	    itemID = (int) mePtr->platformEntryData;
	    if ((mePtr->type == CASCADE_ENTRY)
		    && (mePtr->childMenuRefPtr != NULL)
		    && (mePtr->childMenuRefPtr->menuPtr != NULL)) {
		HMENU childMenuHdl = (HMENU) mePtr->childMenuRefPtr->menuPtr
		    ->platformData;
		if (childMenuHdl != NULL) {
		    itemID = (UINT) childMenuHdl;
		    flags |= MF_POPUP;
		}
		if ((menuPtr->menuType == MENUBAR) 
			&& !(mePtr->childMenuRefPtr->menuPtr->menuFlags
				& MENU_SYSTEM_MENU)) {
		    Tcl_DString ds;
		    TkMenuReferences *menuRefPtr;
		    TkMenu *systemMenuPtr = mePtr->childMenuRefPtr
			->menuPtr;

		    Tcl_DStringInit(&ds);
		    Tcl_DStringAppend(&ds,
			    Tk_PathName(menuPtr->masterMenuPtr->tkwin), -1);
		    Tcl_DStringAppend(&ds, ".system", 7);

		    menuRefPtr = TkFindMenuReferences(menuPtr->interp,
			    Tcl_DStringValue(&ds));
		    
		    Tcl_DStringFree(&ds);

		    if ((menuRefPtr != NULL) 
			    && (menuRefPtr->menuPtr != NULL)
			    && (menuPtr->parentTopLevelPtr != NULL)
			    && (systemMenuPtr->masterMenuPtr
				    == menuRefPtr->menuPtr)) {
			HMENU systemMenuHdl = 
			    (HMENU) systemMenuPtr->platformData;
			HWND wrapper = TkWinGetWrapperWindow(menuPtr
				->parentTopLevelPtr);
			if (wrapper != NULL) {
			    DestroyMenu(systemMenuHdl);
			    systemMenuHdl = GetSystemMenu(wrapper, FALSE);
			    systemMenuPtr->menuFlags |= MENU_SYSTEM_MENU;
			    systemMenuPtr->platformData = 
				(TkMenuPlatformData) systemMenuHdl;
			    if (!(systemMenuPtr->menuFlags 
				    & MENU_RECONFIGURE_PENDING)) {
				systemMenuPtr->menuFlags 
				    |= MENU_RECONFIGURE_PENDING;
				Tcl_DoWhenIdle(ReconfigureWindowsMenu,
					(ClientData) systemMenuPtr);
			    }
			}
		    }
		}
		if (mePtr->childMenuRefPtr->menuPtr->menuFlags 
			& MENU_SYSTEM_MENU) {
		    systemMenu++;
		}
	    }
	}
	if (!systemMenu) {
	    InsertMenu(winMenuHdl, 0xFFFFFFFF, flags, itemID, lpNewItem);
	}
	Tcl_DStringFree(&translatedText);
	if (itemText != NULL) {
	    ckfree(itemText);
	    itemText = NULL;
	}
    }


    if ((menuPtr->menuType == MENUBAR) 
	    && (menuPtr->parentTopLevelPtr != NULL)) {
	DrawMenuBar(TkWinGetWrapperWindow(menuPtr->parentTopLevelPtr));
	Tk_GeometryRequest(menuPtr->parentTopLevelPtr, width, height);
    }
    
    menuPtr->menuFlags &= ~(MENU_RECONFIGURE_PENDING);
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
TkpPostMenu(interp, menuPtr, x, y)
    Tcl_Interp *interp;
    TkMenu *menuPtr;
    int x;
    int y;
{
    HMENU winMenuHdl = (HMENU) menuPtr->platformData;
    int result, flags;
    RECT noGoawayRect;
    POINT point;
    Tk_Window parentWindow = Tk_Parent(menuPtr->tkwin);
    int oldServiceMode = Tcl_GetServiceMode();
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    tsdPtr->inPostMenu++;

    if (menuPtr->menuFlags & MENU_RECONFIGURE_PENDING) {
	Tcl_CancelIdleCall(ReconfigureWindowsMenu, (ClientData) menuPtr);
	ReconfigureWindowsMenu((ClientData) menuPtr);
    }

    result = TkPreprocessMenu(menuPtr);
    if (result != TCL_OK) {
	tsdPtr->inPostMenu--;
	return result;
    }

    /*
     * The post commands could have deleted the menu, which means
     * we are dead and should go away.
     */
    
    if (menuPtr->tkwin == NULL) {
	tsdPtr->inPostMenu--;
    	return TCL_OK;
    }

    if (NULL == parentWindow) {
	noGoawayRect.top = y - 50;
	noGoawayRect.bottom = y + 50;
	noGoawayRect.left = x - 50;
	noGoawayRect.right = x + 50;
    } else {
	int left, top;
	Tk_GetRootCoords(parentWindow, &left, &top);
	noGoawayRect.left = left;
	noGoawayRect.top = top;
	noGoawayRect.bottom = noGoawayRect.top + Tk_Height(parentWindow);
	noGoawayRect.right = noGoawayRect.left + Tk_Width(parentWindow);
    }

    Tcl_SetServiceMode(TCL_SERVICE_NONE);
    
    /*
     * Make an assumption here. If the right button is down,
     * then we want to track it. Otherwise, track the left mouse button.
     */

    flags = TPM_LEFTALIGN;
    if (GetSystemMetrics(SM_SWAPBUTTON)) {
	if (GetAsyncKeyState(VK_LBUTTON) < 0) {
	    flags |= TPM_RIGHTBUTTON;
	} else {
	    flags |= TPM_LEFTBUTTON;
	}
    } else {
	if (GetAsyncKeyState(VK_RBUTTON) < 0) {
	    flags |= TPM_RIGHTBUTTON;
	} else {
	    flags |= TPM_LEFTBUTTON;
	}
    }

    TrackPopupMenu(winMenuHdl, flags, x, y, 0, 
	    tsdPtr->menuHWND, &noGoawayRect);
    Tcl_SetServiceMode(oldServiceMode);

    GetCursorPos(&point);
    Tk_PointerEvent(NULL, point.x, point.y);

    if (tsdPtr->inPostMenu) {
	tsdPtr->inPostMenu = 0;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuNewEntry --
 *
 *	Adds a pointer to a new menu entry structure with the platform-
 *	specific fields filled in.
 *
 * Results:
 *	Standard TCL error.
 *
 * Side effects:
 *	A new command ID is allocated and stored in the platformEntryData
 *	field of mePtr.
 *
 *----------------------------------------------------------------------
 */

int
TkpMenuNewEntry(mePtr)
    TkMenuEntry *mePtr;
{
    int commandID;
    TkMenu *menuPtr = mePtr->menuPtr;

    if (GetNewID(mePtr, &commandID) != TCL_OK) {
    	return TCL_ERROR;
    }

    if (!(menuPtr->menuFlags & MENU_RECONFIGURE_PENDING)) {
    	menuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
    	Tcl_DoWhenIdle(ReconfigureWindowsMenu, (ClientData) menuPtr);
    }
    
    mePtr->platformEntryData = (TkMenuPlatformEntryData) commandID;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinMenuProc --
 *
 *	The window proc for the dummy window we put popups in. This allows
 *	is to post a popup whether or not we know what the parent window
 *	is.
 *
 * Results:
 *	Returns whatever is appropriate for the message in question.
 *
 * Side effects:
 *	Normal side-effect for windows messages.
 *
 *----------------------------------------------------------------------
 */

static LRESULT CALLBACK
TkWinMenuProc(hwnd, message, wParam, lParam)
    HWND hwnd;
    UINT message;
    WPARAM wParam;
    LPARAM lParam;
{
    LRESULT lResult;

    if (!TkWinHandleMenuEvent(&hwnd, &message, &wParam, &lParam, &lResult)) {
	lResult = DefWindowProc(hwnd, message, wParam, lParam);
    }
    return lResult;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinHandleMenuEvent --
 *
 *	Filters out menu messages from messages passed to a top-level.
 *	Will respond appropriately to WM_COMMAND, WM_MENUSELECT,
 *	WM_MEASUREITEM, WM_DRAWITEM
 *
 * Result:
 *	Returns 1 if this handled the message; 0 if it did not.
 *
 * Side effects:
 *	All of the parameters may be modified so that the caller can
 *	think it is getting a different message. plResult points to
 *	the result that should be returned to windows from this message.
 *
 *----------------------------------------------------------------------
 */

int
TkWinHandleMenuEvent(phwnd, pMessage, pwParam, plParam, plResult)
    HWND *phwnd;
    UINT *pMessage;
    WPARAM *pwParam;
    LPARAM *plParam;
    LRESULT *plResult;
{
    Tcl_HashEntry *hashEntryPtr;
    int returnResult = 0;
    TkMenu *menuPtr;
    TkMenuEntry *mePtr;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    switch (*pMessage) {
	case WM_INITMENU:
	    TkMenuInit();
	    hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->winMenuTable, 
                    (char *) *pwParam);
	    if (hashEntryPtr != NULL) {
		tsdPtr->oldServiceMode = Tcl_SetServiceMode(TCL_SERVICE_ALL);
		menuPtr = (TkMenu *) Tcl_GetHashValue(hashEntryPtr);
		tsdPtr->modalMenuPtr = menuPtr;
		if (menuPtr->menuFlags & MENU_RECONFIGURE_PENDING) {
		    Tcl_CancelIdleCall(ReconfigureWindowsMenu, 
			    (ClientData) menuPtr);
		    ReconfigureWindowsMenu((ClientData) menuPtr);
		}
		if (!tsdPtr->inPostMenu) {
		    Tcl_Interp *interp;
		    int code;

		    interp = menuPtr->interp;
		    Tcl_Preserve((ClientData)interp);
		    code = TkPreprocessMenu(menuPtr);
		    if ((code != TCL_OK) && (code != TCL_CONTINUE)
			    && (code != TCL_BREAK)) {
			Tcl_AddErrorInfo(interp, "\n    (menu preprocess)");
			Tcl_BackgroundError(interp);
		    }
		    Tcl_Release((ClientData)interp);
		}
		TkActivateMenuEntry(menuPtr, -1);
		*plResult = 0;
		returnResult = 1;
	    } else {
		tsdPtr->modalMenuPtr = NULL;
	    }
	    break;

	case WM_SYSCOMMAND:
	case WM_COMMAND: {
	    TkMenuInit();
	    if (HIWORD(*pwParam) != 0) {
		break;
	    }
	    hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->commandTable,
		    (char *)LOWORD(*pwParam));
	    if (hashEntryPtr == NULL) {
		break;
	    }
	    mePtr = (TkMenuEntry *) Tcl_GetHashValue(hashEntryPtr);
	    if (mePtr != NULL) {
		TkMenuReferences *menuRefPtr;
		TkMenuEntry *parentEntryPtr;
		Tcl_Interp *interp;
		int code;

		/*
		 * We have to set the parent of this menu to be active
		 * if this is a submenu so that tearoffs will get the
		 * correct title.
		 */

		menuPtr = mePtr->menuPtr;
		menuRefPtr = TkFindMenuReferences(menuPtr->interp,
			Tk_PathName(menuPtr->tkwin));
		if ((menuRefPtr != NULL)
			&& (menuRefPtr->parentEntryPtr != NULL)) {
		    char *name;

		    for (parentEntryPtr = menuRefPtr->parentEntryPtr;
			 ; 
			 parentEntryPtr = 
			     parentEntryPtr->nextCascadePtr) {
			name = Tcl_GetStringFromObj(
			    parentEntryPtr->namePtr, NULL);
			if (strcmp(name, Tk_PathName(menuPtr->tkwin))
				== 0) {
			    break;
			}
		    }
		    if (parentEntryPtr->menuPtr->entries[parentEntryPtr->index]
			    ->state != ENTRY_DISABLED) {
			TkActivateMenuEntry(parentEntryPtr->menuPtr, 
				parentEntryPtr->index);
		    }
		}

		interp = menuPtr->interp;
		Tcl_Preserve((ClientData)interp);
		code = TkInvokeMenu(interp, menuPtr, mePtr->index);
		if (code != TCL_OK && code != TCL_CONTINUE
			&& code != TCL_BREAK) {
		    Tcl_AddErrorInfo(interp, "\n    (menu invoke)");
		    Tcl_BackgroundError(interp);
		}
		Tcl_Release((ClientData)interp);
	    }
	    *plResult = 0;
	    returnResult = 1;
	    break;
	}


	case WM_MENUCHAR: {
	    unsigned char menuChar = (unsigned char) LOWORD(*pwParam);
	    hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->winMenuTable, 
                    (char *) *plParam);
	    if (hashEntryPtr != NULL) {
		int i;

		*plResult = 0;
		menuPtr = (TkMenu *) Tcl_GetHashValue(hashEntryPtr);
		for (i = 0; i < menuPtr->numEntries; i++) {
		    int underline;
		    char *label;

		    underline = menuPtr->entries[i]->underline;
		    if (menuPtr->entries[i]->labelPtr != NULL) {
			label = Tcl_GetStringFromObj(
				menuPtr->entries[i]->labelPtr, NULL);
		    }
		    if ((-1 != underline) 
			    && (NULL != menuPtr->entries[i]->labelPtr)
			    && (CharUpper((LPTSTR) menuChar) 
			    == CharUpper((LPTSTR) (unsigned char) 
			    label[underline]))) {
			*plResult = (2 << 16) | i;
			returnResult = 1;
			break;
		    }
		}
	    }
	    break;
	}

	case WM_MEASUREITEM: {
	    LPMEASUREITEMSTRUCT itemPtr = (LPMEASUREITEMSTRUCT) *plParam;
    
	    if (itemPtr != NULL) {
		mePtr = (TkMenuEntry *) itemPtr->itemData;
		menuPtr = mePtr->menuPtr;

		TkRecomputeMenu(menuPtr);
		itemPtr->itemHeight = mePtr->height;
		itemPtr->itemWidth = mePtr->width;
		if (mePtr->hideMargin) {
		    itemPtr->itemWidth += 2 - indicatorDimensions[1];
		} else {
		    int activeBorderWidth;
		    
		    Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
			    menuPtr->activeBorderWidthPtr, 
			    &activeBorderWidth);
		    itemPtr->itemWidth += 2 * activeBorderWidth;
		}
		*plResult = 1;
		returnResult = 1;
	    }
	    break;
	}
	
	case WM_DRAWITEM: {
	    TkWinDrawable *twdPtr;
	    LPDRAWITEMSTRUCT itemPtr = (LPDRAWITEMSTRUCT) *plParam;
	    Tk_FontMetrics fontMetrics;

	    if (itemPtr != NULL) {
		Tk_Font tkfont;

		mePtr = (TkMenuEntry *) itemPtr->itemData;
		menuPtr = mePtr->menuPtr;
		twdPtr = (TkWinDrawable *) ckalloc(sizeof(TkWinDrawable));
		twdPtr->type = TWD_WINDC;
		twdPtr->winDC.hdc = itemPtr->hDC;

		if (mePtr->state != ENTRY_DISABLED) {
		    if (itemPtr->itemState & ODS_SELECTED) {
			TkActivateMenuEntry(menuPtr, mePtr->index);
		    } else {
			TkActivateMenuEntry(menuPtr, -1);
		    }
		}

		tkfont = Tk_GetFontFromObj(menuPtr->tkwin, menuPtr->fontPtr);
		Tk_GetFontMetrics(tkfont, &fontMetrics);
		TkpDrawMenuEntry(mePtr, (Drawable) twdPtr, tkfont,
			&fontMetrics, itemPtr->rcItem.left,
			itemPtr->rcItem.top, itemPtr->rcItem.right
			- itemPtr->rcItem.left, itemPtr->rcItem.bottom
			- itemPtr->rcItem.top, 0, 0);

		ckfree((char *) twdPtr);
		*plResult = 1;
		returnResult = 1;
	    }
	    break;
	}

	case WM_MENUSELECT: {
	    UINT flags = HIWORD(*pwParam);

	    TkMenuInit();

	    if ((flags == 0xFFFF) && (*plParam == 0)) {
		Tcl_SetServiceMode(tsdPtr->oldServiceMode);
		if (tsdPtr->modalMenuPtr != NULL) {
		    RecursivelyClearActiveMenu(tsdPtr->modalMenuPtr);
		}
	    } else {
		menuPtr = NULL;
 		if (*plParam != 0) {
		    hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->winMenuTable,
			    (char *) *plParam);
		    if (hashEntryPtr != NULL) {
			menuPtr = (TkMenu *) Tcl_GetHashValue(hashEntryPtr);
		    }
		}

		if (menuPtr != NULL) {
	    	    mePtr = NULL;
		    if (flags != 0xFFFF) {
			if (flags & MF_POPUP) {
			    mePtr = menuPtr->entries[LOWORD(*pwParam)];
			} else {
			    hashEntryPtr = Tcl_FindHashEntry(
                                    &tsdPtr->commandTable,
				    (char *) LOWORD(*pwParam));
			    if (hashEntryPtr != NULL) {
				mePtr = (TkMenuEntry *) 
					Tcl_GetHashValue(hashEntryPtr);
			    }
			}
		    }	 

		    if ((mePtr == NULL) || (mePtr->state == ENTRY_DISABLED)) {
			TkActivateMenuEntry(menuPtr, -1);
		    } else {
			TkActivateMenuEntry(menuPtr, mePtr->index);
		    }
		    MenuSelectEvent(menuPtr);
		    Tcl_ServiceAll();
		}
	    }
	}
    }
    return returnResult;
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
TkpSetWindowMenuBar(tkwin, menuPtr)
    Tk_Window tkwin;	    /* The window we are putting the menubar into.*/
    TkMenu *menuPtr;	    /* The menu we are inserting */
{
    HMENU winMenuHdl;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    if (menuPtr != NULL) {
	Tcl_HashEntry *hashEntryPtr;
	int newEntry;

	winMenuHdl = (HMENU) menuPtr->platformData;
	hashEntryPtr = Tcl_FindHashEntry(&tsdPtr->winMenuTable, 
                (char *) winMenuHdl);
	Tcl_DeleteHashEntry(hashEntryPtr);
	DestroyMenu(winMenuHdl);
	winMenuHdl = CreateMenu();
	hashEntryPtr = Tcl_CreateHashEntry(&tsdPtr->winMenuTable, 
                (char *) winMenuHdl, &newEntry);
	Tcl_SetHashValue(hashEntryPtr, (char *) menuPtr);
	menuPtr->platformData = (TkMenuPlatformData) winMenuHdl;
	TkWinSetMenu(tkwin, winMenuHdl);
	if (menuPtr->menuFlags & MENU_RECONFIGURE_PENDING) {
	    Tcl_DoWhenIdle(ReconfigureWindowsMenu, (ClientData) menuPtr);
	    menuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
	}
    } else {
	TkWinSetMenu(tkwin, NULL);
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
    /*
     * Nothing to do.
     */
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

void
GetMenuIndicatorGeometry (
    TkMenu *menuPtr,			/* The menu we are measuring */
    TkMenuEntry *mePtr,			/* The entry we are measuring */
    Tk_Font tkfont,			/* Precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* Precalculated font metrics */
    int *widthPtr,			/* The resulting width */
    int *heightPtr)			/* The resulting height */
{
    *heightPtr = indicatorDimensions[0];
    if (mePtr->hideMargin) {
	*widthPtr = 0;
    } else {
	int borderWidth;

	Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
		menuPtr->borderWidthPtr, &borderWidth);
	*widthPtr = indicatorDimensions[1] - borderWidth;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetMenuAccelGeometry --
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

void
GetMenuAccelGeometry (
    TkMenu *menuPtr,			/* The menu we are measuring */
    TkMenuEntry *mePtr,			/* The entry we are measuring */
    Tk_Font tkfont,			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* The precalculated font metrics */
    int *widthPtr,			/* The resulting width */
    int *heightPtr)			/* The resulting height */
{
    *heightPtr = fmPtr->linespace;
    if (mePtr->type == CASCADE_ENTRY) {
	*widthPtr = 0;
    } else if (mePtr->accelPtr == NULL) {
	*widthPtr = 0;
    } else {
	char *accel = Tcl_GetStringFromObj(mePtr->accelPtr, NULL);
	*widthPtr = Tk_TextWidth(tkfont, accel, mePtr->accelLength);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * GetTearoffEntryGeometry --
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

void
GetTearoffEntryGeometry (
    TkMenu *menuPtr,			/* The menu we are measuring */
    TkMenuEntry *mePtr,			/* The entry we are measuring */
    Tk_Font tkfont,			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* The precalculated font metrics */
    int *widthPtr,			/* The resulting width */
    int *heightPtr)			/* The resulting height */
{
    if (menuPtr->menuType != MASTER_MENU) {
	*heightPtr = 0;
    } else {
	*heightPtr = fmPtr->linespace;
    }
    *widthPtr = 0;
}

/*
 *----------------------------------------------------------------------
 *
 * GetMenuSeparatorGeometry --
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

void
GetMenuSeparatorGeometry (
    TkMenu *menuPtr,			/* The menu we are measuring */
    TkMenuEntry *mePtr,			/* The entry we are measuring */
    Tk_Font tkfont,			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* The precalcualted font metrics */
    int *widthPtr,			/* The resulting width */
    int *heightPtr)			/* The resulting height */
{
    *widthPtr = 0;
    *heightPtr = fmPtr->linespace;
}

/*
 *----------------------------------------------------------------------
 *
 * DrawWindowsSystemBitmap --
 *
 *	Draws the windows system bitmap given by bitmapID into the rect
 *	given by rectPtr in the drawable. The bitmap is centered in the
 *	rectangle. It is not clipped, so if the bitmap is bigger than
 *	the rect it will bleed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Drawing occurs. Some storage is allocated and released.
 *
 *----------------------------------------------------------------------
 */

static void
DrawWindowsSystemBitmap(display, drawable, gc, rectPtr, bitmapID, alignFlags)
    Display *display;			/* The display we are drawing into */
    Drawable drawable;			/* The drawable we are working with */
    GC gc;				/* The GC to draw with */
    CONST RECT *rectPtr;		/* The rectangle to draw into */
    int bitmapID;			/* The windows id of the system
					 * bitmap to draw. */
    int alignFlags;			/* How to align the bitmap inside the
					 * rectangle. */
{
    TkWinDCState state;
    HDC hdc = TkWinGetDrawableDC(display, drawable, &state);
    HDC scratchDC;
    HBITMAP bitmap;
    BITMAP bm;
    POINT ptSize;
    POINT ptOrg;
    int topOffset, leftOffset;
    
    SetBkColor(hdc, gc->background);
    SetTextColor(hdc, gc->foreground);

    scratchDC = CreateCompatibleDC(hdc);
    bitmap = LoadBitmap(NULL, MAKEINTRESOURCE(bitmapID));

    SelectObject(scratchDC, bitmap);
    SetMapMode(scratchDC, GetMapMode(hdc));
    GetObject(bitmap, sizeof(BITMAP), &bm);
    ptSize.x = bm.bmWidth;
    ptSize.y = bm.bmHeight;
    DPtoLP(hdc, &ptSize, 1);

    ptOrg.y = ptOrg.x = 0;
    DPtoLP(hdc, &ptOrg, 1);

    if (alignFlags & ALIGN_BITMAP_TOP) {
	topOffset = 0;
    } else if (alignFlags & ALIGN_BITMAP_BOTTOM) {
	topOffset = (rectPtr->bottom - rectPtr->top) - ptSize.y;
    } else {
	topOffset = (rectPtr->bottom - rectPtr->top) / 2 - (ptSize.y / 2);
    }

    if (alignFlags & ALIGN_BITMAP_LEFT) {
	leftOffset = 0;
    } else if (alignFlags & ALIGN_BITMAP_RIGHT) {
	leftOffset = (rectPtr->right - rectPtr->left) - ptSize.x;
    } else {
	leftOffset = (rectPtr->right - rectPtr->left) / 2 - (ptSize.x / 2);
    }
    
    BitBlt(hdc, rectPtr->left + leftOffset, rectPtr->top + topOffset, ptSize.x,
	    ptSize.y, scratchDC, ptOrg.x, ptOrg.y, SRCCOPY);
    DeleteDC(scratchDC);
    DeleteObject(bitmap);

    TkWinReleaseDrawableDC(drawable, hdc, &state);
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
void
DrawMenuEntryIndicator(menuPtr, mePtr, d, gc, indicatorGC, tkfont, fmPtr, x,
	y, width, height)
    TkMenu *menuPtr;		    /* The menu we are drawing */
    TkMenuEntry *mePtr;		    /* The entry we are drawing */
    Drawable d;			    /* What we are drawing into */
    GC gc;			    /* The gc we are drawing with */
    GC indicatorGC;		    /* The gc for indicator objects */
    Tk_Font tkfont;		    /* The precalculated font */
    CONST Tk_FontMetrics *fmPtr;    /* The precalculated font metrics */
    int x;			    /* Left edge */
    int y;			    /* Top edge */
    int width;
    int height;
{
    if ((mePtr->type == CHECK_BUTTON_ENTRY) 
	    || (mePtr->type == RADIO_BUTTON_ENTRY)) {
    	if (mePtr->indicatorOn && (mePtr->entryFlags & ENTRY_SELECTED)) {
	    RECT rect;
	    GC whichGC;
	    int borderWidth, activeBorderWidth;
	    if (mePtr->state != ENTRY_NORMAL) {
		whichGC = gc;
	    } else {
		whichGC = indicatorGC;
	    }

	    rect.top = y;
	    rect.bottom = y + mePtr->height;
	    Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
		    menuPtr->borderWidthPtr, &borderWidth);
	    Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
		    menuPtr->activeBorderWidthPtr, &activeBorderWidth);
	    rect.left = borderWidth + activeBorderWidth + x;
	    rect.right = mePtr->indicatorSpace + x;

	    if ((mePtr->state == ENTRY_DISABLED)
		    && (menuPtr->disabledFgPtr != NULL)
		    && (versionInfo.dwMajorVersion >= 4)) {
		RECT hilightRect;
		COLORREF oldFgColor = whichGC->foreground;
	    
		whichGC->foreground = GetSysColor(COLOR_3DHILIGHT);
		hilightRect.top = rect.top + 1;
		hilightRect.bottom = rect.bottom + 1;
		hilightRect.left = rect.left + 1;
		hilightRect.right = rect.right + 1;
		DrawWindowsSystemBitmap(menuPtr->display, d, whichGC, 
			&hilightRect, OBM_CHECK, 0);
		whichGC->foreground = oldFgColor;
	    }

	    DrawWindowsSystemBitmap(menuPtr->display, d, whichGC, &rect, 
		    OBM_CHECK, 0);

	    if ((mePtr->state == ENTRY_DISABLED) 
		    && (menuPtr->disabledImageGC != None)
		    && (versionInfo.dwMajorVersion < 4)) {
		XFillRectangle(menuPtr->display, d, menuPtr->disabledImageGC,
			rect.left, rect.top, rect.right, rect.bottom);
	    }
	}
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

void
DrawMenuEntryAccelerator(menuPtr, mePtr, d, gc, tkfont, fmPtr,
	activeBorder, x, y, width, height, drawArrow)
    TkMenu *menuPtr;			/* The menu we are drawing */
    TkMenuEntry *mePtr;			/* The entry we are drawing */
    Drawable d;				/* What we are drawing into */
    GC gc;				/* The gc we are drawing with */
    Tk_Font tkfont;			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr;	/* The precalculated font metrics */
    Tk_3DBorder activeBorder;		/* The border when an item is active */
    int x;				/* left edge */
    int y;				/* top edge */
    int width;				/* Width of menu entry */
    int height;				/* Height of menu entry */
    int drawArrow;			/* For cascade menus, whether of not
					 * to draw the arraw. I cannot figure
					 * out Windows' algorithm for where
					 * to draw this. */
{
    int baseline;
    int leftEdge = x + mePtr->indicatorSpace + mePtr->labelWidth;
    char *accel;
    
    if (mePtr->accelPtr != NULL) {
	accel = Tcl_GetStringFromObj(mePtr->accelPtr, NULL);
    }

    baseline = y + (height + fmPtr->ascent - fmPtr->descent) / 2;

    if ((mePtr->state == ENTRY_DISABLED) && (menuPtr->disabledFgPtr != NULL)
	    && ((mePtr->accelPtr != NULL)
	    || ((mePtr->type == CASCADE_ENTRY) && drawArrow))) {
	if (versionInfo.dwMajorVersion >= 4) {
	    COLORREF oldFgColor = gc->foreground;
	    
	    gc->foreground = GetSysColor(COLOR_3DHILIGHT);
	    if (mePtr->accelPtr != NULL) {
		Tk_DrawChars(menuPtr->display, d, gc, tkfont, accel,
			mePtr->accelLength, leftEdge + 1, baseline + 1);
	    }

	    if (mePtr->type == CASCADE_ENTRY) {
		RECT rect;

		rect.top = y + GetSystemMetrics(SM_CYBORDER) + 1;
		rect.bottom = y + height - GetSystemMetrics(SM_CYBORDER) + 1;
		rect.left = x + mePtr->indicatorSpace + mePtr->labelWidth + 1;
		rect.right = x + width;
		DrawWindowsSystemBitmap(menuPtr->display, d, gc, &rect, 
			OBM_MNARROW, ALIGN_BITMAP_RIGHT);
	    }
	    gc->foreground = oldFgColor;
	}
    }

    if (mePtr->accelPtr != NULL) {
	Tk_DrawChars(menuPtr->display, d, gc, tkfont, accel, 
		mePtr->accelLength, leftEdge, baseline);
    }

    if ((mePtr->state == ENTRY_DISABLED) 
	    && (menuPtr->disabledImageGC != None)
	    && (versionInfo.dwMajorVersion < 4)) {
	XFillRectangle(menuPtr->display, d, menuPtr->disabledImageGC,
		leftEdge, y, width - mePtr->labelWidth 
		- mePtr->indicatorSpace, height);
    }

    if ((mePtr->type == CASCADE_ENTRY) && drawArrow) {
	RECT rect;

	rect.top = y + GetSystemMetrics(SM_CYBORDER);
	rect.bottom = y + height - GetSystemMetrics(SM_CYBORDER);
	rect.left = x + mePtr->indicatorSpace + mePtr->labelWidth;
	rect.right = x + width - 1;
	DrawWindowsSystemBitmap(menuPtr->display, d, gc, &rect, OBM_MNARROW, 
		ALIGN_BITMAP_RIGHT);
	if ((mePtr->state == ENTRY_DISABLED) 
		&& (menuPtr->disabledImageGC != None)
		&& (versionInfo.dwMajorVersion < 4)) {
	    XFillRectangle(menuPtr->display, d, menuPtr->disabledImageGC,
		    rect.left, rect.top, rect.right, rect.bottom);
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
void
DrawMenuSeparator(menuPtr, mePtr, d, gc, tkfont, fmPtr, x, y, width, height)
    TkMenu *menuPtr;			/* The menu we are drawing */
    TkMenuEntry *mePtr;			/* The entry we are drawing */
    Drawable d;				/* What we are drawing into */
    GC gc;				/* The gc we are drawing with */
    Tk_Font tkfont;			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr;	/* The precalculated font metrics */
    int x;				/* left edge */
    int y;				/* top edge */
    int width;				/* width of item */
    int height;				/* height of item */
{
    XPoint points[2];
    Tk_3DBorder border;

    points[0].x = x;
    points[0].y = y + height / 2;
    points[1].x = x + width - 1;
    points[1].y = points[0].y;
    border = Tk_Get3DBorderFromObj(menuPtr->tkwin, menuPtr->borderPtr);
    Tk_Draw3DPolygon(menuPtr->tkwin, d, border, points, 2, 1, 
	    TK_RELIEF_RAISED);
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuUnderline --
 *
 *	On appropriate platforms, draw the underline character for the
 *	menu.
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
DrawMenuUnderline(
    TkMenu *menuPtr,			/* The menu to draw into */
    TkMenuEntry *mePtr,			/* The entry we are drawing */
    Drawable d,				/* What we are drawing into */
    GC gc,				/* The gc to draw into */
    Tk_Font tkfont,			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr,	/* The precalculated font metrics */
    int x,				/* Left Edge */
    int y,				/* Top Edge */
    int width,				/* Width of entry */
    int height)				/* Height of entry */
{
    if (mePtr->underline >= 0) {
	char *label = Tcl_GetStringFromObj(mePtr->labelPtr, NULL);
	char *start = Tcl_UtfAtIndex(label, mePtr->underline);
	char *end = Tcl_UtfNext(start);

    	Tk_UnderlineChars(menuPtr->display, d,
    		gc, tkfont, label, x + mePtr->indicatorSpace,
    		y + (height + fmPtr->ascent - fmPtr->descent) / 2, 
		start - label, end - label);
    }		
}

/*
 *--------------------------------------------------------------
 *
 * MenuKeyBindProc --
 *
 *	This procedure is invoked when keys related to pulling
 *	down menus is pressed. The corresponding Windows events
 *	are generated and passed to DefWindowProc if appropriate.
 *
 * Results:
 *	Always returns TCL_OK.
 *
 * Side effects:
 *	The menu system may take over and process user events
 *	for menu input.
 *
 *--------------------------------------------------------------
 */

static int
MenuKeyBindProc(clientData, interp, eventPtr, tkwin, keySym)
    ClientData clientData;	/* not used in this proc */
    Tcl_Interp *interp;		/* The interpreter of the receiving window. */
    XEvent *eventPtr;		/* The XEvent to process */
    Tk_Window tkwin;		/* The window receiving the event */
    KeySym keySym;		/* The key sym that is produced. */
{
    UINT scanCode;
    UINT virtualKey;
    TkWindow *winPtr = (TkWindow *)tkwin;
    int i;

    if (eventPtr->type == KeyPress) {
	switch (keySym) {
	case XK_Alt_L:
	    scanCode = MapVirtualKey(VK_LMENU, 0);
	    CallWindowProc(DefWindowProc, Tk_GetHWND(Tk_WindowId(tkwin)),
		    WM_SYSKEYDOWN, VK_MENU, (scanCode << 16)
		    | (1 << 29));
	    break;
	case XK_Alt_R:
	    scanCode = MapVirtualKey(VK_RMENU, 0);
	    CallWindowProc(DefWindowProc, Tk_GetHWND(Tk_WindowId(tkwin)),
		    WM_SYSKEYDOWN, VK_MENU, (scanCode << 16)
		    | (1 << 29) | (1 << 24));
	    break;
	case XK_F10:
	    scanCode = MapVirtualKey(VK_F10, 0);
	    CallWindowProc(DefWindowProc, Tk_GetHWND(Tk_WindowId(tkwin)),
		    WM_SYSKEYDOWN, VK_F10, (scanCode << 16));
	    break;
	default:
	    virtualKey = XKeysymToKeycode(winPtr->display, keySym);
	    scanCode = MapVirtualKey(virtualKey, 0);
	    if (0 != scanCode) {
		CallWindowProc(DefWindowProc, Tk_GetHWND(Tk_WindowId(tkwin)),
			WM_SYSKEYDOWN, virtualKey, ((scanCode << 16)
			| (1 << 29)));
		if (eventPtr->xkey.nbytes > 0) {
		    for (i = 0; i < eventPtr->xkey.nbytes; i++) {
			CallWindowProc(DefWindowProc,
				Tk_GetHWND(Tk_WindowId(tkwin)),
				WM_SYSCHAR,
				eventPtr->xkey.trans_chars[i],
				((scanCode << 16) | (1 << 29)));
		    }
		}
	    }
	}
    } else if (eventPtr->type == KeyRelease) {
	switch (keySym) {
	case XK_Alt_L:
	    scanCode = MapVirtualKey(VK_LMENU, 0);
	    CallWindowProc(DefWindowProc, Tk_GetHWND(Tk_WindowId(tkwin)),
		    WM_SYSKEYUP, VK_MENU, (scanCode << 16)
		    | (1 << 29) | (1 << 30) | (1 << 31));
	    break;
	case XK_Alt_R:
	    scanCode = MapVirtualKey(VK_RMENU, 0);
	    CallWindowProc(DefWindowProc, Tk_GetHWND(Tk_WindowId(tkwin)),
		    WM_SYSKEYUP, VK_MENU, (scanCode << 16) | (1 << 24)
		    | (0x111 << 29) | (1 << 30) | (1 << 31));
	    break;
	case XK_F10:
	    scanCode = MapVirtualKey(VK_F10, 0);
	    CallWindowProc(DefWindowProc, Tk_GetHWND(Tk_WindowId(tkwin)),
		    WM_SYSKEYUP, VK_F10, (scanCode << 16)
		    | (1 << 30) | (1 << 31));
	    break;
	default:
	    virtualKey = XKeysymToKeycode(winPtr->display, keySym);
	    scanCode = MapVirtualKey(virtualKey, 0);
	    if (0 != scanCode) {
		CallWindowProc(DefWindowProc, Tk_GetHWND(Tk_WindowId(tkwin)),
			WM_SYSKEYUP, virtualKey, ((scanCode << 16)
			| (1 << 29) | (1 << 30) | (1 << 31)));
	    }
	}
    }
    return TCL_OK;
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
    Tk_Uid uid = Tk_GetUid("all");

    /*
     * We need to set up the bindings for menubars. These have to
     * recreate windows events, so we need to have a C-level
     * binding for this. We have to generate the WM_SYSKEYDOWNS
     * and WM_SYSKEYUPs appropriately.
     */
    
    TkCreateBindingProcedure(interp, bindingTable, (ClientData)uid, 
	    "<Alt_L>", MenuKeyBindProc, NULL, NULL);
    TkCreateBindingProcedure(interp, bindingTable, (ClientData)uid,
	    "<KeyRelease-Alt_L>", MenuKeyBindProc, NULL, NULL);
    TkCreateBindingProcedure(interp, bindingTable, (ClientData)uid, 
	    "<Alt_R>", MenuKeyBindProc, NULL, NULL);
    TkCreateBindingProcedure(interp, bindingTable, (ClientData)uid,
	    "<KeyRelease-Alt_R>", MenuKeyBindProc, NULL, NULL);
    TkCreateBindingProcedure(interp, bindingTable, (ClientData)uid,
	    "<Alt-KeyPress>", MenuKeyBindProc, NULL, NULL);
    TkCreateBindingProcedure(interp, bindingTable, (ClientData)uid,
	    "<Alt-KeyRelease>", MenuKeyBindProc, NULL, NULL);
    TkCreateBindingProcedure(interp, bindingTable, (ClientData)uid,
	    "<KeyPress-F10>", MenuKeyBindProc, NULL, NULL);
    TkCreateBindingProcedure(interp, bindingTable, (ClientData)uid,
	    "<KeyRelease-F10>", MenuKeyBindProc, NULL, NULL);
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
    int activeBorderWidth;
    int leftEdge;
    int imageHeight, imageWidth;

    Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
	    menuPtr->activeBorderWidthPtr, &activeBorderWidth);
    leftEdge = x + indicatorSpace + activeBorderWidth;

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
        Tk_SizeOfBitmap(menuPtr->display, bitmap, &width, &height);
    	XCopyPlane(menuPtr->display, bitmap, d,	gc, 0, 0, (unsigned) width, 
		(unsigned) height, leftEdge,
		(int) (y + (mePtr->height - height)/2), 1);
    } else {
    	if (mePtr->labelLength > 0) {
	    char *label = Tcl_GetStringFromObj(mePtr->labelPtr, NULL);
	    Tk_DrawChars(menuPtr->display, d, gc, tkfont, label, 
		    mePtr->labelLength, leftEdge, baseline);
	    DrawMenuUnderline(menuPtr, mePtr, d, gc, tkfont, fmPtr, x, y,
		    width, height);
    	}
    }

    if (mePtr->state == ENTRY_DISABLED) {
	if (menuPtr->disabledFgPtr == NULL) {
	    XFillRectangle(menuPtr->display, d, menuPtr->disabledGC, x, y,
		    (unsigned) width, (unsigned) height);
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
DrawTearoffEntry(menuPtr, mePtr, d, gc, tkfont, fmPtr, x, y, width, height)
    TkMenu *menuPtr;			/* The menu we are drawing */
    TkMenuEntry *mePtr;			/* The entry we are drawing */
    Drawable d;				/* The drawable we are drawing into */
    GC gc;				/* The gc we are drawing with */
    Tk_Font tkfont;			/* The font we are drawing with */
    CONST Tk_FontMetrics *fmPtr;	/* The metrics we are drawing with */
    int x;
    int y;
    int width;
    int height;
{
    XPoint points[2];
    int segmentWidth, maxX;
    Tk_3DBorder border;

    if (menuPtr->menuType != MASTER_MENU) {
	return;
    }
    
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
TkpConfigureMenuEntry(mePtr)
    register TkMenuEntry *mePtr;	/* Information about menu entry;  may
					 * or may not already have values for
					 * some fields. */
{
    TkMenu *menuPtr = mePtr->menuPtr;

    if (!(menuPtr->menuFlags & MENU_RECONFIGURE_PENDING)) {
	menuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
	Tcl_DoWhenIdle(ReconfigureWindowsMenu, (ClientData) menuPtr);
    }
    return TCL_OK;
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
TkpDrawMenuEntry(mePtr, d, tkfont, menuMetricsPtr, x, y, width, height, 
	strictMotif, drawArrow)
    TkMenuEntry *mePtr;		    /* The entry to draw */
    Drawable d;			    /* What to draw into */
    Tk_Font tkfont;		    /* Precalculated font for menu */
    CONST Tk_FontMetrics *menuMetricsPtr;
				    /* Precalculated metrics for menu */
    int x;			    /* X-coordinate of topleft of entry */
    int y;			    /* Y-coordinate of topleft of entry */
    int width;			    /* Width of the entry rectangle */
    int height;			    /* Height of the current rectangle */
    int strictMotif;		    /* Boolean flag */
    int drawArrow;		    /* Whether or not to draw the cascade
				     * arrow for cascade items. Only applies
				     * to Windows. */
{
    GC gc, indicatorGC;
    TkMenu *menuPtr = mePtr->menuPtr;
    Tk_3DBorder bgBorder, activeBorder;
    CONST Tk_FontMetrics *fmPtr;
    Tk_FontMetrics entryMetrics;
    int padY = (menuPtr->menuType == MENUBAR) ? 3 : 0;
    int adjustedY = y + padY;
    int adjustedHeight = height - 2 * padY;

    /*
     * Choose the gc for drawing the foreground part of the entry.
     */

    if ((mePtr->state == ENTRY_ACTIVE) && !strictMotif) {
	gc = mePtr->activeGC;
	if (gc == NULL) {
	    gc = menuPtr->activeGC;
	}
    } else {
    	TkMenuEntry *cascadeEntryPtr;
    	int parentDisabled = 0;
	char *name;
    	
    	for (cascadeEntryPtr = menuPtr->menuRefPtr->parentEntryPtr;
    		cascadeEntryPtr != NULL;
    		cascadeEntryPtr = cascadeEntryPtr->nextCascadePtr) {
	    name = Tcl_GetStringFromObj(cascadeEntryPtr->namePtr, NULL);
    	    if (strcmp(name, Tk_PathName(menuPtr->tkwin)) == 0) {
    	    	if (mePtr->state == ENTRY_DISABLED) {
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
	    (mePtr->borderPtr == NULL) ? menuPtr->borderPtr
	    : mePtr->borderPtr);
    if (strictMotif) {
	activeBorder = bgBorder;
    } else {
	activeBorder = Tk_Get3DBorderFromObj(menuPtr->tkwin,
	    (mePtr->activeBorderPtr == NULL) ? menuPtr->activeBorderPtr
	    : mePtr->activeBorderPtr);
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
	DrawMenuEntryLabel(menuPtr, mePtr, d, gc, tkfont, fmPtr, x, adjustedY,
		width, adjustedHeight);
	DrawMenuEntryAccelerator(menuPtr, mePtr, d, gc, tkfont, fmPtr,
		activeBorder, x, adjustedY, width, adjustedHeight, drawArrow);
	if (!mePtr->hideMargin) {
	    DrawMenuEntryIndicator(menuPtr, mePtr, d, gc, indicatorGC, tkfont,
		    fmPtr, x, adjustedY, width, adjustedHeight);
	}
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
GetMenuLabelGeometry(mePtr, tkfont, fmPtr, widthPtr, heightPtr)
    TkMenuEntry *mePtr;			/* The entry we are computing */
    Tk_Font tkfont;			/* The precalculated font */
    CONST Tk_FontMetrics *fmPtr;	/* The precalculated metrics */
    int *widthPtr;			/* The resulting width of the label
					 * portion */
    int *heightPtr;			/* The resulting height of the label
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
	    char *label = Tcl_GetStringFromObj(mePtr->labelPtr, NULL);

    	    *widthPtr = Tk_TextWidth(tkfont, label, mePtr->labelLength);
    	} else {
    	    *widthPtr = 0;
    	}
    }
    *heightPtr += 1;
}

/*
 *----------------------------------------------------------------------
 *
 * DrawMenuEntryBackground --
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
    if (mePtr->state == ENTRY_ACTIVE) {
	bgBorder = activeBorder;
    }
    Tk_Fill3DRectangle(menuPtr->tkwin, d, bgBorder,
    	    x, y, width, height, 0, TK_RELIEF_FLAT);
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
    Tk_Font menuFont, tkfont;
    Tk_FontMetrics menuMetrics, entryMetrics, *fmPtr;
    int x, y, height, width, indicatorSpace, labelWidth, accelWidth;
    int windowWidth, windowHeight, accelSpace;
    int i, j, lastColumnBreak = 0;
    int activeBorderWidth, borderWidth;
    
    if (menuPtr->tkwin == NULL) {
	return;
    }

    Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin, 
	    menuPtr->borderWidthPtr, &borderWidth);
    x = y = borderWidth;
    indicatorSpace = labelWidth = accelWidth = 0;
    windowHeight = 0;

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
    accelSpace = Tk_TextWidth(menuFont, "M", 1);
    Tk_GetPixelsFromObj(menuPtr->interp, menuPtr->tkwin,
	    menuPtr->activeBorderWidthPtr, &activeBorderWidth);

    for (i = 0; i < menuPtr->numEntries; i++) {
	if (menuPtr->entries[i]->fontPtr == NULL) {
	    tkfont = menuFont;
	    fmPtr = &menuMetrics;
	} else {
	    tkfont = Tk_GetFontFromObj(menuPtr->tkwin,
		    menuPtr->entries[i]->fontPtr);
    	    Tk_GetFontMetrics(tkfont, &entryMetrics);
    	    fmPtr = &entryMetrics;
    	}
	if ((i > 0) && menuPtr->entries[i]->columnBreak) {
	    if (accelWidth != 0) {
		labelWidth += accelSpace;
	    }
	    for (j = lastColumnBreak; j < i; j++) {
		menuPtr->entries[j]->indicatorSpace = indicatorSpace;
		menuPtr->entries[j]->labelWidth = labelWidth;
		menuPtr->entries[j]->width = indicatorSpace + labelWidth
			+ accelWidth + 2 * activeBorderWidth;
		menuPtr->entries[j]->x = x;
		menuPtr->entries[j]->entryFlags &= ~ENTRY_LAST_COLUMN;
	    }
	    x += indicatorSpace + labelWidth + accelWidth
		    + 2 * borderWidth;
	    indicatorSpace = labelWidth = accelWidth = 0;
	    lastColumnBreak = i;
	    y = borderWidth;
	}

	if (menuPtr->entries[i]->type == SEPARATOR_ENTRY) {
	    GetMenuSeparatorGeometry(menuPtr, menuPtr->entries[i], tkfont,
	    	    fmPtr, &width, &height);
	    menuPtr->entries[i]->height = height;
	} else if (menuPtr->entries[i]->type == TEAROFF_ENTRY) {
	    GetTearoffEntryGeometry(menuPtr, menuPtr->entries[i], tkfont, 
	    	    fmPtr, &width, &height);
	    menuPtr->entries[i]->height = height;
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
	    
	    GetMenuLabelGeometry(menuPtr->entries[i], tkfont, fmPtr, &width,
	    	    &height);
	    menuPtr->entries[i]->height = height;
	    if (width > labelWidth) {
	    	labelWidth = width;
	    }
	
	    GetMenuAccelGeometry(menuPtr, menuPtr->entries[i], tkfont,
		    fmPtr, &width, &height);
	    if (height > menuPtr->entries[i]->height) {
	    	menuPtr->entries[i]->height = height;
	    }
	    if (width > accelWidth) {
	    	accelWidth = width;
	    }

	    GetMenuIndicatorGeometry(menuPtr, menuPtr->entries[i], tkfont, 
	    	    fmPtr, &width, &height);
	    if (height > menuPtr->entries[i]->height) {
	    	menuPtr->entries[i]->height = height;
	    }
	    if (width > indicatorSpace) {
	    	indicatorSpace = width;
	    }

	    menuPtr->entries[i]->height += 2 * activeBorderWidth + 1;
    	}
        menuPtr->entries[i]->y = y;
	y += menuPtr->entries[i]->height;
	if (y > windowHeight) {
	    windowHeight = y;
	}
    }

    if (accelWidth != 0) {
	labelWidth += accelSpace;
    }
    for (j = lastColumnBreak; j < menuPtr->numEntries; j++) {
	menuPtr->entries[j]->indicatorSpace = indicatorSpace;
	menuPtr->entries[j]->labelWidth = labelWidth;
	menuPtr->entries[j]->width = indicatorSpace + labelWidth
		+ accelWidth + 2 * activeBorderWidth;
	menuPtr->entries[j]->x = x;
	menuPtr->entries[j]->entryFlags |= ENTRY_LAST_COLUMN;
    }
    windowWidth = x + indicatorSpace + labelWidth + accelWidth + accelSpace
	    + 2 * activeBorderWidth + 2 * borderWidth;


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
    POINTS rootPoint;
    DWORD msgPos;
   
    event.type = VirtualEvent;
    event.serial = menuPtr->display->request;
    event.send_event = 0;
    event.display = menuPtr->display;
    Tk_MakeWindowExist(menuPtr->tkwin);
    event.event = Tk_WindowId(menuPtr->tkwin);
    event.root = XRootWindow(menuPtr->display, 0);
    event.subwindow = None;
    event.time = TkpGetMS();
    
    msgPos = GetMessagePos();
    rootPoint = MAKEPOINTS(msgPos);
    event.x_root = rootPoint.x;
    event.y_root = rootPoint.y;
    event.state = TkWinGetModifierState();
    event.same_screen = 1;
    event.name = Tk_GetUid("MenuSelect");
    Tk_QueueWindowEvent((XEvent *) &event, TCL_QUEUE_TAIL);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuNotifyToplevelCreate --
 *
 *	This routine reconfigures the menu and the clones indicated by
 *	menuName becuase a toplevel has been created and any system
 *	menus need to be created.
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
    TkMenuReferences *menuRefPtr;
    TkMenu *menuPtr;

    if ((menuName != NULL) && (menuName[0] != '\0')) {
	menuRefPtr = TkFindMenuReferences(interp, menuName);
	if ((menuRefPtr != NULL) && (menuRefPtr->menuPtr != NULL)) {
	    for (menuPtr = menuRefPtr->menuPtr->masterMenuPtr; menuPtr != NULL;
		    menuPtr = menuPtr->nextInstancePtr) {
		if ((menuPtr->menuType == MENUBAR) 
			&& !(menuPtr->menuFlags & MENU_RECONFIGURE_PENDING)) {
		    menuPtr->menuFlags |= MENU_RECONFIGURE_PENDING;
		    Tcl_DoWhenIdle(ReconfigureWindowsMenu, 
			    (ClientData) menuPtr);
		}
	    }
	}
    }
}

/*
 *----------------------------------------------------------------------
 *
 * MenuExitHandler --
 *
 *	Throws away the utility window needed for menus and unregisters
 *	the class.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Menus have to be reinitialized next time.
 *
 *----------------------------------------------------------------------
 */

static void
MenuExitHandler(
    ClientData clientData)	    /* Not used */
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    DestroyWindow(tsdPtr->menuHWND);
    UnregisterClass(MENU_CLASS_NAME, Tk_GetHINSTANCE());
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinGetMenuSystemDefault --
 *
 *	Gets the Windows specific default value for a given X resource
 *	database name.
 *
 * Results:
 *	Returns a Tcl_Obj * with the default value. If there is no
 *	Windows-specific default for this attribute, returns NULL.
 *	This object has a ref count of 0.
 *
 * Side effects:
 *	Storage is allocated.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TkWinGetMenuSystemDefault(
    Tk_Window tkwin,		/* A window to use. */
    char *dbName,		/* The option database name. */
    char *className)		/* The name of the option class. */
{
    Tcl_Obj *valuePtr = NULL;

    if ((strcmp(dbName, "activeBorderWidth") == 0) ||
	    (strcmp(dbName, "borderWidth") == 0)) {
	valuePtr = Tcl_NewIntObj(defaultBorderWidth);
    } else if (strcmp(dbName, "font") == 0) {
	valuePtr = Tcl_NewStringObj(Tcl_DStringValue(&menuFontDString),
		-1);
    }

    return valuePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TkWinMenuSetDefaults --
 *
 *	Sets up the hash tables and the variables used by the menu package.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	lastMenuID gets initialized, and the parent hash and the command hash
 *	are allocated.
 *
 *----------------------------------------------------------------------
 */

void
SetDefaults(
    int firstTime)		    /* Is this the first time this
				     * has been called? */
{
    char sizeString[TCL_INTEGER_SPACE];
    char faceName[LF_FACESIZE];
    HDC scratchDC;
    Tcl_DString boldItalicDString;
    int bold = 0; 
    int italic = 0;
    TEXTMETRIC tm;
    int pointSize;
    HFONT menuFont;


    versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);

    /*
     * If GetVersionEx fails, it means that the version info record
     * is too big for what is compiled. Should never happen, but if
     * it does, we are later than Windows 95 or NT 4.0.
     */

    if (!GetVersionEx(&versionInfo)) {
	versionInfo.dwMajorVersion = 4;
    }

    /*
     * Set all of the default options. The loop will terminate when we run 
     * out of options via a break statement.
     */

    defaultBorderWidth = GetSystemMetrics(SM_CXBORDER);
    if (GetSystemMetrics(SM_CYBORDER) > defaultBorderWidth) {
	defaultBorderWidth = GetSystemMetrics(SM_CYBORDER);
    }


    scratchDC = CreateDC("DISPLAY", NULL, NULL, NULL);
    if (!firstTime) {
	Tcl_DStringFree(&menuFontDString);
    }
    Tcl_DStringInit(&menuFontDString);

    if (versionInfo.dwMajorVersion >= 4) {
	NONCLIENTMETRICS ncMetrics;

	ncMetrics.cbSize = sizeof(ncMetrics);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncMetrics),
		&ncMetrics, 0);
	menuFont = CreateFontIndirect(&ncMetrics.lfMenuFont);
    } else {
	menuFont = GetStockObject(SYSTEM_FONT);
    }
    SelectObject(scratchDC, menuFont);
    GetTextMetrics(scratchDC, &tm);
    GetTextFace(scratchDC, LF_FACESIZE, faceName);
    pointSize = MulDiv(tm.tmHeight - tm.tmInternalLeading,
	    72, GetDeviceCaps(scratchDC, LOGPIXELSY));
    if (tm.tmWeight >= 700) {
	bold = 1;
    }
    if (tm.tmItalic) {
	italic = 1;
    }

    SelectObject(scratchDC, GetStockObject(SYSTEM_FONT));
    DeleteDC(scratchDC);

    DeleteObject(menuFont);
    
    Tcl_DStringAppendElement(&menuFontDString, faceName);
    sprintf(sizeString, "%d", pointSize);
    Tcl_DStringAppendElement(&menuFontDString, sizeString);

    if (bold == 1 || italic == 1) {
	Tcl_DStringInit(&boldItalicDString);
	if (bold == 1) {
	    Tcl_DStringAppendElement(&boldItalicDString, "bold");
	}
	if (italic == 1) {
	    Tcl_DStringAppendElement(&boldItalicDString, "italic");
	}
	Tcl_DStringAppendElement(&menuFontDString, 
		Tcl_DStringValue(&boldItalicDString));
    }

    /*
     * Now we go ahead and get the dimensions of the check mark and the
     * appropriate margins. Since this is fairly hairy, we do it here
     * to save time when traversing large sets of menu items.
     *
     * The code below was given to me by Microsoft over the phone. It
     * is the only way to insure menu items lining up, and is not
     * documented.
     */

    if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {
	indicatorDimensions[0] = GetSystemMetrics(SM_CYMENUCHECK);
	indicatorDimensions[1] = ((GetSystemMetrics(SM_CXFIXEDFRAME) +
		GetSystemMetrics(SM_CXBORDER) 
		+ GetSystemMetrics(SM_CXMENUCHECK) + 7) & 0xFFF8)
		- GetSystemMetrics(SM_CXFIXEDFRAME);
    } else {
	DWORD dimensions = GetMenuCheckMarkDimensions();
	indicatorDimensions[0] = HIWORD(dimensions);
	indicatorDimensions[1] = LOWORD(dimensions);
   }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuInit --
 *
 *	Sets up the process-wide variables used by the menu package.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	lastMenuID gets initialized.
 *
 *----------------------------------------------------------------------
 */

void
TkpMenuInit()
{
    WNDCLASS wndClass;
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    wndClass.style = CS_OWNDC;
    wndClass.lpfnWndProc = TkWinMenuProc;
    wndClass.cbClsExtra = 0;
    wndClass.cbWndExtra = 0;
    wndClass.hInstance = Tk_GetHINSTANCE();
    wndClass.hIcon = NULL;
    wndClass.hCursor = NULL;
    wndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = MENU_CLASS_NAME;
    RegisterClass(&wndClass);

    tsdPtr->menuHWND = CreateWindow(MENU_CLASS_NAME, "MenuWindow", WS_POPUP,
	0, 0, 10, 10, NULL, NULL, Tk_GetHINSTANCE(), NULL);

    Tcl_CreateExitHandler(MenuExitHandler, (ClientData) NULL);
    SetDefaults(1);
}

/*
 *----------------------------------------------------------------------
 *
 * TkpMenuThreadInit --
 *
 *	Sets up the thread-local hash tables used by the menu module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Hash tables winMenuTable and commandTable are initialized.
 *
 *----------------------------------------------------------------------
 */

void
TkpMenuThreadInit()
{
    ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));

    Tcl_InitHashTable(&tsdPtr->winMenuTable, TCL_ONE_WORD_KEYS);
    Tcl_InitHashTable(&tsdPtr->commandTable, TCL_ONE_WORD_KEYS);
}
