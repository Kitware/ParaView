/*
 * tclFCmd.c
 *
 *      This file implements the generic portion of file manipulation 
 *      subcommands of the "file" command. 
 *
 * Copyright (c) 1996-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclPort.h"

/*
 * Declarations for local procedures defined in this file:
 */

static int		CopyRenameOneFile _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *srcPathPtr, Tcl_Obj *destPathPtr, 
			    int copyFlag, int force));
static Tcl_Obj *	FileBasename _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *pathPtr));
static int		FileCopyRename _ANSI_ARGS_((Tcl_Interp *interp,
			    int objc, Tcl_Obj *CONST objv[], int copyFlag));
static int		FileForceOption _ANSI_ARGS_((Tcl_Interp *interp,
			    int objc, Tcl_Obj *CONST objv[], int *forcePtr));

/*
 *---------------------------------------------------------------------------
 *
 * TclFileRenameCmd
 *
 *	This procedure implements the "rename" subcommand of the "file"
 *      command.  Filename arguments need to be translated to native
 *	format before being passed to platform-specific code that
 *	implements rename functionality.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

int
TclFileRenameCmd(interp, objc, objv)
    Tcl_Interp *interp;		/* Interp for error reporting. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument strings passed to Tcl_FileCmd. */
{
    return FileCopyRename(interp, objc, objv, 0);
}

/*
 *---------------------------------------------------------------------------
 *
 * TclFileCopyCmd
 *
 *	This procedure implements the "copy" subcommand of the "file"
 *	command.  Filename arguments need to be translated to native
 *	format before being passed to platform-specific code that
 *	implements copy functionality.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	See the user documentation.
 *
 *---------------------------------------------------------------------------
 */

int
TclFileCopyCmd(interp, objc, objv)
    Tcl_Interp *interp;		/* Used for error reporting */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument strings passed to Tcl_FileCmd. */
{
    return FileCopyRename(interp, objc, objv, 1);
}

/*
 *---------------------------------------------------------------------------
 *
 * FileCopyRename --
 *
 *	Performs the work of TclFileRenameCmd and TclFileCopyCmd.
 *	See comments for those procedures.
 *
 * Results:
 *	See above.
 *
 * Side effects:
 *	See above.
 *
 *---------------------------------------------------------------------------
 */

