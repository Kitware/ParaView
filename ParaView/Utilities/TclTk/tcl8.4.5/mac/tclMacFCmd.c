/* 
 * tclMacFCmd.c --
 *
 * Implements the Macintosh specific portions of the file manipulation
 * subcommands of the "file" command.
 *
 * Copyright (c) 1996-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tclInt.h"
#include "tclMac.h"
#include "tclMacInt.h"
#include "tclPort.h"
#include <FSpCompat.h>
#include <MoreFilesExtras.h>
#include <Strings.h>
#include <Errors.h>
#include <FileCopy.h>
#include <DirectoryCopy.h>
#include <Script.h>
#include <string.h>
#include <Finder.h>
#include <Aliases.h>

/*
 * Callback for the file attributes code.
 */

static int		GetFileFinderAttributes _ANSI_ARGS_((Tcl_Interp *interp,
			    int objIndex, Tcl_Obj *fileName,
			    Tcl_Obj **attributePtrPtr));
static int		GetFileReadOnly _ANSI_ARGS_((Tcl_Interp *interp,
			    int objIndex, Tcl_Obj *fileName,
			    Tcl_Obj **readOnlyPtrPtr));
static int		SetFileFinderAttributes _ANSI_ARGS_((Tcl_Interp *interp,
			    int objIndex, Tcl_Obj *fileName,
			    Tcl_Obj *attributePtr));
static int		SetFileReadOnly _ANSI_ARGS_((Tcl_Interp *interp,
			    int objIndex, Tcl_Obj *fileName,
			    Tcl_Obj *readOnlyPtr));

/*
 * These are indeces into the tclpFileAttrsStrings table below.
 */

#define MAC_CREATOR_ATTRIBUTE	0
#define MAC_HIDDEN_ATTRIBUTE	1
#define MAC_READONLY_ATTRIBUTE	2
#define MAC_TYPE_ATTRIBUTE	3

/*
 * Global variables for the file attributes code.
 */

CONST char *tclpFileAttrStrings[] = {"-creator", "-hidden", "-readonly",
	"-type", (char *) NULL};
CONST TclFileAttrProcs tclpFileAttrProcs[] = {
	{GetFileFinderAttributes, SetFileFinderAttributes},
	{GetFileFinderAttributes, SetFileFinderAttributes},
	{GetFileReadOnly, SetFileReadOnly},
	{GetFileFinderAttributes, SetFileFinderAttributes}};

/*
 * File specific static data
 */

static long startSeed = 248923489;

/*
 * Prototypes for procedure only used in this file
 */

static pascal Boolean 	CopyErrHandler _ANSI_ARGS_((OSErr error, 
			    short failedOperation,
			    short srcVRefNum, long srcDirID,
			    ConstStr255Param srcName, short dstVRefNum,
			    long dstDirID,ConstStr255Param dstName));
static int		DoCopyDirectory _ANSI_ARGS_((CONST char *src,
			    CONST char *dst, Tcl_DString *errorPtr));
static int		DoCopyFile _ANSI_ARGS_((CONST char *src, 
			    CONST char *dst));
static int		DoCreateDirectory _ANSI_ARGS_((CONST char *path));
static int		DoRemoveDirectory _ANSI_ARGS_((CONST char *path, 
			    int recursive, Tcl_DString *errorPtr));
static int		DoRenameFile _ANSI_ARGS_((CONST char *src,
			    CONST char *dst));
OSErr			FSpGetFLockCompat _ANSI_ARGS_((const FSSpec *specPtr, 
			    Boolean *lockedPtr));
static OSErr		GetFileSpecs _ANSI_ARGS_((CONST char *path, 
			    FSSpec *pathSpecPtr, FSSpec *dirSpecPtr,	
			    Boolean *pathExistsPtr, 
			    Boolean *pathIsDirectoryPtr));
static OSErr		MoveRename _ANSI_ARGS_((const FSSpec *srcSpecPtr, 
			    const FSSpec *dstSpecPtr, StringPtr copyName));
