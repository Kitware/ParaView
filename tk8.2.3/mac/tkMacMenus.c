/* 
 * tkMacMenus.c --
 *
 *	These calls set up and manage the menubar for the
 *	Macintosh version of Tk.
 *
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tcl.h"
#include "tclMacInt.h" /* Needed for FSpLocationFromPath */
#include "tk.h"
#include "tkInt.h"
#include "tkMacInt.h"

/*
 * The define Status defined by Xlib.h conflicts with the function Status
 * defined by Devices.h.  We undefine it here to compile.
 */
#undef Status
#include <Devices.h>
#include <Menus.h>
#include <Memory.h>
#include <SegLoad.h>
#include <StandardFile.h>
#include <ToolUtils.h>
#include <Balloons.h>

#define kAppleMenu		256
#define kAppleAboutItem		1
#define kFileMenu		2
#define kEditMenu		3

#define kSourceItem		1
#define kCloseItem		2
#define kQuitItem		4

#define EDIT_CUT		1
#define EDIT_COPY		2
#define EDIT_PASTE		3
#define EDIT_CLEAR		4

MenuHandle tkAppleMenu;
MenuHandle tkFileMenu;
MenuHandle tkEditMenu;

static Tcl_Interp *	gInterp;	/* Interpreter for this application. */

static void GenerateEditEvent _ANSI_ARGS_((int flag));
static void SourceDialog _ANSI_ARGS_((void));

/*
 *----------------------------------------------------------------------
 *
 * TkMacHandleMenuSelect --
 *
 *	Handles events that occur in the Menu bar.
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
TkMacHandleMenuSelect(
    long mResult,
    int optionKeyPressed)
{
    short theItem = LoWord(mResult);
    short theMenu = HiWord(mResult);
    Str255 name;
    Tk_Window tkwin;
    Window window;
    TkDisplay *dispPtr;

    if (mResult == 0) {
    	TkMacHandleTearoffMenu();
	TkMacClearMenubarActive();
	return;
    }

    switch (theMenu) {
	
	case kAppleMenu:
	    switch (theItem) {
		case kAppleAboutItem:
		    {
			Tcl_CmdInfo dummy;
			
			if (optionKeyPressed || gInterp == NULL ||
			    Tcl_GetCommandInfo(gInterp,
				    "tkAboutDialog", &dummy) == 0) {
			    TkAboutDlg();
			} else {
			    Tcl_Eval(gInterp, "tkAboutDialog");
			}
			break;
		    }
		default:
		    GetItem(tkAppleMenu, theItem, name);
		    HiliteMenu(0);
		    OpenDeskAcc(name);
		    return;
	    }
	    break;
	case kFileMenu:
	    switch (theItem) {
		case kSourceItem:
		    /* TODO: source script */
		    SourceDialog();
		    break;
		case kCloseItem:
		    /* Send close event */
		    window = TkMacGetXWindow(FrontWindow());
		    dispPtr = TkGetDisplayList();
		    tkwin = Tk_IdToWindow(dispPtr->display, window);
		    TkGenWMDestroyEvent(tkwin);
		    break;
		case kQuitItem:
		    /* Exit */
		    if (optionKeyPressed || gInterp == NULL) {
			Tcl_Exit(0);
		    } else {
			Tcl_Eval(gInterp, "exit");
		    }
		    break;
	    }
	    break;
	case kEditMenu:
	    /*
	     * This implementation just send keysyms
	     * the Tk thinks are associated with function keys that
	     * do Cut, Copy & Paste on a Sun keyboard.
	     */
	    GenerateEditEvent(theItem);
	    break;
	default:
	    TkMacDispatchMenuEvent(theMenu, theItem);
	    TkMacClearMenubarActive();
	    break;
    }

    /*
     * Finally we unhighlight the menu.
     */
    HiliteMenu(0);
} /* TkMacHandleMenuSelect */

