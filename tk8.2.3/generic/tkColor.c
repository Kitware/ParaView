/* 
 * tkColor.c --
 *
 *	This file maintains a database of color values for the Tk
 *	toolkit, in order to avoid round-trips to the server to
 *	map color names to pixel values.
 *
 * Copyright (c) 1990-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkColor.h"

/*
 * Structures of the following following type are used as keys for 
 * colorValueTable (in TkDisplay).
 */

typedef struct {
    int red, green, blue;	/* Values for desired color. */
    Colormap colormap;		/* Colormap from which color will be
				 * allocated. */
    Display *display;		/* Display for colormap. */
} ValueKey;


/*
 * The structure below is used to allocate thread-local data. 
 */

typedef struct ThreadSpecificData {
    char rgbString[20];            /* */
} ThreadSpecificData;
static Tcl_ThreadDataKey dataKey;

/*
 * Forward declarations for procedures defined in this file:
 */

static void		ColorInit _ANSI_ARGS_((TkDisplay *dispPtr));
static void		DupColorObjProc _ANSI_ARGS_((Tcl_Obj *srcObjPtr,
			    Tcl_Obj *dupObjPtr));
static void		FreeColorObjProc _ANSI_ARGS_((Tcl_Obj *objPtr));
static void		InitColorObj _ANSI_ARGS_((Tcl_Obj *objPtr));

/*
 * The following structure defines the implementation of the "color" Tcl
 * object, which maps a string color name to a TkColor object.  The
 * ptr1 field of the Tcl_Obj points to a TkColor object.
 */

static Tcl_ObjType colorObjType = {
    "color",			/* name */
    FreeColorObjProc,		/* freeIntRepProc */
    DupColorObjProc,		/* dupIntRepProc */
    NULL,			/* updateStringProc */
    NULL			/* setFromAnyProc */
};

/*
 *----------------------------------------------------------------------
 *
 * Tk_AllocColorFromObj --
 *
 *	Given a Tcl_Obj *, map the value to a corresponding
 *	XColor structure based on the tkwin given.
 *
 * Results:
 *	The return value is a pointer to an XColor structure that
 *	indicates the red, blue, and green intensities for the color
 *	given by the string in objPtr, and also specifies a pixel value 
 *	to use to draw in that color.  If an error occurs, NULL is 
 *	returned and an error message will be left in interp's result
 *	(unless interp is NULL).
 *
 * Side effects:
 *	The color is added to an internal database with a reference count.
 *	For each call to this procedure, there should eventually be a call
 *	to Tk_FreeColorFromObj so that the database is cleaned up when colors
 *	aren't in use anymore.
 *
 *----------------------------------------------------------------------
 */

