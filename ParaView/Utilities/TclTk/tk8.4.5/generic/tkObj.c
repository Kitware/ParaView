/* 
 * tkObj.c --
 *
 *	This file contains procedures that implement the common Tk object
 *	types
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkInt.h"

/*
 * The following structure is the internal representation for pixel objects.
 */
 
typedef struct PixelRep {
    double value;
    int units;
    Tk_Window tkwin;
    int returnValue;
} PixelRep;

#define SIMPLE_PIXELREP(objPtr)				\
    ((objPtr)->internalRep.twoPtrValue.ptr2 == 0)

#define SET_SIMPLEPIXEL(objPtr, intval)			\
    (objPtr)->internalRep.twoPtrValue.ptr1 = (VOID *) (intval);	\
    (objPtr)->internalRep.twoPtrValue.ptr2 = 0

#define GET_SIMPLEPIXEL(objPtr)				\
    ((int) (objPtr)->internalRep.twoPtrValue.ptr1)

#define SET_COMPLEXPIXEL(objPtr, repPtr)		\
    (objPtr)->internalRep.twoPtrValue.ptr1 = 0;		\
    (objPtr)->internalRep.twoPtrValue.ptr2 = (VOID *) repPtr

#define GET_COMPLEXPIXEL(objPtr)			\
    ((PixelRep *) (objPtr)->internalRep.twoPtrValue.ptr2)


/*
 * The following structure is the internal representation for mm objects.
 */
 
typedef struct MMRep {
    double value;
    int units;
    Tk_Window tkwin;
    double returnValue;
} MMRep;

/*
 * The following structure is the internal representation for window objects.
 * A WindowRep caches name-to-window lookups.  The cache is invalid
 * if tkwin is NULL or if mainPtr->deletionEpoch does not match epoch.
 */
typedef struct WindowRep {
    Tk_Window tkwin;		/* Cached window; NULL if not found */
    TkMainInfo *mainPtr;	/* MainWindow associated with tkwin */
    long epoch;			/* Value of mainPtr->deletionEpoch at last
				 * successful lookup.  */
} WindowRep;

/*
 * Prototypes for procedures defined later in this file:
 */

static void		DupMMInternalRep _ANSI_ARGS_((Tcl_Obj *srcPtr,
			    Tcl_Obj *copyPtr));
static void		DupPixelInternalRep _ANSI_ARGS_((Tcl_Obj *srcPtr,
			    Tcl_Obj *copyPtr));
static void		DupWindowInternalRep _ANSI_ARGS_((Tcl_Obj *srcPtr,
			    Tcl_Obj *copyPtr));
static void		FreeMMInternalRep _ANSI_ARGS_((Tcl_Obj *objPtr));
static void		FreePixelInternalRep _ANSI_ARGS_((Tcl_Obj *objPtr));
static void		FreeWindowInternalRep _ANSI_ARGS_((Tcl_Obj *objPtr));
static void		UpdateStringOfMM _ANSI_ARGS_((Tcl_Obj *objPtr));
static int		SetMMFromAny _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *objPtr));
static int		SetPixelFromAny _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *objPtr));
static int		SetWindowFromAny _ANSI_ARGS_((Tcl_Interp *interp,
			    Tcl_Obj *objPtr));

/*
 * The following structure defines the implementation of the "pixel"
 * Tcl object, used for measuring distances.  The pixel object remembers
 * its initial display-independant settings.
 */

static Tcl_ObjType pixelObjType = {
    "pixel",			/* name */
    FreePixelInternalRep,	/* freeIntRepProc */
    DupPixelInternalRep,	/* dupIntRepProc */
    NULL,			/* updateStringProc */
    SetPixelFromAny		/* setFromAnyProc */
};

/*
 * The following structure defines the implementation of the "pixel"
 * Tcl object, used for measuring distances.  The pixel object remembers
 * its initial display-independant settings.
 */

static Tcl_ObjType mmObjType = {
    "mm",			/* name */
    FreeMMInternalRep,		/* freeIntRepProc */
    DupMMInternalRep,		/* dupIntRepProc */
    UpdateStringOfMM,		/* updateStringProc */
    SetMMFromAny		/* setFromAnyProc */
};

