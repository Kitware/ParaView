/*
 * tkMacClipboard.c --
 *
 * 	This file manages the clipboard for the Tk toolkit.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"
#include "tkPort.h"
#include "tkMacInt.h"

#include <Scrap.h>
#include <Events.h>

#include "tkSelect.h"

/*
 *----------------------------------------------------------------------
 *
 * TkSelGetSelection --
 *
 *	Retrieve the specified selection from another process.  For
 *	now, only fetching XA_STRING from CLIPBOARD is supported.
 *	Eventually other types should be allowed.
 * 
 * Results:
 *	The return value is a standard Tcl return value.
 *	If an error occurs (such as no selection exists)
 *	then an error message is left in the interp's result.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TkSelGetSelection(
    Tcl_Interp *interp,		/* Interpreter to use for reporting
				 * errors. */
    Tk_Window tkwin,		/* Window on whose behalf to retrieve
				 * the selection (determines display
				 * from which to retrieve). */
    Atom selection,		/* Selection to retrieve. */
    Atom target,		/* Desired form in which selection
				 * is to be returned. */
    Tk_GetSelProc *proc,	/* Procedure to call to process the
				 * selection, once it has been retrieved. */
    ClientData clientData)	/* Arbitrary value to pass to proc. */
{
    int result;
    long length, offset = 0;
    Handle handle;

    if ((selection == Tk_InternAtom(tkwin, "CLIPBOARD"))
	    && (target == XA_STRING)) {
	/* 
	 * Get the scrap from the Macintosh global clipboard.
	 */
	handle = NewHandle(1);
	length = GetScrap(handle, 'TEXT', &offset);
	if (length > 0) {
	    SetHandleSize(handle, (Size) length + 1);
	    HLock(handle);
	    (*handle)[length] = '\0';
	    
	    result = (*proc)(clientData, interp, *handle);
	    
	    HUnlock(handle);
	    DisposeHandle(handle);
	    return result;
	}
	
	DisposeHandle(handle);
    }
    
    Tcl_AppendResult(interp, Tk_GetAtomName(tkwin, selection),
	" selection doesn't exist or form \"", Tk_GetAtomName(tkwin, target),
	"\" not defined", (char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * TkSetSelectionOwner --
 *
 *	This function claims ownership of the specified selection.
 *	If the selection is CLIPBOARD, then we empty the system
 *	clipboard.
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
XSetSelectionOwner(
    Display* display,	/* X Display. */
    Atom selection,	/* What selection to own. */
    Window owner,	/* Window to be the owner. */
    Time time)		/* The current time? */
{
    Tk_Window tkwin;
    TkDisplay *dispPtr;

    /*
     * This is a gross hack because the Tk_InternAtom interface is broken.
     * It expects a Tk_Window, even though it only needs a Tk_Display.
     */

    tkwin = (Tk_Window) TkGetMainInfoList()->winPtr;

    if (selection == Tk_InternAtom(tkwin, "CLIPBOARD")) {

	/*
	 * Only claim and empty the clipboard if we aren't already the
	 * owner of the clipboard.
	 */

	dispPtr = TkGetMainInfoList()->winPtr->dispPtr;
	if (dispPtr->clipboardActive) {
	    return;
	}
	ZeroScrap();
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkSelUpdateClipboard --
 *
 *	This function is called to force the clipboard to be updated
 *	after new data is added.  On the Mac we don't need to do
 *	anything.
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
TkSelUpdateClipboard(
    TkWindow *winPtr,			/* Window associated with clipboard. */
    TkClipboardTarget *targetPtr)	/* Info about the content. */
{
}

/*
 *--------------------------------------------------------------
 *
 * TkSelEventProc --
 *
 *	This procedure is invoked whenever a selection-related
 *	event occurs. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Lots:  depends on the type of event.
 *
 *--------------------------------------------------------------
 */

void
TkSelEventProc(
    Tk_Window tkwin,		/* Window for which event was
				 * targeted. */
    register XEvent *eventPtr)	/* X event:  either SelectionClear,
				 * SelectionRequest, or
				 * SelectionNotify. */
{
    if (eventPtr->type == SelectionClear) {
	TkSelClearSelection(tkwin, eventPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkSelPropProc --
 *
 *	This procedure is invoked when property-change events
 *	occur on windows not known to the toolkit.  This is a stub
 *	function under Windows.
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
TkSelPropProc(
    register XEvent *eventPtr)		/* X PropertyChange event. */
{
}

/*
 *----------------------------------------------------------------------
 *
 * TkSuspendClipboard --
 *
 *	Handle clipboard conversion as required by the suppend event.
 *	This function is also called on exit.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The local scrap is moved to the global scrap.
 *
 *----------------------------------------------------------------------
 */

void
TkSuspendClipboard()
{
    TkClipboardTarget *targetPtr;
    TkClipboardBuffer *cbPtr;
    TkDisplay *dispPtr;
    char *buffer, *p, *endPtr, *buffPtr;
    long length;

    dispPtr = TkGetDisplayList();
    if ((dispPtr == NULL) || !dispPtr->clipboardActive) {
	return;
    }

    for (targetPtr = dispPtr->clipTargetPtr; targetPtr != NULL;
	    targetPtr = targetPtr->nextPtr) {
	if (targetPtr->type == XA_STRING)
	    break;
    }
    if (targetPtr != NULL) {
	length = 0;
	for (cbPtr = targetPtr->firstBufferPtr; cbPtr != NULL;
		cbPtr = cbPtr->nextPtr) {
	    length += cbPtr->length;
	}

	buffer = ckalloc(length);
	buffPtr = buffer;
	for (cbPtr = targetPtr->firstBufferPtr; cbPtr != NULL;
		cbPtr = cbPtr->nextPtr) {
	    for (p = cbPtr->buffer, endPtr = p + cbPtr->length;
		    p < endPtr; p++) {
		if (*p == '\n') {
		    *buffPtr++ = '\r';
		} else {
		    *buffPtr++ = *p;
		}
	    }
	}

	ZeroScrap();
	PutScrap(length, 'TEXT', buffer);
	ckfree(buffer);
    }

    /*
     * The system now owns the scrap.  We tell Tk that it has
     * lost the selection so that it will look for it the next time
     * it needs it.  (Window list NULL if quiting.)
     */

    if (TkGetMainInfoList() != NULL) {
	Tk_ClearSelection((Tk_Window) TkGetMainInfoList()->winPtr, 
		Tk_InternAtom((Tk_Window) TkGetMainInfoList()->winPtr,
			"CLIPBOARD"));
    }

    return;
}
