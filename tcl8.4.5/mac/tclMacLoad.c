/*
 * tclMacLoad.c --
 *
 *	This procedure provides a version of the TclLoadFile for use
 *	on the Macintosh.  This procedure will only work with systems 
 *	that use the Code Fragment Manager.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <CodeFragments.h>
#include <Errors.h>
#include <Resources.h>
#include <Strings.h>
#include <FSpCompat.h>

/*
 * Seems that the 3.0.1 Universal headers leave this define out.  So we
 * define it here...
 */
 
#ifndef fragNoErr
    #define fragNoErr noErr
#endif

#include "tclPort.h"
#include "tclInt.h"
#include "tclMacInt.h"

#if GENERATINGPOWERPC
    #define OUR_ARCH_TYPE kPowerPCCFragArch
#else
    #define OUR_ARCH_TYPE kMotorola68KCFragArch
#endif

/*
 * The following data structure defines the structure of a code fragment
 * resource.  We can cast the resource to be of this type to access
 * any fields we need to see.
 */
struct CfrgHeader {
    long 	res1;
    long 	res2;
    long 	version;
    long 	res3;
    long 	res4;
    long 	filler1;
    long 	filler2;
    long 	itemCount;
    char	arrayStart;	/* Array of externalItems begins here. */
};
typedef struct CfrgHeader CfrgHeader, *CfrgHeaderPtr, **CfrgHeaderPtrHand;

/*
 * The below structure defines a cfrag item within the cfrag resource.
 */
struct CfrgItem {
    OSType 	archType;
    long 	updateLevel;
    long	currVersion;
    long	oldDefVersion;
    long	appStackSize;
    short	appSubFolder;
    char	usage;
    char	location;
    long	codeOffset;
    long	codeLength;
    long	res1;
    long	res2;
    short	itemSize;
    Str255	name;		/* This is actually variable sized. */
};
typedef struct CfrgItem CfrgItem;

/*
 * On MacOS, old shared libraries which contain many code fragments
 * cannot, it seems, be loaded in one go.  We need to look provide
 * the name of a code fragment while we load.  Since with the
 * separation of the 'load' and 'findsymbol' be do not necessarily
 * know a symbol name at load time, we have to store some further
 * information in a structure like this so we can ensure we load
 * properly in 'findsymbol' if the first attempts didn't work.
 */
typedef struct TclMacLoadInfo {
    int loaded;
    CFragConnectionID connID;
    FSSpec fileSpec;
} TclMacLoadInfo;

static int TryToLoad(Tcl_Interp *interp, TclMacLoadInfo *loadInfo, Tcl_Obj *pathPtr, 
		     CONST char *sym /* native */);


/*
 *----------------------------------------------------------------------
 *
 * TclpDlopen --
 *
 *	This procedure is called to carry out dynamic loading of binary
 *	code for the Macintosh.  This implementation is based on the
 *	Code Fragment Manager & will not work on other systems.
 *
 * Results:
 *	The result is TCL_ERROR, and an error message is left in
 *	the interp's result.
 *
 * Side effects:
 *	New binary code is loaded.
 *
 *----------------------------------------------------------------------
 */

int
TclpDlopen(interp, pathPtr, loadHandle, unloadProcPtr)
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Obj *pathPtr;		/* Name of the file containing the desired
				 * code (UTF-8). */
    Tcl_LoadHandle *loadHandle;	/* Filled with token for dynamically loaded
				 * file which will be passed back to 
				 * (*unloadProcPtr)() to unload the file. */
    Tcl_FSUnloadFileProc **unloadProcPtr;
				/* Filled with address of Tcl_FSUnloadFileProc
				 * function which should be used for
				 * this file. */
{
    OSErr err;
    FSSpec fileSpec;
    CONST char *native;
    TclMacLoadInfo *loadInfo;
    
    native = Tcl_FSGetNativePath(pathPtr);
    err = FSpLocationFromPath(strlen(native), native, &fileSpec);
    
    if (err != noErr) {
	Tcl_SetResult(interp, "could not locate shared library", TCL_STATIC);
	return TCL_ERROR;
    }
    
    loadInfo = (TclMacLoadInfo *) ckalloc(sizeof(TclMacLoadInfo));
    loadInfo->loaded = 0;
    loadInfo->fileSpec = fileSpec;
    loadInfo->connID = NULL;
    
    if (TryToLoad(interp, loadInfo, pathPtr, NULL) != TCL_OK) {
	ckfree((char*) loadInfo);
	return TCL_ERROR;
    }

    *loadHandle = (Tcl_LoadHandle)loadInfo;
    *unloadProcPtr = &TclpUnloadFile;
    return TCL_OK;
}

/* 
 * See the comments about 'struct TclMacLoadInfo' above. This
 * function ensures the appropriate library or symbol is
 * loaded.
 */
