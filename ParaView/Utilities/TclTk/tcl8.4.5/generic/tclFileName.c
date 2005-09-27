/* 
 * tclFileName.c --
 *
 *	This file contains routines for converting file names betwen
 *	native and network form.
 *
 * Copyright (c) 1995-1998 Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"
#include "tclRegexp.h"

/* 
 * This define is used to activate Tcl's interpretation of Unix-style
 * paths (containing forward slashes, '.' and '..') on MacOS.  A 
 * side-effect of this is that some paths become ambiguous.
 */
#define MAC_UNDERSTANDS_UNIX_PATHS

#ifdef MAC_UNDERSTANDS_UNIX_PATHS
/*
 * The following regular expression matches the root portion of a Macintosh
 * absolute path.  It will match degenerate Unix-style paths, tilde paths,
 * Unix-style paths, and Mac paths.  The various subexpressions in this
 * can be summarised as follows: ^(/..|~user/unix|~user:mac|/unix|mac:dir).
 * The subexpression indices which match the root portions, are as follows:
 * 
 * degenerate unix-style: 2
 * unix-tilde: 5
 * mac-tilde: 7
 * unix-style: 9 (or 10 to cut off the irrelevant header).
 * mac: 12
 * 
 */

#define MAC_ROOT_PATTERN "^((/+([.][.]?/+)*([.][.]?)?)|(~[^:/]*)(/[^:]*)?|(~[^:]*)(:.*)?|/+([.][.]?/+)*([^:/]+)(/[^:]*)?|([^:]+):.*)$"

/*
 * The following variables are used to hold precompiled regular expressions
 * for use in filename matching.
 */

typedef struct ThreadSpecificData {
    int initialized;
    Tcl_Obj *macRootPatternPtr;
} ThreadSpecificData;

static Tcl_ThreadDataKey dataKey;

static void		FileNameCleanup _ANSI_ARGS_((ClientData clientData));
static void		FileNameInit _ANSI_ARGS_((void));

#endif

/*
 * The following variable is set in the TclPlatformInit call to one
 * of: TCL_PLATFORM_UNIX, TCL_PLATFORM_MAC, or TCL_PLATFORM_WINDOWS.
 */

TclPlatformType tclPlatform = TCL_PLATFORM_UNIX;

/*
 * Prototypes for local procedures defined in this file:
 */

static CONST char *	DoTildeSubst _ANSI_ARGS_((Tcl_Interp *interp,
			    CONST char *user, Tcl_DString *resultPtr));
static CONST char *	ExtractWinRoot _ANSI_ARGS_((CONST char *path,
			    Tcl_DString *resultPtr, int offset, 
			    Tcl_PathType *typePtr));
static int		SkipToChar _ANSI_ARGS_((char **stringPtr,
			    char *match));
static Tcl_Obj*		SplitMacPath _ANSI_ARGS_((CONST char *path));
static Tcl_Obj*		SplitWinPath _ANSI_ARGS_((CONST char *path));
static Tcl_Obj*		SplitUnixPath _ANSI_ARGS_((CONST char *path));
#ifdef MAC_UNDERSTANDS_UNIX_PATHS

/*
 *----------------------------------------------------------------------
 *
 * FileNameInit --
 *
 *	This procedure initializes the patterns used by this module.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Compiles the regular expressions.
 *
 *----------------------------------------------------------------------
 */

