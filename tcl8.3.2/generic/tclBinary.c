/* 
 * tclBinary.c --
 *
 *	This file contains the implementation of the "binary" Tcl built-in
 *	command and the Tcl binary data object.
 *
 * Copyright (c) 1997 by Sun Microsystems, Inc.
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <math.h>
#include "tclInt.h"
#include "tclPort.h"

/*
 * The following constants are used by GetFormatSpec to indicate various
 * special conditions in the parsing of a format specifier.
 */

#define BINARY_ALL -1		/* Use all elements in the argument. */
#define BINARY_NOCOUNT -2	/* No count was specified in format. */

/*
 * Prototypes for local procedures defined in this file:
 */

static void		DupByteArrayInternalRep _ANSI_ARGS_((Tcl_Obj *srcPtr,
			    Tcl_Obj *copyPtr));
static int		FormatNumber _ANSI_ARGS_((Tcl_Interp *interp, int type,
			    Tcl_Obj *src, unsigned char **cursorPtr));
static void		FreeByteArrayInternalRep _ANSI_ARGS_((Tcl_Obj *objPtr));
static int		GetFormatSpec _ANSI_ARGS_((char **formatPtr,
			    char *cmdPtr, int *countPtr));
static Tcl_Obj *	ScanNumber _ANSI_ARGS_((unsigned char *buffer, int type));
static int		SetByteArrayFromAny _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *objPtr));
static void		UpdateStringOfByteArray _ANSI_ARGS_((Tcl_Obj *listPtr));


/*
 * The following object type represents an array of bytes.  An array of
 * bytes is not equivalent to an internationalized string.  Conceptually, a
 * string is an array of 16-bit quantities organized as a sequence of properly
 * formed UTF-8 characters, while a ByteArray is an array of 8-bit quantities.
 * Accessor functions are provided to convert a ByteArray to a String or a
 * String to a ByteArray.  Two or more consecutive bytes in an array of bytes
 * may look like a single UTF-8 character if the array is casually treated as
 * a string.  But obtaining the String from a ByteArray is guaranteed to
 * produced properly formed UTF-8 sequences so that there is a one-to-one
 * map between bytes and characters.
 *
 * Converting a ByteArray to a String proceeds by casting each byte in the
 * array to a 16-bit quantity, treating that number as a Unicode character,
 * and storing the UTF-8 version of that Unicode character in the String.
 * For ByteArrays consisting entirely of values 1..127, the corresponding
 * String representation is the same as the ByteArray representation.
 *
 * Converting a String to a ByteArray proceeds by getting the Unicode
 * representation of each character in the String, casting it to a
 * byte by truncating the upper 8 bits, and then storing the byte in the
 * ByteArray.  Converting from ByteArray to String and back to ByteArray
 * is not lossy, but converting an arbitrary String to a ByteArray may be.
 */

Tcl_ObjType tclByteArrayType = {
    "bytearray",
    FreeByteArrayInternalRep,
    DupByteArrayInternalRep,
    UpdateStringOfByteArray,
    SetByteArrayFromAny
};

/*
 * The following structure is the internal rep for a ByteArray object.
 * Keeps track of how much memory has been used and how much has been
 * allocated for the byte array to enable growing and shrinking of the
 * ByteArray object with fewer mallocs.  
 */

typedef struct ByteArray {
    int used;			/* The number of bytes used in the byte
				 * array. */
    int allocated;		/* The amount of space actually allocated
				 * minus 1 byte. */
    unsigned char bytes[4];	/* The array of bytes.  The actual size of
				 * this field depends on the 'allocated' field
				 * above. */
} ByteArray;

#define BYTEARRAY_SIZE(len)	\
		((unsigned) (sizeof(ByteArray) - 4 + (len)))
#define GET_BYTEARRAY(objPtr) \
		((ByteArray *) (objPtr)->internalRep.otherValuePtr)
#define SET_BYTEARRAY(objPtr, baPtr) \
		(objPtr)->internalRep.otherValuePtr = (VOID *) (baPtr)


/*
 *---------------------------------------------------------------------------
 *
 * Tcl_NewByteArrayObj --
 *
 *	This procedure is creates a new ByteArray object and initializes
 *	it from the given array of bytes.
 *
 * Results:
 *	The newly create object is returned.  This object will have no
 *	initial string representation.  The returned object has a ref count
 *	of 0.
 *
 * Side effects:
 *	Memory allocated for new object and copy of byte array argument.
 *
 *---------------------------------------------------------------------------
 */

#ifdef TCL_MEM_DEBUG
#undef Tcl_NewByteArrayObj


Tcl_Obj *
Tcl_NewByteArrayObj(bytes, length)
    unsigned char *bytes;	/* The array of bytes used to initialize
				 * the new object. */
    int length;			/* Length of the array of bytes, which must
				 * be >= 0. */
{
    return Tcl_DbNewByteArrayObj(bytes, length, "unknown", 0);
}

#else /* if not TCL_MEM_DEBUG */

