/* 
 * tclMacExit.c --
 *
 *	This file contains routines that deal with cleaning up various state
 *	when Tcl/Tk applications quit.  Unfortunantly, not all state is cleaned
 *	up by the process when an application quites or crashes.  Also you
 *	need to do different things depending on wether you are running as
 *	68k code, PowerPC, or a code resource.  The Exit handler code was 
 *	adapted from code posted on alt.sources.mac by Dave Nebinger.
 *
 * Copyright (c) 1995 Dave Nebinger.
 * Copyright (c) 1995-1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclMacInt.h"
#include <SegLoad.h>
#include <Traps.h>
#include <Processes.h>

/*
 * Various typedefs and defines needed to patch ExitToShell.
 */
 
enum {
        uppExitToShellProcInfo = kPascalStackBased
};

#if GENERATINGCFM
typedef UniversalProcPtr ExitToShellUPP;

#define CallExitToShellProc(userRoutine)        \
        CallUniversalProc((UniversalProcPtr)(userRoutine),uppExitToShellProcInfo)
#define NewExitToShellProc(userRoutine) \
        (ExitToShellUPP)NewRoutineDescriptor((ProcPtr)(userRoutine), \
		uppExitToShellProcInfo, GetCurrentArchitecture())

#else
typedef ExitToShellProcPtr ExitToShellUPP;

#define CallExitToShellProc(userRoutine)        \
        (*(userRoutine))()
#define NewExitToShellProc(userRoutine) \
        (ExitToShellUPP)(userRoutine)
#endif

#define DisposeExitToShellProc(userRoutine) \
        DisposeRoutineDescriptor(userRoutine)

#if defined(powerc)||defined(__powerc)
#pragma options align=mac68k
#endif
struct ExitToShellUPPList{
        struct ExitToShellUPPList* nextProc;
        ExitToShellUPP userProc;
};
#if defined(powerc)||defined(__powerc)
#pragma options align=reset
#endif

typedef struct ExitToShellDataStruct ExitToShellDataRec,* ExitToShellDataPtr,** ExitToShellDataHdl;

typedef struct ExitToShellUPPList ExitToShellUPPList,* ExitToShellUPPListPtr,** ExitToShellUPPHdl;

#if defined(powerc)||defined(__powerc)
#pragma options align=mac68k
#endif
struct ExitToShellDataStruct{
    unsigned long a5;
    ExitToShellUPPList* userProcs;
    ExitToShellUPP oldProc;
};
#if defined(powerc)||defined(__powerc)
#pragma options align=reset
#endif

/*
 * Static globals used within this file.
 */
static ExitToShellDataPtr gExitToShellData = (ExitToShellDataPtr) NULL;


/*
 *----------------------------------------------------------------------
 *
 * TclPlatformExit --
 *
 *	This procedure implements the Macintosh specific exit routine.
 *	We explicitly callthe ExitHandler function to do various clean
 *	up.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	We exit the process.
 *
 *----------------------------------------------------------------------
 */

void
TclpExit(
    int status)		/* Ignored. */
{
    TclMacExitHandler();

/* 
 * If we are using the Metrowerks Standard Library, then we will call its exit so that it
 * will get a chance to clean up temp files, and so forth.  It always calls the standard 
 * ExitToShell, so the Tcl handlers will also get called.
 *   
 * If you have another exit, make sure that it does not patch ExitToShell, and does
 * call it.  If so, it will probably work as well.
 *
 */
 
#ifdef __MSL__    
    exit(status);
#else
    ExitToShell();
#endif

}

/*
 *----------------------------------------------------------------------
 *
 * TclMacExitHandler --
 *
 *	This procedure is invoked after Tcl at the last possible moment
 *	to clean up any state Tcl has left around that may cause other
 *	applications to crash.  For example, this function can be used
 *	as the termination routine for CFM applications.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Various cleanup occurs.
 *
 *----------------------------------------------------------------------
 */

void
TclMacExitHandler()
{
    ExitToShellUPPListPtr curProc;

    /*
     * Loop through all installed Exit handlers
     * and call them.  Always make sure we are in
     * a clean state in case we are recursivly called.
     */
    if ((gExitToShellData) != NULL && (gExitToShellData->userProcs != NULL)){
    
	/*
	 * Call the installed exit to shell routines.
	 */
	curProc = gExitToShellData->userProcs;
	do {
	    gExitToShellData->userProcs = curProc->nextProc;
	    CallExitToShellProc(curProc->userProc);
	    DisposeExitToShellProc(curProc->userProc);
	    DisposePtr((Ptr) curProc);
	    curProc = gExitToShellData->userProcs;
	} while (curProc != (ExitToShellUPPListPtr) NULL);
    }

    return;
}

