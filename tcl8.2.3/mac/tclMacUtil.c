/* 
 * tclMacUtil.c --
 *
 *  This contains utility functions used to help with
 *  implementing Macintosh specific portions of the Tcl port.
 *
 * Copyright (c) 1993-1994 Lockheed Missle & Space Company, AI Center
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tcl.h"
#include "tclInt.h"
#include "tclMacInt.h"
#include "tclMath.h"
#include "tclMacPort.h"

#include <Aliases.h>
#include <Errors.h>
#include <Files.h>
#include <Folders.h>
#include <FSpCompat.h>
#include <Strings.h>
#include <TextUtils.h>
#include <MoreFilesExtras.h>

/* 
 * The following two Includes are from the More Files package.
 */
#include <FileCopy.h>
#include <MoreFiles.h>

/*
 *----------------------------------------------------------------------
 *
 * hypotd --
 *
 *	The standard math function hypot is not supported by Think C.
 *	It is included here so everything works. It is supported by
 *	CodeWarrior Pro 1, but the 68K version does not support doubles.
 *	So we hack it in.
 *
 * Results:
 *	Result of computation.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */
 
#if defined(THINK_C) || defined(__MWERKS__)
double hypotd(double x, double y);

double
hypotd(
    double x,		/* X value */
    double y)		/* Y value */
{
    double sum;

    sum = x*x + y*y;
    return sqrt(sum);
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * FSpGetDefaultDir --
 *
 *	This function gets the current default directory.
 *
 * Results:
 *	The provided FSSpec is changed to point to the "default"
 *	directory.  The function returns what ever errors
 *	FSMakeFSSpecCompat may encounter.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
FSpGetDefaultDir(
	FSSpecPtr dirSpec)	/* On return the default directory. */
{
    OSErr err;
    short vRefNum = 0;
    long int dirID = 0;

    err = HGetVol(NULL, &vRefNum, &dirID);
	
    if (err == noErr) {
	err = FSMakeFSSpecCompat(vRefNum, dirID, (ConstStr255Param) NULL,
		dirSpec);
    }
	
    return err;
}

/*
 *----------------------------------------------------------------------
 *
 * FSpSetDefaultDir --
 *
 *	This function sets the default directory to the directory
 *	pointed to by the provided FSSpec.
 *
 * Results:
 *	The function returns what ever errors HSetVol may encounter.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
FSpSetDefaultDir(
	FSSpecPtr dirSpec)	/* The new default directory. */
{
    OSErr err;

    /*
     * The following special case is needed to work around a bug
     * in the Macintosh OS.  (Acutally PC Exchange.)
     */
    
    if (dirSpec->parID == fsRtParID) {
	err = HSetVol(NULL, dirSpec->vRefNum, fsRtDirID);
    } else {
	err = HSetVol(dirSpec->name, dirSpec->vRefNum, dirSpec->parID);
    }
    
    return err;
}

/*
 *----------------------------------------------------------------------
 *
 * FSpFindFolder --
 *
 *	This function is a version of the FindFolder function that 
 *	returns the result as a FSSpec rather than a vRefNum and dirID.
 *
 * Results:
 *	Results will be simaler to that of the FindFolder function.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

OSErr
FSpFindFolder(
    short vRefNum,		/* Volume reference number. */
    OSType folderType,		/* Folder type taken by FindFolder. */
    Boolean createFolder,	/* Should we create it if non-existant. */
    FSSpec *spec)		/* Pointer to resulting directory. */
{
    short foundVRefNum;
    long foundDirID;
    OSErr err;

    err = FindFolder(vRefNum, folderType, createFolder,
	    &foundVRefNum, &foundDirID);
    if (err != noErr) {
	return err;
    }
		
    err = FSMakeFSSpecCompat(foundVRefNum, foundDirID, "\p", spec);
    return err;
}

/*
 *----------------------------------------------------------------------
 *
 * FSpLocationFromPath --
 *
 *	This function obtains an FSSpec for a given macintosh path.
 *	Unlike the More Files function FSpLocationFromFullPath, this
 *	function will also accept partial paths and resolve any aliases
 *	along the path.  
 *
 * Results:
 *	OSErr code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
FSpLocationFromPath(
    int length,			/* Length of path. */
    CONST char *path,		/* The path to convert. */
    FSSpecPtr fileSpecPtr)	/* On return the spec for the path. */
{
    Str255 fileName;
    OSErr err;
    short vRefNum;
    long dirID;
    int pos, cur;
    Boolean isDirectory;
    Boolean wasAlias;

    /*
     * Check to see if this is a full path.  If partial
     * we assume that path starts with the current working
     * directory.  (Ie. volume & dir = 0)
     */
    vRefNum = 0;
    dirID = 0;
    cur = 0;
    if (length == 0) {
        return fnfErr;
    }
    if (path[cur] == ':') {
	cur++;
	if (cur >= length) {
	    /*
	     * If path = ":", just return current directory.
	     */
	    FSMakeFSSpecCompat(0, 0, NULL, fileSpecPtr);
	    return noErr;
	}
    } else {
	while (path[cur] != ':' && cur < length) {
	    cur++;
	}
	if (cur > 255) {
	    return bdNamErr;
	}
	if (cur < length) {
	    /*
	     * This is a full path
	     */
	    cur++;
	    strncpy((char *) fileName + 1, path, cur);
	    fileName[0] = cur;
	    err = FSMakeFSSpecCompat(0, 0, fileName, fileSpecPtr);
	    if (err != noErr) return err;
	    FSpGetDirectoryID(fileSpecPtr, &dirID, &isDirectory);
	    vRefNum = fileSpecPtr->vRefNum;
	} else {
	    cur = 0;
	}
    }
    
    isDirectory = 1;
    while (cur < length) {
	if (!isDirectory) {
	    return dirNFErr;
	}
	pos = cur;
	while (path[pos] != ':' && pos < length) {
	    pos++;
	}
	if (pos == cur) {
	    /* Move up one dir */
	    /* cur++; */
	    strcpy((char *) fileName + 1, "::");
	    fileName[0] = 2;
	} else if (pos - cur > 255) {
	    return bdNamErr;
	} else {
	    strncpy((char *) fileName + 1, &path[cur], pos - cur);
	    fileName[0] = pos - cur;
	}
	err = FSMakeFSSpecCompat(vRefNum, dirID, fileName, fileSpecPtr);
	if (err != noErr) return err;
	err = ResolveAliasFile(fileSpecPtr, true, &isDirectory, &wasAlias);
	if (err != noErr) return err;
	FSpGetDirectoryID(fileSpecPtr, &dirID, &isDirectory);
	vRefNum = fileSpecPtr->vRefNum;
	cur = pos;
	if (path[cur] == ':') {
	    cur++;
	}
    }
    
    return noErr;
}

