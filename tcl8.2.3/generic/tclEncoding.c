/*
 * tclEncoding.c --
 *
 *	Contains the implementation of the encoding conversion package.
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

typedef size_t (LengthProc)_ANSI_ARGS_((CONST char *src));

/*
 * The following data structure represents an encoding, which describes how
 * to convert between various character sets and UTF-8.
 */

typedef struct Encoding {
    char *name;			/* Name of encoding.  Malloced because (1)
				 * hash table entry that owns this encoding
				 * may be freed prior to this encoding being
				 * freed, (2) string passed in the
				 * Tcl_EncodingType structure may not be
				 * persistent. */
    Tcl_EncodingConvertProc *toUtfProc;
				/* Procedure to convert from external
				 * encoding into UTF-8. */
    Tcl_EncodingConvertProc *fromUtfProc;
				/* Procedure to convert from UTF-8 into
				 * external encoding. */
    Tcl_EncodingFreeProc *freeProc;
				/* If non-NULL, procedure to call when this
				 * encoding is deleted. */
    int nullSize;		/* Number of 0x00 bytes that signify
				 * end-of-string in this encoding.  This
				 * number is used to determine the source
				 * string length when the srcLen argument is
				 * negative.  This number can be 1 or 2. */
    ClientData clientData;	/* Arbitrary value associated with encoding
				 * type.  Passed to conversion procedures. */
    LengthProc *lengthProc;	/* Function to compute length of
				 * null-terminated strings in this encoding.
				 * If nullSize is 1, this is strlen; if
				 * nullSize is 2, this is a function that
				 * returns the number of bytes in a 0x0000
				 * terminated string. */
    int refCount;		/* Number of uses of this structure. */
    Tcl_HashEntry *hPtr;	/* Hash table entry that owns this encoding. */
} Encoding;

/*
 * The following structure is the clientData for a dynamically-loaded,
 * table-driven encoding created by LoadTableEncoding().  It maps between
 * Unicode and a single-byte, double-byte, or multibyte (1 or 2 bytes only)
 * encoding.
 */

typedef struct TableEncodingData {
    int fallback;		/* Character (in this encoding) to
				 * substitute when this encoding cannot
				 * represent a UTF-8 character. */
    char prefixBytes[256];	/* If a byte in the input stream is a lead
				 * byte for a 2-byte sequence, the
				 * corresponding entry in this array is 1,
				 * otherwise it is 0. */
    unsigned short **toUnicode;	/* Two dimensional sparse matrix to map
				 * characters from the encoding to Unicode.
				 * Each element of the toUnicode array points
				 * to an array of 256 shorts.  If there is no
				 * corresponding character in Unicode, the
				 * value in the matrix is 0x0000.  malloc'd. */
    unsigned short **fromUnicode;
				/* Two dimensional sparse matrix to map
				 * characters from Unicode to the encoding.
				 * Each element of the fromUnicode array
				 * points to an array of 256 shorts.  If there
				 * is no corresponding character the encoding,
				 * the value in the matrix is 0x0000.
				 * malloc'd. */
} TableEncodingData;

/*
 * The following structures is the clientData for a dynamically-loaded,
 * escape-driven encoding that is itself comprised of other simpler
 * encodings.  An example is "iso-2022-jp", which uses escape sequences to
 * switch between ascii, jis0208, jis0212, gb2312, and ksc5601.  Note that
 * "escape-driven" does not necessarily mean that the ESCAPE character is
 * the character used for switching character sets.
 */

typedef struct EscapeSubTable {
    unsigned int sequenceLen;	/* Length of following string. */
    char sequence[16];		/* Escape code that marks this encoding. */
    char name[32];		/* Name for encoding. */
    Encoding *encodingPtr;	/* Encoding loaded using above name, or NULL
				 * if this sub-encoding has not been needed
				 * yet. */
} EscapeSubTable;

typedef struct EscapeEncodingData {
    int fallback;		/* Character (in this encoding) to
				 * substitute when this encoding cannot
				 * represent a UTF-8 character. */
    unsigned int initLen;	/* Length of following string. */
    char init[16];		/* String to emit or expect before first char
				 * in conversion. */
    unsigned int finalLen;	/* Length of following string. */
    char final[16];		/* String to emit or expect after last char
				 * in conversion. */
    char prefixBytes[256];	/* If a byte in the input stream is the 
				 * first character of one of the escape 
				 * sequences in the following array, the 
				 * corresponding entry in this array is 1,
				 * otherwise it is 0. */
    int numSubTables;		/* Length of following array. */
    EscapeSubTable subTables[1];/* Information about each EscapeSubTable
				 * used by this encoding type.  The actual 
				 * size will be as large as necessary to 
				 * hold all EscapeSubTables. */
} EscapeEncodingData;

/*
 * Constants used when loading an encoding file to identify the type of the
 * file.
 */

#define ENCODING_SINGLEBYTE	0
#define ENCODING_DOUBLEBYTE	1
#define ENCODING_MULTIBYTE	2
#define ENCODING_ESCAPE		3

/*
 * Initialize the default encoding directory.  If this variable contains
 * a non NULL value, it will be the first path used to locate the
 * system encoding files.
 */

char *tclDefaultEncodingDir = NULL;

/*
 * Hash table that keeps track of all loaded Encodings.  Keys are
 * the string names that represent the encoding, values are (Encoding *).
 */
 
static Tcl_HashTable encodingTable;
TCL_DECLARE_MUTEX(encodingMutex)

/*
 * The following are used to hold the default and current system encodings.  
 * If NULL is passed to one of the conversion routines, the current setting 
 * of the system encoding will be used to perform the conversion.
 */

static Tcl_Encoding defaultEncoding;
static Tcl_Encoding systemEncoding;

/*
 * The following variable is used in the sparse matrix code for a
 * TableEncoding to represent a page in the table that has no entries.
 */

static unsigned short emptyPage[256];

/*
 * Procedures used only in this module.
 */

static int		BinaryProc _ANSI_ARGS_((ClientData clientData,
			    CONST char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr));
static void		EscapeFreeProc _ANSI_ARGS_((ClientData clientData));
static int		EscapeFromUtfProc _ANSI_ARGS_((ClientData clientData,
			    CONST char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr));
static int		EscapeToUtfProc _ANSI_ARGS_((ClientData clientData,
			    CONST char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr));
static void		FreeEncoding _ANSI_ARGS_((Tcl_Encoding encoding));
static Encoding *	GetTableEncoding _ANSI_ARGS_((
			    EscapeEncodingData *dataPtr, int state));
static Tcl_Encoding	LoadEncodingFile _ANSI_ARGS_((Tcl_Interp *interp,
			    CONST char *name));
static Tcl_Encoding	LoadTableEncoding _ANSI_ARGS_((Tcl_Interp *interp,
			    CONST char *name, int type, Tcl_Channel chan));
static Tcl_Encoding	LoadEscapeEncoding _ANSI_ARGS_((CONST char *name, 
			    Tcl_Channel chan));
static Tcl_Channel	OpenEncodingFile _ANSI_ARGS_((CONST char *dir,
			    CONST char *name));
static void		TableFreeProc _ANSI_ARGS_((ClientData clientData));
static int		TableFromUtfProc _ANSI_ARGS_((ClientData clientData,
			    CONST char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr));
static int		TableToUtfProc _ANSI_ARGS_((ClientData clientData,
			    CONST char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr));
static size_t		unilen _ANSI_ARGS_((CONST char *src));
static int		UnicodeToUtfProc _ANSI_ARGS_((ClientData clientData,
			    CONST char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr));
static int		UtfToUnicodeProc _ANSI_ARGS_((ClientData clientData,
			    CONST char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr));
static int		UtfToUtfProc _ANSI_ARGS_((ClientData clientData,
			    CONST char *src, int srcLen, int flags,
			    Tcl_EncodingState *statePtr, char *dst, int dstLen,
			    int *srcReadPtr, int *dstWrotePtr,
			    int *dstCharsPtr));


/*
 *---------------------------------------------------------------------------
 *
 * TclInitEncodingSubsystem --
 *
 *	Initialize all resources used by this subsystem on a per-process
 *	basis.  
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Depends on the memory, object, and IO subsystems.
 *
 *---------------------------------------------------------------------------
 */

void
TclInitEncodingSubsystem()
{
    Tcl_EncodingType type;

    Tcl_MutexLock(&encodingMutex);
    Tcl_InitHashTable(&encodingTable, TCL_STRING_KEYS);
    Tcl_MutexUnlock(&encodingMutex);
    
    /*
     * Create a few initial encodings.  Note that the UTF-8 to UTF-8 
     * translation is not a no-op, because it will turn a stream of
     * improperly formed UTF-8 into a properly formed stream.
     */

    type.encodingName	= "identity";
    type.toUtfProc	= BinaryProc;
    type.fromUtfProc	= BinaryProc;
    type.freeProc	= NULL;
    type.nullSize	= 1;
    type.clientData	= NULL;

    defaultEncoding	= Tcl_CreateEncoding(&type);
    systemEncoding	= Tcl_GetEncoding(NULL, type.encodingName);

    type.encodingName	= "utf-8";
    type.toUtfProc	= UtfToUtfProc;
    type.fromUtfProc    = UtfToUtfProc;
    type.freeProc	= NULL;
    type.nullSize	= 1;
    type.clientData	= NULL;
    Tcl_CreateEncoding(&type);

    type.encodingName   = "unicode";
    type.toUtfProc	= UnicodeToUtfProc;
    type.fromUtfProc    = UtfToUnicodeProc;
    type.freeProc	= NULL;
    type.nullSize	= 2;
    type.clientData	= NULL;
    Tcl_CreateEncoding(&type);
}


