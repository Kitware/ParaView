# tk.decls --
#
#	This file contains the declarations for all supported public
#	functions that are exported by the Tk library via the stubs table.
#	This file is used to generate the tkDecls.h, tkPlatDecls.h,
#	tkStub.c, and tkPlatStub.c files.
#	
#
# Copyright (c) 1998-1999 by Scriptics Corporation.
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# 
# RCS: @(#) Id

library tk

# Define the tk interface with 3 sub interfaces:
#     tkPlat	 - platform specific public
#     tkInt	 - generic private
#     tkPlatInt - platform specific private

interface tk
hooks {tkPlat tkInt tkIntPlat tkIntXlib}

# Declare each of the functions in the public Tk interface.  Note that
# the an index should never be reused for a different function in order
# to preserve backwards compatibility.

declare 0 generic {
    void Tk_MainLoop (void)
}

declare 1 generic {
    XColor *Tk_3DBorderColor (Tk_3DBorder border)
}

declare 2 generic {
    GC Tk_3DBorderGC (Tk_Window tkwin, Tk_3DBorder border, \
	    int which)
}

declare 3 generic {
    void Tk_3DHorizontalBevel (Tk_Window tkwin, \
	    Drawable drawable, Tk_3DBorder border, int x, \
	    int y, int width, int height, int leftIn, \
	    int rightIn, int topBevel, int relief)
}

declare 4 generic {
    void Tk_3DVerticalBevel (Tk_Window tkwin, \
	    Drawable drawable, Tk_3DBorder border, int x, \
	    int y, int width, int height, int leftBevel, \
	    int relief)
}

declare 5 generic {
    void Tk_AddOption (Tk_Window tkwin, char *name, \
	    char *value, int priority)
}

declare 6 generic {
    void Tk_BindEvent (Tk_BindingTable bindingTable, \
	    XEvent *eventPtr, Tk_Window tkwin, int numObjects, \
	    ClientData *objectPtr)
}

declare 7 generic {
    void Tk_CanvasDrawableCoords (Tk_Canvas canvas, \
	    double x, double y, short *drawableXPtr, \
	    short *drawableYPtr)
}

declare 8 generic {
    void Tk_CanvasEventuallyRedraw (Tk_Canvas canvas, int x1, int y1, \
	    int x2, int y2)
}

declare 9 generic {
    int Tk_CanvasGetCoord (Tcl_Interp *interp, \
	    Tk_Canvas canvas, char *str, double *doublePtr)
}

declare 10 generic {
    Tk_CanvasTextInfo *Tk_CanvasGetTextInfo (Tk_Canvas canvas)
}

declare 11 generic {
    int Tk_CanvasPsBitmap (Tcl_Interp *interp, \
	    Tk_Canvas canvas, Pixmap bitmap, int x, int y, \
	    int width, int height)
}

declare 12 generic {
    int Tk_CanvasPsColor (Tcl_Interp *interp, \
	    Tk_Canvas canvas, XColor *colorPtr)
}

declare 13 generic {
    int Tk_CanvasPsFont (Tcl_Interp *interp, \
	    Tk_Canvas canvas, Tk_Font font)
}

declare 14 generic {
    void Tk_CanvasPsPath (Tcl_Interp *interp, \
	    Tk_Canvas canvas, double *coordPtr, int numPoints)
}

declare 15 generic {
    int Tk_CanvasPsStipple (Tcl_Interp *interp, \
	    Tk_Canvas canvas, Pixmap bitmap)
}

declare 16 generic {
    double Tk_CanvasPsY (Tk_Canvas canvas, double y)
}

declare 17 generic {
    void Tk_CanvasSetStippleOrigin (Tk_Canvas canvas, GC gc)
}

declare 18 generic {
    int Tk_CanvasTagsParseProc (ClientData clientData, Tcl_Interp *interp, \
	    Tk_Window tkwin, char *value, char *widgRec, int offset)
}

declare 19 generic {
    char * Tk_CanvasTagsPrintProc (ClientData clientData, Tk_Window tkwin, \
	    char *widgRec, int offset, Tcl_FreeProc **freeProcPtr)
}

declare 20 generic {
    Tk_Window	Tk_CanvasTkwin (Tk_Canvas canvas)
}

declare 21 generic {
    void Tk_CanvasWindowCoords (Tk_Canvas canvas, double x, double y, \
	    short *screenXPtr, short *screenYPtr)
}

