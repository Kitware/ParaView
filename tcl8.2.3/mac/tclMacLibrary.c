/*
 * tclMacLibrary.c --
 *
 *	This file should be included in Tcl extensions that want to 
 *	automatically oepn their resource forks when the code is linked. 
 *	These routines should not be exported but should be compiled 
 *	locally by each fragment.  Many thanks to Jay Lieske
 *	<lieske@princeton.edu> who provide an initial version of this
 *	file.
 *
 * Copyright (c) 1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

/*
 * Here is another place that we are using the old routine names...
 */
 
#define OLDROUTINENAMES 1

#include <CodeFragments.h>
#include <Errors.h>
#include <Resources.h>
#include <Strings.h>
#include "tclMacInt.h"

/*
 * These function are not currently defined in any header file.  The
 * only place they should be used is in the Initialization and
 * Termination entry points for a code fragment.  The prototypes
 * are included here to avoid compile errors.
 */

OSErr TclMacInitializeFragment _ANSI_ARGS_((
			struct CFragInitBlock* initBlkPtr));
void TclMacTerminateFragment _ANSI_ARGS_((void));

/*
 * Static functions in this file.
 */

static OSErr OpenLibraryResource _ANSI_ARGS_((
			struct CFragInitBlock* initBlkPtr));
static void CloseLibraryResource _ANSI_ARGS_((void));

/* 
 * The refnum of the opened resource fork.
 */
static short ourResFile = kResFileNotOpened;

/*
 * This is the resource token for the our resource file.
 * It stores the name we registered with the resource facility.
 * We only need to use this if we are actually registering ourselves.
 */
  
#ifdef TCL_REGISTER_LIBRARY
static Tcl_Obj *ourResToken;
#endif

/*
 *----------------------------------------------------------------------
 *
 * TclMacInitializeFragment --
 *
 *	Called by MacOS CFM when the shared library is loaded. All this
 *	function really does is give Tcl a chance to open and register
 *	the resource fork of the library. 
 *
 * Results:
 *	MacOS error code if loading should be canceled.
 *
 * Side effects:
 *	Opens the resource fork of the shared library file.
 *
 *----------------------------------------------------------------------
 */

OSErr
TclMacInitializeFragment(
    struct CFragInitBlock* initBlkPtr)		/* Pointer to our library. */
{
    OSErr err = noErr;

#ifdef __MWERKS__
    {
    	extern OSErr __initialize( CFragInitBlock* initBlkPtr);
    	err = __initialize((CFragInitBlock *) initBlkPtr);
    }
#endif
    if (err == noErr)
    	err = OpenLibraryResource( initBlkPtr);
    return err;
}

/*
 *----------------------------------------------------------------------
 *
 * TclMacTerminateFragment --
 *
 *	Called by MacOS CFM when the shared library is unloaded.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The resource fork of the code fragment is closed.
 *
 *----------------------------------------------------------------------
 */

void 
TclMacTerminateFragment()
{
    CloseLibraryResource();

#ifdef __MWERKS__
    {
    	extern void __terminate(void);
    	__terminate();
    }
#endif
}

/*
 *----------------------------------------------------------------------
 *
 * OpenLibraryResource --
 *
 *	This routine can be called by a MacOS fragment's initialiation 
 *	function to open the resource fork of the file.  
 *	Call it with the same data passed to the initialization function. 
 *	If the fragment loading should fail if the resource fork can't 
 *	be opened, then the initialization function can pass on this 
 *	return value.
 *
 *      If you #define TCL_REGISTER_RESOURCE before compiling this resource, 
 *	then your library will register its open resource fork with the
 *      resource command.
 *
 * Results:
 *	It returns noErr on success and a MacOS error code on failure.
 *
 * Side effects:
 *	The resource fork of the code fragment is opened read-only and 
 *	is installed at the head of the resource chain.
 *
 *----------------------------------------------------------------------
 */

static OSErr 
OpenLibraryResource(
    struct CFragInitBlock* initBlkPtr)
{
    /*
     * The 3.0 version of the Universal headers changed CFragInitBlock
     * to an opaque pointer type.  CFragSystem7InitBlock is now the
     * real pointer.
     */
     
#if !defined(UNIVERSAL_INTERFACES_VERSION) || (UNIVERSAL_INTERFACES_VERSION < 0x0300)
    struct CFragInitBlock *realInitBlkPtr = initBlkPtr;
#else 
    CFragSystem7InitBlock *realInitBlkPtr = (CFragSystem7InitBlock *) initBlkPtr;
#endif
    FSSpec* fileSpec = NULL;
    OSErr err = noErr;
    

    if (realInitBlkPtr->fragLocator.where == kOnDiskFlat) {
    	fileSpec = realInitBlkPtr->fragLocator.u.onDisk.fileSpec;
    } else if (realInitBlkPtr->fragLocator.where == kOnDiskSegmented) {
    	fileSpec = realInitBlkPtr->fragLocator.u.inSegs.fileSpec;
    } else {
    	err = resFNotFound;
    }

    /*
     * Open the resource fork for this library in read-only mode.  
     * This will make it the current res file, ahead of the 
     * application's own resources.
     */
    
    if (fileSpec != NULL) {
	ourResFile = FSpOpenResFile(fileSpec, fsRdPerm);
	if (ourResFile == kResFileNotOpened) {
	    err = ResError();
	} else {
#ifdef TCL_REGISTER_LIBRARY
	    ourResToken = Tcl_NewObj();
	    Tcl_IncrRefCount(ourResToken);
	    p2cstr(realInitBlkPtr->libName);
	    Tcl_SetStringObj(ourResToken, (char *) realInitBlkPtr->libName, -1);
	    c2pstr((char *) realInitBlkPtr->libName);
	    TclMacRegisterResourceFork(ourResFile, ourResToken,
	            TCL_RESOURCE_DONT_CLOSE);
#endif
            SetResFileAttrs(ourResFile, mapReadOnly);
	}
    }
    
    return err;
}

/*
 *----------------------------------------------------------------------
 *
 * CloseLibraryResource --
 *
 *	This routine should be called by a MacOS fragment's termination 
 *	function to close the resource fork of the file 
 *	that was opened with OpenLibraryResource.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The resource fork of the code fragment is closed.
 *
 *----------------------------------------------------------------------
 */

static void
CloseLibraryResource()
{
    if (ourResFile != kResFileNotOpened) {
#ifdef TCL_REGISTER_LIBRARY
        int length;
        TclMacUnRegisterResourceFork(
	        Tcl_GetStringFromObj(ourResToken, &length),
                NULL);
        Tcl_DecrRefCount(ourResToken);
#endif
	CloseResFile(ourResFile);
	ourResFile = kResFileNotOpened;
    }
}