static int
TryToLoad(Tcl_Interp *interp, TclMacLoadInfo *loadInfo, Tcl_Obj *pathPtr,
	  CONST char *sym /* native */) 
{
    OSErr err;
    CFragConnectionID connID;
    Ptr dummy;
    short fragFileRef, saveFileRef;
    Handle fragResource;
    UInt32 offset = 0;
    UInt32 length = kCFragGoesToEOF;
    Str255 errName;
    StringPtr fragName=NULL;

    if (loadInfo->loaded == 1) {
        return TCL_OK;
    }

    /*
     * See if this fragment has a 'cfrg' resource.  It will tell us where
     * to look for the fragment in the file.  If it doesn't exist we will
     * assume we have a ppc frag using the whole data fork.  If it does
     * exist we find the frag that matches the one we are looking for and
     * get the offset and size from the resource.
     */
     
    saveFileRef = CurResFile();
    SetResLoad(false);
    fragFileRef = FSpOpenResFile(&loadInfo->fileSpec, fsRdPerm);
    SetResLoad(true);
    if (fragFileRef != -1) {
	if (sym != NULL) {
	    UseResFile(fragFileRef);
	    fragResource = Get1Resource(kCFragResourceType, kCFragResourceID);
	    HLock(fragResource);
	    if (ResError() == noErr) {
		CfrgItem* srcItem;
		long itemCount, index;
		Ptr itemStart;

		itemCount = (*(CfrgHeaderPtrHand)fragResource)->itemCount;
		itemStart = &(*(CfrgHeaderPtrHand)fragResource)->arrayStart;
		for (index = 0; index < itemCount;
		     index++, itemStart += srcItem->itemSize) {
		    srcItem = (CfrgItem*)itemStart;
		    if (srcItem->archType != OUR_ARCH_TYPE) continue;
		    if (!strncasecmp(sym, (char *) srcItem->name + 1,
			    strlen(sym))) {
			offset = srcItem->codeOffset;
			length = srcItem->codeLength;
			fragName=srcItem->name;
		    }
		}
	    }
	}
	/*
	 * Close the resource file.  If the extension wants to reopen the
	 * resource fork it should use the tclMacLibrary.c file during it's
	 * construction.
	 */
	HUnlock(fragResource);
	ReleaseResource(fragResource);
	CloseResFile(fragFileRef);
	UseResFile(saveFileRef);
	if (sym == NULL) {
	    /* We just return */
	    return TCL_OK;
	}
    }

    /*
     * Now we can attempt to load the fragment using the offset & length
     * obtained from the resource.  We don't worry about the main entry point
     * as we are going to search for specific entry points passed to us.
     */
    
    err = GetDiskFragment(&loadInfo->fileSpec, offset, length, fragName,
	    kLoadCFrag, &connID, &dummy, errName);
    
    if (err != fragNoErr) {
	p2cstr(errName);
	if(pathPtr) {
	Tcl_AppendResult(interp, "couldn't load file \"", 
			 Tcl_GetString(pathPtr),
			 "\": ", errName, (char *) NULL);
	} else if(sym) {
	Tcl_AppendResult(interp, "couldn't load library \"", 
			 sym,
			 "\": ", errName, (char *) NULL);
	}
	return TCL_ERROR;
    }

    loadInfo->connID = connID;
    loadInfo->loaded = 1;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpFindSymbol --
 *
 *	Looks up a symbol, by name, through a handle associated with
 *	a previously loaded piece of code (shared library).
 *
 * Results:
 *	Returns a pointer to the function associated with 'symbol' if
 *	it is found.  Otherwise returns NULL and may leave an error
 *	message in the interp's result.
 *
 *----------------------------------------------------------------------
 */
Tcl_PackageInitProc*
TclpFindSymbol(interp, loadHandle, symbol) 
    Tcl_Interp *interp;
    Tcl_LoadHandle loadHandle;
    CONST char *symbol;
{
    Tcl_DString ds;
    Tcl_PackageInitProc *proc=NULL;
    TclMacLoadInfo *loadInfo = (TclMacLoadInfo *)loadHandle;
    Str255 symbolName;
    CFragSymbolClass symClass;
    OSErr err;
   
    if (loadInfo->loaded == 0) {
	int res;
	/*
	 * First thing we must do is infer the package name from the
	 * sym variable.  We do this by removing the '_Init'.
	 */
	Tcl_UtfToExternalDString(NULL, symbol, -1, &ds);
	Tcl_DStringSetLength(&ds, Tcl_DStringLength(&ds) - 5);
	res = TryToLoad(interp, loadInfo, NULL, Tcl_DStringValue(&ds));
	Tcl_DStringFree(&ds);
	if (res != TCL_OK) {
	    return NULL;
	}
    }
    
    Tcl_UtfToExternalDString(NULL, symbol, -1, &ds);
    strcpy((char *) symbolName + 1, Tcl_DStringValue(&ds));
    symbolName[0] = (unsigned) Tcl_DStringLength(&ds);
    err = FindSymbol(loadInfo->connID, symbolName, (Ptr *) &proc, &symClass);
    Tcl_DStringFree(&ds);
    if (err != fragNoErr || symClass == kDataCFragSymbol) {
	Tcl_SetResult(interp,
		"could not find Initialization routine in library",
		TCL_STATIC);
	return NULL;
    }
    return proc;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpUnloadFile --
 *
 *	Unloads a dynamically loaded binary code file from memory.
 *	Code pointers in the formerly loaded file are no longer valid
 *	after calling this function.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Does nothing.  Can anything be done?
 *
 *----------------------------------------------------------------------
 */

void
TclpUnloadFile(loadHandle)
    Tcl_LoadHandle loadHandle;	/* loadHandle returned by a previous call
				 * to TclpDlopen().  The loadHandle is 
				 * a token that represents the loaded 
				 * file. */
{
    TclMacLoadInfo *loadInfo = (TclMacLoadInfo *)loadHandle;
    if (loadInfo->loaded) {
	CloseConnection((CFragConnectionID*) &(loadInfo->connID));
    }
    ckfree((char*)loadInfo);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGuessPackageName --
 *
 *	If the "load" command is invoked without providing a package
 *	name, this procedure is invoked to try to figure it out.
 *
 * Results:
 *	Always returns 0 to indicate that we couldn't figure out a
 *	package name;  generic code will then try to guess the package
 *	from the file name.  A return value of 1 would have meant that
 *	we figured out the package name and put it in bufPtr.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclGuessPackageName(
    CONST char *fileName,	/* Name of file containing package (already
				 * translated to local form if needed). */
    Tcl_DString *bufPtr)	/* Initialized empty dstring.  Append
				 * package name to this if possible. */
{
    return 0;
}
