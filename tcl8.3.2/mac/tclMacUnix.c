/* 
 * tclMacUnix.c --
 *
 *	This file contains routines to implement several features
 *	available to the Unix implementation, but that require
 *      extra work to do on a Macintosh.  These include routines
 *      Unix Tcl normally hands off to the Unix OS.
 *
 * Copyright (c) 1993-1994 Lockheed Missle & Space Company, AI Center
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <Files.h>
#include <Strings.h>
#include <TextUtils.h>
#include <Finder.h>
#include <FSpCompat.h>
#include <Aliases.h>
#include <Errors.h>

#include "tclInt.h"
#include "tclMacInt.h"

/*
 * The following two Includes are from the More Files package
 */
#include "FileCopy.h"
#include "MoreFiles.h"
#include "MoreFilesExtras.h"

/*
 * The following may not be defined in some versions of
 * MPW header files.
 */
#ifndef kIsInvisible
#define kIsInvisible 0x4000
#endif
#ifndef kIsAlias
#define kIsAlias 0x8000
#endif

/*
 * Missing error codes
 */
#define usageErr		500
#define noSourceErr		501
#define isDirErr		502


/*
 *----------------------------------------------------------------------
 *
 * Tcl_EchoCmd --
 *
 *    Implements the TCL echo command:
 *        echo ?str ...?
 *
 * Results:
 *      Always returns TCL_OK.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_EchoCmd(
    ClientData dummy,			/* Not used. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int argc,				/* Number of arguments. */
    char **argv)			/* Argument strings. */
{
    Tcl_Channel chan;
    int mode, result, i;

    chan = Tcl_GetChannel(interp, "stdout", &mode);
    if (chan == (Tcl_Channel) NULL) {
        return TCL_ERROR;
    }
    for (i = 1; i < argc; i++) {
	result = Tcl_WriteChars(chan, argv[i], -1);
	if (result < 0) {
	    Tcl_AppendResult(interp, "echo: ", Tcl_GetChannelName(chan),
		    ": ", Tcl_PosixError(interp), (char *) NULL);
	    return TCL_ERROR;
	}
        if (i < (argc - 1)) {
	    Tcl_WriteChars(chan, " ", -1);
	}
    }
    Tcl_WriteChars(chan, "\n", -1);
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_LsObjCmd --
 *
 *	This procedure is invoked to process the "ls" Tcl command.
 *	See the user documentation for details on what it does.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *----------------------------------------------------------------------
 */
int
Tcl_LsObjCmd(
    ClientData dummy,			/* Not used. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *CONST objv[])		/* Argument strings. */
{
#define STRING_LENGTH 80
#define CR '\n'
    int i, j;
    int fieldLength, len = 0, maxLen = 0, perLine;
    OSErr err;
    CInfoPBRec paramBlock;
    HFileInfo *hpb = (HFileInfo *)&paramBlock;
    DirInfo *dpb = (DirInfo *)&paramBlock;
    char theFile[256];
    char theLine[STRING_LENGTH + 2];
    int fFlag = false, pFlag = false, aFlag = false, lFlag = false,
	cFlag = false, hFlag = false;
    char *argv;
    Tcl_Obj *newObjv[2], *resultObjPtr;

    /*
     * Process command flags.  End if argument doesn't start
     * with a dash or is a dash by itself.  The remaining arguments
     * should be files.
     */
    for (i = 1; i < objc; i++) {
    	argv = Tcl_GetString(objv[i]);
	if (argv[0] != '-') {
	    break;
	}
		
	if (!strcmp(argv, "-")) {
	    i++;
	    break;
	}
		
	for (j = 1 ; argv[j] ; ++j) {
	    switch(argv[j]) {
	    case 'a':
	    case 'A':
		aFlag = true;
		break;
	    case '1':
		cFlag = false;
		break;
	    case 'C':
		cFlag = true;
		break;
	    case 'F':
		fFlag = true;
		break;
	    case 'H':
		hFlag = true;
		break;
	    case 'p':
		pFlag = true;
		break;
	    case 'l':
		pFlag = false;
		lFlag = true;
		break;
	    default:
		Tcl_AppendResult(interp, "error - unknown flag ",
			"usage: ls -apCFHl1 ?files? ", NULL);
		return TCL_ERROR;
	    }
	}
    }

    objv += i;
    objc -= i;

    /*
     * No file specifications means we search for all files.
     * Glob will be doing most of the work.
     */
     if (!objc) {
	objc = 1;
	newObjv[0] = Tcl_NewStringObj("*", -1);
	newObjv[1] = NULL;
	objv = newObjv;
    }

    if (Tcl_GlobObjCmd(NULL, interp, objc + 1, objv - 1) != TCL_OK) {
    	Tcl_ResetResult(interp);
    	return TCL_ERROR;
    }

    resultObjPtr = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(resultObjPtr);
    if (Tcl_ListObjGetElements(interp, resultObjPtr, &objc, &objv) != TCL_OK) {
    	Tcl_DecrRefCount(resultObjPtr);
    	return TCL_ERROR;
    }

    Tcl_ResetResult(interp);

    /*
     * There are two major methods for listing files: the long
     * method and the normal method.
     */
    if (lFlag) {
	char	creator[5], type[5], time[16], date[16];
	char	lineTag;
	long	size;
	unsigned short flags;
	Tcl_Obj *objPtr;
	char *string;
	int length;

	/*
	 * Print the header for long listing.
	 */
	if (hFlag) {
	    sprintf(theLine, "T %7s %8s %8s %4s %4s %6s %s",
		    "Size", "ModTime", "ModDate",
		    "CRTR", "TYPE", "Flags", "Name");
	    Tcl_AppendResult(interp, theLine, "\n", NULL);
	    Tcl_AppendResult(interp,
		    "-------------------------------------------------------------\n",
		    NULL);
	}
		
	for (i = 0; i < objc; i++) {
	    strcpy(theFile, Tcl_GetString(objv[i]));
			
	    c2pstr(theFile);
	    hpb->ioCompletion = NULL;
	    hpb->ioVRefNum = 0;
	    hpb->ioFDirIndex = 0;
	    hpb->ioNamePtr = (StringPtr) theFile;
	    hpb->ioDirID = 0L;
	    err = PBGetCatInfoSync(&paramBlock);
	    p2cstr((StringPtr) theFile);

	    if (hpb->ioFlAttrib & 16) {
		/*
		 * For directories use zero as the size, use no Creator
		 * type, and use 'DIR ' as the file type.
		 */
		if ((aFlag == false) && (dpb->ioDrUsrWds.frFlags & 0x1000)) {
		    continue;
		}
		lineTag = 'D';
		size = 0;
		IUTimeString(dpb->ioDrMdDat, false, (unsigned char *)time);
		p2cstr((StringPtr)time);
		IUDateString(dpb->ioDrMdDat, shortDate, (unsigned char *)date);
		p2cstr((StringPtr)date);
		strcpy(creator, "    ");
		strcpy(type, "DIR ");
		flags = dpb->ioDrUsrWds.frFlags;
		if (fFlag || pFlag) {
		    strcat(theFile, ":");
		}
	    } else {
		/*
		 * All information for files should be printed.  This
		 * includes size, modtime, moddate, creator type, file
		 * type, flags, anf file name.
		 */
		if ((aFlag == false) &&
			(hpb->ioFlFndrInfo.fdFlags & kIsInvisible)) {
		    continue;
		}
		lineTag = 'F';
		size = hpb->ioFlLgLen + hpb->ioFlRLgLen;
		IUTimeString(hpb->ioFlMdDat, false, (unsigned char *)time);
		p2cstr((StringPtr)time);
		IUDateString(hpb->ioFlMdDat, shortDate, (unsigned char *)date);
		p2cstr((StringPtr)date);
		strncpy(creator, (char *) &hpb->ioFlFndrInfo.fdCreator, 4);
		creator[4] = 0;
		strncpy(type, (char *) &hpb->ioFlFndrInfo.fdType, 4);
		type[4] = 0;
		flags = hpb->ioFlFndrInfo.fdFlags;
		if (fFlag) {
		    if (hpb->ioFlFndrInfo.fdFlags & kIsAlias) {
			strcat(theFile, "@");
		    } else if (hpb->ioFlFndrInfo.fdType == 'APPL') {
			strcat(theFile, "*");
		    }
		}
	    }
			
	    sprintf(theLine, "%c %7ld %8s %8s %-4.4s %-4.4s 0x%4.4X %s",
		    lineTag, size, time, date, creator, type, flags, theFile);
						 
	    Tcl_AppendResult(interp, theLine, "\n", NULL);
	    
	}
		
	objPtr = Tcl_GetObjResult(interp);
	string = Tcl_GetStringFromObj(objPtr, &length);
	if ((length > 0) && (string[length - 1] == '\n')) {
	    Tcl_SetObjLength(objPtr, length - 1);
	}
    } else {
	/*
	 * Not in long format. We only print files names.  If the
	 * -C flag is set we need to print in multiple coloumns.
	 */
	int argCount, linePos;
	Boolean needNewLine = false;

	/*
	 * Fiend the field length: the length each string printed
	 * to the terminal will be.
	 */
	if (!cFlag) {
	    perLine = 1;
	    fieldLength = STRING_LENGTH;
	} else {
	    for (i = 0; i < objc; i++) {
	    	argv = Tcl_GetString(objv[i]);
		len = strlen(argv);
		if (len > maxLen) {
		    maxLen = len;
		}
	    }
	    fieldLength = maxLen + 3;
	    perLine = STRING_LENGTH / fieldLength;
	}

	argCount = 0;
	linePos = 0;
	memset(theLine, ' ', STRING_LENGTH);
	while (argCount < objc) {
	    strcpy(theFile, Tcl_GetString(objv[argCount]));
			
	    c2pstr(theFile);
	    hpb->ioCompletion = NULL;
	    hpb->ioVRefNum = 0;
	    hpb->ioFDirIndex = 0;
	    hpb->ioNamePtr = (StringPtr) theFile;
	    hpb->ioDirID = 0L;
	    err = PBGetCatInfoSync(&paramBlock);
	    p2cstr((StringPtr) theFile);

	    if (hpb->ioFlAttrib & 16) {
		/*
		 * Directory. If -a show hidden files.  If -f or -p
		 * denote that this is a directory.
		 */
		if ((aFlag == false) && (dpb->ioDrUsrWds.frFlags & 0x1000)) {
		    argCount++;
		    continue;
		}
		if (fFlag || pFlag) {
		    strcat(theFile, ":");
		}
	    } else {
		/*
		 * File: If -a show hidden files, if -f show links
		 * (aliases) and executables (APPLs).
		 */
		if ((aFlag == false) &&
			(hpb->ioFlFndrInfo.fdFlags & kIsInvisible)) {
		    argCount++;
		    continue;
		}
		if (fFlag) {
		    if (hpb->ioFlFndrInfo.fdFlags & kIsAlias) {
			strcat(theFile, "@");
		    } else if (hpb->ioFlFndrInfo.fdType == 'APPL') {
			strcat(theFile, "*");
		    }
		}
	    }

	    /*
	     * Print the item, taking into account multi-
	     * coloum output.
	     */
	    strncpy(theLine + (linePos * fieldLength), theFile,
		    strlen(theFile));
	    linePos++;
			
	    if (linePos == perLine) {
		theLine[STRING_LENGTH] = '\0';
		if (needNewLine) {
		    Tcl_AppendResult(interp, "\n", theLine, NULL);
		} else {
		    Tcl_AppendResult(interp, theLine, NULL);
		    needNewLine = true;
		}
		linePos = 0;
		memset(theLine, ' ', STRING_LENGTH);
	    }
			
	    argCount++;
	}
		
	if (linePos != 0) {
	    theLine[STRING_LENGTH] = '\0';
	    if (needNewLine) {
		Tcl_AppendResult(interp, "\n", theLine, NULL);
	    } else {
		Tcl_AppendResult(interp, theLine, NULL);
	    }
	}
    }

    Tcl_DecrRefCount(resultObjPtr);
    	
    return TCL_OK;
}