static int		Pstrequal _ANSI_ARGS_((ConstStr255Param stringA, 
			    ConstStr255Param stringB));
                 
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
 *	ENOENT:	    src doesn't exist.  src or dst is "".
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
DoRenameFile(
    CONST char *src,		/* Pathname of file or dir to be renamed
				 * (native). */
    CONST char *dst)		/* New pathname of file or directory
				 * (native). */
{
    FSSpec srcFileSpec, dstFileSpec, dstDirSpec;
    OSErr err; 
    long srcID, dummy;
    Boolean srcIsDirectory, dstIsDirectory, dstExists, dstLocked;

    err = FSpLLocationFromPath(strlen(src), src, &srcFileSpec);
    if (err == noErr) {
	FSpGetDirectoryID(&srcFileSpec, &srcID, &srcIsDirectory);
    }
    if (err == noErr) {
        err = GetFileSpecs(dst, &dstFileSpec, &dstDirSpec, &dstExists, 
        	&dstIsDirectory);
    }
    if (err == noErr) {
	if (dstExists == 0) {
            err = MoveRename(&srcFileSpec, &dstDirSpec, dstFileSpec.name);
            goto end;
        }
        err = FSpGetFLockCompat(&dstFileSpec, &dstLocked);
        if (dstLocked) {
            FSpRstFLockCompat(&dstFileSpec);
        }
    }
    if (err == noErr) {
        if (srcIsDirectory) {
	    if (dstIsDirectory) {
		/*
		 * The following call will remove an empty directory.  If it
		 * fails, it's because it wasn't empty.
		 */
		 
                if (DoRemoveDirectory(dst, 0, NULL) != TCL_OK) {
                    return TCL_ERROR;
                }
                
                /*
		 * Now that that empty directory is gone, we can try
		 * renaming src.  If that fails, we'll put this empty
		 * directory back, for completeness.
		 */

		err = MoveRename(&srcFileSpec, &dstDirSpec, dstFileSpec.name);
                if (err != noErr) {
		    FSpDirCreateCompat(&dstFileSpec, smSystemScript, &dummy);
		    if (dstLocked) {
		        FSpSetFLockCompat(&dstFileSpec);
		    }
		}
	    } else {
	        errno = ENOTDIR;
	        return TCL_ERROR;
	    }
	} else {   
	    if (dstIsDirectory) {
		errno = EISDIR;
		return TCL_ERROR;
	    } else {                                
		/*
		 * Overwrite existing file by:
		 * 
		 * 1. Rename existing file to temp name.
		 * 2. Rename old file to new name.
		 * 3. If success, delete temp file.  If failure,
		 *    put temp file back to old name.
		 */

	        Str31 tmpName;
	        FSSpec tmpFileSpec;

	        err = GenerateUniqueName(dstFileSpec.vRefNum, &startSeed,
	        	dstFileSpec.parID, dstFileSpec.parID, tmpName);
	        if (err == noErr) {
	            err = FSpRenameCompat(&dstFileSpec, tmpName);
	        }
	        if (err == noErr) {
	            err = FSMakeFSSpecCompat(dstFileSpec.vRefNum,
	            	    dstFileSpec.parID, tmpName, &tmpFileSpec);
	        }
	        if (err == noErr) {
	            err = MoveRename(&srcFileSpec, &dstDirSpec, 
	            	    dstFileSpec.name);
	        }
	        if (err == noErr) {
		    FSpDeleteCompat(&tmpFileSpec);
		} else {
		    FSpDeleteCompat(&dstFileSpec);
		    FSpRenameCompat(&tmpFileSpec, dstFileSpec.name);
	            if (dstLocked) {
	            	FSpSetFLockCompat(&dstFileSpec);
	            }
	        }
	    }
   	}
    }    

    end:    
    if (err != noErr) {
	errno = TclMacOSErrorToPosixError(err);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *--------------------------------------------------------------------------
 *
 * MoveRename --
 *
 *	Helper function for TclpRenameFile.  Renames a file or directory
 *	into the same directory or another directory.  The target name
 * 	must not already exist in the destination directory.
 *
 *	Don't use FSpMoveRenameCompat because it doesn't work with
 *	directories or with locked files. 
 *
 * Results:
 *	Returns a mac error indicating the cause of the failure.
 *
 * Side effects:
 *	Creates a temp file in the target directory to handle a rename
 *	between directories.
 *
 *--------------------------------------------------------------------------
 */
  
static OSErr		
MoveRename(
    const FSSpec *srcFileSpecPtr,   /* Source object. */
    const FSSpec *dstDirSpecPtr,    /* Destination directory. */
    StringPtr copyName)		    /* New name for object in destination 
    				     * directory. */
{
    OSErr err;
    long srcID, dstID;
    Boolean srcIsDir, dstIsDir;
    Str31 tmpName;
    FSSpec dstFileSpec, srcDirSpec, tmpSrcFileSpec, tmpDstFileSpec;
    Boolean locked;
    
    if (srcFileSpecPtr->parID == 1) {
        /*
         * Trying to rename a volume.
         */
          
        return badMovErr;
    }
    if (srcFileSpecPtr->vRefNum != dstDirSpecPtr->vRefNum) {
	/*
	 * Renaming across volumes.
	 */
	 
        return diffVolErr;
    }
    err = FSpGetFLockCompat(srcFileSpecPtr, &locked);
    if (locked) {
        FSpRstFLockCompat(srcFileSpecPtr);
    }
    if (err == noErr) {
	err = FSpGetDirectoryID(dstDirSpecPtr, &dstID, &dstIsDir);
    }
    if (err == noErr) {
        if (srcFileSpecPtr->parID == dstID) {
            /*
             * Renaming object within directory. 
             */
            
            err = FSpRenameCompat(srcFileSpecPtr, copyName);
            goto done; 
        }
        if (Pstrequal(srcFileSpecPtr->name, copyName)) {
	    /*
	     * Moving object to another directory (under same name). 
	     */
	 
	    err = FSpCatMoveCompat(srcFileSpecPtr, dstDirSpecPtr);
	    goto done; 
        } 
        err = FSpGetDirectoryID(srcFileSpecPtr, &srcID, &srcIsDir);
    } 
    if (err == noErr) {
        /*
         * Fullblown: rename source object to temp name, move temp to
         * dest directory, and rename temp to target.
         */
          
        err = GenerateUniqueName(srcFileSpecPtr->vRefNum, &startSeed,
       		srcFileSpecPtr->parID, dstID, tmpName);
        FSMakeFSSpecCompat(srcFileSpecPtr->vRefNum, srcFileSpecPtr->parID,
         	tmpName, &tmpSrcFileSpec);
        FSMakeFSSpecCompat(dstDirSpecPtr->vRefNum, dstID, tmpName,
         	&tmpDstFileSpec);
    }
    if (err == noErr) {
        err = FSpRenameCompat(srcFileSpecPtr, tmpName);
    }
    if (err == noErr) {
        err = FSpCatMoveCompat(&tmpSrcFileSpec, dstDirSpecPtr);
        if (err == noErr) {
            err = FSpRenameCompat(&tmpDstFileSpec, copyName);
            if (err == noErr) {
                goto done;
            }
            FSMakeFSSpecCompat(srcFileSpecPtr->vRefNum, srcFileSpecPtr->parID,
             	    NULL, &srcDirSpec);
            FSpCatMoveCompat(&tmpDstFileSpec, &srcDirSpec);
        }                 
        FSpRenameCompat(&tmpSrcFileSpec, srcFileSpecPtr->name);
    }
    
    done:
    if (locked != false) {
    	if (err == noErr) {
	    FSMakeFSSpecCompat(dstDirSpecPtr->vRefNum, 
	    	    dstID, copyName, &dstFileSpec);
            FSpSetFLockCompat(&dstFileSpec);
        } else {
            FSpSetFLockCompat(srcFileSpecPtr);
        }
    }
    return err;
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
DoCopyFile(
    CONST char *src,		/* Pathname of file to be copied (native). */
    CONST char *dst)		/* Pathname of file to copy to (native). */
{
    OSErr err, dstErr;
    Boolean dstExists, dstIsDirectory, dstLocked;
    FSSpec srcFileSpec, dstFileSpec, dstDirSpec, tmpFileSpec;
    Str31 tmpName;
	
    err = FSpLLocationFromPath(strlen(src), src, &srcFileSpec);
    if (err == noErr) {
        err = GetFileSpecs(dst, &dstFileSpec, &dstDirSpec, &dstExists,
        	&dstIsDirectory);
    }
    if (dstExists) {
        if (dstIsDirectory) {
            errno = EISDIR;
            return TCL_ERROR;
        }
        err = FSpGetFLockCompat(&dstFileSpec, &dstLocked);
        if (dstLocked) {
            FSpRstFLockCompat(&dstFileSpec);
        }
        
        /*
         * Backup dest file.
         */
         
        dstErr = GenerateUniqueName(dstFileSpec.vRefNum, &startSeed, dstFileSpec.parID, 
    	        dstFileSpec.parID, tmpName);
        if (dstErr == noErr) {
            dstErr = FSpRenameCompat(&dstFileSpec, tmpName);
        }   
    }
    if (err == noErr) {
    	err = FSpFileCopy(&srcFileSpec, &dstDirSpec, 
    		(StringPtr) dstFileSpec.name, NULL, 0, true);
    }
    if ((dstExists != false) && (dstErr == noErr)) {
        FSMakeFSSpecCompat(dstFileSpec.vRefNum, dstFileSpec.parID,
        	tmpName, &tmpFileSpec);
	if (err == noErr) {
	    /* 
	     * Delete backup file. 
	     */
	     
	    FSpDeleteCompat(&tmpFileSpec);
	} else {
	
	    /* 
	     * Restore backup file.
	     */
	     
	    FSpDeleteCompat(&dstFileSpec);
	    FSpRenameCompat(&tmpFileSpec, dstFileSpec.name);
	    if (dstLocked) {
	        FSpSetFLockCompat(&dstFileSpec);
	    }
	}
    }
    
    if (err != noErr) {
	errno = TclMacOSErrorToPosixError(err);
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
TclpDeleteFile(
    CONST char *path)		/* Pathname of file to be removed (native). */
{
    OSErr err;
    FSSpec fileSpec;
    Boolean isDirectory;
    long dirID;
    
    err = FSpLLocationFromPath(strlen(path), path, &fileSpec);
    if (err == noErr) {
	/*
     	 * Since FSpDeleteCompat will delete an empty directory, make sure
     	 * that this isn't a directory first.
         */
        
        FSpGetDirectoryID(&fileSpec, &dirID, &isDirectory);
	if (isDirectory == true) {
            errno = EISDIR;
            return TCL_ERROR;
        }
    }
    err = FSpDeleteCompat(&fileSpec);
    if (err == fLckdErr) {
    	FSpRstFLockCompat(&fileSpec);
    	err = FSpDeleteCompat(&fileSpec);
    	if (err != noErr) {
    	    FSpSetFLockCompat(&fileSpec);
    	}
    }
    if (err != noErr) {
	errno = TclMacOSErrorToPosixError(err);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjCreateDirectory, DoCreateDirectory --
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
DoCreateDirectory(
    CONST char *path)		/* Pathname of directory to create (native). */
{
    OSErr err;
    FSSpec dirSpec;
    long outDirID;
	
    err = FSpLocationFromPath(strlen(path), path, &dirSpec);
    if (err == noErr) {
        err = dupFNErr;		/* EEXIST. */
    } else if (err == fnfErr) {
        err = FSpDirCreateCompat(&dirSpec, smSystemScript, &outDirID);
    } 
    
    if (err != noErr) {
	errno = TclMacOSErrorToPosixError(err);
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjCopyDirectory, DoCopyDirectory --
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
TclpObjCopyDirectory(srcPathPtr, destPathPtr, errorPtr)
    Tcl_Obj *srcPathPtr;
    Tcl_Obj *destPathPtr;
    Tcl_Obj **errorPtr;
{
    Tcl_DString ds;
    int ret;
    ret = DoCopyDirectory(Tcl_FSGetNativePath(srcPathPtr),
			  Tcl_FSGetNativePath(destPathPtr), &ds);
    if (ret != TCL_OK) {
	*errorPtr = Tcl_NewStringObj(Tcl_DStringValue(&ds), -1);
	Tcl_DStringFree(&ds);
	Tcl_IncrRefCount(*errorPtr);
    }
    return ret;
}

static int
DoCopyDirectory(
    CONST char *src,		/* Pathname of directory to be copied
				 * (Native). */
    CONST char *dst,		/* Pathname of target directory (Native). */
    Tcl_DString *errorPtr)	/* If non-NULL, uninitialized or free
				 * DString filled with UTF-8 name of file
				 * causing error. */
{
    OSErr err, saveErr;
    long srcID, tmpDirID;
    FSSpec srcFileSpec, dstFileSpec, dstDirSpec, tmpDirSpec, tmpFileSpec;
    Boolean srcIsDirectory, srcLocked;
    Boolean dstIsDirectory, dstExists;
    Str31 tmpName;

    err = FSpLocationFromPath(strlen(src), src, &srcFileSpec);
    if (err == noErr) {
    	err = FSpGetDirectoryID(&srcFileSpec, &srcID, &srcIsDirectory);
    }
    if (err == noErr) {
        if (srcIsDirectory == false) {
            err = afpObjectTypeErr;	/* ENOTDIR. */
        }
    }
    if (err == noErr) {
        err = GetFileSpecs(dst, &dstFileSpec, &dstDirSpec, &dstExists,
        	&dstIsDirectory);
    }
    if (dstExists) {
        if (dstIsDirectory == false) {
            err = afpObjectTypeErr;	/* ENOTDIR. */
        } else {
            err = dupFNErr;		/* EEXIST. */
        }
    }
    if (err != noErr) {
        goto done;
    }        
    if ((srcFileSpec.vRefNum == dstFileSpec.vRefNum) &&
    	    (srcFileSpec.parID == dstFileSpec.parID) &&
            (Pstrequal(srcFileSpec.name, dstFileSpec.name) != 0)) {
        /*
         * Copying on top of self.  No-op.
         */
                    
        goto done;
    }

    /*
     * This algorthm will work making a copy of the source directory in
     * the current directory with a new name, in a new directory with the
     * same name, and in a new directory with a new name:
     *
     * 1. Make dstDir/tmpDir.
     * 2. Copy srcDir/src to dstDir/tmpDir/src
     * 3. Rename dstDir/tmpDir/src to dstDir/tmpDir/dst (if necessary).
     * 4. CatMove dstDir/tmpDir/dst to dstDir/dst.
     * 5. Remove dstDir/tmpDir.
     */
                
    err = FSpGetFLockCompat(&srcFileSpec, &srcLocked);
    if (srcLocked) {
        FSpRstFLockCompat(&srcFileSpec);
    }
    if (err == noErr) {
        err = GenerateUniqueName(dstFileSpec.vRefNum, &startSeed, dstFileSpec.parID, 
    	        dstFileSpec.parID, tmpName);
    }
    if (err == noErr) {
        FSMakeFSSpecCompat(dstFileSpec.vRefNum, dstFileSpec.parID,
        	tmpName, &tmpDirSpec);
        err = FSpDirCreateCompat(&tmpDirSpec, smSystemScript, &tmpDirID);
    }
    if (err == noErr) {
	err = FSpDirectoryCopy(&srcFileSpec, &tmpDirSpec, NULL, NULL, 0, true,
	    	CopyErrHandler);
    }
    
    /* 
     * Even if the Copy failed, Rename/Move whatever did get copied to the
     * appropriate final destination, if possible.  
     */
     
    saveErr = err;
    err = noErr;
    if (Pstrequal(srcFileSpec.name, dstFileSpec.name) == 0) {
        err = FSMakeFSSpecCompat(tmpDirSpec.vRefNum, tmpDirID, 
        	srcFileSpec.name, &tmpFileSpec);
        if (err == noErr) {
            err = FSpRenameCompat(&tmpFileSpec, dstFileSpec.name);
        }
    }
    if (err == noErr) {
        err = FSMakeFSSpecCompat(tmpDirSpec.vRefNum, tmpDirID,
        	dstFileSpec.name, &tmpFileSpec);
    }
    if (err == noErr) {
        err = FSpCatMoveCompat(&tmpFileSpec, &dstDirSpec);
    }
    if (err == noErr) {
        if (srcLocked) {
            FSpSetFLockCompat(&dstFileSpec);
        }
    }
    
    FSpDeleteCompat(&tmpDirSpec);
    
    if (saveErr != noErr) {
        err = saveErr;
    }
    
    done:
    if (err != noErr) {
        errno = TclMacOSErrorToPosixError(err);
        if (errorPtr != NULL) {
            Tcl_ExternalToUtfDString(NULL, dst, -1, errorPtr);
        }
        return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * CopyErrHandler --
 *
 *      This procedure is called from the MoreFiles procedure 
 *      FSpDirectoryCopy whenever an error occurs.
 *
 * Results:
 *      False if the condition should not be considered an error, true
 *      otherwise.
 *
 * Side effects:
 *      Since FSpDirectoryCopy() is called only after removing any 
 *      existing target directories, there shouldn't be any errors.
 *      
 *----------------------------------------------------------------------
 */

static pascal Boolean 
CopyErrHandler(
    OSErr error,		/* Error that occured */
    short failedOperation,	/* operation that caused the error */
    short srcVRefNum,		/* volume ref number of source */
    long srcDirID,		/* directory id of source */
    ConstStr255Param srcName,	/* name of source */
    short dstVRefNum,		/* volume ref number of dst */
    long dstDirID,		/* directory id of dst */
    ConstStr255Param dstName)	/* name of dst directory */
{
    return true;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjRemoveDirectory, DoRemoveDirectory --
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
    int ret;
    ret = DoRemoveDirectory(Tcl_FSGetNativePath(pathPtr),recursive, &ds);
    if (ret != TCL_OK) {
	*errorPtr = Tcl_NewStringObj(Tcl_DStringValue(&ds), -1);
	Tcl_DStringFree(&ds);
	Tcl_IncrRefCount(*errorPtr);
    }
    return ret;
}

static int
DoRemoveDirectory(
    CONST char *path,		/* Pathname of directory to be removed
				 * (native). */
    int recursive,		/* If non-zero, removes directories that
				 * are nonempty.  Otherwise, will only remove
				 * empty directories. */
    Tcl_DString *errorPtr)	/* If non-NULL, uninitialized or free
				 * DString filled with UTF-8 name of file
				 * causing error. */
{
    OSErr err;
    FSSpec fileSpec;
    long dirID;
    int locked;
    Boolean isDirectory;
    CInfoPBRec pb;
    Str255 fileName;


    locked = 0;
    err = FSpLocationFromPath(strlen(path), path, &fileSpec);
    if (err != noErr) {
        goto done;
    }   

    /*
     * Since FSpDeleteCompat will delete a file, make sure this isn't
     * a file first.
     */
         
    isDirectory = 1;
    FSpGetDirectoryID(&fileSpec, &dirID, &isDirectory);
    if (isDirectory == 0) {
        errno = ENOTDIR;
        return TCL_ERROR;
    }
    
    err = FSpDeleteCompat(&fileSpec);
    if (err == fLckdErr) {
        locked = 1;
    	FSpRstFLockCompat(&fileSpec);
    	err = FSpDeleteCompat(&fileSpec);
    }
    if (err == noErr) {
	return TCL_OK;
    }
    if (err != fBsyErr) {
        goto done;
    }
     
    if (recursive == 0) {
	/*
	 * fBsyErr means one of three things: file busy, directory not empty, 
	 * or working directory control block open.  Determine if directory
	 * is empty. If directory is not empty, return EEXIST.
	 */

	pb.hFileInfo.ioVRefNum = fileSpec.vRefNum;
	pb.hFileInfo.ioDirID = dirID;
	pb.hFileInfo.ioNamePtr = (StringPtr) fileName;
	pb.hFileInfo.ioFDirIndex = 1;
	if (PBGetCatInfoSync(&pb) == noErr) {
	    err = dupFNErr;	/* EEXIST */
	    goto done;
	}
    }
	
    /*
     * DeleteDirectory removes a directory and all its contents, including
     * any locked files.  There is no interface to get the name of the 
     * file that caused the error, if an error occurs deleting this tree,
     * unless we rewrite DeleteDirectory ourselves.
     */
	 
    err = DeleteDirectory(fileSpec.vRefNum, dirID, NULL);

    done:
    if (err != noErr) {
	if (errorPtr != NULL) {
	    Tcl_UtfToExternalDString(NULL, path, -1, errorPtr);
	}
        if (locked) {
            FSpSetFLockCompat(&fileSpec);
        }
    	errno = TclMacOSErrorToPosixError(err);
    	return TCL_ERROR;
    }
    return TCL_OK;
}
			    
/*
 *---------------------------------------------------------------------------
 *
 * GetFileSpecs --
 *
 *	Gets FSSpecs for the specified path and its parent directory.
 *
 * Results:
 *	The return value is noErr if there was no error getting FSSpecs,
 *	otherwise it is an error describing the problem.  Fills buffers 
 *	with information, as above.  
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static OSErr
GetFileSpecs(
    CONST char *path,		/* The path to query. */
    FSSpec *pathSpecPtr,	/* Filled with information about path. */
    FSSpec *dirSpecPtr,		/* Filled with information about path's
    				 * parent directory. */
    Boolean *pathExistsPtr,	/* Set to true if path actually exists, 
    				 * false if it doesn't or there was an 
    				 * error reading the specified path. */
    Boolean *pathIsDirectoryPtr)/* Set to true if path is itself a directory,
    				 * otherwise false. */
{
    CONST char *dirName;
    OSErr err;
    int argc;
    CONST char **argv;
    long d;
    Tcl_DString buffer;
        
    *pathExistsPtr = false;
    *pathIsDirectoryPtr = false;
    
    Tcl_DStringInit(&buffer);
    Tcl_SplitPath(path, &argc, &argv);
    if (argc == 1) {
        dirName = ":";
    } else {
        dirName = Tcl_JoinPath(argc - 1, argv, &buffer);
    }
    err = FSpLocationFromPath(strlen(dirName), dirName, dirSpecPtr);
    Tcl_DStringFree(&buffer);
    ckfree((char *) argv);

    if (err == noErr) {
        err = FSpLocationFromPath(strlen(path), path, pathSpecPtr);
        if (err == noErr) {
            *pathExistsPtr = true;
            err = FSpGetDirectoryID(pathSpecPtr, &d, pathIsDirectoryPtr);
        } else if (err == fnfErr) {
            err = noErr;
        }
    }
    return err;
}

/*
 *-------------------------------------------------------------------------
 *
 * FSpGetFLockCompat --
 *
 *	Determines if there exists a software lock on the specified
 *	file.  The software lock could prevent the file from being 
 *	renamed or moved.
 *
 * Results:
 *	Standard macintosh error code.  
 *
 * Side effects:
 *	None.
 *
 *
 *-------------------------------------------------------------------------
 */
 
OSErr
FSpGetFLockCompat(
    const FSSpec *specPtr,	/* File to query. */
    Boolean *lockedPtr)		/* Set to true if file is locked, false
    				 * if it isn't or there was an error reading
    				 * specified file. */
{
    CInfoPBRec pb;
    OSErr err;
    
    pb.hFileInfo.ioVRefNum = specPtr->vRefNum;
    pb.hFileInfo.ioDirID = specPtr->parID;
    pb.hFileInfo.ioNamePtr = (StringPtr) specPtr->name;
    pb.hFileInfo.ioFDirIndex = 0;
    
    err = PBGetCatInfoSync(&pb);
    if ((err == noErr) && (pb.hFileInfo.ioFlAttrib & 0x01)) {
        *lockedPtr = true;
    } else {
        *lockedPtr = false;
    }
    return err;
}
    
/*
 *----------------------------------------------------------------------
 *
 * Pstrequal --
 *
 *      Pascal string compare. 
 *
 * Results:
 *      Returns 1 if strings equal, 0 otherwise.
 *
 * Side effects:
 *      None.
 *      
 *----------------------------------------------------------------------
 */

static int 
Pstrequal (
    ConstStr255Param stringA,	/* Pascal string A */
    ConstStr255Param stringB)   /* Pascal string B */
{
    int i, len;
    
    len = *stringA;
    for (i = 0; i <= len; i++) {
        if (*stringA++ != *stringB++) {
            return 0;
        }
    }
    return 1;
}
    
/*
 *----------------------------------------------------------------------
 *
 * GetFileFinderAttributes --
 *
 *	Returns a Tcl_Obj containing the value of a file attribute
 *	which is part of the FInfo record. Which attribute is controlled
 *	by objIndex.
 *
 * Results:
 *      Returns a standard TCL error. If the return value is TCL_OK,
 *	the new creator or file type object is put into attributePtrPtr.
 *	The object will have ref count 0. If there is an error,
 *	attributePtrPtr is not touched.
 *
 * Side effects:
 *      A new object is allocated if the file is valid.
 *      
 *----------------------------------------------------------------------
 */

static int
GetFileFinderAttributes(
    Tcl_Interp *interp,		/* The interp to report errors with. */
    int objIndex,		/* The index of the attribute option. */
    Tcl_Obj *fileName,	/* The name of the file (UTF-8). */
    Tcl_Obj **attributePtrPtr)	/* A pointer to return the object with. */
{
    OSErr err;
    FSSpec fileSpec;
    FInfo finfo;
    CONST char *native;

    native=Tcl_FSGetNativePath(fileName);
    err = FSpLLocationFromPath(strlen(native),
	    native, &fileSpec);

    if (err == noErr) {
    	err = FSpGetFInfo(&fileSpec, &finfo);
    }
    
    if (err == noErr) {
    	switch (objIndex) {
    	    case MAC_CREATOR_ATTRIBUTE:
    	    	*attributePtrPtr = Tcl_NewOSTypeObj(finfo.fdCreator);
    	    	break;
    	    case MAC_HIDDEN_ATTRIBUTE:
    	    	*attributePtrPtr = Tcl_NewBooleanObj(finfo.fdFlags
    	    		& kIsInvisible);
    	    	break;
    	    case MAC_TYPE_ATTRIBUTE:
    	    	*attributePtrPtr = Tcl_NewOSTypeObj(finfo.fdType);
    	    	break;
    	}
    } else if (err == fnfErr) {
    	long dirID;
    	Boolean isDirectory = 0;
    	
    	err = FSpGetDirectoryID(&fileSpec, &dirID, &isDirectory);
    	if ((err == noErr) && isDirectory) {
    	    if (objIndex == MAC_HIDDEN_ATTRIBUTE) {
    	    	*attributePtrPtr = Tcl_NewBooleanObj(0);
    	    } else {
    	    	*attributePtrPtr = Tcl_NewOSTypeObj('Fldr');
    	    }
    	}
    }
    
    if (err != noErr) {
    	errno = TclMacOSErrorToPosixError(err);
    	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), 
    		"could not read \"", Tcl_GetString(fileName), "\": ",
    		Tcl_PosixError(interp), (char *) NULL);
    	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * GetFileReadOnly --
 *
 *	Returns a Tcl_Obj containing a Boolean value indicating whether
 *	or not the file is read-only. The object will have ref count 0.
 *	This procedure just checks the Finder attributes; it does not
 *	check AppleShare sharing attributes.
 *
 * Results:
 *      Returns a standard TCL error. If the return value is TCL_OK,
 *	the new creator type object is put into readOnlyPtrPtr.
 *	If there is an error, readOnlyPtrPtr is not touched.
 *
 * Side effects:
 *      A new object is allocated if the file is valid.
 *      
 *----------------------------------------------------------------------
 */

static int
GetFileReadOnly(
    Tcl_Interp *interp,		/* The interp to report errors with. */
    int objIndex,		/* The index of the attribute. */
    Tcl_Obj *fileName,	/* The name of the file (UTF-8). */
    Tcl_Obj **readOnlyPtrPtr)	/* A pointer to return the object with. */
{
    OSErr err;
    FSSpec fileSpec;
    CInfoPBRec paramBlock;
    CONST char *native;

    native=Tcl_FSGetNativePath(fileName);
    err = FSpLLocationFromPath(strlen(native),
	    native, &fileSpec);
    
    if (err == noErr) {
    	if (err == noErr) {
    	    paramBlock.hFileInfo.ioCompletion = NULL;
    	    paramBlock.hFileInfo.ioNamePtr = fileSpec.name;
    	    paramBlock.hFileInfo.ioVRefNum = fileSpec.vRefNum;
    	    paramBlock.hFileInfo.ioFDirIndex = 0;
    	    paramBlock.hFileInfo.ioDirID = fileSpec.parID;
    	    err = PBGetCatInfo(&paramBlock, 0);
    	    if (err == noErr) {
    	    
    	    	/*
    	    	 * For some unknown reason, the Mac does not give
    	    	 * symbols for the bits in the ioFlAttrib field.
    	    	 * 1 -> locked.
    	    	 */
    	    
    	    	*readOnlyPtrPtr = Tcl_NewBooleanObj(
    	    		paramBlock.hFileInfo.ioFlAttrib & 1);
    	    }
    	}
    }
    if (err != noErr) {
    	errno = TclMacOSErrorToPosixError(err);
    	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), 
    		"could not read \"", Tcl_GetString(fileName), "\": ",
    		Tcl_PosixError(interp), (char *) NULL);
    	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SetFileFinderAttributes --
 *
 *	Sets the file to the creator or file type given by attributePtr.
 *	objIndex determines whether the creator or file type is set.
 *
 * Results:
 *	Returns a standard TCL error.
 *
 * Side effects:
 *      The file's attribute is set.
 *      
 *----------------------------------------------------------------------
 */

static int
SetFileFinderAttributes(
    Tcl_Interp *interp,		/* The interp to report errors with. */
    int objIndex,		/* The index of the attribute. */
    Tcl_Obj *fileName,	/* The name of the file (UTF-8). */
    Tcl_Obj *attributePtr)	/* The command line object. */
{
    OSErr err;
    FSSpec fileSpec;
    FInfo finfo;
    CONST char *native;

    native=Tcl_FSGetNativePath(fileName);
    err = FSpLLocationFromPath(strlen(native),
	    native, &fileSpec);
    
    if (err == noErr) {
    	err = FSpGetFInfo(&fileSpec, &finfo);
    }
    
    if (err == noErr) {
    	switch (objIndex) {
    	    case MAC_CREATOR_ATTRIBUTE:
    	    	if (Tcl_GetOSTypeFromObj(interp, attributePtr,
    	    		&finfo.fdCreator) != TCL_OK) {
    	    	    return TCL_ERROR;
    	    	}
    	    	break;
    	    case MAC_HIDDEN_ATTRIBUTE: {
    	    	int hidden;
    	    	
    	    	if (Tcl_GetBooleanFromObj(interp, attributePtr, &hidden)
    	    		!= TCL_OK) {
    	    	    return TCL_ERROR;
    	    	}
    	    	if (hidden) {
    	    	    finfo.fdFlags |= kIsInvisible;
    	    	} else {
    	    	    finfo.fdFlags &= ~kIsInvisible;
    	    	}
    	    	break;
    	    }
    	    case MAC_TYPE_ATTRIBUTE:
    	    	if (Tcl_GetOSTypeFromObj(interp, attributePtr,
    	    		&finfo.fdType) != TCL_OK) {
    	    	    return TCL_ERROR;
    	    	}
    	    	break;
    	}
    	err = FSpSetFInfo(&fileSpec, &finfo);
    } else if (err == fnfErr) {
    	long dirID;
    	Boolean isDirectory = 0;
    	
    	err = FSpGetDirectoryID(&fileSpec, &dirID, &isDirectory);
    	if ((err == noErr) && isDirectory) {
    	    Tcl_Obj *resultPtr = Tcl_GetObjResult(interp);
    	    Tcl_AppendStringsToObj(resultPtr, "cannot set ",
    	    	    tclpFileAttrStrings[objIndex], ": \"",
    	    	    Tcl_GetString(fileName), "\" is a directory", (char *) NULL);
    	    return TCL_ERROR;
    	}
    }
    
    if (err != noErr) {
    	errno = TclMacOSErrorToPosixError(err);
    	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), 
    		"could not read \"", Tcl_GetString(fileName), "\": ",
    		Tcl_PosixError(interp), (char *) NULL);
    	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SetFileReadOnly --
 *
 *	Sets the file to be read-only according to the Boolean value
 *	given by hiddenPtr.
 *
 * Results:
 *	Returns a standard TCL error.
 *
 * Side effects:
 *      The file's attribute is set.
 *      
 *----------------------------------------------------------------------
 */

static int
SetFileReadOnly(
    Tcl_Interp *interp,		/* The interp to report errors with. */
    int objIndex,		/* The index of the attribute. */
    Tcl_Obj *fileName,	/* The name of the file (UTF-8). */
    Tcl_Obj *readOnlyPtr)	/* The command line object. */
{
    OSErr err;
    FSSpec fileSpec;
    HParamBlockRec paramBlock;
    int hidden;
    CONST char *native;

    native=Tcl_FSGetNativePath(fileName);
    err = FSpLLocationFromPath(strlen(native),
	    native, &fileSpec);
    
    if (err == noErr) {
    	if (Tcl_GetBooleanFromObj(interp, readOnlyPtr, &hidden) != TCL_OK) {
    	    return TCL_ERROR;
    	}
    
    	paramBlock.fileParam.ioCompletion = NULL;
    	paramBlock.fileParam.ioNamePtr = fileSpec.name;
    	paramBlock.fileParam.ioVRefNum = fileSpec.vRefNum;
    	paramBlock.fileParam.ioDirID = fileSpec.parID;
    	if (hidden) {
    	    err = PBHSetFLock(&paramBlock, 0);
    	} else {
    	    err = PBHRstFLock(&paramBlock, 0);
    	}
    }
    
    if (err == fnfErr) {
    	long dirID;
    	Boolean isDirectory = 0;
    	err = FSpGetDirectoryID(&fileSpec, &dirID, &isDirectory);
    	if ((err == noErr) && isDirectory) {
    	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
    	    	    "cannot set a directory to read-only when File Sharing is turned off",
    	    	    (char *) NULL);
    	    return TCL_ERROR;
    	} else {
    	    err = fnfErr;
    	}
    }
    
    if (err != noErr) {
    	errno = TclMacOSErrorToPosixError(err);
    	Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), 
    		"could not read \"", Tcl_GetString(fileName), "\": ",
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
 *	Lists the currently mounted volumes
 *
 * Results:
 *	The list of volumes.
 *
 * Side effects:
 *	None
 *
 *---------------------------------------------------------------------------
 */
Tcl_Obj*
TclpObjListVolumes(void)
{
    HParamBlockRec pb;
    Str255 name;
    OSErr theError = noErr;
    Tcl_Obj *resultPtr, *elemPtr;
    short volIndex = 1;
    Tcl_DString dstr;

    resultPtr = Tcl_NewObj();
        
    /*
     * We use two facts:
     * 1) The Mac volumes are enumerated by the ioVolIndex parameter of
     * the HParamBlockRec.  They run through the integers contiguously, 
     * starting at 1.  
     * 2) PBHGetVInfoSync returns an error when you ask for a volume index
     * that does not exist.
     * 
     */
        
    while ( 1 ) {
        pb.volumeParam.ioNamePtr = (StringPtr) &name;
        pb.volumeParam.ioVolIndex = volIndex;
                
        theError = PBHGetVInfoSync(&pb);

        if ( theError != noErr ) {
            break;
        }
        
        Tcl_ExternalToUtfDString(NULL, (CONST char *)&name[1], name[0], &dstr);
        elemPtr = Tcl_NewStringObj(Tcl_DStringValue(&dstr),
		Tcl_DStringLength(&dstr));
        Tcl_AppendToObj(elemPtr, ":", 1);
        Tcl_ListObjAppendElement(NULL, resultPtr, elemPtr);
        
        Tcl_DStringFree(&dstr);
                
        volIndex++;             
    }

    Tcl_IncrRefCount(resultPtr);
    return resultPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * TclpObjNormalizePath --
 *
 *	This function scans through a path specification and replaces
 *	it, in place, with a normalized version.  On MacOS, this means
 *	resolving all aliases present in the path and replacing the head of
 *	pathPtr with the absolute case-sensitive path to the last file or
 *	directory that could be validated in the path.
 *
 * Results:
 *	The new 'nextCheckpoint' value, giving as far as we could
 *	understand in the path.
 *
 * Side effects:
 *	The pathPtr string, which must contain a valid path, is
 *	possibly modified in place.
 *
 *---------------------------------------------------------------------------
 */

int
TclpObjNormalizePath(interp, pathPtr, nextCheckpoint)
    Tcl_Interp *interp;
    Tcl_Obj *pathPtr;
    int nextCheckpoint;
{
    #define MAXMACFILENAMELEN 31  /* assumed to be < sizeof(StrFileName) */
 
    StrFileName fileName;
    StringPtr fileNamePtr;
    int fileNameLen,newPathLen;
    Handle newPathHandle;
    OSErr err;
    short vRefNum;
    long dirID;
    Boolean isDirectory;
    Boolean wasAlias=FALSE;
    FSSpec fileSpec, lastFileSpec;
    
    Tcl_DString nativeds;

    char cur;
    int firstCheckpoint=nextCheckpoint, lastCheckpoint;
    int origPathLen;
    char *path = Tcl_GetStringFromObj(pathPtr,&origPathLen);
    
    {
	int currDirValid=0;    
	/*
	 * check if substring to first ':' after initial
	 * nextCheckpoint is a valid relative or absolute
	 * path to a directory, if not we return without
	 * normalizing anything
	 */
	
	while (1) {
	    cur = path[nextCheckpoint];
	    if (cur == ':' || cur == 0) {
		if (cur == ':') { 
		    /* jump over separator */
		    nextCheckpoint++; cur = path[nextCheckpoint]; 
		} 
		Tcl_UtfToExternalDString(NULL,path,nextCheckpoint,&nativeds);
		err = FSpLLocationFromPath(Tcl_DStringLength(&nativeds), 
					  Tcl_DStringValue(&nativeds), 
					  &fileSpec);
		Tcl_DStringFree(&nativeds);
		if (err == noErr) {
			lastFileSpec=fileSpec;
			err = ResolveAliasFile(&fileSpec, true, &isDirectory, 
				       &wasAlias);
			if (err == noErr) {
		    err = FSpGetDirectoryID(&fileSpec, &dirID, &isDirectory);
		    currDirValid = ((err == noErr) && isDirectory);
		    vRefNum = fileSpec.vRefNum;
		    }
		}
		break;
	    }
	    nextCheckpoint++;
	}
	
	if(!currDirValid) {
	    /* can't determine root dir, bail out */
	    return firstCheckpoint; 
	}
    }
	
    /*
     * Now vRefNum and dirID point to a valid
     * directory, so walk the rest of the path
     * ( code adapted from FSpLocationFromPath() )
     */

    lastCheckpoint=nextCheckpoint;
    while (1) {
	cur = path[nextCheckpoint];
	if (cur == ':' || cur == 0) {
	    fileNameLen=nextCheckpoint-lastCheckpoint;
	    fileNamePtr=fileName;
	    if(fileNameLen==0) {
		if (cur == ':') {
		    /*
		     * special case for empty dirname i.e. encountered
		     * a '::' path component: get parent dir of currDir
		     */
		    fileName[0]=2;
		    strcpy((char *) fileName + 1, "::");
		    lastCheckpoint--;
		} else {
		    /*
		     * empty filename, i.e. want FSSpec for currDir
		     */
		    fileNamePtr=NULL;
		}
	    } else {
		Tcl_UtfToExternalDString(NULL,&path[lastCheckpoint],
					 fileNameLen,&nativeds);
		fileNameLen=Tcl_DStringLength(&nativeds);
		if(fileNameLen > MAXMACFILENAMELEN) { 
		    err = bdNamErr;
		} else {
		fileName[0]=fileNameLen;
		strncpy((char *) fileName + 1, Tcl_DStringValue(&nativeds), 
			fileNameLen);
		}
		Tcl_DStringFree(&nativeds);
	    }
	    if(err == noErr)
	    err=FSMakeFSSpecCompat(vRefNum, dirID, fileNamePtr, &fileSpec);
	    if(err != noErr) {
		if(err != fnfErr) {
		    /*
		     * this can occur if trying to get parent of a root
		     * volume via '::' or when using an illegal
		     * filename; revert to last checkpoint and stop
		     * processing path further
		     */
		    err=FSMakeFSSpecCompat(vRefNum, dirID, NULL, &fileSpec);
		    if(err != noErr) {
			/* should never happen, bail out */
			return firstCheckpoint; 
		    }
		    nextCheckpoint=lastCheckpoint;
		    cur = path[lastCheckpoint];
		}
    		break; /* arrived at nonexistent file or dir */
	    } else {
		/* fileSpec could point to an alias, resolve it */
		lastFileSpec=fileSpec;
		err = ResolveAliasFile(&fileSpec, true, &isDirectory, 
				       &wasAlias);
		if (err != noErr || !isDirectory) {
		    break; /* fileSpec doesn't point to a dir */
		}
	    }
	    if (cur == 0) break; /* arrived at end of path */
	    
	    /* fileSpec points to possibly nonexisting subdirectory; validate */
	    err = FSpGetDirectoryID(&fileSpec, &dirID, &isDirectory);
	    if (err != noErr || !isDirectory) {
	        break; /* fileSpec doesn't point to existing dir */
	    }
	    vRefNum = fileSpec.vRefNum;
    	
	    /* found a new valid subdir in path, continue processing path */
	    lastCheckpoint=nextCheckpoint+1;
	}
	wasAlias=FALSE;
	nextCheckpoint++;
    }
    
    if (wasAlias)
    	fileSpec=lastFileSpec;
    
    /*
     * fileSpec now points to a possibly nonexisting file or dir
     *  inside a valid dir; get full path name to it
     */
    
    err=FSpPathFromLocation(&fileSpec, &newPathLen, &newPathHandle);
    if(err != noErr) {
	return firstCheckpoint; /* should not see any errors here, bail out */
    }
    
    HLock(newPathHandle);
    Tcl_ExternalToUtfDString(NULL,*newPathHandle,newPathLen,&nativeds);
    if (cur != 0) {
	/* not at end, append remaining path */
    	if ( newPathLen==0 || (*(*newPathHandle+(newPathLen-1))!=':' && path[nextCheckpoint] !=':')) {
	    Tcl_DStringAppend(&nativeds, ":" , 1);
	}
	Tcl_DStringAppend(&nativeds, &path[nextCheckpoint], 
			  strlen(&path[nextCheckpoint]));
    }
    DisposeHandle(newPathHandle);
    
    fileNameLen=Tcl_DStringLength(&nativeds);
    Tcl_SetStringObj(pathPtr,Tcl_DStringValue(&nativeds),fileNameLen);
    Tcl_DStringFree(&nativeds);
    
    return nextCheckpoint+(fileNameLen-origPathLen);
}