/*
 *----------------------------------------------------------------------
 *
 * FSpPathFromLocation --
 *
 *	This function obtains a full path name for a given macintosh
 *	FSSpec.  Unlike the More Files function FSpGetFullPath, this
 *	function will return a C string in the Handle.  It also will
 *	create paths for FSSpec that do not yet exist.
 *
 * Results:
 *	OSErr code.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

OSErr
FSpPathFromLocation(
    FSSpec *spec,		/* The location we want a path for. */
    int *length,		/* Length of the resulting path. */
    Handle *fullPath)		/* Handle to path. */
{
    OSErr err;
    FSSpec tempSpec;
    CInfoPBRec pb;
	
    *fullPath = NULL;
	
    /* 
     * Make a copy of the input FSSpec that can be modified.
     */
    BlockMoveData(spec, &tempSpec, sizeof(FSSpec));
	
    if (tempSpec.parID == fsRtParID) {
	/* 
	 * The object is a volume.  Add a colon to make it a full 
	 * pathname.  Allocate a handle for it and we are done.
	 */
	tempSpec.name[0] += 2;
	tempSpec.name[tempSpec.name[0] - 1] = ':';
	tempSpec.name[tempSpec.name[0]] = '\0';
		
	err = PtrToHand(&tempSpec.name[1], fullPath, tempSpec.name[0]);
    } else {
	/* 
	 * The object isn't a volume.  Is the object a file or a directory? 
	 */
	pb.dirInfo.ioNamePtr = tempSpec.name;
	pb.dirInfo.ioVRefNum = tempSpec.vRefNum;
	pb.dirInfo.ioDrDirID = tempSpec.parID;
	pb.dirInfo.ioFDirIndex = 0;
	err = PBGetCatInfoSync(&pb);

	if ((err == noErr) || (err == fnfErr)) {
	    /* 
	     * If the file doesn't currently exist we start over.  If the
	     * directory exists everything will work just fine.  Otherwise we
	     * will just fail later.  If the object is a directory, append a
	     * colon so full pathname ends with colon.
	     */
	    if (err == fnfErr) {
		BlockMoveData(spec, &tempSpec, sizeof(FSSpec));
	    } else if ( (pb.hFileInfo.ioFlAttrib & ioDirMask) != 0 ) {
		tempSpec.name[0] += 1;
		tempSpec.name[tempSpec.name[0]] = ':';
	    }
			
	    /* 
	     * Create a new Handle for the object - make it a C string.
	     */
	    tempSpec.name[0] += 1;
	    tempSpec.name[tempSpec.name[0]] = '\0';
	    err = PtrToHand(&tempSpec.name[1], fullPath, tempSpec.name[0]);
	    if (err == noErr) {
		/* 
		 * Get the ancestor directory names - loop until we have an 
		 * error or find the root directory.
		 */
		pb.dirInfo.ioNamePtr = tempSpec.name;
		pb.dirInfo.ioVRefNum = tempSpec.vRefNum;
		pb.dirInfo.ioDrParID = tempSpec.parID;
		do {
		    pb.dirInfo.ioFDirIndex = -1;
		    pb.dirInfo.ioDrDirID = pb.dirInfo.ioDrParID;
		    err = PBGetCatInfoSync(&pb);
		    if (err == noErr) {
			/* 
			 * Append colon to directory name and add 
			 * directory name to beginning of fullPath.
			 */
			++tempSpec.name[0];
			tempSpec.name[tempSpec.name[0]] = ':';
						
			(void) Munger(*fullPath, 0, NULL, 0, &tempSpec.name[1],
				tempSpec.name[0]);
			err = MemError();
		    }
		} while ( (err == noErr) &&
			(pb.dirInfo.ioDrDirID != fsRtDirID) );
	    }
	}
    }
    
    /*
     * On error Dispose the handle, set it to NULL & return the err.
     * Otherwise, set the length & return.
     */
    if (err == noErr) {
	*length = GetHandleSize(*fullPath) - 1;
    } else {
	if ( *fullPath != NULL ) {
	    DisposeHandle(*fullPath);
	}
	*fullPath = NULL;
	*length = 0;
    }

    return err;
}

/*
 *----------------------------------------------------------------------
 *
 * GetGlobalMouse --
 *
 *	This procedure obtains the current mouse position in global
 *	coordinates.
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
GetGlobalMouse(
    Point *mouse)		/* Mouse position. */
{
    EventRecord event;
    
    OSEventAvail(0, &event);
    *mouse = event.where;
}