/*
 * The following structure defines the implementation of the "window"
 * Tcl object.
 */

static Tcl_ObjType windowObjType = {
    "window",				/* name */
    FreeWindowInternalRep,		/* freeIntRepProc */
    DupWindowInternalRep,		/* dupIntRepProc */
    NULL,				/* updateStringProc */
    SetWindowFromAny			/* setFromAnyProc */
};



/*
 *----------------------------------------------------------------------
 *
 * Tk_GetPixelsFromObj --
 *
 *	Attempt to return a pixel value from the Tcl object "objPtr". If the
 *	object is not already a pixel value, an attempt will be made to convert
 *	it to one.
 *
 * Results:
 *	The return value is a standard Tcl object result. If an error occurs
 *	during conversion, an error message is left in the interpreter's
 *	result unless "interp" is NULL.
 *
 * Side effects:
 *	If the object is not already a pixel, the conversion will free
 *	any old internal representation. 
 *
 *----------------------------------------------------------------------
 */

int
Tk_GetPixelsFromObj(interp, tkwin, objPtr, intPtr)
    Tcl_Interp *interp; 	/* Used for error reporting if not NULL. */
    Tk_Window tkwin;
    Tcl_Obj *objPtr;		/* The object from which to get pixels. */
    int *intPtr;		/* Place to store resulting pixels. */
{
    int result;
    double d;
    PixelRep *pixelPtr;
    static double bias[] = {
	1.0,	10.0,	25.4,	25.4 / 72.0
    };

    if (objPtr->typePtr != &pixelObjType) {
	result = SetPixelFromAny(interp, objPtr);
	if (result != TCL_OK) {
	    return result;
	}
    }

    if (SIMPLE_PIXELREP(objPtr)) {
	*intPtr = GET_SIMPLEPIXEL(objPtr);
    } else {
	pixelPtr = GET_COMPLEXPIXEL(objPtr);
	if (pixelPtr->tkwin != tkwin) {
	    d = pixelPtr->value;
	    if (pixelPtr->units >= 0) {
		d *= bias[pixelPtr->units] * WidthOfScreen(Tk_Screen(tkwin));
		d /= WidthMMOfScreen(Tk_Screen(tkwin));
	    }
	    if (d < 0) {
		pixelPtr->returnValue = (int) (d - 0.5);
	    } else {
		pixelPtr->returnValue = (int) (d + 0.5);
	    }
	    pixelPtr->tkwin = tkwin;
	}
        *intPtr = pixelPtr->returnValue;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FreePixelInternalRep --
 *
 *	Deallocate the storage associated with a pixel object's internal
 *	representation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees objPtr's internal representation and sets objPtr's
 *	internalRep to NULL.
 *
 *----------------------------------------------------------------------
 */

static void
FreePixelInternalRep(objPtr)
    Tcl_Obj *objPtr;		/* Pixel object with internal rep to free. */
{
    PixelRep *pixelPtr;
    
    if (!SIMPLE_PIXELREP(objPtr)) {
	pixelPtr = GET_COMPLEXPIXEL(objPtr);
	ckfree((char *) pixelPtr);
    }
    SET_SIMPLEPIXEL(objPtr, 0);
    objPtr->typePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DupPixelInternalRep --
 *
 *	Initialize the internal representation of a pixel Tcl_Obj to a
 *	copy of the internal representation of an existing pixel object. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	copyPtr's internal rep is set to the pixel corresponding to
 *	srcPtr's internal rep.
 *
 *----------------------------------------------------------------------
 */

static void
DupPixelInternalRep(srcPtr, copyPtr)
    register Tcl_Obj *srcPtr;	/* Object with internal rep to copy. */
    register Tcl_Obj *copyPtr;	/* Object with internal rep to set. */
{
    PixelRep *oldPtr, *newPtr;
    
    copyPtr->typePtr = srcPtr->typePtr;

    if (SIMPLE_PIXELREP(srcPtr)) {
	SET_SIMPLEPIXEL(copyPtr, GET_SIMPLEPIXEL(srcPtr));
    } else {
	oldPtr = GET_COMPLEXPIXEL(srcPtr);
	newPtr = (PixelRep *) ckalloc(sizeof(PixelRep));
	newPtr->value = oldPtr->value;
	newPtr->units = oldPtr->units;
	newPtr->tkwin = oldPtr->tkwin;
	newPtr->returnValue = oldPtr->returnValue;
	SET_COMPLEXPIXEL(copyPtr, newPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * SetPixelFromAny --
 *
 *	Attempt to generate a pixel internal form for the Tcl object
 *	"objPtr".
 *
 * Results:
 *	The return value is a standard Tcl result. If an error occurs during
 *	conversion, an error message is left in the interpreter's result
 *	unless "interp" is NULL.
 *
 * Side effects:
 *	If no error occurs, a pixel representation of the object is
 *	stored internally and the type of "objPtr" is set to pixel.
 *
 *----------------------------------------------------------------------
 */

static int
SetPixelFromAny(interp, objPtr)
    Tcl_Interp *interp;		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr;		/* The object to convert. */
{
    Tcl_ObjType *typePtr;
    char *string, *rest;
    double d;
    int i, units;
    PixelRep *pixelPtr;

    string = Tcl_GetStringFromObj(objPtr, NULL);

    d = strtod(string, &rest);
    if (rest == string) {
	/*
	 * Must copy string before resetting the result in case a caller
	 * is trying to convert the interpreter's result to pixels.
	 */

	char buf[100];

	error:
	sprintf(buf, "bad screen distance \"%.50s\"", string);
	Tcl_ResetResult(interp);
	Tcl_AppendResult(interp, buf, NULL);
	return TCL_ERROR;
    }
    while ((*rest != '\0') && isspace(UCHAR(*rest))) {
	rest++;
    }
    switch (*rest) {
	case '\0':
	    units = -1;
	    break;

	case 'm':
	    units = 0;
	    break;

	case 'c':
	    units = 1;
	    break;

	case 'i':
	    units = 2;
	    break;

	case 'p':
	    units = 3;
	    break;

	default:
	    goto error;
    }

    /*
     * Free the old internalRep before setting the new one. 
     */

    typePtr = objPtr->typePtr;
    if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
	(*typePtr->freeIntRepProc)(objPtr);
    }

    objPtr->typePtr = &pixelObjType;

    i = (int) d;
    if ((units < 0) && (i == d)) {
	SET_SIMPLEPIXEL(objPtr, i);
    } else {
	pixelPtr = (PixelRep *) ckalloc(sizeof(PixelRep));
	pixelPtr->value = d;
	pixelPtr->units = units;
	pixelPtr->tkwin = NULL;
	pixelPtr->returnValue = i;
	SET_COMPLEXPIXEL(objPtr, pixelPtr);
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetMMFromObj --
 *
 *	Attempt to return an mm value from the Tcl object "objPtr". If the
 *	object is not already an mm value, an attempt will be made to convert
 *	it to one.
 *
 * Results:
 *	The return value is a standard Tcl object result. If an error occurs
 *	during conversion, an error message is left in the interpreter's
 *	result unless "interp" is NULL.
 *
 * Side effects:
 *	If the object is not already a pixel, the conversion will free
 *	any old internal representation. 
 *
 *----------------------------------------------------------------------
 */

int
Tk_GetMMFromObj(interp, tkwin, objPtr, doublePtr)
    Tcl_Interp *interp; 	/* Used for error reporting if not NULL. */
    Tk_Window tkwin;
    Tcl_Obj *objPtr;		/* The object from which to get mms. */
    double *doublePtr;		/* Place to store resulting millimeters. */
{
    int result;
    double d;
    MMRep *mmPtr;
    static double bias[] = {
	10.0,	25.4,	1.0,	25.4 / 72.0
    };

    if (objPtr->typePtr != &mmObjType) {
	result = SetMMFromAny(interp, objPtr);
	if (result != TCL_OK) {
	    return result;
	}
    }

    mmPtr = (MMRep *) objPtr->internalRep.otherValuePtr;
    if (mmPtr->tkwin != tkwin) {
	d = mmPtr->value;
	if (mmPtr->units == -1) {
	    d /= WidthOfScreen(Tk_Screen(tkwin));
	    d *= WidthMMOfScreen(Tk_Screen(tkwin));
	} else {
	    d *= bias[mmPtr->units];
	}
	mmPtr->tkwin = tkwin;
	mmPtr->returnValue = d;
    }
    *doublePtr = mmPtr->returnValue;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeMMInternalRep --
 *
 *	Deallocate the storage associated with a mm object's internal
 *	representation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees objPtr's internal representation and sets objPtr's
 *	internalRep to NULL.
 *
 *----------------------------------------------------------------------
 */

static void
FreeMMInternalRep(objPtr)
    Tcl_Obj *objPtr;		/* MM object with internal rep to free. */
{
    ckfree((char *) objPtr->internalRep.otherValuePtr);
    objPtr->internalRep.otherValuePtr = NULL;
    objPtr->typePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * DupMMInternalRep --
 *
 *	Initialize the internal representation of a pixel Tcl_Obj to a
 *	copy of the internal representation of an existing pixel object. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	copyPtr's internal rep is set to the pixel corresponding to
 *	srcPtr's internal rep.
 *
 *----------------------------------------------------------------------
 */

static void
DupMMInternalRep(srcPtr, copyPtr)
    register Tcl_Obj *srcPtr;	/* Object with internal rep to copy. */
    register Tcl_Obj *copyPtr;	/* Object with internal rep to set. */
{
    MMRep *oldPtr, *newPtr;
    
    copyPtr->typePtr = srcPtr->typePtr;
    oldPtr = (MMRep *) srcPtr->internalRep.otherValuePtr;
    newPtr = (MMRep *) ckalloc(sizeof(MMRep));
    newPtr->value = oldPtr->value;
    newPtr->units = oldPtr->units;
    newPtr->tkwin = oldPtr->tkwin;
    newPtr->returnValue = oldPtr->returnValue;
    copyPtr->internalRep.otherValuePtr = (VOID *) newPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * UpdateStringOfMM --
 *
 *      Update the string representation for a pixel Tcl_Obj
 *      this function is only called, if the pixel Tcl_Obj has no unit,
 *      because with units the string representation is created by
 *      SetMMFromAny
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      The object's string is set to a valid string that results from
 *      the double-to-string conversion.
 *
 *----------------------------------------------------------------------
 */

static void
UpdateStringOfMM(objPtr)
    register Tcl_Obj *objPtr;   /* pixel obj with string rep to update. */
{
    MMRep *mmPtr;
    char buffer[TCL_DOUBLE_SPACE];
    register int len;

    mmPtr = (MMRep *) objPtr->internalRep.otherValuePtr;
    /* assert( mmPtr->units == -1 && objPtr->bytes == NULL ); */
    if ((mmPtr->units != -1) || (objPtr->bytes != NULL)) {
        panic("UpdateStringOfMM: false precondition");
    }

    Tcl_PrintDouble((Tcl_Interp *) NULL, mmPtr->value, buffer);
    len = strlen(buffer);

    objPtr->bytes = (char *) ckalloc((unsigned) len + 1);
    strcpy(objPtr->bytes, buffer);
    objPtr->length = len;
}

/*
 *----------------------------------------------------------------------
 *
 * SetMMFromAny --
 *
 *	Attempt to generate a mm internal form for the Tcl object
 *	"objPtr".
 *
 * Results:
 *	The return value is a standard Tcl result. If an error occurs during
 *	conversion, an error message is left in the interpreter's result
 *	unless "interp" is NULL.
 *
 * Side effects:
 *	If no error occurs, a mm representation of the object is
 *	stored internally and the type of "objPtr" is set to mm.
 *
 *----------------------------------------------------------------------
 */

static int
SetMMFromAny(interp, objPtr)
    Tcl_Interp *interp;		/* Used for error reporting if not NULL. */
    Tcl_Obj *objPtr;		/* The object to convert. */
{
    Tcl_ObjType *typePtr;
    char *string, *rest;
    double d;
    int units;
    MMRep *mmPtr;

    static Tcl_ObjType *tclDoubleObjType = NULL;
    static Tcl_ObjType *tclIntObjType = NULL;

    if (tclDoubleObjType == NULL) {
	/*
	 * Cache the object types for comaprison below.
	 * This allows optimized checks for standard cases.
	 */

	tclDoubleObjType = Tcl_GetObjType("double");
	tclIntObjType    = Tcl_GetObjType("int");
    }

    if (objPtr->typePtr == tclDoubleObjType) {
	Tcl_GetDoubleFromObj(interp, objPtr, &d);
	units = -1;
    } else if (objPtr->typePtr == tclIntObjType) {
	Tcl_GetIntFromObj(interp, objPtr, &units);
	d = (double) units;
	units = -1;

	/*
	 * In the case of ints, we need to ensure that a valid
	 * string exists in order for int-but-not-string objects
	 * to be converted back to ints again from mm obj types.
	 */
	(void) Tcl_GetStringFromObj(objPtr, NULL);
    } else {
	/*
	 * It wasn't a known int or double, so parse it.
	 */

	string = Tcl_GetStringFromObj(objPtr, NULL);

	d = strtod(string, &rest);
	if (rest == string) {
	    /*
	     * Must copy string before resetting the result in case a caller
	     * is trying to convert the interpreter's result to mms.
	     */

	    error:
            Tcl_AppendResult(interp, "bad screen distance \"", string,
                    "\"", (char *) NULL);
            return TCL_ERROR;
        }
        while ((*rest != '\0') && isspace(UCHAR(*rest))) {
            rest++;
        }
        switch (*rest) {
	    case '\0':
		units = -1;
		break;

	    case 'c':
		units = 0;
		break;

	    case 'i':
		units = 1;
		break;

	    case 'm':
		units = 2;
		break;

	    case 'p':
		units = 3;
		break;

	    default:
		goto error;
	}
    }

    /*
     * Free the old internalRep before setting the new one. 
     */

    typePtr = objPtr->typePtr;
    if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
	(*typePtr->freeIntRepProc)(objPtr);
    }

    objPtr->typePtr	= &mmObjType;

    mmPtr		= (MMRep *) ckalloc(sizeof(MMRep));
    mmPtr->value	= d;
    mmPtr->units	= units;
    mmPtr->tkwin	= NULL;
    mmPtr->returnValue	= d;

    objPtr->internalRep.otherValuePtr = (VOID *) mmPtr;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * TkGetWindowFromObj --
 *
 *	Attempt to return a Tk_Window from the Tcl object "objPtr". If the
 *	object is not already a Tk_Window, an attempt will be made to convert
 *	it to one.
 *
 * Results:
 *	The return value is a standard Tcl object result. If an error occurs
 *	during conversion, an error message is left in the interpreter's
 *	result unless "interp" is NULL.
 *
 * Side effects:
 *	If the object is not already a Tk_Window, the conversion will free
 *	any old internal representation. 
 *
 *----------------------------------------------------------------------
 */

int
TkGetWindowFromObj(interp, tkwin, objPtr, windowPtr)
    Tcl_Interp *interp; 	/* Used for error reporting if not NULL. */
    Tk_Window tkwin;		/* A token to get the main window from. */
    Tcl_Obj *objPtr;		/* The object from which to get window. */
    Tk_Window *windowPtr;	/* Place to store resulting window. */
{
    TkMainInfo *mainPtr = ((TkWindow *)tkwin)->mainPtr;
    register WindowRep *winPtr;
    int result;

    result = Tcl_ConvertToType(interp, objPtr, &windowObjType);
    if (result != TCL_OK) {
	return result;
    }

    winPtr = (WindowRep *) objPtr->internalRep.otherValuePtr;
    if (    winPtr->tkwin == NULL
	 || winPtr->mainPtr == NULL
	 || winPtr->mainPtr != mainPtr 
	 || winPtr->epoch != mainPtr->deletionEpoch) 
    {
	/* Cache is invalid.
	 */
	winPtr->tkwin = Tk_NameToWindow(interp,
		Tcl_GetStringFromObj(objPtr, NULL), tkwin);
	winPtr->mainPtr = mainPtr;
	winPtr->epoch = mainPtr ? mainPtr->deletionEpoch : 0;
    }

    *windowPtr = winPtr->tkwin;

    if (winPtr->tkwin == NULL) {
	/* ASSERT: Tk_NameToWindow has left error message in interp */
	return TCL_ERROR;
    }
    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * SetWindowFromAny --
 *	Generate a windowObj internal form for the Tcl object "objPtr".
 *
 * Results:
 *	Always returns TCL_OK. 
 *
 * Side effects:
 *	Sets objPtr's internal representation to an uninitialized
 *	windowObj. Frees the old internal representation, if any.
 *
 * See also:
 * 	TkGetWindowFromObj, which initializes the WindowRep cache.
 *
 *----------------------------------------------------------------------
 */

static int
SetWindowFromAny(interp, objPtr)
    Tcl_Interp *interp;		/* Used for error reporting if not NULL. */
    register Tcl_Obj *objPtr;	/* The object to convert. */
{
    Tcl_ObjType *typePtr;
    WindowRep *winPtr;

    /*
     * Free the old internalRep before setting the new one. 
     */

    Tcl_GetStringFromObj(objPtr, NULL);
    typePtr = objPtr->typePtr;
    if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
	(*typePtr->freeIntRepProc)(objPtr);
    }

    winPtr = (WindowRep *) ckalloc(sizeof(WindowRep));
    winPtr->tkwin = NULL;
    winPtr->mainPtr = NULL;
    winPtr->epoch = 0;

    objPtr->internalRep.otherValuePtr = (VOID*)winPtr;
    objPtr->typePtr = &windowObjType;

    return TCL_OK;
}

/*
 *----------------------------------------------------------------------
 *
 * DupWindowInternalRep --
 *
 *	Initialize the internal representation of a window Tcl_Obj to a
 *	copy of the internal representation of an existing window object. 
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	copyPtr's internal rep is set to refer to the same window as
 *	srcPtr's internal rep.
 *
 *----------------------------------------------------------------------
 */

static void
DupWindowInternalRep(srcPtr, copyPtr)
    register Tcl_Obj *srcPtr;
    register Tcl_Obj *copyPtr;
{
    register WindowRep *oldPtr, *newPtr;

    oldPtr = srcPtr->internalRep.otherValuePtr;
    newPtr = (WindowRep *) ckalloc(sizeof(WindowRep));
    newPtr->tkwin = oldPtr->tkwin;
    newPtr->mainPtr = oldPtr->mainPtr;
    newPtr->epoch = oldPtr->epoch;
    copyPtr->internalRep.otherValuePtr = (VOID *)newPtr;
    copyPtr->typePtr = srcPtr->typePtr;
}

/*
 *----------------------------------------------------------------------
 *
 * FreeWindowInternalRep --
 *
 *	Deallocate the storage associated with a window object's internal
 *	representation.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Frees objPtr's internal representation and sets objPtr's
 *	internalRep to NULL.
 *
 *----------------------------------------------------------------------
 */

static void
FreeWindowInternalRep(objPtr)
    Tcl_Obj *objPtr;		/* Window object with internal rep to free. */
{
    ckfree((char *) objPtr->internalRep.otherValuePtr);
    objPtr->internalRep.otherValuePtr = NULL;
    objPtr->typePtr = NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * TkRegisterObjTypes --
 *
 *	Registers Tk's Tcl_ObjType structures with the Tcl run-time.
 *
 * Results:
 *	None
 *
 * Side effects:
 *	All instances of Tcl_ObjType structues used in Tk are registered
 *	with Tcl.
 *
 *----------------------------------------------------------------------
 */

void
TkRegisterObjTypes()
{
    Tcl_RegisterObjType(&tkBorderObjType);
    Tcl_RegisterObjType(&tkBitmapObjType);
    Tcl_RegisterObjType(&tkColorObjType);
    Tcl_RegisterObjType(&tkCursorObjType);
    Tcl_RegisterObjType(&tkFontObjType);
    Tcl_RegisterObjType(&mmObjType);
    Tcl_RegisterObjType(&tkOptionObjType);
    Tcl_RegisterObjType(&pixelObjType);
    Tcl_RegisterObjType(&tkStateKeyObjType);
    Tcl_RegisterObjType(&windowObjType);
}