Tcl_Obj *
Tcl_NewByteArrayObj(bytes, length)
    unsigned char *bytes;	/* The array of bytes used to initialize
				 * the new object. */
    int length;			/* Length of the array of bytes, which must
				 * be >= 0. */
{
    Tcl_Obj *objPtr;

    TclNewObj(objPtr);
    Tcl_SetByteArrayObj(objPtr, bytes, length);
    return objPtr;
}
#endif /* TCL_MEM_DEBUG */

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_DbNewByteArrayObj --
 *
 *	This procedure is normally called when debugging: i.e., when
 *	TCL_MEM_DEBUG is defined. It is the same as the Tcl_NewByteArrayObj
 *	above except that it calls Tcl_DbCkalloc directly with the file name
 *	and line number from its caller. This simplifies debugging since then
 *	the checkmem command will report the correct file name and line number
 *	when reporting objects that haven't been freed.
 *
 *	When TCL_MEM_DEBUG is not defined, this procedure just returns the
 *	result of calling Tcl_NewByteArrayObj.
 *
 * Results:
 *	The newly create object is returned.  This object will have no
 *	initial string representation.  The returned object has a ref count
 *	of 0.
 *
 * Side effects:
 *	Memory allocated for new object and copy of byte array argument.
 *
 *---------------------------------------------------------------------------
 */

#ifdef TCL_MEM_DEBUG

Tcl_Obj *
Tcl_DbNewByteArrayObj(bytes, length, file, line)
    unsigned char *bytes;	/* The array of bytes used to initialize
				 * the new object. */
    int length;			/* Length of the array of bytes, which must
				 * be >= 0. */
    char *file;			/* The name of the source file calling this
				 * procedure; used for debugging. */
    int line;			/* Line number in the source file; used
				 * for debugging. */
{
    Tcl_Obj *objPtr;

    TclDbNewObj(objPtr, file, line);
    Tcl_SetByteArrayObj(objPtr, bytes, length);
    return objPtr;
}

#else /* if not TCL_MEM_DEBUG */

Tcl_Obj *
Tcl_DbNewByteArrayObj(bytes, length, file, line)
    unsigned char *bytes;	/* The array of bytes used to initialize
				 * the new object. */
    int length;			/* Length of the array of bytes, which must
				 * be >= 0. */
    char *file;			/* The name of the source file calling this
				 * procedure; used for debugging. */
    int line;			/* Line number in the source file; used
				 * for debugging. */
{
    return Tcl_NewByteArrayObj(bytes, length);
}
#endif /* TCL_MEM_DEBUG */

/*
 *---------------------------------------------------------------------------
 *
 * Tcl_SetByteArrayObj --
 *
 *	Modify an object to be a ByteArray object and to have the specified
 *	array of bytes as its value.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The object's old string rep and internal rep is freed.
 *	Memory allocated for copy of byte array argument.
 *
 *----------------------------------------------------------------------
 */

void
Tcl_SetByteArrayObj(objPtr, bytes, length)
    Tcl_Obj *objPtr;		/* Object to initialize as a ByteArray. */
    unsigned char *bytes;	/* The array of bytes to use as the new
				 * value. */
    int length;			/* Length of the array of bytes, which must
				 * be >= 0. */
{
    Tcl_ObjType *typePtr;
    ByteArray *byteArrayPtr;

    if (Tcl_IsShared(objPtr)) {
	panic("Tcl_SetByteArrayObj called with shared object");
    }
    typePtr = objPtr->typePtr;
    if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
	(*typePtr->freeIntRepProc)(objPtr);
    }
    Tcl_InvalidateStringRep(objPtr);

    byteArrayPtr = (ByteArray *) ckalloc(BYTEARRAY_SIZE(length));
    byteArrayPtr->used = length;
    byteArrayPtr->allocated = length;
    memcpy((VOID *) byteArrayPtr->bytes, (VOID *) bytes, (size_t) length);

    objPtr->typePtr = &tclByteArrayType;
    SET_BYTEARRAY(objPtr, byteArrayPtr);
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_GetByteArrayFromObj --
 *
 *	Attempt to get the array of bytes from the Tcl object.  If the
 *	object is not already a ByteArray object, an attempt will be
 *	made to convert it to one.
 *
 * Results:
 *	Pointer to array of bytes representing the ByteArray object.
 *
 * Side effects:
 *	Frees old internal rep.  Allocates memory for new internal rep.
 *
 *----------------------------------------------------------------------
 */

