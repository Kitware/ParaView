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
 * The following regular expression matches the root portion of a Windows
 * absolute or volume relative path.  It will match both UNC and drive relative
 * paths.
 */

#define WIN_ROOT_PATTERN "^(([a-zA-Z]:)|[/\\\\][/\\\\]+([^/\\\\]+)[/\\\\]+([^/\\\\]+)|([/\\\\]))([/\\\\])*"

/*
 * The following regular expression matches the root portion of a Macintosh
 * absolute path.  It will match degenerate Unix-style paths, tilde paths,
 * Unix-style paths, and Mac paths.
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

/*
 * The following variable is set in the TclPlatformInit call to one
 * of: TCL_PLATFORM_UNIX, TCL_PLATFORM_MAC, or TCL_PLATFORM_WINDOWS.
 */

TclPlatformType tclPlatform = TCL_PLATFORM_UNIX;

/*
 * The "globParameters" argument of the globbing functions is an 
 * or'ed combination of the following values:
 */

#define GLOBMODE_NO_COMPLAIN      1
#define GLOBMODE_JOIN             2
#define GLOBMODE_DIR              4

/*
 * Prototypes for local procedures defined in this file:
 */

static char *		DoTildeSubst _ANSI_ARGS_((Tcl_Interp *interp,
			    CONST char *user, Tcl_DString *resultPtr));
static CONST char *	ExtractWinRoot _ANSI_ARGS_((CONST char *path,
			    Tcl_DString *resultPtr, int offset, Tcl_PathType *typePtr));
static void		FileNameCleanup _ANSI_ARGS_((ClientData clientData));
static void		FileNameInit _ANSI_ARGS_((void));
static int		SkipToChar _ANSI_ARGS_((char **stringPtr,
			    char *match));
static char *		SplitMacPath _ANSI_ARGS_((CONST char *path,
			    Tcl_DString *bufPtr));
static char *		SplitWinPath _ANSI_ARGS_((CONST char *path,
			    Tcl_DString *bufPtr));