declare 22 generic {
    void Tk_ChangeWindowAttributes (Tk_Window tkwin, unsigned long valueMask, \
	    XSetWindowAttributes *attsPtr)
}

declare 23 generic {
    int Tk_CharBbox (Tk_TextLayout layout, int index, int *xPtr, \
	    int *yPtr, int *widthPtr, int *heightPtr)
}

declare 24 generic {
    void Tk_ClearSelection (Tk_Window tkwin, Atom selection)
}

declare 25 generic {
    int Tk_ClipboardAppend (Tcl_Interp *interp,Tk_Window tkwin, \
	    Atom target, Atom format, char* buffer)
}

declare 26 generic {
    int Tk_ClipboardClear (Tcl_Interp *interp, Tk_Window tkwin)
}

declare 27 generic {
    int Tk_ConfigureInfo (Tcl_Interp *interp, \
	    Tk_Window tkwin, Tk_ConfigSpec *specs, \
	    char *widgRec, char *argvName, int flags)
}

declare 28 generic {
    int Tk_ConfigureValue (Tcl_Interp *interp, \
	    Tk_Window tkwin, Tk_ConfigSpec *specs, \
	    char *widgRec, char *argvName, int flags)
}

declare 29 generic {
    int Tk_ConfigureWidget (Tcl_Interp *interp, \
	    Tk_Window tkwin, Tk_ConfigSpec *specs, \
	    int argc, char **argv, char *widgRec, \
	    int flags)
}

declare 30 generic {
    void Tk_ConfigureWindow (Tk_Window tkwin, \
	    unsigned int valueMask, XWindowChanges *valuePtr)
}

declare 31 generic {
    Tk_TextLayout Tk_ComputeTextLayout (Tk_Font font, \
	    CONST char *str, int numChars, int wrapLength, \
	    Tk_Justify justify, int flags, int *widthPtr, \
	    int *heightPtr)
}

declare 32 generic {
    Tk_Window Tk_CoordsToWindow (int rootX, int rootY, Tk_Window tkwin)
}

declare 33 generic {
    unsigned long Tk_CreateBinding (Tcl_Interp *interp, \
	    Tk_BindingTable bindingTable, ClientData object, \
	    char *eventStr, char *command, int append)
}

declare 34 generic {
    Tk_BindingTable Tk_CreateBindingTable (Tcl_Interp *interp)
}

declare 35 generic {
    Tk_ErrorHandler Tk_CreateErrorHandler (Display *display, \
	    int errNum, int request, int minorCode, \
	    Tk_ErrorProc *errorProc, ClientData clientData)
}

declare 36 generic {
    void Tk_CreateEventHandler (Tk_Window token, \
	    unsigned long mask, Tk_EventProc *proc, \
	    ClientData clientData)
}

declare 37 generic {
    void Tk_CreateGenericHandler (Tk_GenericProc *proc, ClientData clientData)
}

declare 38 generic {
    void Tk_CreateImageType (Tk_ImageType *typePtr)
}

declare 39 generic {
    void Tk_CreateItemType (Tk_ItemType *typePtr)
}

declare 40 generic {
    void Tk_CreatePhotoImageFormat (Tk_PhotoImageFormat *formatPtr)
}

declare 41 generic {
    void Tk_CreateSelHandler (Tk_Window tkwin, \
	    Atom selection, Atom target, \
	    Tk_SelectionProc *proc, ClientData clientData, \
	    Atom format)
}

declare 42 generic {
    Tk_Window Tk_CreateWindow (Tcl_Interp *interp, \
	    Tk_Window parent, char *name, char *screenName)
}

declare 43 generic {
    Tk_Window Tk_CreateWindowFromPath (Tcl_Interp *interp, Tk_Window tkwin, \
	    char *pathName, char *screenName)
}

declare 44 generic {
    int Tk_DefineBitmap (Tcl_Interp *interp, CONST char *name, char *source, \
	    int width, int height)
}

declare 45 generic {
    void Tk_DefineCursor (Tk_Window window, Tk_Cursor cursor)
}

declare 46 generic {
    void Tk_DeleteAllBindings (Tk_BindingTable bindingTable, ClientData object)
}

declare 47 generic {
    int Tk_DeleteBinding (Tcl_Interp *interp, \
	    Tk_BindingTable bindingTable, ClientData object, \
	    char *eventStr)
}

