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
			    int objIndex, CONST char *fileName,
			    Tcl_Obj **attributePtrPtr));
static int		GetOwnerAttribute _ANSI_ARGS_((Tcl_Interp *interp,
			    int objIndex, CONST char *fileName,
			    Tcl_Obj **attributePtrPtr));
static int		GetPermissionsAttribute _ANSI_ARGS_((
			    Tcl_Interp *interp, int objIndex,
			    CONST char *fileName, Tcl_Obj **attributePtrPtr));
static int		SetGroupAttribute _ANSI_ARGS_((Tcl_Interp *interp,
			    int objIndex, CONST char *fileName,
			    Tcl_Obj *attributePtr));
static int		SetOwnerAttribute _ANSI_ARGS_((Tcl_Interp *interp,
			    int objIndex, CONST char *fileName,
			    Tcl_Obj *attributePtr));
static int		SetPermissionsAttribute _ANSI_ARGS_((
			    Tcl_Interp *interp, int objIndex,
			    CONST char *fileName, Tcl_Obj *attributePtr));
			  
/*
 * Prototype for the TraverseUnixTree callback function.
 */

typedef int (TraversalProc) _ANSI_ARGS_((Tcl_DString *srcPtr,
	Tcl_DString *dstPtr, CONST struct stat *statBufPtr, int type,
	Tcl_DString *errorPtr));

/*
 * Constants and variables necessary for file attributes subcommand.
 */

enum {
    UNIX_GROUP_ATTRIBUTE,
    UNIX_OWNER_ATTRIBUTE,
    UNIX_PERMISSIONS_ATTRIBUTE
};

char *tclpFileAttrStrings[] = {
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
			    CONST char *dst, CONST struct stat *statBufPtr));
static int		CopyFileAtts _ANSI_ARGS_((CONST char *src,
			    CONST char *dst, CONST struct stat *statBufPtr));
static int		DoCopyFile _ANSI_ARGS_((Tcl_DString *srcPtr,
			    Tcl_DString *dstPtr));
static int		DoCreateDirectory _ANSI_ARGS_((Tcl_DString *pathPtr));
static int		DoDeleteFile _ANSI_ARGS_((Tcl_DString *pathPtr));
static int		DoRemoveDirectory _ANSI_ARGS_((Tcl_DString *pathPtr,
			    int recursive, Tcl_DString *errorPtr));
static int		DoRenameFile _ANSI_ARGS_((CONST char *src,
			    CONST char *dst));
static int		TraversalCopy _ANSI_ARGS_((Tcl_DString *srcPtr,
			    Tcl_DString *dstPtr, CONST struct stat *statBufPtr,
			    int type, Tcl_DString *errorPtr));
static int		TraversalDelete _ANSI_ARGS_((Tcl_DString *srcPtr,
			    Tcl_DString *dstPtr, CONST struct stat *statBufPtr,
			    int type, Tcl_DString *errorPtr));
static int		TraverseUnixTree _ANSI_ARGS_((
			    TraversalProc *traversalProc,
			    Tcl_DString *sourcePtr, Tcl_DString *destPtr,
			    Tcl_DString *errorPtr));