static char *		SplitUnixPath _ANSI_ARGS_((CONST char *path,
			    Tcl_DString *bufPtr));

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
    FileNameInit();


    if (path[0] == '/' || path[0] == '\\') {
	/* Might be a UNC or Vol-Relative path */
	char *host, *share, *tail;
	int hlen, slen;
	if (path[1] != '/' && path[1] != '\\') {
	    Tcl_DStringSetLength(resultPtr, offset);
	    *typePtr = TCL_PATH_VOLUME_RELATIVE;
	    Tcl_DStringAppend(resultPtr, "/", 1);
	    return &path[1];
    }
	host = (char *)&path[2];

	/* Skip seperators */
	while (host[0] == '/' || host[0] == '\\') host++;

	for (hlen = 0; host[hlen];hlen++) {
	    if (host[hlen] == '/' || host[hlen] == '\\')
		break;
	}
	if (host[hlen] == 0 || host[hlen+1] == 0) {
	    *typePtr = TCL_PATH_VOLUME_RELATIVE;
	    Tcl_DStringAppend(resultPtr, "/", 1);
	    return &path[2];
	}
	Tcl_DStringSetLength(resultPtr, offset);
	share = &host[hlen];

	/* Skip seperators */
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

	/* Skip seperators */
	while (tail[0] == '/' || tail[0] == '\\') tail++;

	*typePtr = TCL_PATH_ABSOLUTE;
	return tail;
    } else if (path[1] == ':') {
	/* Might be a drive sep */
	Tcl_DStringSetLength(resultPtr, offset);

	if (path[2] != '/' && path[2] != '\\') {
	    *typePtr = TCL_PATH_VOLUME_RELATIVE;
	    Tcl_DStringAppend(resultPtr, path, 2);
	    return &path[2];
    } else {
	    char *tail = (char*)&path[3];

	    /* Skip seperators */
	    while (tail[0] == '/' || tail[0] == '\\') tail++;

	    *typePtr = TCL_PATH_ABSOLUTE;
	    Tcl_DStringAppend(resultPtr, path, 2);
	Tcl_DStringAppend(resultPtr, "/", 1);

    return tail;
	}
    } else {
	*typePtr = TCL_PATH_RELATIVE;
	return path;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetPathType --
 *
 *	Determines whether a given path is relative to the current
 *	directory, relative to the current volume, or absolute.
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
    char *path;
{
    ThreadSpecificData *tsdPtr;
    Tcl_PathType type = TCL_PATH_ABSOLUTE;
    Tcl_RegExp re;

    switch (tclPlatform) {
   	case TCL_PLATFORM_UNIX:
	    /*
	     * Paths that begin with / or ~ are absolute.
	     */

	    if ((path[0] != '/') && (path[0] != '~')) {
		type = TCL_PATH_RELATIVE;
	    }
	    break;

	case TCL_PLATFORM_MAC:
	    if (path[0] == ':') {
		type = TCL_PATH_RELATIVE;
	    } else if (path[0] != '~') {
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
		    char *unixRoot, *dummy;

		    Tcl_RegExpRange(re, 2, &unixRoot, &dummy);
		    if (unixRoot) {
			type = TCL_PATH_RELATIVE;
		    }
		}
	    }
	    break;
	
	case TCL_PLATFORM_WINDOWS:
	    if (path[0] != '~') {
		Tcl_DString ds;

		Tcl_DStringInit(&ds);
		(VOID)ExtractWinRoot(path, &ds, 0, &type);
		Tcl_DStringFree(&ds);
	    }
	    break;
    }
    return type;
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
    char ***argvPtr;		/* Pointer to place to store pointer to array
				 * of pointers to path elements. */
{
    int i, size;
    char *p;
    Tcl_DString buffer;

    Tcl_DStringInit(&buffer);

    /*
     * Perform platform specific splitting.  These routines will leave the
     * result in the specified buffer.  Individual elements are terminated
     * with a null character.
     */

    p = NULL;			/* Needed only to prevent gcc warnings. */
    switch (tclPlatform) {
   	case TCL_PLATFORM_UNIX:
	    p = SplitUnixPath(path, &buffer);
	    break;

	case TCL_PLATFORM_WINDOWS:
	    p = SplitWinPath(path, &buffer);
	    break;
	    
	case TCL_PLATFORM_MAC:
	    p = SplitMacPath(path, &buffer);
	    break;
    }

    /*
     * Compute the number of elements in the result.
     */

    size = Tcl_DStringLength(&buffer);
    *argcPtr = 0;
    for (i = 0; i < size; i++) {
	if (p[i] == '\0') {
	    (*argcPtr)++;
	}
    }
    
    /*
     * Allocate a buffer large enough to hold the contents of the
     * DString plus the argv pointers and the terminating NULL pointer.
     */

    *argvPtr = (char **) ckalloc((unsigned)
	    ((((*argcPtr) + 1) * sizeof(char *)) + size));

    /*
     * Position p after the last argv pointer and copy the contents of
     * the DString.
     */

    p = (char *) &(*argvPtr)[(*argcPtr) + 1];
    memcpy((VOID *) p, (VOID *) Tcl_DStringValue(&buffer), (size_t) size);

    /*
     * Now set up the argv pointers.
     */

    for (i = 0; i < *argcPtr; i++) {
	(*argvPtr)[i] = p;
	while ((*p++) != '\0') {}
    }
    (*argvPtr)[i] = NULL;

    Tcl_DStringFree(&buffer);
}

/*
 *----------------------------------------------------------------------
 *
 * SplitUnixPath --
 *
 *	This routine is used by Tcl_SplitPath to handle splitting
 *	Unix paths.
 *
 * Results:
 *	Stores a null separated array of strings in the specified
 *	Tcl_DString.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
SplitUnixPath(path, bufPtr)
    CONST char *path;		/* Pointer to string containing a path. */
    Tcl_DString *bufPtr;	/* Pointer to DString to use for the result. */
{
    int length;
    CONST char *p, *elementStart;

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
	Tcl_DStringAppend(bufPtr, "/", 2);
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
	    if ((elementStart[0] == '~') && (elementStart != path)) {
		Tcl_DStringAppend(bufPtr, "./", 2);
	    }
	    Tcl_DStringAppend(bufPtr, elementStart, length);
	    Tcl_DStringAppend(bufPtr, "", 1);
	}
	if (*p++ == '\0') {
	    break;
	}
    }
    return Tcl_DStringValue(bufPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * SplitWinPath --
 *
 *	This routine is used by Tcl_SplitPath to handle splitting
 *	Windows paths.
 *
 * Results:
 *	Stores a null separated array of strings in the specified
 *	Tcl_DString.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
SplitWinPath(path, bufPtr)
    CONST char *path;		/* Pointer to string containing a path. */
    Tcl_DString *bufPtr;	/* Pointer to DString to use for the result. */
{
    int length;
    CONST char *p, *elementStart;
    Tcl_PathType type = TCL_PATH_ABSOLUTE;

    p = ExtractWinRoot(path, bufPtr, 0, &type);

    /*
     * Terminate the root portion, if we matched something.
     */

    if (p != path) {
	Tcl_DStringAppend(bufPtr, "", 1);
    }

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
	    if ((elementStart[0] == '~') && (elementStart != path)) {
		Tcl_DStringAppend(bufPtr, "./", 2);
	    }
	    Tcl_DStringAppend(bufPtr, elementStart, length);
	    Tcl_DStringAppend(bufPtr, "", 1);
	}
    } while (*p++ != '\0');

    return Tcl_DStringValue(bufPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * SplitMacPath --
 *
 *	This routine is used by Tcl_SplitPath to handle splitting
 *	Macintosh paths.
 *
 * Results:
 *	Returns a newly allocated argv array.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static char *
SplitMacPath(path, bufPtr)
    CONST char *path;		/* Pointer to string containing a path. */
    Tcl_DString *bufPtr;	/* Pointer to DString to use for the result. */
{
    int isMac = 0;		/* 1 if is Mac-style, 0 if Unix-style path. */
    int i, length;
    CONST char *p, *elementStart;
    Tcl_RegExp re;
    ThreadSpecificData *tsdPtr = TCL_TSD_INIT(&dataKey);

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
	char *start, *end;

	/*
	 * Treat degenerate absolute paths like / and /../.. as
	 * Mac relative file names for lack of anything else to do.
	 */

	Tcl_RegExpRange(re, 2, &start, &end);
	if (start) {
	    Tcl_DStringAppend(bufPtr, ":", 1);
	    Tcl_RegExpRange(re, 0, &start, &end);
	    Tcl_DStringAppend(bufPtr, path, end - start + 1);
	    return Tcl_DStringValue(bufPtr);
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
	 * Append the element and terminate it with a : and a null.  Note that
	 * we are forcing the DString to contain an extra null at the end.
	 */

	Tcl_DStringAppend(bufPtr, start, length);
	Tcl_DStringAppend(bufPtr, ":", 2);
	p = end;
    } else {
	isMac = (strchr(path, ':') != NULL);
	p = path;
    }
    
    if (isMac) {

	/*
	 * p is pointing at the first colon in the path.  There
	 * will always be one, since this is a Mac-style path.
	 */

	elementStart = p++;
	while ((p = strchr(p, ':')) != NULL) {
	    length = p - elementStart;
	    if (length == 1) {
		while (*p == ':') {
		    Tcl_DStringAppend(bufPtr, "::", 3);
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
		Tcl_DStringAppend(bufPtr, elementStart, length);
		Tcl_DStringAppend(bufPtr, "", 1);
		elementStart = p++;
	    }
	}
	if (elementStart[1] != '\0' || elementStart == path) {
	    if ((elementStart[1] != '~') && (elementStart[1] != '\0')
			&& (strchr(elementStart+1, '/') == NULL)) {
		    elementStart++;
	    }
	    Tcl_DStringAppend(bufPtr, elementStart, -1);
	    Tcl_DStringAppend(bufPtr, "", 1);
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
		    Tcl_DStringAppend(bufPtr, ":", 2);
		} else if ((length == 2) && (elementStart[0] == '.')
			&& (elementStart[1] == '.')) {
		    Tcl_DStringAppend(bufPtr, "::", 3);
		} else {
		    if (*elementStart == '~') {
			Tcl_DStringAppend(bufPtr, ":", 1);
		    }
		    Tcl_DStringAppend(bufPtr, elementStart, length);
		    Tcl_DStringAppend(bufPtr, "", 1);
		}
	    }
	    if (*p++ == '\0') {
		break;
	    }
	}
    }
    return Tcl_DStringValue(bufPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_JoinPath --
 *
 *	Combine a list of paths in a platform specific manner.
 *
 * Results:
 *	Appends the joined path to the end of the specified
 *	returning a pointer to the resulting string.  Note that
 *	the Tcl_DString must already be initialized.
 *
 * Side effects:
 *	Modifies the Tcl_DString.
 *
 *----------------------------------------------------------------------
 */

char *
Tcl_JoinPath(argc, argv, resultPtr)
    int argc;
    char **argv;
    Tcl_DString *resultPtr;	/* Pointer to previously initialized DString. */
{
    int oldLength, length, i, needsSep;
    Tcl_DString buffer;
    char c, *dest;
    CONST char *p;
    Tcl_PathType type = TCL_PATH_ABSOLUTE;

    Tcl_DStringInit(&buffer);
    oldLength = Tcl_DStringLength(resultPtr);

    switch (tclPlatform) {
   	case TCL_PLATFORM_UNIX:
	    for (i = 0; i < argc; i++) {
		p = argv[i];
		/*
		 * If the path is absolute, reset the result buffer.
		 * Consume any duplicate leading slashes or a ./ in
		 * front of a tilde prefixed path that isn't at the
		 * beginning of the path.
		 */

#ifdef __QNX__
		/*
		 * Check for QNX //<node id> prefix
		 */
		if (*p && (strlen(p) > 3) && (p[0] == '/') && (p[1] == '/')
			&& isdigit(UCHAR(p[2]))) { /* INTL: digit */
		    p += 3;
		    while (isdigit(UCHAR(*p))) { /* INTL: digit */
			++p;
		    }
		}
#endif
		if (*p == '/') {
		    Tcl_DStringSetLength(resultPtr, oldLength);
		    Tcl_DStringAppend(resultPtr, "/", 1);
		    while (*p == '/') {
			p++;
		    }
		} else if (*p == '~') {
		    Tcl_DStringSetLength(resultPtr, oldLength);
		} else if ((Tcl_DStringLength(resultPtr) != oldLength)
			&& (p[0] == '.') && (p[1] == '/')
			&& (p[2] == '~')) {
		    p += 2;
		}

		if (*p == '\0') {
		    continue;
		}

		/*
		 * Append a separator if needed.
		 */

		length = Tcl_DStringLength(resultPtr);
		if ((length != oldLength)
			&& (Tcl_DStringValue(resultPtr)[length-1] != '/')) {
		    Tcl_DStringAppend(resultPtr, "/", 1);
		    length++;
		}

		/*
		 * Append the element, eliminating duplicate and trailing
		 * slashes.
		 */

		Tcl_DStringSetLength(resultPtr, (int) (length + strlen(p)));
		dest = Tcl_DStringValue(resultPtr) + length;
		for (; *p != '\0'; p++) {
		    if (*p == '/') {
			while (p[1] == '/') {
			    p++;
			}
			if (p[1] != '\0') {
			    *dest++ = '/';
			}
		    } else {
			*dest++ = *p;
		    }
		}
		length = dest - Tcl_DStringValue(resultPtr);
		Tcl_DStringSetLength(resultPtr, length);
	    }
	    break;

	case TCL_PLATFORM_WINDOWS:
	    /*
	     * Iterate over all of the components.  If a component is
	     * absolute, then reset the result and start building the
	     * path from the current component on.
	     */

	    for (i = 0; i < argc; i++) {
		p = ExtractWinRoot(argv[i], resultPtr, oldLength, &type);
		length = Tcl_DStringLength(resultPtr);
		
		/*
		 * If the pointer didn't move, then this is a relative path
		 * or a tilde prefixed path.
		 */

		if (p == argv[i]) {
		    /*
		     * Remove the ./ from tilde prefixed elements unless
		     * it is the first component.
		     */

		    if ((length != oldLength)
			    && (p[0] == '.')
			    && ((p[1] == '/') || (p[1] == '\\'))
			    && (p[2] == '~')) {
			p += 2;
		    } else if (*p == '~') {
			Tcl_DStringSetLength(resultPtr, oldLength);
			length = oldLength;
		    }
		}

		if (*p != '\0') {
		    /*
		     * Check to see if we need to append a separator.
		     */

		    
		    if (length != oldLength) {
			c = Tcl_DStringValue(resultPtr)[length-1];
			if ((c != '/') && (c != ':')) {
			    Tcl_DStringAppend(resultPtr, "/", 1);
			}
		    }

		    /*
		     * Append the element, eliminating duplicate and
		     * trailing slashes.
		     */

		    length = Tcl_DStringLength(resultPtr);
		    Tcl_DStringSetLength(resultPtr, (int) (length + strlen(p)));
		    dest = Tcl_DStringValue(resultPtr) + length;
		    for (; *p != '\0'; p++) {
			if ((*p == '/') || (*p == '\\')) {
			    while ((p[1] == '/') || (p[1] == '\\')) {
				p++;
			    }
			    if (p[1] != '\0') {
				*dest++ = '/';
			    }
			} else {
			    *dest++ = *p;
			}
		    }
		    length = dest - Tcl_DStringValue(resultPtr);
		    Tcl_DStringSetLength(resultPtr, length);
		}
	    }
	    break;

	case TCL_PLATFORM_MAC:
	    needsSep = 1;
	    for (i = 0; i < argc; i++) {
		Tcl_DStringSetLength(&buffer, 0);
		p = SplitMacPath(argv[i], &buffer);
		if ((*p != ':') && (*p != '\0')
			&& (strchr(p, ':') != NULL)) {
		    Tcl_DStringSetLength(resultPtr, oldLength);
		    length = strlen(p);
		    Tcl_DStringAppend(resultPtr, p, length);
		    needsSep = 0;
		    p += length+1;
		}

		/*
		 * Now append the rest of the path elements, skipping
		 * : unless it is the first element of the path, and
		 * watching out for :: et al. so we don't end up with
		 * too many colons in the result.
		 */

		for (; *p != '\0'; p += length+1) {
		    if (p[0] == ':' && p[1] == '\0') {
			if (Tcl_DStringLength(resultPtr) != oldLength) {
			    p++;
			} else {
			    needsSep = 0;
			}
		    } else {
			c = p[1];
			if (*p == ':') {
			    if (!needsSep) {
				p++;
			    }
			} else {
			    if (needsSep) {
				Tcl_DStringAppend(resultPtr, ":", 1);
			    }
			}
			needsSep = (c == ':') ? 0 : 1;
		    }
		    length = strlen(p);
		    Tcl_DStringAppend(resultPtr, p, length);
		}
	    }
	    break;
			       
    }
    Tcl_DStringFree(&buffer);
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
    char *name;			/* File name, which may begin with "~" (to
				 * indicate current user's home directory) or
				 * "~<user>" (to indicate any user's home
				 * directory). */
    Tcl_DString *bufferPtr;	/* Uninitialized or free DString filled
				 * with name after tilde substitution. */
{
    register char *p;

    /*
     * Handle tilde substitutions, if needed.
     */

    if (name[0] == '~') {
	int argc, length;
	char **argv;
	Tcl_DString temp;

	Tcl_SplitPath(name, &argc, (char ***) &argv);
	
	/*
	 * Strip the trailing ':' off of a Mac path before passing the user
	 * name to DoTildeSubst.
	 */

	if (tclPlatform == TCL_PLATFORM_MAC) {
	    length = strlen(argv[0]);
	    argv[0][length-1] = '\0';
	}
	
	Tcl_DStringInit(&temp);
	argv[0] = DoTildeSubst(interp, argv[0]+1, &temp);
	if (argv[0] == NULL) {
	    Tcl_DStringFree(&temp);
	    ckfree((char *)argv);
	    return NULL;
	}
	Tcl_DStringInit(bufferPtr);
	Tcl_JoinPath(argc, (char **) argv, bufferPtr);
	Tcl_DStringFree(&temp);
	ckfree((char*)argv);
    } else {
	Tcl_DStringInit(bufferPtr);
	Tcl_JoinPath(1, (char **) &name, bufferPtr);
    }

    /*
     * Convert forward slashes to backslashes in Windows paths because
     * some system interfaces don't accept forward slashes.
     */

    if (tclPlatform == TCL_PLATFORM_WINDOWS) {
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
	    if (strchr(name, ':') == NULL) {
		lastSep = strrchr(name, '/');
	    } else {
		lastSep = strrchr(name, ':');
	    }
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
    if ((p != NULL) && (lastSep != NULL)
	    && (lastSep > p)) {
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

static char *
DoTildeSubst(interp, user, resultPtr)
    Tcl_Interp *interp;		/* Interpreter in which to store error
				 * message (if necessary). */
    CONST char *user;		/* Name of user whose home directory should be
				 * substituted, or "" for current user. */
    Tcl_DString *resultPtr;	/* Initialized DString filled with name
				 * after tilde substitution. */
{
    char *dir;

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
    return resultPtr->string;
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
    int index, i, globFlags, pathlength, length, join, dir, result;
    char *string, *pathOrDir, *separators;
    Tcl_Obj *typePtr, *resultPtr, *look;
    Tcl_DString prefix, directory;
    static char *options[] = {
	"-directory", "-join", "-nocomplain", "-path", "-types", "--", NULL
    };
    enum options {
	GLOB_DIR, GLOB_JOIN, GLOB_NOCOMPLAIN, GLOB_PATH, GLOB_TYPE, GLOB_LAST
    };
    enum pathDirOptions {PATH_NONE = -1 , PATH_GENERAL = 0, PATH_DIR = 1};
    GlobTypeData *globTypes = NULL;

    globFlags = 0;
    join = 0;
    dir = PATH_NONE;
    pathOrDir = NULL;
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
	        globFlags |= GLOBMODE_NO_COMPLAIN;
		break;
	    case GLOB_DIR:				/* -dir */
		if (i == (objc-1)) {
		    Tcl_AppendToObj(resultPtr,
			    "missing argument to \"-directory\"", -1);
		    return TCL_ERROR;
		}
		if (dir != -1) {
		    Tcl_AppendToObj(resultPtr,
			    "\"-directory\" cannot be used with \"-path\"",
			    -1);
		    return TCL_ERROR;
		}
		dir = PATH_DIR;
		globFlags |= GLOBMODE_DIR;
		pathOrDir = Tcl_GetStringFromObj(objv[i+1], &pathlength);
		i++;
		break;
	    case GLOB_JOIN:				/* -join */
		join = 1;
		break;
	    case GLOB_PATH:				/* -path */
	        if (i == (objc-1)) {
		    Tcl_AppendToObj(resultPtr,
			    "missing argument to \"-path\"", -1);
		    return TCL_ERROR;
		}
		if (dir != -1) {
		    Tcl_AppendToObj(resultPtr,
			    "\"-path\" cannot be used with \"-directory\"",
			    -1);
		    return TCL_ERROR;
		}
		dir = PATH_GENERAL;
		pathOrDir = Tcl_GetStringFromObj(objv[i+1], &pathlength);
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
		break;
	}
    }
    endOfForLoop:
    if (objc - i < 1) {
        Tcl_WrongNumArgs(interp, 1, objv, "?switches? name ?name ...?");
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
	char *last;

	/*
	 * Find the last path separator in the path
	 */
	last = pathOrDir + pathlength;
	for (; last != pathOrDir; last--) {
	    if (strchr(separators, *(last-1)) != NULL) {
		break;
	    }
	}
	if (last == pathOrDir + pathlength) {
	    /* It's really a directory */
	    dir = 1;
	} else {
	    Tcl_DString pref;
	    char *search, *find;
	    Tcl_DStringInit(&pref);
	    Tcl_DStringInit(&directory);
	    if (last == pathOrDir) {
		/* The whole thing is a prefix */
		Tcl_DStringAppend(&pref, pathOrDir, -1);
		pathOrDir = NULL;
	    } else {
		/* Have to split off the end */
		Tcl_DStringAppend(&pref, last, pathOrDir+pathlength-last);
		Tcl_DStringAppend(&directory, pathOrDir, last-pathOrDir-1);
		pathOrDir = Tcl_DStringValue(&directory);
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

    if (typePtr != NULL) {
	/* 
	 * The rest of the possible type arguments (except 'd') are
	 * platform specific.  We don't complain when they are used
	 * on an incompatible platform.
	 */
	Tcl_ListObjLength(interp, typePtr, &length);
	globTypes = (GlobTypeData*) ckalloc(sizeof(GlobTypeData));
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
		 * Error cases
		 */
		badTypesArg:
		Tcl_AppendToObj(resultPtr, "bad argument to \"-types\": ", -1);
		Tcl_AppendObjToObj(resultPtr, look);
		result = TCL_ERROR;
		goto endOfGlob;
		badMacTypesArg:
		Tcl_AppendToObj(resultPtr,
			"only one MacOS type or creator argument to \"-types\" allowed", -1);
		result = TCL_ERROR;
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
    if ((globFlags & GLOBMODE_NO_COMPLAIN) == 0) {
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
	if (dir == PATH_GENERAL) {
	    Tcl_DStringFree(&directory);
	}
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
 * Results:
 *	The return value is a standard Tcl result indicating whether
 *	an error occurred in globbing.  After a normal return the
 *	result in interp (set by TclDoGlob) holds all of the file names
 *	given by the dir and rem arguments.  After an error the
 *	result in interp will hold an error message.
 *
 * Side effects:
 *	The currentArgString is written to.
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
    char *unquotedPrefix;	/* Prefix to glob pattern, if non-null, which
                             	 * is considered literally.  May be static. */
    int globFlags;		/* Stores or'ed combination of flags */
    GlobTypeData *types;	/* Struct containing acceptable types.
				 * May be NULL. */
{
    char *separators;
    char *head, *tail, *start;
    char c;
    int result;
    Tcl_DString buffer;

    separators = NULL;		/* lint. */
    switch (tclPlatform) {
	case TCL_PLATFORM_UNIX:
	    separators = "/";
	    break;
	case TCL_PLATFORM_WINDOWS:
	    separators = "/\\:";
	    break;
	case TCL_PLATFORM_MAC:
	    if (unquotedPrefix == NULL) {
		separators = (strchr(pattern, ':') == NULL) ? "/" : ":";
	    } else {
		separators = ":";
	    }
	    break;
    }

    Tcl_DStringInit(&buffer);
    if (unquotedPrefix != NULL) {
	start = unquotedPrefix;
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
	 * Determine the home directory for the specified user.  Note that
	 * we don't allow special characters in the user name.
	 */
	
	c = *tail;
	*tail = '\0';
	/*
	 * I don't think we need to worry about special characters in
	 * the user name anymore (Vince Darley, June 1999), since the
	 * new code is designed to handle special chars.
	 */
#ifndef NOT_NEEDED_ANYMORE
	head = DoTildeSubst(interp, start+1, &buffer);
#else
	
	if (strpbrk(start+1, "\\[]*?{}") == NULL) {
	    head = DoTildeSubst(interp, start+1, &buffer);
	} else {
	    if (!(globFlags & GLOBMODE_NO_COMPLAIN)) {
		Tcl_ResetResult(interp);
		Tcl_AppendResult(interp, "globbing characters not ",
			"supported in user names", (char *) NULL);
	    }
	    head = NULL;
	}
#endif
	*tail = c;
	if (head == NULL) {
	    if (globFlags & GLOBMODE_NO_COMPLAIN) {
		/*
		 * We should in fact pass down the nocomplain flag 
		 * or save the interp result or use another mechanism
		 * so the interp result is not mangled on errors in that case.
		 * but that would a bigger change than reasonable for a patch
		 * release.
		 * (see fileName.test 15.2-15.4 for expected behaviour)
		 */
		Tcl_ResetResult(interp);
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
	    Tcl_DStringAppend(&buffer,unquotedPrefix,-1);
	}
    }
    /* 
     * If the prefix is a directory, make sure it ends in a directory
     * separator.
     */
    if (unquotedPrefix != NULL) {
	if (globFlags & GLOBMODE_DIR) {
	    c = Tcl_DStringValue(&buffer)[Tcl_DStringLength(&buffer)-1];
	    if (strchr(separators, c) == NULL) {
		Tcl_DStringAppend(&buffer,separators,1);
	    }
	}
    }

    result = TclDoGlob(interp, separators, &buffer, tail, types);
    Tcl_DStringFree(&buffer);
    if (result != TCL_OK) {
	if (globFlags & GLOBMODE_NO_COMPLAIN) {
	    Tcl_ResetResult(interp);
	    return TCL_OK;
	}
    }
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
    GlobTypeData *types;	/* List object containing list of acceptable types.
				 * May be NULL. */
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
	if ((*tail == '\\') && (strchr(separators, tail[1]) != NULL)) {
	    tail++;
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
	    if (*separators == '/') {
		if (((length == 0) && (count == 0))
			|| ((length > 0) && (lastChar != ':'))) {
		    Tcl_DStringAppend(headPtr, ":", 1);
		}
	    } else {
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
	    }
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
	    result = TclDoGlob(interp, separators,
		    headPtr, Tcl_DStringValue(&newName), types);
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
	/*
	 * Look for matching files in the current directory.  The
	 * implementation of this function is platform specific, but may
	 * recursively call TclDoGlob.  For each file that matches, it will
	 * add the match onto the interp's result, or call TclDoGlob if there
	 * are more characters to be processed.
	 */

	return TclpMatchFilesTypes(interp, separators, headPtr, tail, p, types);
    }
    Tcl_DStringAppend(headPtr, tail, p-tail);
    if (*p != '\0') {
	return TclDoGlob(interp, separators, headPtr, p, types);
    }

    /*
     * There are no more wildcards in the pattern and no more unprocessed
     * characters in the tail, so now we can construct the path and verify
     * the existence of the file.
     */

    switch (tclPlatform) {
	case TCL_PLATFORM_MAC: {
	    if (strchr(Tcl_DStringValue(headPtr), ':') == NULL) {
		Tcl_DStringAppend(headPtr, ":", 1);
	    }
	    name = Tcl_DStringValue(headPtr);
	    if (TclpAccess(name, F_OK) == 0) {
		if ((name[1] != '\0') && (strchr(name+1, ':') == NULL)) {
		    Tcl_ListObjAppendElement(interp, Tcl_GetObjResult(interp), 
					     Tcl_NewStringObj(name + 1,-1));
		} else {
		    Tcl_ListObjAppendElement(interp, Tcl_GetObjResult(interp), 
					     Tcl_NewStringObj(name,-1));
		}
	    }
	    break;
	}
	case TCL_PLATFORM_WINDOWS: {
	    int exists;
	    
	    /*
	     * We need to convert slashes to backslashes before checking
	     * for the existence of the file.  Once we are done, we need
	     * to convert the slashes back.
	     */

	    if (Tcl_DStringLength(headPtr) == 0) {
		if (((*name == '\\') && (name[1] == '/' || name[1] == '\\'))
			|| (*name == '/')) {
		    Tcl_DStringAppend(headPtr, "\\", 1);
		} else {
		    Tcl_DStringAppend(headPtr, ".", 1);
		}
	    } else {
		for (p = Tcl_DStringValue(headPtr); *p != '\0'; p++) {
		    if (*p == '/') {
			*p = '\\';
		    }
		}
	    }
	    name = Tcl_DStringValue(headPtr);
	    exists = (TclpAccess(name, F_OK) == 0);

	    for (p = name; *p != '\0'; p++) {
		if (*p == '\\') {
		    *p = '/';
		}
	    }
	    if (exists) {
		Tcl_ListObjAppendElement(interp, Tcl_GetObjResult(interp), 
					 Tcl_NewStringObj(name,-1));
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
	    name = Tcl_DStringValue(headPtr);
	    if (TclpAccess(name, F_OK) == 0) {
		Tcl_ListObjAppendElement(interp, Tcl_GetObjResult(interp), 
					 Tcl_NewStringObj(name,-1));
	    }
	    break;
	}
    }

    return TCL_OK;
}