declare 48 generic {
    void Tk_DeleteBindingTable (Tk_BindingTable bindingTable)
}

declare 49 generic {
    void Tk_DeleteErrorHandler (Tk_ErrorHandler handler)
}

declare 50 generic {
    void Tk_DeleteEventHandler (Tk_Window token, \
	    unsigned long mask, Tk_EventProc *proc, \
	    ClientData clientData)
}

declare 51 generic {
    void Tk_DeleteGenericHandler (Tk_GenericProc *proc, ClientData clientData)
}

declare 52 generic {
    void Tk_DeleteImage (Tcl_Interp *interp, char *name)
}

declare 53 generic {
    void Tk_DeleteSelHandler (Tk_Window tkwin, Atom selection, Atom target)
}

declare 54 generic {
    void Tk_DestroyWindow (Tk_Window tkwin)
}

declare 55 generic {
    char * Tk_DisplayName (Tk_Window tkwin)
}

declare 56 generic {
    int Tk_DistanceToTextLayout (Tk_TextLayout layout, int x, int y)
}

declare 57 generic {
    void Tk_Draw3DPolygon (Tk_Window tkwin, \
	    Drawable drawable, Tk_3DBorder border, \
	    XPoint *pointPtr, int numPoints, int borderWidth, \
	    int leftRelief)
}

declare 58 generic {
    void Tk_Draw3DRectangle (Tk_Window tkwin, Drawable drawable, \
	    Tk_3DBorder border, int x, int y, int width, int height, \
	    int borderWidth, int relief)
}

declare 59 generic {
    void Tk_DrawChars (Display *display, Drawable drawable, GC gc, \
	    Tk_Font tkfont, CONST char *source, int numBytes, int x, int y)
}

declare 60 generic {
    void Tk_DrawFocusHighlight (Tk_Window tkwin, GC gc, int width, \
	    Drawable drawable)
}

declare 61 generic {
    void Tk_DrawTextLayout (Display *display, \
	    Drawable drawable, GC gc, Tk_TextLayout layout, \
	    int x, int y, int firstChar, int lastChar)
}

declare 62 generic {
    void Tk_Fill3DPolygon (Tk_Window tkwin, \
	    Drawable drawable, Tk_3DBorder border, \
	    XPoint *pointPtr, int numPoints, int borderWidth, \
	    int leftRelief)
}

declare 63 generic {
    void Tk_Fill3DRectangle (Tk_Window tkwin, \
	    Drawable drawable, Tk_3DBorder border, int x, \
	    int y, int width, int height, int borderWidth, \
	    int relief)
}

declare 64 generic {
    Tk_PhotoHandle Tk_FindPhoto (Tcl_Interp *interp, char *imageName)
}

declare 65 generic {
    Font Tk_FontId (Tk_Font font)
}

declare 66 generic {
    void Tk_Free3DBorder (Tk_3DBorder border)
}

declare 67 generic {
    void Tk_FreeBitmap (Display *display, Pixmap bitmap)
}

declare 68 generic {
    void Tk_FreeColor (XColor *colorPtr)
}

declare 69 generic {
    void Tk_FreeColormap (Display *display, Colormap colormap)
}

declare 70 generic {
    void Tk_FreeCursor (Display *display, Tk_Cursor cursor)
}

declare 71 generic {
    void Tk_FreeFont (Tk_Font f)
}

declare 72 generic {
    void Tk_FreeGC (Display *display, GC gc)
}

declare 73 generic {
    void Tk_FreeImage (Tk_Image image)
}

declare 74 generic {
    void Tk_FreeOptions (Tk_ConfigSpec *specs, \
	    char *widgRec, Display *display, int needFlags)
}

declare 75 generic {
    void Tk_FreePixmap (Display *display, Pixmap pixmap)
}

declare 76 generic {
    void Tk_FreeTextLayout (Tk_TextLayout textLayout)
}

declare 77 generic {
    void Tk_FreeXId (Display *display, XID xid)
}

declare 78 generic {
    GC Tk_GCForColor (XColor *colorPtr, Drawable drawable)
}

declare 79 generic {
    void Tk_GeometryRequest (Tk_Window tkwin, int reqWidth,  int reqHeight)
}

declare 80 generic {
    Tk_3DBorder	Tk_Get3DBorder (Tcl_Interp *interp, Tk_Window tkwin, \
	    Tk_Uid colorName)
}

