/*
 * tclMacResource.c --
 *
 *	This file contains several commands that manipulate or use
 *	Macintosh resources.  Included are extensions to the "source"
 *	command, the mac specific "beep" and "resource" commands, and
 *	administration for open resource file references.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <Errors.h>
#include <FSpCompat.h>
#include <Processes.h>
#include <Resources.h>
#include <Sound.h>
#include <Strings.h>
#include <Traps.h>
#include <LowMem.h>

#include "FullPath.h"
#include "tcl.h"
#include "tclInt.h"
#include "tclMac.h"
#include "tclMacInt.h"
#include "tclMacPort.h"

/*
 * This flag tells the RegisterResource function to insert the
 * resource into the tail of the resource fork list.  Needed only
 * Resource_Init.
 */
 
#define TCL_RESOURCE_INSERT_TAIL 1
/*
 * 2 is taken by TCL_RESOURCE_DONT_CLOSE
 * which is the only public flag to TclMacRegisterResourceFork.
 */
 
#define TCL_RESOURCE_CHECK_IF_OPEN 4

/*
 * Pass this in the mode parameter of SetSoundVolume to determine
 * which volume to set.
 */

enum WhichVolume {
    SYS_BEEP_VOLUME,    /* This sets the volume for SysBeep calls */ 
    DEFAULT_SND_VOLUME, /* This one for SndPlay calls */
    RESET_VOLUME        /* And this undoes the last call to SetSoundVolume */
};
 
/*
 * Hash table to track open resource files.
 */

typedef struct OpenResourceFork {
    short fileRef;
    int   flags;
} OpenResourceFork;



static Tcl_HashTable nameTable;		/* Id to process number mapping. */
static Tcl_HashTable resourceTable;	/* Process number to id mapping. */
static Tcl_Obj *resourceForkList;       /* Ordered list of resource forks */
static int appResourceIndex;            /* This is the index of the application*
					 * in the list of resource forks */
static int newId = 0;			/* Id source. */
static int initialized = 0;		/* 0 means static structures haven't 
					 * been initialized yet. */
static int osTypeInit = 0;		/* 0 means Tcl object of osType hasn't 
					 * been initialized yet. */
/*
 * Prototypes for procedures defined later in this file:
 */

static void		DupOSTypeInternalRep _ANSI_ARGS_((Tcl_Obj *srcPtr,
			    Tcl_Obj *copyPtr));
static void		ResourceInit _ANSI_ARGS_((void));
static void             BuildResourceForkList _ANSI_ARGS_((void));
static int		SetOSTypeFromAny _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *objPtr));
static void		UpdateStringOfOSType _ANSI_ARGS_((Tcl_Obj *objPtr));
static OpenResourceFork* GetRsrcRefFromObj _ANSI_ARGS_((Tcl_Obj *objPtr,
		                int okayOnReadOnly, const char *operation,
	                        Tcl_Obj *resultPtr));

static void 		SetSoundVolume(int volume, enum WhichVolume mode);

/*
 * The structures below defines the Tcl object type defined in this file by
 * means of procedures that can be invoked by generic object code.
 */

static Tcl_ObjType osType = {
    "ostype",				/* name */
    (Tcl_FreeInternalRepProc *) NULL,   /* freeIntRepProc */
    DupOSTypeInternalRep,	        /* dupIntRepProc */
    UpdateStringOfOSType,		/* updateStringProc */
    SetOSTypeFromAny			/* setFromAnyProc */
};

