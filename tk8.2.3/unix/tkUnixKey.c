/* 
 * tkUnixKey.c --
 *
 *	This file contains routines for dealing with international keyboard
 *	input.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"


/*
 *----------------------------------------------------------------------
 *
 * TkpGetString --
 *
 *	Retrieve the UTF string associated with a keyboard event.
 *
 * Results:
 *	Returns the UTF string.
 *
 * Side effects:
 *	Stores the input string in the specified Tcl_DString.  Modifies
 *	the internal input state.  This routine can only be called
 *	once for a given event.
 *
 *----------------------------------------------------------------------
 */

char *
TkpGetString(winPtr, eventPtr, dsPtr)
    TkWindow *winPtr;		/* Window where event occurred:  needed to
				 * get input context. */
    XEvent *eventPtr;		/* X keyboard event. */
    Tcl_DString *dsPtr;		/* Uninitialized or empty string to hold
				 * result. */
{
    int len;
    Tcl_DString buf;
    Status status;

    /*
     * Overallocate the dstring to the maximum stack amount.
     */

    Tcl_DStringInit(&buf);
    Tcl_DStringSetLength(&buf, TCL_DSTRING_STATIC_SIZE-1);
    
#ifdef TK_USE_INPUT_METHODS
    if ((winPtr->inputContext != NULL)
	    && (eventPtr->type == KeyPress)) {
	len = XmbLookupString(winPtr->inputContext, &eventPtr->xkey,
		Tcl_DStringValue(&buf), Tcl_DStringLength(&buf),
		(KeySym *) NULL, &status);
	/*
	 * If the buffer wasn't big enough, grow the buffer and try again.
	 */

	if (status == XBufferOverflow) {
	    Tcl_DStringSetLength(&buf, len);
	    len = XmbLookupString(winPtr->inputContext, &eventPtr->xkey,
		    Tcl_DStringValue(&buf), len, (KeySym *) NULL, &status);
	}
	if ((status != XLookupChars)
		&& (status != XLookupBoth)) {
	    len = 0;
	}
    } else {
	len = XLookupString(&eventPtr->xkey, Tcl_DStringValue(&buf),
		Tcl_DStringLength(&buf), (KeySym *) NULL,
		(XComposeStatus *) NULL);
    }
#else /* TK_USE_INPUT_METHODS */
    len = XLookupString(&eventPtr->xkey, Tcl_DStringValue(&buf),
	    Tcl_DStringLength(&buf), (KeySym *) NULL,
	    (XComposeStatus *) NULL);
#endif /* TK_USE_INPUT_METHODS */
    Tcl_DStringSetLength(&buf, len);

    Tcl_ExternalToUtfDString(NULL, Tcl_DStringValue(&buf), len, dsPtr);
    Tcl_DStringFree(&buf);

    return Tcl_DStringValue(dsPtr);
}
