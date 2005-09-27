/*
 * TkMacMDEF.c --
 *
 *	This module is implements the MDEF for tkMenus. The address of the
 *	real entry proc will be blasted into the MDEF.
 *
 * Copyright (c) 1996 by Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#define MAC_TCL
#define NeedFunctionPrototypes 1
#define NeedWidePrototypes 0

#include <Menus.h>
#include <LowMem.h>
#include "tkMacInt.h"


/*
 * The following structure is built from assembly equates in MPW 3.0 
 * AIncludes file: "Private.a." We're forced to update several locations not
 * documented in "Inside Mac" to make our MDEF behave properly with hierarchical 
 * menus.
 */

#if STRUCTALIGNMENTSUPPORTED
#pragma options align=mac68k
#endif
typedef struct mbPrivate {
    Byte unknown[6];
    Rect mbItemRect; 		/* rect of currently chosen menu item */
} mbPrivate;
#if STRUCTALIGNMENTSUPPORTED
#pragma options align=reset
#endif

/*
 * We are forced to update a low-memory global to get cascades to work. This
 * global does not have a LMEquate associated with it.
 */

#define SELECTRECT (*(Rect *)0x09fa) 	/* Menu select seems to need this */
#define	MBSAVELOC (*(short *)0x0B5C) 	/* address of handle to mbarproc private data redefined below */

pascal void		main _ANSI_ARGS_((short message,
			    MenuHandle menu, Rect *menuRect,
			    Point hitPt, short *whichItem));


/*
 *----------------------------------------------------------------------
 *
 * TkMacStdMenu --
 *
 *	The dispatch routine called by the system to handle menu drawing,
 *	scrolling, etc. This is a stub; the address of the real routine
 *	is blasted in. The real routine will be a UniversalProcPtr,
 *	which will give the real dispatch routine in Tk globals
 *	and the like.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	This routine causes menus to be drawn and will certainly allocate
 *	memory as a result. Also, the menu can scroll up and down, and
 *	various other interface actions can take place
 *
 *----------------------------------------------------------------------
 */

pascal void
main(
    short message,		/* What action are we taking? */
    MenuHandle menu,		/* The menu we are working with */
    Rect *menuRect,		/* A pointer to the rect we are working with */
    Point hitPt,		/* Where the mouse was hit for appropriate
    				 * messages. */
    short *whichItemPtr)	/* Output result. Which item was hit by
    				 * the user? */
{	
    /*
     * The constant 'MDEF' is what will be punched during menu intialization.
     */

    TkMenuDefProcPtr procPtr = (TkMenuDefProcPtr) 'MDEF';
    TkMenuLowMemGlobals globals;
    short oldItem;
   
    globals.menuDisable = LMGetMenuDisable();
    globals.menuTop = LMGetTopMenuItem();
    globals.menuBottom = LMGetAtMenuBottom();
    if (MBSAVELOC == -1) {
        globals.itemRect = (**(mbPrivate***)&MBSAVELOC)->mbItemRect;
    }
    if (message == mChooseMsg) {
        oldItem = *whichItemPtr;
    }
    
    TkCallMenuDefProc(procPtr, message, menu, menuRect, hitPt, whichItemPtr,
    	    &globals);
    
    LMSetMenuDisable(globals.menuDisable);
    LMSetTopMenuItem(globals.menuTop);
    LMSetAtMenuBottom(globals.menuBottom);
    if ((message == mChooseMsg) && (oldItem != *whichItemPtr) 
    	    && (MBSAVELOC != -1)) {
    	(**(mbPrivate***)&MBSAVELOC)->mbItemRect = globals.itemRect;
      	SELECTRECT = globals.itemRect;
    }
}