/*
 *----------------------------------------------------------------------
 *
 * Tcl_ResourceObjCmd --
 *
 *	This procedure is invoked to process the "resource" Tcl command.
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
Tcl_ResourceObjCmd(
    ClientData clientData,		/* Not used. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *CONST objv[])		/* Argument values. */
{
    Tcl_Obj *resultPtr, *objPtr;
    int index, result;
    long fileRef, rsrcId;
    FSSpec fileSpec;
    char *stringPtr;
    char errbuf[16];
    OpenResourceFork *resourceRef;
    Handle resource = NULL;
    OSErr err;
    int count, i, limitSearch = false, length;
    short id, saveRef, resInfo;
    Str255 theName;
    OSType rezType;
    int gotInt, releaseIt = 0, force;
    char *resourceId = NULL;	
    long size;
    char macPermision;
    int mode;

    static char *switches[] = {"close", "delete" ,"files", "list", 
            "open", "read", "types", "write", (char *) NULL
    };
	        
    enum {
            RESOURCE_CLOSE, RESOURCE_DELETE, RESOURCE_FILES, RESOURCE_LIST, 
            RESOURCE_OPEN, RESOURCE_READ, RESOURCE_TYPES, RESOURCE_WRITE
    };
              
    static char *writeSwitches[] = {
            "-id", "-name", "-file", "-force", (char *) NULL
    };
            
    enum {
            RESOURCE_WRITE_ID, RESOURCE_WRITE_NAME, 
            RESOURCE_WRITE_FILE, RESOURCE_FORCE
    };
            
    static char *deleteSwitches[] = {"-id", "-name", "-file", (char *) NULL};
             
    enum {RESOURCE_DELETE_ID, RESOURCE_DELETE_NAME, RESOURCE_DELETE_FILE};

    resultPtr = Tcl_GetObjResult(interp);
    
    if (objc < 2) {
	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], switches, "option", 0, &index)
	    != TCL_OK) {
	return TCL_ERROR;
    }
    if (!initialized) {
	ResourceInit();
    }
    result = TCL_OK;

    switch (index) {
	case RESOURCE_CLOSE:			
	    if (objc != 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "resourceRef");
		return TCL_ERROR;
	    }
	    stringPtr = Tcl_GetStringFromObj(objv[2], &length);
	    fileRef = TclMacUnRegisterResourceFork(stringPtr, resultPtr);
	    
	    if (fileRef >= 0) {
	        CloseResFile((short) fileRef);
	        return TCL_OK;
	    } else {
	        return TCL_ERROR;
	    }
	case RESOURCE_DELETE:
	    if (!((objc >= 3) && (objc <= 9) && ((objc % 2) == 1))) {
		Tcl_WrongNumArgs(interp, 2, objv, 
		    "?-id resourceId? ?-name resourceName? ?-file \
resourceRef? resourceType");
		return TCL_ERROR;
	    }
	    
	    i = 2;
	    fileRef = -1;
	    gotInt = false;
	    resourceId = NULL;
	    limitSearch = false;

	    while (i < (objc - 2)) {
		if (Tcl_GetIndexFromObj(interp, objv[i], deleteSwitches,
			"option", 0, &index) != TCL_OK) {
		    return TCL_ERROR;
		}

		switch (index) {
		    case RESOURCE_DELETE_ID:		
			if (Tcl_GetLongFromObj(interp, objv[i+1], &rsrcId)
				!= TCL_OK) {
			    return TCL_ERROR;
			}
			gotInt = true;
			break;
		    case RESOURCE_DELETE_NAME:		
			resourceId = Tcl_GetStringFromObj(objv[i+1], &length);
			if (length > 255) {
			    Tcl_AppendStringsToObj(resultPtr,"-name argument ",
			            "too long, must be < 255 characters",
			            (char *) NULL);
			    return TCL_ERROR;
			}
			strcpy((char *) theName, resourceId);
			resourceId = (char *) theName;
			c2pstr(resourceId);
			break;
		    case RESOURCE_DELETE_FILE:
		        resourceRef = GetRsrcRefFromObj(objv[i+1], 0, 
		                "delete from", resultPtr);
		        if (resourceRef == NULL) {
		            return TCL_ERROR;
		        }	
			limitSearch = true;
			break;
		}
		i += 2;
	    }
	    
	    if ((resourceId == NULL) && !gotInt) {
		Tcl_AppendStringsToObj(resultPtr,"you must specify either ",
		        "\"-id\" or \"-name\" or both ",
		        "to \"resource delete\"",
		        (char *) NULL);
	        return TCL_ERROR;
            }

	    if (Tcl_GetOSTypeFromObj(interp, objv[i], &rezType) != TCL_OK) {
		return TCL_ERROR;
	    }

	    if (limitSearch) {
		saveRef = CurResFile();
		UseResFile((short) resourceRef->fileRef);
	    }
	    
	    SetResLoad(false);
	    
	    if (gotInt == true) {
	        if (limitSearch) {
		    resource = Get1Resource(rezType, rsrcId);
		} else {
		    resource = GetResource(rezType, rsrcId);
		}
                err = ResError();
            
                if (err == resNotFound || resource == NULL) {
	            Tcl_AppendStringsToObj(resultPtr, "resource not found",
	                (char *) NULL);
	            result = TCL_ERROR;
	            goto deleteDone;               
                } else if (err != noErr) {
                    char buffer[16];
                
                    sprintf(buffer, "%12d", err);
	            Tcl_AppendStringsToObj(resultPtr, "resource error #",
	                    buffer, "occured while trying to find resource",
	                    (char *) NULL);
	            result = TCL_ERROR;
	            goto deleteDone;               
	        }
	    } 
	    
	    if (resourceId != NULL) {
	        Handle tmpResource;
	        if (limitSearch) {
	            tmpResource = Get1NamedResource(rezType,
			    (StringPtr) resourceId);
	        } else {
	            tmpResource = GetNamedResource(rezType,
			    (StringPtr) resourceId);
	        }
                err = ResError();
            
                if (err == resNotFound || tmpResource == NULL) {
	            Tcl_AppendStringsToObj(resultPtr, "resource not found",
	                (char *) NULL);
	            result = TCL_ERROR;
	            goto deleteDone;               
                } else if (err != noErr) {
                    char buffer[16];
                
                    sprintf(buffer, "%12d", err);
	            Tcl_AppendStringsToObj(resultPtr, "resource error #",
	                    buffer, "occured while trying to find resource",
	                    (char *) NULL);
	            result = TCL_ERROR;
	            goto deleteDone;               
	        }
	        
	        if (gotInt) { 
	            if (resource != tmpResource) {
	                Tcl_AppendStringsToObj(resultPtr,
				"\"-id\" and \"-name\" ",
	                        "values do not point to the same resource",
	                        (char *) NULL);
	                result = TCL_ERROR;
	                goto deleteDone;
	            }
	        } else {
	            resource = tmpResource;
	        }
	    }
	        
       	    resInfo = GetResAttrs(resource);
	    
	    if ((resInfo & resProtected) == resProtected) {
	        Tcl_AppendStringsToObj(resultPtr, "resource ",
	                "cannot be deleted: it is protected.",
	                (char *) NULL);
	        result = TCL_ERROR;
	        goto deleteDone;               
	    } else if ((resInfo & resSysHeap) == resSysHeap) {   
	        Tcl_AppendStringsToObj(resultPtr, "resource",
	                "cannot be deleted: it is in the system heap.",
	                (char *) NULL);
	        result = TCL_ERROR;
	        goto deleteDone;               
	    }
	    
	    /*
	     * Find the resource file, if it was not specified,
	     * so we can flush the changes now.  Perhaps this is
	     * a little paranoid, but better safe than sorry.
	     */
	     
	    RemoveResource(resource);
	    
	    if (!limitSearch) {
	        UpdateResFile(HomeResFile(resource));
	    } else {
	        UpdateResFile(resourceRef->fileRef);
	    }
	    
	    
	    deleteDone:
	    
            SetResLoad(true);
	    if (limitSearch) {
                 UseResFile(saveRef);                        
	    }
	    return result;
	    
	case RESOURCE_FILES:
	    if ((objc < 2) || (objc > 3)) {
		Tcl_SetStringObj(resultPtr,
		        "wrong # args: should be \"resource files \
?resourceId?\"", -1);
		return TCL_ERROR;
	    }
	    
	    if (objc == 2) {
	        stringPtr = Tcl_GetStringFromObj(resourceForkList, &length);
	        Tcl_SetStringObj(resultPtr, stringPtr, length);
	    } else {
                FCBPBRec fileRec;
                Handle pathHandle;
                short pathLength;
                Str255 fileName;
                Tcl_DString dstr;
	        
	        if (strcmp(Tcl_GetString(objv[2]), "ROM Map") == 0) {
	            Tcl_SetStringObj(resultPtr,"no file path for ROM Map", -1);
	            return TCL_ERROR;
	        }
	        
	        resourceRef = GetRsrcRefFromObj(objv[2], 1, "files", resultPtr);
	        if (resourceRef == NULL) {
	            return TCL_ERROR;
	        }

                fileRec.ioCompletion = NULL;
                fileRec.ioFCBIndx = 0;
                fileRec.ioNamePtr = fileName;
                fileRec.ioVRefNum = 0;
                fileRec.ioRefNum = resourceRef->fileRef;
                err = PBGetFCBInfo(&fileRec, false);
                if (err != noErr) {
                    Tcl_SetStringObj(resultPtr,
                            "could not get FCB for resource file", -1);
                    return TCL_ERROR;
                }
                
                err = GetFullPath(fileRec.ioFCBVRefNum, fileRec.ioFCBParID,
                        fileRec.ioNamePtr, &pathLength, &pathHandle);
                if ( err != noErr) {
                    Tcl_SetStringObj(resultPtr,
                            "could not get file path from token", -1);
                    return TCL_ERROR;
                }
                
                HLock(pathHandle);
                Tcl_ExternalToUtfDString(NULL, *pathHandle, pathLength, &dstr);
                
                Tcl_SetStringObj(resultPtr, Tcl_DStringValue(&dstr), Tcl_DStringLength(&dstr));
                HUnlock(pathHandle);
                DisposeHandle(pathHandle);
                Tcl_DStringFree(&dstr);
            }                    	    
	    return TCL_OK;
	case RESOURCE_LIST:			
	    if (!((objc == 3) || (objc == 4))) {
		Tcl_WrongNumArgs(interp, 2, objv, "resourceType ?resourceRef?");
		return TCL_ERROR;
	    }
	    if (Tcl_GetOSTypeFromObj(interp, objv[2], &rezType) != TCL_OK) {
		return TCL_ERROR;
	    }

	    if (objc == 4) {
	        resourceRef = GetRsrcRefFromObj(objv[3], 1, 
		                "list", resultPtr);
		if (resourceRef == NULL) {
		    return TCL_ERROR;
		}	

		saveRef = CurResFile();
		UseResFile((short) resourceRef->fileRef);
		limitSearch = true;
	    }

	    Tcl_ResetResult(interp);
	    if (limitSearch) {
		count = Count1Resources(rezType);
	    } else {
		count = CountResources(rezType);
	    }
	    SetResLoad(false);
	    for (i = 1; i <= count; i++) {
		if (limitSearch) {
		    resource = Get1IndResource(rezType, i);
		} else {
		    resource = GetIndResource(rezType, i);
		}
		if (resource != NULL) {
		    GetResInfo(resource, &id, (ResType *) &rezType, theName);
		    if (theName[0] != 0) {
		        
			objPtr = Tcl_NewStringObj((char *) theName + 1,
				theName[0]);
		    } else {
			objPtr = Tcl_NewIntObj(id);
		    }
		    ReleaseResource(resource);
		    result = Tcl_ListObjAppendElement(interp, resultPtr,
			    objPtr);
		    if (result != TCL_OK) {
			Tcl_DecrRefCount(objPtr);
			break;
		    }
		}
	    }
	    SetResLoad(true);
	
	    if (limitSearch) {
		UseResFile(saveRef);
	    }
	
	    return TCL_OK;
	case RESOURCE_OPEN: {
	    Tcl_DString ds, buffer;
	    char *str, *native;
	    int length;
	    			
	    if (!((objc == 3) || (objc == 4))) {
		Tcl_WrongNumArgs(interp, 2, objv, "fileName ?permissions?");
		return TCL_ERROR;
	    }
	    str = Tcl_GetStringFromObj(objv[2], &length);
	    if (Tcl_TranslateFileName(interp, str, &buffer) == NULL) {
	        return TCL_ERROR;
	    }
	    native = Tcl_UtfToExternalDString(NULL, Tcl_DStringValue(&buffer),
	    	    Tcl_DStringLength(&buffer), &ds);
	    err = FSpLocationFromPath(Tcl_DStringLength(&ds), native, &fileSpec);
	    Tcl_DStringFree(&ds);
	    Tcl_DStringFree(&buffer);

	    if (!((err == noErr) || (err == fnfErr))) {
		Tcl_AppendStringsToObj(resultPtr, "invalid path", (char *) NULL);
		return TCL_ERROR;
	    }

	    /*
	     * Get permissions for the file.  We really only understand
	     * read-only and shared-read-write.  If no permissions are 
	     * given we default to read only.
	     */
	    
	    if (objc == 4) {
		stringPtr = Tcl_GetStringFromObj(objv[3], &length);
		mode = TclGetOpenMode(interp, stringPtr, &index);
		if (mode == -1) {
		    /* TODO: TclGetOpenMode doesn't work with Obj commands. */
		    return TCL_ERROR;
		}
		switch (mode & (O_RDONLY | O_WRONLY | O_RDWR)) {
		    case O_RDONLY:
			macPermision = fsRdPerm;
		    break;
		    case O_WRONLY:
		    case O_RDWR:
			macPermision = fsRdWrPerm;
			break;
		    default:
			panic("Tcl_ResourceObjCmd: invalid mode value");
		    break;
		}
	    } else {
		macPermision = fsRdPerm;
	    }
	    
	    /*
	     * Don't load in any of the resources in the file, this could 
	     * cause problems if you open a file that has CODE resources...
	     */
	     
	    SetResLoad(false); 
	    fileRef = (long) FSpOpenResFileCompat(&fileSpec, macPermision);
	    SetResLoad(true);
	    
	    if (fileRef == -1) {
	    	err = ResError();
		if (((err == fnfErr) || (err == eofErr)) &&
			(macPermision == fsRdWrPerm)) {
		    /*
		     * No resource fork existed for this file.  Since we are
		     * opening it for writing we will create the resource fork
		     * now.
		     */
		     
		    HCreateResFile(fileSpec.vRefNum, fileSpec.parID,
			    fileSpec.name);
		    fileRef = (long) FSpOpenResFileCompat(&fileSpec,
			    macPermision);
		    if (fileRef == -1) {
			goto openError;
		    }
		} else if (err == fnfErr) {
		    Tcl_AppendStringsToObj(resultPtr,
			"file does not exist", (char *) NULL);
		    return TCL_ERROR;
		} else if (err == eofErr) {
		    Tcl_AppendStringsToObj(resultPtr,
			"file does not contain resource fork", (char *) NULL);
		    return TCL_ERROR;
		} else {
		    openError:
		    Tcl_AppendStringsToObj(resultPtr,
			"error opening resource file", (char *) NULL);
		    return TCL_ERROR;
		}
	    }
	    	    
            /*
             * The FspOpenResFile function does not set the ResFileAttrs.
             * Even if you open the file read only, the mapReadOnly
             * attribute is not set.  This means we can't detect writes to a 
             * read only resource fork until the write fails, which is bogus.  
             * So set it here...
             */
            
            if (macPermision == fsRdPerm) {
                SetResFileAttrs(fileRef, mapReadOnly);
            }
            
            Tcl_SetStringObj(resultPtr, "", 0);
            if (TclMacRegisterResourceFork(fileRef, resultPtr, 
                    TCL_RESOURCE_CHECK_IF_OPEN) != TCL_OK) {
                CloseResFile(fileRef);
		return TCL_ERROR;
            }
	    return TCL_OK;
	}
	case RESOURCE_READ:			
	    if (!((objc == 4) || (objc == 5))) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"resourceType resourceId ?resourceRef?");
		return TCL_ERROR;
	    }

	    if (Tcl_GetOSTypeFromObj(interp, objv[2], &rezType) != TCL_OK) {
		return TCL_ERROR;
	    }
	    
	    if (Tcl_GetLongFromObj((Tcl_Interp *) NULL, objv[3], &rsrcId)
		    != TCL_OK) {
		resourceId = Tcl_GetStringFromObj(objv[3], &length);
            }

	    if (objc == 5) {
		stringPtr = Tcl_GetStringFromObj(objv[4], &length);
	    } else {
		stringPtr = NULL;
	    }
	
	    resource = Tcl_MacFindResource(interp, rezType, resourceId,
		rsrcId, stringPtr, &releaseIt);
			    
	    if (resource != NULL) {
		size = GetResourceSizeOnDisk(resource);
		Tcl_SetByteArrayObj(resultPtr, (unsigned char *) *resource, size);

		/*
		 * Don't release the resource unless WE loaded it...
		 */
		 
		if (releaseIt) {
		    ReleaseResource(resource);
		}
		return TCL_OK;
	    } else {
		Tcl_AppendStringsToObj(resultPtr, "could not load resource",
		    (char *) NULL);
		return TCL_ERROR;
	    }
	case RESOURCE_TYPES:			
	    if (!((objc == 2) || (objc == 3))) {
		Tcl_WrongNumArgs(interp, 2, objv, "?resourceRef?");
		return TCL_ERROR;
	    }

	    if (objc == 3) {
	        resourceRef = GetRsrcRefFromObj(objv[2], 1, 
		                "get types of", resultPtr);
		if (resourceRef == NULL) {
		    return TCL_ERROR;
		}
			
		saveRef = CurResFile();
		UseResFile((short) resourceRef->fileRef);
		limitSearch = true;
	    }

	    if (limitSearch) {
		count = Count1Types();
	    } else {
		count = CountTypes();
	    }
	    for (i = 1; i <= count; i++) {
		if (limitSearch) {
		    Get1IndType((ResType *) &rezType, i);
		} else {
		    GetIndType((ResType *) &rezType, i);
		}
		objPtr = Tcl_NewOSTypeObj(rezType);
		result = Tcl_ListObjAppendElement(interp, resultPtr, objPtr);
		if (result != TCL_OK) {
		    Tcl_DecrRefCount(objPtr);
		    break;
		}
	    }
		
	    if (limitSearch) {
		UseResFile(saveRef);
	    }
		
	    return result;
	case RESOURCE_WRITE:			
	    if ((objc < 4) || (objc > 11)) {
		Tcl_WrongNumArgs(interp, 2, objv, 
		"?-id resourceId? ?-name resourceName? ?-file resourceRef?\
 ?-force? resourceType data");
		return TCL_ERROR;
	    }
	    
	    i = 2;
	    gotInt = false;
	    resourceId = NULL;
	    limitSearch = false;
	    force = 0;

	    while (i < (objc - 2)) {
		if (Tcl_GetIndexFromObj(interp, objv[i], writeSwitches,
			"switch", 0, &index) != TCL_OK) {
		    return TCL_ERROR;
		}

		switch (index) {
		    case RESOURCE_WRITE_ID:		
			if (Tcl_GetLongFromObj(interp, objv[i+1], &rsrcId)
				!= TCL_OK) {
			    return TCL_ERROR;
			}
			gotInt = true;
		        i += 2;
			break;
		    case RESOURCE_WRITE_NAME:		
			resourceId = Tcl_GetStringFromObj(objv[i+1], &length);
			strcpy((char *) theName, resourceId);
			resourceId = (char *) theName;
			c2pstr(resourceId);
		        i += 2;
			break;
		    case RESOURCE_WRITE_FILE:		
	                resourceRef = GetRsrcRefFromObj(objv[i+1], 0, 
		                        "write to", resultPtr);
                        if (resourceRef == NULL) {
                            return TCL_ERROR;
		        }	
			limitSearch = true;
		        i += 2;
			break;
		    case RESOURCE_FORCE:
		        force = 1;
		        i += 1;
		        break;
		}
	    }
	    if (Tcl_GetOSTypeFromObj(interp, objv[i], &rezType) != TCL_OK) {
		return TCL_ERROR;
	    }
	    stringPtr = (char *) Tcl_GetByteArrayFromObj(objv[i+1], &length);

	    if (gotInt == false) {
		rsrcId = UniqueID(rezType);
	    }
	    if (resourceId == NULL) {
		resourceId = (char *) "\p";
	    }
	    if (limitSearch) {
		saveRef = CurResFile();
		UseResFile((short) resourceRef->fileRef);
	    }
	    
	    /*
	     * If we are adding the resource by number, then we must make sure
	     * there is not already a resource of that number.  We are not going
	     * load it here, since we want to detect whether we loaded it or
	     * not.  Remember that releasing some resources in particular menu
	     * related ones, can be fatal.
	     */
	     
	    if (gotInt == true) {
	        SetResLoad(false);
	        resource = Get1Resource(rezType,rsrcId);
	        SetResLoad(true);
	    }     
	    	    
	    if (resource == NULL) {
	        /*
	         * We get into this branch either if there was not already a
	         * resource of this type & id, or the id was not specified.
	         */
	         
	        resource = NewHandle(length);
	        if (resource == NULL) {
	            resource = NewHandleSys(length);
	            if (resource == NULL) {
	                panic("could not allocate memory to write resource");
	            }
	        }
	        HLock(resource);
	        memcpy(*resource, stringPtr, length);
	        HUnlock(resource);
	        AddResource(resource, rezType, (short) rsrcId,
		    (StringPtr) resourceId);
		releaseIt = 1;
            } else {
                /* 
                 * We got here because there was a resource of this type 
                 * & ID in the file. 
                 */ 
                
                if (*resource == NULL) {
                    releaseIt = 1;
                } else {
                    releaseIt = 0;
                }
               
                if (!force) {
                    /*
                     *We only overwrite extant resources
                     * when the -force flag has been set.
                     */
                     
                    sprintf(errbuf,"%d", rsrcId);
                  
                    Tcl_AppendStringsToObj(resultPtr, "the resource ",
                          errbuf, " already exists, use \"-force\"",
                          " to overwrite it.", (char *) NULL);
                    
                    result = TCL_ERROR;
                    goto writeDone;
                } else if (GetResAttrs(resource) & resProtected) {
                    /*  
                     *  
                     * Next, check to see if it is protected...
                     */
                 
                    sprintf(errbuf,"%d", rsrcId);
                    Tcl_AppendStringsToObj(resultPtr,
			    "could not write resource id ",
                            errbuf, " of type ",
                            Tcl_GetStringFromObj(objv[i],&length),
                            ", it was protected.",(char *) NULL);
                    result = TCL_ERROR;
                    goto writeDone;
                } else {
                    /*
                     * Be careful, the resource might already be in memory
                     * if something else loaded it.
                     */
                     
                    if (*resource == 0) {
                    	LoadResource(resource);
                    	err = ResError();
                    	if (err != noErr) {
                            sprintf(errbuf,"%d", rsrcId);
                            Tcl_AppendStringsToObj(resultPtr,
				    "error loading resource ",
                                    errbuf, " of type ",
                                    Tcl_GetStringFromObj(objv[i],&length),
                                    " to overwrite it", (char *) NULL);
                            goto writeDone;
                    	}
                    }
                     
                    SetHandleSize(resource, length);
                    if ( MemError() != noErr ) {
                        panic("could not allocate memory to write resource");
                    }

                    HLock(resource);
	            memcpy(*resource, stringPtr, length);
	            HUnlock(resource);
	           
                    ChangedResource(resource);
                
                    /*
                     * We also may have changed the name...
                     */ 
                 
                    SetResInfo(resource, rsrcId, (StringPtr) resourceId);
                }
            }
            
	    err = ResError();
	    if (err != noErr) {
		Tcl_AppendStringsToObj(resultPtr,
			"error adding resource to resource map",
		        (char *) NULL);
		result = TCL_ERROR;
		goto writeDone;
	    }
	    
	    WriteResource(resource);
	    err = ResError();
	    if (err != noErr) {
		Tcl_AppendStringsToObj(resultPtr,
			"error writing resource to disk",
		        (char *) NULL);
		result = TCL_ERROR;
	    }
	    
	    writeDone:
	    
	    if (releaseIt) {
	        ReleaseResource(resource);
	        err = ResError();
	        if (err != noErr) {
		    Tcl_AppendStringsToObj(resultPtr,
			    "error releasing resource",
		            (char *) NULL);
		    result = TCL_ERROR;
	        }
	    }
	    
	    if (limitSearch) {
		UseResFile(saveRef);
	    }

	    return result;
	default:
	    panic("Tcl_GetIndexFromObj returned unrecognized option");
	    return TCL_ERROR;	/* Should never be reached. */
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_MacSourceObjCmd --
 *
 *	This procedure is invoked to process the "source" Tcl command.
 *	See the user documentation for details on what it does.  In 
 *	addition, it supports sourceing from the resource fork of
 *	type 'TEXT'.
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
Tcl_MacSourceObjCmd(
    ClientData dummy,			/* Not used. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *CONST objv[])		/* Argument objects. */
{
    char *errNum = "wrong # args: ";
    char *errBad = "bad argument: ";
    char *errStr;
    char *fileName = NULL, *rsrcName = NULL;
    long rsrcID = -1;
    char *string;
    int length;

    if (objc < 2 || objc > 4)  {
    	errStr = errNum;
    	goto sourceFmtErr;
    }
    
    if (objc == 2)  {
	string = Tcl_GetStringFromObj(objv[1], &length);
	return Tcl_EvalFile(interp, string);
    }
    
    /*
     * The following code supports a few older forms of this command
     * for backward compatability.
     */
    string = Tcl_GetStringFromObj(objv[1], &length);
    if (!strcmp(string, "-rsrc") || !strcmp(string, "-rsrcname")) {
	rsrcName = Tcl_GetStringFromObj(objv[2], &length);
    } else if (!strcmp(string, "-rsrcid")) {
	if (Tcl_GetLongFromObj(interp, objv[2], &rsrcID) != TCL_OK) {
	    return TCL_ERROR;
	}
    } else {
    	errStr = errBad;
    	goto sourceFmtErr;
    }
    
    if (objc == 4) {
	fileName = Tcl_GetStringFromObj(objv[3], &length);
    }
    return Tcl_MacEvalResource(interp, rsrcName, rsrcID, fileName);
	
    sourceFmtErr:
    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), errStr, "should be \"",
		Tcl_GetString(objv[0]), " fileName\" or \"",
		Tcl_GetString(objv[0]),	" -rsrc name ?fileName?\" or \"", 
		Tcl_GetString(objv[0]), " -rsrcid id ?fileName?\"",
		(char *) NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_BeepObjCmd --
 *
 *	This procedure makes the beep sound.
 *
 * Results:
 *	A standard Tcl result.
 *
 * Side effects:
 *	Makes a beep.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_BeepObjCmd(
    ClientData dummy,			/* Not used. */
    Tcl_Interp *interp,			/* Current interpreter. */
    int objc,				/* Number of arguments. */
    Tcl_Obj *CONST objv[])		/* Argument values. */
{
    Tcl_Obj *resultPtr, *objPtr;
    Handle sound;
    Str255 sndName;
    int volume = -1, length;
    char * sndArg = NULL;

    resultPtr = Tcl_GetObjResult(interp);
    if (objc == 1) {
	SysBeep(1);
	return TCL_OK;
    } else if (objc == 2) {
	if (!strcmp(Tcl_GetStringFromObj(objv[1], &length), "-list")) {
	    int count, i;
	    short id;
	    Str255 theName;
	    ResType rezType;
			
	    count = CountResources('snd ');
	    for (i = 1; i <= count; i++) {
		sound = GetIndResource('snd ', i);
		if (sound != NULL) {
		    GetResInfo(sound, &id, &rezType, theName);
		    if (theName[0] == 0) {
			continue;
		    }
		    objPtr = Tcl_NewStringObj((char *) theName + 1,
			    theName[0]);
		    Tcl_ListObjAppendElement(interp, resultPtr, objPtr);
		}
	    }
	    return TCL_OK;
	} else {
	    sndArg = Tcl_GetStringFromObj(objv[1], &length);
	}
    } else if (objc == 3) {
	if (!strcmp(Tcl_GetStringFromObj(objv[1], &length), "-volume")) {
	    Tcl_GetIntFromObj(interp, objv[2], &volume);
	} else {
	    goto beepUsage;
	}
    } else if (objc == 4) {
	if (!strcmp(Tcl_GetStringFromObj(objv[1], &length), "-volume")) {
	    Tcl_GetIntFromObj(interp, objv[2], &volume);
	    sndArg = Tcl_GetStringFromObj(objv[3], &length);
	} else {
	    goto beepUsage;
	}
    } else {
	goto beepUsage;
    }
	
    /*
     * Play the sound
     */
    if (sndArg == NULL) {
	/*
         * Set Volume for SysBeep
         */

	if (volume >= 0) {
	    SetSoundVolume(volume, SYS_BEEP_VOLUME);
	}
	SysBeep(1);

	/*
         * Reset Volume
         */

	if (volume >= 0) {
	    SetSoundVolume(0, RESET_VOLUME);
	}
    } else {
	strcpy((char *) sndName + 1, sndArg);
	sndName[0] = length;
	sound = GetNamedResource('snd ', sndName);
	if (sound != NULL) {
	    /*
             * Set Volume for Default Output device
             */

	    if (volume >= 0) {
		SetSoundVolume(volume, DEFAULT_SND_VOLUME);
	    }

	    SndPlay(NULL, (SndListHandle) sound, false);

	    /*
             * Reset Volume
             */

	    if (volume >= 0) {
		SetSoundVolume(0, RESET_VOLUME);
	    }
	} else {
	    Tcl_AppendStringsToObj(resultPtr, " \"", sndArg, 
		    "\" is not a valid sound.  (Try ",
		    Tcl_GetString(objv[0]), " -list)", NULL);
	    return TCL_ERROR;
	}
    }

    return TCL_OK;

    beepUsage:
    Tcl_WrongNumArgs(interp, 1, objv, "[-volume num] [-list | sndName]?");
    return TCL_ERROR;
}

/*
 *-----------------------------------------------------------------------------
 *
 * SetSoundVolume --
 *
 *	Set the volume for either the SysBeep or the SndPlay call depending
 *	on the value of mode (SYS_BEEP_VOLUME or DEFAULT_SND_VOLUME
 *      respectively.
 *
 *      It also stores the last channel set, and the old value of its 
 *	VOLUME.  If you call SetSoundVolume with a mode of RESET_VOLUME, 
 *	it will undo the last setting.  The volume parameter is
 *      ignored in this case.
 *
 * Side Effects:
 *	Sets the System Volume
 *
 * Results:
 *      None
 *
 *-----------------------------------------------------------------------------
 */

void
SetSoundVolume(
    int volume,              /* This is the new volume */
    enum WhichVolume mode)   /* This flag says which volume to
			      * set: SysBeep, SndPlay, or instructs us
			      * to reset the volume */
{
    static int hasSM3 = -1;
    static enum WhichVolume oldMode;
    static long oldVolume = -1;

    /*
     * The volume setting calls only work if we have SoundManager
     * 3.0 or higher.  So we check that here.
     */
    
    if (hasSM3 == -1) {
    	if (GetToolboxTrapAddress(_SoundDispatch) 
		!= GetToolboxTrapAddress(_Unimplemented)) {
	    NumVersion SMVers = SndSoundManagerVersion();
	    if (SMVers.majorRev > 2) {
	    	hasSM3 = 1;
	    } else {
		hasSM3 = 0;
	    }
	} else {
	    /*
	     * If the SoundDispatch trap is not present, then
	     * we don't have the SoundManager at all.
	     */
	    
	    hasSM3 = 0;
	}
    }
    
    /*
     * If we don't have Sound Manager 3.0, we can't set the sound volume.
     * We will just ignore the request rather than raising an error.
     */
    
    if (!hasSM3) {
    	return;
    }
    
    switch (mode) {
    	case SYS_BEEP_VOLUME:
	    GetSysBeepVolume(&oldVolume);
	    SetSysBeepVolume(volume);
	    oldMode = SYS_BEEP_VOLUME;
	    break;
	case DEFAULT_SND_VOLUME:
	    GetDefaultOutputVolume(&oldVolume);
	    SetDefaultOutputVolume(volume);
	    oldMode = DEFAULT_SND_VOLUME;
	    break;
	case RESET_VOLUME:
	    /*
	     * If oldVolume is -1 someone has made a programming error
	     * and called reset before setting the volume.  This is benign
	     * however, so we will just exit.
	     */
	  
	    if (oldVolume != -1) {	
	        if (oldMode == SYS_BEEP_VOLUME) {
	    	    SetSysBeepVolume(oldVolume);
	        } else if (oldMode == DEFAULT_SND_VOLUME) {
		    SetDefaultOutputVolume(oldVolume);
	        }
	    }
	    oldVolume = -1;
    }
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_MacEvalResource --
 *
 *	Used to extend the source command.  Sources Tcl code from a Text
 *	resource.  Currently only sources the resouce by name file ID may be
 *	supported at a later date.
 *
 * Side Effects:
 *	Depends on the Tcl code in the resource.
 *
 * Results:
 *      Returns a Tcl result.
 *
 *-----------------------------------------------------------------------------
 */

int
Tcl_MacEvalResource(
    Tcl_Interp *interp,		/* Interpreter in which to process file. */
    char *resourceName,		/* Name of TEXT resource to source,
				   NULL if number should be used. */
    int resourceNumber,		/* Resource id of source. */
    char *fileName)		/* Name of file to process.
				   NULL if application resource. */
{
    Handle sourceText;
    Str255 rezName;
    char msg[200];
    int result, iOpenedResFile = false;
    short saveRef, fileRef = -1;
    char idStr[64];
    FSSpec fileSpec;
    Tcl_DString buffer;
    char *nativeName;

    saveRef = CurResFile();
	
    if (fileName != NULL) {
	OSErr err;
	
	nativeName = Tcl_TranslateFileName(interp, fileName, &buffer);
	if (nativeName == NULL) {
	    return TCL_ERROR;
	}
	err = FSpLocationFromPath(strlen(nativeName), nativeName,
                &fileSpec);
	Tcl_DStringFree(&buffer);
	if (err != noErr) {
	    Tcl_AppendResult(interp, "Error finding the file: \"", 
		fileName, "\".", NULL);
	    return TCL_ERROR;
	}
		
	fileRef = FSpOpenResFileCompat(&fileSpec, fsRdPerm);
	if (fileRef == -1) {
	    Tcl_AppendResult(interp, "Error reading the file: \"", 
		fileName, "\".", NULL);
	    return TCL_ERROR;
	}
		
	UseResFile(fileRef);
	iOpenedResFile = true;
    } else {
	/*
	 * The default behavior will search through all open resource files.
	 * This may not be the behavior you desire.  If you want the behavior
	 * of this call to *only* search the application resource fork, you
	 * must call UseResFile at this point to set it to the application
	 * file.  This means you must have already obtained the application's 
	 * fileRef when the application started up.
	 */
    }
	
    /*
     * Load the resource by name or ID
     */
    if (resourceName != NULL) {
	strcpy((char *) rezName + 1, resourceName);
	rezName[0] = strlen(resourceName);
	sourceText = GetNamedResource('TEXT', rezName);
    } else {
	sourceText = GetResource('TEXT', (short) resourceNumber);
    }
	
    if (sourceText == NULL) {
	result = TCL_ERROR;
    } else {
	char *sourceStr = NULL;

	HLock(sourceText);
	sourceStr = Tcl_MacConvertTextResource(sourceText);
	HUnlock(sourceText);
	ReleaseResource(sourceText);
		
	/*
	 * We now evaluate the Tcl source
	 */
	result = Tcl_Eval(interp, sourceStr);
	ckfree(sourceStr);
	if (result == TCL_RETURN) {
	    result = TCL_OK;
	} else if (result == TCL_ERROR) {
	    sprintf(msg, "\n    (rsrc \"%.150s\" line %d)",
                    resourceName,
		    interp->errorLine);
	    Tcl_AddErrorInfo(interp, msg);
	}
				
	goto rezEvalCleanUp;
    }
	
    rezEvalError:
    sprintf(idStr, "ID=%d", resourceNumber);
    Tcl_AppendResult(interp, "The resource \"",
	    (resourceName != NULL ? resourceName : idStr),
	    "\" could not be loaded from ",
	    (fileName != NULL ? fileName : "application"),
	    ".", NULL);

    rezEvalCleanUp:

    /* 
     * TRICKY POINT: The code that you are sourcing here could load a
     * shared library.  This will go AHEAD of the resource we stored away
     * in saveRef on the resource path.  
     * If you restore the saveRef in this case, you will never be able
     * to get to the resources in the shared library, since you are now
     * pointing too far down on the resource list.  
     * So, we only reset the current resource file if WE opened a resource
     * explicitly, and then only if the CurResFile is still the 
     * one we opened... 
     */
     
    if (iOpenedResFile && (CurResFile() == fileRef)) {
        UseResFile(saveRef);
    }
	
    if (fileRef != -1) {
	CloseResFile(fileRef);
    }

    return result;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_MacConvertTextResource --
 *
 *	Converts a TEXT resource into a Tcl suitable string.
 *
 * Side Effects:
 *	Mallocs the returned memory, converts '\r' to '\n', and appends a NULL.
 *
 * Results:
 *      A new malloced string.
 *
 *-----------------------------------------------------------------------------
 */

char *
Tcl_MacConvertTextResource(
    Handle resource)		/* Handle to TEXT resource. */
{
    int i, size;
    char *resultStr;

    size = GetResourceSizeOnDisk(resource);
    
    resultStr = ckalloc(size + 1);
    
    for (i=0; i<size; i++) {
	if ((*resource)[i] == '\r') {
	    resultStr[i] = '\n';
	} else {
	    resultStr[i] = (*resource)[i];
	}
    }
    
    resultStr[size] = '\0';

    return resultStr;
}

/*
 *-----------------------------------------------------------------------------
 *
 * Tcl_MacFindResource --
 *
 *	Higher level interface for loading resources.
 *
 * Side Effects:
 *	Attempts to load a resource.
 *
 * Results:
 *      A handle on success.
 *
 *-----------------------------------------------------------------------------
 */

Handle
Tcl_MacFindResource(
    Tcl_Interp *interp,		/* Interpreter in which to process file. */
    long resourceType,		/* Type of resource to load. */
    char *resourceName,		/* Name of resource to find,
				 * NULL if number should be used. */
    int resourceNumber,		/* Resource id of source. */
    char *resFileRef,		/* Registered resource file reference,
				 * NULL if searching all open resource files. */
    int *releaseIt)	        /* Should we release this resource when done. */
{
    Tcl_HashEntry *nameHashPtr;
    OpenResourceFork *resourceRef;
    int limitSearch = false;
    short saveRef;
    Handle resource;

    if (resFileRef != NULL) {
	nameHashPtr = Tcl_FindHashEntry(&nameTable, resFileRef);
	if (nameHashPtr == NULL) {
	    Tcl_AppendResult(interp, "invalid resource file reference \"",
			     resFileRef, "\"", (char *) NULL);
	    return NULL;
	}
	resourceRef = (OpenResourceFork *) Tcl_GetHashValue(nameHashPtr);
	saveRef = CurResFile();
	UseResFile((short) resourceRef->fileRef);
	limitSearch = true;
    }

    /* 
     * Some system resources (for example system resources) should not 
     * be released.  So we set autoload to false, and try to get the resource.
     * If the Master Pointer of the returned handle is null, then resource was 
     * not in memory, and it is safe to release it.  Otherwise, it is not.
     */
    
    SetResLoad(false);
	 
    if (resourceName == NULL) {
	if (limitSearch) {
	    resource = Get1Resource(resourceType, resourceNumber);
	} else {
	    resource = GetResource(resourceType, resourceNumber);
	}
    } else {
	c2pstr(resourceName);
	if (limitSearch) {
	    resource = Get1NamedResource(resourceType,
		    (StringPtr) resourceName);
	} else {
	    resource = GetNamedResource(resourceType,
		    (StringPtr) resourceName);
	}
	p2cstr((StringPtr) resourceName);
    }
    
    if (*resource == NULL) {
    	*releaseIt = 1;
    	LoadResource(resource);
    } else {
    	*releaseIt = 0;
    }
    
    SetResLoad(true);
    	

    if (limitSearch) {
	UseResFile(saveRef);
    }

    return resource;
}

/*
 *----------------------------------------------------------------------
 *
 * ResourceInit --
 *
 *	Initialize the structures used for resource management.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Read the code.
 *
 *----------------------------------------------------------------------
 */

static void
ResourceInit()
{

    initialized = 1;
    Tcl_InitHashTable(&nameTable, TCL_STRING_KEYS);
    Tcl_InitHashTable(&resourceTable, TCL_ONE_WORD_KEYS);
    resourceForkList = Tcl_NewObj();
    Tcl_IncrRefCount(resourceForkList);

    BuildResourceForkList();
    
}
/***/

/*Tcl_RegisterObjType(typePtr) */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_NewOSTypeObj --
 *
 *	This procedure is used to create a new resource name type object.
 *
 * Results:
 *	The newly created object is returned. This object will have a NULL
 *	string representation. The returned object has ref count 0.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_NewOSTypeObj(
    OSType newOSType)		/* Int used to initialize the new object. */
{
    register Tcl_Obj *objPtr;

    if (!osTypeInit) {
	osTypeInit = 1;
	Tcl_RegisterObjType(&osType);
    }

    objPtr = Tcl_NewObj();
    objPtr->bytes = NULL;
    objPtr->internalRep.longValue = newOSType;
    objPtr->typePtr = &osType;
    return objPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetOSTypeObj --
 *
 *	Modify an object to be a resource type and to have the 
 *	specified long value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The object's old string rep, if any, is freed. Also, any old
 *	internal rep is freed. 
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetOSTypeObj(
    Tcl_Obj *objPtr,		/* Object whose internal rep to init. */
    OSType newOSType)		/* Integer used to set object's value. */
{
    register Tcl_ObjType *oldTypePtr = objPtr->typePtr;

    if (!osTypeInit) {
	osTypeInit = 1;
	Tcl_RegisterObjType(&osType);
    }

    if ((oldTypePtr != NULL) && (oldTypePtr->freeIntRepProc != NULL)) {
	oldTypePtr->freeIntRepProc(objPtr);
    }
    
    objPtr->internalRep.longValue = newOSType;
    objPtr->typePtr = &osType;

    Tcl_InvalidateStringRep(objPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetOSTypeFromObj --
 *
 *	Attempt to return an int from the Tcl object "objPtr". If the object
 *	is not already an int, an attempt will be made to convert it to one.
 *
 * Results:
 *	The return value is a standard Tcl object result. If an error occurs
 *	during conversion, an error message is left in interp->objResult
 *	unless "interp" is NULL.
 *
 * Side effects:
 *	If the object is not already an int, the conversion will free
 *	any old internal representation.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetOSTypeFromObj(
    Tcl_Interp *interp, 	/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr,		/* The object from which to get a int. */
    OSType *osTypePtr)		/* Place to store resulting int. */
{
    register int result;
    
    if (!osTypeInit) {
	osTypeInit = 1;
	Tcl_RegisterObjType(&osType);
    }

    if (objPtr->typePtr == &osType) {
	*osTypePtr = objPtr->internalRep.longValue;
	return TCL_OK;
    }

    result = SetOSTypeFromAny(interp, objPtr);
    if (result == TCL_OK) {
	*osTypePtr = objPtr->internalRep.longValue;
    }
    return result;
}

/*
 *----------------------------------------------------------------------
 *
 * DupOSTypeInternalRep --
 *
 *	Initialize the internal representation of an int Tcl_Obj to a
 *	copy of the internal representation of an existing int object. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	"copyPtr"s internal rep is set to the integer corresponding to
 *	"srcPtr"s internal rep.
 *
 *----------------------------------------------------------------------
 */

static void
DupOSTypeInternalRep(
    Tcl_Obj *srcPtr,	/* Object with internal rep to copy. */
    Tcl_Obj *copyPtr)	/* Object with internal rep to set. */
{
    copyPtr->internalRep.longValue = srcPtr->internalRep.longValue;
    copyPtr->typePtr = &osType;
}

/*
 *----------------------------------------------------------------------
 *
 * SetOSTypeFromAny --
 *
 *	Attempt to generate an integer internal form for the Tcl object
 *	"objPtr".
 *
 * Results:
 *	The return value is a standard object Tcl result. If an error occurs
 *	during conversion, an error message is left in interp->objResult
 *	unless "interp" is NULL.
 *
 * Side effects:
 *	If no error occurs, an int is stored as "objPtr"s internal
 *	representation. 
 *
 *----------------------------------------------------------------------
 */

static int
SetOSTypeFromAny(
    Tcl_Interp *interp,		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr)		/* The object to convert. */
{
    Tcl_ObjType *oldTypePtr = objPtr->typePtr;
    char *string;
    int length;
    long newOSType;

    /*
     * Get the string representation. Make it up-to-date if necessary.
     */

    string = Tcl_GetStringFromObj(objPtr, &length);

    if (length != 4) {
	if (interp != NULL) {
	    Tcl_ResetResult(interp);
	    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp),
		    "expected Macintosh OS type but got \"", string, "\"",
		    (char *) NULL);
	}
	return TCL_ERROR;
    }
    newOSType =  *((long *) string);
    
    /*
     * The conversion to resource type succeeded. Free the old internalRep 
     * before setting the new one.
     */

    if ((oldTypePtr != NULL) &&	(oldTypePtr->freeIntRepProc != NULL)) {
	oldTypePtr->freeIntRepProc(objPtr);
    }
    
    objPtr->internalRep.longValue = newOSType;
    objPtr->typePtr = &osType;
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateStringOfOSType --
 *
 *	Update the string representation for an resource type object.
 *	Note: This procedure does not free an existing old string rep
 *	so storage will be lost if this has not already been done. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The object's string is set to a valid string that results from
 *	the int-to-string conversion.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateStringOfOSType(
    register Tcl_Obj *objPtr)	/* Int object whose string rep to update. */
{
    objPtr->bytes = ckalloc(5);
    sprintf(objPtr->bytes, "%-4.4s", &(objPtr->internalRep.longValue));
    objPtr->length = 4;
}

/*
 *----------------------------------------------------------------------
 *
 * GetRsrcRefFromObj --
 *
 *	Given a String object containing a resource file token, return
 *	the OpenResourceFork structure that it represents, or NULL if 
 *	the token cannot be found.  If okayOnReadOnly is false, it will 
 *      also check whether the token corresponds to a read-only file, 
 *      and return NULL if it is.
 *
 * Results:
 *	A pointer to an OpenResourceFork structure, or NULL.
 *
 * Side effects:
 *	An error message may be left in resultPtr.
 *
 *----------------------------------------------------------------------
 */

static OpenResourceFork *
GetRsrcRefFromObj(
    register Tcl_Obj *objPtr,	/* String obj containing file token     */
    int okayOnReadOnly,         /* Whether this operation is okay for a *
                                 * read only file.                      */
    const char *operation,      /* String containing the operation we   *
                                 * were trying to perform, used for errors */
    Tcl_Obj *resultPtr)         /* Tcl_Obj to contain error message     */
{
    char *stringPtr;
    Tcl_HashEntry *nameHashPtr;
    OpenResourceFork *resourceRef;
    int length;
    OSErr err;
    
    stringPtr = Tcl_GetStringFromObj(objPtr, &length);
    nameHashPtr = Tcl_FindHashEntry(&nameTable, stringPtr);
    if (nameHashPtr == NULL) {
        Tcl_AppendStringsToObj(resultPtr,
	        "invalid resource file reference \"",
	        stringPtr, "\"", (char *) NULL);
        return NULL;
    }

    resourceRef = (OpenResourceFork *) Tcl_GetHashValue(nameHashPtr);
    
    if (!okayOnReadOnly) {
        err = GetResFileAttrs((short) resourceRef->fileRef);
        if (err & mapReadOnly) {
            Tcl_AppendStringsToObj(resultPtr, "cannot ", operation, 
                    " resource file \"",
                    stringPtr, "\", it was opened read only",
                    (char *) NULL);
            return NULL;
        }
    }
    return resourceRef;
}

/*
 *----------------------------------------------------------------------
 *
 * TclMacRegisterResourceFork --
 *
 *	Register an open resource fork in the table of open resources 
 *	managed by the procedures in this file.  If the resource file
 *      is already registered with the table, then no new token is made.
 *
 *      The behavior is controlled by the value of tokenPtr, and of the 
 *	flags variable.  For tokenPtr, the possibilities are:
 *	  - NULL: The new token is auto-generated, but not returned.
 *        - The string value of tokenPtr is the empty string: Then
 *		the new token is auto-generated, and returned in tokenPtr
 *	  - tokenPtr has a value: The string value will be used for the token,
 *		unless it is already in use, in which case a new token will
 *		be generated, and returned in tokenPtr.
 *
 *      For the flags variable:  it can be one of:
 *	  - TCL_RESOURCE__INSERT_TAIL: The element is inserted at the
 *              end of the list of open resources.  Used only in Resource_Init.
 *	  - TCL_RESOURCE_DONT_CLOSE: The resource close command will not close
 *	        this resource.
 *	  - TCL_RESOURCE_CHECK_IF_OPEN: This will check to see if this file's
 *	        resource fork is already opened by this Tcl shell, and return 
 *	        an error without registering the resource fork.
 *
 * Results:
 *	Standard Tcl Result
 *
 * Side effects:
 *	An entry may be added to the resource name table.
 *
 *----------------------------------------------------------------------
 */

int
TclMacRegisterResourceFork(
    short fileRef,        	/* File ref for an open resource fork. */
    Tcl_Obj *tokenPtr,		/* A Tcl Object to which to write the  *
				 * new token */
    int flags)	     		/* 1 means insert at the head of the resource
                                 * fork list, 0 means at the tail */

{
    Tcl_HashEntry *resourceHashPtr;
    Tcl_HashEntry *nameHashPtr;
    OpenResourceFork *resourceRef;
    int new;
    char *resourceId = NULL;
   
    if (!initialized) {
        ResourceInit();
    }
    
    /*
     * If we were asked to, check that this file has not been opened
     * already with a different permission.  It it has, then return an error.
     */
     
    new = 1;
    
    if (flags & TCL_RESOURCE_CHECK_IF_OPEN) {
        Tcl_HashSearch search;
        short oldFileRef, filePermissionFlag;
        FCBPBRec newFileRec, oldFileRec;
        OSErr err;
        
        oldFileRec.ioCompletion = NULL;
        oldFileRec.ioFCBIndx = 0;
        oldFileRec.ioNamePtr = NULL;
        
        newFileRec.ioCompletion = NULL;
        newFileRec.ioFCBIndx = 0;
        newFileRec.ioNamePtr = NULL;
        newFileRec.ioVRefNum = 0;
        newFileRec.ioRefNum = fileRef;
        err = PBGetFCBInfo(&newFileRec, false);
        filePermissionFlag = ( newFileRec.ioFCBFlags >> 12 ) & 0x1;
            
        
        resourceHashPtr = Tcl_FirstHashEntry(&resourceTable, &search);
        while (resourceHashPtr != NULL) {
            oldFileRef = (short) Tcl_GetHashKey(&resourceTable,
                    resourceHashPtr);
            if (oldFileRef == fileRef) {
                new = 0;
                break;
            }
            oldFileRec.ioVRefNum = 0;
            oldFileRec.ioRefNum = oldFileRef;
            err = PBGetFCBInfo(&oldFileRec, false);
            
            /*
             * err might not be noErr either because the file has closed 
             * out from under us somehow, which is bad but we're not going
             * to fix it here, OR because it is the ROM MAP, which has a 
             * fileRef, but can't be gotten to by PBGetFCBInfo.
             */
            if ((err == noErr) 
                    && (newFileRec.ioFCBVRefNum == oldFileRec.ioFCBVRefNum)
                    && (newFileRec.ioFCBFlNm == oldFileRec.ioFCBFlNm)) {
                /*
		 * In MacOS 8.1 it seems like we get different file refs even
                 * though we pass the same file & permissions.  This is not
                 * what Inside Mac says should happen, but it does, so if it
                 * does, then close the new res file and return the original
                 * one...
		 */
                 
                if (filePermissionFlag == ((oldFileRec.ioFCBFlags >> 12) & 0x1)) {
                    CloseResFile(fileRef);
                    new = 0;
                    break;
                } else {
                    if (tokenPtr != NULL) {
                        Tcl_SetStringObj(tokenPtr, "Resource already open with different permissions.", -1);
                    }   	
                    return TCL_ERROR;
                }
            }
            resourceHashPtr = Tcl_NextHashEntry(&search);
        }
    }
       
    
    /*
     * If the file has already been opened with these same permissions, then it
     * will be in our list and we will have set new to 0 above.
     * So we will just return the token (if tokenPtr is non-null)
     */
     
    if (new) {
        resourceHashPtr = Tcl_CreateHashEntry(&resourceTable,
		(char *) fileRef, &new);
    }
    
    if (!new) {
        if (tokenPtr != NULL) {   
            resourceId = (char *) Tcl_GetHashValue(resourceHashPtr);
	    Tcl_SetStringObj(tokenPtr, resourceId, -1);
        }
        return TCL_OK;
    }        

    /*
     * If we were passed in a result pointer which is not an empty
     * string, attempt to use that as the key.  If the key already
     * exists, silently fall back on resource%d...
     */
     
    if (tokenPtr != NULL) {
        char *tokenVal;
        int length;
        tokenVal = (char *) Tcl_GetStringFromObj(tokenPtr, &length);
        if (length > 0) {
            nameHashPtr = Tcl_FindHashEntry(&nameTable, tokenVal);
            if (nameHashPtr == NULL) {
                resourceId = ckalloc(length + 1);
                memcpy(resourceId, tokenVal, length);
                resourceId[length] = '\0';
            }
        }
    }
    
    if (resourceId == NULL) {	
        resourceId = (char *) ckalloc(15);
        sprintf(resourceId, "resource%d", newId);
    }
    
    Tcl_SetHashValue(resourceHashPtr, resourceId);
    newId++;

    nameHashPtr = Tcl_CreateHashEntry(&nameTable, resourceId, &new);
    if (!new) {
	panic("resource id has repeated itself");
    }
    
    resourceRef = (OpenResourceFork *) ckalloc(sizeof(OpenResourceFork));
    resourceRef->fileRef = fileRef;
    resourceRef->flags = flags;
    
    Tcl_SetHashValue(nameHashPtr, (ClientData) resourceRef);
    if (tokenPtr != NULL) {
        Tcl_SetStringObj(tokenPtr, resourceId, -1);
    }
    
    if (flags & TCL_RESOURCE_INSERT_TAIL) {
        Tcl_ListObjAppendElement(NULL, resourceForkList, tokenPtr);
    } else {
        Tcl_ListObjReplace(NULL, resourceForkList, 0, 0, 1, &tokenPtr);	
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TclMacUnRegisterResourceFork --
 *
 *	Removes the entry for an open resource fork from the table of 
 *	open resources managed by the procedures in this file.
 *      If resultPtr is not NULL, it will be used for error reporting.
 *
 * Results:
 *	The fileRef for this token, or -1 if an error occured.
 *
 * Side effects:
 *	An entry is removed from the resource name table.
 *
 *----------------------------------------------------------------------
 */

short
TclMacUnRegisterResourceFork(
    char *tokenPtr,
    Tcl_Obj *resultPtr)

{
    Tcl_HashEntry *resourceHashPtr;
    Tcl_HashEntry *nameHashPtr;
    OpenResourceFork *resourceRef;
    char *resourceId = NULL;
    short fileRef;
    char *bytes;
    int i, match, index, listLen, length, elemLen;
    Tcl_Obj **elemPtrs;
    
     
    nameHashPtr = Tcl_FindHashEntry(&nameTable, tokenPtr);
    if (nameHashPtr == NULL) {
        if (resultPtr != NULL) {
	    Tcl_AppendStringsToObj(resultPtr,
		    "invalid resource file reference \"",
		    tokenPtr, "\"", (char *) NULL);
        }
	return -1;
    }
    
    resourceRef = (OpenResourceFork *) Tcl_GetHashValue(nameHashPtr);
    fileRef = resourceRef->fileRef;
        
    if ( resourceRef->flags & TCL_RESOURCE_DONT_CLOSE ) {
        if (resultPtr != NULL) {
	    Tcl_AppendStringsToObj(resultPtr,
		    "can't close \"", tokenPtr, "\" resource file", 
		    (char *) NULL);
	}
	return -1;
    }            

    Tcl_DeleteHashEntry(nameHashPtr);
    ckfree((char *) resourceRef);
    
    
    /* 
     * Now remove the resource from the resourceForkList object 
     */
     
    Tcl_ListObjGetElements(NULL, resourceForkList, &listLen, &elemPtrs);
    
 
    index = -1;
    length = strlen(tokenPtr);
    
    for (i = 0; i < listLen; i++) {
	match = 0;
	bytes = Tcl_GetStringFromObj(elemPtrs[i], &elemLen);
	if (length == elemLen) {
		match = (memcmp(bytes, tokenPtr,
			(size_t) length) == 0);
	}
	if (match) {
	    index = i;
	    break;
	}
    }
    if (!match) {
        panic("the resource Fork List is out of synch!");
    }
    
    Tcl_ListObjReplace(NULL, resourceForkList, index, 1, 0, NULL);
    
    resourceHashPtr = Tcl_FindHashEntry(&resourceTable, (char *) fileRef);
    
    if (resourceHashPtr == NULL) {
	panic("Resource & Name tables are out of synch in resource command.");
    }
    ckfree(Tcl_GetHashValue(resourceHashPtr));
    Tcl_DeleteHashEntry(resourceHashPtr);
    
    return fileRef;

}


/*
 *----------------------------------------------------------------------
 *
 * BuildResourceForkList --
 *
 *	Traverses the list of open resource forks, and builds the 
 *	list of resources forks.  Also creates a resource token for any that 
 *      are opened but not registered with our resource system.
 *      This is based on code from Apple DTS.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *      The list of resource forks is updated.
 *	The resource name table may be augmented.
 *
 *----------------------------------------------------------------------
 */

void
BuildResourceForkList()
{
    Handle currentMapHandle, mSysMapHandle;  
    Ptr tempPtr;
    FCBPBRec fileRec;
    char fileName[256];
    char appName[62];
    Tcl_Obj *nameObj;
    OSErr err;
    ProcessSerialNumber psn;
    ProcessInfoRec info;
    FSSpec fileSpec;
        
    /* 
     * Get the application name, so we can substitute
     * the token "application" for the application's resource.
     */ 
     
    GetCurrentProcess(&psn);
    info.processInfoLength = sizeof(ProcessInfoRec);
    info.processName = (StringPtr) &appName;
    info.processAppSpec = &fileSpec;
    GetProcessInformation(&psn, &info);
    p2cstr((StringPtr) appName);

    
    fileRec.ioCompletion = NULL;
    fileRec.ioVRefNum = 0;
    fileRec.ioFCBIndx = 0;
    fileRec.ioNamePtr = (StringPtr) &fileName;
    
    
    currentMapHandle = LMGetTopMapHndl();
    mSysMapHandle = LMGetSysMapHndl();
    
    while (1) {
        /* 
         * Now do the ones opened after the application.
         */
       
        nameObj = Tcl_NewObj();
        
        tempPtr = *currentMapHandle;

        fileRec.ioRefNum = *((short *) (tempPtr + 20));
        err = PBGetFCBInfo(&fileRec, false);
        
        if (err != noErr) {
            /*
             * The ROM resource map does not correspond to an opened file...
             */
             Tcl_SetStringObj(nameObj, "ROM Map", -1);
        } else {
            p2cstr((StringPtr) fileName);
            if (strcmp(fileName,(char *) appName) == 0) {
                Tcl_SetStringObj(nameObj, "application", -1);
            } else {
                Tcl_SetStringObj(nameObj, fileName, -1);
            }
            c2pstr(fileName);
        }
        
        TclMacRegisterResourceFork(fileRec.ioRefNum, nameObj, 
            TCL_RESOURCE_DONT_CLOSE | TCL_RESOURCE_INSERT_TAIL);
       
        if (currentMapHandle == mSysMapHandle) {
            break;
        }
        
        currentMapHandle = *((Handle *) (tempPtr + 16));
    }
}
