/*
 * tclUnixFCmd.c
 *
 *      This file implements the unix specific portion of file manipulation 
 *      subcommands of the "file" command.  All filename arguments should
 *	already be translated to native format.
 *
 * Copyright (c) 1996-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 *
 * Portions of this code were derived from NetBSD source code which has
 * the following copyright notice:
 *
 * Copyright (c) 1988, 1993, 1994
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "tclInt.h"
#include "tclPort.h"
#include <utime.h>
#include <grp.h>
#ifndef HAVE_ST_BLKSIZE
#ifndef NO_FSTATFS
#include <sys/statfs.h>
#endif
#endif

/*
 * The following constants specify the type of callback when
 * TraverseUnixTree() calls the traverseProc()
 */

#define DOTREE_PRED   1     /* pre-order directory  */
#define DOTREE_POSTD  2     /* post-order directory */
#define DOTREE_F      3     /* regular file */

/*
 * Callbacks for file attributes code.
 */

static int		GetGroupAttribute _ANSI_ARGS_((Tcl_Interp *interp,
			    int objIndex, Tcl_Obj *fileName,
			    Tcl_Obj **attributePtrPtr));
static int		GetOwnerAttribute _ANSI_ARGS_((Tcl_Interp *interp,
			    int objIndex, Tcl_Obj *fileName,
			    Tcl_Obj **attributePtrPtr));
static int		GetPermissionsAttribute _ANSI_ARGS_((
			    Tcl_Interp *interp, int objIndex,
			    Tcl_Obj *fileName, Tcl_Obj **attributePtrPtr));
static int		SetGroupAttribute _ANSI_ARGS_((Tcl_Interp *interp,
			    int objIndex, Tcl_Obj *fileName,
			    Tcl_Obj *attributePtr));
static int		SetOwnerAttribute _ANSI_ARGS_((Tcl_Interp *interp,
			    int objIndex, Tcl_Obj *fileName,
			    Tcl_Obj *attributePtr));
static int		SetPermissionsAttribute _ANSI_ARGS_((
			    Tcl_Interp *interp, int objIndex,
			    Tcl_Obj *fileName, Tcl_Obj *attributePtr));
static int		GetModeFromPermString _ANSI_ARGS_((
			    Tcl_Interp *interp, char *modeStringPtr,
			    mode_t *modePtr));

/*
 * Prototype for the TraverseUnixTree callback function.
 */

typedef int (TraversalProc) _ANSI_ARGS_((Tcl_DString *srcPtr,
	Tcl_DString *dstPtr, CONST Tcl_StatBuf *statBufPtr, int type,
	Tcl_DString *errorPtr));

/*
 * Constants and variables necessary for file attributes subcommand.
 */

enum {
    UNIX_GROUP_ATTRIBUTE,
    UNIX_OWNER_ATTRIBUTE,
    UNIX_PERMISSIONS_ATTRIBUTE
};

CONST char *tclpFileAttrStrings[] = {
    "-group",
    "-owner",
    "-permissions",
    (char *) NULL
};

CONST TclFileAttrProcs tclpFileAttrProcs[] = {
    {GetGroupAttribute,		SetGroupAttribute},
    {GetOwnerAttribute,		SetOwnerAttribute},
    {GetPermissionsAttribute,	SetPermissionsAttribute}
};

/*
 * Declarations for local procedures defined in this file:
 */

static int		CopyFile _ANSI_ARGS_((CONST char *src,
			    CONST char *dst, CONST Tcl_StatBuf *statBufPtr));
static int		CopyFileAtts _ANSI_ARGS_((CONST char *src,
			    CONST char *dst, CONST Tcl_StatBuf *statBufPtr));
static int		DoCopyFile _ANSI_ARGS_((CONST char *srcPtr,
			    CONST char *dstPtr));
static int		DoCreateDirectory _ANSI_ARGS_((CONST char *pathPtr));
static int		DoRemoveDirectory _ANSI_ARGS_((Tcl_DString *pathPtr,
			    int recursive, Tcl_DString *errorPtr));
static int		DoRenameFile _ANSI_ARGS_((CONST char *src,
			    CONST char *dst));
static int		TraversalCopy _ANSI_ARGS_((Tcl_DString *srcPtr,
			    Tcl_DString *dstPtr, CONST Tcl_StatBuf *statBufPtr,
			    int type, Tcl_DString *errorPtr));
static int		TraversalDelete _ANSI_ARGS_((Tcl_DString *srcPtr,
			    Tcl_DString *dstPtr, CONST Tcl_StatBuf *statBufPtr,
			    int type, Tcl_DString *errorPtr));
static int		TraverseUnixTree _ANSI_ARGS_((
			    TraversalProc *traversalProc,
			    Tcl_DString *sourcePtr, Tcl_DString *destPtr,
			    Tcl_DString *errorPtr));

#ifdef PURIFY
/*
 * realpath and purify don't mix happily.  It has been noted that realpath
 * should not be used with purify because of bogus warnings, but just
 * memset'ing the resolved path will squelch those.  This assumes we are
 * passing the standard MAXPATHLEN size resolved arg.
 */
static char *		Realpath _ANSI_ARGS_((CONST char *path,
			    char *resolved));

char *
Realpath(path, resolved)
    CONST char *path;
    char *resolved;
{
    memset(resolved, 0, MAXPATHLEN);
    return realpath(path, resolved);
}
#else
#define Realpath realpath
#endif


/*
 *---------------------------------------------------------------------------
 *
 * TclpObjRenameFile, DoRenameFile --
 *
 *      Changes the name of an existing file or directory, from src to dst.
 *	If src and dst refer to the same file or directory, does nothing
 *	and returns success.  Otherwise if dst already exists, it will be
 *	deleted and replaced by src subject to the following conditions:
 *	    If src is a directory, dst may be an empty directory.
 *	    If src is a file, dst may be a file.
 *	In any other situation where dst already exists, the rename will
 *	fail.  
 *
 * Results:
 *	If the directory was successfully created, returns TCL_OK.
 *	Otherwise the return value is TCL_ERROR and errno is set to
 *	indicate the error.  Some possible values for errno are:
 *
 *	EACCES:     src or dst parent directory can't be read and/or written.
 *	EEXIST:	    dst is a non-empty directory.
 *	EINVAL:	    src is a root directory or dst is a subdirectory of src.
 *	EISDIR:	    dst is a directory, but src is not.
 *	ENOENT:	    src doesn't exist, or src or dst is "".
 *	ENOTDIR:    src is a directory, but dst is not.  
 *	EXDEV:	    src and dst are on different filesystems.
 *	
 * Side effects:
 *	The implementation of rename may allow cross-filesystem renames,
 *	but the caller should be prepared to emulate it with copy and
 *	delete if errno is EXDEV.
 *
 *---------------------------------------------------------------------------
 */

