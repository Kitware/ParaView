/* 
 * tkMacBitmap.c --
 *
 *	This file handles the implementation of native bitmaps.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include "tkPort.h"
#include "tk.h"
#include "tkMacInt.h"

#include <Icons.h>
#include <Dialogs.h>
#include <Resources.h>
#include <Strings.h>

/*
 * Depending on the resource type there are different ways to
 * draw native icons.
 */
#define TYPE1	0	/* Family icon suite. */
#define TYPE2	1	/* ICON resource. */
#define TYPE3	2	/* cicn resource. */

/*
 * This data structure describes the id and type of a given icon.
 * It is used as the source for native icons.
 */
typedef struct {
    int id;		/* Resource Id for Icon. */
    long int type;	/* Type of icon. */
} NativeIcon;

/*
 * This structure holds information about native bitmaps.
 */

typedef struct {
    char *name;			/* Name of icon. */
    long int type;		/* Type of icon. */
    int id;			/* Id of icon. */
    int size;			/* Size of icon. */
} BuiltInIcon;

/*
 * This array mapps a string name to the supported builtin icons
 * on the Macintosh.
 */

static BuiltInIcon builtInIcons[] = {
    {"document",	TYPE1,	kGenericDocumentIconResource,		32},
    {"stationery",	TYPE1,	kGenericStationeryIconResource,		32},
    {"edition",		TYPE1,	kGenericEditionFileIconResource,	32},
    {"application",	TYPE1,	kGenericApplicationIconResource,	32},
    {"accessory",	TYPE1,	kGenericDeskAccessoryIconResource,	32},
    {"folder",		TYPE1,	kGenericFolderIconResource,		32},
    {"pfolder",		TYPE1,	kPrivateFolderIconResource,		32},
    {"trash",		TYPE1,	kTrashIconResource,			32},
    {"floppy",		TYPE1,	kFloppyIconResource,			32},
    {"ramdisk",		TYPE1,	kGenericRAMDiskIconResource,		32},
    {"cdrom",		TYPE1,	kGenericCDROMIconResource,		32},
    {"preferences",	TYPE1,	kGenericPreferencesIconResource,	32},
    {"querydoc",	TYPE1,	kGenericQueryDocumentIconResource,	32},
    {"stop",		TYPE2,	kStopIcon,				32},
    {"note",		TYPE2,	kNoteIcon,				32},
    {"caution",		TYPE2,	kCautionIcon,				32},
    {(char *) NULL,	0,	0, 					0}
};

/*
 *----------------------------------------------------------------------
 *
 * TkpDefineNativeBitmaps --
 *
 *	Add native bitmaps.
 *
 * Results:
 *	A standard Tcl result.  If an error occurs then TCL_ERROR is
 *	returned and a message is left in the interp's result.
 *
 * Side effects:
 *	"Name" is entered into the bitmap table and may be used from
 *	here on to refer to the given bitmap.
 *
 *----------------------------------------------------------------------
 */

void
TkpDefineNativeBitmaps()
{
    int new;
    Tcl_HashEntry *predefHashPtr;
    TkPredefBitmap *predefPtr;
    char * name;
    BuiltInIcon *builtInPtr;
    NativeIcon *nativeIconPtr;
    Tcl_HashTable *tablePtr;
    
    for (builtInPtr = builtInIcons; builtInPtr->name != NULL; builtInPtr++) {
	name = Tk_GetUid(builtInPtr->name);
	tablePtr = TkGetBitmapPredefTable();
	predefHashPtr = Tcl_CreateHashEntry(tablePtr, name, &new);
	if (!new) {
	    continue;
	}
	predefPtr = (TkPredefBitmap *) ckalloc(sizeof(TkPredefBitmap));
	nativeIconPtr = (NativeIcon *) ckalloc(sizeof(NativeIcon));
	nativeIconPtr->id = builtInPtr->id;
	nativeIconPtr->type = builtInPtr->type;
	predefPtr->source = (char *) nativeIconPtr;
	predefPtr->width = builtInPtr->size;
	predefPtr->height = builtInPtr->size;
	predefPtr->native = 1;
	Tcl_SetHashValue(predefHashPtr, predefPtr);
    }
}

