/* 
 * tclStringObj.c --
 *
 *	This file contains procedures that implement string operations on Tcl
 *	objects.  Some string operations work with UTF strings and others
 *	require Unicode format.  Functions that require knowledge of the width
 *	of each character, such as indexing, operate on Unicode data.
 *
 *	A Unicode string is an internationalized string.  Conceptually, a
 *	Unicode string is an array of 16-bit quantities organized as a sequence
 *	of properly formed UTF-8 characters.  There is a one-to-one map between
 *	Unicode and UTF characters.  Because Unicode characters have a fixed
 *	width, operations such as indexing operate on Unicode data.  The String
 *	ojbect is opitmized for the case where each UTF char in a string is
 *	only one byte.  In this case, we store the value of numChars, but we
 *	don't store the Unicode data (unless Tcl_GetUnicode is explicitly
 *	called).
 *
 *	The String object type stores one or both formats.  The default
 *	behavior is to store UTF.  Once Unicode is calculated by a function, it
 *	is stored in the internal rep for future access (without an additional
 *	O(n) cost).
 *
 *	To allow many appends to be done to an object without constantly
 *	reallocating the space for the string or Unicode representation, we
 *	allocate double the space for the string or Unicode and use the
 *	internal representation to keep track of how much space is used
 *	vs. allocated.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 * Copyright (c) 1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id */

#include "tclInt.h"

/*
 * Prototypes for procedures defined later in this file:
 */

static void		AppendUnicodeToUnicodeRep _ANSI_ARGS_((
    			    Tcl_Obj *objPtr, Tcl_UniChar *unicode,
			    int appendNumChars));
static void		AppendUnicodeToUtfRep _ANSI_ARGS_((
    			    Tcl_Obj *objPtr, Tcl_UniChar *unicode,
			    int numChars));
static void		AppendUtfToUnicodeRep _ANSI_ARGS_((Tcl_Obj *objPtr,
    			    char *bytes, int numBytes));
static void		AppendUtfToUtfRep _ANSI_ARGS_((Tcl_Obj *objPtr,
    			    char *bytes, int numBytes));

static void		FillUnicodeRep _ANSI_ARGS_((Tcl_Obj *objPtr));

static void		FreeStringInternalRep _ANSI_ARGS_((Tcl_Obj *objPtr));
static void		DupStringInternalRep _ANSI_ARGS_((Tcl_Obj *objPtr,
			    Tcl_Obj *copyPtr));
static int		SetStringFromAny _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *objPtr));
static void		UpdateStringOfString _ANSI_ARGS_((Tcl_Obj *objPtr));

/*
 * The structure below defines the string Tcl object type by means of
 * procedures that can be invoked by generic object code.
 */

Tcl_ObjType tclStringType = {
    "string",				/* name */
    FreeStringInternalRep,		/* freeIntRepPro */
    DupStringInternalRep,		/* dupIntRepProc */
    UpdateStringOfString,		/* updateStringProc */
    SetStringFromAny			/* setFromAnyProc */
};

/*
 * The following structure is the internal rep for a String object.
 * It keeps track of how much memory has been used and how much has been
 * allocated for the Unicode and UTF string to enable growing and
 * shrinking of the UTF and Unicode reps of the String object with fewer
 * mallocs.  To optimize string length and indexing operations, this
 * structure also stores the number of characters (same of UTF and Unicode!)
 * once that value has been computed.
 */

typedef struct String {
    int numChars;		/* The number of chars in the string.
				 * -1 means this value has not been
				 * calculated. >= 0 means that there is a
				 * valid Unicode rep, or that the number
				 * of UTF bytes == the number of chars. */
    size_t allocated;		/* The amount of space actually allocated
				 * for the UTF string (minus 1 byte for
				 * the termination char). */
    size_t uallocated;		/* The amount of space actually allocated
				 * for the Unicode string. 0 means the
				 * Unicode string rep is invalid. */
    Tcl_UniChar unicode[2];	/* The array of Unicode chars.  The actual
				 * size of this field depends on the
				 * 'uallocated' field above. */
} String;

#define STRING_SIZE(len)	\
		((unsigned) (sizeof(String) + ((len-1) * sizeof(Tcl_UniChar))))
#define GET_STRING(objPtr) \
		((String *) (objPtr)->internalRep.otherValuePtr)
#define SET_STRING(objPtr, stringPtr) \
		(objPtr)->internalRep.otherValuePtr = (VOID *) (stringPtr)


/*
 *----------------------------------------------------------------------
 *
 * Tcl_NewStringObj --
 *
 *	This procedure is normally called when not debugging: i.e., when
 *	TCL_MEM_DEBUG is not defined. It creates a new string object and
 *	initializes it from the byte pointer and length arguments.
 *
 *	When TCL_MEM_DEBUG is defined, this procedure just returns the
 *	result of calling the debugging version Tcl_DbNewStringObj.
 *
 * Results:
 *	A newly created string object is returned that has ref count zero.
 *
 * Side effects:
 *	The new object's internal string representation will be set to a
 *	copy of the length bytes starting at "bytes". If "length" is
 *	negative, use bytes up to the first NULL byte; i.e., assume "bytes"
 *	points to a C-style NULL-terminated string. The object's type is set
 *	to NULL. An extra NULL is added to the end of the new object's byte
 *	array.
 *
 *----------------------------------------------------------------------
 */

#ifdef TCL_MEM_DEBUG
#undef Tcl_NewStringObj

