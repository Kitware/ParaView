/*
 * tkMacInt.h --
 *
 *	Declarations of Macintosh specific shared variables and procedures.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TKMACINT
#define _TKMACINT

#include "tkInt.h"

#include "tkMac.h"

#include <AppleEvents.h>
#include <Windows.h>
#include <QDOffscreen.h>
#include <Menus.h>

#define TK_MAC_68K_STACK_GROWTH (256*1024)

struct TkWindowPrivate {
    TkWindow *winPtr;     	/* Ptr to tk window or NULL if Pixmap */
    GWorldPtr portPtr;     	/* Either WindowRef or off screen world */
    int xOff;	       		/* X offset from toplevel window */
    int yOff;		       	/* Y offset from toplevel window */
    RgnHandle clipRgn;		/* Visable region of window */
    RgnHandle aboveClipRgn;	/* Visable region of window & it's children */
    int referenceCount;		/* Don't delete toplevel until children are
				 * gone. */
    struct TkWindowPrivate *toplevel;	/* Pointer to the toplevel
					 * datastruct. */
    int flags;			/* Various state see defines below. */
};
typedef struct TkWindowPrivate MacDrawable;

/*
 * This list is used to keep track of toplevel windows that have a Mac
 * window attached. This is useful for several things, not the least
 * of which is maintaining floating windows.
 */

typedef struct TkMacWindowList {
    struct TkMacWindowList *nextPtr;	/* The next window in the list. */
    TkWindow *winPtr;			/* This window */
} TkMacWindowList;

/*
 * Defines use for the flags field of the MacDrawable data structure.
 */
 
#define TK_SCROLLBAR_GROW	1
#define TK_CLIP_INVALID		2
#define TK_HOST_EXISTS		4
#define TK_DRAWN_UNDER_MENU	8

/*
 * I am reserving TK_EMBEDDED = 0x100 in the MacDrawable flags
 * This is defined in tk.h. We need to duplicate the TK_EMBEDDED flag in the
 * TkWindow structure for the window,  but in the MacWin.  This way we can still tell
 * what the correct port is after the TKWindow  structure has been freed.  This 
 * actually happens when you bind destroy of a toplevel to Destroy of a child.
 */

/*
 * This structure is for handling Netscape-type in process
 * embedding where Tk does not control the top-level.  It contains
 * various functions that are needed by Mac specific routines, like
 * TkMacGetDrawablePort.  The definitions of the function types
 * are in tclMac.h.
 */

typedef struct {
	Tk_MacEmbedRegisterWinProc *registerWinProc;
	Tk_MacEmbedGetGrafPortProc *getPortProc;
	Tk_MacEmbedMakeContainerExistProc *containerExistProc;
	Tk_MacEmbedGetClipProc *getClipProc;
	Tk_MacEmbedGetOffsetInParentProc *getOffsetProc;
} TkMacEmbedHandler;

extern TkMacEmbedHandler *gMacEmbedHandler;

/*
 * Defines used for TkMacInvalidateWindow
 */
 
#define TK_WINDOW_ONLY 0
#define TK_PARENT_WINDOW 1

/*
 * Accessor for the privatePtr flags field for the TK_HOST_EXISTS field
 */
 
#define TkMacHostToplevelExists(tkwin) \
    (((TkWindow *) (tkwin))->privatePtr->toplevel->flags & TK_HOST_EXISTS)

/*
 * Defines use for the flags argument to TkGenWMConfigureEvent.
 */
 
#define TK_LOCATION_CHANGED	1
#define TK_SIZE_CHANGED		2
#define TK_BOTH_CHANGED		3

/*
 * Variables shared among various Mac Tk modules but are not
 * exported to the outside world.
 */
 
extern int tkMacAppInFront;

/*
 * Globals shared among Macintosh Tk
 */
 