declare 81 generic {
    void Tk_GetAllBindings (Tcl_Interp *interp, \
	    Tk_BindingTable bindingTable, ClientData object)
}

declare 82 generic {
    int Tk_GetAnchor (Tcl_Interp *interp, \
	    char *str, Tk_Anchor *anchorPtr)
}

declare 83 generic {
    char * Tk_GetAtomName (Tk_Window tkwin, Atom atom)
}

declare 84 generic {
    char * Tk_GetBinding (Tcl_Interp *interp, \
	    Tk_BindingTable bindingTable, ClientData object, \
	    char *eventStr)
}

declare 85 generic {
    Pixmap Tk_GetBitmap (Tcl_Interp *interp, Tk_Window tkwin, CONST char * str)
}

declare 86 generic {
    Pixmap Tk_GetBitmapFromData (Tcl_Interp *interp, \
	    Tk_Window tkwin, char *source, int width, int height)
}

declare 87 generic {
    int Tk_GetCapStyle (Tcl_Interp *interp, char *str, int *capPtr)
}

declare 88 generic {
    XColor * Tk_GetColor (Tcl_Interp *interp, Tk_Window tkwin, Tk_Uid name)
}

declare 89 generic {
    XColor * Tk_GetColorByValue (Tk_Window tkwin, XColor *colorPtr)
}

declare 90 generic {
    Colormap Tk_GetColormap (Tcl_Interp *interp, Tk_Window tkwin, char *str)
}

declare 91 generic {
    Tk_Cursor Tk_GetCursor (Tcl_Interp *interp, Tk_Window tkwin, \
	    Tk_Uid str)
}

declare 92 generic {
    Tk_Cursor Tk_GetCursorFromData (Tcl_Interp *interp, \
	    Tk_Window tkwin, char *source, char *mask, \
	    int width, int height, int xHot, int yHot, \
	    Tk_Uid fg, Tk_Uid bg)
}

declare 93 generic {
    Tk_Font Tk_GetFont (Tcl_Interp *interp, \
	    Tk_Window tkwin, CONST char *str)
}

declare 94 generic {
    Tk_Font Tk_GetFontFromObj (Tk_Window tkwin, Tcl_Obj *objPtr)
}

declare 95 generic {
    void Tk_GetFontMetrics (Tk_Font font, Tk_FontMetrics *fmPtr)
}

declare 96 generic {
    GC Tk_GetGC (Tk_Window tkwin, unsigned long valueMask, XGCValues *valuePtr)
}

declare 97 generic {
    Tk_Image Tk_GetImage (Tcl_Interp *interp, Tk_Window tkwin, char *name, \
	    Tk_ImageChangedProc *changeProc, ClientData clientData)
}

declare 98 generic {
    ClientData Tk_GetImageMasterData (Tcl_Interp *interp, \
	    char *name, Tk_ImageType **typePtrPtr)
}

declare 99 generic {
    Tk_ItemType * Tk_GetItemTypes (void)
}

declare 100 generic {
    int Tk_GetJoinStyle (Tcl_Interp *interp, char *str, int *joinPtr)
}

declare 101 generic {
    int Tk_GetJustify (Tcl_Interp *interp, \
	    char *str, Tk_Justify *justifyPtr)
}

declare 102 generic {
    int Tk_GetNumMainWindows (void)
}

declare 103 generic {
    Tk_Uid Tk_GetOption (Tk_Window tkwin, char *name, char *className)
}

declare 104 generic {
    int Tk_GetPixels (Tcl_Interp *interp, \
	    Tk_Window tkwin, char *str, int *intPtr)
}

declare 105 generic {
    Pixmap Tk_GetPixmap (Display *display, Drawable d, \
	    int width, int height, int depth)
}

declare 106 generic {
    int Tk_GetRelief (Tcl_Interp *interp, char *name, int *reliefPtr)
}

declare 107 generic {
    void Tk_GetRootCoords (Tk_Window tkwin, int *xPtr, int *yPtr)
}

declare 108 generic {
    int Tk_GetScrollInfo (Tcl_Interp *interp, \
	    int argc, char **argv, double *dblPtr, int *intPtr)
}

declare 109 generic {
    int Tk_GetScreenMM (Tcl_Interp *interp, \
	    Tk_Window tkwin, char *str, double *doublePtr)
}