int 
TclpObjRenameFile(srcPathPtr, destPathPtr)
    Tcl_Obj *srcPathPtr;
    Tcl_Obj *destPathPtr;
{
    return DoRenameFile(Tcl_FSGetNativePath(srcPathPtr), 
			Tcl_FSGetNativePath(destPathPtr));
}

static int
DoRenameFile(src, dst)
    CONST char *src;		/* Pathname of file or dir to be renamed
				 * (native). */
    CONST char *dst;		/* New pathname of file or directory
				 * (native). */
{
    if (rename(src, dst) == 0) {			/* INTL: Native. */
	return TCL_OK;
    }
    if (errno == ENOTEMPTY) {
	errno = EEXIST;
    }

    /*
     * IRIX returns EIO when you attept to move a directory into
     * itself.  We just map EIO to EINVAL get the right message on SGI.
     * Most platforms don't return EIO except in really strange cases.
     */
    
    if (errno == EIO) {
	errno = EINVAL;
    }
    
#ifndef NO_REALPATH
    /*
     * SunOS 4.1.4 reports overwriting a non-empty directory with a
     * directory as EINVAL instead of EEXIST (first rule out the correct
     * EINVAL result code for moving a directory into itself).  Must be
     * conditionally compiled because realpath() not defined on all systems.
     */

    if (errno == EINVAL) {
	char srcPath[MAXPATHLEN], dstPath[MAXPATHLEN];
	DIR *dirPtr;
	Tcl_DirEntry *dirEntPtr;

	if ((Realpath((char *) src, srcPath) != NULL)	/* INTL: Native. */
		&& (Realpath((char *) dst, dstPath) != NULL) /* INTL: Native. */
		&& (strncmp(srcPath, dstPath, strlen(srcPath)) != 0)) {
	    dirPtr = opendir(dst);			/* INTL: Native. */
	    if (dirPtr != NULL) {
		while (1) {
		    dirEntPtr = TclOSreaddir(dirPtr); /* INTL: Native. */
		    if (dirEntPtr == NULL) {
			break;
		    }
		    if ((strcmp(dirEntPtr->d_name, ".") != 0) &&
			    (strcmp(dirEntPtr->d_name, "..") != 0)) {
			errno = EEXIST;
			closedir(dirPtr);
			return TCL_ERROR;
		    }
		}
		closedir(dirPtr);
	    }
	}
	errno = EINVAL;
    }
#endif	/* !NO_REALPATH */

    if (strcmp(src, "/") == 0) {
	/*
	 * Alpha reports renaming / as EBUSY and Linux reports it as EACCES,
	 * instead of EINVAL.
	 */
	 
	errno = EINVAL;
    }

    /*
     * DEC Alpha OSF1 V3.0 returns EACCES when attempting to move a
     * file across filesystems and the parent directory of that file is
     * not writable.  Most other systems return EXDEV.  Does nothing to
     * correct this behavior.
     */

    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjCopyFile, DoCopyFile --
 *
 *      Copy a single file (not a directory).  If dst already exists and
 *	is not a directory, it is removed.
 *
 * Results:
 *	If the file was successfully copied, returns TCL_OK.  Otherwise
 *	the return value is TCL_ERROR and errno is set to indicate the
 *	error.  Some possible values for errno are:
 *
 *	EACCES:     src or dst parent directory can't be read and/or written.
 *	EISDIR:	    src or dst is a directory.
 *	ENOENT:	    src doesn't exist.  src or dst is "".
 *
 * Side effects:
 *      This procedure will also copy symbolic links, block, and
 *      character devices, and fifos.  For symbolic links, the links 
 *      themselves will be copied and not what they point to.  For the
 *	other special file types, the directory entry will be copied and
 *	not the contents of the device that it refers to.
 *
 *---------------------------------------------------------------------------
 */

int 
TclpObjCopyFile(srcPathPtr, destPathPtr)
    Tcl_Obj *srcPathPtr;
    Tcl_Obj *destPathPtr;
{
    return DoCopyFile(Tcl_FSGetNativePath(srcPathPtr), 
		      Tcl_FSGetNativePath(destPathPtr));
}

static int
DoCopyFile(src, dst)
    CONST char *src;	/* Pathname of file to be copied (native). */
    CONST char *dst;	/* Pathname of file to copy to (native). */
{
    Tcl_StatBuf srcStatBuf, dstStatBuf;

    /*
     * Have to do a stat() to determine the filetype.
     */
    
    if (TclOSlstat(src, &srcStatBuf) != 0) {		/* INTL: Native. */
	return TCL_ERROR;
    }
    if (S_ISDIR(srcStatBuf.st_mode)) {
	errno = EISDIR;
	return TCL_ERROR;
    }

    /*
     * symlink, and some of the other calls will fail if the target 
     * exists, so we remove it first
     */
    
    if (TclOSlstat(dst, &dstStatBuf) == 0) {		/* INTL: Native. */
	if (S_ISDIR(dstStatBuf.st_mode)) {
	    errno = EISDIR;
	    return TCL_ERROR;
	}
    }
    if (unlink(dst) != 0) {				/* INTL: Native. */
	if (errno != ENOENT) {
	    return TCL_ERROR;
	} 
    }

    switch ((int) (srcStatBuf.st_mode & S_IFMT)) {
#ifndef DJGPP
        case S_IFLNK: {
	    char link[MAXPATHLEN];
	    int length;

	    length = readlink(src, link, sizeof(link)); /* INTL: Native. */
	    if (length == -1) {
		return TCL_ERROR;
	    }
	    link[length] = '\0';
	    if (symlink(link, dst) < 0) {		/* INTL: Native. */
		return TCL_ERROR;
	    }
	    break;
	}
#endif
        case S_IFBLK:
        case S_IFCHR: {
	    if (mknod(dst, srcStatBuf.st_mode,		/* INTL: Native. */
		    srcStatBuf.st_rdev) < 0) {
		return TCL_ERROR;
	    }
	    return CopyFileAtts(src, dst, &srcStatBuf);
	}
        case S_IFIFO: {
	    if (mkfifo(dst, srcStatBuf.st_mode) < 0) {	/* INTL: Native. */
		return TCL_ERROR;
	    }
	    return CopyFileAtts(src, dst, &srcStatBuf);
	}
        default: {
	    return CopyFile(src, dst, &srcStatBuf);
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CopyFile - 
 *
 *      Helper function for TclpCopyFile.  Copies one regular file,
 *	using read() and write().
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *      A file is copied.  Dst will be overwritten if it exists.
 *
 *----------------------------------------------------------------------
 */

static int 
CopyFile(src, dst, statBufPtr) 
    CONST char *src;		/* Pathname of file to copy (native). */
    CONST char *dst;		/* Pathname of file to create/overwrite
				 * (native). */
    CONST Tcl_StatBuf *statBufPtr;
				/* Used to determine mode and blocksize. */
{
    int srcFd;
    int dstFd;
    u_int blockSize;   /* Optimal I/O blocksize for filesystem */
    char *buffer;      /* Data buffer for copy */
    size_t nread;

    if ((srcFd = TclOSopen(src, O_RDONLY, 0)) < 0) {	/* INTL: Native. */
	return TCL_ERROR;
    }

    dstFd = TclOSopen(dst, O_CREAT|O_TRUNC|O_WRONLY,	/* INTL: Native. */
	    statBufPtr->st_mode);
    if (dstFd < 0) {
	close(srcFd); 
	return TCL_ERROR;
    }

#ifdef HAVE_ST_BLKSIZE
    blockSize = statBufPtr->st_blksize;
#else
#ifndef NO_FSTATFS
    {
	struct statfs fs;
	if (fstatfs(srcFd, &fs, sizeof(fs), 0) == 0) {
	    blockSize = fs.f_bsize;
	} else {
	    blockSize = 4096;
	}
    }
#else 
    blockSize = 4096;
#endif
#endif

    buffer = ckalloc(blockSize);
    while (1) {
	nread = read(srcFd, buffer, blockSize);
	if ((nread == -1) || (nread == 0)) {
	    break;
	}
	if (write(dstFd, buffer, nread) != nread) {
	    nread = (size_t) -1;
	    break;
	}
    }
	
    ckfree(buffer);
    close(srcFd);
    if ((close(dstFd) != 0) || (nread == -1)) {
	unlink(dst);					/* INTL: Native. */
	return TCL_ERROR;
    }
    if (CopyFileAtts(src, dst, statBufPtr) == TCL_ERROR) {
	/*
	 * The copy succeeded, but setting the permissions failed, so be in
	 * a consistent state, we remove the file that was created by the
	 * copy.
	 */

	unlink(dst);					/* INTL: Native. */
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjDeleteFile, TclpDeleteFile --
 *
 *      Removes a single file (not a directory).
 *
 * Results:
 *	If the file was successfully deleted, returns TCL_OK.  Otherwise
 *	the return value is TCL_ERROR and errno is set to indicate the
 *	error.  Some possible values for errno are:
 *
 *	EACCES:     a parent directory can't be read and/or written.
 *	EISDIR:	    path is a directory.
 *	ENOENT:	    path doesn't exist or is "".
 *
 * Side effects:
 *      The file is deleted, even if it is read-only.
 *
 *---------------------------------------------------------------------------
 */

int 
TclpObjDeleteFile(pathPtr)
    Tcl_Obj *pathPtr;
{
    return TclpDeleteFile(Tcl_FSGetNativePath(pathPtr));
}

int
TclpDeleteFile(path)
    CONST char *path;	/* Pathname of file to be removed (native). */
{
    if (unlink(path) != 0) {				/* INTL: Native. */
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpCreateDirectory, DoCreateDirectory --
 *
 *      Creates the specified directory.  All parent directories of the
 *	specified directory must already exist.  The directory is
 *	automatically created with permissions so that user can access
 *	the new directory and create new files or subdirectories in it.
 *
 * Results:
 *	If the directory was successfully created, returns TCL_OK.
 *	Otherwise the return value is TCL_ERROR and errno is set to
 *	indicate the error.  Some possible values for errno are:
 *
 *	EACCES:     a parent directory can't be read and/or written.
 *	EEXIST:	    path already exists.
 *	ENOENT:	    a parent directory doesn't exist.
 *
 * Side effects:
 *      A directory is created with the current umask, except that
 *	permission for u+rwx will always be added.
 *
 *---------------------------------------------------------------------------
 */

int 
TclpObjCreateDirectory(pathPtr)
    Tcl_Obj *pathPtr;
{
    return DoCreateDirectory(Tcl_FSGetNativePath(pathPtr));
}

static int
DoCreateDirectory(path)
    CONST char *path;	/* Pathname of directory to create (native). */
{
    mode_t mode;

    mode = umask(0);
    umask(mode);

    /*
     * umask return value is actually the inverse of the permissions.
     */

    mode = (0777 & ~mode) | S_IRUSR | S_IWUSR | S_IXUSR;

    if (mkdir(path, mode) != 0) {			/* INTL: Native. */
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjCopyDirectory --
 *
 *      Recursively copies a directory.  The target directory dst must
 *	not already exist.  Note that this function does not merge two
 *	directory hierarchies, even if the target directory is an an
 *	empty directory.
 *
 * Results:
 *	If the directory was successfully copied, returns TCL_OK.
 *	Otherwise the return value is TCL_ERROR, errno is set to indicate
 *	the error, and the pathname of the file that caused the error
 *	is stored in errorPtr.  See TclpObjCreateDirectory and 
 *	TclpObjCopyFile for a description of possible values for errno.
 *
 * Side effects:
 *      An exact copy of the directory hierarchy src will be created
 *	with the name dst.  If an error occurs, the error will
 *      be returned immediately, and remaining files will not be
 *	processed.
 *
 *---------------------------------------------------------------------------
 */

int 
TclpObjCopyDirectory(srcPathPtr, destPathPtr, errorPtr)
    Tcl_Obj *srcPathPtr;
    Tcl_Obj *destPathPtr;
    Tcl_Obj **errorPtr;
{
    Tcl_DString ds;
    Tcl_DString srcString, dstString;
    int ret;
    Tcl_Obj *transPtr;
    
    transPtr = Tcl_FSGetTranslatedPath(NULL,srcPathPtr);
    Tcl_UtfToExternalDString(NULL, 
			     (transPtr != NULL ? Tcl_GetString(transPtr) : NULL), 
			     -1, &srcString);
    if (transPtr != NULL) {
	Tcl_DecrRefCount(transPtr);
    }
    transPtr = Tcl_FSGetTranslatedPath(NULL,destPathPtr);
    Tcl_UtfToExternalDString(NULL, 
			     (transPtr != NULL ? Tcl_GetString(transPtr) : NULL), 
			     -1, &dstString);
    if (transPtr != NULL) {
	Tcl_DecrRefCount(transPtr);
    }

    ret = TraverseUnixTree(TraversalCopy, &srcString, &dstString, &ds);

    Tcl_DStringFree(&srcString);
    Tcl_DStringFree(&dstString);

    if (ret != TCL_OK) {
	*errorPtr = Tcl_NewStringObj(Tcl_DStringValue(&ds), -1);
	Tcl_DStringFree(&ds);
	Tcl_IncrRefCount(*errorPtr);
    }
    return ret;
}


/*
 *---------------------------------------------------------------------------
 *
 * TclpRemoveDirectory, DoRemoveDirectory --
 *
 *	Removes directory (and its contents, if the recursive flag is set).
 *
 * Results:
 *	If the directory was successfully removed, returns TCL_OK.
 *	Otherwise the return value is TCL_ERROR, errno is set to indicate
 *	the error, and the pathname of the file that caused the error
 *	is stored in errorPtr.  Some possible values for errno are:
 *
 *	EACCES:     path directory can't be read and/or written.
 *	EEXIST:	    path is a non-empty directory.
 *	EINVAL:	    path is a root directory.
 *	ENOENT:	    path doesn't exist or is "".
 * 	ENOTDIR:    path is not a directory.
 *
 * Side effects:
 *	Directory removed.  If an error occurs, the error will be returned
 *	immediately, and remaining files will not be deleted.
 *
 *---------------------------------------------------------------------------
 */
 
int 
TclpObjRemoveDirectory(pathPtr, recursive, errorPtr)
    Tcl_Obj *pathPtr;
    int recursive;
    Tcl_Obj **errorPtr;
{
    Tcl_DString ds;
    Tcl_DString pathString;
    int ret;
    Tcl_Obj *transPtr = Tcl_FSGetTranslatedPath(NULL, pathPtr);

    Tcl_UtfToExternalDString(NULL, 
			     (transPtr != NULL ? Tcl_GetString(transPtr) : NULL), 
			     -1, &pathString);
    if (transPtr != NULL) {
	Tcl_DecrRefCount(transPtr);
    }
    ret = DoRemoveDirectory(&pathString, recursive, &ds);
    Tcl_DStringFree(&pathString);

    if (ret != TCL_OK) {
	*errorPtr = Tcl_NewStringObj(Tcl_DStringValue(&ds), -1);
	Tcl_DStringFree(&ds);
	Tcl_IncrRefCount(*errorPtr);
    }
    return ret;
}

static int
DoRemoveDirectory(pathPtr, recursive, errorPtr)
    Tcl_DString *pathPtr;	/* Pathname of directory to be removed
				 * (native). */
    int recursive;		/* If non-zero, removes directories that
				 * are nonempty.  Otherwise, will only remove
				 * empty directories. */
    Tcl_DString *errorPtr;	/* If non-NULL, uninitialized or free
				 * DString filled with UTF-8 name of file
				 * causing error. */
{
    CONST char *path;
    mode_t oldPerm = 0;
    int result;
    
    path = Tcl_DStringValue(pathPtr);
    
    if (recursive != 0) {
	/* We should try to change permissions so this can be deleted */
	Tcl_StatBuf statBuf;
	int newPerm;

	if (TclOSstat(path, &statBuf) == 0) {
	    oldPerm = (mode_t) (statBuf.st_mode & 0x00007FFF);
	}
	
	newPerm = oldPerm | (64+128+256);
	chmod(path, (mode_t) newPerm);
    }
    
    if (rmdir(path) == 0) {				/* INTL: Native. */
	return TCL_OK;
    }
    if (errno == ENOTEMPTY) {
	errno = EEXIST;
    }

    result = TCL_OK;
    if ((errno != EEXIST) || (recursive == 0)) {
	if (errorPtr != NULL) {
	    Tcl_ExternalToUtfDString(NULL, path, -1, errorPtr);
	}
	result = TCL_ERROR;
    }
    
    /*
     * The directory is nonempty, but the recursive flag has been
     * specified, so we recursively remove all the files in the directory.
     */

    if (result == TCL_OK) {
	result = TraverseUnixTree(TraversalDelete, pathPtr, NULL, errorPtr);
    }
    
    if ((result != TCL_OK) && (recursive != 0)) {
        /* Try to restore permissions */
        chmod(path, oldPerm);
    }
    return result;
}
	
/*
 *---------------------------------------------------------------------------
 *
 * TraverseUnixTree --
 *
 *      Traverse directory tree specified by sourcePtr, calling the function 
 *	traverseProc for each file and directory encountered.  If destPtr 
 *	is non-null, each of name in the sourcePtr directory is appended to 
 *	the directory specified by destPtr and passed as the second argument 
 *	to traverseProc() .
 *
 * Results:
 *      Standard Tcl result.
 *
 * Side effects:
 *      None caused by TraverseUnixTree, however the user specified 
 *	traverseProc() may change state.  If an error occurs, the error will
 *      be returned immediately, and remaining files will not be processed.
 *
 *---------------------------------------------------------------------------
 */

static int 
TraverseUnixTree(traverseProc, sourcePtr, targetPtr, errorPtr)
    TraversalProc *traverseProc;/* Function to call for every file and
				 * directory in source hierarchy. */
    Tcl_DString *sourcePtr;	/* Pathname of source directory to be
				 * traversed (native). */
    Tcl_DString *targetPtr;	/* Pathname of directory to traverse in
				 * parallel with source directory (native). */
    Tcl_DString *errorPtr;	/* If non-NULL, uninitialized or free
				 * DString filled with UTF-8 name of file
				 * causing error. */
{
    Tcl_StatBuf statBuf;
    CONST char *source, *errfile;
    int result, sourceLen;
    int targetLen;
    Tcl_DirEntry *dirEntPtr;
    DIR *dirPtr;

    errfile = NULL;
    result = TCL_OK;
    targetLen = 0;		/* lint. */

    source = Tcl_DStringValue(sourcePtr);
    if (TclOSlstat(source, &statBuf) != 0) {		/* INTL: Native. */
	errfile = source;
	goto end;
    }
    if (!S_ISDIR(statBuf.st_mode)) {
	/*
	 * Process the regular file
	 */

	return (*traverseProc)(sourcePtr, targetPtr, &statBuf, DOTREE_F,
		errorPtr);
    }
    dirPtr = opendir(source);				/* INTL: Native. */
    if (dirPtr == NULL) {
	/* 
	 * Can't read directory
	 */

	errfile = source;
	goto end;
    }
    result = (*traverseProc)(sourcePtr, targetPtr, &statBuf, DOTREE_PRED,
	    errorPtr);
    if (result != TCL_OK) {
	closedir(dirPtr);
	return result;
    }
    
    Tcl_DStringAppend(sourcePtr, "/", 1);
    sourceLen = Tcl_DStringLength(sourcePtr);	

    if (targetPtr != NULL) {
	Tcl_DStringAppend(targetPtr, "/", 1);
	targetLen = Tcl_DStringLength(targetPtr);
    }

    while ((dirEntPtr = TclOSreaddir(dirPtr)) != NULL) { /* INTL: Native. */
	if ((dirEntPtr->d_name[0] == '.')
		&& ((dirEntPtr->d_name[1] == '\0')
			|| (strcmp(dirEntPtr->d_name, "..") == 0))) {
	    continue;
	}

	/* 
	 * Append name after slash, and recurse on the file.
	 */

	Tcl_DStringAppend(sourcePtr, dirEntPtr->d_name, -1);
	if (targetPtr != NULL) {
	    Tcl_DStringAppend(targetPtr, dirEntPtr->d_name, -1);
	}
	result = TraverseUnixTree(traverseProc, sourcePtr, targetPtr,
		errorPtr);
	if (result != TCL_OK) {
	    break;
	}
	
	/*
	 * Remove name after slash.
	 */

	Tcl_DStringSetLength(sourcePtr, sourceLen);
	if (targetPtr != NULL) {
	    Tcl_DStringSetLength(targetPtr, targetLen);
	}
    }
    closedir(dirPtr);
    
    /*
     * Strip off the trailing slash we added
     */

    Tcl_DStringSetLength(sourcePtr, sourceLen - 1);
    if (targetPtr != NULL) {
	Tcl_DStringSetLength(targetPtr, targetLen - 1);
    }

    if (result == TCL_OK) {
	/*
	 * Call traverseProc() on a directory after visiting all the
	 * files in that directory.
	 */

	result = (*traverseProc)(sourcePtr, targetPtr, &statBuf, DOTREE_POSTD,
		errorPtr);
    }
    end:
    if (errfile != NULL) {
	if (errorPtr != NULL) {
	    Tcl_ExternalToUtfDString(NULL, errfile, -1, errorPtr);
	}
	result = TCL_ERROR;
    }
	    
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * TraversalCopy
 *
 *      Called from TraverseUnixTree in order to execute a recursive copy
 *      of a directory.
 *
 * Results:
 *      Standard Tcl result.
 *
 * Side effects:
 *      The file or directory src may be copied to dst, depending on 
 *      the value of type.
 *      
 *----------------------------------------------------------------------
 */

static int 
TraversalCopy(srcPtr, dstPtr, statBufPtr, type, errorPtr) 
    Tcl_DString *srcPtr;	/* Source pathname to copy (native). */
    Tcl_DString *dstPtr;	/* Destination pathname of copy (native). */
    CONST Tcl_StatBuf *statBufPtr;
				/* Stat info for file specified by srcPtr. */
    int type;                   /* Reason for call - see TraverseUnixTree(). */
    Tcl_DString *errorPtr;	/* If non-NULL, uninitialized or free
				 * DString filled with UTF-8 name of file
				 * causing error. */
{
    switch (type) {
	case DOTREE_F:
	    if (DoCopyFile(Tcl_DStringValue(srcPtr), 
		    Tcl_DStringValue(dstPtr)) == TCL_OK) {
		return TCL_OK;
	    }
	    break;

	case DOTREE_PRED:
	    if (DoCreateDirectory(Tcl_DStringValue(dstPtr)) == TCL_OK) {
		return TCL_OK;
	    }
	    break;

	case DOTREE_POSTD:
	    if (CopyFileAtts(Tcl_DStringValue(srcPtr),
		    Tcl_DStringValue(dstPtr), statBufPtr) == TCL_OK) {
		return TCL_OK;
	    }
	    break;

    }

    /*
     * There shouldn't be a problem with src, because we already checked it
     * to get here.
     */

    if (errorPtr != NULL) {
	Tcl_ExternalToUtfDString(NULL, Tcl_DStringValue(dstPtr),
		Tcl_DStringLength(dstPtr), errorPtr);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * TraversalDelete --
 *
 *      Called by procedure TraverseUnixTree for every file and directory
 *	that it encounters in a directory hierarchy. This procedure unlinks
 *      files, and removes directories after all the containing files 
 *      have been processed.
 *
 * Results:
 *      Standard Tcl result.
 *
 * Side effects:
 *      Files or directory specified by src will be deleted.
 *
 *----------------------------------------------------------------------
 */

static int
TraversalDelete(srcPtr, ignore, statBufPtr, type, errorPtr) 
    Tcl_DString *srcPtr;	/* Source pathname (native). */
    Tcl_DString *ignore;	/* Destination pathname (not used). */
    CONST Tcl_StatBuf *statBufPtr;
				/* Stat info for file specified by srcPtr. */
    int type;                   /* Reason for call - see TraverseUnixTree(). */
    Tcl_DString *errorPtr;	/* If non-NULL, uninitialized or free
				 * DString filled with UTF-8 name of file
				 * causing error. */
{
    switch (type) {
        case DOTREE_F: {
	    if (TclpDeleteFile(Tcl_DStringValue(srcPtr)) == 0) {
		return TCL_OK;
	    }
	    break;
	}
        case DOTREE_PRED: {
	    return TCL_OK;
	}
        case DOTREE_POSTD: {
	    if (DoRemoveDirectory(srcPtr, 0, NULL) == 0) {
		return TCL_OK;
	    }
	    break;
	}	    
    }
    if (errorPtr != NULL) {
	Tcl_ExternalToUtfDString(NULL, Tcl_DStringValue(srcPtr),
		Tcl_DStringLength(srcPtr), errorPtr);
    }
    return TCL_ERROR;
}

/*
 *---------------------------------------------------------------------------
 *
 * CopyFileAtts --
 *
 *	Copy the file attributes such as owner, group, permissions,
 *	and modification date from one file to another.
 *
 * Results:
 *	Standard Tcl result.
 *
 * Side effects:
 *	user id, group id, permission bits, last modification time, and
 *	last access time are updated in the new file to reflect the
 *	old file.
 *
 *---------------------------------------------------------------------------
 */

static int
CopyFileAtts(src, dst, statBufPtr) 
    CONST char *src;		/* Path name of source file (native). */
    CONST char *dst;		/* Path name of target file (native). */
    CONST Tcl_StatBuf *statBufPtr;
				/* Stat info for source file */
{
    struct utimbuf tval;
    mode_t newMode;
    
    newMode = statBufPtr->st_mode
	    & (S_ISUID | S_ISGID | S_IRWXU | S_IRWXG | S_IRWXO);
	
    /* 
     * Note that if you copy a setuid file that is owned by someone
     * else, and you are not root, then the copy will be setuid to you.
     * The most correct implementation would probably be to have the
     * copy not setuid to anyone if the original file was owned by 
     * someone else, but this corner case isn't currently handled.
     * It would require another lstat(), or getuid().
     */
    
    if (chmod(dst, newMode)) {				/* INTL: Native. */
	newMode &= ~(S_ISUID | S_ISGID);
	if (chmod(dst, newMode)) {			/* INTL: Native. */
	    return TCL_ERROR;
	}
    }

    tval.actime = statBufPtr->st_atime; 
    tval.modtime = statBufPtr->st_mtime; 

    if (utime(dst, &tval)) {				/* INTL: Native. */
	return TCL_ERROR;
    }
    return TCL_OK;
}


/*
 *----------------------------------------------------------------------
 *
 * GetGroupAttribute
 *
 *      Gets the group attribute of a file.
 *
 * Results:
 *      Standard TCL result. Returns a new Tcl_Obj in attributePtrPtr
 *	if there is no error.
 *
 * Side effects:
 *      A new object is allocated.
 *      
 *----------------------------------------------------------------------
 */

static int
GetGroupAttribute(interp, objIndex, fileName, attributePtrPtr)
    Tcl_Interp *interp;		/* The interp we are using for errors. */
    int objIndex;		/* The index of the attribute. */
    Tcl_Obj *fileName;  	/* The name of the file (UTF-8). */
    Tcl_Obj **attributePtrPtr;	/* A pointer to return the object with. */
{
    Tcl_StatBuf statBuf;
    struct group *groupPtr;
    int result;

    result = TclpObjStat(fileName, &statBuf);
    
    if (result != 0) {
	Tcl_AppendResult(interp, "could not read \"", 
		Tcl_GetString(fileName), "\": ",
		Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }

    groupPtr = getgrgid(statBuf.st_gid);		/* INTL: Native. */
    if (groupPtr == NULL) {
	*attributePtrPtr = Tcl_NewIntObj((int) statBuf.st_gid);
    } else {
	Tcl_DString ds;
	CONST char *utf;

	utf = Tcl_ExternalToUtfDString(NULL, groupPtr->gr_name, -1, &ds); 
	*attributePtrPtr = Tcl_NewStringObj(utf, -1);
	Tcl_DStringFree(&ds);
    }
    endgrent();
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetOwnerAttribute
 *
 *      Gets the owner attribute of a file.
 *
 * Results:
 *      Standard TCL result. Returns a new Tcl_Obj in attributePtrPtr
 *	if there is no error.
 *
 * Side effects:
 *      A new object is allocated.
 *      
 *----------------------------------------------------------------------
 */

static int
GetOwnerAttribute(interp, objIndex, fileName, attributePtrPtr)
    Tcl_Interp *interp;		/* The interp we are using for errors. */
    int objIndex;		/* The index of the attribute. */
    Tcl_Obj *fileName;  	/* The name of the file (UTF-8). */
    Tcl_Obj **attributePtrPtr;	/* A pointer to return the object with. */
{
    Tcl_StatBuf statBuf;
    struct passwd *pwPtr;
    int result;

    result = TclpObjStat(fileName, &statBuf);
    
    if (result != 0) {
	Tcl_AppendResult(interp, "could not read \"", 
		Tcl_GetString(fileName), "\": ",
		Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }

    pwPtr = getpwuid(statBuf.st_uid);			/* INTL: Native. */
    if (pwPtr == NULL) {
	*attributePtrPtr = Tcl_NewIntObj((int) statBuf.st_uid);
    } else {
	Tcl_DString ds;
	CONST char *utf;

	utf = Tcl_ExternalToUtfDString(NULL, pwPtr->pw_name, -1, &ds); 
	*attributePtrPtr = Tcl_NewStringObj(utf, Tcl_DStringLength(&ds));
	Tcl_DStringFree(&ds);
    }
    endpwent();
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetPermissionsAttribute
 *
 *      Gets the group attribute of a file.
 *
 * Results:
 *      Standard TCL result. Returns a new Tcl_Obj in attributePtrPtr
 *	if there is no error. The object will have ref count 0.
 *
 * Side effects:
 *      A new object is allocated.
 *      
 *----------------------------------------------------------------------
 */

static int
GetPermissionsAttribute(interp, objIndex, fileName, attributePtrPtr)
    Tcl_Interp *interp;		    /* The interp we are using for errors. */
    int objIndex;		    /* The index of the attribute. */
    Tcl_Obj *fileName;  	    /* The name of the file (UTF-8). */
    Tcl_Obj **attributePtrPtr;	    /* A pointer to return the object with. */
{
    Tcl_StatBuf statBuf;
    char returnString[7];
    int result;

    result = TclpObjStat(fileName, &statBuf);
    
    if (result != 0) {
	Tcl_AppendResult(interp, "could not read \"", 
		Tcl_GetString(fileName), "\": ",
		Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }

    sprintf(returnString, "%0#5lo", (long) (statBuf.st_mode & 0x00007FFF));

    *attributePtrPtr = Tcl_NewStringObj(returnString, -1);
    
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetGroupAttribute --
 *
 *      Sets the group of the file to the specified group.
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      As above.
 *      
 *---------------------------------------------------------------------------
 */

static int
SetGroupAttribute(interp, objIndex, fileName, attributePtr)
    Tcl_Interp *interp;		    /* The interp for error reporting. */
    int objIndex;		    /* The index of the attribute. */
    Tcl_Obj *fileName;	            /* The name of the file (UTF-8). */
    Tcl_Obj *attributePtr;	    /* New group for file. */
{
    long gid;
    int result;
    CONST char *native;

    if (Tcl_GetLongFromObj(NULL, attributePtr, &gid) != TCL_OK) {
	Tcl_DString ds;
	struct group *groupPtr;
	CONST char *string;
	int length;

	string = Tcl_GetStringFromObj(attributePtr, &length);

	native = Tcl_UtfToExternalDString(NULL, string, length, &ds);
	groupPtr = getgrnam(native);			/* INTL: Native. */
	Tcl_DStringFree(&ds);

	if (groupPtr == NULL) {
	    endgrent();
	    Tcl_AppendResult(interp, "could not set group for file \"",
		    Tcl_GetString(fileName), "\": group \"", 
		    string, "\" does not exist",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	gid = groupPtr->gr_gid;
    }

    native = Tcl_FSGetNativePath(fileName);
    result = chown(native, (uid_t) -1, (gid_t) gid);	/* INTL: Native. */

    endgrent();
    if (result != 0) {
	Tcl_AppendResult(interp, "could not set group for file \"",
	    Tcl_GetString(fileName), "\": ", Tcl_PosixError(interp), 
	    (char *) NULL);
	return TCL_ERROR;
    }    
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetOwnerAttribute --
 *
 *      Sets the owner of the file to the specified owner.
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      As above.
 *      
 *---------------------------------------------------------------------------
 */

static int
SetOwnerAttribute(interp, objIndex, fileName, attributePtr)
    Tcl_Interp *interp;		    /* The interp for error reporting. */
    int objIndex;		    /* The index of the attribute. */
    Tcl_Obj *fileName;   	    /* The name of the file (UTF-8). */
    Tcl_Obj *attributePtr;	    /* New owner for file. */
{
    long uid;
    int result;
    CONST char *native;

    if (Tcl_GetLongFromObj(NULL, attributePtr, &uid) != TCL_OK) {
	Tcl_DString ds;
	struct passwd *pwPtr;
	CONST char *string;
	int length;

	string = Tcl_GetStringFromObj(attributePtr, &length);

	native = Tcl_UtfToExternalDString(NULL, string, length, &ds);
	pwPtr = getpwnam(native);			/* INTL: Native. */
	Tcl_DStringFree(&ds);

	if (pwPtr == NULL) {
	    Tcl_AppendResult(interp, "could not set owner for file \"",
			     Tcl_GetString(fileName), "\": user \"", 
			     string, "\" does not exist",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	uid = pwPtr->pw_uid;
    }

    native = Tcl_FSGetNativePath(fileName);
    result = chown(native, (uid_t) uid, (gid_t) -1);   /* INTL: Native. */

    if (result != 0) {
	Tcl_AppendResult(interp, "could not set owner for file \"", 
			 Tcl_GetString(fileName), "\": ", 
			 Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetPermissionsAttribute
 *
 *      Sets the file to the given permission.
 *
 * Results:
 *      Standard TCL result.
 *
 * Side effects:
 *      The permission of the file is changed.
 *      
 *---------------------------------------------------------------------------
 */

static int
SetPermissionsAttribute(interp, objIndex, fileName, attributePtr)
    Tcl_Interp *interp;		    /* The interp we are using for errors. */
    int objIndex;		    /* The index of the attribute. */
    Tcl_Obj *fileName;  	    /* The name of the file (UTF-8). */
    Tcl_Obj *attributePtr;	    /* The attribute to set. */
{
    long mode;
    mode_t newMode;
    int result;
    CONST char *native;

    /*
     * First try if the string is a number
     */
    if (Tcl_GetLongFromObj(NULL, attributePtr, &mode) == TCL_OK) {
        newMode = (mode_t) (mode & 0x00007FFF);
    } else {
	Tcl_StatBuf buf;
	char *modeStringPtr = Tcl_GetString(attributePtr);

	/*
	 * Try the forms "rwxrwxrwx" and "ugo=rwx"
	 *
	 * We get the current mode of the file, in order to allow for
	 * ug+-=rwx style chmod strings.
	 */
	result = TclpObjStat(fileName, &buf);
	if (result != 0) {
	    Tcl_AppendResult(interp, "could not read \"", 
		    Tcl_GetString(fileName), "\": ",
		    Tcl_PosixError(interp), (char *) NULL);
	    return TCL_ERROR;
	}
	newMode = (mode_t) (buf.st_mode & 0x00007FFF);

	if (GetModeFromPermString(NULL, modeStringPtr, &newMode) != TCL_OK) {
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "unknown permission string format \"",
		    modeStringPtr, "\"", (char *) NULL);
	    return TCL_ERROR;
	}
    }

    native = Tcl_FSGetNativePath(fileName);
    result = chmod(native, newMode);		/* INTL: Native. */
    if (result != 0) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"could not set permissions for file \"", 
		Tcl_GetString(fileName), "\": ",
		Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjListVolumes --
 *
 *	Lists the currently mounted volumes, which on UNIX is just /.
 *
 * Results:
 *	The list of volumes.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Obj*
TclpObjListVolumes(void)
{
    Tcl_Obj *resultPtr = Tcl_NewStringObj("/",1);

    Tcl_IncrRefCount(resultPtr);
    return resultPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * GetModeFromPermString --
 *
 *	This procedure is invoked to process the "file permissions"
 *	Tcl command, to check for a "rwxrwxrwx" or "ugoa+-=rwxst" string.
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

static int
GetModeFromPermString(interp, modeStringPtr, modePtr)
    Tcl_Interp *interp;		/* The interp we are using for errors. */
    char *modeStringPtr;	/* Permissions string */
    mode_t *modePtr;		/* pointer to the mode value */
{
    mode_t newMode;
    mode_t oldMode;		/* Storage for the value of the old mode
				 * (that is passed in), to allow for the
				 * chmod style manipulation */
    int i,n, who, op, what, op_found, who_found;

    /*
     * We start off checking for an "rwxrwxrwx" style permissions string
     */
    if (strlen(modeStringPtr) != 9) {
        goto chmodStyleCheck;
    }

    newMode = 0;
    for (i = 0; i < 9; i++) {
	switch (*(modeStringPtr+i)) {
	    case 'r':
		if ((i%3) != 0) {
		    goto chmodStyleCheck;
		}
		newMode |= (1<<(8-i));
		break;
	    case 'w':
		if ((i%3) != 1) {
		    goto chmodStyleCheck;
		}
		newMode |= (1<<(8-i));
		break;
	    case 'x':
		if ((i%3) != 2) {
		    goto chmodStyleCheck;
		}
		newMode |= (1<<(8-i));
		break;
	    case 's':
		if (((i%3) != 2) || (i > 5)) {
		    goto chmodStyleCheck;
		}
		newMode |= (1<<(8-i));
		newMode |= (1<<(11-(i/3)));
		break;
	    case 'S':
		if (((i%3) != 2) || (i > 5)) {
		    goto chmodStyleCheck;
		}
		newMode |= (1<<(11-(i/3)));
		break;
	    case 't':
		if (i != 8) {
		    goto chmodStyleCheck;
		}
		newMode |= (1<<(8-i));
		newMode |= (1<<9);
		break;
	    case 'T':
		if (i != 8) {
		    goto chmodStyleCheck;
		}
		newMode |= (1<<9);
		break;
	    case '-':
		break;
	    default:
		/*
		 * Oops, not what we thought it was, so go on
		 */
		goto chmodStyleCheck;
	}
    }
    *modePtr = newMode;
    return TCL_OK;

    chmodStyleCheck:
    /*
     * We now check for an "ugoa+-=rwxst" style permissions string
     */

    for (n = 0 ; *(modeStringPtr+n) != '\0' ; n = n + i) {
	oldMode = *modePtr;
	who = op = what = op_found = who_found = 0;
	for (i = 0 ; *(modeStringPtr+n+i) != '\0' ; i++ ) {
	    if (!who_found) {
		/* who */
		switch (*(modeStringPtr+n+i)) {
		    case 'u' :
			who |= 0x9c0;
			continue;
		    case 'g' :
			who |= 0x438;
			continue;
		    case 'o' :
			who |= 0x207;
			continue;
		    case 'a' :
			who |= 0xfff;
			continue;
		}
	    }
	    who_found = 1;
	    if (who == 0) {
		who = 0xfff;
	    }
	    if (!op_found) {
		/* op */
		switch (*(modeStringPtr+n+i)) {
		    case '+' :
			op = 1;
			op_found = 1;
			continue;
		    case '-' :
			op = 2;
			op_found = 1;
			continue;
		    case '=' :
			op = 3;
			op_found = 1;
			continue;
		    default  :
			return TCL_ERROR;
		}
	    }
	    /* what */
	    switch (*(modeStringPtr+n+i)) {
		case 'r' :
		    what |= 0x124;
		    continue;
		case 'w' :
		    what |= 0x92;
		    continue;
		case 'x' :
		    what |= 0x49;
		    continue;
		case 's' :
		    what |= 0xc00;
		    continue;
		case 't' :
		    what |= 0x200;
		    continue;
		case ',' :
		    break;
		default  :
		    return TCL_ERROR;
	    }
	    if (*(modeStringPtr+n+i) == ',') {
		i++;
		break;
	    }
	}
	switch (op) {
	    case 1 :
		*modePtr = oldMode | (who & what);
		continue;
	    case 2 :
		*modePtr = oldMode & ~(who & what);
		continue;
	    case 3 :
		*modePtr = (oldMode & ~who) | (who & what);
		continue;
	}
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjNormalizePath --
 *
 *	This function scans through a path specification and replaces
 *	it, in place, with a normalized version.  A normalized version
 *	is one in which all symlinks in the path are replaced with
 *	their expanded form (except a symlink at the very end of the
 *	path).
 *
 * Results:
 *	The new 'nextCheckpoint' value, giving as far as we could
 *	understand in the path.
 *
 * Side effects:
 *	The pathPtr string, is modified.
 *
 *---------------------------------------------------------------------------
 */
int
TclpObjNormalizePath(interp, pathPtr, nextCheckpoint)
    Tcl_Interp *interp;
    Tcl_Obj *pathPtr;
    int nextCheckpoint;
{
    char *currentPathEndPosition;
    int pathLen;
    char cur;
    char *path = Tcl_GetStringFromObj(pathPtr, &pathLen);
#ifndef NO_REALPATH
    char normPath[MAXPATHLEN];
    Tcl_DString ds;
    CONST char *nativePath; 
#endif
    /* 
     * We add '1' here because if nextCheckpoint is zero we know
     * that '/' exists, and if it isn't zero, it must point at
     * a directory separator which we also know exists.
     */
    currentPathEndPosition = path + nextCheckpoint;
    if (*currentPathEndPosition == '/') {
	currentPathEndPosition++;
    }

#ifndef NO_REALPATH
    /* For speed, try to get the entire path in one go */
    if (nextCheckpoint == 0) {
        char *lastDir = strrchr(currentPathEndPosition, '/');
	if (lastDir != NULL) {
	    nativePath = Tcl_UtfToExternalDString(NULL, path, 
						  lastDir - path, &ds);
	    if (Realpath(nativePath, normPath) != NULL) {
		nextCheckpoint = lastDir - path;
		goto wholeStringOk;
	    }
	}
    }
    /* Else do it the slow way */
#endif
    
    while (1) {
	cur = *currentPathEndPosition;
	if ((cur == '/') && (path != currentPathEndPosition)) {
	    /* Reached directory separator */
	    Tcl_DString ds;
	    CONST char *nativePath;
	    int accessOk;

	    nativePath = Tcl_UtfToExternalDString(NULL, path, 
		    currentPathEndPosition - path, &ds);
	    accessOk = access(nativePath, F_OK);
	    Tcl_DStringFree(&ds);
	    if (accessOk != 0) {
		/* File doesn't exist */
		break;
	    }
	    /* Update the acceptable point */
	    nextCheckpoint = currentPathEndPosition - path;
	} else if (cur == 0) {
	    /* Reached end of string */
	    break;
	}
	currentPathEndPosition++;
    }
    /* 
     * We should really now convert this to a canonical path.  We do
     * that with 'realpath' if we have it available.  Otherwise we could
     * step through every single path component, checking whether it is a 
     * symlink, but that would be a lot of work, and most modern OSes 
     * have 'realpath'.
     */
#ifndef NO_REALPATH
    /* 
     * If we only had '/foo' or '/' then we never increment nextCheckpoint
     * and we don't need or want to go through 'Realpath'.  Also, on some
     * platforms, passing an empty string to 'Realpath' will give us the
     * normalized pwd, which is not what we want at all!
     */
    if (nextCheckpoint == 0) return 0;
    
    nativePath = Tcl_UtfToExternalDString(NULL, path, nextCheckpoint, &ds);
    if (Realpath(nativePath, normPath) != NULL) {
	int newNormLen;
	wholeStringOk:
	newNormLen = strlen(normPath);
	if ((newNormLen == Tcl_DStringLength(&ds))
		&& (strcmp(normPath, nativePath) == 0)) {
	    /* String is unchanged */
	    Tcl_DStringFree(&ds);
	    if (path[nextCheckpoint] != '\0') {
		nextCheckpoint++;
	    }
	    return nextCheckpoint;
	}
	
	/* 
	 * Free up the native path and put in its place the
	 * converted, normalized path.
	 */
	Tcl_DStringFree(&ds);
	Tcl_ExternalToUtfDString(NULL, normPath, (int) newNormLen, &ds);

	if (path[nextCheckpoint] != '\0') {
	    /* not at end, append remaining path */
	    int normLen = Tcl_DStringLength(&ds);
	    Tcl_DStringAppend(&ds, path + nextCheckpoint,
		    pathLen - nextCheckpoint);
	    /* 
	     * We recognise up to and including the directory
	     * separator.
	     */	
	    nextCheckpoint = normLen + 1;
	} else {
	    /* We recognise the whole string */ 
	    nextCheckpoint = Tcl_DStringLength(&ds);
	}
	/* 
	 * Overwrite with the normalized path.
	 */
	Tcl_SetStringObj(pathPtr, Tcl_DStringValue(&ds),
		Tcl_DStringLength(&ds));
    }
    Tcl_DStringFree(&ds);
#endif	/* !NO_REALPATH */

    return nextCheckpoint;
}



