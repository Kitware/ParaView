/*
 * tkMacInt.h --
 *
 *	Declarations of Macintosh specific exported variables and procedures.
 *
 * Copyright (c) 1995-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#ifndef _TKMAC
#define _TKMAC

#ifndef _TK
#include <tk.h>
#endif

#ifndef _TKINT
#include "tkInt.h"
#endif

#include <Windows.h>
#include <QDOffscreen.h>

#ifdef BUILD_tk
# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLEXPORT
#endif

/*
 * This variable is exported and can be used by extensions.  It is the
 * way Tk extensions should access the QD Globals.  This is so Tk
 * can support embedding itself in another window. 
 */

EXTERN QDGlobalsPtr tcl_macQdPtr;

/*
 * Structures and function types for handling Netscape-type in process
 * embedding where Tk does not control the top-level
 */
typedef  int (Tk_MacEmbedRegisterWinProc) (int winID, Tk_Window window);
typedef GWorldPtr (Tk_MacEmbedGetGrafPortProc) (Tk_Window window); 
typedef int (Tk_MacEmbedMakeContainerExistProc) (Tk_Window window); 
typedef void (Tk_MacEmbedGetClipProc) (Tk_Window window, RgnHandle rgn); 
typedef void (Tk_MacEmbedGetOffsetInParentProc) (Tk_Window window, Point *ulCorner);

#include "tkPlatDecls.h"

# undef TCL_STORAGE_CLASS
# define TCL_STORAGE_CLASS DLLIMPORT

#endif /* _TKMAC */