declare 110 generic {
    int Tk_GetSelection (Tcl_Interp *interp, \
	    Tk_Window tkwin, Atom selection, Atom target, \
	    Tk_GetSelProc *proc, ClientData clientData)
}

declare 111 generic {
    Tk_Uid Tk_GetUid (CONST char *str)
}

declare 112 generic {
    Visual * Tk_GetVisual (Tcl_Interp *interp, \
	    Tk_Window tkwin, char *str, int *depthPtr, \
	    Colormap *colormapPtr)
}

declare 113 generic {
    void Tk_GetVRootGeometry (Tk_Window tkwin, \
	    int *xPtr, int *yPtr, int *widthPtr, int *heightPtr)
}

declare 114 generic {
    int Tk_Grab (Tcl_Interp *interp, Tk_Window tkwin, int grabGlobal)
}

declare 115 generic {
    void Tk_HandleEvent (XEvent *eventPtr)
}

declare 116 generic {
    Tk_Window Tk_IdToWindow (Display *display, Window window)
}

declare 117 generic {
    void Tk_ImageChanged (Tk_ImageMaster master, int x, int y, \
	    int width, int height, int imageWidth, int imageHeight)
}

declare 118 generic {
    int Tk_Init (Tcl_Interp *interp)
}

declare 119 generic {
    Atom Tk_InternAtom (Tk_Window tkwin, char *name)
}

declare 120 generic {
    int Tk_IntersectTextLayout (Tk_TextLayout layout, int x, int y, \
	    int width, int height)
}

declare 121 generic {
    void Tk_MaintainGeometry (Tk_Window slave, \
	    Tk_Window master, int x, int y, int width, int height)
}

declare 122 generic {
    Tk_Window Tk_MainWindow (Tcl_Interp *interp)
}

declare 123 generic {
    void Tk_MakeWindowExist (Tk_Window tkwin)
}

declare 124 generic {
    void Tk_ManageGeometry (Tk_Window tkwin, \
	    Tk_GeomMgr *mgrPtr, ClientData clientData)
}

declare 125 generic {
    void Tk_MapWindow (Tk_Window tkwin)
}

declare 126 generic {
    int Tk_MeasureChars (Tk_Font tkfont, \
	    CONST char *source, int numBytes, int maxPixels, \
	    int flags, int *lengthPtr)
}

declare 127 generic {
    void Tk_MoveResizeWindow (Tk_Window tkwin, \
	    int x, int y, int width, int height)
}

declare 128 generic {
    void Tk_MoveWindow (Tk_Window tkwin, int x, int y)
}

declare 129 generic {
    void Tk_MoveToplevelWindow (Tk_Window tkwin, int x, int y)
}

declare 130 generic {
    char * Tk_NameOf3DBorder (Tk_3DBorder border)
}

declare 131 generic {
    char * Tk_NameOfAnchor (Tk_Anchor anchor)
}

declare 132 generic {
    char * Tk_NameOfBitmap (Display *display, Pixmap bitmap)
}

declare 133 generic {
    char * Tk_NameOfCapStyle (int cap)
}

declare 134 generic {
    char * Tk_NameOfColor (XColor *colorPtr)
}

declare 135 generic {
    char * Tk_NameOfCursor (Display *display, Tk_Cursor cursor)
}

declare 136 generic {
    char * Tk_NameOfFont (Tk_Font font)
}

declare 137 generic {
    char * Tk_NameOfImage (Tk_ImageMaster imageMaster)
}

declare 138 generic {
    char * Tk_NameOfJoinStyle (int join)
}

declare 139 generic {
    char * Tk_NameOfJustify (Tk_Justify justify)
}

declare 140 generic {
    char * Tk_NameOfRelief (int relief)
}

declare 141 generic {
    Tk_Window Tk_NameToWindow (Tcl_Interp *interp, \
	    char *pathName, Tk_Window tkwin)
}

declare 142 generic {
    void Tk_OwnSelection (Tk_Window tkwin, \
	    Atom selection, Tk_LostSelProc *proc, \
	    ClientData clientData)
}

declare 143 generic {
    int Tk_ParseArgv (Tcl_Interp *interp, \
	    Tk_Window tkwin, int *argcPtr, char **argv, \
	    Tk_ArgvInfo *argTable, int flags)
}

declare 144 generic {
    void Tk_PhotoPutBlock (Tk_PhotoHandle handle, \
	    Tk_PhotoImageBlock *blockPtr, int x, int y, \
	    int width, int height)
}