static int
FileCopyRename(interp, objc, objv, copyFlag)
    Tcl_Interp *interp;		/* Used for error reporting. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument strings passed to Tcl_FileCmd. */
    int copyFlag;		/* If non-zero, copy source(s).  Otherwise,
				 * rename them. */
{
    int i, result, force;
    Tcl_StatBuf statBuf; 
    Tcl_Obj *target;

    i = FileForceOption(interp, objc - 2, objv + 2, &force);
    if (i < 0) {
	return TCL_ERROR;
    }
    i += 2;
    if ((objc - i) < 2) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " ", Tcl_GetString(objv[1]), 
		" ?options? source ?source ...? target\"", 
		(char *) NULL);
	return TCL_ERROR;
    }

    /*
     * If target doesn't exist or isn't a directory, try the copy/rename.
     * More than 2 arguments is only valid if the target is an existing
     * directory.
     */

    target = objv[objc - 1];
    if (Tcl_FSConvertToPathType(interp, target) != TCL_OK) {
	return TCL_ERROR;
    }

    result = TCL_OK;

    /*
     * Call Tcl_FSStat() so that if target is a symlink that points to a
     * directory we will put the sources in that directory instead of
     * overwriting the symlink.
     */

    if ((Tcl_FSStat(target, &statBuf) != 0) || !S_ISDIR(statBuf.st_mode)) {
	if ((objc - i) > 2) {
	    errno = ENOTDIR;
	    Tcl_PosixError(interp);
	    Tcl_AppendResult(interp, "error ",
		    ((copyFlag) ? "copying" : "renaming"), ": target \"",
		    Tcl_GetString(target), "\" is not a directory", 
		    (char *) NULL);
	    result = TCL_ERROR;
	} else {
	    /*
	     * Even though already have target == translated(objv[i+1]),
	     * pass the original argument down, so if there's an error, the
	     * error message will reflect the original arguments.
	     */

	    result = CopyRenameOneFile(interp, objv[i], objv[i + 1], copyFlag,
		    force);
	}
	return result;
    }
    
    /*
     * Move each source file into target directory.  Extract the basename
     * from each source, and append it to the end of the target path.
     */

    for ( ; i < objc - 1; i++) {
	Tcl_Obj *jargv[2];
	Tcl_Obj *source, *newFileName;
	Tcl_Obj *temp;
	
	source = FileBasename(interp, objv[i]);
	if (source == NULL) {
	    result = TCL_ERROR;
	    break;
	}
	jargv[0] = objv[objc - 1];
	jargv[1] = source;
	temp = Tcl_NewListObj(2, jargv);
	newFileName = Tcl_FSJoinPath(temp, -1);
	Tcl_IncrRefCount(newFileName);
	result = CopyRenameOneFile(interp, objv[i], newFileName, copyFlag,
		force);
	Tcl_DecrRefCount(newFileName);
	Tcl_DecrRefCount(temp);
	Tcl_DecrRefCount(source);

	if (result == TCL_ERROR) {
	    break;
	}
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclFileMakeDirsCmd
 *
 *	This procedure implements the "mkdir" subcommand of the "file"
 *      command.  Filename arguments need to be translated to native
 *	format before being passed to platform-specific code that
 *	implements mkdir functionality.
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
TclFileMakeDirsCmd(interp, objc, objv)
    Tcl_Interp *interp;		/* Used for error reporting. */
    int objc;			/* Number of arguments */
    Tcl_Obj *CONST objv[];	/* Argument strings passed to Tcl_FileCmd. */
{
    Tcl_Obj *errfile;
    int result, i, j, pobjc;
    Tcl_Obj *split = NULL;
    Tcl_Obj *target = NULL;
    Tcl_StatBuf statBuf;

    errfile = NULL;

    result = TCL_OK;
    for (i = 2; i < objc; i++) {
	if (Tcl_FSConvertToPathType(interp, objv[i]) != TCL_OK) {
	    result = TCL_ERROR;
	    break;
	}

	split = Tcl_FSSplitPath(objv[i],&pobjc);
	if (pobjc == 0) {
	    errno = ENOENT;
	    errfile = objv[i];
	    break;
	}
	for (j = 0; j < pobjc; j++) {
	    target = Tcl_FSJoinPath(split, j + 1);
	    Tcl_IncrRefCount(target);
	    /*
	     * Call Tcl_FSStat() so that if target is a symlink that
	     * points to a directory we will create subdirectories in
	     * that directory.
	     */

	    if (Tcl_FSStat(target, &statBuf) == 0) {
		if (!S_ISDIR(statBuf.st_mode)) {
		    errno = EEXIST;
		    errfile = target;
		    goto done;
		}
	    } else if ((errno != ENOENT)
		    || (Tcl_FSCreateDirectory(target) != TCL_OK)) {
		errfile = target;
		goto done;
	    }
	    /* Forget about this sub-path */
	    Tcl_DecrRefCount(target);
	    target = NULL;
	}
	Tcl_DecrRefCount(split);
	split = NULL;
    }
	
    done:
    if (errfile != NULL) {
	Tcl_AppendResult(interp, "can't create directory \"",
		Tcl_GetString(errfile), "\": ", Tcl_PosixError(interp), 
		(char *) NULL);
	result = TCL_ERROR;
    }
    if (split != NULL) {
	Tcl_DecrRefCount(split);
    }
    if (target != NULL) {
	Tcl_DecrRefCount(target);
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFileDeleteCmd
 *
 *	This procedure implements the "delete" subcommand of the "file"
 *      command.
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
TclFileDeleteCmd(interp, objc, objv)
    Tcl_Interp *interp;		/* Used for error reporting */
    int objc;			/* Number of arguments */
    Tcl_Obj *CONST objv[];	/* Argument strings passed to Tcl_FileCmd. */
{
    int i, force, result;
    Tcl_Obj *errfile;
    Tcl_Obj *errorBuffer = NULL;
    
    i = FileForceOption(interp, objc - 2, objv + 2, &force);
    if (i < 0) {
	return TCL_ERROR;
    }
    i += 2;
    if ((objc - i) < 1) {
	Tcl_AppendResult(interp, "wrong # args: should be \"", 
		Tcl_GetString(objv[0]), " ", Tcl_GetString(objv[1]), 
		" ?options? file ?file ...?\"", (char *) NULL);
	return TCL_ERROR;
    }

    errfile = NULL;
    result = TCL_OK;

    for ( ; i < objc; i++) {
	Tcl_StatBuf statBuf;

	errfile = objv[i];
	if (Tcl_FSConvertToPathType(interp, objv[i]) != TCL_OK) {
	    result = TCL_ERROR;
	    goto done;
	}

	/*
	 * Call lstat() to get info so can delete symbolic link itself.
	 */

	if (Tcl_FSLstat(objv[i], &statBuf) != 0) {
	    /*
	     * Trying to delete a file that does not exist is not
	     * considered an error, just a no-op
	     */

	    if (errno != ENOENT) {
		result = TCL_ERROR;
	    }
	} else if (S_ISDIR(statBuf.st_mode)) {
	    /* 
	     * We own a reference count on errorBuffer, if it was set
	     * as a result of this call. 
	     */
	    result = Tcl_FSRemoveDirectory(objv[i], force, &errorBuffer);
	    if (result != TCL_OK) {
		if ((force == 0) && (errno == EEXIST)) {
		    Tcl_AppendResult(interp, "error deleting \"", 
			    Tcl_GetString(objv[i]),
			    "\": directory not empty", (char *) NULL);
		    Tcl_PosixError(interp);
		    goto done;
		}

		/* 
		 * If possible, use the untranslated name for the file.
		 */
		 
		errfile = errorBuffer;
		/* FS supposed to check between translated objv and errfile */
		if (Tcl_FSEqualPaths(objv[i], errfile)) {
		    errfile = objv[i];
		}
	    }
	} else {
	    result = Tcl_FSDeleteFile(objv[i]);
	}
	
	if (result != TCL_OK) {
	    result = TCL_ERROR;
	    /* 
	     * It is important that we break on error, otherwise we
	     * might end up owning reference counts on numerous
	     * errorBuffers.
	     */
	    break;
	}
    }
    if (result != TCL_OK) {
	if (errfile == NULL) {
	    /* 
	     * We try to accomodate poor error results from our 
	     * Tcl_FS calls 
	     */
	    Tcl_AppendResult(interp, "error deleting unknown file: ", 
		    Tcl_PosixError(interp), (char *) NULL);
	} else {
	    Tcl_AppendResult(interp, "error deleting \"", 
		    Tcl_GetString(errfile), "\": ", 
		    Tcl_PosixError(interp), (char *) NULL);
	}
    } 
    done:
    if (errorBuffer != NULL) {
	Tcl_DecrRefCount(errorBuffer);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * CopyRenameOneFile
 *
 *	Copies or renames specified source file or directory hierarchy
 *	to the specified target.  
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Target is overwritten if the force flag is set.  Attempting to
 *	copy/rename a file onto a directory or a directory onto a file
 *	will always result in an error.  
 *
 *----------------------------------------------------------------------
 */

static int
CopyRenameOneFile(interp, source, target, copyFlag, force) 
    Tcl_Interp *interp;		/* Used for error reporting. */
    Tcl_Obj *source;		/* Pathname of file to copy.  May need to
				 * be translated. */
    Tcl_Obj *target;		/* Pathname of file to create/overwrite.
				 * May need to be translated. */
    int copyFlag;		/* If non-zero, copy files.  Otherwise,
				 * rename them. */
    int force;			/* If non-zero, overwrite target file if it
				 * exists.  Otherwise, error if target already
				 * exists. */
{
    int result;
    Tcl_Obj *errfile, *errorBuffer;
    /* If source is a link, then this is the real file/directory */
    Tcl_Obj *actualSource = NULL;
    Tcl_StatBuf sourceStatBuf, targetStatBuf;

    if (Tcl_FSConvertToPathType(interp, source) != TCL_OK) {
	return TCL_ERROR;
    }
    if (Tcl_FSConvertToPathType(interp, target) != TCL_OK) {
	return TCL_ERROR;
    }
    
    errfile = NULL;
    errorBuffer = NULL;
    result = TCL_ERROR;
    
    /*
     * We want to copy/rename links and not the files they point to, so we
     * use lstat(). If target is a link, we also want to replace the 
     * link and not the file it points to, so we also use lstat() on the
     * target.
     */

    if (Tcl_FSLstat(source, &sourceStatBuf) != 0) {
	errfile = source;
	goto done;
    }
    if (Tcl_FSLstat(target, &targetStatBuf) != 0) {
	if (errno != ENOENT) {
	    errfile = target;
	    goto done;
	}
    } else {
	if (force == 0) {
	    errno = EEXIST;
	    errfile = target;
	    goto done;
	}

        /* 
         * Prevent copying or renaming a file onto itself.  Under Windows, 
         * stat always returns 0 for st_ino.  However, the Windows-specific 
         * code knows how to deal with copying or renaming a file on top of
         * itself.  It might be a good idea to write a stat that worked.
         */
     
        if ((sourceStatBuf.st_ino != 0) && (targetStatBuf.st_ino != 0)) {
            if ((sourceStatBuf.st_ino == targetStatBuf.st_ino) &&
            	    (sourceStatBuf.st_dev == targetStatBuf.st_dev)) {
            	result = TCL_OK;
            	goto done;
            }
        }

	/*
	 * Prevent copying/renaming a file onto a directory and
	 * vice-versa.  This is a policy decision based on the fact that
	 * existing implementations of copy and rename on all platforms
	 * also prevent this.
	 */

	if (S_ISDIR(sourceStatBuf.st_mode)
                && !S_ISDIR(targetStatBuf.st_mode)) {
	    errno = EISDIR;
	    Tcl_AppendResult(interp, "can't overwrite file \"", 
		    Tcl_GetString(target), "\" with directory \"", 
		    Tcl_GetString(source), "\"", (char *) NULL);
	    goto done;
	}
	if (!S_ISDIR(sourceStatBuf.st_mode)
	        && S_ISDIR(targetStatBuf.st_mode)) {
	    errno = EISDIR;
	    Tcl_AppendResult(interp, "can't overwrite directory \"", 
		    Tcl_GetString(target), "\" with file \"", 
		    Tcl_GetString(source), "\"", (char *) NULL);
	    goto done;
	}
    }

    if (copyFlag == 0) {
	result = Tcl_FSRenameFile(source, target);
	if (result == TCL_OK) {
	    goto done;
	}
	    
	if (errno == EINVAL) {
	    Tcl_AppendResult(interp, "error renaming \"", 
		    Tcl_GetString(source), "\" to \"",
		    Tcl_GetString(target), "\": trying to rename a volume or ",
		    "move a directory into itself", (char *) NULL);
	    goto done;
	} else if (errno != EXDEV) {
	    errfile = target;
	    goto done;
	}
	
	/*
	 * The rename failed because the move was across file systems.
	 * Fall through to copy file and then remove original.  Note that
	 * the low-level Tcl_FSRenameFileProc in the filesystem is allowed 
	 * to implement cross-filesystem moves itself, if it desires.
	 */
    }

    actualSource = source;
    Tcl_IncrRefCount(actualSource);
#if 0
#ifdef S_ISLNK
    /* 
     * To add a flag to make 'copy' copy links instead of files, we could
     * add a condition to ignore this 'if' here.
     */
    if (copyFlag && S_ISLNK(sourceStatBuf.st_mode)) {
	/* 
	 * We want to copy files not links.  Therefore we must follow the
	 * link.  There are two purposes to this 'stat' call here.  First
	 * we want to know if the linked-file/dir actually exists, and
	 * second, in the block of code which follows, some 20 lines
	 * down, we want to check if the thing is a file or directory.
	 */
	if (Tcl_FSStat(source, &sourceStatBuf) != 0) {
	    /* Actual file doesn't exist */
	    Tcl_AppendResult(interp, 
		    "error copying \"", Tcl_GetString(source), 
		    "\": the target of this link doesn't exist",
		    (char *) NULL);
	    goto done;
	} else {
	    int counter = 0;
	    while (1) {
		Tcl_Obj *path = Tcl_FSLink(actualSource, NULL, 0);
		if (path == NULL) {
		    break;
		}
		Tcl_DecrRefCount(actualSource);
		actualSource = path;
		counter++;
		/* Arbitrary limit of 20 links to follow */
		if (counter > 20) {
		    /* Too many links */
		    Tcl_SetErrno(EMLINK);
		    errfile = source;
		    goto done;
		}
	    }
	    /* Now 'actualSource' is the correct file */
	}
    }
#endif
#endif

    if (S_ISDIR(sourceStatBuf.st_mode)) {
	result = Tcl_FSCopyDirectory(actualSource, target, &errorBuffer);
	if (result != TCL_OK) {
	    if (errno == EXDEV) {
		/* 
		 * The copy failed because we're trying to do a
		 * cross-filesystem copy.  We do this through our Tcl
		 * library.
		 */
		Tcl_SavedResult savedResult;
		Tcl_Obj *copyCommand = Tcl_NewListObj(0,NULL);
		Tcl_IncrRefCount(copyCommand);
		Tcl_ListObjAppendElement(interp, copyCommand, 
			Tcl_NewStringObj("::tcl::CopyDirectory",-1));
		if (copyFlag) {
		    Tcl_ListObjAppendElement(interp, copyCommand, 
					     Tcl_NewStringObj("copying",-1));
		} else {
		    Tcl_ListObjAppendElement(interp, copyCommand, 
					     Tcl_NewStringObj("renaming",-1));
		}
		Tcl_ListObjAppendElement(interp, copyCommand, source);
		Tcl_ListObjAppendElement(interp, copyCommand, target);
		Tcl_SaveResult(interp, &savedResult);
		result = Tcl_EvalObjEx(interp, copyCommand, 
				       TCL_EVAL_GLOBAL | TCL_EVAL_DIRECT);
		Tcl_DecrRefCount(copyCommand);
		if (result != TCL_OK) {
		    /* 
		     * There was an error in the Tcl-level copy.
		     * We will pass on the Tcl error message and
		     * can ensure this by setting errfile to NULL
		     */
		    Tcl_DiscardResult(&savedResult);
		    errfile = NULL;
		} else {
		    /* The copy was successful */
		    Tcl_RestoreResult(interp, &savedResult);
		}
	    } else {
		errfile = errorBuffer;
		if (Tcl_FSEqualPaths(errfile, source)) {
		    errfile = source;
		} else if (Tcl_FSEqualPaths(errfile, target)) {
		    errfile = target;
		}
	    }
	}
    } else {
	result = Tcl_FSCopyFile(actualSource, target);
	if ((result != TCL_OK) && (errno == EXDEV)) {
	    result = TclCrossFilesystemCopy(interp, source, target);
	}
	if (result != TCL_OK) {
	    /* 
	     * We could examine 'errno' to double-check if the problem
	     * was with the target, but we checked the source above,
	     * so it should be quite clear 
	     */
	    errfile = target;
	    /* 
	     * We now need to reset the result, because the above call,
	     * if it failed, may have put an error message in place.
	     * (Ideally we would prefer not to pass an interpreter in
	     * above, but the channel IO code used by
	     * TclCrossFilesystemCopy currently requires one)
	     */
	    Tcl_ResetResult(interp);
	}
    }
    if ((copyFlag == 0) && (result == TCL_OK)) {
	if (S_ISDIR(sourceStatBuf.st_mode)) {
	    result = Tcl_FSRemoveDirectory(source, 1, &errorBuffer);
	    if (result != TCL_OK) {
		if (Tcl_FSEqualPaths(errfile, source) == 0) {
		    errfile = source;
		}
	    }
	} else {
	    result = Tcl_FSDeleteFile(source);
	    if (result != TCL_OK) {
		errfile = source;
	    }
	}
	if (result != TCL_OK) {
	    Tcl_AppendResult(interp, "can't unlink \"", 
		Tcl_GetString(errfile), "\": ",
		Tcl_PosixError(interp), (char *) NULL);
	    errfile = NULL;
	}
    }
    
    done:
    if (errfile != NULL) {
	Tcl_AppendResult(interp, 
		((copyFlag) ? "error copying \"" : "error renaming \""),
		 Tcl_GetString(source), (char *) NULL);
	if (errfile != source) {
	    Tcl_AppendResult(interp, "\" to \"", Tcl_GetString(target), 
			     (char *) NULL);
	    if (errfile != target) {
		Tcl_AppendResult(interp, "\": \"", Tcl_GetString(errfile), 
				 (char *) NULL);
	    }
	}
	Tcl_AppendResult(interp, "\": ", Tcl_PosixError(interp),
		(char *) NULL);
    }
    if (errorBuffer != NULL) {
        Tcl_DecrRefCount(errorBuffer);
    }
    if (actualSource != NULL) {
	Tcl_DecrRefCount(actualSource);
    }
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * FileForceOption --
 *
 *	Helps parse command line options for file commands that take
 *	the "-force" and "--" options.
 *
 * Results:
 *	The return value is how many arguments from argv were consumed
 *	by this function, or -1 if there was an error parsing the
 *	options.  If an error occurred, an error message is left in the
 *	interp's result.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static int
FileForceOption(interp, objc, objv, forcePtr)
    Tcl_Interp *interp;		/* Interp, for error return. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument strings.  First command line
				 * option, if it exists, begins at 0. */
    int *forcePtr;		/* If the "-force" was specified, *forcePtr
				 * is filled with 1, otherwise with 0. */
{
    int force, i;
    
    force = 0;
    for (i = 0; i < objc; i++) {
	if (Tcl_GetString(objv[i])[0] != '-') {
	    break;
	}
	if (strcmp(Tcl_GetString(objv[i]), "-force") == 0) {
	    force = 1;
	} else if (strcmp(Tcl_GetString(objv[i]), "--") == 0) {
	    i++;
	    break;
	} else {
	    Tcl_AppendResult(interp, "bad option \"", Tcl_GetString(objv[i]), 
		    "\": should be -force or --", (char *)NULL);
	    return -1;
	}
    }
    *forcePtr = force;
    return i;
}
/*
 *---------------------------------------------------------------------------
 *
 * FileBasename --
 *
 *	Given a path in either tcl format (with / separators), or in the
 *	platform-specific format for the current platform, return all the
 *	characters in the path after the last directory separator.  But,
 *	if path is the root directory, returns no characters.
 *
 * Results:
 *	Returns the string object that represents the basename.  If there 
 *	is an error, an error message is left in interp, and NULL is 
 *	returned.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static Tcl_Obj *
FileBasename(interp, pathPtr)
    Tcl_Interp *interp;		/* Interp, for error return. */
    Tcl_Obj *pathPtr;		/* Path whose basename to extract. */
{
    int objc;
    Tcl_Obj *splitPtr;
    Tcl_Obj *resultPtr = NULL;
    
    splitPtr = Tcl_FSSplitPath(pathPtr, &objc);

    if (objc != 0) {
	if ((objc == 1) && (*Tcl_GetString(pathPtr) == '~')) {
	    Tcl_DecrRefCount(splitPtr);
	    if (Tcl_FSConvertToPathType(interp, pathPtr) != TCL_OK) {
		return NULL;
	    }
	    splitPtr = Tcl_FSSplitPath(pathPtr, &objc);
	}

	/*
	 * Return the last component, unless it is the only component, and it
	 * is the root of an absolute path.
	 */

	if (objc > 0) {
	    Tcl_ListObjIndex(NULL, splitPtr, objc-1, &resultPtr);
	    if ((objc == 1) &&
	      (Tcl_FSGetPathType(resultPtr) != TCL_PATH_RELATIVE)) {
		resultPtr = NULL;
	    }
	}
    }
    if (resultPtr == NULL) {
	resultPtr = Tcl_NewObj();
    }
    Tcl_IncrRefCount(resultPtr);
    Tcl_DecrRefCount(splitPtr);
    return resultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * TclFileAttrsCmd --
 *
 *      Sets or gets the platform-specific attributes of a file.  The
 *      objc-objv points to the file name with the rest of the command
 *      line following.  This routine uses platform-specific tables of
 *      option strings and callbacks.  The callback to get the
 *      attributes take three parameters:
 *	    Tcl_Interp *interp;	    The interp to report errors with.
 *				    Since this is an object-based API,
 *				    the object form of the result should 
 *				    be used.
 *	    CONST char *fileName;   This is extracted using
 *				    Tcl_TranslateFileName.
 *	    TclObj **attrObjPtrPtr; A new object to hold the attribute
 *				    is allocated and put here.
 *	The first two parameters of the callback used to write out the
 *	attributes are the same. The third parameter is:
 *	    CONST *attrObjPtr;	    A pointer to the object that has
 *				    the new attribute.
 *	They both return standard TCL errors; if the routine to get
 *	an attribute fails, no object is allocated and *attrObjPtrPtr
 *	is unchanged.
 *
 * Results:
 *      Standard TCL error.
 *
 * Side effects:
 *      May set file attributes for the file name.
 *      
 *----------------------------------------------------------------------
 */

int
TclFileAttrsCmd(interp, objc, objv)
    Tcl_Interp *interp;		/* The interpreter for error reporting. */
    int objc;			/* Number of command line arguments. */
    Tcl_Obj *CONST objv[];	/* The command line objects. */
{
    int result;
    CONST char ** attributeStrings;
    Tcl_Obj* objStrings = NULL;
    int numObjStrings = -1;
    Tcl_Obj *filePtr;
    
    if (objc < 3) {
	Tcl_WrongNumArgs(interp, 2, objv,
		"name ?option? ?value? ?option value ...?");
	return TCL_ERROR;
    }

    filePtr = objv[2];
    if (Tcl_FSConvertToPathType(interp, filePtr) != TCL_OK) {
    	return TCL_ERROR;
    }
    
    objc -= 3;
    objv += 3;
    result = TCL_ERROR;
    Tcl_SetErrno(0);
    attributeStrings = Tcl_FSFileAttrStrings(filePtr, &objStrings);
    if (attributeStrings == NULL) {
	int index;
	Tcl_Obj *objPtr;
	if (objStrings == NULL) {
	    if (Tcl_GetErrno() != 0) {
		/* 
		 * There was an error, probably that the filePtr is
		 * not accepted by any filesystem
		 */
		Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), 
			"could not read \"", Tcl_GetString(filePtr), 
			"\": ", Tcl_PosixError(interp), 
			(char *) NULL);
		return TCL_ERROR;
	    }
	    goto end;
	}
	/* We own the object now */
	Tcl_IncrRefCount(objStrings);
        /* Use objStrings as a list object */
	if (Tcl_ListObjLength(interp, objStrings, &numObjStrings) != TCL_OK) {
	    goto end;
	}
	attributeStrings = (CONST char **)
		ckalloc ((1+numObjStrings) * sizeof(char*));
	for (index = 0; index < numObjStrings; index++) {
	    Tcl_ListObjIndex(interp, objStrings, index, &objPtr);
	    attributeStrings[index] = Tcl_GetString(objPtr);
	}
	attributeStrings[index] = NULL;
    }
    if (objc == 0) {
	/*
	 * Get all attributes.
	 */

	int index;
	Tcl_Obj *listPtr;
	 
	listPtr = Tcl_NewListObj(0, NULL);
	for (index = 0; attributeStrings[index] != NULL; index++) {
	    Tcl_Obj *objPtr = Tcl_NewStringObj(attributeStrings[index], -1);
	    Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	    /* We now forget about objPtr, it is in the list */
	    objPtr = NULL;
	    if (Tcl_FSFileAttrsGet(interp, index, filePtr,
		    &objPtr) != TCL_OK) {
		Tcl_DecrRefCount(listPtr);
		goto end;
	    }
	    Tcl_ListObjAppendElement(interp, listPtr, objPtr);
	}
    	Tcl_SetObjResult(interp, listPtr);
    } else if (objc == 1) {
	/*
	 * Get one attribute.
	 */

	int index;
	Tcl_Obj *objPtr = NULL;

	if (numObjStrings == 0) {
	    Tcl_AppendResult(interp, "bad option \"",
		    Tcl_GetString(objv[0]), "\", there are no file attributes"
			     " in this filesystem.", (char *) NULL);
	    goto end;
	}

	if (Tcl_GetIndexFromObj(interp, objv[0], attributeStrings,
		"option", 0, &index) != TCL_OK) {
	    goto end;
	}
	if (Tcl_FSFileAttrsGet(interp, index, filePtr,
		&objPtr) != TCL_OK) {
	    goto end;
	}
	Tcl_SetObjResult(interp, objPtr);
    } else {
	/*
	 * Set option/value pairs.
	 */

	int i, index;
        
	if (numObjStrings == 0) {
	    Tcl_AppendResult(interp, "bad option \"",
		    Tcl_GetString(objv[0]), "\", there are no file attributes"
			     " in this filesystem.", (char *) NULL);
	    goto end;
	}

    	for (i = 0; i < objc ; i += 2) {
    	    if (Tcl_GetIndexFromObj(interp, objv[i], attributeStrings,
		    "option", 0, &index) != TCL_OK) {
		goto end;
    	    }
	    if (i + 1 == objc) {
		Tcl_AppendResult(interp, "value for \"",
			Tcl_GetString(objv[i]), "\" missing",
			(char *) NULL);
		goto end;
	    }
    	    if (Tcl_FSFileAttrsSet(interp, index, filePtr,
    	    	    objv[i + 1]) != TCL_OK) {
		goto end;
    	    }
    	}
    }
    result = TCL_OK;

    end:
    if (numObjStrings != -1) {
	/* Free up the array we allocated */
	ckfree((char*)attributeStrings);
	/* 
	 * We don't need this object that was passed to us
	 * any more.
	 */
	if (objStrings != NULL) {
	    Tcl_DecrRefCount(objStrings);
	}
    }
    return result;
}
