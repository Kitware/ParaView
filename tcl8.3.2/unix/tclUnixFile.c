/* 
 * tclUnixFile.c --
 *
 *      This file contains wrappers around UNIX file handling functions.
 *      These wrappers mask differences between Windows and UNIX.
 *
 * Copyright (c) 1995-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"


/*
 *---------------------------------------------------------------------------
 *
 * TclpFindExecutable --
 *
 *	This procedure computes the absolute path name of the current
 *	application, given its argv[0] value.
 *
 * Results:
 *	A dirty UTF string that is the path to the executable.  At this
 *	point we may not know the system encoding.  Convert the native
 *	string value to UTF using the default encoding.  The assumption
 *	is that we will still be able to parse the path given the path
 *	name contains ASCII string and '/' chars do not conflict with
 *	other UTF chars.
 *
 * Side effects:
 *	The variable tclNativeExecutableName gets filled in with the file
 *	name for the application, if we figured it out.  If we couldn't
 *	figure it out, tclNativeExecutableName is set to NULL.
 *
 *---------------------------------------------------------------------------
 */

char *
TclpFindExecutable(argv0)
    CONST char *argv0;		/* The value of the application's argv[0]
				 * (native). */
{
    CONST char *name, *p;
    struct stat statBuf;
    int length;
    Tcl_DString buffer, nameString;

    if (argv0 == NULL) {
	return NULL;
    }
    if (tclNativeExecutableName != NULL) {
	return tclNativeExecutableName;
    }

    Tcl_DStringInit(&buffer);

    name = argv0;
    for (p = name; *p != '\0'; p++) {
	if (*p == '/') {
	    /*
	     * The name contains a slash, so use the name directly
	     * without doing a path search.
	     */

	    goto gotName;
	}
    }

    p = getenv("PATH");					/* INTL: Native. */
    if (p == NULL) {
	/*
	 * There's no PATH environment variable; use the default that
	 * is used by sh.
	 */

	p = ":/bin:/usr/bin";
    } else if (*p == '\0') {
	/*
	 * An empty path is equivalent to ".".
	 */

	p = "./";
    }

    /*
     * Search through all the directories named in the PATH variable
     * to see if argv[0] is in one of them.  If so, use that file
     * name.
     */

    while (1) {
	while (isspace(UCHAR(*p))) {		/* INTL: BUG */
	    p++;
	}
	name = p;
	while ((*p != ':') && (*p != 0)) {
	    p++;
	}
	Tcl_DStringSetLength(&buffer, 0);
	if (p != name) {
	    Tcl_DStringAppend(&buffer, name, p - name);
	    if (p[-1] != '/') {
		Tcl_DStringAppend(&buffer, "/", 1);
	    }
	}
	name = Tcl_DStringAppend(&buffer, argv0, -1);

	/*
	 * INTL: The following calls to access() and stat() should not be
	 * converted to Tclp routines because they need to operate on native
	 * strings directly.
	 */

	if ((access(name, X_OK) == 0)		/* INTL: Native. */
		&& (stat(name, &statBuf) == 0)	/* INTL: Native. */
		&& S_ISREG(statBuf.st_mode)) {
	    goto gotName;
	}
	if (*p == '\0') {
	    break;
	} else if (*(p+1) == 0) {
	    p = "./";
	} else {
	    p++;
	}
    }
    goto done;

    /*
     * If the name starts with "/" then just copy it to tclExecutableName.
     */

    gotName:
    if (name[0] == '/')  {
	Tcl_ExternalToUtfDString(NULL, name, -1, &nameString);
	tclNativeExecutableName = (char *)
		ckalloc((unsigned) (Tcl_DStringLength(&nameString) + 1));
	strcpy(tclNativeExecutableName, Tcl_DStringValue(&nameString));
	Tcl_DStringFree(&nameString);
	goto done;
    }

    /*
     * The name is relative to the current working directory.  First
     * strip off a leading "./", if any, then add the full path name of
     * the current working directory.
     */

    if ((name[0] == '.') && (name[1] == '/')) {
	name += 2;
    }

    Tcl_ExternalToUtfDString(NULL, name, -1, &nameString);

    Tcl_DStringFree(&buffer);
    TclpGetCwd(NULL, &buffer);

    length = Tcl_DStringLength(&buffer) + Tcl_DStringLength(&nameString) + 2;
    tclNativeExecutableName = (char *) ckalloc((unsigned) length);
    strcpy(tclNativeExecutableName, Tcl_DStringValue(&buffer));
    tclNativeExecutableName[Tcl_DStringLength(&buffer)] = '/';
    strcpy(tclNativeExecutableName + Tcl_DStringLength(&buffer) + 1,
	    Tcl_DStringValue(&nameString));
    Tcl_DStringFree(&nameString);
    
    done:
    Tcl_DStringFree(&buffer);
    return tclNativeExecutableName;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpMatchFilesTypes --
 *
 *	This routine is used by the globbing code to search a
 *	directory for all files which match a given pattern.
 *
 * Results: 
 *	If the tail argument is NULL, then the matching files are
 *	added to the the interp's result.  Otherwise, TclDoGlob is called
 *	recursively for each matching subdirectory.  The return value
 *	is a standard Tcl result indicating whether an error occurred
 *	in globbing.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

int
TclpMatchFilesTypes(interp, separators, dirPtr, pattern, tail, types)
    Tcl_Interp *interp;		/* Interpreter to receive results. */
    char *separators;		/* Directory separators to pass to TclDoGlob */
    Tcl_DString *dirPtr;	/* Contains path to directory to search. */
    char *pattern;		/* Pattern to match against. */
    char *tail;			/* Pointer to end of pattern.  Tail must
				 * point to a location in pattern and must
				 * not be static. */
    GlobTypeData *types;	/* Object containing list of acceptable types.
				 * May be NULL. */
{
    char *native, *fname, *dirName, *patternEnd = tail;
    char savedChar = 0;		/* lint. */
    DIR *d;
    Tcl_DString ds;
    struct stat statBuf;
    int matchHidden;
    int result = TCL_OK;
    int baseLength = Tcl_DStringLength(dirPtr);
    Tcl_Obj *resultPtr;

    /*
     * Make sure that the directory part of the name really is a
     * directory.  If the directory name is "", use the name "."
     * instead, because some UNIX systems don't treat "" like "."
     * automatically.  Keep the "" for use in generating file names,
     * otherwise "glob foo.c" would return "./foo.c".
     */

    if (Tcl_DStringLength(dirPtr) == 0) {
	dirName = ".";
    } else {
	dirName = Tcl_DStringValue(dirPtr);
    }

    if ((TclpStat(dirName, &statBuf) != 0)		/* INTL: UTF-8. */
	    || !S_ISDIR(statBuf.st_mode)) {
	return TCL_OK;
    }

    /*
     * Check to see if the pattern needs to compare with hidden files.
     */

    if ((pattern[0] == '.')
	    || ((pattern[0] == '\\') && (pattern[1] == '.'))) {
	matchHidden = 1;
    } else {
	matchHidden = 0;
    }

    /*
     * Now open the directory for reading and iterate over the contents.
     */

    native = Tcl_UtfToExternalDString(NULL, dirName, -1, &ds);
    d = opendir(native);				/* INTL: Native. */
    Tcl_DStringFree(&ds);
    if (d == NULL) {
	Tcl_ResetResult(interp);

	/*
	 * Strip off a trailing '/' if necessary, before reporting the error.
	 */

	if (baseLength > 0) {
	    savedChar = (Tcl_DStringValue(dirPtr))[baseLength-1];
	    if (savedChar == '/') {
		(Tcl_DStringValue(dirPtr))[baseLength-1] = '\0';
	    }
	}
	Tcl_AppendResult(interp, "couldn't read directory \"",
		Tcl_DStringValue(dirPtr), "\": ",
		Tcl_PosixError(interp), (char *) NULL);
	if (baseLength > 0) {
	    (Tcl_DStringValue(dirPtr))[baseLength-1] = savedChar;
	}
	return TCL_ERROR;
    }

    /*
     * Clean up the end of the pattern and the tail pointer.  Leave
     * the tail pointing to the first character after the path separator
     * following the pattern, or NULL.  Also, ensure that the pattern
     * is null-terminated.
     */

    if (*tail == '\\') {
	tail++;
    }
    if (*tail == '\0') {
	tail = NULL;
    } else {
	tail++;
    }
    savedChar = *patternEnd;
    *patternEnd = '\0';

    resultPtr = Tcl_GetObjResult(interp);
    while (1) {
	char *utf;
	struct dirent *entryPtr;
	
	entryPtr = readdir(d);				/* INTL: Native. */
	if (entryPtr == NULL) {
	    break;
	}

	if (types != NULL && (types->perm & TCL_GLOB_PERM_HIDDEN)) {
	    /* 
	     * We explicitly asked for hidden files, so turn around
	     * and ignore any file which isn't hidden.
	     */
	    if (*entryPtr->d_name != '.') {
	        continue;
	    }
	} else if (!matchHidden && (*entryPtr->d_name == '.')) {
	    /*
	     * Don't match names starting with "." unless the "." is
	     * present in the pattern.
	     */
	    continue;
	}

	/*
	 * Now check to see if the file matches.  If there are more
	 * characters to be processed, then ensure matching files are
	 * directories before calling TclDoGlob. Otherwise, just add
	 * the file to the result.
	 */

	utf = Tcl_ExternalToUtfDString(NULL, entryPtr->d_name, -1, &ds);
	if (Tcl_StringMatch(utf, pattern) != 0) {
	    Tcl_DStringSetLength(dirPtr, baseLength);
	    Tcl_DStringAppend(dirPtr, utf, -1);
	    fname = Tcl_DStringValue(dirPtr);
	    if (tail == NULL) {
		int typeOk = 1;
		if (types != NULL) {
		    if (types->perm != 0) {
			struct stat buf;

			if (TclpStat(fname, &buf) != 0) {
			    panic("stat failed on known file");
			}
			/* 
			 * readonly means that there are NO write permissions
			 * (even for user), but execute is OK for anybody
			 */
			if (
			    ((types->perm & TCL_GLOB_PERM_RONLY) &&
				    (buf.st_mode & (S_IWOTH|S_IWGRP|S_IWUSR))) ||
			    ((types->perm & TCL_GLOB_PERM_R) &&
				    (TclpAccess(fname, R_OK) != 0)) ||
			    ((types->perm & TCL_GLOB_PERM_W) &&
				    (TclpAccess(fname, W_OK) != 0)) ||
			    ((types->perm & TCL_GLOB_PERM_X) &&
				    (TclpAccess(fname, X_OK) != 0))
			    ) {
			    typeOk = 0;
			}
		    }
		    if (typeOk && (types->type != 0)) {
			struct stat buf;
			/*
			 * We must match at least one flag to be listed
			 */
			typeOk = 0;
			if (TclpLstat(fname, &buf) >= 0) {
			    /*
			     * In order bcdpfls as in 'find -t'
			     */
			    if (
				((types->type & TCL_GLOB_TYPE_BLOCK) &&
					S_ISBLK(buf.st_mode)) ||
				((types->type & TCL_GLOB_TYPE_CHAR) &&
					S_ISCHR(buf.st_mode)) ||
				((types->type & TCL_GLOB_TYPE_DIR) &&
					S_ISDIR(buf.st_mode)) ||
				((types->type & TCL_GLOB_TYPE_PIPE) &&
					S_ISFIFO(buf.st_mode)) ||
				((types->type & TCL_GLOB_TYPE_FILE) &&
					S_ISREG(buf.st_mode))
#ifdef S_ISLNK
				|| ((types->type & TCL_GLOB_TYPE_LINK) &&
					S_ISLNK(buf.st_mode))
#endif
#ifdef S_ISSOCK
				|| ((types->type & TCL_GLOB_TYPE_SOCK) &&
					S_ISSOCK(buf.st_mode))
#endif
				) {
				typeOk = 1;
			    }
			} else {
			    /* Posix error occurred */
			}
		    }
		}
		if (typeOk) {
		    Tcl_ListObjAppendElement(interp, resultPtr, 
			    Tcl_NewStringObj(fname,
				    Tcl_DStringLength(dirPtr)));
		}
	    } else if ((TclpStat(fname, &statBuf) == 0)
		    && S_ISDIR(statBuf.st_mode)) {
		Tcl_DStringAppend(dirPtr, "/", 1);
		result = TclDoGlob(interp, separators, dirPtr, tail, types);
		if (result != TCL_OK) {
		    Tcl_DStringFree(&ds);
		    break;
		}
	    }
	}
	Tcl_DStringFree(&ds);
    }
    *patternEnd = savedChar;

    closedir(d);
    return result;
}

/* 
 * TclpMatchFiles --
 * 
 * This function is now obsolete.  Call the above function 
 * 'TclpMatchFilesTypes' instead.
 */
int
TclpMatchFiles(interp, separators, dirPtr, pattern, tail)
    Tcl_Interp *interp;		/* Interpreter to receive results. */
    char *separators;		/* Directory separators to pass to TclDoGlob */
    Tcl_DString *dirPtr;	/* Contains path to directory to search. */
    char *pattern;		/* Pattern to match against. */
    char *tail;			/* Pointer to end of pattern.  Tail must
				 * point to a location in pattern and must
				 * not be static. */
{
    return TclpMatchFilesTypes(interp,separators,dirPtr,pattern,tail,NULL);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpGetUserHome --
 *
 *	This function takes the specified user name and finds their
 *	home directory.
 *
 * Results:
 *	The result is a pointer to a string specifying the user's home
 *	directory, or NULL if the user's home directory could not be
 *	determined.  Storage for the result string is allocated in
 *	bufferPtr; the caller must call Tcl_DStringFree() when the result
 *	is no longer needed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
TclpGetUserHome(name, bufferPtr)
    CONST char *name;		/* User name for desired home directory. */
    Tcl_DString *bufferPtr;	/* Uninitialized or free DString filled
				 * with name of user's home directory. */
{
    struct passwd *pwPtr;
    Tcl_DString ds;
    char *native;

    native = Tcl_UtfToExternalDString(NULL, name, -1, &ds);
    pwPtr = getpwnam(native);				/* INTL: Native. */
    Tcl_DStringFree(&ds);
    
    if (pwPtr == NULL) {
	endpwent();
	return NULL;
    }
    Tcl_ExternalToUtfDString(NULL, pwPtr->pw_dir, -1, bufferPtr);
    endpwent();
    return Tcl_DStringValue(bufferPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpAccess --
 *
 *	This function replaces the library version of access().
 *
 * Results:
 *	See access() documentation.
 *
 * Side effects:
 *	See access() documentation.
 *
 *---------------------------------------------------------------------------
 */

int
TclpAccess(path, mode)
    CONST char *path;		/* Path of file to access (UTF-8). */
    int mode;			/* Permission setting. */
{
    int result;
    Tcl_DString ds;
    char *native;
    
    native = Tcl_UtfToExternalDString(NULL, path, -1, &ds);
    result = access(native, mode);			/* INTL: Native. */
    Tcl_DStringFree(&ds);

    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpChdir --
 *
 *	This function replaces the library version of chdir().
 *
 * Results:
 *	See chdir() documentation.
 *
 * Side effects:
 *	See chdir() documentation.  
 *
 *---------------------------------------------------------------------------
 */

int
TclpChdir(dirName)
    CONST char *dirName;     	/* Path to new working directory (UTF-8). */
{
    int result;
    Tcl_DString ds;
    char *native;

    native = Tcl_UtfToExternalDString(NULL, dirName, -1, &ds);
    result = chdir(native);				/* INTL: Native. */
    Tcl_DStringFree(&ds);

    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclpLstat --
 *
 *	This function replaces the library version of lstat().
 *
 * Results:
 *	See lstat() documentation.
 *
 * Side effects:
 *	See lstat() documentation.
 *
 *----------------------------------------------------------------------
 */

int
TclpLstat(path, bufPtr)
    CONST char *path;		/* Path of file to stat (UTF-8). */
    struct stat *bufPtr;	/* Filled with results of stat call. */
{
    int result;
    Tcl_DString ds;
    char *native;
    
    native = Tcl_UtfToExternalDString(NULL, path, -1, &ds);
    result = lstat(native, bufPtr);			/* INTL: Native. */
    Tcl_DStringFree(&ds);

    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpGetCwd --
 *
 *	This function replaces the library version of getcwd().
 *
 * Results:
 *	The result is a pointer to a string specifying the current
 *	directory, or NULL if the current directory could not be
 *	determined.  If NULL is returned, an error message is left in the
 *	interp's result.  Storage for the result string is allocated in
 *	bufferPtr; the caller must call Tcl_DStringFree() when the result
 *	is no longer needed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

char *
TclpGetCwd(interp, bufferPtr)
    Tcl_Interp *interp;		/* If non-NULL, used for error reporting. */
    Tcl_DString *bufferPtr;	/* Uninitialized or free DString filled
				 * with name of current directory. */
{
    char buffer[MAXPATHLEN+1];

#ifdef USEGETWD
    if (getwd(buffer) == NULL) {			/* INTL: Native. */
#else
    if (getcwd(buffer, MAXPATHLEN + 1) == NULL) {	/* INTL: Native. */
#endif
	if (interp != NULL) {
	    Tcl_AppendResult(interp,
		    "error getting working directory name: ",
		    Tcl_PosixError(interp), (char *) NULL);
	}
	return NULL;
    }
    return Tcl_ExternalToUtfDString(NULL, buffer, -1, bufferPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpReadlink --
 *
 *	This function replaces the library version of readlink().
 *
 * Results:
 *	The result is a pointer to a string specifying the contents
 *	of the symbolic link given by 'path', or NULL if the symbolic
 *	link could not be read.  Storage for the result string is
 *	allocated in bufferPtr; the caller must call Tcl_DStringFree()
 *	when the result is no longer needed.
 *
 * Side effects:
 *	See readlink() documentation.
 *
 *---------------------------------------------------------------------------
 */

char *
TclpReadlink(path, linkPtr)
    CONST char *path;		/* Path of file to readlink (UTF-8). */
    Tcl_DString *linkPtr;	/* Uninitialized or free DString filled
				 * with contents of link (UTF-8). */
{
    char link[MAXPATHLEN];
    int length;
    char *native;
    Tcl_DString ds;

    native = Tcl_UtfToExternalDString(NULL, path, -1, &ds);
    length = readlink(native, link, sizeof(link));	/* INTL: Native. */
    Tcl_DStringFree(&ds);
    
    if (length < 0) {
	return NULL;
    }

    Tcl_ExternalToUtfDString(NULL, link, length, linkPtr);
    return Tcl_DStringValue(linkPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * TclpStat --
 *
 *	This function replaces the library version of stat().
 *
 * Results:
 *	See stat() documentation.
 *
 * Side effects:
 *	See stat() documentation.
 *
 *----------------------------------------------------------------------
 */

int
TclpStat(path, bufPtr)
    CONST char *path;		/* Path of file to stat (in UTF-8). */
    struct stat *bufPtr;	/* Filled with results of stat call. */
{
    int result;
    Tcl_DString ds;
    char *native;
    
    native = Tcl_UtfToExternalDString(NULL, path, -1, &ds);
    result = stat(native, bufPtr);			/* INTL: Native. */
    Tcl_DStringFree(&ds);

    return result;
}