Tcl_Obj *
Tcl_NewStringObj(bytes, length)
    CONST char *bytes;		/* Points to the first of the length bytes
				 * used to initialize the new object. */
    int length;			/* The number of bytes to copy from "bytes"
				 * when initializing the new object. If 
				 * negative, use bytes up to the first
				 * NULL byte. */
{
    return Tcl_DbNewStringObj(bytes, length, "unknown", 0);
}

#else /* if not TCL_MEM_DEBUG */

Tcl_Obj *
Tcl_NewStringObj(bytes, length)
    CONST char *bytes;		/* Points to the first of the length bytes
				 * used to initialize the new object. */
    int length;			/* The number of bytes to copy from "bytes"
				 * when initializing the new object. If 
				 * negative, use bytes up to the first
				 * NULL byte. */
{
    register Tcl_Obj *objPtr;

    if (length < 0) {
	length = (bytes? strlen(bytes) : 0);
    }
    TclNewObj(objPtr);
    TclInitStringRep(objPtr, bytes, length);
    return objPtr;
}
#endif /* TCL_MEM_DEBUG */

/*
 *----------------------------------------------------------------------
 *
 * Tcl_DbNewStringObj --
 *
 *	This procedure is normally called when debugging: i.e., when
 *	TCL_MEM_DEBUG is defined. It creates new string objects. It is the
 *	same as the Tcl_NewStringObj procedure above except that it calls
 *	Tcl_DbCkalloc directly with the file name and line number from its
 *	caller. This simplifies debugging since then the checkmem command
 *	will report the correct file name and line number when reporting
 *	objects that haven't been freed.
 *
 *	When TCL_MEM_DEBUG is not defined, this procedure just returns the
 *	result of calling Tcl_NewStringObj.
 *
 * Results:
 *	A newly created string object is returned that has ref count zero.
 *
 * Side effects:
 *	The new object's internal string representation will be set to a
 *	copy of the length bytes starting at "bytes". If "length" is
 *	negative, use bytes up to the first NULL byte; i.e., assume "bytes"
 *	points to a C-style NULL-terminated string. The object's type is set
 *	to NULL. An extra NULL is added to the end of the new object's byte
 *	array.
 *
 *----------------------------------------------------------------------
 */

#ifdef TCL_MEM_DEBUG

Tcl_Obj *
Tcl_DbNewStringObj(bytes, length, file, line)
    CONST char *bytes;		/* Points to the first of the length bytes
				 * used to initialize the new object. */
    int length;			/* The number of bytes to copy from "bytes"
				 * when initializing the new object. If 
				 * negative, use bytes up to the first
				 * NULL byte. */
    char *file;			/* The name of the source file calling this
				 * procedure; used for debugging. */
    int line;			/* Line number in the source file; used
				 * for debugging. */
{
    register Tcl_Obj *objPtr;

    if (length < 0) {
	length = (bytes? strlen(bytes) : 0);
    }
    TclDbNewObj(objPtr, file, line);
    TclInitStringRep(objPtr, bytes, length);
    return objPtr;
}

#else /* if not TCL_MEM_DEBUG */

Tcl_Obj *
Tcl_DbNewStringObj(bytes, length, file, line)
    CONST char *bytes;		/* Points to the first of the length bytes
				 * used to initialize the new object. */
    register int length;	/* The number of bytes to copy from "bytes"
				 * when initializing the new object. If 
				 * negative, use bytes up to the first
				 * NULL byte. */
    char *file;			/* The name of the source file calling this
				 * procedure; used for debugging. */
    int line;			/* Line number in the source file; used
				 * for debugging. */
{
    return Tcl_NewStringObj(bytes, length);
}
#endif /* TCL_MEM_DEBUG */

/*
 *---------------------------------------------------------------------------
 *
 * TclNewUnicodeObj --
 *
 *	This procedure is creates a new String object and initializes
 *	it from the given Utf String.  If the Utf String is the same size
 *	as the Unicode string, don't duplicate the data.
 *
 * Results:
 *	The newly created object is returned.  This object will have no
 *	initial string representation.  The returned object has a ref count
 *	of 0.
 *
 * Side effects:
 *	Memory allocated for new object and copy of Unicode argument.
 *
 *---------------------------------------------------------------------------
 */