/*
 *----------------------------------------------------------------------
 *
 * TclMacInstallExitToShellPatch --
 *
 *	This procedure installs a way to clean up state at the latest
 *	possible moment before we exit.  These are things that must
 *	be cleaned up or the system will crash.  The exact way in which
 *	this is implemented depends on the architecture in which we are
 *	running.  For 68k applications we patch the ExitToShell call.
 *	For PowerPC applications we just create a list of procs to call.
 *	The function ExitHandler should be installed in the Code 
 *	Fragments terminiation routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Installs the new routine.
 *
 *----------------------------------------------------------------------
 */

OSErr 
TclMacInstallExitToShellPatch(
    ExitToShellProcPtr newProc)		/* Function pointer. */
{
    ExitToShellUPP exitHandler;
    ExitToShellUPPListPtr listPtr;

    if (gExitToShellData == (ExitToShellDataPtr) NULL){
	TclMacInitExitToShell(true);
    }

    /*
     * Add the passed in function pointer to the list of functions
     * to be called when ExitToShell is called.
     */
    exitHandler = NewExitToShellProc(newProc);
    listPtr = (ExitToShellUPPListPtr) NewPtrClear(sizeof(ExitToShellUPPList));
    listPtr->userProc = exitHandler;
    listPtr->nextProc = gExitToShellData->userProcs;
    gExitToShellData->userProcs = listPtr;

    return noErr;
}

/*
 *----------------------------------------------------------------------
 *
 * ExitToShellPatchRoutine --
 *
 *	This procedure is invoked when someone calls ExitToShell for
 *	this application.  This function performs some last miniute
 *	clean up and then calls the real ExitToShell routine.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Various cleanup occurs.
 *
 *----------------------------------------------------------------------
 */

static pascal void
ExitToShellPatchRoutine()
{
    ExitToShellUPP oldETS;
    long oldA5;

    /*
     * Set up our A5 world.  This allows us to have
     * access to our global variables in the 68k world.
     */
    oldA5 = SetCurrentA5();
    SetA5(gExitToShellData->a5);

    /*
     * Call the function that invokes all
     * of the handlers.
     */
    TclMacExitHandler();

    /*
     * Call the origional ExitToShell routine.
     */
    oldETS = gExitToShellData->oldProc;
    DisposePtr((Ptr) gExitToShellData);
    SetA5(oldA5);
    CallExitToShellProc(oldETS);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * TclMacInitExitToShell --
 *
 *	This procedure initializes the ExitToShell clean up machanism.
 *	Generally, this is handled automatically when users make a call
 *	to InstallExitToShellPatch.  However, it can be called 
 *	explicitly at startup time to turn off the patching mechanism.
 *	This can be used by code resources which could be removed from
 *	the application before ExitToShell is called.
 *
 *	Note, if we are running from CFM code we never install the
 *	patch.  Instead, the function ExitHandler should be installed
 *	as the terminiation routine for the code fragment.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Creates global state.
 *
 *----------------------------------------------------------------------
 */

void 
TclMacInitExitToShell(
    int usePatch)	/* True if on 68k. */
{
    if (gExitToShellData == (ExitToShellDataPtr) NULL){
#if GENERATINGCFM
	gExitToShellData = (ExitToShellDataPtr)
	  NewPtr(sizeof(ExitToShellDataRec));
	gExitToShellData->a5 = SetCurrentA5();
	gExitToShellData->userProcs = (ExitToShellUPPList*) NULL;
#else
	ExitToShellUPP oldExitToShell, newExitToShellPatch;
	short exitToShellTrap;
	
	/*
	 * Initialize patch mechanism.
	 */
	 
	gExitToShellData = (ExitToShellDataPtr) NewPtr(sizeof(ExitToShellDataRec));
	gExitToShellData->a5 = SetCurrentA5();
	gExitToShellData->userProcs = (ExitToShellUPPList*) NULL;

	/*
	 * Save state needed to call origional ExitToShell routine.  Install
	 * the new ExitToShell code in it's place.
	 */
	if (usePatch) {
	    exitToShellTrap = _ExitToShell & 0x3ff;
	    newExitToShellPatch = NewExitToShellProc(ExitToShellPatchRoutine);
	    oldExitToShell = (ExitToShellUPP)
	      NGetTrapAddress(exitToShellTrap, ToolTrap);
	    NSetTrapAddress((UniversalProcPtr) newExitToShellPatch,
		    exitToShellTrap, ToolTrap);
	    gExitToShellData->oldProc = oldExitToShell;
	}
#endif
    }
}