/*
 *----------------------------------------------------------------------
 *
 * TclFinalizeEncodingSubsystem --
 *
 *	Release the state associated with the encoding subsystem.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees all of the encodings.
 *
 *----------------------------------------------------------------------
 */

void
TclFinalizeEncodingSubsystem()
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    Encoding *encodingPtr;

    Tcl_MutexLock(&encodingMutex);
    hPtr = Tcl_FirstHashEntry(&encodingTable, &search);
    while (hPtr != NULL) {
	encodingPtr = (Encoding *) Tcl_GetHashValue(hPtr);
	if (encodingPtr->freeProc != NULL) {
	    (*encodingPtr->freeProc)(encodingPtr->clientData);
	}
	ckfree((char *) encodingPtr->name);
	ckfree((char *) encodingPtr);
	hPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&encodingTable);
    Tcl_MutexUnlock(&encodingMutex);
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_GetDefaultEncodingDir --
 *
 *
 * Results:
 *
 * Side effects:
 *
 *-------------------------------------------------------------------------
 */

char *
Tcl_GetDefaultEncodingDir()
{
    return tclDefaultEncodingDir;
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_SetDefaultEncodingDir --
 *
 *
 * Results:
 *
 * Side effects:
 *
 *-------------------------------------------------------------------------
 */

void
Tcl_SetDefaultEncodingDir(path)
    char *path;
{
    tclDefaultEncodingDir = (char *)ckalloc((unsigned) strlen(path) + 1);
    strcpy(tclDefaultEncodingDir, path);
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_GetEncoding --
 *
 *	Given the name of a encoding, find the corresponding Tcl_Encoding
 *	token.  If the encoding did not already exist, Tcl attempts to
 *	dynamically load an encoding by that name.
 *
 * Results:
 *	Returns a token that represents the encoding.  If the name didn't
 *	refer to any known or loadable encoding, NULL is returned.  If
 *	NULL was returned, an error message is left in interp's result
 *	object, unless interp was NULL.
 *
 * Side effects:
 *	The new encoding type is entered into a table visible to all
 *	interpreters, keyed off the encoding's name.  For each call to
 *	this procedure, there should eventually be a call to
 *	Tcl_FreeEncoding, so that the database can be cleaned up when
 *	encodings aren't needed anymore.
 *
 *-------------------------------------------------------------------------
 */

Tcl_Encoding
Tcl_GetEncoding(interp, name)
    Tcl_Interp *interp;		/* Interp for error reporting, if not NULL. */
    CONST char *name;		/* The name of the desired encoding. */
{
    Tcl_HashEntry *hPtr;
    Encoding *encodingPtr;

    Tcl_MutexLock(&encodingMutex);
    if (name == NULL) {
	encodingPtr = (Encoding *) systemEncoding;
	encodingPtr->refCount++;
	Tcl_MutexUnlock(&encodingMutex);
	return systemEncoding;
    }

    hPtr = Tcl_FindHashEntry(&encodingTable, name);
    if (hPtr != NULL) {
	encodingPtr = (Encoding *) Tcl_GetHashValue(hPtr);
	encodingPtr->refCount++;
	Tcl_MutexUnlock(&encodingMutex);
	return (Tcl_Encoding) encodingPtr;
    }
    Tcl_MutexUnlock(&encodingMutex);
    return LoadEncodingFile(interp, name);
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_FreeEncoding --
 *
 *	This procedure is called to release an encoding allocated by
 *	Tcl_CreateEncoding() or Tcl_GetEncoding().
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with the encoding is decremented
 *	and the encoding may be deleted if nothing is using it anymore.
 *
 *---------------------------------------------------------------------------
 */

void
Tcl_FreeEncoding(encoding)
    Tcl_Encoding encoding;
{
    Tcl_MutexLock(&encodingMutex);
    FreeEncoding(encoding);
    Tcl_MutexUnlock(&encodingMutex);
}

/*
 *----------------------------------------------------------------------
 *
 * FreeEncoding --
 *
 *	This procedure is called to release an encoding by procedures
 *	that already have the encodingMutex.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with the encoding is decremented
 *	and the encoding may be deleted if nothing is using it anymore.
 *
 *----------------------------------------------------------------------
 */

static void
FreeEncoding(encoding)
    Tcl_Encoding encoding;
{
    Encoding *encodingPtr;
    
    encodingPtr = (Encoding *) encoding;
    if (encodingPtr == NULL) {
	return;
    }
    encodingPtr->refCount--;
    if (encodingPtr->refCount == 0) {
	if (encodingPtr->freeProc != NULL) {
	    (*encodingPtr->freeProc)(encodingPtr->clientData);
	}
	if (encodingPtr->hPtr != NULL) {
	    Tcl_DeleteHashEntry(encodingPtr->hPtr);
	}
	ckfree((char *) encodingPtr->name);
	ckfree((char *) encodingPtr);
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_GetEncodingName --
 *
 *	Given an encoding, return the name that was used to constuct
 *	the encoding.
 *
 * Results:
 *	The name of the encoding.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

char *
Tcl_GetEncodingName(encoding)
    Tcl_Encoding encoding;	/* The encoding whose name to fetch. */
{
    Encoding *encodingPtr;

    if (encoding == NULL) {
	encoding = systemEncoding;
    }
    encodingPtr = (Encoding *) encoding;
    return encodingPtr->name;
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_GetEncodingNames --
 *
 *	Get the list of all known encodings, including the ones stored
 *	as files on disk in the encoding path.
 *
 * Results:
 *	Modifies interp's result object to hold a list of all the available
 *	encodings.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

void
Tcl_GetEncodingNames(interp)
    Tcl_Interp *interp;		/* Interp to hold result. */
{
    Tcl_HashSearch search;
    Tcl_HashEntry *hPtr;
    Tcl_Obj *pathPtr, *resultPtr;
    int dummy;

    Tcl_HashTable table;

    Tcl_MutexLock(&encodingMutex);
    Tcl_InitHashTable(&table, TCL_STRING_KEYS);
    hPtr = Tcl_FirstHashEntry(&encodingTable, &search);
    while (hPtr != NULL) {
	Encoding *encodingPtr;
	
	encodingPtr = (Encoding *) Tcl_GetHashValue(hPtr);
	Tcl_CreateHashEntry(&table, encodingPtr->name, &dummy);
	hPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_MutexUnlock(&encodingMutex);

    pathPtr = TclGetLibraryPath();
    if (pathPtr != NULL) {
	int i, objc;
	Tcl_Obj **objv;
	Tcl_DString pwdString;
	char globArgString[10];

	objc = 0;
	Tcl_ListObjGetElements(NULL, pathPtr, &objc, &objv);

	Tcl_GetCwd(interp, &pwdString);

	for (i = 0; i < objc; i++) {
	    char *string;
	    int j, objc2, length;
	    Tcl_Obj **objv2;

	    string = Tcl_GetStringFromObj(objv[i], NULL);
	    Tcl_ResetResult(interp);

	    /*
	     * TclGlob() changes the contents of globArgString, which causes
	     * a segfault if we pass in a pointer to non-writeable memory.
	     * TclGlob() puts its results directly into interp.
	     */

	    strcpy(globArgString, "*.enc");
	    if ((Tcl_Chdir(string) == 0)
		    && (Tcl_Chdir("encoding") == 0)
		    && (TclGlob(interp, globArgString, 0) == TCL_OK)) {
		objc2 = 0;

		Tcl_ListObjGetElements(NULL, Tcl_GetObjResult(interp), &objc2,
			&objv2);

		for (j = 0; j < objc2; j++) {
		    string = Tcl_GetStringFromObj(objv2[j], &length);
		    length -= 4;
		    if (length > 0) {
			string[length] = '\0';
			Tcl_CreateHashEntry(&table, string, &dummy);
			string[length] = '.';
		    }
		}
	    }
	    Tcl_Chdir(Tcl_DStringValue(&pwdString));
	}
	Tcl_DStringFree(&pwdString);
    }

    /*
     * Clear any values placed in the result by globbing.
     */

    Tcl_ResetResult(interp);
    resultPtr = Tcl_GetObjResult(interp);

    hPtr = Tcl_FirstHashEntry(&table, &search);
    while (hPtr != NULL) {
	Tcl_Obj *strPtr;

	strPtr = Tcl_NewStringObj(Tcl_GetHashKey(&table, hPtr), -1);
	Tcl_ListObjAppendElement(NULL, resultPtr, strPtr);
	hPtr = Tcl_NextHashEntry(&search);
    }
    Tcl_DeleteHashTable(&table);
}

/*
 *------------------------------------------------------------------------
 *
 * Tcl_SetSystemEncoding --
 *
 *	Sets the default encoding that should be used whenever the user
 *	passes a NULL value in to one of the conversion routines.
 *	If the supplied name is NULL, the system encoding is reset to the
 *	default system encoding.
 *
 * Results:
 *	The return value is TCL_OK if the system encoding was successfully
 *	set to the encoding specified by name, TCL_ERROR otherwise.  If
 *	TCL_ERROR is returned, an error message is left in interp's result
 *	object, unless interp was NULL.
 *
 * Side effects:
 *	The reference count of the new system encoding is incremented.
 *	The reference count of the old system encoding is decremented and 
 *	it may be freed.  
 *
 *------------------------------------------------------------------------
 */

int
Tcl_SetSystemEncoding(interp, name)
    Tcl_Interp *interp;		/* Interp for error reporting, if not NULL. */
    CONST char *name;		/* The name of the desired encoding, or NULL
				 * to reset to default encoding. */
{
    Tcl_Encoding encoding;
    Encoding *encodingPtr;

    if (name == NULL) {
	Tcl_MutexLock(&encodingMutex);
	encoding = defaultEncoding;
	encodingPtr = (Encoding *) encoding;
	encodingPtr->refCount++;
	Tcl_MutexUnlock(&encodingMutex);
    } else {
	encoding = Tcl_GetEncoding(interp, name);
	if (encoding == NULL) {
	    return TCL_ERROR;
	}
    }

    Tcl_MutexLock(&encodingMutex);
    FreeEncoding(systemEncoding);
    systemEncoding = encoding;
    Tcl_MutexUnlock(&encodingMutex);

    return TCL_OK;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_CreateEncoding --
 *
 *	This procedure is called to define a new encoding and the procedures
 *	that are used to convert between the specified encoding and Unicode.  
 *
 * Results:
 *	Returns a token that represents the encoding.  If an encoding with
 *	the same name already existed, the old encoding token remains
 *	valid and continues to behave as it used to, and will eventually
 *	be garbage collected when the last reference to it goes away.  Any
 *	subsequent calls to Tcl_GetEncoding with the specified name will
 *	retrieve the most recent encoding token.
 *
 * Side effects:
 *	The new encoding type is entered into a table visible to all
 *	interpreters, keyed off the encoding's name.  For each call to
 *	this procedure, there should eventually be a call to
 *	Tcl_FreeEncoding, so that the database can be cleaned up when
 *	encodings aren't needed anymore.
 *
 *---------------------------------------------------------------------------
 */ 

Tcl_Encoding
Tcl_CreateEncoding(typePtr)
    Tcl_EncodingType *typePtr;	/* The encoding type. */
{
    Tcl_HashEntry *hPtr;
    int new;
    Encoding *encodingPtr;
    char *name;

    Tcl_MutexLock(&encodingMutex);
    hPtr = Tcl_CreateHashEntry(&encodingTable, typePtr->encodingName, &new);
    if (new == 0) {
	/*
	 * Remove old encoding from hash table, but don't delete it until
	 * last reference goes away.
	 */
	 
	encodingPtr = (Encoding *) Tcl_GetHashValue(hPtr);
	encodingPtr->hPtr = NULL;
    }

    name = ckalloc((unsigned) strlen(typePtr->encodingName) + 1);
    
    encodingPtr = (Encoding *) ckalloc(sizeof(Encoding));
    encodingPtr->name		= strcpy(name, typePtr->encodingName);
    encodingPtr->toUtfProc	= typePtr->toUtfProc;
    encodingPtr->fromUtfProc	= typePtr->fromUtfProc;
    encodingPtr->freeProc	= typePtr->freeProc;
    encodingPtr->nullSize	= typePtr->nullSize;
    encodingPtr->clientData	= typePtr->clientData;
    if (typePtr->nullSize == 1) {
	encodingPtr->lengthProc = strlen;
    } else {
	encodingPtr->lengthProc = unilen;
    }
    encodingPtr->refCount	= 1;
    encodingPtr->hPtr		= hPtr;
    Tcl_SetHashValue(hPtr, encodingPtr);

    Tcl_MutexUnlock(&encodingMutex);

    return (Tcl_Encoding) encodingPtr;
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_ExternalToUtfDString --
 *
 *	Convert a source buffer from the specified encoding into UTF-8.
 *	If any of the bytes in the source buffer are invalid or cannot
 *	be represented in the target encoding, a default fallback
 *	character will be substituted.
 *
 * Results:
 *	The converted bytes are stored in the DString, which is then NULL
 *	terminated.  The return value is a pointer to the value stored 
 *	in the DString.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

char * 
Tcl_ExternalToUtfDString(encoding, src, srcLen, dstPtr)
    Tcl_Encoding encoding;	/* The encoding for the source string, or
				 * NULL for the default system encoding. */
    CONST char *src;		/* Source string in specified encoding. */
    int srcLen;			/* Source string length in bytes, or < 0 for
				 * encoding-specific string length. */
    Tcl_DString *dstPtr;	/* Uninitialized or free DString in which 
				 * the converted string is stored. */
{
    char *dst;
    Tcl_EncodingState state;
    Encoding *encodingPtr;
    int flags, dstLen, result, soFar, srcRead, dstWrote, dstChars;

    Tcl_DStringInit(dstPtr);
    dst = Tcl_DStringValue(dstPtr);
    dstLen = dstPtr->spaceAvl - 1;
    
    if (encoding == NULL) {
	encoding = systemEncoding;
    }
    encodingPtr = (Encoding *) encoding;

    if (src == NULL) {
	srcLen = 0;
    } else if (srcLen < 0) {
	srcLen = (*encodingPtr->lengthProc)(src);
    }
    flags = TCL_ENCODING_START | TCL_ENCODING_END;
    while (1) {
	result = (*encodingPtr->toUtfProc)(encodingPtr->clientData, src,
		srcLen, flags, &state, dst, dstLen, &srcRead, &dstWrote,
		&dstChars);
	soFar = dst + dstWrote - Tcl_DStringValue(dstPtr);
	if (result != TCL_CONVERT_NOSPACE) {
	    Tcl_DStringSetLength(dstPtr, soFar);
	    return Tcl_DStringValue(dstPtr);
	}
	flags &= ~TCL_ENCODING_START;
	src += srcRead;
	srcLen -= srcRead;
	if (Tcl_DStringLength(dstPtr) == 0) {
	    Tcl_DStringSetLength(dstPtr, dstLen);
	}
	Tcl_DStringSetLength(dstPtr, 2 * Tcl_DStringLength(dstPtr) + 1);
	dst = Tcl_DStringValue(dstPtr) + soFar;
	dstLen = Tcl_DStringLength(dstPtr) - soFar - 1;
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_ExternalToUtf --
 *
 *	Convert a source buffer from the specified encoding into UTF-8,
 *
 * Results:
 *	The return value is one of TCL_OK, TCL_CONVERT_MULTIBYTE,
 *	TCL_CONVERT_SYNTAX, TCL_CONVERT_UNKNOWN, or TCL_CONVERT_NOSPACE,
 *	as documented in tcl.h.
 *
 * Side effects:
 *	The converted bytes are stored in the output buffer.  
 *
 *-------------------------------------------------------------------------
 */

int
Tcl_ExternalToUtf(interp, encoding, src, srcLen, flags, statePtr, dst,
	dstLen, srcReadPtr, dstWrotePtr, dstCharsPtr)
    Tcl_Interp *interp;		/* Interp for error return, if not NULL. */
    Tcl_Encoding encoding;	/* The encoding for the source string, or
				 * NULL for the default system encoding. */
    CONST char *src;		/* Source string in specified encoding. */
    int srcLen;			/* Source string length in bytes, or < 0 for
				 * encoding-specific string length. */
    int flags;			/* Conversion control flags. */
    Tcl_EncodingState *statePtr;/* Place for conversion routine to store
				 * state information used during a piecewise
				 * conversion.  Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst;			/* Output buffer in which converted string
				 * is stored. */
    int dstLen;			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr;		/* Filled with the number of bytes from the
				 * source string that were converted.  This
				 * may be less than the original source length
				 * if there was a problem converting some
				 * source characters. */
    int *dstWrotePtr;		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr;		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    Encoding *encodingPtr;
    int result, srcRead, dstWrote, dstChars;
    Tcl_EncodingState state;
    
    if (encoding == NULL) {
	encoding = systemEncoding;
    }
    encodingPtr = (Encoding *) encoding;

    if (src == NULL) {
	srcLen = 0;
    } else if (srcLen < 0) {
	srcLen = (*encodingPtr->lengthProc)(src);
    }
    if (statePtr == NULL) {
	flags |= TCL_ENCODING_START | TCL_ENCODING_END;
	statePtr = &state;
    }
    if (srcReadPtr == NULL) {
	srcReadPtr = &srcRead;
    }
    if (dstWrotePtr == NULL) {
	dstWrotePtr = &dstWrote;
    }
    if (dstCharsPtr == NULL) {
	dstCharsPtr = &dstChars;
    }

    /*
     * If there are any null characters in the middle of the buffer, they will
     * converted to the UTF-8 null character (\xC080).  To get the actual 
     * \0 at the end of the destination buffer, we need to append it manually.
     */

    dstLen--;
    result = (*encodingPtr->toUtfProc)(encodingPtr->clientData, src, srcLen,
	    flags, statePtr, dst, dstLen, srcReadPtr, dstWrotePtr,
	    dstCharsPtr);
    dst[*dstWrotePtr] = '\0';
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_UtfToExternalDString --
 *
 *	Convert a source buffer from UTF-8 into the specified encoding.
 *	If any of the bytes in the source buffer are invalid or cannot
 *	be represented in the target encoding, a default fallback
 *	character will be substituted.
 *
 * Results:
 *	The converted bytes are stored in the DString, which is then
 *	NULL terminated in an encoding-specific manner.  The return value 
 *	is a pointer to the value stored in the DString.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

char *
Tcl_UtfToExternalDString(encoding, src, srcLen, dstPtr)
    Tcl_Encoding encoding;	/* The encoding for the converted string,
				 * or NULL for the default system encoding. */
    CONST char *src;		/* Source string in UTF-8. */
    int srcLen;			/* Source string length in bytes, or < 0 for
				 * strlen(). */
    Tcl_DString *dstPtr;	/* Uninitialized or free DString in which 
				 * the converted string is stored. */
{
    char *dst;
    Tcl_EncodingState state;
    Encoding *encodingPtr;
    int flags, dstLen, result, soFar, srcRead, dstWrote, dstChars;
    
    Tcl_DStringInit(dstPtr);
    dst = Tcl_DStringValue(dstPtr);
    dstLen = dstPtr->spaceAvl - 1;

    if (encoding == NULL) {
	encoding = systemEncoding;
    }
    encodingPtr = (Encoding *) encoding;

    if (src == NULL) {
	srcLen = 0;
    } else if (srcLen < 0) {
	srcLen = strlen(src);
    }
    flags = TCL_ENCODING_START | TCL_ENCODING_END;
    while (1) {
	result = (*encodingPtr->fromUtfProc)(encodingPtr->clientData, src,
		srcLen, flags, &state, dst, dstLen, &srcRead, &dstWrote,
		&dstChars);
	soFar = dst + dstWrote - Tcl_DStringValue(dstPtr);
	if (result != TCL_CONVERT_NOSPACE) {
	    if (encodingPtr->nullSize == 2) {
	        Tcl_DStringSetLength(dstPtr, soFar + 1);
	    }
	    Tcl_DStringSetLength(dstPtr, soFar);
	    return Tcl_DStringValue(dstPtr);
	}
	flags &= ~TCL_ENCODING_START;
	src += srcRead;
	srcLen -= srcRead;
	if (Tcl_DStringLength(dstPtr) == 0) {
	    Tcl_DStringSetLength(dstPtr, dstLen);
	}
	Tcl_DStringSetLength(dstPtr, 2 * Tcl_DStringLength(dstPtr) + 1);
	dst = Tcl_DStringValue(dstPtr) + soFar;
	dstLen = Tcl_DStringLength(dstPtr) - soFar - 1;
    }
}

/*
 *-------------------------------------------------------------------------
 *
 * Tcl_UtfToExternal --
 *
 *	Convert a buffer from UTF-8 into the specified encoding.
 *
 * Results:
 *	The return value is one of TCL_OK, TCL_CONVERT_MULTIBYTE,
 *	TCL_CONVERT_SYNTAX, TCL_CONVERT_UNKNOWN, or TCL_CONVERT_NOSPACE,
 *	as documented in tcl.h.
 *
 * Side effects:
 *	The converted bytes are stored in the output buffer.  
 *
 *-------------------------------------------------------------------------
 */

int
Tcl_UtfToExternal(interp, encoding, src, srcLen, flags, statePtr, dst,
	dstLen, srcReadPtr, dstWrotePtr, dstCharsPtr)
    Tcl_Interp *interp;		/* Interp for error return, if not NULL. */
    Tcl_Encoding encoding;	/* The encoding for the converted string,
				 * or NULL for the default system encoding. */
    CONST char *src;		/* Source string in UTF-8. */
    int srcLen;			/* Source string length in bytes, or < 0 for
				 * strlen(). */
    int flags;			/* Conversion control flags. */
    Tcl_EncodingState *statePtr;/* Place for conversion routine to store
				 * state information used during a piecewise
				 * conversion.  Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst;			/* Output buffer in which converted string
				 * is stored. */
    int dstLen;			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr;		/* Filled with the number of bytes from the
				 * source string that were converted.  This
				 * may be less than the original source length
				 * if there was a problem converting some
				 * source characters. */
    int *dstWrotePtr;		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr;		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    Encoding *encodingPtr;
    int result, srcRead, dstWrote, dstChars;
    Tcl_EncodingState state;
    
    if (encoding == NULL) {
	encoding = systemEncoding;
    }
    encodingPtr = (Encoding *) encoding;

    if (src == NULL) {
	srcLen = 0;
    } else if (srcLen < 0) {
	srcLen = strlen(src);
    }
    if (statePtr == NULL) {
	flags |= TCL_ENCODING_START | TCL_ENCODING_END;
	statePtr = &state;
    }
    if (srcReadPtr == NULL) {
	srcReadPtr = &srcRead;
    }
    if (dstWrotePtr == NULL) {
	dstWrotePtr = &dstWrote;
    }
    if (dstCharsPtr == NULL) {
	dstCharsPtr = &dstChars;
    }

    dstLen -= encodingPtr->nullSize;
    result = (*encodingPtr->fromUtfProc)(encodingPtr->clientData, src, srcLen,
	    flags, statePtr, dst, dstLen, srcReadPtr, dstWrotePtr,
	    dstCharsPtr);
    if (encodingPtr->nullSize == 2) {
	dst[*dstWrotePtr + 1] = '\0';
    }
    dst[*dstWrotePtr] = '\0';
    
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_FindExecutable --
 *
 *	This procedure computes the absolute path name of the current
 *	application, given its argv[0] value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The variable tclExecutableName gets filled in with the file
 *	name for the application, if we figured it out.  If we couldn't
 *	figure it out, tclExecutableName is set to NULL.
 *
 *---------------------------------------------------------------------------
 */

void
Tcl_FindExecutable(argv0)
    CONST char *argv0;		/* The value of the application's argv[0]
				 * (native). */
{
    CONST char *name;
    Tcl_DString buffer, nameString;

    TclInitSubsystems(argv0);

    if (argv0 == NULL) {
	goto done;
    }
    if (tclExecutableName != NULL) {
	ckfree(tclExecutableName);
	tclExecutableName = NULL;
    }
    if ((name = TclpFindExecutable(argv0)) == NULL) {
	goto done;
    }

    /*
     * The value returned from TclpNameOfExecutable is a UTF string that
     * is possibly dirty depending on when it was initialized.  To assure
     * that the UTF string is a properly encoded native string for this
     * system, convert the UTF string to the default native encoding
     * before the default encoding is initialized.  Then, convert it back
     * to UTF after the system encoding is loaded.
     */
    
    Tcl_UtfToExternalDString(NULL, name, -1, &buffer);
    TclFindEncodings(argv0);

    /*
     * Now it is OK to convert the native string back to UTF and set
     * the value of the tclExecutableName.
     */
    
    Tcl_ExternalToUtfDString(NULL, Tcl_DStringValue(&buffer), -1, &nameString);
    tclExecutableName = (char *)
	ckalloc((unsigned) (Tcl_DStringLength(&nameString) + 1));
    strcpy(tclExecutableName, Tcl_DStringValue(&nameString));

    Tcl_DStringFree(&buffer);
    Tcl_DStringFree(&nameString);
    return;
	
    done:
    TclFindEncodings(argv0);
}

/*
 *---------------------------------------------------------------------------
 *
 * LoadEncodingFile --
 *
 *	Read a file that describes an encoding and create a new Encoding
 *	from the data.  
 *
 * Results:
 *	The return value is the newly loaded Encoding, or NULL if
 *	the file didn't exist of was in the incorrect format.  If NULL was
 *	returned, an error message is left in interp's result object,
 *	unless interp was NULL.
 *
 * Side effects:
 *	File read from disk.  
 *
 *---------------------------------------------------------------------------
 */

static Tcl_Encoding
LoadEncodingFile(interp, name)
    Tcl_Interp *interp;		/* Interp for error reporting, if not NULL. */
    CONST char *name;		/* The name of the encoding file on disk
				 * and also the name for new encoding. */
{
    int objc, i, ch;
    Tcl_Obj **objv;
    Tcl_Obj *pathPtr;
    Tcl_Channel chan;
    Tcl_Encoding encoding;

    pathPtr = TclGetLibraryPath();
    if (pathPtr == NULL) {
	goto unknown;
    }
    objc = 0;
    Tcl_ListObjGetElements(NULL, pathPtr, &objc, &objv);

    chan = NULL;
    for (i = 0; i < objc; i++) {
	chan = OpenEncodingFile(Tcl_GetString(objv[i]), name);
	if (chan != NULL) {
	    break;
	}
    }

    if (chan == NULL) {
	goto unknown;
    }

    Tcl_SetChannelOption(NULL, chan, "-encoding", "utf-8");

    while (1) {
	Tcl_DString ds;

	Tcl_DStringInit(&ds);
	Tcl_Gets(chan, &ds);
	ch = Tcl_DStringValue(&ds)[0];
	Tcl_DStringFree(&ds);
	if (ch != '#') {
	    break;
	}
    }

    encoding = NULL;
    switch (ch) {
	case 'S': {
	    encoding = LoadTableEncoding(interp, name, ENCODING_SINGLEBYTE,
		    chan);
	    break;
	}
	case 'D': {
	    encoding = LoadTableEncoding(interp, name, ENCODING_DOUBLEBYTE,
		    chan);
	    break;
	}
	case 'M': {
	    encoding = LoadTableEncoding(interp, name, ENCODING_MULTIBYTE,
		    chan);
	    break;
	}
	case 'E': {
	    encoding = LoadEscapeEncoding(name, chan);
	    break;
	}
    }
    if ((encoding == NULL) && (interp != NULL)) {
	Tcl_AppendResult(interp, "invalid encoding file \"", name, "\"", NULL);
    }
    Tcl_Close(NULL, chan);
    return encoding;

    unknown:
    if (interp != NULL) {
	Tcl_AppendResult(interp, "unknown encoding \"", name, "\"", NULL);
    }
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * OpenEncodingFile --
 *
 *	Look for the file encoding/<name>.enc in the specified
 *	directory.
 *
 * Results:
 *	Returns an open file channel if the file exists.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Channel
OpenEncodingFile(dir, name)
    CONST char *dir;
    CONST char *name;

{
    char *argv[3];
    Tcl_DString pathString;
    char *path;
    Tcl_Channel chan;
    
    argv[0] = (char *) dir;
    argv[1] = "encoding";
    argv[2] = (char *) name;

    Tcl_DStringInit(&pathString);
    Tcl_JoinPath(3, argv, &pathString);
    path = Tcl_DStringAppend(&pathString, ".enc", -1);
    chan = Tcl_OpenFileChannel(NULL, path, "r", 0);
    Tcl_DStringFree(&pathString);

    return chan;
}

/*
 *-------------------------------------------------------------------------
 *
 * LoadTableEncoding --
 *
 *	Helper function for LoadEncodingTable().  Loads a table to that 
 *	converts between Unicode and some other encoding and creates an 
 *	encoding (using a TableEncoding structure) from that information.
 *
 *	File contains binary data, but begins with a marker to indicate 
 *	byte-ordering, so that same binary file can be read on either
 *	endian platforms.
 *
 * Results:
 *	The return value is the new encoding, or NULL if the encoding 
 *	could not be created (because the file contained invalid data).
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static Tcl_Encoding
LoadTableEncoding(interp, name, type, chan)
    Tcl_Interp *interp;		/* Interp for temporary obj while reading. */
    CONST char *name;		/* Name for new encoding. */
    int type;			/* Type of encoding (ENCODING_?????). */
    Tcl_Channel chan;		/* File containing new encoding. */
{
    Tcl_DString lineString;
    Tcl_Obj *objPtr;
    char *line;
    int i, hi, lo, numPages, symbol, fallback;
    unsigned char used[256];
    unsigned int size;
    TableEncodingData *dataPtr;
    unsigned short *pageMemPtr;
    Tcl_EncodingType encType;
    char *hex;
    static char staticHex[] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0, 0, 0,
	10, 11, 12, 13, 14, 15
    };

    hex = staticHex - '0';

    Tcl_DStringInit(&lineString);
    Tcl_Gets(chan, &lineString);
    line = Tcl_DStringValue(&lineString);

    fallback = (int) strtol(line, &line, 16);
    symbol = (int) strtol(line, &line, 10);
    numPages = (int) strtol(line, &line, 10);
    Tcl_DStringFree(&lineString);

    if (numPages < 0) {
	numPages = 0;
    } else if (numPages > 256) {
	numPages = 256;
    }

    memset(used, 0, sizeof(used));

#undef PAGESIZE
#define PAGESIZE    (256 * sizeof(unsigned short))

    dataPtr = (TableEncodingData *) ckalloc(sizeof(TableEncodingData));
    memset(dataPtr, 0, sizeof(TableEncodingData));

    dataPtr->fallback = fallback;

    /*
     * Read the table that maps characters to Unicode.  Performs a single
     * malloc to get the memory for the array and all the pages needed by
     * the array.
     */

    size = 256 * sizeof(unsigned short *) + numPages * PAGESIZE;
    dataPtr->toUnicode = (unsigned short **) ckalloc(size);
    memset(dataPtr->toUnicode, 0, size);
    pageMemPtr = (unsigned short *) (dataPtr->toUnicode + 256);

    if (interp == NULL) {
	objPtr = Tcl_NewObj();
    } else {
	objPtr = Tcl_GetObjResult(interp);
    }
    for (i = 0; i < numPages; i++) {
	int ch;
	char *p;

	Tcl_ReadChars(chan, objPtr, 3 + 16 * (16 * 4 + 1), 0);
	p = Tcl_GetString(objPtr);
	hi = (hex[(int)p[0]] << 4) + hex[(int)p[1]];
	dataPtr->toUnicode[hi] = pageMemPtr;
	p += 2;
	for (lo = 0; lo < 256; lo++) {
	    if ((lo & 0x0f) == 0) {
		p++;
	    }
	    ch = (hex[(int)p[0]] << 12) + (hex[(int)p[1]] << 8)
		+ (hex[(int)p[2]] << 4) + hex[(int)p[3]];
	    if (ch != 0) {
		used[ch >> 8] = 1;
	    }
	    *pageMemPtr = (unsigned short) ch;
	    pageMemPtr++;
	    p += 4;
	}
    }
    if (interp == NULL) {
	Tcl_DecrRefCount(objPtr);
    } else {
	Tcl_ResetResult(interp);
    }
	
    if (type == ENCODING_DOUBLEBYTE) {
	memset(dataPtr->prefixBytes, 1, sizeof(dataPtr->prefixBytes));
    } else {
	for (hi = 1; hi < 256; hi++) {
	    if (dataPtr->toUnicode[hi] != NULL) {
		dataPtr->prefixBytes[hi] = 1;
	    }
	}
    }

    /*
     * Invert toUnicode array to produce the fromUnicode array.  Performs a
     * single malloc to get the memory for the array and all the pages
     * needed by the array.  While reading in the toUnicode array, we
     * remembered what pages that would be needed for the fromUnicode array.
     */

    if (symbol) {
	used[0] = 1;
    }
    numPages = 0;
    for (hi = 0; hi < 256; hi++) {
	if (used[hi]) {
	    numPages++;
	}
    }
    size = 256 * sizeof(unsigned short *) + numPages * PAGESIZE;
    dataPtr->fromUnicode = (unsigned short **) ckalloc(size);
    memset(dataPtr->fromUnicode, 0, size);
    pageMemPtr = (unsigned short *) (dataPtr->fromUnicode + 256);

    for (hi = 0; hi < 256; hi++) {
	if (dataPtr->toUnicode[hi] == NULL) {
	    dataPtr->toUnicode[hi] = emptyPage;
	} else {
	    for (lo = 0; lo < 256; lo++) {
		int ch;

		ch = dataPtr->toUnicode[hi][lo];
		if (ch != 0) {
		    unsigned short *page;
		    
		    page = dataPtr->fromUnicode[ch >> 8];
		    if (page == NULL) {
			page = pageMemPtr;
			pageMemPtr += 256;
			dataPtr->fromUnicode[ch >> 8] = page;
		    }
		    page[ch & 0xff] = (unsigned short) ((hi << 8) + lo);
		}
	    }
	}
    }
    if (type == ENCODING_MULTIBYTE) {
	/*
	 * If multibyte encodings don't have a backslash character, define
	 * one.  Otherwise, on Windows, native file names won't work because
	 * the backslash in the file name will map to the unknown character
	 * (question mark) when converting from UTF-8 to external encoding.
	 */

	if (dataPtr->fromUnicode[0] != NULL) {
	    if (dataPtr->fromUnicode[0]['\\'] == '\0') {
		dataPtr->fromUnicode[0]['\\'] = '\\';
	    }
	}
    }
    if (symbol) {
	unsigned short *page;
	
	/*
	 * Make a special symbol encoding that not only maps the symbol
	 * characters from their Unicode code points down into page 0, but
	 * also ensure that the characters on page 0 map to themselves.
	 * This is so that a symbol font can be used to display a simple
	 * string like "abcd" and have alpha, beta, chi, delta show up,
	 * rather than have "unknown" chars show up because strictly
	 * speaking the symbol font doesn't have glyphs for those low ascii
	 * chars.
	 */

	page = dataPtr->fromUnicode[0];
	if (page == NULL) {
	    page = pageMemPtr;
	    dataPtr->fromUnicode[0] = page;
	}
	for (lo = 0; lo < 256; lo++) {
	    if (dataPtr->toUnicode[0][lo] != 0) {
		page[lo] = (unsigned short) lo;
	    }
	}
    }
    for (hi = 0; hi < 256; hi++) {
	if (dataPtr->fromUnicode[hi] == NULL) {
	    dataPtr->fromUnicode[hi] = emptyPage;
	}
    }
    encType.encodingName    = name;
    encType.toUtfProc	    = TableToUtfProc;
    encType.fromUtfProc	    = TableFromUtfProc;
    encType.freeProc	    = TableFreeProc;
    encType.nullSize	    = (type == ENCODING_DOUBLEBYTE) ? 2 : 1;
    encType.clientData	    = (ClientData) dataPtr;
    return Tcl_CreateEncoding(&encType);

}

/*
 *-------------------------------------------------------------------------
 *
 * LoadEscapeEncoding --
 *
 *	Helper function for LoadEncodingTable().  Loads a state machine
 *	that converts between Unicode and some other encoding.  
 *
 *	File contains text data that describes the escape sequences that
 *	are used to choose an encoding and the associated names for the 
 *	sub-encodings.
 *
 * Results:
 *	The return value is the new encoding, or NULL if the encoding 
 *	could not be created (because the file contained invalid data).
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static Tcl_Encoding
LoadEscapeEncoding(name, chan)
    CONST char *name;		/* Name for new encoding. */
    Tcl_Channel chan;		/* File containing new encoding. */
{
    int i;
    unsigned int size;
    Tcl_DString escapeData;
    char init[16], final[16];
    EscapeEncodingData *dataPtr;
    Tcl_EncodingType type;

    init[0] = '\0';
    final[0] = '\0';
    Tcl_DStringInit(&escapeData);

    while (1) {
	int argc;
	char **argv;
	char *line;
	Tcl_DString lineString;
	
	Tcl_DStringInit(&lineString);
	if (Tcl_Gets(chan, &lineString) < 0) {
	    break;
	}
	line = Tcl_DStringValue(&lineString);
        if (Tcl_SplitList(NULL, line, &argc, &argv) != TCL_OK) {
	    continue;
	}
	if (argc >= 2) {
	    if (strcmp(argv[0], "name") == 0) {
		;
	    } else if (strcmp(argv[0], "init") == 0) {
		strncpy(init, argv[1], sizeof(init));
		init[sizeof(init) - 1] = '\0';
	    } else if (strcmp(argv[0], "final") == 0) {
		strncpy(final, argv[1], sizeof(final));
		final[sizeof(final) - 1] = '\0';
	    } else {
		EscapeSubTable est;

		strncpy(est.sequence, argv[1], sizeof(est.sequence));
		est.sequence[sizeof(est.sequence) - 1] = '\0';
		est.sequenceLen = strlen(est.sequence);

		strncpy(est.name, argv[0], sizeof(est.name));
		est.name[sizeof(est.name) - 1] = '\0';

		est.encodingPtr = NULL;
		Tcl_DStringAppend(&escapeData, (char *) &est, sizeof(est));
	    }
	}
	ckfree((char *) argv);
	Tcl_DStringFree(&lineString);
    }

    size = sizeof(EscapeEncodingData)
	    - sizeof(EscapeSubTable) + Tcl_DStringLength(&escapeData);
    dataPtr = (EscapeEncodingData *) ckalloc(size);
    dataPtr->initLen = strlen(init);
    strcpy(dataPtr->init, init);
    dataPtr->finalLen = strlen(final);
    strcpy(dataPtr->final, final);
    dataPtr->numSubTables = Tcl_DStringLength(&escapeData) / sizeof(EscapeSubTable);
    memcpy((VOID *) dataPtr->subTables, (VOID *) Tcl_DStringValue(&escapeData),
	    (size_t) Tcl_DStringLength(&escapeData));
    Tcl_DStringFree(&escapeData);

    memset(dataPtr->prefixBytes, 0, sizeof(dataPtr->prefixBytes));
    for (i = 0; i < dataPtr->numSubTables; i++) {
	dataPtr->prefixBytes[UCHAR(dataPtr->subTables[i].sequence[0])] = 1;
    }
    if (dataPtr->init[0] != '\0') {
	dataPtr->prefixBytes[UCHAR(dataPtr->init[0])] = 1;
    }
    if (dataPtr->final[0] != '\0') {
	dataPtr->prefixBytes[UCHAR(dataPtr->final[0])] = 1;
    }

    type.encodingName	= name;
    type.toUtfProc	= EscapeToUtfProc;
    type.fromUtfProc    = EscapeFromUtfProc;
    type.freeProc	= EscapeFreeProc;
    type.nullSize	= 1;
    type.clientData	= (ClientData) dataPtr;

    return Tcl_CreateEncoding(&type);
}

/*
 *-------------------------------------------------------------------------
 *
 * BinaryProc --
 *
 *	The default conversion when no other conversion is specified.
 *	No translation is done; source bytes are copied directly to 
 *	destination bytes.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int
BinaryProc(clientData, src, srcLen, flags, statePtr, dst, dstLen,
	srcReadPtr, dstWrotePtr, dstCharsPtr)
    ClientData clientData;	/* Not used. */
    CONST char *src;		/* Source string (unknown encoding). */
    int srcLen;			/* Source string length in bytes. */
    int flags;			/* Conversion control flags. */
    Tcl_EncodingState *statePtr;/* Place for conversion routine to store
				 * state information used during a piecewise
				 * conversion.  Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst;			/* Output buffer in which converted string
				 * is stored. */
    int dstLen;			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr;		/* Filled with the number of bytes from the
				 * source string that were converted. */
    int *dstWrotePtr;		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr;		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    int result;

    result = TCL_OK;
    dstLen -= TCL_UTF_MAX - 1;
    if (dstLen < 0) {
	dstLen = 0;
    }
    if (srcLen > dstLen) {
	srcLen = dstLen;
	result = TCL_CONVERT_NOSPACE;
    }

    *srcReadPtr = srcLen;
    *dstWrotePtr = srcLen;
    *dstCharsPtr = srcLen;
    for ( ; --srcLen >= 0; ) {
	*dst++ = *src++;
    }
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * UtfToUtfProc --
 *
 *	Convert from UTF-8 to UTF-8.  Note that the UTF-8 to UTF-8 
 *	translation is not a no-op, because it will turn a stream of
 *	improperly formed UTF-8 into a properly formed stream.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int 
UtfToUtfProc(clientData, src, srcLen, flags, statePtr, dst, dstLen,
	srcReadPtr, dstWrotePtr, dstCharsPtr)
    ClientData clientData;	/* Not used. */
    CONST char *src;		/* Source string in UTF-8. */
    int srcLen;			/* Source string length in bytes. */
    int flags;			/* Conversion control flags. */
    Tcl_EncodingState *statePtr;/* Place for conversion routine to store
				 * state information used during a piecewise
				 * conversion.  Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst;			/* Output buffer in which converted string
				 * is stored. */
    int dstLen;			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr;		/* Filled with the number of bytes from the
				 * source string that were converted.  This
				 * may be less than the original source length
				 * if there was a problem converting some
				 * source characters. */
    int *dstWrotePtr;		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr;		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    CONST char *srcStart, *srcEnd, *srcClose;
    char *dstStart, *dstEnd;
    int result, numChars;
    Tcl_UniChar ch;

    result = TCL_OK;
    
    srcStart = src;
    srcEnd = src + srcLen;
    srcClose = srcEnd;
    if ((flags & TCL_ENCODING_END) == 0) {
	srcClose -= TCL_UTF_MAX;
    }

    dstStart = dst;
    dstEnd = dst + dstLen - TCL_UTF_MAX;

    for (numChars = 0; src < srcEnd; numChars++) {
	if ((src > srcClose) && (!Tcl_UtfCharComplete(src, srcEnd - src))) {
	    /*
	     * If there is more string to follow, this will ensure that the
	     * last UTF-8 character in the source buffer hasn't been cut off.
	     */

	    result = TCL_CONVERT_MULTIBYTE;
	    break;
	}
	if (dst > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
	}
	src += Tcl_UtfToUniChar(src, &ch);
	dst += Tcl_UniCharToUtf(ch, dst);
    }

    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * UnicodeToUtfProc --
 *
 *	Convert from Unicode to UTF-8.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int 
UnicodeToUtfProc(clientData, src, srcLen, flags, statePtr, dst, dstLen,
	srcReadPtr, dstWrotePtr, dstCharsPtr)
    ClientData clientData;	/* Not used. */
    CONST char *src;		/* Source string in Unicode. */
    int srcLen;			/* Source string length in bytes. */
    int flags;			/* Conversion control flags. */
    Tcl_EncodingState *statePtr;/* Place for conversion routine to store
				 * state information used during a piecewise
				 * conversion.  Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst;			/* Output buffer in which converted string
				 * is stored. */
    int dstLen;			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr;		/* Filled with the number of bytes from the
				 * source string that were converted.  This
				 * may be less than the original source length
				 * if there was a problem converting some
				 * source characters. */
    int *dstWrotePtr;		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr;		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    CONST Tcl_UniChar *wSrc, *wSrcStart, *wSrcEnd;
    char *dstEnd, *dstStart;
    int result, numChars;
    
    result = TCL_OK;
    if ((srcLen % sizeof(Tcl_UniChar)) != 0) {
	result = TCL_CONVERT_MULTIBYTE;
	srcLen /= sizeof(Tcl_UniChar);
	srcLen *= sizeof(Tcl_UniChar);
    }

    wSrc = (Tcl_UniChar *) src;

    wSrcStart = (Tcl_UniChar *) src;
    wSrcEnd = (Tcl_UniChar *) (src + srcLen);

    dstStart = dst;
    dstEnd = dst + dstLen - TCL_UTF_MAX;

    for (numChars = 0; wSrc < wSrcEnd; numChars++) {
	if (dst > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
	}
	dst += Tcl_UniCharToUtf(*wSrc, dst);
	wSrc++;
    }

    *srcReadPtr = (char *) wSrc - (char *) wSrcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * UtfToUnicodeProc --
 *
 *	Convert from UTF-8 to Unicode.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int 
UtfToUnicodeProc(clientData, src, srcLen, flags, statePtr, dst, dstLen,
	srcReadPtr, dstWrotePtr, dstCharsPtr)
    ClientData clientData;	/* TableEncodingData that specifies encoding. */
    CONST char *src;		/* Source string in UTF-8. */
    int srcLen;			/* Source string length in bytes. */
    int flags;			/* Conversion control flags. */
    Tcl_EncodingState *statePtr;/* Place for conversion routine to store
				 * state information used during a piecewise
				 * conversion.  Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst;			/* Output buffer in which converted string
				 * is stored. */
    int dstLen;			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr;		/* Filled with the number of bytes from the
				 * source string that were converted.  This
				 * may be less than the original source length
				 * if there was a problem converting some
				 * source characters. */
    int *dstWrotePtr;		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr;		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    CONST char *srcStart, *srcEnd, *srcClose;
    Tcl_UniChar *wDst, *wDstStart, *wDstEnd;
    int result, numChars;
    
    srcStart = src;
    srcEnd = src + srcLen;
    srcClose = srcEnd;
    if ((flags & TCL_ENCODING_END) == 0) {
	srcClose -= TCL_UTF_MAX;
    }

    wDst = (Tcl_UniChar *) dst;
    wDstStart = (Tcl_UniChar *) dst;
    wDstEnd = (Tcl_UniChar *) (dst + dstLen - sizeof(Tcl_UniChar));

    result = TCL_OK;
    for (numChars = 0; src < srcEnd; numChars++) {
	if ((src > srcClose) && (!Tcl_UtfCharComplete(src, srcEnd - src))) {
	    /*
	     * If there is more string to follow, this will ensure that the
	     * last UTF-8 character in the source buffer hasn't been cut off.
	     */

	    result = TCL_CONVERT_MULTIBYTE;
	    break;
	}
	if (wDst > wDstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	    break;
        }
	src += Tcl_UtfToUniChar(src, wDst);
	wDst++;
    }
    *srcReadPtr = src - srcStart;
    *dstWrotePtr = (char *) wDst - (char *) wDstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * TableToUtfProc --
 *
 *	Convert from the encoding specified by the TableEncodingData into
 *	UTF-8.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int 
TableToUtfProc(clientData, src, srcLen, flags, statePtr, dst, dstLen,
	srcReadPtr, dstWrotePtr, dstCharsPtr)
    ClientData clientData;	/* TableEncodingData that specifies
				 * encoding. */
    CONST char *src;		/* Source string in specified encoding. */
    int srcLen;			/* Source string length in bytes. */
    int flags;			/* Conversion control flags. */
    Tcl_EncodingState *statePtr;/* Place for conversion routine to store
				 * state information used during a piecewise
				 * conversion.  Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst;			/* Output buffer in which converted string
				 * is stored. */
    int dstLen;			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr;		/* Filled with the number of bytes from the
				 * source string that were converted.  This
				 * may be less than the original source length
				 * if there was a problem converting some
				 * source characters. */
    int *dstWrotePtr;		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr;		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    CONST char *srcStart, *srcEnd;
    char *dstEnd, *dstStart, *prefixBytes;
    int result, byte, numChars;
    Tcl_UniChar ch;
    unsigned short **toUnicode;
    unsigned short *pageZero;
    TableEncodingData *dataPtr;
    
    srcStart = src;
    srcEnd = src + srcLen;

    dstStart = dst;
    dstEnd = dst + dstLen - TCL_UTF_MAX;

    dataPtr = (TableEncodingData *) clientData;
    toUnicode = dataPtr->toUnicode;
    prefixBytes = dataPtr->prefixBytes;
    pageZero = toUnicode[0];

    result = TCL_OK;
    for (numChars = 0; src < srcEnd; numChars++) {
        if (dst > dstEnd) {
            result = TCL_CONVERT_NOSPACE;
            break;
        }
	byte = *((unsigned char *) src);
	if (prefixBytes[byte]) {
	    src++;
	    if (src >= srcEnd) {
		src--;
		result = TCL_CONVERT_MULTIBYTE;
		break;
	    }
	    ch = toUnicode[byte][*((unsigned char *) src)];
	} else {
	    ch = pageZero[byte];
	}
	if ((ch == 0) && (byte != 0)) {
	    if (flags & TCL_ENCODING_STOPONERROR) {
		result = TCL_CONVERT_SYNTAX;
		break;
	    }
	    if (prefixBytes[byte]) {
		src--;
	    }
	    ch = (Tcl_UniChar) byte;
	}
	dst += Tcl_UniCharToUtf(ch, dst);
        src++;
    }
    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * TableFromUtfProc --
 *
 *	Convert from UTF-8 into the encoding specified by the
 *	TableEncodingData.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int 
TableFromUtfProc(clientData, src, srcLen, flags, statePtr, dst, dstLen,
	srcReadPtr, dstWrotePtr, dstCharsPtr)
    ClientData clientData;	/* TableEncodingData that specifies
				 * encoding. */
    CONST char *src;		/* Source string in UTF-8. */
    int srcLen;			/* Source string length in bytes. */
    int flags;			/* Conversion control flags. */
    Tcl_EncodingState *statePtr;/* Place for conversion routine to store
				 * state information used during a piecewise
				 * conversion.  Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst;			/* Output buffer in which converted string
				 * is stored. */
    int dstLen;			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr;		/* Filled with the number of bytes from the
				 * source string that were converted.  This
				 * may be less than the original source length
				 * if there was a problem converting some
				 * source characters. */
    int *dstWrotePtr;		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr;		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    CONST char *srcStart, *srcEnd, *srcClose;
    char *dstStart, *dstEnd, *prefixBytes;
    Tcl_UniChar ch;
    int result, len, word, numChars;
    TableEncodingData *dataPtr;
    unsigned short **fromUnicode;
    
    result = TCL_OK;    

    dataPtr = (TableEncodingData *) clientData;
    prefixBytes = dataPtr->prefixBytes;
    fromUnicode = dataPtr->fromUnicode;
    
    srcStart = src;
    srcEnd = src + srcLen;
    srcClose = srcEnd;
    if ((flags & TCL_ENCODING_END) == 0) {
	srcClose -= TCL_UTF_MAX;
    }

    dstStart = dst;
    dstEnd = dst + dstLen - 1;

    for (numChars = 0; src < srcEnd; numChars++) {
	if ((src > srcClose) && (!Tcl_UtfCharComplete(src, srcEnd - src))) {
	    /*
	     * If there is more string to follow, this will ensure that the
	     * last UTF-8 character in the source buffer hasn't been cut off.
	     */

	    result = TCL_CONVERT_MULTIBYTE;
	    break;
	}
        len = Tcl_UtfToUniChar(src, &ch);
	word = fromUnicode[(ch >> 8)][ch & 0xff];
	if ((word == 0) && (ch != 0)) {
	    if (flags & TCL_ENCODING_STOPONERROR) {
		result = TCL_CONVERT_UNKNOWN;
		break;
	    }
	    word = dataPtr->fallback; 
	}
	if (prefixBytes[(word >> 8)] != 0) {
	    if (dst + 1 > dstEnd) {
		result = TCL_CONVERT_NOSPACE;
		break;
	    }
	    dst[0] = (char) (word >> 8);
	    dst[1] = (char) word;
	    dst += 2;
	} else {
	    if (dst > dstEnd) {
		result = TCL_CONVERT_NOSPACE;
		break;
	    }
	    dst[0] = (char) word;
	    dst++;
	} 
	src += len;
    }
    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * TableFreeProc --
 *
 *	This procedure is invoked when an encoding is deleted.  It deletes
 *	the memory used by the TableEncodingData.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

static void
TableFreeProc(clientData)
    ClientData clientData;	/* TableEncodingData that specifies
				 * encoding. */
{
    TableEncodingData *dataPtr;

    dataPtr = (TableEncodingData *) clientData;
    ckfree((char *) dataPtr->toUnicode);
    ckfree((char *) dataPtr->fromUnicode);
    ckfree((char *) dataPtr);
}

/*
 *-------------------------------------------------------------------------
 *
 * EscapeToUtfProc --
 *
 *	Convert from the encoding specified by the EscapeEncodingData into
 *	UTF-8.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int 
EscapeToUtfProc(clientData, src, srcLen, flags, statePtr, dst, dstLen,
	srcReadPtr, dstWrotePtr, dstCharsPtr)
    ClientData clientData;	/* EscapeEncodingData that specifies
				 * encoding. */
    CONST char *src;		/* Source string in specified encoding. */
    int srcLen;			/* Source string length in bytes. */
    int flags;			/* Conversion control flags. */
    Tcl_EncodingState *statePtr;/* Place for conversion routine to store
				 * state information used during a piecewise
				 * conversion.  Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst;			/* Output buffer in which converted string
				 * is stored. */
    int dstLen;			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr;		/* Filled with the number of bytes from the
				 * source string that were converted.  This
				 * may be less than the original source length
				 * if there was a problem converting some
				 * source characters. */
    int *dstWrotePtr;		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr;		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    EscapeEncodingData *dataPtr;
    char *prefixBytes, *tablePrefixBytes;
    unsigned short **tableToUnicode;
    Encoding *encodingPtr;
    int state, result, numChars;
    CONST char *srcStart, *srcEnd;
    char *dstStart, *dstEnd;

    result = TCL_OK;

    tablePrefixBytes = NULL;	/* lint. */
    tableToUnicode = NULL;	/* lint. */

    dataPtr = (EscapeEncodingData *) clientData;
    prefixBytes = dataPtr->prefixBytes;
    encodingPtr = NULL;

    srcStart = src;
    srcEnd = src + srcLen;

    dstStart = dst;
    dstEnd = dst + dstLen - TCL_UTF_MAX;

    state = (int) *statePtr;
    if (flags & TCL_ENCODING_START) {
	state = 0;
    }

    for (numChars = 0; src < srcEnd; ) {
	int byte, hi, lo, ch;

        if (dst > dstEnd) {
            result = TCL_CONVERT_NOSPACE;
            break;
        }
	byte = *((unsigned char *) src);
	if (prefixBytes[byte]) {
	    unsigned int left, len, longest;
	    int checked, i;
	    EscapeSubTable *subTablePtr;
	    
	    /*
	     * Saw the beginning of an escape sequence. 
	     */
	     
	    left = srcEnd - src;
	    len = dataPtr->initLen;
	    longest = len;
	    checked = 0;
	    if (len <= left) {
		checked++;
		if ((len > 0) && 
			(memcmp(src, dataPtr->init, len) == 0)) {
		    /*
		     * If we see initialization string, skip it, even if we're
		     * not at the beginning of the buffer. 
		     */
		     
		    src += len;
		    continue;
		}
	    }
	    len = dataPtr->finalLen;
	    if (len > longest) {
		longest = len;
	    }
	    if (len <= left) {
		checked++;
		if ((len > 0) && 
			(memcmp(src, dataPtr->final, len) == 0)) {
		    /*
		     * If we see finalization string, skip it, even if we're
		     * not at the end of the buffer. 
		     */
		     
		    src += len;
		    continue;
		}
	    }
	    subTablePtr = dataPtr->subTables;
	    for (i = 0; i < dataPtr->numSubTables; i++) {
		len = subTablePtr->sequenceLen;
		if (len > longest) {
		    longest = len;
		}
		if (len <= left) {
		    checked++;
		    if ((len > 0) && 
			    (memcmp(src, subTablePtr->sequence, len) == 0)) {
			state = i;
			encodingPtr = NULL;
			subTablePtr = NULL;
			src += len;
			break;
		    }
		}
		subTablePtr++;
	    }
	    if (subTablePtr == NULL) {
		/*
		 * A match was found, the escape sequence was consumed, and
		 * the state was updated.
		 */

		continue;
	    }

	    /*
	     * We have a split-up or unrecognized escape sequence.  If we
	     * checked all the sequences, then it's a syntax error,
	     * otherwise we need more bytes to determine a match.
	     */

	    if ((checked == dataPtr->numSubTables + 2)
		    || (flags & TCL_ENCODING_END)) {
		if ((flags & TCL_ENCODING_STOPONERROR) == 0) {
		    /*
		     * Skip the unknown escape sequence.
		     */

		    src += longest;
		    continue;
		}
		result = TCL_CONVERT_SYNTAX;
	    } else {
		result = TCL_CONVERT_MULTIBYTE;
	    }
	    break;
	}

	if (encodingPtr == NULL) {
	    TableEncodingData *tableDataPtr;

	    encodingPtr = GetTableEncoding(dataPtr, state);
	    tableDataPtr = (TableEncodingData *) encodingPtr->clientData;
	    tablePrefixBytes = tableDataPtr->prefixBytes;
	    tableToUnicode = tableDataPtr->toUnicode;
	}
	if (tablePrefixBytes[byte]) {
	    src++;
	    if (src >= srcEnd) {
		src--;
		result = TCL_CONVERT_MULTIBYTE;
		break;
	    }
	    hi = byte;
	    lo = *((unsigned char *) src);
	} else {
	    hi = 0;
	    lo = byte;
	}
	ch = tableToUnicode[hi][lo];
	dst += Tcl_UniCharToUtf(ch, dst);
	src++;
	numChars++;
    }

    *statePtr = (Tcl_EncodingState) state;
    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *-------------------------------------------------------------------------
 *
 * EscapeFromUtfProc --
 *
 *	Convert from UTF-8 into the encoding specified by the
 *	EscapeEncodingData.
 *
 * Results:
 *	Returns TCL_OK if conversion was successful.
 *
 * Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */

static int 
EscapeFromUtfProc(clientData, src, srcLen, flags, statePtr, dst, dstLen,
	srcReadPtr, dstWrotePtr, dstCharsPtr)
    ClientData clientData;	/* EscapeEncodingData that specifies
				 * encoding. */
    CONST char *src;		/* Source string in UTF-8. */
    int srcLen;			/* Source string length in bytes. */
    int flags;			/* Conversion control flags. */
    Tcl_EncodingState *statePtr;/* Place for conversion routine to store
				 * state information used during a piecewise
				 * conversion.  Contents of statePtr are
				 * initialized and/or reset by conversion
				 * routine under control of flags argument. */
    char *dst;			/* Output buffer in which converted string
				 * is stored. */
    int dstLen;			/* The maximum length of output buffer in
				 * bytes. */
    int *srcReadPtr;		/* Filled with the number of bytes from the
				 * source string that were converted.  This
				 * may be less than the original source length
				 * if there was a problem converting some
				 * source characters. */
    int *dstWrotePtr;		/* Filled with the number of bytes that were
				 * stored in the output buffer as a result of
				 * the conversion. */
    int *dstCharsPtr;		/* Filled with the number of characters that
				 * correspond to the bytes stored in the
				 * output buffer. */
{
    EscapeEncodingData *dataPtr;
    Encoding *encodingPtr;
    CONST char *srcStart, *srcEnd, *srcClose;
    char *dstStart, *dstEnd;
    int state, result, numChars;
    TableEncodingData *tableDataPtr;
    char *tablePrefixBytes;
    unsigned short **tableFromUnicode;
    
    result = TCL_OK;    

    dataPtr = (EscapeEncodingData *) clientData;

    srcStart = src;
    srcEnd = src + srcLen;
    srcClose = srcEnd;
    if ((flags & TCL_ENCODING_END) == 0) {
	srcClose -= TCL_UTF_MAX;
    }

    dstStart = dst;
    dstEnd = dst + dstLen - 1;

    if (flags & TCL_ENCODING_START) {
	unsigned int len;
	
	state = 0;
	len = dataPtr->subTables[0].sequenceLen;
	if (dst + dataPtr->initLen + len > dstEnd) {
	    *srcReadPtr = 0;
	    *dstWrotePtr = 0;
	    return TCL_CONVERT_NOSPACE;
	}
	memcpy((VOID *) dst, (VOID *) dataPtr->init,
		(size_t) dataPtr->initLen);
	dst += dataPtr->initLen;
	memcpy((VOID *) dst, (VOID *) dataPtr->subTables[0].sequence,
		(size_t) len);
	dst += len;
    } else {
        state = (int) *statePtr;
    }

    encodingPtr = GetTableEncoding(dataPtr, state);
    tableDataPtr = (TableEncodingData *) encodingPtr->clientData;
    tablePrefixBytes = tableDataPtr->prefixBytes;
    tableFromUnicode = tableDataPtr->fromUnicode;

    for (numChars = 0; src < srcEnd; numChars++) {
	unsigned int len;
	int word;
	Tcl_UniChar ch;
	
	if ((src > srcClose) && (!Tcl_UtfCharComplete(src, srcEnd - src))) {
	    /*
	     * If there is more string to follow, this will ensure that the
	     * last UTF-8 character in the source buffer hasn't been cut off.
	     */

	    result = TCL_CONVERT_MULTIBYTE;
	    break;
	}
        len = Tcl_UtfToUniChar(src, &ch);
	word = tableFromUnicode[(ch >> 8)][ch & 0xff];

	if ((word == 0) && (ch != 0)) {
	    int oldState;
	    EscapeSubTable *subTablePtr;
	    
	    oldState = state;
	    for (state = 0; state < dataPtr->numSubTables; state++) {
		encodingPtr = GetTableEncoding(dataPtr, state);
		tableDataPtr = (TableEncodingData *) encodingPtr->clientData;
	    	word = tableDataPtr->fromUnicode[(ch >> 8)][ch & 0xff];
		if (word != 0) {
		    break;
		}
	    }

	    if (word == 0) {
		state = oldState;
		if (flags & TCL_ENCODING_STOPONERROR) {
		    result = TCL_CONVERT_UNKNOWN;
		    break;
		}
		encodingPtr = GetTableEncoding(dataPtr, state);
		tableDataPtr = (TableEncodingData *) encodingPtr->clientData;
		word = tableDataPtr->fallback;
	    } 
	    
	    tablePrefixBytes = tableDataPtr->prefixBytes;
	    tableFromUnicode = tableDataPtr->fromUnicode;

	    subTablePtr = &dataPtr->subTables[state];
	    if (dst + subTablePtr->sequenceLen > dstEnd) {
		result = TCL_CONVERT_NOSPACE;
		break;
	    }
	    memcpy((VOID *) dst, (VOID *) subTablePtr->sequence,
		    (size_t) subTablePtr->sequenceLen);
	    dst += subTablePtr->sequenceLen;
	}

	if (tablePrefixBytes[(word >> 8)] != 0) {
	    if (dst + 1 > dstEnd) {
		result = TCL_CONVERT_NOSPACE;
		break;
	    }
	    dst[0] = (char) (word >> 8);
	    dst[1] = (char) word;
	    dst += 2;
	} else {
	    if (dst > dstEnd) {
		result = TCL_CONVERT_NOSPACE;
		break;
	    }
	    dst[0] = (char) word;
	    dst++;
	} 
	src += len;
    }

    if ((result == TCL_OK) && (flags & TCL_ENCODING_END)) {
	if (dst + dataPtr->finalLen > dstEnd) {
	    result = TCL_CONVERT_NOSPACE;
	} else {
	    memcpy((VOID *) dst, (VOID *) dataPtr->final,
		    (size_t) dataPtr->finalLen);
	    dst += dataPtr->finalLen;
	}
    }

    *statePtr = (Tcl_EncodingState) state;
    *srcReadPtr = src - srcStart;
    *dstWrotePtr = dst - dstStart;
    *dstCharsPtr = numChars;
    return result;
}

/*
 *---------------------------------------------------------------------------
 *
 * EscapeFreeProc --
 *
 *	This procedure is invoked when an EscapeEncodingData encoding is 
 *	deleted.  It deletes the memory used by the encoding.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory freed.
 *
 *---------------------------------------------------------------------------
 */

static void
EscapeFreeProc(clientData)
    ClientData clientData;	/* EscapeEncodingData that specifies encoding. */
{
    EscapeEncodingData *dataPtr;
    EscapeSubTable *subTablePtr;
    int i;

    dataPtr = (EscapeEncodingData *) clientData;
    if (dataPtr == NULL) {
	return;
    }
    subTablePtr = dataPtr->subTables;
    for (i = 0; i < dataPtr->numSubTables; i++) {
	FreeEncoding((Tcl_Encoding) subTablePtr->encodingPtr);
	subTablePtr++;
    }
    ckfree((char *) dataPtr);
}

/*
 *---------------------------------------------------------------------------
 *
 * GetTableEncoding --
 *
 *	Helper function for the EscapeEncodingData conversions.  Gets the
 *	encoding (of type TextEncodingData) that represents the specified
 *	state.
 *
 * Results:
 *	The return value is the encoding.
 *
 * Side effects:
 *	If the encoding that represents the specified state has not
 *	already been used by this EscapeEncoding, it will be loaded
 *	and cached in the dataPtr.
 *
 *---------------------------------------------------------------------------
 */

static Encoding *
GetTableEncoding(dataPtr, state)
    EscapeEncodingData *dataPtr;/* Contains names of encodings. */
    int state;			/* Index in dataPtr of desired Encoding. */
{
    EscapeSubTable *subTablePtr;
    Encoding *encodingPtr;
    
    subTablePtr = &dataPtr->subTables[state];
    encodingPtr = subTablePtr->encodingPtr;
    if (encodingPtr == NULL) {
	encodingPtr = (Encoding *) Tcl_GetEncoding(NULL, subTablePtr->name);
	if ((encodingPtr == NULL) 
		|| (encodingPtr->toUtfProc != TableToUtfProc)) {
	    panic("EscapeToUtfProc: invalid sub table");
	}
	subTablePtr->encodingPtr = encodingPtr;
    }
    return encodingPtr;
}

/*
 *---------------------------------------------------------------------------
 *
 * unilen --
 *
 *	A helper function for the Tcl_ExternalToUtf functions.  This
 *	function is similar to strlen for double-byte characters: it
 *	returns the number of bytes in a 0x0000 terminated string.
 *
 * Results:
 *	As above.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

static size_t
unilen(src)
    CONST char *src;
{
    unsigned short *p;

    p = (unsigned short *) src;
    while (*p != 0x0000) {
	p++;
    }
    return (char *) p - src;
}

	