Tcl_Obj *
Tcl_NewUnicodeObj(unicode, numChars)
    Tcl_UniChar *unicode;	/* The unicode string used to initialize
				 * the new object. */
    int numChars;		/* Number of characters in the unicode
				 * string. */
{
    Tcl_Obj *objPtr;
    String *stringPtr;
    size_t uallocated;

    if (numChars < 0) {
	numChars = 0;
	if (unicode) {
	    while (unicode[numChars] != 0) { numChars++; }
	}
    }
    uallocated = (numChars + 1) * sizeof(Tcl_UniChar);

    /*
     * Create a new obj with an invalid string rep.
     */

    TclNewObj(objPtr);
    Tcl_InvalidateStringRep(objPtr);
    objPtr->typePtr = &tclStringType;

    stringPtr = (String *) ckalloc(STRING_SIZE(uallocated));
    stringPtr->numChars = numChars;
    stringPtr->uallocated = uallocated;
    stringPtr->allocated = 0;
    memcpy((VOID *) stringPtr->unicode, (VOID *) unicode, uallocated);
    stringPtr->unicode[numChars] = 0;
    SET_STRING(objPtr, stringPtr);
    return objPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetCharLength --
 *
 *	Get the length of the Unicode string from the Tcl object.
 *
 * Results:
 *	Pointer to unicode string representing the unicode object.
 *
 * Side effects:
 *	Frees old internal rep.  Allocates memory for new "String"
 *	internal rep.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_GetCharLength(objPtr)
    Tcl_Obj *objPtr;	/* The String object to get the num chars of. */
{
    String *stringPtr;
    
    SetStringFromAny(NULL, objPtr);
    stringPtr = GET_STRING(objPtr);

    /*
     * If numChars is unknown, then calculate the number of characaters
     * while populating the Unicode string.
     */
    
    if (stringPtr->numChars == -1) {

	stringPtr->numChars = Tcl_NumUtfChars(objPtr->bytes, objPtr->length);

 	if (stringPtr->numChars == objPtr->length) {

	    /*
	     * Since we've just calculated the number of chars, and all
	     * UTF chars are 1-byte long, we don't need to store the
	     * unicode string.
	     */

	    stringPtr->uallocated = 0;

	} else {
    
	    /*
	     * Since we've just calucalated the number of chars, and not
	     * all UTF chars are 1-byte long, go ahead and populate the
	     * unicode string.
	     */

	    FillUnicodeRep(objPtr);

	    /*
	     * We need to fetch the pointer again because we have just
	     * reallocated the structure to make room for the Unicode data.
	     */
	    
	    stringPtr = GET_STRING(objPtr);
	}
    }
    return stringPtr->numChars;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetUniChar --
 *
 *	Get the index'th Unicode character from the String object.  The
 *	index is assumed to be in the appropriate range.
 *
 * Results:
 *	Returns the index'th Unicode character in the Object.
 *
 * Side effects:
 *	Fills unichar with the index'th Unicode character.
 *
 *----------------------------------------------------------------------
 */

Tcl_UniChar
Tcl_GetUniChar(objPtr, index)
    Tcl_Obj *objPtr;	/* The object to get the Unicode charater from. */
    int index;		/* Get the index'th Unicode character. */
{
    Tcl_UniChar unichar;
    String *stringPtr;
    
    SetStringFromAny(NULL, objPtr);
    stringPtr = GET_STRING(objPtr);

    if (stringPtr->numChars == -1) {

	/*
	 * We haven't yet calculated the length, so we don't have the
	 * Unicode str.  We need to know the number of chars before we
	 * can do indexing.
	 */

	Tcl_GetCharLength(objPtr);

	/*
	 * We need to fetch the pointer again because we may have just
	 * reallocated the structure.
	 */
	
	stringPtr = GET_STRING(objPtr);
    }
    if (stringPtr->uallocated == 0) {

	/*
	 * All of the characters in the Utf string are 1 byte chars,
	 * so we don't store the unicode char.  We get the Utf string
	 * and convert the index'th byte to a Unicode character.
	 */
	
	Tcl_UtfToUniChar(&objPtr->bytes[index], &unichar);	
    } else {
	unichar = stringPtr->unicode[index];
    }
    return unichar;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetUnicode --
 *
 *	Get the Unicode form of the String object.  If
 *	the object is not already a String object, it will be converted
 *	to one.  If the String object does not have a Unicode rep, then
 *	one is create from the UTF string format.
 *
 * Results:
 *	Returns a pointer to the object's internal Unicode string.
 *
 * Side effects:
 *	Converts the object to have the String internal rep.
 *
 *----------------------------------------------------------------------
 */

Tcl_UniChar *
Tcl_GetUnicode(objPtr)
    Tcl_Obj *objPtr;	/* The object to find the unicode string for. */
{
    String *stringPtr;
    
    SetStringFromAny(NULL, objPtr);
    stringPtr = GET_STRING(objPtr);
    
    if ((stringPtr->numChars == -1) || (stringPtr->uallocated == 0)) {

	/*
	 * We haven't yet calculated the length, or all of the characters
	 * in the Utf string are 1 byte chars (so we didn't store the
	 * unicode str).  Since this function must return a unicode string,
	 * and one has not yet been stored, force the Unicode to be
	 * calculated and stored now.
	 */

	FillUnicodeRep(objPtr);

	/*
	 * We need to fetch the pointer again because we have just
	 * reallocated the structure to make room for the Unicode data.
	 */
	
	stringPtr = GET_STRING(objPtr);
    }
    return stringPtr->unicode;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetRange --
 *
 *	Create a Tcl Object that contains the chars between first and last
 *	of the object indicated by "objPtr".  If the object is not already
 *	a String object, convert it to one.  The first and last indices
 *	are assumed to be in the appropriate range.
 *
 * Results:
 *	Returns a new Tcl Object of the String type.
 *
 * Side effects:
 *	Changes the internal rep of "objPtr" to the String type.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj*
Tcl_GetRange(objPtr, first, last)
   
 Tcl_Obj *objPtr;		/* The Tcl object to find the range of. */
    int first;			/* First index of the range. */
    int last;			/* Last index of the range. */
{
    Tcl_Obj *newObjPtr;		/* The Tcl object to find the range of. */
    String *stringPtr;
    
    SetStringFromAny(NULL, objPtr);
    stringPtr = GET_STRING(objPtr);

    if (stringPtr->numChars == -1) {
    
	/*
	 * We haven't yet calculated the length, so we don't have the
	 * Unicode str.  We need to know the number of chars before we
	 * can do indexing.
	 */

	Tcl_GetCharLength(objPtr);

	/*
	 * We need to fetch the pointer again because we may have just
	 * reallocated the structure.
	 */
	
	stringPtr = GET_STRING(objPtr);
    }

    if (stringPtr->numChars == objPtr->length) {
	char *str = Tcl_GetString(objPtr);

	/*
	 * All of the characters in the Utf string are 1 byte chars,
	 * so we don't store the unicode char.  Create a new string
	 * object containing the specified range of chars.
	 */
	
	newObjPtr = Tcl_NewStringObj(&str[first], last-first+1);

	/*
	 * Since we know the new string only has 1-byte chars, we
	 * can set it's numChars field.
	 */
	
	SetStringFromAny(NULL, newObjPtr);
	stringPtr = GET_STRING(newObjPtr);
	stringPtr->numChars = last-first+1;
    } else {
	newObjPtr = Tcl_NewUnicodeObj(stringPtr->unicode + first,
		last-first+1);
    }
    return newObjPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetStringObj --
 *
 *	Modify an object to hold a string that is a copy of the bytes
 *	indicated by the byte pointer and length arguments. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The object's string representation will be set to a copy of
 *	the "length" bytes starting at "bytes". If "length" is negative, use
 *	bytes up to the first NULL byte; i.e., assume "bytes" points to a
 *	C-style NULL-terminated string. The object's old string and internal
 *	representations are freed and the object's type is set NULL.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetStringObj(objPtr, bytes, length)
    register Tcl_Obj *objPtr;	/* Object whose internal rep to init. */
    char *bytes;		/* Points to the first of the length bytes
				 * used to initialize the object. */
    register int length;	/* The number of bytes to copy from "bytes"
				 * when initializing the object. If 
				 * negative, use bytes up to the first
				 * NULL byte.*/
{
    register Tcl_ObjType *oldTypePtr = objPtr->typePtr;

    /*
     * Free any old string rep, then set the string rep to a copy of
     * the length bytes starting at "bytes".
     */

    if (Tcl_IsShared(objPtr)) {
	panic("Tcl_SetStringObj called with shared object");
    }

    /*
     * Set the type to NULL and free any internal rep for the old type.
     */

    if ((oldTypePtr != NULL) && (oldTypePtr->freeIntRepProc != NULL)) {
	oldTypePtr->freeIntRepProc(objPtr);
    }
    objPtr->typePtr = NULL;

    Tcl_InvalidateStringRep(objPtr);
    if (length < 0) {
	length = (bytes? strlen(bytes) : 0);
    }
    TclInitStringRep(objPtr, bytes, length);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetObjLength --
 *
 *	This procedure changes the length of the string representation
 *	of an object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	If the size of objPtr's string representation is greater than
 *	length, then it is reduced to length and a new terminating null
 *	byte is stored in the strength.  If the length of the string
 *	representation is greater than length, the storage space is
 *	reallocated to the given length; a null byte is stored at the
 *	end, but other bytes past the end of the original string
 *	representation are undefined.  The object's internal
 *	representation is changed to "expendable string".
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetObjLength(objPtr, length)
    register Tcl_Obj *objPtr;	/* Pointer to object.  This object must
				 * not currently be shared. */
    register int length;	/* Number of bytes desired for string
				 * representation of object, not including
				 * terminating null byte. */
{
    char *new;
    String *stringPtr;

    if (Tcl_IsShared(objPtr)) {
	panic("Tcl_SetObjLength called with shared object");
    }
    SetStringFromAny(NULL, objPtr);
        
    /*
     * Invalidate the unicode data.
     */

    stringPtr = GET_STRING(objPtr);
    stringPtr->numChars = -1;
    stringPtr->uallocated = 0;

    if (length > (int) stringPtr->allocated) {

	/*
	 * Not enough space in current string. Reallocate the string
	 * space and free the old string.
	 */

	new = (char *) ckalloc((unsigned) (length+1));
	if (objPtr->bytes != NULL) {
	    memcpy((VOID *) new, (VOID *) objPtr->bytes,
		    (size_t) objPtr->length);
	    Tcl_InvalidateStringRep(objPtr);
	}
	objPtr->bytes = new;
	stringPtr->allocated = length;
    }
    
    objPtr->length = length;
    if ((objPtr->bytes != NULL) && (objPtr->bytes != tclEmptyStringRep)) {
	objPtr->bytes[length] = 0;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * TclSetUnicodeObj --
 *
 *	Modify an object to hold the Unicode string indicated by "unicode".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Memory allocated for new "String" internal rep.
 *
 *---------------------------------------------------------------------------
 */

void
Tcl_SetUnicodeObj(objPtr, unicode, numChars)
    Tcl_Obj *objPtr;		/* The object to set the string of. */
    Tcl_UniChar *unicode;	/* The unicode string used to initialize
				 * the object. */
    int numChars;		/* Number of characters in the unicode
				 * string. */
{
    Tcl_ObjType *typePtr;
    String *stringPtr;
    size_t uallocated;

    if (numChars < 0) {
	numChars = 0;
	if (unicode) {
	    while (unicode[numChars] != 0) { numChars++; }
	}
    }
    uallocated = (numChars + 1) * sizeof(Tcl_UniChar);

    /*
     * Free the internal rep if one exists, and invalidate the string rep.
     */

    typePtr = objPtr->typePtr;
    if ((typePtr != NULL) && (typePtr->freeIntRepProc) != NULL) {
	(*typePtr->freeIntRepProc)(objPtr);
    }
    objPtr->typePtr = &tclStringType;

    /*
     * Allocate enough space for the String structure + Unicode string.
     */
	
    stringPtr = (String *) ckalloc(STRING_SIZE(uallocated));
    stringPtr->numChars = numChars;
    stringPtr->uallocated = uallocated;
    stringPtr->allocated = 0;
    memcpy((VOID *) stringPtr->unicode, (VOID *) unicode, uallocated);
    stringPtr->unicode[numChars] = 0;
    SET_STRING(objPtr, stringPtr);
    Tcl_InvalidateStringRep(objPtr);
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendToObj --
 *
 *	This procedure appends a sequence of bytes to an object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The bytes at *bytes are appended to the string representation
 *	of objPtr.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AppendToObj(objPtr, bytes, length)
    register Tcl_Obj *objPtr;	/* Points to the object to append to. */
    char *bytes;		/* Points to the bytes to append to the
				 * object. */
    register int length;	/* The number of bytes to append from
				 * "bytes". If < 0, then append all bytes
				 * up to NULL byte. */
{
    String *stringPtr;

    if (Tcl_IsShared(objPtr)) {
	panic("Tcl_AppendToObj called with shared object");
    }
    
    SetStringFromAny(NULL, objPtr);

    if (length < 0) {
	length = (bytes ? strlen(bytes) : 0);
    }
    if (length == 0) {
	return;
    }

    /*
     * If objPtr has a valid Unicode rep, then append the Unicode
     * conversion of "bytes" to the objPtr's Unicode rep, otherwise
     * append "bytes" to objPtr's string rep.
     */

    stringPtr = GET_STRING(objPtr);
    if (stringPtr->uallocated > 0) {
	AppendUtfToUnicodeRep(objPtr, bytes, length);

	stringPtr = GET_STRING(objPtr);
    } else {
	AppendUtfToUtfRep(objPtr, bytes, length);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendUnicodeToObj --
 *
 *	This procedure appends a Unicode string to an object in the
 *	most efficient manner possible.  Length must be >= 0.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Invalidates the string rep and creates a new Unicode string.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AppendUnicodeToObj(objPtr, unicode, length)
    register Tcl_Obj *objPtr;	/* Points to the object to append to. */
    Tcl_UniChar *unicode;	/* The unicode string to append to the
			         * object. */
    int length;			/* Number of chars in "unicode". */
{
    String *stringPtr;

    if (Tcl_IsShared(objPtr)) {
	panic("Tcl_AppendUnicodeToObj called with shared object");
    }

    if (length == 0) {
	return;
    }

    SetStringFromAny(NULL, objPtr);

    /*
     * TEMPORARY!!!  This is terribly inefficient, but it works, and Don
     * needs for me to check this stuff in ASAP.  -Melissa
     */
    
/*     UpdateStringOfString(objPtr); */
/*     AppendUnicodeToUtfRep(objPtr, unicode, length); */
/*     return; */

    /*
     * If objPtr has a valid Unicode rep, then append the "unicode"
     * to the objPtr's Unicode rep, otherwise the UTF conversion of
     * "unicode" to objPtr's string rep.
     */

    stringPtr = GET_STRING(objPtr);
    if (stringPtr->uallocated > 0) {
	AppendUnicodeToUnicodeRep(objPtr, unicode, length);
    } else {
	AppendUnicodeToUtfRep(objPtr, unicode, length);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendObjToObj --
 *
 *	This procedure appends the string rep of one object to another.
 *	"objPtr" cannot be a shared object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The string rep of appendObjPtr is appended to the string 
 *	representation of objPtr.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AppendObjToObj(objPtr, appendObjPtr)
    Tcl_Obj *objPtr;		/* Points to the object to append to. */
    Tcl_Obj *appendObjPtr;	/* Object to append. */
{
    String *stringPtr;
    int length, numChars, allOneByteChars;
    char *bytes;

    SetStringFromAny(NULL, objPtr);

    /*
     * If objPtr has a valid Unicode rep, then get a Unicode string
     * from appendObjPtr and append it.
     */

    stringPtr = GET_STRING(objPtr);
    if (stringPtr->uallocated > 0) {
	
	/*
	 * If appendObjPtr is not of the "String" type, don't convert it.
	 */

	if (appendObjPtr->typePtr == &tclStringType) {
	    stringPtr = GET_STRING(appendObjPtr);
	    if ((stringPtr->numChars == -1)
		    || (stringPtr->uallocated == 0)) {
		
		/*
		 * If appendObjPtr is a string obj with no valide Unicode
		 * rep, then fill its unicode rep.
		 */

		FillUnicodeRep(appendObjPtr);
		stringPtr = GET_STRING(appendObjPtr);
	    }
	    AppendUnicodeToUnicodeRep(objPtr, stringPtr->unicode,
		    stringPtr->numChars);
	} else {
	    bytes = Tcl_GetStringFromObj(appendObjPtr, &length);
	    AppendUtfToUnicodeRep(objPtr, bytes, length);
	}
	return;
    }

    /*
     * Append to objPtr's UTF string rep.  If we know the number of
     * characters in both objects before appending, then set the combined
     * number of characters in the final (appended-to) object.
     */

    bytes = Tcl_GetStringFromObj(appendObjPtr, &length);

    allOneByteChars = 0;
    numChars = stringPtr->numChars;
    if ((numChars >= 0) && (appendObjPtr->typePtr == &tclStringType)) {
	stringPtr = GET_STRING(appendObjPtr);
	if ((stringPtr->numChars >= 0) && (stringPtr->numChars == length)) {
	    numChars += stringPtr->numChars;
	    allOneByteChars = 1;
	}
    }

    AppendUtfToUtfRep(objPtr, bytes, length);

    if (allOneByteChars) {
	stringPtr = GET_STRING(objPtr);
	stringPtr->numChars = numChars;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * AppendUnicodeToUnicodeRep --
 *
 *	This procedure appends the contents of "unicode" to the Unicode
 *	rep of "objPtr".  objPtr must already have a valid Unicode rep.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	objPtr's internal rep is reallocated.
 *
 *----------------------------------------------------------------------
 */

static void
AppendUnicodeToUnicodeRep(objPtr, unicode, appendNumChars)
    Tcl_Obj *objPtr;	      /* Points to the object to append to. */
    Tcl_UniChar *unicode;     /* String to append. */
    int appendNumChars;	      /* Number of chars of "unicode" to append. */
{
    String *stringPtr;
    int numChars;
    size_t newSize;

    if (appendNumChars < 0) {
	appendNumChars = 0;
	if (unicode) {
	    while (unicode[appendNumChars] != 0) { appendNumChars++; }
	}
    }
    if (appendNumChars == 0) {
	return;
    }

    SetStringFromAny(NULL, objPtr);
    stringPtr = GET_STRING(objPtr);
    
    /*
     * If not enough space has been allocated for the unicode rep,
     * reallocate the internal rep object with double the amount of
     * space needed, so the unicode string can grow without being
     * reallocated.
     */

    numChars = stringPtr->numChars + appendNumChars;
    newSize = (numChars + 1) * sizeof(Tcl_UniChar);

    if (newSize > stringPtr->uallocated) {
	stringPtr->uallocated = newSize * 2;
	stringPtr = (String *) ckrealloc((char*)stringPtr,
		STRING_SIZE(stringPtr->uallocated));
	SET_STRING(objPtr, stringPtr);
    }

    /*
     * Copy the new string onto the end of the old string, then add the
     * trailing null.
     */

    memcpy((VOID*) (stringPtr->unicode + stringPtr->numChars), unicode,
	    appendNumChars * sizeof(Tcl_UniChar));
    stringPtr->unicode[numChars] = 0;
    stringPtr->numChars = numChars;

    SET_STRING(objPtr, stringPtr);
    Tcl_InvalidateStringRep(objPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * AppendUnicodeToUtfRep --
 *
 *	This procedure converts the contents of "unicode" to UTF and
 *	appends the UTF to the string rep of "objPtr".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	objPtr's internal rep is reallocated.
 *
 *----------------------------------------------------------------------
 */

static void
AppendUnicodeToUtfRep(objPtr, unicode, numChars)
    Tcl_Obj *objPtr;	      /* Points to the object to append to. */
    Tcl_UniChar *unicode;     /* String to convert to UTF. */
    int numChars;	      /* Number of chars of "unicode" to convert. */
{
    Tcl_DString dsPtr;
    char *bytes;
    
    if (numChars < 0) {
	numChars = 0;
	if (unicode) {
	    while (unicode[numChars] != 0) { numChars++; }
	}
    }
    if (numChars == 0) {
	return;
    }

    Tcl_DStringInit(&dsPtr);
    bytes = (char *)Tcl_UniCharToUtfDString(unicode, numChars, &dsPtr);
    AppendUtfToUtfRep(objPtr, bytes, Tcl_DStringLength(&dsPtr));
    Tcl_DStringFree(&dsPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * AppendUtfToUnicodeRep --
 *
 *	This procedure converts the contents of "bytes" to Unicode and
 *	appends the Unicode to the Unicode rep of "objPtr".  objPtr must
 *	already have a valid Unicode rep.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	objPtr's internal rep is reallocated.
 *
 *----------------------------------------------------------------------
 */

static void
AppendUtfToUnicodeRep(objPtr, bytes, numBytes)
    Tcl_Obj *objPtr;	/* Points to the object to append to. */
    char *bytes;		/* String to convert to Unicode. */
    int numBytes;	/* Number of bytes of "bytes" to convert. */
{
    Tcl_DString dsPtr;
    int numChars;
    Tcl_UniChar *unicode;

    if (numBytes < 0) {
	numBytes = (bytes ? strlen(bytes) : 0);
    }
    if (numBytes == 0) {
	return;
    }
    
    Tcl_DStringInit(&dsPtr);
    numChars = Tcl_NumUtfChars(bytes, numBytes);
    unicode = (Tcl_UniChar *)Tcl_UtfToUniCharDString(bytes, numBytes, &dsPtr);
    AppendUnicodeToUnicodeRep(objPtr, unicode, numChars);
    Tcl_DStringFree(&dsPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * AppendUtfToUtfRep --
 *
 *	This procedure appends "numBytes" bytes of "bytes" to the UTF string
 *	rep of "objPtr".  objPtr must already have a valid String rep.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	objPtr's internal rep is reallocated.
 *
 *----------------------------------------------------------------------
 */

static void
AppendUtfToUtfRep(objPtr, bytes, numBytes)
    Tcl_Obj *objPtr;	/* Points to the object to append to. */
    char *bytes;	/* String to append. */
    int numBytes;	/* Number of bytes of "bytes" to append. */
{
    String *stringPtr;
    int newLength, oldLength;

    if (numBytes < 0) {
	numBytes = (bytes ? strlen(bytes) : 0);
    }
    if (numBytes == 0) {
	return;
    }

    /*
     * Copy the new string onto the end of the old string, then add the
     * trailing null.
     */

    oldLength = objPtr->length;
    newLength = numBytes + oldLength;

    stringPtr = GET_STRING(objPtr);
    if (newLength > (int) stringPtr->allocated) {

	/*
	 * There isn't currently enough space in the string
	 * representation so allocate additional space.  Overallocate the
	 * space by doubling it so that we won't have to do as much
	 * reallocation in the future.
	 */

	Tcl_SetObjLength(objPtr, 2*newLength);
    } else {

	/*
	 * Invalidate the unicode data.
	 */
	
	stringPtr->numChars = -1;
	stringPtr->uallocated = 0;
    }
    
    memcpy((VOID *) (objPtr->bytes + oldLength), (VOID *) bytes,
	    (size_t) numBytes);
    objPtr->bytes[newLength] = 0;
    objPtr->length = newLength;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendStringsToObjVA --
 *
 *	This procedure appends one or more null-terminated strings
 *	to an object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of all the string arguments are appended to the
 *	string representation of objPtr.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AppendStringsToObjVA (objPtr, argList)
    Tcl_Obj *objPtr;		/* Points to the object to append to. */
    va_list argList;		/* Variable argument list. */
{
#define STATIC_LIST_SIZE 16
    String *stringPtr;
    int newLength, oldLength;
    register char *string, *dst;
    char *static_list[STATIC_LIST_SIZE];
    char **args = static_list;
    int nargs_space = STATIC_LIST_SIZE;
    int nargs, i;

    if (Tcl_IsShared(objPtr)) {
	panic("Tcl_AppendStringsToObj called with shared object");
    }

    SetStringFromAny(NULL, objPtr);

    /*
     * Figure out how much space is needed for all the strings, and
     * expand the string representation if it isn't big enough. If no
     * bytes would be appended, just return.  Note that on some platforms
     * (notably OS/390) the argList is an array so we need to use memcpy.
     */

    nargs = 0;
    newLength = oldLength = objPtr->length;
    while (1) {
	string = va_arg(argList, char *);
	if (string == NULL) {
	    break;
	}
 	if (nargs >= nargs_space) {
 	    /* 
 	     * Expand the args buffer
 	     */
 	    nargs_space += STATIC_LIST_SIZE;
 	    if (args == static_list) {
 	    	args = (void *)ckalloc(nargs_space * sizeof(char *));
 		for (i = 0; i < nargs; ++i) {
 		    args[i] = static_list[i];
 		}
 	    } else {
 		args = (void *)ckrealloc((void *)args,
			nargs_space * sizeof(char *));
 	    }
 	}
	newLength += strlen(string);
	args[nargs++] = string;
    }
    if (newLength == oldLength) {
	goto done;
    }

    stringPtr = GET_STRING(objPtr);
    if (newLength > (int) stringPtr->allocated) {

	/*
	 * There isn't currently enough space in the string
	 * representation so allocate additional space.  If the current
	 * string representation isn't empty (i.e. it looks like we're
	 * doing a series of appends) then overallocate the space so
	 * that we won't have to do as much reallocation in the future.
	 */

	Tcl_SetObjLength(objPtr,
		(objPtr->length == 0) ? newLength : 2*newLength);
    }

    /*
     * Make a second pass through the arguments, appending all the
     * strings to the object.
     */

    dst = objPtr->bytes + oldLength;
    for (i = 0; i < nargs; ++i) {
 	string = args[i];
	if (string == NULL) {
	    break;
	}
	while (*string != 0) {
	    *dst = *string;
	    dst++;
	    string++;
	}
    }

    /*
     * Add a null byte to terminate the string.  However, be careful:
     * it's possible that the object is totally empty (if it was empty
     * originally and there was nothing to append).  In this case dst is
     * NULL; just leave everything alone.
     */

    if (dst != NULL) {
	*dst = 0;
    }
    objPtr->length = newLength;

    done:
    /*
     * If we had to allocate a buffer from the heap, 
     * free it now.
     */
 
    if (args != static_list) {
     	ckfree((void *)args);
    }
#undef STATIC_LIST_SIZE
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppendStringsToObj --
 *
 *	This procedure appends one or more null-terminated strings
 *	to an object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The contents of all the string arguments are appended to the
 *	string representation of objPtr.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_AppendStringsToObj TCL_VARARGS_DEF(Tcl_Obj *,arg1)
{
    register Tcl_Obj *objPtr;
    va_list argList;

    objPtr = TCL_VARARGS_START(Tcl_Obj *,arg1,argList);
    Tcl_AppendStringsToObjVA(objPtr, argList);
    va_end(argList);
}

/*
 *---------------------------------------------------------------------------
 *
 * FillUnicodeRep --
 *
 *	Populate the Unicode internal rep with the Unicode form of its string
 *	rep.  The object must alread have a "String" internal rep.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Reallocates the String internal rep.
 *
 *---------------------------------------------------------------------------
 */

static void
FillUnicodeRep(objPtr)
    Tcl_Obj *objPtr;	/* The object in which to fill the unicode rep. */
{
    String *stringPtr;
    size_t uallocated;
    char *src, *srcEnd;
    Tcl_UniChar *dst;
    src = objPtr->bytes;
    
    stringPtr = GET_STRING(objPtr);
    if (stringPtr->numChars == -1) {
	stringPtr->numChars = Tcl_NumUtfChars(src, objPtr->length);
    }

    uallocated = stringPtr->numChars * sizeof(Tcl_UniChar);
    if (uallocated > stringPtr->uallocated) {
    
	/*
	 * If not enough space has been allocated for the unicode rep,
	 * reallocate the internal rep object.
	 */

	/*
	 * There isn't currently enough space in the Unicode
	 * representation so allocate additional space.  If the current
	 * Unicode representation isn't empty (i.e. it looks like we've
	 * done some appends) then overallocate the space so
	 * that we won't have to do as much reallocation in the future.
	 */

	if (stringPtr->uallocated > 0) {
	    uallocated *= 2;
	}
	stringPtr = (String *) ckrealloc((char*) stringPtr,
		STRING_SIZE(uallocated));
	stringPtr->uallocated = uallocated;
    }

    /*
     * Convert src to Unicode and store the coverted data in "unicode".
     */
    
    srcEnd = src + objPtr->length;
    for (dst = stringPtr->unicode; src < srcEnd; dst++) {
	src += Tcl_UtfToUniChar(src, dst);
    }
    *dst = 0;
    
    SET_STRING(objPtr, stringPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * DupStringInternalRep --
 *
 *	Initialize the internal representation of a new Tcl_Obj to a
 *	copy of the internal representation of an existing string object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	copyPtr's internal rep is set to a copy of srcPtr's internal
 *	representation.
 *
 *----------------------------------------------------------------------
 */

static void
DupStringInternalRep(srcPtr, copyPtr)
    register Tcl_Obj *srcPtr;	/* Object with internal rep to copy.  Must
				 * have an internal rep of type "String". */
    register Tcl_Obj *copyPtr;	/* Object with internal rep to set.  Must
				 * not currently have an internal rep.*/
{
    String *srcStringPtr = GET_STRING(srcPtr);
    String *copyStringPtr = NULL;

    /*
     * If the src obj is a string of 1-byte Utf chars, then copy the
     * string rep of the source object and create an "empty" Unicode
     * internal rep for the new object.  Otherwise, copy Unicode
     * internal rep, and invalidate the string rep of the new object.
     */
    
    if (srcStringPtr->uallocated == 0) {
    	copyStringPtr = (String *) ckalloc(sizeof(String));
	copyStringPtr->uallocated = 0;
    } else {
	copyStringPtr = (String *) ckalloc(
	    STRING_SIZE(srcStringPtr->uallocated));
	copyStringPtr->uallocated = srcStringPtr->uallocated;

	memcpy((VOID *) copyStringPtr->unicode,
		(VOID *) srcStringPtr->unicode,
		(size_t) srcStringPtr->numChars * sizeof(Tcl_UniChar));
	copyStringPtr->unicode[srcStringPtr->numChars] = 0;
    }
    copyStringPtr->numChars = srcStringPtr->numChars;
    copyStringPtr->allocated = srcStringPtr->allocated;

    /*
     * Tricky point: the string value was copied by generic object
     * management code, so it doesn't contain any extra bytes that
     * might exist in the source object.
     */

    copyStringPtr->allocated = copyPtr->length;

    SET_STRING(copyPtr, copyStringPtr);
    copyPtr->typePtr = &tclStringType;
}

/*
 *----------------------------------------------------------------------
 *
 * SetStringFromAny --
 *
 *	Create an internal representation of type "String" for an object.
 *
 * Results:
 *	This operation always succeeds and returns TCL_OK.
 *
 * Side effects:
 *	Any old internal reputation for objPtr is freed and the
 *	internal representation is set to "String".
 *
 *----------------------------------------------------------------------
 */

static int
SetStringFromAny(interp, objPtr)
    Tcl_Interp *interp;		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr;		/* The object to convert. */
{
    String *stringPtr;

    /*
     * The Unicode object is opitmized for the case where each UTF char
     * in a string is only one byte.  In this case, we store the value of
     * numChars, but we don't copy the bytes to the unicodeObj->unicode.
     */

    if (objPtr->typePtr != &tclStringType) {

	if (objPtr->typePtr != NULL) {
	    if (objPtr->bytes == NULL) {
		objPtr->typePtr->updateStringProc(objPtr);
	    }
	    if ((objPtr->typePtr->freeIntRepProc) != NULL) {
		(*objPtr->typePtr->freeIntRepProc)(objPtr);
	    }
	}
	objPtr->typePtr = &tclStringType;

	/*
	 * Allocate enough space for the basic String structure.
	 */

	stringPtr = (String *) ckalloc(sizeof(String));
	stringPtr->numChars = -1;
	stringPtr->uallocated = 0;

	if (objPtr->bytes != NULL) {
	    stringPtr->allocated = objPtr->length;	    
 	    objPtr->bytes[objPtr->length] = 0;
	} else {
	    objPtr->length = 0;
	}
	SET_STRING(objPtr, stringPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateStringOfString --
 *
 *	Update the string representation for an object whose internal
 *	representation is "String".
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The object's string may be set by converting its Unicode
 *	represention to UTF format.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateStringOfString(objPtr)
    Tcl_Obj *objPtr;		/* Object with string rep to update. */
{
    int i, length, size;
    Tcl_UniChar *unicode;
    char dummy[TCL_UTF_MAX];
    char *dst;
    String *stringPtr;

    stringPtr = GET_STRING(objPtr);
    if ((objPtr->bytes == NULL) || (stringPtr->allocated == 0)) {

	if (stringPtr->numChars <= 0) {

	    /*
	     * If there is no Unicode rep, or the string has 0 chars,
	     * then set the string rep to an empty string.
	     */

	    objPtr->bytes = tclEmptyStringRep;
	    objPtr->length = 0;
	    return;
	}

	unicode = stringPtr->unicode;
	length = stringPtr->numChars * sizeof(Tcl_UniChar);

	/*
	 * Translate the Unicode string to UTF.  "size" will hold the
	 * amount of space the UTF string needs.
	 */

	size = 0;
	for (i = 0; i < stringPtr->numChars; i++) {
	    size += Tcl_UniCharToUtf((int) unicode[i], dummy);
	}
	
	dst = (char *) ckalloc((unsigned) (size + 1));
	objPtr->bytes = dst;
	objPtr->length = size;
	stringPtr->allocated = size;

	for (i = 0; i < stringPtr->numChars; i++) {
	    dst += Tcl_UniCharToUtf(unicode[i], dst);
	}
	*dst = '\0';
    }
    return;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeStringInternalRep --
 *
 *	Deallocate the storage associated with a String data object's
 *	internal representation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees memory. 
 *
 *----------------------------------------------------------------------
 */

static void
FreeStringInternalRep(objPtr)
    Tcl_Obj *objPtr;		/* Object with internal rep to free. */
{
    ckfree((char *) GET_STRING(objPtr));
}