XColor *
Tk_AllocColorFromObj(interp, tkwin, objPtr)
    Tcl_Interp *interp;		/* Used only for error reporting.  If NULL,
				 * then no messages are provided. */
    Tk_Window tkwin;		/* Window in which the color will be used.*/
    Tcl_Obj *objPtr;		/* Object that describes the color; string
				 * value is a color name such as "red" or
				 * "#ff0000".*/
{
    TkColor *tkColPtr;

    if (objPtr->typePtr != &colorObjType) {
	InitColorObj(objPtr);
    }
    tkColPtr = (TkColor *) objPtr->internalRep.twoPtrValue.ptr1;

    /*
     * If the object currently points to a TkColor, see if it's the
     * one we want.  If so, increment its reference count and return.
     */

    if (tkColPtr != NULL) {
	if (tkColPtr->resourceRefCount == 0) {
	    /*
	     * This is a stale reference: it refers to a TkColor that's
	     * no longer in use.  Clear the reference.
	     */

	    FreeColorObjProc(objPtr);
	    tkColPtr = NULL;
	} else if ((Tk_Screen(tkwin) == tkColPtr->screen)
		&& (Tk_Colormap(tkwin) == tkColPtr->colormap)) {
	    tkColPtr->resourceRefCount++;
	    return (XColor *) tkColPtr;
	}
    }

    /*
     * The object didn't point to the TkColor that we wanted.  Search
     * the list of TkColors with the same name to see if one of the
     * other TkColors is the right one.
     */

    if (tkColPtr != NULL) {
	TkColor *firstColorPtr = 
		(TkColor *) Tcl_GetHashValue(tkColPtr->hashPtr);
	FreeColorObjProc(objPtr);
	for (tkColPtr = firstColorPtr; tkColPtr != NULL;
		tkColPtr = tkColPtr->nextPtr) {
	    if ((Tk_Screen(tkwin) == tkColPtr->screen)
		    && (Tk_Colormap(tkwin) == tkColPtr->colormap)) {
		tkColPtr->resourceRefCount++;
		tkColPtr->objRefCount++;
		objPtr->internalRep.twoPtrValue.ptr1 = (VOID *) tkColPtr;
		return (XColor *) tkColPtr;
	    }
	}
    }

    /*
     * Still no luck.  Call Tk_GetColor to allocate a new TkColor object.
     */

    tkColPtr = (TkColor *) Tk_GetColor(interp, tkwin, Tcl_GetString(objPtr));
    objPtr->internalRep.twoPtrValue.ptr1 = (VOID *) tkColPtr;
    if (tkColPtr != NULL) {
	tkColPtr->objRefCount++;
    }
    return (XColor *) tkColPtr;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetColor --
 *
 *	Given a string name for a color, map the name to a corresponding
 *	XColor structure.
 *
 * Results:
 *	The return value is a pointer to an XColor structure that
 *	indicates the red, blue, and green intensities for the color
 *	given by "name", and also specifies a pixel value to use to
 *	draw in that color.  If an error occurs, NULL is returned and
 *	an error message will be left in the interp's result.
 *
 * Side effects:
 *	The color is added to an internal database with a reference count.
 *	For each call to this procedure, there should eventually be a call
 *	to Tk_FreeColor so that the database is cleaned up when colors
 *	aren't in use anymore.
 *
 *----------------------------------------------------------------------
 */

XColor *
Tk_GetColor(interp, tkwin, name)
    Tcl_Interp *interp;		/* Place to leave error message if
				 * color can't be found. */
    Tk_Window tkwin;		/* Window in which color will be used. */
    char *name;			/* Name of color to be allocated (in form
				 * suitable for passing to XParseColor). */
{
    Tcl_HashEntry *nameHashPtr;
    int new;
    TkColor *tkColPtr;
    TkColor *existingColPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (!dispPtr->colorInit) {
	ColorInit(dispPtr);
    }

    /*
     * First, check to see if there's already a mapping for this color
     * name.
     */

    nameHashPtr = Tcl_CreateHashEntry(&dispPtr->colorNameTable, name, &new);
    if (!new) {
	existingColPtr = (TkColor *) Tcl_GetHashValue(nameHashPtr);
	for (tkColPtr = existingColPtr;  tkColPtr != NULL;
		tkColPtr = tkColPtr->nextPtr) {
	    if ((tkColPtr->screen == Tk_Screen(tkwin))
		    && (Tk_Colormap(tkwin) == tkColPtr->colormap)) {
		tkColPtr->resourceRefCount++;
		return &tkColPtr->color;
	    }
	}
    } else {
	existingColPtr = NULL;
    }

    /*
     * The name isn't currently known.  Map from the name to a pixel
     * value.
     */

    tkColPtr = TkpGetColor(tkwin, name);
    if (tkColPtr == NULL) {
	if (interp != NULL) {
	    if (*name == '#') {
		Tcl_AppendResult(interp, "invalid color name \"", name,
			"\"", (char *) NULL);
	    } else {
		Tcl_AppendResult(interp, "unknown color name \"", name,
			"\"", (char *) NULL);
	    }
	}
	if (new) {
	    Tcl_DeleteHashEntry(nameHashPtr);
	}
	return (XColor *) NULL;
    }

    /*
     * Now create a new TkColor structure and add it to colorNameTable
     * (in TkDisplay).
     */

    tkColPtr->magic = COLOR_MAGIC;
    tkColPtr->gc = None;
    tkColPtr->screen = Tk_Screen(tkwin);
    tkColPtr->colormap = Tk_Colormap(tkwin);
    tkColPtr->visual  = Tk_Visual(tkwin);
    tkColPtr->resourceRefCount = 1;
    tkColPtr->objRefCount = 0;
    tkColPtr->type = TK_COLOR_BY_NAME;
    tkColPtr->hashPtr = nameHashPtr;
    tkColPtr->nextPtr = existingColPtr;
    Tcl_SetHashValue(nameHashPtr, tkColPtr);

    return &tkColPtr->color;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetColorByValue --
 *
 *	Given a desired set of red-green-blue intensities for a color,
 *	locate a pixel value to use to draw that color in a given
 *	window.
 *
 * Results:
 *	The return value is a pointer to an XColor structure that
 *	indicates the closest red, blue, and green intensities available
 *	to those specified in colorPtr, and also specifies a pixel
 *	value to use to draw in that color.
 *
 * Side effects:
 *	The color is added to an internal database with a reference count.
 *	For each call to this procedure, there should eventually be a call
 *	to Tk_FreeColor, so that the database is cleaned up when colors
 *	aren't in use anymore.
 *
 *----------------------------------------------------------------------
 */

XColor *
Tk_GetColorByValue(tkwin, colorPtr)
    Tk_Window tkwin;		/* Window where color will be used. */
    XColor *colorPtr;		/* Red, green, and blue fields indicate
				 * desired color. */
{
    ValueKey valueKey;
    Tcl_HashEntry *valueHashPtr;
    int new;
    TkColor *tkColPtr;
    Display *display = Tk_Display(tkwin);
    TkDisplay *dispPtr = TkGetDisplay(display);

    if (!dispPtr->colorInit) {
	ColorInit(dispPtr);
    }

    /*
     * First, check to see if there's already a mapping for this color
     * name.
     */

    valueKey.red = colorPtr->red;
    valueKey.green = colorPtr->green;
    valueKey.blue = colorPtr->blue;
    valueKey.colormap = Tk_Colormap(tkwin);
    valueKey.display = display;
    valueHashPtr = Tcl_CreateHashEntry(&dispPtr->colorValueTable, 
            (char *) &valueKey, &new);
    if (!new) {
	tkColPtr = (TkColor *) Tcl_GetHashValue(valueHashPtr);
	tkColPtr->resourceRefCount++;
	return &tkColPtr->color;
    }

    /*
     * The name isn't currently known.  Find a pixel value for this
     * color and add a new structure to colorValueTable (in TkDisplay).
     */

    tkColPtr = TkpGetColorByValue(tkwin, colorPtr);
    tkColPtr->magic = COLOR_MAGIC;
    tkColPtr->gc = None;
    tkColPtr->screen = Tk_Screen(tkwin);
    tkColPtr->colormap = valueKey.colormap;
    tkColPtr->visual  = Tk_Visual(tkwin);
    tkColPtr->resourceRefCount = 1;
    tkColPtr->objRefCount = 0;
    tkColPtr->type = TK_COLOR_BY_VALUE;
    tkColPtr->hashPtr = valueHashPtr;
    tkColPtr->nextPtr = NULL;
    Tcl_SetHashValue(valueHashPtr, tkColPtr);
    return &tkColPtr->color;
}

/*
 *--------------------------------------------------------------
 *
 * Tk_NameOfColor --
 *
 *	Given a color, return a textual string identifying
 *	the color.
 *
 * Results:
 *	If colorPtr was created by Tk_GetColor, then the return
 *	value is the "string" that was used to create it.
 *	Otherwise the return value is a string that could have
 *	been passed to Tk_GetColor to allocate that color.  The
 *	storage for the returned string is only guaranteed to
 *	persist up until the next call to this procedure.
 *
 * Side effects:
 *	None.
 *
 *--------------------------------------------------------------
 */

char *
Tk_NameOfColor(colorPtr)
    XColor *colorPtr;		/* Color whose name is desired. */
{
    register TkColor *tkColPtr = (TkColor *) colorPtr;
    
    if ((tkColPtr->magic == COLOR_MAGIC) &&
	    (tkColPtr->type == TK_COLOR_BY_NAME)) {
	return tkColPtr->hashPtr->key.string;
    } else {
	ThreadSpecificData *tsdPtr = (ThreadSpecificData *) 
            Tcl_GetThreadData(&dataKey, sizeof(ThreadSpecificData));
	sprintf(tsdPtr->rgbString, "#%04x%04x%04x", colorPtr->red, 
		colorPtr->green, colorPtr->blue);
	return tsdPtr->rgbString;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GCForColor --
 *
 *	Given a color allocated from this module, this procedure
 *	returns a GC that can be used for simple drawing with that
 *	color.
 *
 * Results:
 *	The return value is a GC with color set as its foreground
 *	color and all other fields defaulted.  This GC is only valid
 *	as long as the color exists;  it is freed automatically when
 *	the last reference to the color is freed.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

GC
Tk_GCForColor(colorPtr, drawable)
    XColor *colorPtr;		/* Color for which a GC is desired. Must
				 * have been allocated by Tk_GetColor. */
    Drawable drawable;		/* Drawable in which the color will be
				 * used (must have same screen and depth
				 * as the one for which the color was
				 * allocated). */
{
    TkColor *tkColPtr = (TkColor *) colorPtr;
    XGCValues gcValues;

    /*
     * Do a quick sanity check to make sure this color was really
     * allocated by Tk_GetColor.
     */

    if (tkColPtr->magic != COLOR_MAGIC) {
	panic("Tk_GCForColor called with bogus color");
    }

    if (tkColPtr->gc == None) {
	gcValues.foreground = tkColPtr->color.pixel;
	tkColPtr->gc = XCreateGC(DisplayOfScreen(tkColPtr->screen),
		drawable, GCForeground, &gcValues);
    }
    return tkColPtr->gc;
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeColor --
 *
 *	This procedure is called to release a color allocated by
 *	Tk_GetColor.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with colorPtr is deleted, and
 *	the color is released to X if there are no remaining uses
 *	for it.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeColor(colorPtr)
    XColor *colorPtr;		/* Color to be released.  Must have been
				 * allocated by Tk_GetColor or
				 * Tk_GetColorByValue. */
{
    TkColor *tkColPtr = (TkColor *) colorPtr;
    Screen *screen = tkColPtr->screen;
    TkColor *prevPtr;

    /*
     * Do a quick sanity check to make sure this color was really
     * allocated by Tk_GetColor.
     */

    if (tkColPtr->magic != COLOR_MAGIC) {
	panic("Tk_FreeColor called with bogus color");
    }

    tkColPtr->resourceRefCount--;
    if (tkColPtr->resourceRefCount > 0) {
	return;
    }

    /*
     * This color is no longer being actively used, so free the color
     * resources associated with it and remove it from the hash table.
     * no longer any objects referencing it.
     */

    if (tkColPtr->gc != None) {
	XFreeGC(DisplayOfScreen(screen), tkColPtr->gc);
	tkColPtr->gc = None;
    }
    TkpFreeColor(tkColPtr);

    prevPtr = (TkColor *) Tcl_GetHashValue(tkColPtr->hashPtr);
    if (prevPtr == tkColPtr) {
	if (tkColPtr->nextPtr == NULL) {
	    Tcl_DeleteHashEntry(tkColPtr->hashPtr);
	} else  {
	    Tcl_SetHashValue(tkColPtr->hashPtr, tkColPtr->nextPtr);
	}
    } else {
	while (prevPtr->nextPtr != tkColPtr) {
	    prevPtr = prevPtr->nextPtr;
	}
	prevPtr->nextPtr = tkColPtr->nextPtr;
    }

    /*
     * Free the TkColor structure if there are no objects referencing
     * it.  However, if there are objects referencing it then keep the
     * structure around; it will get freed when the last reference is
     * cleared
     */

    if (tkColPtr->objRefCount == 0) {
	ckfree((char *) tkColPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_FreeColorFromObj --
 *
 *	This procedure is called to release a color allocated by
 *	Tk_AllocColorFromObj. It does not throw away the Tcl_Obj *;
 *	it only gets rid of the hash table entry for this color
 *	and clears the cached value that is normally stored in the object.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The reference count associated with the color represented by
 *	objPtr is decremented, and the color is released to X if there are 
 *	no remaining uses for it.
 *
 *----------------------------------------------------------------------
 */

void
Tk_FreeColorFromObj(tkwin, objPtr)
    Tk_Window tkwin;		/* The window this color lives in. Needed
				 * for the screen and colormap values. */
    Tcl_Obj *objPtr;		/* The Tcl_Obj * to be freed. */
{
    Tk_FreeColor(Tk_GetColorFromObj(tkwin, objPtr));
}

/*
 *---------------------------------------------------------------------------
 *
 * FreeColorObjProc -- 
 *
 *	This proc is called to release an object reference to a color.
 *	Called when the object's internal rep is released or when
 *	the cached tkColPtr needs to be changed.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The object reference count is decremented. When both it
 *	and the hash ref count go to zero, the color's resources
 *	are released.
 *
 *---------------------------------------------------------------------------
 */

static void
FreeColorObjProc(objPtr)
    Tcl_Obj *objPtr;		/* The object we are releasing. */
{
    TkColor *tkColPtr = (TkColor *) objPtr->internalRep.twoPtrValue.ptr1;

    if (tkColPtr != NULL) {
	tkColPtr->objRefCount--;
	if ((tkColPtr->objRefCount == 0) 
		&& (tkColPtr->resourceRefCount == 0)) {
	    ckfree((char *) tkColPtr);
	}
	objPtr->internalRep.twoPtrValue.ptr1 = (VOID *) NULL;
    }
}

/*
 *---------------------------------------------------------------------------
 *
 * DupColorObjProc -- 
 *
 *	When a cached color object is duplicated, this is called to
 *	update the internal reps.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The color's objRefCount is incremented and the internal rep
 *	of the copy is set to point to it.
 *
 *---------------------------------------------------------------------------
 */

static void
DupColorObjProc(srcObjPtr, dupObjPtr)
    Tcl_Obj *srcObjPtr;		/* The object we are copying from. */
    Tcl_Obj *dupObjPtr;		/* The object we are copying to. */
{
    TkColor *tkColPtr = (TkColor *) srcObjPtr->internalRep.twoPtrValue.ptr1;
    
    dupObjPtr->typePtr = srcObjPtr->typePtr;
    dupObjPtr->internalRep.twoPtrValue.ptr1 = (VOID *) tkColPtr;

    if (tkColPtr != NULL) {
	tkColPtr->objRefCount++;
    }
}

/*
 *----------------------------------------------------------------------
 *
 * Tk_GetColorFromObj --
 *
 *	Returns the color referred to by a Tcl object.  The color must
 *	already have been allocated via a call to Tk_AllocColorFromObj
 *	or Tk_GetColor.
 *
 * Results:
 *	Returns the XColor * that matches the tkwin and the string rep
 *	of objPtr.
 *
 * Side effects:
 *	If the object is not already a color, the conversion will free
 *	any old internal representation. 
 *
 *----------------------------------------------------------------------
 */

XColor *
Tk_GetColorFromObj(tkwin, objPtr)
    Tk_Window tkwin;		/* The window in which the color will be
				 * used. */
    Tcl_Obj *objPtr;		/* String value contains the name of the
				 * desired color. */
{
    TkColor *tkColPtr;
    Tcl_HashEntry *hashPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    if (objPtr->typePtr != &colorObjType) {
	InitColorObj(objPtr);
    }

    tkColPtr = (TkColor *) objPtr->internalRep.twoPtrValue.ptr1;
    if (tkColPtr != NULL) {
	if ((tkColPtr->resourceRefCount > 0)
		&& (Tk_Screen(tkwin) == tkColPtr->screen)
		&& (Tk_Colormap(tkwin) == tkColPtr->colormap)) {
	    /*
	     * The object already points to the right TkColor structure.
	     * Just return it.
	     */

	    return (XColor *) tkColPtr;
	}
	hashPtr = tkColPtr->hashPtr;
	FreeColorObjProc(objPtr);
    } else {
	hashPtr = Tcl_FindHashEntry(&dispPtr->colorNameTable, 
                Tcl_GetString(objPtr));
	if (hashPtr == NULL) {
	    goto error;
	}
    }

    /*
     * At this point we've got a hash table entry, off of which hang
     * one or more TkColor structures.  See if any of them will work.
     */

    for (tkColPtr = (TkColor *) Tcl_GetHashValue(hashPtr);
	    (tkColPtr != NULL); tkColPtr = tkColPtr->nextPtr) {
	if ((Tk_Screen(tkwin) == tkColPtr->screen)
		&& (Tk_Colormap(tkwin) == tkColPtr->colormap)) {
	    objPtr->internalRep.twoPtrValue.ptr1 = (VOID *) tkColPtr;
	    tkColPtr->objRefCount++;
	    return (XColor *) tkColPtr;
	}
    }

    error:
    panic(" Tk_GetColorFromObj called with non-existent color!");
    /*
     * The following code isn't reached; it's just there to please compilers.
     */
    return NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * InitColorObj --
 *
 *	Bookeeping procedure to change an objPtr to a color type.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	The old internal rep of the object is freed. The object's
 *	type is set to color with a NULL TkColor pointer (the pointer
 *	will be set later by either Tk_AllocColorFromObj or
 *	Tk_GetColorFromObj).
 *
 *----------------------------------------------------------------------
 */

static void
InitColorObj(objPtr)
    Tcl_Obj *objPtr;		/* The object to convert. */
{
    Tcl_ObjType *typePtr;

    /*
     * Free the old internalRep before setting the new one. 
     */

    Tcl_GetString(objPtr);
    typePtr = objPtr->typePtr;
    if ((typePtr != NULL) && (typePtr->freeIntRepProc != NULL)) {
	(*typePtr->freeIntRepProc)(objPtr);
    }
    objPtr->typePtr = &colorObjType;
    objPtr->internalRep.twoPtrValue.ptr1 = (VOID *) NULL;
}

/*
 *----------------------------------------------------------------------
 *
 * ColorInit --
 *
 *	Initialize the structure used for color management.
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
ColorInit(dispPtr)
    TkDisplay *dispPtr;
{
    if (!dispPtr->colorInit) {
        dispPtr->colorInit = 1;
	Tcl_InitHashTable(&dispPtr->colorNameTable, TCL_STRING_KEYS);
	Tcl_InitHashTable(&dispPtr->colorValueTable, 
                sizeof(ValueKey)/sizeof(int));
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkDebugColor --
 *
 *	This procedure returns debugging information about a color.
 *
 * Results:
 *	The return value is a list with one sublist for each TkColor
 *	corresponding to "name".  Each sublist has two elements that
 *	contain the resourceRefCount and objRefCount fields from the
 *	TkColor structure.
 *
 * Side effects:
 *	None.
 *
 *----------------------------------------------------------------------
 */

Tcl_Obj *
TkDebugColor(tkwin, name)
    Tk_Window tkwin;		/* The window in which the color will be
				 * used (not currently used). */
    char *name;			/* Name of the desired color. */
{
    TkColor *tkColPtr;
    Tcl_HashEntry *hashPtr;
    Tcl_Obj *resultPtr, *objPtr;
    TkDisplay *dispPtr = ((TkWindow *) tkwin)->dispPtr;

    resultPtr = Tcl_NewObj();
    hashPtr = Tcl_FindHashEntry(&dispPtr->colorNameTable, name);
    if (hashPtr != NULL) {
	tkColPtr = (TkColor *) Tcl_GetHashValue(hashPtr);
	if (tkColPtr == NULL) {
	    panic("TkDebugColor found empty hash table entry");
	}
	for ( ; (tkColPtr != NULL); tkColPtr = tkColPtr->nextPtr) {
	    objPtr = Tcl_NewObj();
	    Tcl_ListObjAppendElement(NULL, objPtr,
		    Tcl_NewIntObj(tkColPtr->resourceRefCount));
	    Tcl_ListObjAppendElement(NULL, objPtr,
		    Tcl_NewIntObj(tkColPtr->objRefCount)); 
	    Tcl_ListObjAppendElement(NULL, resultPtr, objPtr);
	}
    }
    return resultPtr;
}