declare 145 generic {
    void Tk_PhotoPutZoomedBlock (Tk_PhotoHandle handle, \
	    Tk_PhotoImageBlock *blockPtr, int x, int y, \
	    int width, int height, int zoomX, int zoomY, \
	    int subsampleX, int subsampleY)
}

declare 146 generic {
    int Tk_PhotoGetImage (Tk_PhotoHandle handle, Tk_PhotoImageBlock *blockPtr)
}

declare 147 generic {
    void Tk_PhotoBlank (Tk_PhotoHandle handle)
}

declare 148 generic {
    void Tk_PhotoExpand (Tk_PhotoHandle handle, int width, int height )
}

declare 149 generic {
    void Tk_PhotoGetSize (Tk_PhotoHandle handle, int *widthPtr, int *heightPtr)
}

declare 150 generic {
    void Tk_PhotoSetSize (Tk_PhotoHandle handle, int width, int height)
}

declare 151 generic {
    int Tk_PointToChar (Tk_TextLayout layout, int x, int y)
}

declare 152 generic {
    int Tk_PostscriptFontName (Tk_Font tkfont, Tcl_DString *dsPtr)
}

declare 153 generic {
    void Tk_PreserveColormap (Display *display, Colormap colormap)
}

declare 154 generic {
    void Tk_QueueWindowEvent (XEvent *eventPtr, Tcl_QueuePosition position)
}

declare 155 generic {
    void Tk_RedrawImage (Tk_Image image, int imageX, \
	    int imageY, int width, int height, \
	    Drawable drawable, int drawableX, int drawableY)
}

declare 156 generic {
    void Tk_ResizeWindow (Tk_Window tkwin, int width, int height)
}

declare 157 generic {
    int Tk_RestackWindow (Tk_Window tkwin, int aboveBelow, Tk_Window other)
}

declare 158 generic {
    Tk_RestrictProc *Tk_RestrictEvents (Tk_RestrictProc *proc, \
	    ClientData arg, ClientData *prevArgPtr)
}

declare 159 generic {
    int Tk_SafeInit (Tcl_Interp *interp)
}

declare 160 generic {
    char * Tk_SetAppName (Tk_Window tkwin, char *name)
}

declare 161 generic {
    void Tk_SetBackgroundFromBorder (Tk_Window tkwin, Tk_3DBorder border)
}

declare 162 generic {
    void Tk_SetClass (Tk_Window tkwin, char *className)
}

declare 163 generic {
    void Tk_SetGrid (Tk_Window tkwin, int reqWidth, int reqHeight, \
	    int gridWidth, int gridHeight)
}

declare 164 generic {
    void Tk_SetInternalBorder (Tk_Window tkwin, int width)
}

declare 165 generic {
    void Tk_SetWindowBackground (Tk_Window tkwin, unsigned long pixel)
}

declare 166 generic {
    void Tk_SetWindowBackgroundPixmap (Tk_Window tkwin, Pixmap pixmap)
}

declare 167 generic {
    void Tk_SetWindowBorder (Tk_Window tkwin, unsigned long pixel)
}

declare 168 generic {
    void Tk_SetWindowBorderWidth (Tk_Window tkwin, int width)
}

declare 169 generic {
    void Tk_SetWindowBorderPixmap (Tk_Window tkwin, Pixmap pixmap)
}

declare 170 generic {
    void Tk_SetWindowColormap (Tk_Window tkwin, Colormap colormap)
}

declare 171 generic {
    int Tk_SetWindowVisual (Tk_Window tkwin, Visual *visual, int depth,\
	    Colormap colormap)
}

declare 172 generic {
    void Tk_SizeOfBitmap (Display *display, Pixmap bitmap, int *widthPtr, \
	    int *heightPtr)
}

declare 173 generic {
    void Tk_SizeOfImage (Tk_Image image, int *widthPtr, int *heightPtr)
}

declare 174 generic {
    int Tk_StrictMotif (Tk_Window tkwin)
}

declare 175 generic {
    void Tk_TextLayoutToPostscript (Tcl_Interp *interp, Tk_TextLayout layout)
}

declare 176 generic {
    int Tk_TextWidth (Tk_Font font, CONST char *str, int numBytes)
}

declare 177 generic {
    void Tk_UndefineCursor (Tk_Window window)
}