/*
 *---------------------------------------------------------------------------
 *
 * TclpRenameFile, DoRenameFile --
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
TclpRenameFile(src, dst)
    CONST char *src;		/* Pathname of file or dir to be renamed
				 * (UTF-8). */
    CONST char *dst;		/* New pathname of file or directory
				 * (UTF-8). */
{
    int result;
    Tcl_DString srcString, dstString;

    Tcl_UtfToExternalDString(NULL, src, -1, &srcString);
    Tcl_UtfToExternalDString(NULL, dst, -1, &dstString);
    result = DoRenameFile(Tcl_DStringValue(&srcString),
	    Tcl_DStringValue(&dstString));
    Tcl_DStringFree(&srcString);
    Tcl_DStringFree(&dstString);
    return result;
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
	struct dirent *dirEntPtr;

	if ((realpath((char *) src, srcPath) != NULL)	/* INTL: Native. */
		&& (realpath((char *) dst, dstPath) != NULL) /* INTL: Native. */
		&& (strncmp(srcPath, dstPath, strlen(srcPath)) != 0)) {
	    dirPtr = opendir(dst);			/* INTL: Native. */
	    if (dirPtr != NULL) {
		while (1) {
		    dirEntPtr = readdir(dirPtr);	/* INTL: Native. */
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
 * TclpCopyFile, DoCopyFile --
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
TclpCopyFile(src, dst)
    CONST char *src;		/* Pathname of file to be copied (UTF-8). */
    CONST char *dst;		/* Pathname of file to copy to (UTF-8). */
{
    int result;
    Tcl_DString srcString, dstString;

    Tcl_UtfToExternalDString(NULL, src, -1, &srcString);
    Tcl_UtfToExternalDString(NULL, dst, -1, &dstString);
    result = DoCopyFile(&srcString, &dstString);
    Tcl_DStringFree(&srcString);
    Tcl_DStringFree(&dstString);
    return result;
}

static int
DoCopyFile(srcPtr, dstPtr)
    Tcl_DString *srcPtr;	/* Pathname of file to be copied (native). */
    Tcl_DString *dstPtr;	/* Pathname of file to copy to (native). */
{
    struct stat srcStatBuf, dstStatBuf;
    CONST char *src, *dst;

    src = Tcl_DStringValue(srcPtr);
    dst = Tcl_DStringValue(dstPtr);

    /*
     * Have to do a stat() to determine the filetype.
     */
    
    if (lstat(src, &srcStatBuf) != 0) {			/* INTL: Native. */
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
    
    if (lstat(dst, &dstStatBuf) == 0) {			/* INTL: Native. */
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
    CONST struct stat *statBufPtr;
				/* Used to determine mode and blocksize. */
{
    int srcFd;
    int dstFd;
    u_int blockSize;   /* Optimal I/O blocksize for filesystem */
    char *buffer;      /* Data buffer for copy */
    size_t nread;

    if ((srcFd = open(src, O_RDONLY, 0)) < 0) {		/* INTL: Native. */
	return TCL_ERROR;
    }

    dstFd = open(dst, O_CREAT | O_TRUNC | O_WRONLY,	/* INTL: Native. */
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
 * TclpDeleteFile, DoDeleteFile --
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
TclpDeleteFile(path) 
    CONST char *path;		/* Pathname of file to be removed (UTF-8). */
{
    int result;
    Tcl_DString pathString;

    Tcl_UtfToExternalDString(NULL, path, -1, &pathString);
    result = DoDeleteFile(&pathString);
    Tcl_DStringFree(&pathString);
    return result;
}

static int
DoDeleteFile(pathPtr)
    Tcl_DString *pathPtr;	/* Pathname of file to be removed (native). */
{
    CONST char *path;

    path = Tcl_DStringValue(pathPtr);
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
TclpCreateDirectory(path)
    CONST char *path;		/* Pathname of directory to create (UTF-8). */
{
    int result;
    Tcl_DString pathString;

    Tcl_UtfToExternalDString(NULL, path, -1, &pathString);
    result = DoCreateDirectory(&pathString);
    Tcl_DStringFree(&pathString);
    return result;
}

static int
DoCreateDirectory(pathPtr)
    Tcl_DString *pathPtr;	/* Pathname of directory to create (native). */
{
    mode_t mode;
    CONST char *path;

    path = Tcl_DStringValue(pathPtr);

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
 * TclpCopyDirectory --
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
 *	is stored in errorPtr.  See TclpCreateDirectory and TclpCopyFile
 *	for a description of possible values for errno.
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
TclpCopyDirectory(src, dst, errorPtr)
    CONST char *src;		/* Pathname of directory to be copied
				 * (UTF-8). */
    CONST char *dst;		/* Pathname of target directory (UTF-8). */
    Tcl_DString *errorPtr;	/* If non-NULL, uninitialized or free
				 * DString filled with UTF-8 name of file
				 * causing error. */
{
    Tcl_DString srcString, dstString;
    int result;

    Tcl_UtfToExternalDString(NULL, src, -1, &srcString);
    Tcl_UtfToExternalDString(NULL, dst, -1, &dstString);

    result = TraverseUnixTree(TraversalCopy, &srcString, &dstString, errorPtr);

    Tcl_DStringFree(&srcString);
    Tcl_DStringFree(&dstString);
    return result;
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
TclpRemoveDirectory(path, recursive, errorPtr) 
    CONST char *path;		/* Pathname of directory to be removed
				 * (UTF-8). */
    int recursive;		/* If non-zero, removes directories that
				 * are nonempty.  Otherwise, will only remove
				 * empty directories. */
    Tcl_DString *errorPtr;	/* If non-NULL, uninitialized or free
				 * DString filled with UTF-8 name of file
				 * causing error. */
{
    int result;
    Tcl_DString pathString;

    Tcl_UtfToExternalDString(NULL, path, -1, &pathString);
    result = DoRemoveDirectory(&pathString, recursive, errorPtr);
    Tcl_DStringFree(&pathString);

    return result;
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

    path = Tcl_DStringValue(pathPtr);
    if (rmdir(path) == 0) {				/* INTL: Native. */
	return TCL_OK;
    }
    if (errno == ENOTEMPTY) {
	errno = EEXIST;
    }
    if ((errno != EEXIST) || (recursive == 0)) {
	if (errorPtr != NULL) {
	    Tcl_ExternalToUtfDString(NULL, path, -1, errorPtr);
	}
	return TCL_ERROR;
    }
    
    /*
     * The directory is nonempty, but the recursive flag has been
     * specified, so we recursively remove all the files in the directory.
     */

    return TraverseUnixTree(TraversalDelete, pathPtr, NULL, errorPtr);
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
    struct stat statBuf;
    CONST char *source, *errfile;
    int result, sourceLen;
    int targetLen;
    struct dirent *dirEntPtr;
    DIR *dirPtr;

    errfile = NULL;
    result = TCL_OK;
    targetLen = 0;		/* lint. */

    source = Tcl_DStringValue(sourcePtr);
    if (lstat(source, &statBuf) != 0) {			/* INTL: Native. */
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
				  
    while ((dirEntPtr = readdir(dirPtr)) != NULL) {	/* INTL: Native. */
	if ((strcmp(dirEntPtr->d_name, ".") == 0)
	        || (strcmp(dirEntPtr->d_name, "..") == 0)) {
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
 *      Called from TraverseUnixTree in order to execute a recursive copy of a 
 *      directory. 
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
    CONST struct stat *statBufPtr;
				/* Stat info for file specified by srcPtr. */
    int type;                   /* Reason for call - see TraverseUnixTree(). */
    Tcl_DString *errorPtr;	/* If non-NULL, uninitialized or free
				 * DString filled with UTF-8 name of file
				 * causing error. */
{
    switch (type) {
	case DOTREE_F:
	    if (DoCopyFile(srcPtr, dstPtr) == TCL_OK) {
		return TCL_OK;
	    }
	    break;

	case DOTREE_PRED:
	    if (DoCreateDirectory(dstPtr) == TCL_OK) {
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
    CONST struct stat *statBufPtr;
				/* Stat info for file specified by srcPtr. */
    int type;                   /* Reason for call - see TraverseUnixTree(). */
    Tcl_DString *errorPtr;	/* If non-NULL, uninitialized or free
				 * DString filled with UTF-8 name of file
				 * causing error. */
{
    switch (type) {
        case DOTREE_F: {
	    if (DoDeleteFile(srcPtr) == 0) {
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
    CONST struct stat *statBufPtr;
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
    CONST char *fileName;	/* The name of the file (UTF-8). */
    Tcl_Obj **attributePtrPtr;	/* A pointer to return the object with. */
{
    struct stat statBuf;
    struct group *groupPtr;
    int result;

    result = TclStat(fileName, &statBuf);
    
    if (result != 0) {
	Tcl_AppendResult(interp, "could not read \"", fileName, "\": ",
		Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }

    groupPtr = getgrgid(statBuf.st_gid);		/* INTL: Native. */
    if (groupPtr == NULL) {
	*attributePtrPtr = Tcl_NewIntObj(statBuf.st_gid);
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
    CONST char *fileName;	/* The name of the file (UTF-8). */
    Tcl_Obj **attributePtrPtr;	/* A pointer to return the object with. */
{
    struct stat statBuf;
    struct passwd *pwPtr;
    int result;

    result = TclStat(fileName, &statBuf);
    
    if (result != 0) {
	Tcl_AppendResult(interp, "could not read \"", fileName, "\": ",
		Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }

    pwPtr = getpwuid(statBuf.st_uid);			/* INTL: Native. */
    if (pwPtr == NULL) {
	*attributePtrPtr = Tcl_NewIntObj(statBuf.st_uid);
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
    CONST char *fileName;	    /* The name of the file (UTF-8). */
    Tcl_Obj **attributePtrPtr;	    /* A pointer to return the object with. */
{
    struct stat statBuf;
    char returnString[7];
    int result;

    result = TclStat(fileName, &statBuf);
    
    if (result != 0) {
	Tcl_AppendResult(interp, "could not read \"", fileName, "\": ",
		Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }

    sprintf(returnString, "%0#5lo", (statBuf.st_mode & 0x00007FFF));

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
    CONST char *fileName;	    /* The name of the file (UTF-8). */
    Tcl_Obj *attributePtr;	    /* New group for file. */
{
    long gid;
    int result;
    Tcl_DString ds;
    CONST char *native;

    if (Tcl_GetLongFromObj(NULL, attributePtr, &gid) != TCL_OK) {
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
		    fileName, "\": group \"", string, "\" does not exist",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	gid = groupPtr->gr_gid;
    }

    native = Tcl_UtfToExternalDString(NULL, fileName, -1, &ds);
    result = chown(native, -1, (gid_t) gid);		/* INTL: Native. */
    Tcl_DStringFree(&ds);

    endgrent();
    if (result != 0) {
	Tcl_AppendResult(interp, "could not set group for file \"",
		fileName, "\": ", Tcl_PosixError(interp), (char *) NULL);
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
    CONST char *fileName;	    /* The name of the file (UTF-8). */
    Tcl_Obj *attributePtr;	    /* New owner for file. */
{
    long uid;
    int result;
    Tcl_DString ds;
    CONST char *native;

    if (Tcl_GetLongFromObj(NULL, attributePtr, &uid) != TCL_OK) {
	struct passwd *pwPtr;
	CONST char *string;
	int length;

	string = Tcl_GetStringFromObj(attributePtr, &length);

	native = Tcl_UtfToExternalDString(NULL, string, length, &ds);
	pwPtr = getpwnam(native);			/* INTL: Native. */
	Tcl_DStringFree(&ds);

	if (pwPtr == NULL) {
	    Tcl_AppendResult(interp, "could not set owner for file \"",
		    fileName, "\": user \"", string, "\" does not exist",
		    (char *) NULL);
	    return TCL_ERROR;
	}
	uid = pwPtr->pw_uid;
    }

    native = Tcl_UtfToExternalDString(NULL, fileName, -1, &ds);
    result = chown(native, uid, -1);			/* INTL: Native. */
    Tcl_DStringFree(&ds);

    if (result != 0) {
	Tcl_AppendResult(interp, "could not set owner for file \"", fileName,
		"\": ", Tcl_PosixError(interp), (char *) NULL);
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
    CONST char *fileName;	    /* The name of the file (UTF-8). */
    Tcl_Obj *attributePtr;	    /* The attribute to set. */
{
    long mode;
    int result;
    CONST char *native;
    Tcl_DString ds;

    if (Tcl_GetLongFromObj(interp, attributePtr, &mode) != TCL_OK) {
	return TCL_ERROR;
    }

    native = Tcl_UtfToExternalDString(NULL, fileName, -1, &ds);
    result = chmod(native, (mode_t) mode);		/* INTL: Native. */
    Tcl_DStringFree(&ds);
    if (result != 0) {
	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		"could not set permissions for file \"", fileName, "\": ",
		Tcl_PosixError(interp), (char *) NULL);
	return TCL_ERROR;
    }
    return TCL_OK;
}
/*
 *---------------------------------------------------------------------------
 *
 * TclpListVolumes --
 *
 *	Lists the currently mounted volumes, which on UNIX is just /.
 *
 * Results:
 *	A standard Tcl result.  Will always be TCL_OK, since there is no way
 *	that this command can fail.  Also, the interpreter's result is set to 
 *	the list of volumes.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

int
TclpListVolumes(interp)
    Tcl_Interp *interp;			/* Interpreter to which to pass
					 * the volume list. */
{
    Tcl_Obj *resultPtr;
    
    resultPtr = Tcl_GetObjResult(interp);
    Tcl_SetStringObj(resultPtr, "/", 1);
    return TCL_OK;	
}