extern MenuHandle tkAppleMenu;		/* Handle to the Apple Menu */
extern MenuHandle tkFileMenu;		/* Handles to menus */
extern MenuHandle tkEditMenu;		/* Handles to menus */
extern RgnHandle tkMenuCascadeRgn;	/* A region to clip with. */
extern int tkUseMenuCascadeRgn;		/* If this is 1, clipping code
					 * should intersect tkMenuCascadeRgn
					 * before drawing occurs.
					 * tkMenuCascadeRgn will only
					 * be valid when the value of this
					 * variable is 1. */
extern TkMacWindowList *tkMacWindowListPtr;
					/* The list of toplevels */

/*
 * The following types and defines are for MDEF support.
 */

#if STRUCTALIGNMENTSUPPORTED
#pragma options align=mac8k
#endif
typedef struct TkMenuLowMemGlobals {
    long menuDisable;			/* A combination of the menu and the item
    					 * that the mouse is currently over. */
    short menuTop;			/* Where in global coords the top of the
    					 * menu is. */
    short menuBottom;			/* Where in global coords the bottom of
    					 * the menu is. */
    Rect itemRect;			/* This is the rectangle of the currently
    					 * selected item. */
    short scrollFlag;			/* This is used by the MDEF and the
    					 * Menu Manager to control when scrolling
    					 * starts. With hierarchicals, an
    					 * mChooseMsg can come before an
    					 * mDrawMsg, and scrolling should not
    					 * occur until after the mDrawMsg.
    					 * The mDrawMsg sets this flag;
    					 * mChooseMsg checks the flag and
    					 * does not scroll if it is set;
    					 * and then resets the flag. */
} TkMenuLowMemGlobals;
#if STRUCTALIGNMENTSUPPORTED
#pragma options align=reset
#endif

typedef pascal void (*TkMenuDefProcPtr) (short message, MenuHandle theMenu,
	Rect *menuRectPtr, Point hitPt, short *whichItemPtr,
	TkMenuLowMemGlobals *globalsPtr);
enum {
    tkUppMenuDefProcInfo = kPascalStackBased
	    | STACK_ROUTINE_PARAMETER(1, SIZE_CODE(sizeof(short)))
	    | STACK_ROUTINE_PARAMETER(2, SIZE_CODE(sizeof(MenuRef)))
	    | STACK_ROUTINE_PARAMETER(3, SIZE_CODE(sizeof(Rect*)))
	    | STACK_ROUTINE_PARAMETER(4, SIZE_CODE(sizeof(Point)))
	    | STACK_ROUTINE_PARAMETER(5, SIZE_CODE(sizeof(short*)))
	    | STACK_ROUTINE_PARAMETER(6, SIZE_CODE(sizeof(TkMenuLowMemGlobals *)))
};

#if GENERATINGCFM
typedef UniversalProcPtr TkMenuDefUPP;
#else
typedef TkMenuDefProcPtr TkMenuDefUPP;
#endif

#if GENERATINGCFM
#define TkNewMenuDefProc(userRoutine)	\
	(TkMenuDefUPP) NewRoutineDescriptor((ProcPtr)(userRoutine), \
	tkUppMenuDefProcInfo, GetCurrentArchitecture())
#else
#define TkNewMenuDefProc(userRoutine) 	\
	((TkMenuDefUPP) (userRoutine))
#endif

#if GENERATINGCFM
#define TkCallMenuDefProc(userRoutine, message, theMenu, menuRectPtr, hitPt, \
	whichItemPtr, globalsPtr) \
	CallUniversalProc((UniversalProcPtr)(userRoutine), TkUppMenuDefProcInfo, \
	(message), (theMenu), (menuRectPtr), (hitPt), (whichItemPtr), \
	(globalsPtr))
#else
#define TkCallMenuDefProc(userRoutine, message, theMenu, menuRectPtr, hitPt, \
	whichItemPtr, globalsPtr) \
	(*(userRoutine))((message), (theMenu), (menuRectPtr), (hitPt), \
	(whichItemPtr), (globalsPtr))
#endif

#include "tkIntPlatDecls.h"

#endif /* _TKMACINT */