declare 178 generic {
    void Tk_UnderlineChars (Display *display, \
	    Drawable drawable, GC gc, Tk_Font tkfont, \
	    CONST char *source, int x, int y, int firstByte, \
	    int lastByte)
}

declare 179 generic {
    void Tk_UnderlineTextLayout (Display *display, Drawable drawable, GC gc, \
	    Tk_TextLayout layout, int x, int y, \
	    int underline)
}

declare 180 generic {
    void Tk_Ungrab (Tk_Window tkwin)
}

declare 181 generic {
    void Tk_UnmaintainGeometry (Tk_Window slave, Tk_Window master)
}

declare 182 generic {
    void Tk_UnmapWindow (Tk_Window tkwin)
}

declare 183 generic {
    void Tk_UnsetGrid (Tk_Window tkwin)
}

declare 184 generic {
    void Tk_UpdatePointer (Tk_Window tkwin, int x, int y, int state)
}

# new functions for 8.1

declare 185 generic {
    Pixmap  Tk_AllocBitmapFromObj (Tcl_Interp *interp, Tk_Window tkwin, \
    Tcl_Obj *objPtr)
}

declare 186 generic {
    Tk_3DBorder Tk_Alloc3DBorderFromObj (Tcl_Interp *interp, Tk_Window tkwin, \
	    Tcl_Obj *objPtr)
}

declare 187 generic {
    XColor *  Tk_AllocColorFromObj (Tcl_Interp *interp, Tk_Window tkwin, \
	    Tcl_Obj *objPtr)
}

declare 188 generic {
    Tk_Cursor Tk_AllocCursorFromObj (Tcl_Interp *interp, Tk_Window tkwin, \
	    Tcl_Obj *objPtr)
}

declare 189 generic {
    Tk_Font  Tk_AllocFontFromObj (Tcl_Interp *interp, Tk_Window tkwin, \
	    Tcl_Obj *objPtr)

}

declare 190 generic {
    Tk_OptionTable Tk_CreateOptionTable (Tcl_Interp *interp, \
	    CONST Tk_OptionSpec *templatePtr)
}

declare 191 generic {
    void  Tk_DeleteOptionTable (Tk_OptionTable optionTable)
}

declare 192 generic {
    void  Tk_Free3DBorderFromObj (Tk_Window tkwin, Tcl_Obj *objPtr)
}

declare 193 generic {
    void  Tk_FreeBitmapFromObj (Tk_Window tkwin, Tcl_Obj *objPtr)
}

declare 194 generic {
    void  Tk_FreeColorFromObj (Tk_Window tkwin, Tcl_Obj *objPtr)
}

declare 195 generic {
    void  Tk_FreeConfigOptions (char *recordPtr, Tk_OptionTable optionToken, \
	    Tk_Window tkwin)

}

declare 196 generic {
    void  Tk_FreeSavedOptions (Tk_SavedOptions *savePtr)
}

declare 197 generic {
    void  Tk_FreeCursorFromObj (Tk_Window tkwin, Tcl_Obj *objPtr)
}

declare 198 generic {
    void  Tk_FreeFontFromObj (Tk_Window tkwin, Tcl_Obj *objPtr)
}

declare 199 generic {
    Tk_3DBorder Tk_Get3DBorderFromObj (Tk_Window tkwin, Tcl_Obj *objPtr)
}

declare 200 generic {
    int  Tk_GetAnchorFromObj (Tcl_Interp *interp, Tcl_Obj *objPtr, \
	    Tk_Anchor *anchorPtr)
}

declare 201 generic {
    Pixmap  Tk_GetBitmapFromObj (Tk_Window tkwin, Tcl_Obj *objPtr)
}

declare 202 generic {
    XColor *  Tk_GetColorFromObj (Tk_Window tkwin, Tcl_Obj *objPtr)
}

declare 203 generic {
    Tk_Cursor Tk_GetCursorFromObj (Tk_Window tkwin, Tcl_Obj *objPtr)
}

declare 204 generic {
    Tcl_Obj * Tk_GetOptionInfo (Tcl_Interp *interp, \
	    char *recordPtr, Tk_OptionTable optionTable, \
	    Tcl_Obj *namePtr, Tk_Window tkwin)
}

declare 205 generic {
    Tcl_Obj * Tk_GetOptionValue (Tcl_Interp *interp, char *recordPtr, \
	    Tk_OptionTable optionTable, Tcl_Obj *namePtr, Tk_Window tkwin)
}