/*
 *----------------------------------------------------------------------
 *
 * TkMacInitMenus --
 *
 *	This procedure initializes the Macintosh menu bar.
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
TkMacInitMenus(
    Tcl_Interp 	*interp)
{
    gInterp = interp;

    /* 
     * At this point, InitMenus() should have already been called. 
     */

    if (TkMacUseMenuID(256) != TCL_OK) {
    	panic("Menu ID 256 is already in use!");
    }
    tkAppleMenu = NewMenu(256, "\p\024");
    if (tkAppleMenu == NULL) {
	panic("memory - menus");
    }
    InsertMenu(tkAppleMenu, 0);
    AppendMenu(tkAppleMenu, "\pAbout Tcl & TkÉ");
    AppendMenu(tkAppleMenu, "\p(-");
    AddResMenu(tkAppleMenu, 'DRVR');

    if (TkMacUseMenuID(kFileMenu) != TCL_OK) {
    	panic("Menu ID %d is already in use!", kFileMenu);
    }
    tkFileMenu = NewMenu(kFileMenu, "\pFile");
    if (tkFileMenu == NULL) {
	panic("memory - menus");
    }
    InsertMenu(tkFileMenu, 0);
    AppendMenu(tkFileMenu, "\pSourceÉ");
    AppendMenu(tkFileMenu, "\pClose/W");
    AppendMenu(tkFileMenu, "\p(-");
    AppendMenu(tkFileMenu, "\pQuit/Q");

    if (TkMacUseMenuID(kEditMenu) != TCL_OK) {
    	panic("Menu ID %d is already in use!", kEditMenu);
    }
    tkEditMenu = NewMenu(kEditMenu, "\pEdit");
    if (tkEditMenu == NULL) {
	panic("memory - menus");
    }
    InsertMenu(tkEditMenu, 0);
    AppendMenu(tkEditMenu, "\pCut/X");
    AppendMenu(tkEditMenu, "\pCopy/C");
    AppendMenu(tkEditMenu, "\pPaste/V");
    AppendMenu(tkEditMenu, "\pClear");
    if (TkMacUseMenuID(kHMHelpMenuID) != TCL_OK) {
    	panic("Help menu ID %s is already in use!", kHMHelpMenuID);
    }
    
    DrawMenuBar();
    TkMacSetHelpMenuItemCount();
    
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * GenerateEditEvent --
 *
 *	Takes an edit menu item and posts the corasponding a virtual 
 *	event to Tk's event queue.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	May place events of queue.
 *
 *----------------------------------------------------------------------
 */

static void 
GenerateEditEvent(
    int flag)
{
    XVirtualEvent event;
    Point where;
    Tk_Window tkwin;
    Window window;
    TkDisplay *dispPtr;

    window = TkMacGetXWindow(FrontWindow());
    dispPtr = TkGetDisplayList();
    tkwin = Tk_IdToWindow(dispPtr->display, window);
    tkwin = (Tk_Window) ((TkWindow *) tkwin)->dispPtr->focusPtr;
    if (tkwin == NULL) {
	return;
    }

    event.type = VirtualEvent;
    event.serial = Tk_Display(tkwin)->request;
    event.send_event = false;
    event.display = Tk_Display(tkwin);
    event.event = Tk_WindowId(tkwin);
    event.root = XRootWindow(Tk_Display(tkwin), 0);
    event.subwindow = None;
    event.time = TkpGetMS();
    
    GetMouse(&where);
    tkwin = Tk_TopCoordsToWindow(tkwin, where.h, where.v, 
	    &event.x, &event.y);
    LocalToGlobal(&where);
    event.x_root = where.h;
    event.y_root = where.v;
    event.state = TkMacButtonKeyState();
    event.same_screen = true;
    
    switch (flag) {
	case EDIT_CUT:
	    event.name = Tk_GetUid("Cut");
	    break;
	    
	case EDIT_COPY:
	    event.name = Tk_GetUid("Copy");
	    break;
	    
	case EDIT_PASTE:
	    event.name = Tk_GetUid("Paste");
	    break;
	    
	case EDIT_CLEAR:
	    event.name = Tk_GetUid("Clear");
	    break;
    }
    Tk_QueueWindowEvent((XEvent *) &event, TCL_QUEUE_TAIL);
}

/*
 *----------------------------------------------------------------------
 *
 * SourceDialog --
 *
 *	Presents a dialog to the user for selecting a Tcl file.  The
 *	selected file will be sourced into the main interpreter.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static void 
SourceDialog()
{
    StandardFileReply reply;
    OSType fileTypes[1];
    OSErr err;
    int length, result;
    Handle path;
    
    if (gInterp == NULL) {
	return;
    }
    
    fileTypes[0] = 'TEXT';
    StandardGetFile(NULL, 1, fileTypes, &reply);
    if (reply.sfGood == false) {
	return;
    }
    
    err = FSpPathFromLocation(&reply.sfFile, &length, &path);
    if (err == noErr) {
	HLock(path);
	result = Tcl_EvalFile(gInterp, *path);
	HUnlock(path);
	DisposeHandle(path);
    }
    if (result == TCL_ERROR) {
	Tcl_BackgroundError(gInterp);
    }	   
}