/*
 *----------------------------------------------------------------------
 *
 * TkpCreateNativeBitmap --
 *
 *	Add native bitmaps.
 *
 * Results:
 *	A standard Tcl result.  If an error occurs then TCL_ERROR is
 *	returned and a message is left in the interp's result.
 *
 * Side effects:
 *	"Name" is entered into the bitmap table and may be used from
 *	here on to refer to the given bitmap.
 *
 *----------------------------------------------------------------------
 */

Pixmap
TkpCreateNativeBitmap(
    Display *display,
    char * source)		/* Info about the icon to build. */
{
    Pixmap pix;
    GWorldPtr destPort;
    Rect destRect;
    Handle icon;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    NativeIcon *nativeIconPtr;
    
    pix = Tk_GetPixmap(display, None, 32, 32, 0);
    destPort = TkMacGetDrawablePort(pix);

    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);

    nativeIconPtr = (NativeIcon *) source;
    SetRect(&destRect, 0, 0, 32, 32);
    if (nativeIconPtr->type == TYPE1) {
	RGBColor white = {0xFFFF, 0xFFFF, 0xFFFF};

	RGBForeColor(&white);
	PaintRect(&destRect);
	PlotIconID(&destRect, atAbsoluteCenter, ttNone, nativeIconPtr->id);
    } else if (nativeIconPtr->type == TYPE2) {
	icon = GetIcon(nativeIconPtr->id);
	if (icon != NULL) {
	    RGBColor black = {0, 0, 0};
	
	    RGBForeColor(&black);
	    PlotIcon(&destRect, icon);
	    ReleaseResource(icon);
	}
    }

    SetGWorld(saveWorld, saveDevice);
    return pix;
}

/*
 *----------------------------------------------------------------------
 *
 * TkpGetNativeAppBitmap --
 *
 *	Add native bitmaps.
 *
 * Results:
 *	A standard Tcl result.  If an error occurs then TCL_ERROR is
 *	returned and a message is left in the interp's result.
 *
 * Side effects:
 *	"Name" is entered into the bitmap table and may be used from
 *	here on to refer to the given bitmap.
 *
 *----------------------------------------------------------------------
 */

Pixmap
TkpGetNativeAppBitmap(
    Display *display,	/* The display. */
    CONST char *name,	/* The name of the bitmap. */
    int *width,		/* The width & height of the bitmap. */
    int *height)
{
    Pixmap pix;
    CGrafPtr saveWorld;
    GDHandle saveDevice;
    GWorldPtr destPort;
    Rect destRect;
    Handle resource;
    int type, destWrote;
    Str255 nativeName;
    
    /*
     * macRoman is the encoding that the resource fork uses.
     */

    Tcl_UtfToExternal(NULL, Tcl_GetEncoding(NULL, "macRoman"), name,
	    strlen(name), 0, NULL, 
	    (char *) &nativeName[1],
	    255, NULL, &destWrote, NULL); /* Internalize native */
    nativeName[0] = destWrote;

    resource = GetNamedResource('cicn', nativeName);
    if (resource != NULL) {
	type = TYPE3;
    } else {
	resource = GetNamedResource('ICON', nativeName);
	if (resource != NULL) {
	    type = TYPE2;
	}
    }
    
    if (resource == NULL) {
	return NULL;
    }
    
    pix = Tk_GetPixmap(display, None, 32, 32, 0);
    destPort = TkMacGetDrawablePort(pix);

    GetGWorld(&saveWorld, &saveDevice);
    SetGWorld(destPort, NULL);
    
    SetRect(&destRect, 0, 0, 32, 32);
    if (type == TYPE2) {
	RGBColor black = {0, 0, 0};
	
	RGBForeColor(&black);
	PlotIcon(&destRect, resource);
	ReleaseResource(resource);
    } else if (type == TYPE3) {
	RGBColor white = {0xFFFF, 0xFFFF, 0xFFFF};
	short id;
	ResType theType;
	Str255 dummy;
	
	/*
	 * We need to first paint the background white.  Also, for
	 * some reason we *must* use GetCIcon instead of GetNamedResource
	 * for PlotCIcon to work - so we use GetResInfo to get the id.
	 */
	RGBForeColor(&white);
	PaintRect(&destRect);
	GetResInfo(resource, &id, &theType, dummy);
	ReleaseResource(resource);
	resource = (Handle) GetCIcon(id);
	PlotCIcon(&destRect, (CIconHandle) resource);
	DisposeCIcon((CIconHandle) resource);
    }
    
    *width = 32;
    *height = 32;
    SetGWorld(saveWorld, saveDevice);
    return pix;
}