unsigned char *
Tcl_GetByteArrayFromObj(objPtr, lengthPtr)
    Tcl_Obj *objPtr;		/* The ByteArray object. */
    int *lengthPtr;		/* If non-NULL, filled with length of the
				 * array of bytes in the ByteArray object. */
{
    ByteArray *baPtr;
    
    SetByteArrayFromAny(NULL, objPtr);
    baPtr = GET_BYTEARRAY(objPtr);

    if (lengthPtr != NULL) {
	*lengthPtr = baPtr->used;
    }
    return (unsigned char *) baPtr->bytes;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_SetByteArrayLength --
 *
 *	This procedure changes the length of the byte array for this
 *	object.  Once the caller has set the length of the array, it
 *	is acceptable to directly modify the bytes in the array up until
 *	Tcl_GetStringFromObj() has been called on this object.
 *
 * Results:
 *	The new byte array of the specified length.
 *
 * Side effects:
 *	Allocates enough memory for an array of bytes of the requested
 *	size.  When growing the array, the old array is copied to the
 *	new array; new bytes are undefined.  When shrinking, the
 *	old array is truncated to the specified length.
 *
 *---------------------------------------------------------------------------
 */

unsigned char *
Tcl_SetByteArrayLength(objPtr, length)
    Tcl_Obj *objPtr;		/* The ByteArray object. */
    int length;			/* New length for internal byte array. */
{
    ByteArray *byteArrayPtr, *newByteArrayPtr;
    
    if (Tcl_IsShared(objPtr)) {
	panic("Tcl_SetObjLength called with shared object");
    }
    if (objPtr->typePtr != &tclByteArrayType) {
	SetByteArrayFromAny(NULL, objPtr);
    }

    byteArrayPtr = GET_BYTEARRAY(objPtr);
    if (length > byteArrayPtr->allocated) {
	newByteArrayPtr = (ByteArray *) ckalloc(BYTEARRAY_SIZE(length));
	newByteArrayPtr->used = length;
	newByteArrayPtr->allocated = length;
	memcpy((VOID *) newByteArrayPtr->bytes,
		(VOID *) byteArrayPtr->bytes, (size_t) byteArrayPtr->used);
	ckfree((char *) byteArrayPtr);
	byteArrayPtr = newByteArrayPtr;
	SET_BYTEARRAY(objPtr, byteArrayPtr);
    }
    Tcl_InvalidateStringRep(objPtr);
    byteArrayPtr->used = length;
    return byteArrayPtr->bytes;
}

/*
 *---------------------------------------------------------------------------
 *
 * SetByteArrayFromAny --
 *
 *	Generate the ByteArray internal rep from the string rep.
 *
 * Results:
 *	The return value is always TCL_OK.
 *
 * Side effects:
 *	A ByteArray object is stored as the internal rep of objPtr.
 *
 *---------------------------------------------------------------------------
 */

static int
SetByteArrayFromAny(interp, objPtr)
    Tcl_Interp *interp;		/* Not used. */
    Tcl_Obj *objPtr;		/* The object to convert to type ByteArray. */
{
    Tcl_ObjType *typePtr;
    int length;
    char *src, *srcEnd;
    unsigned char *dst;
    ByteArray *byteArrayPtr;
    Tcl_UniChar ch;
    
    typePtr = objPtr->typePtr;
    if (typePtr != &tclByteArrayType) {
	src = Tcl_GetStringFromObj(objPtr, &length);
	srcEnd = src + length;

	byteArrayPtr = (ByteArray *) ckalloc(BYTEARRAY_SIZE(length));
	for (dst = byteArrayPtr->bytes; src < srcEnd; ) {
	    src += Tcl_UtfToUniChar(src, &ch);
	    *dst++ = (unsigned char) ch;
	}

	byteArrayPtr->used = dst - byteArrayPtr->bytes;
	byteArrayPtr->allocated = length;

	if ((typePtr != NULL) && (typePtr->freeIntRepProc) != NULL) {
	    (*typePtr->freeIntRepProc)(objPtr);
	}
	objPtr->typePtr = &tclByteArrayType;
	SET_BYTEARRAY(objPtr, byteArrayPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeByteArrayInternalRep --
 *
 *	Deallocate the storage associated with a ByteArray data object's
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
FreeByteArrayInternalRep(objPtr)
    Tcl_Obj *objPtr;		/* Object with internal rep to free. */
{
    ckfree((char *) GET_BYTEARRAY(objPtr));
}

/*
 *---------------------------------------------------------------------------
 *
 * DupByteArrayInternalRep --
 *
 *	Initialize the internal representation of a ByteArray Tcl_Obj
 *	to a copy of the internal representation of an existing ByteArray
 *	object. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Allocates memory.
 *
 *---------------------------------------------------------------------------
 */

static void
DupByteArrayInternalRep(srcPtr, copyPtr)
    Tcl_Obj *srcPtr;		/* Object with internal rep to copy. */
    Tcl_Obj *copyPtr;		/* Object with internal rep to set. */
{
    int length;
    ByteArray *srcArrayPtr, *copyArrayPtr;    

    srcArrayPtr = GET_BYTEARRAY(srcPtr);
    length = srcArrayPtr->used;

    copyArrayPtr = (ByteArray *) ckalloc(BYTEARRAY_SIZE(length));
    copyArrayPtr->used = length;
    copyArrayPtr->allocated = length;
    memcpy((VOID *) copyArrayPtr->bytes, (VOID *) srcArrayPtr->bytes,
	    (size_t) length);
    SET_BYTEARRAY(copyPtr, copyArrayPtr);

    copyPtr->typePtr = &tclByteArrayType;
}

/*
 *---------------------------------------------------------------------------
 *
 * UpdateStringOfByteArray --
 *
 *	Update the string representation for a ByteArray data object.
 *	Note: This procedure does not invalidate an existing old string rep
 *	so storage will be lost if this has not already been done. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The object's string is set to a valid string that results from
 *	the ByteArray-to-string conversion.
 *
 *	The object becomes a string object -- the internal rep is
 *	discarded and the typePtr becomes NULL.
 *
 *---------------------------------------------------------------------------
 */

static void
UpdateStringOfByteArray(objPtr)
    Tcl_Obj *objPtr;		/* ByteArray object whose string rep to
				 * update. */
{
    int i, length, size;
    unsigned char *src;
    char *dst;
    ByteArray *byteArrayPtr;

    byteArrayPtr = GET_BYTEARRAY(objPtr);
    src = byteArrayPtr->bytes;
    length = byteArrayPtr->used;

    /*
     * How much space will string rep need?
     */
     
    size = length;
    for (i = 0; i < length; i++) {
	if ((src[i] == 0) || (src[i] > 127)) {
	    size++;
	}
    }

    dst = (char *) ckalloc((unsigned) (size + 1));
    objPtr->bytes = dst;
    objPtr->length = size;

    if (size == length) {
	memcpy((VOID *) dst, (VOID *) src, (size_t) size);
	dst[size] = '\0';
    } else {
	for (i = 0; i < length; i++) {
	    dst += Tcl_UniCharToUtf(src[i], dst);
	}
	*dst = '\0';
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_BinaryObjCmd --
 *
 *	This procedure implements the "binary" Tcl command.
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
Tcl_BinaryObjCmd(dummy, interp, objc, objv)
    ClientData dummy;		/* Not used. */
    Tcl_Interp *interp;		/* Current interpreter. */
    int objc;			/* Number of arguments. */
    Tcl_Obj *CONST objv[];	/* Argument objects. */
{
    int arg;			/* Index of next argument to consume. */
    int value = 0;		/* Current integer value to be packed.
				 * Initialized to avoid compiler warning. */
    char cmd;			/* Current format character. */
    int count;			/* Count associated with current format
				 * character. */
    char *format;		/* Pointer to current position in format
				 * string. */
    Tcl_Obj *resultPtr;		/* Object holding result buffer. */
    unsigned char *buffer;	/* Start of result buffer. */
    unsigned char *cursor;	/* Current position within result buffer. */
    unsigned char *maxPos;	/* Greatest position within result buffer that
				 * cursor has visited.*/
    char *errorString, *errorValue, *str;
    int offset, size, length, index;
    static char *options[] = { 
	"format",	"scan",		NULL 
    };
    enum options { 
	BINARY_FORMAT,	BINARY_SCAN
    };

    if (objc < 2) {
    	Tcl_WrongNumArgs(interp, 1, objv, "option ?arg arg ...?");
	return TCL_ERROR;
    }

    if (Tcl_GetIndexFromObj(interp, objv[1], options, "option", 0,
	    &index) != TCL_OK) {
    	return TCL_ERROR;
    }

    switch ((enum options) index) {
	case BINARY_FORMAT: {
	    if (objc < 3) {
		Tcl_WrongNumArgs(interp, 2, objv, "formatString ?arg arg ...?");
		return TCL_ERROR;
	    }

	    /*
	     * To avoid copying the data, we format the string in two passes.
	     * The first pass computes the size of the output buffer.  The
	     * second pass places the formatted data into the buffer.
	     */

	    format = Tcl_GetString(objv[2]);
	    arg = 3;
	    offset = 0;
	    length = 0;
	    while (*format != '\0') {
		str = format;
		if (!GetFormatSpec(&format, &cmd, &count)) {
		    break;
		}
		switch (cmd) {
		    case 'a':
		    case 'A':
		    case 'b':
		    case 'B':
		    case 'h':
		    case 'H': {
			/*
			 * For string-type specifiers, the count corresponds
			 * to the number of bytes in a single argument.
			 */

			if (arg >= objc) {
			    goto badIndex;
			}
			if (count == BINARY_ALL) {
			    Tcl_GetByteArrayFromObj(objv[arg], &count);
			} else if (count == BINARY_NOCOUNT) {
			    count = 1;
			}
			arg++;
			if (cmd == 'a' || cmd == 'A') {
			    offset += count;
			} else if (cmd == 'b' || cmd == 'B') {
			    offset += (count + 7) / 8;
			} else {
			    offset += (count + 1) / 2;
			}
			break;
		    }
		    case 'c': {
			size = 1;
			goto doNumbers;
		    }
		    case 's':
		    case 'S': {
			size = 2;
			goto doNumbers;
		    }
		    case 'i':
		    case 'I': {
			size = 4;
			goto doNumbers;
		    }
		    case 'f': {
			size = sizeof(float);
			goto doNumbers;
		    }
		    case 'd': {
			size = sizeof(double);
			
			doNumbers:
			if (arg >= objc) {
			    goto badIndex;
			}

			/*
			 * For number-type specifiers, the count corresponds
			 * to the number of elements in the list stored in
			 * a single argument.  If no count is specified, then
			 * the argument is taken as a single non-list value.
			 */

			if (count == BINARY_NOCOUNT) {
			    arg++;
			    count = 1;
			} else {
			    int listc;
			    Tcl_Obj **listv;
			    if (Tcl_ListObjGetElements(interp, objv[arg++],
				    &listc, &listv) != TCL_OK) {
				return TCL_ERROR;
			    }
			    if (count == BINARY_ALL) {
				count = listc;
			    } else if (count > listc) {
			        Tcl_AppendResult(interp, 
					"number of elements in list does not match count",
					(char *) NULL);
				return TCL_ERROR;
			    }
			}
			offset += count*size;
			break;
		    }
		    case 'x': {
			if (count == BINARY_ALL) {
			    Tcl_AppendResult(interp, 
				    "cannot use \"*\" in format string with \"x\"",
				    (char *) NULL);
			    return TCL_ERROR;
			} else if (count == BINARY_NOCOUNT) {
			    count = 1;
			}
			offset += count;
			break;
		    }
		    case 'X': {
			if (count == BINARY_NOCOUNT) {
			    count = 1;
			}
			if ((count > offset) || (count == BINARY_ALL)) {
			    count = offset;
			}
			if (offset > length) {
			    length = offset;
			}
			offset -= count;
			break;
		    }
		    case '@': {
			if (offset > length) {
			    length = offset;
			}
			if (count == BINARY_ALL) {
			    offset = length;
			} else if (count == BINARY_NOCOUNT) {
			    goto badCount;
			} else {
			    offset = count;
			}
			break;
		    }
		    default: {
			errorString = str;
			goto badfield;
		    }
		}
	    }
	    if (offset > length) {
		length = offset;
	    }
	    if (length == 0) {
		return TCL_OK;
	    }

	    /*
	     * Prepare the result object by preallocating the caclulated
	     * number of bytes and filling with nulls.
	     */

	    resultPtr = Tcl_GetObjResult(interp);
	    buffer = Tcl_SetByteArrayLength(resultPtr, length);
	    memset((VOID *) buffer, 0, (size_t) length);

	    /*
	     * Pack the data into the result object.  Note that we can skip
	     * the error checking during this pass, since we have already
	     * parsed the string once.
	     */

	    arg = 3;
	    format = Tcl_GetString(objv[2]);
	    cursor = buffer;
	    maxPos = cursor;
	    while (*format != 0) {
		if (!GetFormatSpec(&format, &cmd, &count)) {
		    break;
		}
		if ((count == 0) && (cmd != '@')) {
		    arg++;
		    continue;
		}
		switch (cmd) {
		    case 'a':
		    case 'A': {
			char pad = (char) (cmd == 'a' ? '\0' : ' ');
			unsigned char *bytes;

			bytes = Tcl_GetByteArrayFromObj(objv[arg++], &length);

			if (count == BINARY_ALL) {
			    count = length;
			} else if (count == BINARY_NOCOUNT) {
			    count = 1;
			}
			if (length >= count) {
			    memcpy((VOID *) cursor, (VOID *) bytes,
				    (size_t) count);
			} else {
			    memcpy((VOID *) cursor, (VOID *) bytes,
				    (size_t) length);
			    memset((VOID *) (cursor + length), pad,
			            (size_t) (count - length));
			}
			cursor += count;
			break;
		    }
		    case 'b':
		    case 'B': {
			unsigned char *last;
			
			str = Tcl_GetStringFromObj(objv[arg++], &length);
			if (count == BINARY_ALL) {
			    count = length;
			} else if (count == BINARY_NOCOUNT) {
			    count = 1;
			}
			last = cursor + ((count + 7) / 8);
			if (count > length) {
			    count = length;
			}
			value = 0;
			errorString = "binary";
			if (cmd == 'B') {
			    for (offset = 0; offset < count; offset++) {
				value <<= 1;
				if (str[offset] == '1') {
				    value |= 1;
				} else if (str[offset] != '0') {
				    errorValue = str;
				    goto badValue;
				}
				if (((offset + 1) % 8) == 0) {
				    *cursor++ = (unsigned char) value;
				    value = 0;
				}
			    }
			} else {
			    for (offset = 0; offset < count; offset++) {
				value >>= 1;
				if (str[offset] == '1') {
				    value |= 128;
				} else if (str[offset] != '0') {
				    errorValue = str;
				    goto badValue;
				}
				if (!((offset + 1) % 8)) {
				    *cursor++ = (unsigned char) value;
				    value = 0;
				}
			    }
			}
			if ((offset % 8) != 0) {
			    if (cmd == 'B') {
				value <<= 8 - (offset % 8);
			    } else {
				value >>= 8 - (offset % 8);
			    }
			    *cursor++ = (unsigned char) value;
			}
			while (cursor < last) {
			    *cursor++ = '\0';
			}
			break;
		    }
		    case 'h':
		    case 'H': {
			unsigned char *last;
			int c;
			
			str = Tcl_GetStringFromObj(objv[arg++], &length);
			if (count == BINARY_ALL) {
			    count = length;
			} else if (count == BINARY_NOCOUNT) {
			    count = 1;
			}
			last = cursor + ((count + 1) / 2);
			if (count > length) {
			    count = length;
			}
			value = 0;
			errorString = "hexadecimal";
			if (cmd == 'H') {
			    for (offset = 0; offset < count; offset++) {
				value <<= 4;
				if (!isxdigit(UCHAR(str[offset]))) { /* INTL: digit */
				    errorValue = str;
				    goto badValue;
				}
				c = str[offset] - '0';
				if (c > 9) {
				    c += ('0' - 'A') + 10;
				}
				if (c > 16) {
				    c += ('A' - 'a');
				}
				value |= (c & 0xf);
				if (offset % 2) {
				    *cursor++ = (char) value;
				    value = 0;
				}
			    }
			} else {
			    for (offset = 0; offset < count; offset++) {
				value >>= 4;

				if (!isxdigit(UCHAR(str[offset]))) { /* INTL: digit */
				    errorValue = str;
				    goto badValue;
				}
				c = str[offset] - '0';
				if (c > 9) {
				    c += ('0' - 'A') + 10;
				}
				if (c > 16) {
				    c += ('A' - 'a');
				}
				value |= ((c << 4) & 0xf0);
				if (offset % 2) {
				    *cursor++ = (unsigned char)(value & 0xff);
				    value = 0;
				}
			    }
			}
			if (offset % 2) {
			    if (cmd == 'H') {
				value <<= 4;
			    } else {
				value >>= 4;
			    }
			    *cursor++ = (unsigned char) value;
			}

			while (cursor < last) {
			    *cursor++ = '\0';
			}
			break;
		    }
		    case 'c':
		    case 's':
		    case 'S':
		    case 'i':
		    case 'I':
		    case 'd':
		    case 'f': {
			int listc, i;
			Tcl_Obj **listv;

			if (count == BINARY_NOCOUNT) {
			    /*
			     * Note that we are casting away the const-ness of
			     * objv, but this is safe since we aren't going to
			     * modify the array.
			     */

			    listv = (Tcl_Obj**)(objv + arg);
			    listc = 1;
			    count = 1;
			} else {
			    Tcl_ListObjGetElements(interp, objv[arg],
				    &listc, &listv);
			    if (count == BINARY_ALL) {
				count = listc;
			    }
			}
			arg++;
			for (i = 0; i < count; i++) {
			    if (FormatNumber(interp, cmd, listv[i], &cursor)
				    != TCL_OK) {
				return TCL_ERROR;
			    }
			}
			break;
		    }
		    case 'x': {
			if (count == BINARY_NOCOUNT) {
			    count = 1;
			}
			memset(cursor, 0, (size_t) count);
			cursor += count;
			break;
		    }
		    case 'X': {
			if (cursor > maxPos) {
			    maxPos = cursor;
			}
			if (count == BINARY_NOCOUNT) {
			    count = 1;
			}
			if ((count == BINARY_ALL)
				|| (count > (cursor - buffer))) {
			    cursor = buffer;
			} else {
			    cursor -= count;
			}
			break;
		    }
		    case '@': {
			if (cursor > maxPos) {
			    maxPos = cursor;
			}
			if (count == BINARY_ALL) {
			    cursor = maxPos;
			} else {
			    cursor = buffer + count;
			}
			break;
		    }
		}
	    }
	    break;
	}
	case BINARY_SCAN: {
	    int i;
	    Tcl_Obj *valuePtr, *elementPtr;

	    if (objc < 4) {
		Tcl_WrongNumArgs(interp, 2, objv,
			"value formatString ?varName varName ...?");
		return TCL_ERROR;
	    }
	    buffer = Tcl_GetByteArrayFromObj(objv[2], &length);
	    format = Tcl_GetString(objv[3]);
	    cursor = buffer;
	    arg = 4;
	    offset = 0;
	    while (*format != '\0') {
		str = format;
		if (!GetFormatSpec(&format, &cmd, &count)) {
		    goto done;
		}
		switch (cmd) {
		    case 'a':
		    case 'A': {
			unsigned char *src;

			if (arg >= objc) {
			    goto badIndex;
			}
			if (count == BINARY_ALL) {
			    count = length - offset;
			} else {
			    if (count == BINARY_NOCOUNT) {
				count = 1;
			    }
			    if (count > (length - offset)) {
				goto done;
			    }
			}

			src = buffer + offset;
			size = count;

			/*
			 * Trim trailing nulls and spaces, if necessary.
			 */

			if (cmd == 'A') {
			    while (size > 0) {
				if (src[size-1] != '\0' && src[size-1] != ' ') {
				    break;
				}
				size--;
			    }
			}
			valuePtr = Tcl_NewByteArrayObj(src, size);
			resultPtr = Tcl_ObjSetVar2(interp, objv[arg],
				NULL, valuePtr, TCL_LEAVE_ERR_MSG);
			arg++;
			if (resultPtr == NULL) {
			    Tcl_DecrRefCount(valuePtr);	/* unneeded */
			    return TCL_ERROR;
			}
			offset += count;
			break;
		    }
		    case 'b':
		    case 'B': {
			unsigned char *src;
			char *dest;

			if (arg >= objc) {
			    goto badIndex;
			}
			if (count == BINARY_ALL) {
			    count = (length - offset) * 8;
			} else {
			    if (count == BINARY_NOCOUNT) {
				count = 1;
			    }
			    if (count > (length - offset) * 8) {
				goto done;
			    }
			}
			src = buffer + offset;
			valuePtr = Tcl_NewObj();
			Tcl_SetObjLength(valuePtr, count);
			dest = Tcl_GetString(valuePtr);

			if (cmd == 'b') {
			    for (i = 0; i < count; i++) {
				if (i % 8) {
				    value >>= 1;
				} else {
				    value = *src++;
				}
				*dest++ = (char) ((value & 1) ? '1' : '0');
			    }
			} else {
			    for (i = 0; i < count; i++) {
				if (i % 8) {
				    value <<= 1;
				} else {
				    value = *src++;
				}
				*dest++ = (char) ((value & 0x80) ? '1' : '0');
			    }
			}
			
			resultPtr = Tcl_ObjSetVar2(interp, objv[arg],
				NULL, valuePtr, TCL_LEAVE_ERR_MSG);
			arg++;
			if (resultPtr == NULL) {
			    Tcl_DecrRefCount(valuePtr);	/* unneeded */
			    return TCL_ERROR;
			}
			offset += (count + 7 ) / 8;
			break;
		    }
		    case 'h':
		    case 'H': {
			char *dest;
			unsigned char *src;
			int i;
			static char hexdigit[] = "0123456789abcdef";

			if (arg >= objc) {
			    goto badIndex;
			}
			if (count == BINARY_ALL) {
			    count = (length - offset)*2;
			} else {
			    if (count == BINARY_NOCOUNT) {
				count = 1;
			    }
			    if (count > (length - offset)*2) {
				goto done;
			    }
			}
			src = buffer + offset;
			valuePtr = Tcl_NewObj();
			Tcl_SetObjLength(valuePtr, count);
			dest = Tcl_GetString(valuePtr);

			if (cmd == 'h') {
			    for (i = 0; i < count; i++) {
				if (i % 2) {
				    value >>= 4;
				} else {
				    value = *src++;
				}
				*dest++ = hexdigit[value & 0xf];
			    }
			} else {
			    for (i = 0; i < count; i++) {
				if (i % 2) {
				    value <<= 4;
				} else {
				    value = *src++;
				}
				*dest++ = hexdigit[(value >> 4) & 0xf];
			    }
			}
			
			resultPtr = Tcl_ObjSetVar2(interp, objv[arg],
				NULL, valuePtr, TCL_LEAVE_ERR_MSG);
			arg++;
			if (resultPtr == NULL) {
			    Tcl_DecrRefCount(valuePtr);	/* unneeded */
			    return TCL_ERROR;
			}
			offset += (count + 1) / 2;
			break;
		    }
		    case 'c': {
			size = 1;
			goto scanNumber;
		    }
		    case 's':
		    case 'S': {
			size = 2;
			goto scanNumber;
		    }
		    case 'i':
		    case 'I': {
			size = 4;
			goto scanNumber;
		    }
		    case 'f': {
			size = sizeof(float);
			goto scanNumber;
		    }
		    case 'd': {
			unsigned char *src;

			size = sizeof(double);
			/* fall through */
			
			scanNumber:
			if (arg >= objc) {
			    goto badIndex;
			}
			if (count == BINARY_NOCOUNT) {
			    if ((length - offset) < size) {
				goto done;
			    }
			    valuePtr = ScanNumber(buffer+offset, cmd);
			    offset += size;
			} else {
			    if (count == BINARY_ALL) {
				count = (length - offset) / size;
			    }
			    if ((length - offset) < (count * size)) {
				goto done;
			    }
			    valuePtr = Tcl_NewObj();
			    src = buffer+offset;
			    for (i = 0; i < count; i++) {
				elementPtr = ScanNumber(src, cmd);
				src += size;
				Tcl_ListObjAppendElement(NULL, valuePtr,
					elementPtr);
			    }
			    offset += count*size;
			}

			resultPtr = Tcl_ObjSetVar2(interp, objv[arg],
				NULL, valuePtr, TCL_LEAVE_ERR_MSG);
			arg++;
			if (resultPtr == NULL) {
			    Tcl_DecrRefCount(valuePtr);	/* unneeded */
			    return TCL_ERROR;
			}
			break;
		    }
		    case 'x': {
			if (count == BINARY_NOCOUNT) {
			    count = 1;
			}
			if ((count == BINARY_ALL)
				|| (count > (length - offset))) {
			    offset = length;
			} else {
			    offset += count;
			}
			break;
		    }
		    case 'X': {
			if (count == BINARY_NOCOUNT) {
			    count = 1;
			}
			if ((count == BINARY_ALL) || (count > offset)) {
			    offset = 0;
			} else {
			    offset -= count;
			}
			break;
		    }
		    case '@': {
			if (count == BINARY_NOCOUNT) {
			    goto badCount;
			}
			if ((count == BINARY_ALL) || (count > length)) {
			    offset = length;
			} else {
			    offset = count;
			}
			break;
		    }
		    default: {
			errorString = str;
			goto badfield;
		    }
		}
	    }

	    /*
	     * Set the result to the last position of the cursor.
	     */

	    done:
	    Tcl_ResetResult(interp);
	    Tcl_SetLongObj(Tcl_GetObjResult(interp), arg - 4);
	    break;
	}
    }
    return TCL_OK;

    badValue:
    Tcl_ResetResult(interp);
    Tcl_AppendStringsToObj(Tcl_GetObjResult(interp), "expected ", errorString,
	    " string but got \"", errorValue, "\" instead", NULL);
    return TCL_ERROR;

    badCount:
    errorString = "missing count for \"@\" field specifier";
    goto error;

    badIndex:
    errorString = "not enough arguments for all format specifiers";
    goto error;

    badfield: {
	Tcl_UniChar ch;
	char buf[TCL_UTF_MAX + 1];

	Tcl_UtfToUniChar(errorString, &ch);
	buf[Tcl_UniCharToUtf(ch, buf)] = '\0';
	Tcl_AppendResult(interp, "bad field specifier \"", buf, "\"", NULL);
	return TCL_ERROR;
    }

    error:
    Tcl_AppendResult(interp, errorString, NULL);
    return TCL_ERROR;
}

/*
 *----------------------------------------------------------------------
 *
 * GetFormatSpec --
 *
 *	This function parses the format strings used in the binary
 *	format and scan commands.
 *
 * Results:
 *	Moves the formatPtr to the start of the next command. Returns
 *	the current command character and count in cmdPtr and countPtr.
 *	The count is set to BINARY_ALL if the count character was '*'
 *	or BINARY_NOCOUNT if no count was specified.  Returns 1 on
 *	success, or 0 if the string did not have a format specifier.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static int
GetFormatSpec(formatPtr, cmdPtr, countPtr)
    char **formatPtr;		/* Pointer to format string. */
    char *cmdPtr;		/* Pointer to location of command char. */
    int *countPtr;		/* Pointer to repeat count value. */
{
    /*
     * Skip any leading blanks.
     */

    while (**formatPtr == ' ') {
	(*formatPtr)++;
    }

    /*
     * The string was empty, except for whitespace, so fail.
     */

    if (!(**formatPtr)) {
	return 0;
    }

    /*
     * Extract the command character and any trailing digits or '*'.
     */

    *cmdPtr = **formatPtr;
    (*formatPtr)++;
    if (**formatPtr == '*') {
	(*formatPtr)++;
	(*countPtr) = BINARY_ALL;
    } else if (isdigit(UCHAR(**formatPtr))) { /* INTL: digit */
	(*countPtr) = strtoul(*formatPtr, formatPtr, 10);
    } else {
	(*countPtr) = BINARY_NOCOUNT;
    }
    return 1;
}

/*
 *----------------------------------------------------------------------
 *
 * FormatNumber --
 *
 *	This routine is called by Tcl_BinaryObjCmd to format a number
 *	into a location pointed at by cursor.
 *
 * Results:
 *	 A standard Tcl result.
 *
 * Side effects:
 *	Moves the cursor to the next location to be written into.
 *
 *----------------------------------------------------------------------
 */

static int
FormatNumber(interp, type, src, cursorPtr)
    Tcl_Interp *interp;		/* Current interpreter, used to report
				 * errors. */
    int type;			/* Type of number to format. */
    Tcl_Obj *src;		/* Number to format. */
    unsigned char **cursorPtr;	/* Pointer to index into destination buffer. */
{
    int value;
    double dvalue;

    if ((type == 'd') || (type == 'f')) {
	/*
	 * For floating point types, we need to copy the data using
	 * memcpy to avoid alignment issues.
	 */

	if (Tcl_GetDoubleFromObj(interp, src, &dvalue) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (type == 'd') {
	    memcpy((VOID *) *cursorPtr, (VOID *) &dvalue, sizeof(double));
	    *cursorPtr += sizeof(double);
	} else {
	    float fvalue;

	    /*
	     * Because some compilers will generate floating point exceptions
	     * on an overflow cast (e.g. Borland), we restrict the values
	     * to the valid range for float.
	     */

	    if (fabs(dvalue) > (double)FLT_MAX) {
		fvalue = (dvalue >= 0.0) ? FLT_MAX : -FLT_MAX;
	    } else {
		fvalue = (float) dvalue;
	    }
	    memcpy((VOID *) *cursorPtr, (VOID *) &fvalue, sizeof(float));
	    *cursorPtr += sizeof(float);
	}
    } else {
	if (Tcl_GetIntFromObj(interp, src, &value) != TCL_OK) {
	    return TCL_ERROR;
	}
	if (type == 'c') {
	    *(*cursorPtr)++ = (unsigned char) value;
	} else if (type == 's') {
	    *(*cursorPtr)++ = (unsigned char) value;
	    *(*cursorPtr)++ = (unsigned char) (value >> 8);
	} else if (type == 'S') {
	    *(*cursorPtr)++ = (unsigned char) (value >> 8);
	    *(*cursorPtr)++ = (unsigned char) value;
	} else if (type == 'i') {
	    *(*cursorPtr)++ = (unsigned char) value;
	    *(*cursorPtr)++ = (unsigned char) (value >> 8);
	    *(*cursorPtr)++ = (unsigned char) (value >> 16);
	    *(*cursorPtr)++ = (unsigned char) (value >> 24);
	} else if (type == 'I') {
	    *(*cursorPtr)++ = (unsigned char) (value >> 24);
	    *(*cursorPtr)++ = (unsigned char) (value >> 16);
	    *(*cursorPtr)++ = (unsigned char) (value >> 8);
	    *(*cursorPtr)++ = (unsigned char) value;
	}
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * ScanNumber --
 *
 *	This routine is called by Tcl_BinaryObjCmd to scan a number
 *	out of a buffer.
 *
 * Results:
 *	Returns a newly created object containing the scanned number.
 *	This object has a ref count of zero.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

static Tcl_Obj *
ScanNumber(buffer, type)
    unsigned char *buffer;	/* Buffer to scan number from. */
    int type;			/* Format character from "binary scan" */
{
    long value;

    /*
     * We cannot rely on the compiler to properly sign extend integer values
     * when we cast from smaller values to larger values because we don't know
     * the exact size of the integer types.  So, we have to handle sign
     * extension explicitly by checking the high bit and padding with 1's as
     * needed.
     */

    switch (type) {
	case 'c': {
	    /*
	     * Characters need special handling.  We want to produce a
	     * signed result, but on some platforms (such as AIX) chars
	     * are unsigned.  To deal with this, check for a value that
	     * should be negative but isn't.
	     */

	    value = buffer[0];
	    if (value & 0x80) {
		value |= -0x100;
	    }
	    return Tcl_NewLongObj((long)value);
	}
	case 's': {
	    value = (long) (buffer[0] + (buffer[1] << 8));
	    goto shortValue;
	}
	case 'S': {
	    value = (long) (buffer[1] + (buffer[0] << 8));
	    shortValue:
	    if (value & 0x8000) {
		value |= -0x10000;
	    }
	    return Tcl_NewLongObj(value);
	}
	case 'i': {
	    value = (long) (buffer[0] 
		    + (buffer[1] << 8)
		    + (buffer[2] << 16)
		    + (buffer[3] << 24));
	    goto intValue;
	}
	case 'I': {
	    value = (long) (buffer[3]
		    + (buffer[2] << 8)
		    + (buffer[1] << 16)
		    + (buffer[0] << 24));
	    intValue:
	    /*
	     * Check to see if the value was sign extended properly on
	     * systems where an int is more than 32-bits.
	     */

	    if ((value & (((unsigned int)1)<<31)) && (value > 0)) {
		value -= (((unsigned int)1)<<31);
		value -= (((unsigned int)1)<<31);
	    }
	    return Tcl_NewLongObj(value);
	}
	case 'f': {
	    float fvalue;
	    memcpy((VOID *) &fvalue, (VOID *) buffer, sizeof(float));
	    return Tcl_NewDoubleObj(fvalue);
	}
	case 'd': {
	    double dvalue;
	    memcpy((VOID *) &dvalue, (VOID *) buffer, sizeof(double));
	    return Tcl_NewDoubleObj(dvalue);
	}
    }
    return NULL;
}