static void
FileNameInit()
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    if (!tsdPtr->initialized) {
	tsdPtr->initialized = 1;
	tsdPtr->macRootPatternPtr = Tcl_NewStringObj(MAC_ROOT_PATTERN, -1);
	Tcl_CreateThreadExitHandler(FileNameCleanup, NULL);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * FileNameCleanup --
 *
 *	This procedure is a Tcl_ExitProc used to clean up the static
 *	data structures used in this file.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Deallocates storage used by the procedures in this file.
 *
 *----------------------------------------------------------------------
 */

static void
FileNameCleanup(clientData)
    ClientData clientData;	/* Not used. */
{
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
    Tcl_DecrRefCount(tsdPtr->macRootPatternPtr);
    tsdPtr->initialized = 0;
}
#endif

/*
 *----------------------------------------------------------------------
 *
 * ExtractWinRoot --
 *
 *	Matches the root portion of a Windows path and appends it
 *	to the specified Tcl_DString.
 *	
 * Results:
 *	Returns the position in the path immediately after the root
 *	including any trailing slashes.
 *	Appends a cleaned up version of the root to the Tcl_DString
 *	at the specified offest.
 *
 * Side effects:
 *	Modifies the specified Tcl_DString.
 *
 *----------------------------------------------------------------------
 */

static CONST char *
ExtractWinRoot(path, resultPtr, offset, typePtr)
    CONST char *path;		/* Path to parse. */
    Tcl_DString *resultPtr;	/* Buffer to hold result. */
    int offset;			/* Offset in buffer where result should be
				 * stored. */
    Tcl_PathType *typePtr;	/* Where to store pathType result */
{
    if (path[0] == '/' || path[0] == '\\') {
	/* Might be a UNC or Vol-Relative path */
	CONST char *host, *share, *tail;
	int hlen, slen;
	if (path[1] != '/' && path[1] != '\\') {
	    Tcl_DStringSetLength(resultPtr, offset);
	    *typePtr = TCL_PATH_VOLUME_RELATIVE;
	    Tcl_DStringAppend(resultPtr, "/", 1);
	    return &path[1];
	}
	host = &path[2];

	/* Skip separators */
	while (host[0] == '/' || host[0] == '\\') host++;

	for (hlen = 0; host[hlen];hlen++) {
	    if (host[hlen] == '/' || host[hlen] == '\\')
		break;
	}
	if (host[hlen] == 0 || host[hlen+1] == 0) {
	    /* 
	     * The path given is simply of the form 
	     * '/foo', '//foo', '/////foo' or the same
	     * with backslashes.  If there is exactly
	     * one leading '/' the path is volume relative
	     * (see filename man page).  If there are more
	     * than one, we are simply assuming they
	     * are superfluous and we trim them away.
	     * (An alternative interpretation would
	     * be that it is a host name, but we have
	     * been documented that that is not the case).
	     */
	    *typePtr = TCL_PATH_VOLUME_RELATIVE;
	    Tcl_DStringAppend(resultPtr, "/", 1);
	    return &path[2];
	}
	Tcl_DStringSetLength(resultPtr, offset);
	share = &host[hlen];

	/* Skip separators */
	while (share[0] == '/' || share[0] == '\\') share++;

	for (slen = 0; share[slen];slen++) {
	    if (share[slen] == '/' || share[slen] == '\\')
		break;
	}
	Tcl_DStringAppend(resultPtr, "//", 2);
	Tcl_DStringAppend(resultPtr, host, hlen);
	Tcl_DStringAppend(resultPtr, "/", 1);
	Tcl_DStringAppend(resultPtr, share, slen);

	tail = &share[slen];

	/* Skip separators */
	while (tail[0] == '/' || tail[0] == '\\') tail++;

	*typePtr = TCL_PATH_ABSOLUTE;
	return tail;
    } else if (*path && path[1] == ':') {
	/* Might be a drive sep */
	Tcl_DStringSetLength(resultPtr, offset);

	if (path[2] != '/' && path[2] != '\\') {
	    *typePtr = TCL_PATH_VOLUME_RELATIVE;
	    Tcl_DStringAppend(resultPtr, path, 2);
	    return &path[2];
	} else {
	    char *tail = (char*)&path[3];

	    /* Skip separators */
	    while (*tail && (tail[0] == '/' || tail[0] == '\\')) tail++;

	    *typePtr = TCL_PATH_ABSOLUTE;
	    Tcl_DStringAppend(resultPtr, path, 2);
	    Tcl_DStringAppend(resultPtr, "/", 1);

	    return tail;
	}
    } else {
	int abs = 0;
	if (path[0] == 'c' && path[1] == 'o') {
	    if (path[2] == 'm' && path[3] >= '1' && path[3] <= '9') {
		/* May have match for 'com[1-9]:?', which is a serial port */
	        if (path[4] == '\0') {
	            abs = 4;
	        } else if (path [4] == ':' && path[5] == '\0') {
		    abs = 5;
	        }
	    } else if (path[2] == 'n' && path[3] == '\0') {
		/* Have match for 'con' */
		abs = 3;
	    }
	} else if (path[0] == 'l' && path[1] == 'p' && path[2] == 't') {
	    if (path[3] >= '1' && path[3] <= '9') {
		/* May have match for 'lpt[1-9]:?' */
		if (path[4] == '\0') {
		    abs = 4;
		} else if (path [4] == ':' && path[5] == '\0') {
		    abs = 5;
		}
	    }
	} else if (path[0] == 'p' && path[1] == 'r' 
		   && path[2] == 'n' && path[3] == '\0') {
	    /* Have match for 'prn' */
	    abs = 3;
	} else if (path[0] == 'n' && path[1] == 'u' 
		   && path[2] == 'l' && path[3] == '\0') {
	    /* Have match for 'nul' */
	    abs = 3;
	} else if (path[0] == 'a' && path[1] == 'u' 
		   && path[2] == 'x' && path[3] == '\0') {
	    /* Have match for 'aux' */
	    abs = 3;
	}
	if (abs != 0) {
	    *typePtr = TCL_PATH_ABSOLUTE;
	    Tcl_DStringSetLength(resultPtr, offset);
	    Tcl_DStringAppend(resultPtr, path, abs);
	    return path + abs;
	}
    }
    /* Anything else is treated as relative */
    *typePtr = TCL_PATH_RELATIVE;
    return path;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetPathType --
 *
 *	Determines whether a given path is relative to the current
 *	directory, relative to the current volume, or absolute.
 *	
 *	The objectified Tcl_FSGetPathType should be used in
 *	preference to this function (as you can see below, this
 *	is just a wrapper around that other function).
 *
 * Results:
 *	Returns one of TCL_PATH_ABSOLUTE, TCL_PATH_RELATIVE, or
 *	TCL_PATH_VOLUME_RELATIVE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_PathType
Tcl_GetPathType(path)
    CONST char *path;
{
    Tcl_PathType type;
    Tcl_Obj *tempObj = Tcl_NewStringObj(path,-1);
    Tcl_IncrRefCount(tempObj);
    type = Tcl_FSGetPathType(tempObj);
    Tcl_DecrRefCount(tempObj);
    return type;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpGetNativePathType --
 *
 *	Determines whether a given path is relative to the current
 *	directory, relative to the current volume, or absolute, but
 *	ONLY FOR THE NATIVE FILESYSTEM. This function is called from
 *	tclIOUtil.c (but needs to be here due to its dependence on
 *	static variables/functions in this file).  The exported
 *	function Tcl_FSGetPathType should be used by extensions.
 *
 * Results:
 *	Returns one of TCL_PATH_ABSOLUTE, TCL_PATH_RELATIVE, or
 *	TCL_PATH_VOLUME_RELATIVE.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_PathType
TclpGetNativePathType(pathObjPtr, driveNameLengthPtr, driveNameRef)
    Tcl_Obj *pathObjPtr;
    int *driveNameLengthPtr;
    Tcl_Obj **driveNameRef;
{
    Tcl_PathType type = TCL_PATH_ABSOLUTE;
    int pathLen;
    char *path = Tcl_GetStringFromObj(pathObjPtr, &pathLen);
    
    if (path[0] == '~') {
	/* 
	 * This case is common to all platforms.
	 * Paths that begin with ~ are absolute.
	 */
	if (driveNameLengthPtr != NULL) {
	    char *end = path + 1;
	    while ((*end != '\0') && (*end != '/')) {
		end++;
	    }
	    *driveNameLengthPtr = end - path;
	}
    } else {
	switch (tclPlatform) {
	    case TCL_PLATFORM_UNIX: {
		char *origPath = path;
	        
		/*
		 * Paths that begin with / are absolute.
		 */

#ifdef __QNX__
		/*
		 * Check for QNX //<node id> prefix
		 */
		if (*path && (pathLen > 3) && (path[0] == '/') 
		  && (path[1] == '/') && isdigit(UCHAR(path[2]))) {
		    path += 3;
		    while (isdigit(UCHAR(*path))) {
			++path;
		    }
		}
#endif
		if (path[0] == '/') {
		    if (driveNameLengthPtr != NULL) {
			/* 
			 * We need this addition in case the QNX code 
			 * was used 
			 */
			*driveNameLengthPtr = (1 + path - origPath);
		    }
		} else {
		    type = TCL_PATH_RELATIVE;
		}
		break;
	    }
	    case TCL_PLATFORM_MAC:
		if (path[0] == ':') {
		    type = TCL_PATH_RELATIVE;
		} else {
#ifdef MAC_UNDERSTANDS_UNIX_PATHS
		    ThreadSpecificData *tsdPtr;
		    Tcl_RegExp re;

		    tsdPtr = TCL_TSD_INIT(&dataKey);

		    /*
		     * Since we have eliminated the easy cases, use the
		     * root pattern to look for the other types.
		     */

		    FileNameInit();
		    re = Tcl_GetRegExpFromObj(NULL, tsdPtr->macRootPatternPtr,
			    REG_ADVANCED);

		    if (!Tcl_RegExpExec(NULL, re, path, path)) {
			type = TCL_PATH_RELATIVE;
		    } else {
			CONST char *root, *end;
			Tcl_RegExpRange(re, 2, &root, &end);
			if (root != NULL) {
			    type = TCL_PATH_RELATIVE;
			} else {
			    if (driveNameLengthPtr != NULL) {
				Tcl_RegExpRange(re, 0, &root, &end);
				*driveNameLengthPtr = end - root;
			    }
			    if (driveNameRef != NULL) {
				if (*root == '/') {
				    char *c;
				    int gotColon = 0;
				    *driveNameRef = Tcl_NewStringObj(root + 1,
					    end - root -1);
				    c = Tcl_GetString(*driveNameRef);
				    while (*c != '\0') {
					if (*c == '/') {
					    gotColon++;
					    *c = ':';
					}
					c++;
				    }
				    /* 
				     * If there is no colon, we have just a
				     * volume name so we must add a colon so
				     * it is an absolute path.
				     */
				    if (gotColon == 0) {
				        Tcl_AppendToObj(*driveNameRef, ":", 1);
				    } else if ((gotColon > 1) &&
					    (*(c-1) == ':')) {
					/* We have an extra colon */
				        Tcl_SetObjLength(*driveNameRef, 
					  c - Tcl_GetString(*driveNameRef) - 1);
				    }
				}
			    }
			}
		    }
#else
		    if (path[0] == '~') {
		    } else if (path[0] == ':') {
			type = TCL_PATH_RELATIVE;
		    } else {
			char *colonPos = strchr(path,':');
			if (colonPos == NULL) {
			    type = TCL_PATH_RELATIVE;
			} else {
			}
		    }
		    if (type == TCL_PATH_ABSOLUTE) {
			if (driveNameLengthPtr != NULL) {
			    *driveNameLengthPtr = strlen(path);
			}
		    }
#endif
		}
		break;
	    
	    case TCL_PLATFORM_WINDOWS: {
		Tcl_DString ds;
		CONST char *rootEnd;
		
		Tcl_DStringInit(&ds);
		rootEnd = ExtractWinRoot(path, &ds, 0, &type);
		if ((rootEnd != path) && (driveNameLengthPtr != NULL)) {
		    *driveNameLengthPtr = rootEnd - path;
		    if (driveNameRef != NULL) {
			*driveNameRef = Tcl_NewStringObj(Tcl_DStringValue(&ds),
				Tcl_DStringLength(&ds));
			Tcl_IncrRefCount(*driveNameRef);
		    }
		}
		Tcl_DStringFree(&ds);
		break;
	    }
	}
    }
    return type;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpNativeSplitPath --
 *
 *      This function takes the given Tcl_Obj, which should be a valid
 *      path, and returns a Tcl List object containing each segment
 *      of that path as an element.
 *
 *      Note this function currently calls the older Split(Plat)Path
 *      functions, which require more memory allocation than is
 *      desirable.
 *      
 * Results:
 *      Returns list object with refCount of zero.  If the passed in
 *      lenPtr is non-NULL, we use it to return the number of elements
 *      in the returned list.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Obj* 
TclpNativeSplitPath(pathPtr, lenPtr)
    Tcl_Obj *pathPtr;		/* Path to split. */
    int *lenPtr;		/* int to store number of path elements. */
{
    Tcl_Obj *resultPtr = NULL;  /* Needed only to prevent gcc warnings. */

    /*
     * Perform platform specific splitting. 
     */

    switch (tclPlatform) {
	case TCL_PLATFORM_UNIX:
	    resultPtr = SplitUnixPath(Tcl_GetString(pathPtr));
	    break;

	case TCL_PLATFORM_WINDOWS:
	    resultPtr = SplitWinPath(Tcl_GetString(pathPtr));
	    break;
	    
	case TCL_PLATFORM_MAC:
	    resultPtr = SplitMacPath(Tcl_GetString(pathPtr));
	    break;
    }

    /*
     * Compute the number of elements in the result.
     */

    if (lenPtr != NULL) {
	Tcl_ListObjLength(NULL, resultPtr, lenPtr);
    }
    return resultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SplitPath --
 *
 *	Split a path into a list of path components.  The first element
 *	of the list will have the same path type as the original path.
 *
 * Results:
 *	Returns a standard Tcl result.  The interpreter result contains
 *	a list of path components.
 *	*argvPtr will be filled in with the address of an array
 *	whose elements point to the elements of path, in order.
 *	*argcPtr will get filled in with the number of valid elements
 *	in the array.  A single block of memory is dynamically allocated
 *	to hold both the argv array and a copy of the path elements.
 *	The caller must eventually free this memory by calling ckfree()
 *	on *argvPtr.  Note:  *argvPtr and *argcPtr are only modified
 *	if the procedure returns normally.
 *
 * Side effects:
 *	Allocates memory.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SplitPath(path, argcPtr, argvPtr)
    CONST char *path;		/* Pointer to string containing a path. */
    int *argcPtr;		/* Pointer to location to fill in with
				 * the number of elements in the path. */
    CONST char ***argvPtr;	/* Pointer to place to store pointer to array
				 * of pointers to path elements. */
{
    Tcl_Obj *resultPtr = NULL;  /* Needed only to prevent gcc warnings. */
    Tcl_Obj *tmpPtr, *eltPtr;
    int i, size, len;
    char *p, *str;

    /*
     * Perform the splitting, using objectified, vfs-aware code.
     */

    tmpPtr = Tcl_NewStringObj(path, -1);
    Tcl_IncrRefCount(tmpPtr);
    resultPtr = Tcl_FSSplitPath(tmpPtr, argcPtr);
    Tcl_DecrRefCount(tmpPtr);

    /* Calculate space required for the result */
    
    size = 1;
    for (i = 0; i < *argcPtr; i++) {
	Tcl_ListObjIndex(NULL, resultPtr, i, &eltPtr);
	Tcl_GetStringFromObj(eltPtr, &len);
	size += len + 1;
    }
    
    /*
     * Allocate a buffer large enough to hold the contents of all of
     * the list plus the argv pointers and the terminating NULL pointer.
     */

    *argvPtr = (CONST char **) ckalloc((unsigned)
	    ((((*argcPtr) + 1) * sizeof(char *)) + size));

    /*
     * Position p after the last argv pointer and copy the contents of
     * the list in, piece by piece.
     */

    p = (char *) &(*argvPtr)[(*argcPtr) + 1];
    for (i = 0; i < *argcPtr; i++) {
	Tcl_ListObjIndex(NULL, resultPtr, i, &eltPtr);
	str = Tcl_GetStringFromObj(eltPtr, &len);
	memcpy((VOID *) p, (VOID *) str, (size_t) len+1);
	p += len+1;
    }
    
    /*
     * Now set up the argv pointers.
     */

    p = (char *) &(*argvPtr)[(*argcPtr) + 1];

    for (i = 0; i < *argcPtr; i++) {
	(*argvPtr)[i] = p;
	while ((*p++) != '\0') {}
    }
    (*argvPtr)[i] = NULL;

    /*
     * Free the result ptr given to us by Tcl_FSSplitPath
     */

    Tcl_DecrRefCount(resultPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * SplitUnixPath --
 *
 *	This routine is used by Tcl_(FS)SplitPath to handle splitting
 *	Unix paths.
 *
 * Results:
 *	Returns a newly allocated Tcl list object.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj*
SplitUnixPath(path)
    CONST char *path;		/* Pointer to string containing a path. */
{
    int length;
    CONST char *p, *elementStart;
    Tcl_Obj *result = Tcl_NewObj();

    /*
     * Deal with the root directory as a special case.
     */

#ifdef __QNX__
    /*
     * Check for QNX //<node id> prefix
     */
    if ((path[0] == '/') && (path[1] == '/')
	    && isdigit(UCHAR(path[2]))) { /* INTL: digit */
	path += 3;
	while (isdigit(UCHAR(*path))) { /* INTL: digit */
	    ++path;
	}
    }
#endif

    if (path[0] == '/') {
	Tcl_ListObjAppendElement(NULL, result, Tcl_NewStringObj("/",1));
	p = path+1;
    } else {
	p = path;
    }

    /*
     * Split on slashes.  Embedded elements that start with tilde will be
     * prefixed with "./" so they are not affected by tilde substitution.
     */

    for (;;) {
	elementStart = p;
	while ((*p != '\0') && (*p != '/')) {
	    p++;
	}
	length = p - elementStart;
	if (length > 0) {
	    Tcl_Obj *nextElt;
	    if ((elementStart[0] == '~') && (elementStart != path)) {
		nextElt = Tcl_NewStringObj("./",2);
		Tcl_AppendToObj(nextElt, elementStart, length);
	    } else {
		nextElt = Tcl_NewStringObj(elementStart, length);
	    }
	    Tcl_ListObjAppendElement(NULL, result, nextElt);
	}
	if (*p++ == '\0') {
	    break;
	}
    }
    return result;
}


/*
 *----------------------------------------------------------------------
 *
 * SplitWinPath --
 *
 *	This routine is used by Tcl_(FS)SplitPath to handle splitting
 *	Windows paths.
 *
 * Results:
 *	Returns a newly allocated Tcl list object.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj*
SplitWinPath(path)
    CONST char *path;		/* Pointer to string containing a path. */
{
    int length;
    CONST char *p, *elementStart;
    Tcl_PathType type = TCL_PATH_ABSOLUTE;
    Tcl_DString buf;
    Tcl_Obj *result = Tcl_NewObj();
    Tcl_DStringInit(&buf);
    
    p = ExtractWinRoot(path, &buf, 0, &type);

    /*
     * Terminate the root portion, if we matched something.
     */

    if (p != path) {
	Tcl_ListObjAppendElement(NULL, result, 
				 Tcl_NewStringObj(Tcl_DStringValue(&buf), 
						  Tcl_DStringLength(&buf)));
    }
    Tcl_DStringFree(&buf);
    
    /*
     * Split on slashes.  Embedded elements that start with tilde will be
     * prefixed with "./" so they are not affected by tilde substitution.
     */

    do {
	elementStart = p;
	while ((*p != '\0') && (*p != '/') && (*p != '\\')) {
	    p++;
	}
	length = p - elementStart;
	if (length > 0) {
	    Tcl_Obj *nextElt;
	    if ((elementStart[0] == '~') && (elementStart != path)) {
		nextElt = Tcl_NewStringObj("./",2);
		Tcl_AppendToObj(nextElt, elementStart, length);
	    } else {
		nextElt = Tcl_NewStringObj(elementStart, length);
	    }
	    Tcl_ListObjAppendElement(NULL, result, nextElt);
	}
    } while (*p++ != '\0');

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * SplitMacPath --
 *
 *	This routine is used by Tcl_(FS)SplitPath to handle splitting
 *	Macintosh paths.
 *
 * Results:
 *	Returns a newly allocated Tcl list object.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj*
SplitMacPath(path)
    CONST char *path;		/* Pointer to string containing a path. */
{
    int isMac = 0;		/* 1 if is Mac-style, 0 if Unix-style path. */
    int length;
    CONST char *p, *elementStart;
    Tcl_Obj *result;
#ifdef MAC_UNDERSTANDS_UNIX_PATHS
    Tcl_RegExp re;
    int i;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);
#endif
    
    result = Tcl_NewObj();
    
#ifdef MAC_UNDERSTANDS_UNIX_PATHS
    /*
     * Initialize the path name parser for Macintosh path names.
     */

    FileNameInit();

    /*
     * Match the root portion of a Mac path name.
     */

    i = 0;			/* Needed only to prevent gcc warnings. */

    re = Tcl_GetRegExpFromObj(NULL, tsdPtr->macRootPatternPtr, REG_ADVANCED);

    if (Tcl_RegExpExec(NULL, re, path, path) == 1) {
	CONST char *start, *end;
	Tcl_Obj *nextElt;

	/*
	 * Treat degenerate absolute paths like / and /../.. as
	 * Mac relative file names for lack of anything else to do.
	 */

	Tcl_RegExpRange(re, 2, &start, &end);
	if (start) {
	    Tcl_Obj *elt = Tcl_NewStringObj(":", 1);
	    Tcl_RegExpRange(re, 0, &start, &end);
	    Tcl_AppendToObj(elt, path, end - start);
	    Tcl_ListObjAppendElement(NULL, result, elt);
	    return result;
	}

	Tcl_RegExpRange(re, 5, &start, &end);
	if (start) {
	    /*
	     * Unix-style tilde prefixed paths.
	     */

	    isMac = 0;
	    i = 5;
	} else {
	    Tcl_RegExpRange(re, 7, &start, &end);
	    if (start) {
		/*
		 * Mac-style tilde prefixed paths.
		 */

		isMac = 1;
		i = 7;
	    } else {
		Tcl_RegExpRange(re, 10, &start, &end);
		if (start) {
		    /*
		     * Normal Unix style paths.
		     */

		    isMac = 0;
		    i = 10;
		} else {
		    Tcl_RegExpRange(re, 12, &start, &end);
		    if (start) {
			/*
			 * Normal Mac style paths.
			 */

			isMac = 1;
			i = 12;
		    }
		}
	    }
	}
	Tcl_RegExpRange(re, i, &start, &end);
	length = end - start;

	/*
	 * Append the element and terminate it with a : 
	 */

	nextElt = Tcl_NewStringObj(start, length);
	Tcl_AppendToObj(nextElt, ":", 1);
	Tcl_ListObjAppendElement(NULL, result, nextElt);
	p = end;
    } else {
	isMac = (strchr(path, ':') != NULL);
	p = path;
    }
#else
    if ((path[0] != ':') && (path[0] == '~' || (strchr(path,':') != NULL))) {
	CONST char *end;
	Tcl_Obj *nextElt;

	isMac = 1;
	
	end = strchr(path,':');
	if (end == NULL) {
	    length = strlen(path);
	} else {
	    length = end - path;
	}

	/*
	 * Append the element and terminate it with a :
	 */

	nextElt = Tcl_NewStringObj(path, length);
	Tcl_AppendToObj(nextElt, ":", 1);
	Tcl_ListObjAppendElement(NULL, result, nextElt);
	p = path + length;
    } else {
	isMac = (strchr(path, ':') != NULL);
	isMac = 1;
	p = path;
    }
#endif
    
    if (isMac) {

	/*
	 * p is pointing at the first colon in the path.  There
	 * will always be one, since this is a Mac-style path.
	 * (This is no longer true if MAC_UNDERSTANDS_UNIX_PATHS 
	 * is false, so we must check whether 'p' points to the
	 * end of the string.)
	 */
	elementStart = p;
	if (*p == ':') {
	    p++;
	}
	
	while ((p = strchr(p, ':')) != NULL) {
	    length = p - elementStart;
	    if (length == 1) {
		while (*p == ':') {
		    Tcl_ListObjAppendElement(NULL, result,
			    Tcl_NewStringObj("::", 2));
		    elementStart = p++;
		}
	    } else {
		/*
		 * If this is a simple component, drop the leading colon.
		 */

		if ((elementStart[1] != '~')
			&& (strchr(elementStart+1, '/') == NULL)) {
		    elementStart++;
		    length--;
		}
		Tcl_ListObjAppendElement(NULL, result, 
			Tcl_NewStringObj(elementStart, length));
		elementStart = p++;
	    }
	}
	if (elementStart[0] != ':') {
	    if (elementStart[0] != '\0') {
		Tcl_ListObjAppendElement(NULL, result, 
			Tcl_NewStringObj(elementStart, -1));
	    }
	} else {
	    if (elementStart[1] != '\0' || elementStart == path) {
		if ((elementStart[1] != '~') && (elementStart[1] != '\0')
			&& (strchr(elementStart+1, '/') == NULL)) {
		    elementStart++;
		}
		Tcl_ListObjAppendElement(NULL, result, 
			Tcl_NewStringObj(elementStart, -1));
	    }
	}
    } else {

	/*
	 * Split on slashes, suppress extra /'s, and convert .. to ::. 
	 */

	for (;;) {
	    elementStart = p;
	    while ((*p != '\0') && (*p != '/')) {
		p++;
	    }
	    length = p - elementStart;
	    if (length > 0) {
		if ((length == 1) && (elementStart[0] == '.')) {
		    Tcl_ListObjAppendElement(NULL, result, 
					     Tcl_NewStringObj(":", 1));
		} else if ((length == 2) && (elementStart[0] == '.')
			&& (elementStart[1] == '.')) {
		    Tcl_ListObjAppendElement(NULL, result, 
					     Tcl_NewStringObj("::", 2));
		} else {
		    Tcl_Obj *nextElt;
		    if (*elementStart == '~') {
			nextElt = Tcl_NewStringObj(":",1);
			Tcl_AppendToObj(nextElt, elementStart, length);
		    } else {
			nextElt = Tcl_NewStringObj(elementStart, length);
		    }
		    Tcl_ListObjAppendElement(NULL, result, nextElt);
		}
	    }
	    if (*p++ == '\0') {
		break;
	    }
	}
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_FSJoinToPath --
 *
 *      This function takes the given object, which should usually be a
 *      valid path or NULL, and joins onto it the array of paths
 *      segments given.
 *
 * Results:
 *      Returns object with refCount of zero
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Obj* 
Tcl_FSJoinToPath(basePtr, objc, objv)
    Tcl_Obj *basePtr;
    int objc;
    Tcl_Obj *CONST objv[];
{
    int i;
    Tcl_Obj *lobj, *ret;

    if (basePtr == NULL) {
	lobj = Tcl_NewListObj(0, NULL);
    } else {
	lobj = Tcl_NewListObj(1, &basePtr);
    }
    
    for (i = 0; i<objc;i++) {
	Tcl_ListObjAppendElement(NULL, lobj, objv[i]);
    }
    ret = Tcl_FSJoinPath(lobj, -1);
    Tcl_DecrRefCount(lobj);
    return ret;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpNativeJoinPath --
 *
 *      'prefix' is absolute, 'joining' is relative to prefix.
 *
 * Results:
 *      modifies prefix
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

void
TclpNativeJoinPath(prefix, joining)
    Tcl_Obj *prefix;
    char* joining;
{
    int length, needsSep;
    char *dest, *p, *start;
    
    start = Tcl_GetStringFromObj(prefix, &length);

    /*
     * Remove the ./ from tilde prefixed elements unless
     * it is the first component.
     */
    
    p = joining;
    
    if (length != 0) {
	if ((p[0] == '.') && (p[1] == '/') && (p[2] == '~')) {
	    p += 2;
	}
    }
       
    if (*p == '\0') {
	return;
    }


    switch (tclPlatform) {
        case TCL_PLATFORM_UNIX:
	    /*
	     * Append a separator if needed.
	     */

	    if (length > 0 && (start[length-1] != '/')) {
		Tcl_AppendToObj(prefix, "/", 1);
		length++;
	    }
	    needsSep = 0;
	    
	    /*
	     * Append the element, eliminating duplicate and trailing
	     * slashes.
	     */

	    Tcl_SetObjLength(prefix, length + (int) strlen(p));
	    
	    dest = Tcl_GetString(prefix) + length;
	    for (; *p != '\0'; p++) {
		if (*p == '/') {
		    while (p[1] == '/') {
			p++;
		    }
		    if (p[1] != '\0') {
			if (needsSep) {
			    *dest++ = '/';
			}
		    }
		} else {
		    *dest++ = *p;
		    needsSep = 1;
		}
	    }
	    length = dest - Tcl_GetString(prefix);
	    Tcl_SetObjLength(prefix, length);
	    break;

	case TCL_PLATFORM_WINDOWS:
	    /*
	     * Check to see if we need to append a separator.
	     */

	    if ((length > 0) && 
		(start[length-1] != '/') && (start[length-1] != ':')) {
		Tcl_AppendToObj(prefix, "/", 1);
		length++;
	    }
	    needsSep = 0;
	    
	    /*
	     * Append the element, eliminating duplicate and
	     * trailing slashes.
	     */

	    Tcl_SetObjLength(prefix, length + (int) strlen(p));
	    dest = Tcl_GetString(prefix) + length;
	    for (; *p != '\0'; p++) {
		if ((*p == '/') || (*p == '\\')) {
		    while ((p[1] == '/') || (p[1] == '\\')) {
			p++;
		    }
		    if ((p[1] != '\0') && needsSep) {
			*dest++ = '/';
		    }
		} else {
		    *dest++ = *p;
		    needsSep = 1;
		}
	    }
	    length = dest - Tcl_GetString(prefix);
	    Tcl_SetObjLength(prefix, length);
	    break;

	case TCL_PLATFORM_MAC: {
	    int newLength;
	    
	    /*
	     * Sort out separators.  We basically add the object we've
	     * been given, but we have to make sure that there is
	     * exactly one separator inbetween (unless the object we're
	     * adding contains multiple contiguous colons, all of which
	     * we must add).  Also if an object is just ':' we don't
	     * bother to add it unless it's the very first element.
	     */

#ifdef MAC_UNDERSTANDS_UNIX_PATHS
	    int adjustedPath = 0;
	    if ((strchr(p, ':') == NULL) && (strchr(p, '/') != NULL)) {
		char *start = p;
		adjustedPath = 1;
		while (*start != '\0') {
		    if (*start == '/') {
		        *start = ':';
		    }
		    start++;
		}
	    }
#endif
	    if (length > 0) {
		if ((p[0] == ':') && (p[1] == '\0')) {
		    return;
		}
		if (start[length-1] != ':') {
		    if (*p != '\0' && *p != ':') {
			Tcl_AppendToObj(prefix, ":", 1);
			length++;
		    }
		} else if (*p == ':') {
		    p++;
		}
	    } else {
		if (*p != '\0' && *p != ':') {
		    Tcl_AppendToObj(prefix, ":", 1);
		    length++;
		}
	    }
	    
	    /*
	     * Append the element
	     */

	    newLength = strlen(p);
	    /* 
	     * It may not be good to just do 'Tcl_AppendToObj(prefix,
	     * p, newLength)' because the object may contain duplicate
	     * colons which we want to get rid of.
	     */
	    Tcl_AppendToObj(prefix, p, newLength);
	    
	    /* Remove spurious trailing single ':' */
	    dest = Tcl_GetString(prefix) + length + newLength;
	    if (*(dest-1) == ':') {
		if (dest-1 > Tcl_GetString(prefix)) {
		    if (*(dest-2) != ':') {
		        Tcl_SetObjLength(prefix, length + newLength -1);
		    }
		}
	    }
#ifdef MAC_UNDERSTANDS_UNIX_PATHS
	    /* Revert the path to what it was */
	    if (adjustedPath) {
		char *start = joining;
		while (*start != '\0') {
		    if (*start == ':') {
			*start = '/';
		    }
		    start++;
		}
	    }
#endif
	    break;
	}
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_JoinPath --
 *
 *	Combine a list of paths in a platform specific manner.  The
 *	function 'Tcl_FSJoinPath' should be used in preference where
 *	possible.
 *
 * Results:
 *	Appends the joined path to the end of the specified 
 *	Tcl_DString returning a pointer to the resulting string.  Note
 *	that the Tcl_DString must already be initialized.
 *
 * Side effects:
 *	Modifies the Tcl_DString.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_JoinPath(argc, argv, resultPtr)
    int argc;
    CONST char * CONST *argv;
    Tcl_DString *resultPtr;	/* Pointer to previously initialized DString */
{
    int i, len;
    Tcl_Obj *listObj = Tcl_NewObj();
    Tcl_Obj *resultObj;
    char *resultStr;

    /* Build the list of paths */
    for (i = 0; i < argc; i++) {
        Tcl_ListObjAppendElement(NULL, listObj,
		Tcl_NewStringObj(argv[i], -1));
    }

    /* Ask the objectified code to join the paths */
    Tcl_IncrRefCount(listObj);
    resultObj = Tcl_FSJoinPath(listObj, argc);
    Tcl_IncrRefCount(resultObj);
    Tcl_DecrRefCount(listObj);

    /* Store the result */
    resultStr = Tcl_GetStringFromObj(resultObj, &len);
    Tcl_DStringAppend(resultPtr, resultStr, len);
    Tcl_DecrRefCount(resultObj);

    /* Return a pointer to the result */
    return Tcl_DStringValue(resultPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_TranslateFileName --
 *
 *	Converts a file name into a form usable by the native system
 *	interfaces.  If the name starts with a tilde, it will produce a
 *	name where the tilde and following characters have been replaced
 *	by the home directory location for the named user.
 *
 * Results:
 *	The return value is a pointer to a string containing the name
 *	after tilde substitution.  If there was no tilde substitution,
 *	the return value is a pointer to a copy of the original string.
 *	If there was an error in processing the name, then an error
 *	message is left in the interp's result (if interp was not NULL)
 *	and the return value is NULL.  Space for the return value is
 *	allocated in bufferPtr; the caller must call Tcl_DStringFree()
 *	to free the space if the return value was not NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_TranslateFileName(interp, name, bufferPtr)
    Tcl_Interp *interp;		/* Interpreter in which to store error
				 * message (if necessary). */
    CONST char *name;		/* File name, which may begin with "~" (to
				 * indicate current user's home directory) or
				 * "~<user>" (to indicate any user's home
				 * directory). */
    Tcl_DString *bufferPtr;	/* Uninitialized or free DString filled
				 * with name after tilde substitution. */
{
    Tcl_Obj *path = Tcl_NewStringObj(name, -1);
    Tcl_Obj *transPtr;

    Tcl_IncrRefCount(path);
    transPtr = Tcl_FSGetTranslatedPath(interp, path);
    if (transPtr == NULL) {
	Tcl_DecrRefCount(path);
	return NULL;
    }
    
    Tcl_DStringInit(bufferPtr);
    Tcl_DStringAppend(bufferPtr, Tcl_GetString(transPtr), -1);
    Tcl_DecrRefCount(path);
    Tcl_DecrRefCount(transPtr);
    
    /*
     * Convert forward slashes to backslashes in Windows paths because
     * some system interfaces don't accept forward slashes.
     */

    if (tclPlatform == TCL_PLATFORM_WINDOWS) {
	register char *p;
	for (p = Tcl_DStringValue(bufferPtr); *p != '\0'; p++) {
	    if (*p == '/') {
		*p = '\\';
	    }
	}
    }
    return Tcl_DStringValue(bufferPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclGetExtension --
 *
 *	This function returns a pointer to the beginning of the
 *	extension part of a file name.
 *
 * Results:
 *	Returns a pointer into name which indicates where the extension
 *	starts.  If there is no extension, returns NULL.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
TclGetExtension(name)
    char *name;			/* File name to parse. */
{
    char *p, *lastSep;

    /*
     * First find the last directory separator.
     */

    lastSep = NULL;		/* Needed only to prevent gcc warnings. */
    switch (tclPlatform) {
	case TCL_PLATFORM_UNIX:
	    lastSep = strrchr(name, '/');
	    break;

	case TCL_PLATFORM_MAC:
#ifdef MAC_UNDERSTANDS_UNIX_PATHS
	    if (strchr(name, ':') == NULL) {
		lastSep = strrchr(name, '/');
	    } else {
		lastSep = strrchr(name, ':');
	    }
#else
	    lastSep = strrchr(name, ':');
#endif
	    break;

	case TCL_PLATFORM_WINDOWS:
	    lastSep = NULL;
	    for (p = name; *p != '\0'; p++) {
		if (strchr("/\\:", *p) != NULL) {
		    lastSep = p;
		}
	    }
	    break;
    }
    p = strrchr(name, '.');
    if ((p != NULL) && (lastSep != NULL) && (lastSep > p)) {
	p = NULL;
    }

    /*
     * In earlier versions, we used to back up to the first period in a series
     * so that "foo..o" would be split into "foo" and "..o".  This is a
     * confusing and usually incorrect behavior, so now we split at the last
     * period in the name.
     */

    return p;
}

/*
 *----------------------------------------------------------------------
 *
 * DoTildeSubst --
 *
 *	Given a string following a tilde, this routine returns the
 *	corresponding home directory.
 *
 * Results:
 *	The result is a pointer to a static string containing the home
 *	directory in native format.  If there was an error in processing
 *	the substitution, then an error message is left in the interp's
 *	result and the return value is NULL.  On success, the results
 *	are appended to resultPtr, and the contents of resultPtr are
 *	returned.
 *
 * Side effects:
 *	Information may be left in resultPtr.
 *
 *----------------------------------------------------------------------
 */

static CONST char *
DoTildeSubst(interp, user, resultPtr)
    Tcl_Interp *interp;		/* Interpreter in which to store error
				 * message (if necessary). */
    CONST char *user;		/* Name of user whose home directory should be
				 * substituted, or "" for current user. */
    Tcl_DString *resultPtr;	/* Initialized DString filled with name
				 * after tilde substitution. */
{
    CONST char *dir;

    if (*user == '\0') {
	Tcl_DString dirString;
	
	dir = TclGetEnv("HOME", &dirString);
	if (dir == NULL) {
	    if (interp) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "couldn't find HOME environment ",
			"variable to expand path", (char *) NULL);
	    }
	    return NULL;
	}
	Tcl_JoinPath(1, &dir, resultPtr);
	Tcl_DStringFree(&dirString);
    } else {
	if (TclpGetUserHome(user, resultPtr) == NULL) {	
	    if (interp) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "user \"", user, "\" doesn't exist",
			(char *) NULL);
	    }
	    return NULL;
	}
    }
    return Tcl_DStringValue(resultPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GlobObjCmd --
 *
 *	This procedure is invoked to process the "glob" Tcl command.
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

	/* ARGSUSED */
int
Tcl_GlobObjCmd(dummy, interp, objc, objv)
    ClientData dummy;			/* Not used. */
    Tcl_Interp *interp;			/* Current interpreter. */
    int objc;				/* Number of arguments. */
    Tcl_Obj *CONST objv[];		/* Argument objects. */
{
    int index, i, globFlags, length, join, dir, result;
    char *string, *separators;
    Tcl_Obj *typePtr, *resultPtr, *look;
    Tcl_Obj *pathOrDir = NULL;
    Tcl_DString prefix;
    static CONST char *options[] = {
	"-directory", "-join", "-nocomplain", "-path", "-tails", 
	"-types", "--", NULL
    };
    enum options {
	GLOB_DIR, GLOB_JOIN, GLOB_NOCOMPLAIN, GLOB_PATH, GLOB_TAILS, 
	GLOB_TYPE, GLOB_LAST
    };
    enum pathDirOptions {PATH_NONE = -1 , PATH_GENERAL = 0, PATH_DIR = 1};
    Tcl_GlobTypeData *globTypes = NULL;

    globFlags = 0;
    join = 0;
    dir = PATH_NONE;
    typePtr = NULL;
    resultPtr = Tcl_GetObjResult(interp);
    for (i = 1; i < objc; i++) {
	if (Tcl_GetIndexFromObj(interp, objv[i], options, "option", 0, &index)
		!= TCL_OK) {
	    string = Tcl_GetStringFromObj(objv[i], &length);
	    if (string[0] == '-') {
		/*
		 * It looks like the command contains an option so signal
		 * an error
		 */
		return TCL_ERROR;
	    } else {
		/*
		 * This clearly isn't an option; assume it's the first
		 * glob pattern.  We must clear the error
		 */
		Tcl_ResetResult(interp);
		break;
	    }
	}
	switch (index) {
	    case GLOB_NOCOMPLAIN:			/* -nocomplain */
	        globFlags |= TCL_GLOBMODE_NO_COMPLAIN;
		break;
	    case GLOB_DIR:				/* -dir */
		if (i == (objc-1)) {
		    Tcl_AppendToObj(resultPtr,
			    "missing argument to \"-directory\"", -1);
		    return TCL_ERROR;
		}
		if (dir != PATH_NONE) {
		    Tcl_AppendToObj(resultPtr,
			    "\"-directory\" cannot be used with \"-path\"",
			    -1);
		    return TCL_ERROR;
		}
		dir = PATH_DIR;
		globFlags |= TCL_GLOBMODE_DIR;
		pathOrDir = objv[i+1];
		i++;
		break;
	    case GLOB_JOIN:				/* -join */
		join = 1;
		break;
	    case GLOB_TAILS:				/* -tails */
	        globFlags |= TCL_GLOBMODE_TAILS;
		break;
	    case GLOB_PATH:				/* -path */
	        if (i == (objc-1)) {
		    Tcl_AppendToObj(resultPtr,
			    "missing argument to \"-path\"", -1);
		    return TCL_ERROR;
		}
		if (dir != PATH_NONE) {
		    Tcl_AppendToObj(resultPtr,
			    "\"-path\" cannot be used with \"-directory\"",
			    -1);
		    return TCL_ERROR;
		}
		dir = PATH_GENERAL;
		pathOrDir = objv[i+1];
		i++;
		break;
	    case GLOB_TYPE:				/* -types */
	        if (i == (objc-1)) {
		    Tcl_AppendToObj(resultPtr,
			    "missing argument to \"-types\"", -1);
		    return TCL_ERROR;
		}
		typePtr = objv[i+1];
		if (Tcl_ListObjLength(interp, typePtr, &length) != TCL_OK) {
		    return TCL_ERROR;
		}
		i++;
		break;
	    case GLOB_LAST:				/* -- */
	        i++;
		goto endOfForLoop;
	}
    }
    endOfForLoop:
    if (objc - i < 1) {
        Tcl_WrongNumArgs(interp, 1, objv, "?switches? name ?name ...?");
	return TCL_ERROR;
    }
    if ((globFlags & TCL_GLOBMODE_TAILS) && (pathOrDir == NULL)) {
	Tcl_AppendToObj(resultPtr,
	  "\"-tails\" must be used with either \"-directory\" or \"-path\"",
	  -1);
	return TCL_ERROR;
    }
    
    separators = NULL;		/* lint. */
    switch (tclPlatform) {
	case TCL_PLATFORM_UNIX:
	    separators = "/";
	    break;
	case TCL_PLATFORM_WINDOWS:
	    separators = "/\\:";
	    break;
	case TCL_PLATFORM_MAC:
	    separators = ":";
	    break;
    }
    if (dir == PATH_GENERAL) {
	int pathlength;
	char *last;
	char *first = Tcl_GetStringFromObj(pathOrDir,&pathlength);

	/*
	 * Find the last path separator in the path
	 */
	last = first + pathlength;
	for (; last != first; last--) {
	    if (strchr(separators, *(last-1)) != NULL) {
		break;
	    }
	}
	if (last == first + pathlength) {
	    /* It's really a directory */
	    dir = PATH_DIR;
	} else {
	    Tcl_DString pref;
	    char *search, *find;
	    Tcl_DStringInit(&pref);
	    if (last == first) {
		/* The whole thing is a prefix */
		Tcl_DStringAppend(&pref, first, -1);
		pathOrDir = NULL;
	    } else {
		/* Have to split off the end */
		Tcl_DStringAppend(&pref, last, first+pathlength-last);
		pathOrDir = Tcl_NewStringObj(first, last-first-1);
	    }
	    /* Need to quote 'prefix' */
	    Tcl_DStringInit(&prefix);
	    search = Tcl_DStringValue(&pref);
	    while ((find = (strpbrk(search, "\\[]*?{}"))) != NULL) {
	        Tcl_DStringAppend(&prefix, search, find-search);
	        Tcl_DStringAppend(&prefix, "\\", 1);
	        Tcl_DStringAppend(&prefix, find, 1);
	        search = find+1;
	        if (*search == '\0') {
	            break;
	        }
	    }
	    if (*search != '\0') {
		Tcl_DStringAppend(&prefix, search, -1);
	    }
	    Tcl_DStringFree(&pref);
	}
    }
    
    if (pathOrDir != NULL) {
	Tcl_IncrRefCount(pathOrDir);
    }
    
    if (typePtr != NULL) {
	/* 
	 * The rest of the possible type arguments (except 'd') are
	 * platform specific.  We don't complain when they are used
	 * on an incompatible platform.
	 */
	Tcl_ListObjLength(interp, typePtr, &length);
	globTypes = (Tcl_GlobTypeData*) ckalloc(sizeof(Tcl_GlobTypeData));
	globTypes->type = 0;
	globTypes->perm = 0;
	globTypes->macType = NULL;
	globTypes->macCreator = NULL;
	while(--length >= 0) {
	    int len;
	    char *str;
	    Tcl_ListObjIndex(interp, typePtr, length, &look);
	    str = Tcl_GetStringFromObj(look, &len);
	    if (strcmp("readonly", str) == 0) {
		globTypes->perm |= TCL_GLOB_PERM_RONLY;
	    } else if (strcmp("hidden", str) == 0) {
		globTypes->perm |= TCL_GLOB_PERM_HIDDEN;
	    } else if (len == 1) {
		switch (str[0]) {
		  case 'r':
		    globTypes->perm |= TCL_GLOB_PERM_R;
		    break;
		  case 'w':
		    globTypes->perm |= TCL_GLOB_PERM_W;
		    break;
		  case 'x':
		    globTypes->perm |= TCL_GLOB_PERM_X;
		    break;
		  case 'b':
		    globTypes->type |= TCL_GLOB_TYPE_BLOCK;
		    break;
		  case 'c':
		    globTypes->type |= TCL_GLOB_TYPE_CHAR;
		    break;
		  case 'd':
		    globTypes->type |= TCL_GLOB_TYPE_DIR;
		    break;
		  case 'p':
		    globTypes->type |= TCL_GLOB_TYPE_PIPE;
		    break;
		  case 'f':
		    globTypes->type |= TCL_GLOB_TYPE_FILE;
		    break;
	          case 'l':
		    globTypes->type |= TCL_GLOB_TYPE_LINK;
		    break;
		  case 's':
		    globTypes->type |= TCL_GLOB_TYPE_SOCK;
		    break;
		  default:
		    goto badTypesArg;
		}
	    } else if (len == 4) {
		/* This is assumed to be a MacOS file type */
		if (globTypes->macType != NULL) {
		    goto badMacTypesArg;
		}
		globTypes->macType = look;
		Tcl_IncrRefCount(look);
	    } else {
		Tcl_Obj* item;
		if ((Tcl_ListObjLength(NULL, look, &len) == TCL_OK) &&
			(len == 3)) {
		    Tcl_ListObjIndex(interp, look, 0, &item);
		    if (!strcmp("macintosh", Tcl_GetString(item))) {
			Tcl_ListObjIndex(interp, look, 1, &item);
			if (!strcmp("type", Tcl_GetString(item))) {
			    Tcl_ListObjIndex(interp, look, 2, &item);
			    if (globTypes->macType != NULL) {
				goto badMacTypesArg;
			    }
			    globTypes->macType = item;
			    Tcl_IncrRefCount(item);
			    continue;
			} else if (!strcmp("creator", Tcl_GetString(item))) {
			    Tcl_ListObjIndex(interp, look, 2, &item);
			    if (globTypes->macCreator != NULL) {
				goto badMacTypesArg;
			    }
			    globTypes->macCreator = item;
			    Tcl_IncrRefCount(item);
			    continue;
			}
		    }
		}
		/*
		 * Error cases.  We re-get the interpreter's result,
		 * just to be sure it hasn't changed, and we reset
		 * the 'join' flag to zero, since we haven't yet
		 * made use of it.
		 */
		badTypesArg:
		resultPtr = Tcl_GetObjResult(interp);
		Tcl_AppendToObj(resultPtr, "bad argument to \"-types\": ", -1);
		Tcl_AppendObjToObj(resultPtr, look);
		result = TCL_ERROR;
		join = 0;
		goto endOfGlob;
		badMacTypesArg:
		resultPtr = Tcl_GetObjResult(interp);
		Tcl_AppendToObj(resultPtr,
		   "only one MacOS type or creator argument"
		   " to \"-types\" allowed", -1);
		result = TCL_ERROR;
		join = 0;
		goto endOfGlob;
	    }
	}
    }

    /* 
     * Now we perform the actual glob below.  This may involve joining
     * together the pattern arguments, dealing with particular file types
     * etc.  We use a 'goto' to ensure we free any memory allocated along
     * the way.
     */
    objc -= i;
    objv += i;
    /* 
     * We re-retrieve this, in case it was changed in 
     * the Tcl_ResetResult above 
     */
    resultPtr = Tcl_GetObjResult(interp);
    result = TCL_OK;
    if (join) {
	if (dir != PATH_GENERAL) {
	    Tcl_DStringInit(&prefix);
	}
	for (i = 0; i < objc; i++) {
	    string = Tcl_GetStringFromObj(objv[i], &length);
	    Tcl_DStringAppend(&prefix, string, length);
	    if (i != objc -1) {
		Tcl_DStringAppend(&prefix, separators, 1);
	    }
	}
	if (TclGlob(interp, Tcl_DStringValue(&prefix), pathOrDir,
		globFlags, globTypes) != TCL_OK) {
	    result = TCL_ERROR;
	    goto endOfGlob;
	}
    } else {
	if (dir == PATH_GENERAL) {
	    Tcl_DString str;
	    for (i = 0; i < objc; i++) {
		Tcl_DStringInit(&str);
		if (dir == PATH_GENERAL) {
		    Tcl_DStringAppend(&str, Tcl_DStringValue(&prefix),
			    Tcl_DStringLength(&prefix));
		}
		string = Tcl_GetStringFromObj(objv[i], &length);
		Tcl_DStringAppend(&str, string, length);
		if (TclGlob(interp, Tcl_DStringValue(&str), pathOrDir,
			globFlags, globTypes) != TCL_OK) {
		    result = TCL_ERROR;
		    Tcl_DStringFree(&str);
		    goto endOfGlob;
		}
	    }
	    Tcl_DStringFree(&str);
	} else {
	    for (i = 0; i < objc; i++) {
		string = Tcl_GetString(objv[i]);
		if (TclGlob(interp, string, pathOrDir,
			globFlags, globTypes) != TCL_OK) {
		    result = TCL_ERROR;
		    goto endOfGlob;
		}
	    }
	}
    }
    if ((globFlags & TCL_GLOBMODE_NO_COMPLAIN) == 0) {
	if (Tcl_ListObjLength(interp, Tcl_GetObjResult(interp),
		&length) != TCL_OK) {
	    /* This should never happen.  Maybe we should be more dramatic */
	    result = TCL_ERROR;
	    goto endOfGlob;
	}
	if (length == 0) {
	    Tcl_AppendResult(interp, "no files matched glob pattern",
		    (join || (objc == 1)) ? " \"" : "s \"", (char *) NULL);
	    if (join) {
		Tcl_AppendResult(interp, Tcl_DStringValue(&prefix),
			(char *) NULL);
	    } else {
		char *sep = "";
		for (i = 0; i < objc; i++) {
		    string = Tcl_GetString(objv[i]);
		    Tcl_AppendResult(interp, sep, string, (char *) NULL);
		    sep = " ";
		}
	    }
	    Tcl_AppendResult(interp, "\"", (char *) NULL);
	    result = TCL_ERROR;
	}
    }
  endOfGlob:
    if (join || (dir == PATH_GENERAL)) {
	Tcl_DStringFree(&prefix);
    }
    if (pathOrDir != NULL) {
	Tcl_DecrRefCount(pathOrDir);
    }
    if (globTypes != NULL) {
	if (globTypes->macType != NULL) {
	    Tcl_DecrRefCount(globTypes->macType);
	}
	if (globTypes->macCreator != NULL) {
	    Tcl_DecrRefCount(globTypes->macCreator);
	}
	ckfree((char *) globTypes);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclGlob --
 *
 *	This procedure prepares arguments for the TclDoGlob call.
 *	It sets the separator string based on the platform, performs
 *      tilde substitution, and calls TclDoGlob.
 *      
 *      The interpreter's result, on entry to this function, must
 *      be a valid Tcl list (e.g. it could be empty), since we will
 *      lappend any new results to that list.  If it is not a valid
 *      list, this function will fail to do anything very meaningful.
 *
 * Results:
 *	The return value is a standard Tcl result indicating whether
 *	an error occurred in globbing.  After a normal return the
 *	result in interp (set by TclDoGlob) holds all of the file names
 *	given by the pattern and unquotedPrefix arguments.  After an 
 *	error the result in interp will hold an error message, unless
 *	the 'TCL_GLOBMODE_NO_COMPLAIN' flag was given, in which case
 *	an error results in a TCL_OK return leaving the interpreter's
 *	result unmodified.
 *
 * Side effects:
 *	The 'pattern' is written to.
 *
 *----------------------------------------------------------------------
 */

	/* ARGSUSED */
int
TclGlob(interp, pattern, unquotedPrefix, globFlags, types)
    Tcl_Interp *interp;		/* Interpreter for returning error message
				 * or appending list of matching file names. */
    char *pattern;		/* Glob pattern to match. Must not refer
				 * to a static string. */
    Tcl_Obj *unquotedPrefix;	/* Prefix to glob pattern, if non-null, which
                             	 * is considered literally. */
    int globFlags;		/* Stores or'ed combination of flags */
    Tcl_GlobTypeData *types;	/* Struct containing acceptable types.
				 * May be NULL. */
{
    char *separators;
    CONST char *head;
    char *tail, *start;
    char c;
    int result, prefixLen;
    Tcl_DString buffer;
    Tcl_Obj *oldResult;

    separators = NULL;		/* lint. */
    switch (tclPlatform) {
	case TCL_PLATFORM_UNIX:
	    separators = "/";
	    break;
	case TCL_PLATFORM_WINDOWS:
	    separators = "/\\:";
	    break;
	case TCL_PLATFORM_MAC:
#ifdef MAC_UNDERSTANDS_UNIX_PATHS
	    if (unquotedPrefix == NULL) {
		separators = (strchr(pattern, ':') == NULL) ? "/" : ":";
	    } else {
		separators = ":";
	    }
#else
	    separators = ":";
#endif
	    break;
    }

    Tcl_DStringInit(&buffer);
    if (unquotedPrefix != NULL) {
	start = Tcl_GetString(unquotedPrefix);
    } else {
	start = pattern;
    }

    /*
     * Perform tilde substitution, if needed.
     */

    if (start[0] == '~') {
	
	/*
	 * Find the first path separator after the tilde.
	 */
	for (tail = start; *tail != '\0'; tail++) {
	    if (*tail == '\\') {
		if (strchr(separators, tail[1]) != NULL) {
		    break;
		}
	    } else if (strchr(separators, *tail) != NULL) {
		break;
	    }
	}

	/*
	 * Determine the home directory for the specified user.  
	 */
	
	c = *tail;
	*tail = '\0';
	if (globFlags & TCL_GLOBMODE_NO_COMPLAIN) {
	    /* 
	     * We will ignore any error message here, and we
	     * don't want to mess up the interpreter's result.
	     */
	    head = DoTildeSubst(NULL, start+1, &buffer);
	} else {
	    head = DoTildeSubst(interp, start+1, &buffer);
	}
	*tail = c;
	if (head == NULL) {
	    if (globFlags & TCL_GLOBMODE_NO_COMPLAIN) {
		return TCL_OK;
	    } else {
		return TCL_ERROR;
	    }
	}
	if (head != Tcl_DStringValue(&buffer)) {
	    Tcl_DStringAppend(&buffer, head, -1);
	}
	if (unquotedPrefix != NULL) {
	    Tcl_DStringAppend(&buffer, tail, -1);
	    tail = pattern;
	}
    } else {
	tail = pattern;
	if (unquotedPrefix != NULL) {
	    Tcl_DStringAppend(&buffer,Tcl_GetString(unquotedPrefix),-1);
	}
    }
    
    /* 
     * We want to remember the length of the current prefix,
     * in case we are using TCL_GLOBMODE_TAILS.  Also if we
     * are using TCL_GLOBMODE_DIR, we must make sure the
     * prefix ends in a directory separator.
     */
    prefixLen = Tcl_DStringLength(&buffer);

    if (prefixLen > 0) {
	c = Tcl_DStringValue(&buffer)[prefixLen-1];
	if (strchr(separators, c) == NULL) {
	    /* 
	     * If the prefix is a directory, make sure it ends in a
	     * directory separator.
	     */
	    if (globFlags & TCL_GLOBMODE_DIR) {
		Tcl_DStringAppend(&buffer,separators,1);
	    }
	    prefixLen++;
	}
    }

    /* 
     * We need to get the old result, in case it is over-written
     * below when we still need it.
     */
    oldResult = Tcl_GetObjResult(interp);
    Tcl_IncrRefCount(oldResult);
    Tcl_ResetResult(interp);
    
    result = TclDoGlob(interp, separators, &buffer, tail, types);
    
    if (result != TCL_OK) {
	if (globFlags & TCL_GLOBMODE_NO_COMPLAIN) {
	    /* Put back the old result and reset the return code */
	    Tcl_SetObjResult(interp, oldResult);
	    result = TCL_OK;
	}
    } else {
	/* 
	 * Now we must concatenate the 'oldResult' and the current
	 * result, and then place that into the interpreter.
	 * 
	 * If we only want the tails, we must strip off the prefix now.
	 * It may seem more efficient to pass the tails flag down into
	 * TclDoGlob, Tcl_FSMatchInDirectory, but those functions are
	 * continually adjusting the prefix as the various pieces of
	 * the pattern are assimilated, so that would add a lot of
	 * complexity to the code.  This way is a little slower (when
	 * the -tails flag is given), but much simpler to code.
	 */
	int objc, i;
	Tcl_Obj **objv;

	/* Ensure sole ownership */
	if (Tcl_IsShared(oldResult)) {
	    Tcl_DecrRefCount(oldResult);
	    oldResult = Tcl_DuplicateObj(oldResult);
	    Tcl_IncrRefCount(oldResult);
	}

	Tcl_ListObjGetElements(NULL, Tcl_GetObjResult(interp), 
			       &objc, &objv);
#ifdef MAC_TCL
	/* adjust prefixLen if TclDoGlob prepended a ':' */
	if ((prefixLen > 0) && (objc > 0)
	&& (Tcl_DStringValue(&buffer)[0] != ':')) {
	    char *str = Tcl_GetStringFromObj(objv[0],NULL);
	    if (str[0] == ':') {
		    prefixLen++;
	    }
	}
#endif
	for (i = 0; i< objc; i++) {
	    Tcl_Obj* elt;
	    if (globFlags & TCL_GLOBMODE_TAILS) {
		int len;
		char *oldStr = Tcl_GetStringFromObj(objv[i],&len);
		if (len == prefixLen) {
		    if ((pattern[0] == '\0')
			|| (strchr(separators, pattern[0]) == NULL)) {
			elt = Tcl_NewStringObj(".",1);
		    } else {
			elt = Tcl_NewStringObj("/",1);
		    }
		} else {
		    elt = Tcl_NewStringObj(oldStr + prefixLen, 
						len - prefixLen);
		}
	    } else {
		elt = objv[i];
	    }
	    /* Assumption that 'oldResult' is a valid list */
	    Tcl_ListObjAppendElement(interp, oldResult, elt);
	}
	Tcl_SetObjResult(interp, oldResult);
    }
    /* 
     * Release our temporary copy.  All code paths above must
     * end here so we free our reference.
     */
    Tcl_DecrRefCount(oldResult);
    Tcl_DStringFree(&buffer);
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * SkipToChar --
 *
 *	This function traverses a glob pattern looking for the next
 *	unquoted occurance of the specified character at the same braces
 *	nesting level.
 *
 * Results:
 *	Updates stringPtr to point to the matching character, or to
 *	the end of the string if nothing matched.  The return value
 *	is 1 if a match was found at the top level, otherwise it is 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
SkipToChar(stringPtr, match)
    char **stringPtr;			/* Pointer string to check. */
    char *match;			/* Pointer to character to find. */
{
    int quoted, level;
    register char *p;

    quoted = 0;
    level = 0;

    for (p = *stringPtr; *p != '\0'; p++) {
	if (quoted) {
	    quoted = 0;
	    continue;
	}
	if ((level == 0) && (*p == *match)) {
	    *stringPtr = p;
	    return 1;
	}
	if (*p == '{') {
	    level++;
	} else if (*p == '}') {
	    level--;
	} else if (*p == '\\') {
	    quoted = 1;
	}
    }
    *stringPtr = p;
    return 0;
}

/*
 *----------------------------------------------------------------------
 *
 * TclDoGlob --
 *
 *	This recursive procedure forms the heart of the globbing
 *	code.  It performs a depth-first traversal of the tree
 *	given by the path name to be globbed.  The directory and
 *	remainder are assumed to be native format paths.  The prefix 
 *	contained in 'headPtr' is not used as a glob pattern, simply
 *	as a path specifier, so it can contain unquoted glob-sensitive
 *	characters (if the directories to which it points contain
 *	such strange characters).
 *
 * Results:
 *	The return value is a standard Tcl result indicating whether
 *	an error occurred in globbing.  After a normal return the
 *	result in interp will be set to hold all of the file names
 *	given by the dir and rem arguments.  After an error the
 *	result in interp will hold an error message.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclDoGlob(interp, separators, headPtr, tail, types)
    Tcl_Interp *interp;		/* Interpreter to use for error reporting
				 * (e.g. unmatched brace). */
    char *separators;		/* String containing separator characters
				 * that should be used to identify globbing
				 * boundaries. */
    Tcl_DString *headPtr;	/* Completely expanded prefix. */
    char *tail;			/* The unexpanded remainder of the path.
				 * Must not be a pointer to a static string. */
    Tcl_GlobTypeData *types;	/* List object containing list of acceptable 
                            	 * types. May be NULL. */
{
    int baseLength, quoted, count;
    int result = TCL_OK;
    char *name, *p, *openBrace, *closeBrace, *firstSpecialChar, savedChar;
    char lastChar = 0;
    
    int length = Tcl_DStringLength(headPtr);

    if (length > 0) {
	lastChar = Tcl_DStringValue(headPtr)[length-1];
    }

    /*
     * Consume any leading directory separators, leaving tail pointing
     * just past the last initial separator.
     */

    count = 0;
    name = tail;
    for (; *tail != '\0'; tail++) {
	if (*tail == '\\') {
	    /* 
	     * If the first character is escaped, either we have a directory
	     * separator, or we have any other character.  In the latter case
	     * the rest of tail is a pattern, and we must break from the loop.
	     * This is particularly important on Windows where '\' is both
	     * the escaping character and a directory separator.
	     */
	    if (strchr(separators, tail[1]) != NULL) {
		tail++;
	    } else {
		break;
	    }
	} else if (strchr(separators, *tail) == NULL) {
	    break;
	}
	count++;
    }

    /*
     * Deal with path separators.  On the Mac, we have to watch out
     * for multiple separators, since they are special in Mac-style
     * paths.
     */

    switch (tclPlatform) {
	case TCL_PLATFORM_MAC:
#ifdef MAC_UNDERSTANDS_UNIX_PATHS
	    if (*separators == '/') {
		if (((length == 0) && (count == 0))
			|| ((length > 0) && (lastChar != ':'))) {
		    Tcl_DStringAppend(headPtr, ":", 1);
		}
	    } else {
#endif
		if (count == 0) {
		    if ((length > 0) && (lastChar != ':')) {
			Tcl_DStringAppend(headPtr, ":", 1);
		    }
		} else {
		    if (lastChar == ':') {
			count--;
		    }
		    while (count-- > 0) {
			Tcl_DStringAppend(headPtr, ":", 1);
		    }
		}
#ifdef MAC_UNDERSTANDS_UNIX_PATHS
	    }
#endif
	    break;
	case TCL_PLATFORM_WINDOWS:
	    /*
	     * If this is a drive relative path, add the colon and the
	     * trailing slash if needed.  Otherwise add the slash if
	     * this is the first absolute element, or a later relative
	     * element.  Add an extra slash if this is a UNC path.
	     */

	    if (*name == ':') {
		Tcl_DStringAppend(headPtr, ":", 1);
		if (count > 1) {
		    Tcl_DStringAppend(headPtr, "/", 1);
		}
	    } else if ((*tail != '\0')
		    && (((length > 0)
			    && (strchr(separators, lastChar) == NULL))
			    || ((length == 0) && (count > 0)))) {
		Tcl_DStringAppend(headPtr, "/", 1);
		if ((length == 0) && (count > 1)) {
		    Tcl_DStringAppend(headPtr, "/", 1);
		}
	    }
	    
	    break;
	case TCL_PLATFORM_UNIX:
	    /*
	     * Add a separator if this is the first absolute element, or
	     * a later relative element.
	     */

	    if ((*tail != '\0')
		    && (((length > 0)
			    && (strchr(separators, lastChar) == NULL))
			    || ((length == 0) && (count > 0)))) {
		Tcl_DStringAppend(headPtr, "/", 1);
	    }
	    break;
    }

    /*
     * Look for the first matching pair of braces or the first
     * directory separator that is not inside a pair of braces.
     */

    openBrace = closeBrace = NULL;
    quoted = 0;
    for (p = tail; *p != '\0'; p++) {
	if (quoted) {
	    quoted = 0;
	} else if (*p == '\\') {
	    quoted = 1;
	    if (strchr(separators, p[1]) != NULL) {
		break;			/* Quoted directory separator. */
	    }
	} else if (strchr(separators, *p) != NULL) {
	    break;			/* Unquoted directory separator. */
	} else if (*p == '{') {
	    openBrace = p;
	    p++;
	    if (SkipToChar(&p, "}")) {
		closeBrace = p;		/* Balanced braces. */
		break;
	    }
	    Tcl_SetResult(interp, "unmatched open-brace in file name",
		    TCL_STATIC);
	    return TCL_ERROR;
	} else if (*p == '}') {
	    Tcl_SetResult(interp, "unmatched close-brace in file name",
		    TCL_STATIC);
	    return TCL_ERROR;
	}
    }

    /*
     * Substitute the alternate patterns from the braces and recurse.
     */

    if (openBrace != NULL) {
	char *element;
	Tcl_DString newName;
	Tcl_DStringInit(&newName);

	/*
	 * For each element within in the outermost pair of braces,
	 * append the element and the remainder to the fixed portion
	 * before the first brace and recursively call TclDoGlob.
	 */

	Tcl_DStringAppend(&newName, tail, openBrace-tail);
	baseLength = Tcl_DStringLength(&newName);
	length = Tcl_DStringLength(headPtr);
	*closeBrace = '\0';
	for (p = openBrace; p != closeBrace; ) {
	    p++;
	    element = p;
	    SkipToChar(&p, ",");
	    Tcl_DStringSetLength(headPtr, length);
	    Tcl_DStringSetLength(&newName, baseLength);
	    Tcl_DStringAppend(&newName, element, p-element);
	    Tcl_DStringAppend(&newName, closeBrace+1, -1);
	    result = TclDoGlob(interp, separators, headPtr, 
			       Tcl_DStringValue(&newName), types);
	    if (result != TCL_OK) {
		break;
	    }
	}
	*closeBrace = '}';
	Tcl_DStringFree(&newName);
	return result;
    }

    /*
     * At this point, there are no more brace substitutions to perform on
     * this path component.  The variable p is pointing at a quoted or
     * unquoted directory separator or the end of the string.  So we need
     * to check for special globbing characters in the current pattern.
     * We avoid modifying tail if p is pointing at the end of the string.
     */

    if (*p != '\0') {

	/*
	 * Note that we are modifying the string in place.  This won't work
	 * if the string is a static.
	 */

	savedChar = *p;
	*p = '\0';
	firstSpecialChar = strpbrk(tail, "*[]?\\");
	*p = savedChar;
    } else {
	firstSpecialChar = strpbrk(tail, "*[]?\\");
    }

    if (firstSpecialChar != NULL) {
	int ret;
	Tcl_Obj *head = Tcl_NewStringObj(Tcl_DStringValue(headPtr),-1);
	Tcl_IncrRefCount(head);
	/*
	 * Look for matching files in the given directory.  The
	 * implementation of this function is platform specific.  For
	 * each file that matches, it will add the match onto the
	 * resultPtr given.
	 */
	if (*p == '\0') {
	    ret = Tcl_FSMatchInDirectory(interp, Tcl_GetObjResult(interp), 
					 head, tail, types);
	} else {
	    Tcl_Obj* resultPtr;

	    /* 
	     * We do the recursion ourselves.  This makes implementing
	     * Tcl_FSMatchInDirectory for each filesystem much easier.
	     */
	    Tcl_GlobTypeData dirOnly = { TCL_GLOB_TYPE_DIR, 0, NULL, NULL };
	    char save = *p;
	    
	    *p = '\0';
	    resultPtr = Tcl_NewListObj(0, NULL);
	    ret = Tcl_FSMatchInDirectory(interp, resultPtr, 
					 head, tail, &dirOnly);
	    *p = save;
	    if (ret == TCL_OK) {
		int resLength;
		ret = Tcl_ListObjLength(interp, resultPtr, &resLength);
		if (ret == TCL_OK) {
		    int i;
		    for (i =0; i< resLength; i++) {
			Tcl_Obj *elt;
			Tcl_DString ds;
			Tcl_ListObjIndex(interp, resultPtr, i, &elt);
			Tcl_DStringInit(&ds);
			Tcl_DStringAppend(&ds, Tcl_GetString(elt), -1);
			if(tclPlatform == TCL_PLATFORM_MAC) {
			    Tcl_DStringAppend(&ds, ":",1);
			} else {			
			    Tcl_DStringAppend(&ds, "/",1);
			}
			ret = TclDoGlob(interp, separators, &ds, p+1, types);
			Tcl_DStringFree(&ds);
			if (ret != TCL_OK) {
			    break;
			}
		    }
		}
	    }
	    Tcl_DecrRefCount(resultPtr);
	}
	Tcl_DecrRefCount(head);
	return ret;
    }
    Tcl_DStringAppend(headPtr, tail, p-tail);
    if (*p != '\0') {
	return TclDoGlob(interp, separators, headPtr, p, types);
    } else {
	/*
	 * This is the code path reached by a command like 'glob foo'.
	 *
	 * There are no more wildcards in the pattern and no more
	 * unprocessed characters in the tail, so now we can construct
	 * the path, and pass it to Tcl_FSMatchInDirectory with an
	 * empty pattern to verify the existence of the file and check
	 * it is of the correct type (if a 'types' flag it given -- if
	 * no such flag was given, we could just use 'Tcl_FSLStat', but
	 * for simplicity we keep to a common approach).
	 */

	Tcl_Obj *nameObj;

	switch (tclPlatform) {
	    case TCL_PLATFORM_MAC: {
		if (strchr(Tcl_DStringValue(headPtr), ':') == NULL) {
		    Tcl_DStringAppend(headPtr, ":", 1);
		}
		break;
	    }
	    case TCL_PLATFORM_WINDOWS: {
		if (Tcl_DStringLength(headPtr) == 0) {
		    if (((*name == '\\') && (name[1] == '/' || name[1] == '\\'))
			    || (*name == '/')) {
			Tcl_DStringAppend(headPtr, "/", 1);
		    } else {
			Tcl_DStringAppend(headPtr, ".", 1);
		    }
		}
#if defined(__CYGWIN__) && defined(__WIN32__)
		{
		extern int cygwin_conv_to_win32_path 
		    _ANSI_ARGS_((CONST char *, char *));
		char winbuf[MAX_PATH+1];

		cygwin_conv_to_win32_path(Tcl_DStringValue(headPtr), winbuf);
		Tcl_DStringFree(headPtr);
		Tcl_DStringAppend(headPtr, winbuf, -1);
		}
#endif /* __CYGWIN__ && __WIN32__ */
		/* 
		 * Convert to forward slashes.  This is required to pass
		 * some Tcl tests.  We should probably remove the conversions
		 * here and in tclWinFile.c, since they aren't needed since
		 * the dropping of support for Win32s.
		 */
		for (p = Tcl_DStringValue(headPtr); *p != '\0'; p++) {
		    if (*p == '\\') {
			*p = '/';
		    }
		}
		break;
	    }
	    case TCL_PLATFORM_UNIX: {
		if (Tcl_DStringLength(headPtr) == 0) {
		    if ((*name == '\\' && name[1] == '/') || (*name == '/')) {
			Tcl_DStringAppend(headPtr, "/", 1);
		    } else {
			Tcl_DStringAppend(headPtr, ".", 1);
		    }
		}
		break;
	    }
	}
	/* Common for all platforms */
	name = Tcl_DStringValue(headPtr);
	nameObj = Tcl_NewStringObj(name, Tcl_DStringLength(headPtr));

	Tcl_IncrRefCount(nameObj);
	Tcl_FSMatchInDirectory(interp, Tcl_GetObjResult(interp), nameObj, 
			       NULL, types);
	Tcl_DecrRefCount(nameObj);
	return TCL_OK;
    }
}


/*
 *---------------------------------------------------------------------------
 *
 * TclFileDirname
 *
 *	This procedure calculates the directory above a given 
 *	path: basically 'file dirname'.  It is used both by
 *	the 'dirname' subcommand of file and by code in tclIOUtil.c.
 *
 * Results:
 *	NULL if an error occurred, otherwise a Tcl_Obj owned by
 *	the caller (i.e. most likely with refCount 1).
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Obj*
TclFileDirname(interp, pathPtr)
    Tcl_Interp *interp;		/* Used for error reporting */
    Tcl_Obj *pathPtr;           /* Path to take dirname of */
{
    int splitElements;
    Tcl_Obj *splitPtr;
    Tcl_Obj *splitResultPtr = NULL;

    /* 
     * The behaviour we want here is slightly different to
     * the standard Tcl_FSSplitPath in the handling of home
     * directories; Tcl_FSSplitPath preserves the "~" while 
     * this code computes the actual full path name, if we
     * had just a single component.
     */	    
    splitPtr = Tcl_FSSplitPath(pathPtr, &splitElements);
    if ((splitElements == 1) && (Tcl_GetString(pathPtr)[0] == '~')) {
	Tcl_DecrRefCount(splitPtr);
	splitPtr = Tcl_FSGetNormalizedPath(interp, pathPtr);
	if (splitPtr == NULL) {
	    return NULL;
	}
	splitPtr = Tcl_FSSplitPath(splitPtr, &splitElements);
    }

    /*
     * Return all but the last component.  If there is only one
     * component, return it if the path was non-relative, otherwise
     * return the current directory.
     */

    if (splitElements > 1) {
	splitResultPtr = Tcl_FSJoinPath(splitPtr, splitElements - 1);
    } else if (splitElements == 0 || 
      (Tcl_FSGetPathType(pathPtr) == TCL_PATH_RELATIVE)) {
	splitResultPtr = Tcl_NewStringObj(
		((tclPlatform == TCL_PLATFORM_MAC) ? ":" : "."), 1);
    } else {
	Tcl_ListObjIndex(NULL, splitPtr, 0, &splitResultPtr);
    }
    Tcl_IncrRefCount(splitResultPtr);
    Tcl_DecrRefCount(splitPtr);
    return splitResultPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_AllocStatBuf
 *
 *     This procedure allocates a Tcl_StatBuf on the heap.  It exists
 *     so that extensions may be used unchanged on systems where
 *     largefile support is optional.
 *
 * Results:
 *     A pointer to a Tcl_StatBuf which may be deallocated by being
 *     passed to ckfree().
 *
 * Side effects:
 *      None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_StatBuf *
Tcl_AllocStatBuf() {
    return (Tcl_StatBuf *) ckalloc(sizeof(Tcl_StatBuf));
}