declare 206 generic {
    int  Tk_GetJustifyFromObj (Tcl_Interp *interp, \
	    Tcl_Obj *objPtr, Tk_Justify *justifyPtr)
}

declare 207 generic {
    int  Tk_GetMMFromObj (Tcl_Interp *interp, \
	    Tk_Window tkwin, Tcl_Obj *objPtr, double *doublePtr)
}

declare 208 generic {
    int  Tk_GetPixelsFromObj (Tcl_Interp *interp, \
	    Tk_Window tkwin, Tcl_Obj *objPtr, int *intPtr)
}

declare 209 generic {
    int  Tk_GetReliefFromObj (Tcl_Interp *interp, \
	    Tcl_Obj *objPtr, int *resultPtr)
}

declare 210 generic {
    int  Tk_GetScrollInfoObj (Tcl_Interp *interp, \
	    int objc, Tcl_Obj *CONST objv[], double *dblPtr, int *intPtr)
}

declare 211 generic {
    int  Tk_InitOptions (
       Tcl_Interp *interp, char *recordPtr, \
	       Tk_OptionTable optionToken, Tk_Window tkwin)
}

declare 212 generic {
    void  Tk_MainEx (int argc, char **argv, Tcl_AppInitProc *appInitProc, \
	    Tcl_Interp *interp)
}

declare 213 generic {
    void  Tk_RestoreSavedOptions (Tk_SavedOptions *savePtr)
}

declare 214 generic {
    int  Tk_SetOptions (Tcl_Interp *interp, char *recordPtr, \
	    Tk_OptionTable optionTable, int objc, \
	    Tcl_Obj *CONST objv[], Tk_Window tkwin, \
	    Tk_SavedOptions *savePtr, int *maskPtr)
}

declare 215 generic {
    void Tk_InitConsoleChannels(Tcl_Interp *interp)
}

declare 216 generic {
    int Tk_CreateConsoleWindow(Tcl_Interp *interp)
}

# Define the platform specific public Tk interface.  These functions are
# only available on the designated platform.

interface tkPlat

# Unix specific functions
#   (none)

# Windows specific functions

declare 0 win {
    Window Tk_AttachHWND (Tk_Window tkwin, HWND hwnd)
}

declare 1 win {
    HINSTANCE Tk_GetHINSTANCE (void)
}

declare 2 win {
    HWND Tk_GetHWND (Window window)
}

declare 3 win {
    Tk_Window Tk_HWNDToWindow (HWND hwnd)
}

declare 4 win {
    void Tk_PointerEvent (HWND hwnd, int x, int y)
}

declare 5 win {
    int Tk_TranslateWinEvent (HWND hwnd, \
	    UINT message, WPARAM wParam, LPARAM lParam, LRESULT *result)
}

# Mac specific functions

declare 0 mac {
    void Tk_MacSetEmbedHandler ( \
	    Tk_MacEmbedRegisterWinProc *registerWinProcPtr, \
	    Tk_MacEmbedGetGrafPortProc *getPortProcPtr, \
	    Tk_MacEmbedMakeContainerExistProc *containerExistProcPtr, \
	    Tk_MacEmbedGetClipProc *getClipProc, \
	    Tk_MacEmbedGetOffsetInParentProc *getOffsetProc)
}
 
declare 1 mac {
    void Tk_MacTurnOffMenus (void)
}

declare 2 mac {
    void Tk_MacTkOwnsCursor (int tkOwnsIt)
}

declare 3 mac {
    void TkMacInitMenus (Tcl_Interp *interp)
}

declare 4 mac {
    void TkMacInitAppleEvents (Tcl_Interp *interp)
}

declare 5 mac {
    int TkMacConvertEvent (EventRecord *eventPtr)
}

declare 6 mac {
    int TkMacConvertTkEvent (EventRecord *eventPtr, Window window)
}

declare 7 mac {
    void TkGenWMConfigureEvent (Tk_Window tkwin, \
	    int x, int y, int width, int height, int flags)
}

declare 8 mac {
    void TkMacInvalClipRgns (TkWindow *winPtr)
}

declare 9 mac {
    int TkMacHaveAppearance (void)
}

declare 10 mac {
    GWorldPtr TkMacGetDrawablePort (Drawable drawable)
}

