/* 
 * tkMacTclCode.r --
 *
 *	This file creates resources from the Tcl code that is
 *	usually stored in the TCL_LIBRARY
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tkMacTclCode.r 1.1 98/01/21 22:22:38
 */

#include <Types.r>
#include <SysTypes.r>

#define TK_LIBRARY_RESOURCES 3000

/* 
 * The mechanisim below loads Tcl source into the resource fork of the
 * application.  The example below creates a TEXT resource named
 * "Init" from the file "init.tcl".  This allows applications to use
 * Tcl to define the behavior of the application without having to
 * require some predetermined file structure - all needed Tcl "files"
 * are located within the application.  To source a file for the
 * resource fork the source command has been modified to support
 * sourcing from resources.  In the below case "source -rsrc {Init}"
 * will load the TEXT resource named "Init".
 */

read 'TEXT' (TK_LIBRARY_RESOURCES+1, "tk", purgeable,preload) 
	"::library:tk.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+2, "button", purgeable) 
	"::library:button.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+3, "dialog", purgeable) 
	"::library:dialog.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+4, "entry", purgeable) 
	"::library:entry.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+5, "focus", purgeable) 
	"::library:focus.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+6, "listbox", purgeable) 
	"::library:listbox.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+7, "menu", purgeable) 
	"::library:menu.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+8, "optMenu", purgeable) 
	"::library:optMenu.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+9, "palette", purgeable) 
	"::library:palette.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+10, "scale", purgeable) 
	"::library:scale.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+11, "scrlbar", purgeable) 
	"::library:scrlbar.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+12, "tearoff", purgeable) 
	"::library:tearoff.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+13, "text", purgeable) 
	"::library:text.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+14, "bgerror", purgeable) 
	"::library:bgerror.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+15, "console", purgeable) 
	"::library:console.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+16, "msgbox", purgeable) 
	"::library:msgbox.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+17, "comdlg", purgeable) 
	"::library:comdlg.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+18, "spinbox", purgeable) 
	"::library:spinbox.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+19, "panedwindow", purgeable) 
	"::library:panedwindow.tcl";
read 'TEXT' (TK_LIBRARY_RESOURCES+20, "msgcat", purgeable) 
	":::tcl:library:msgcat:msgcat.tcl";
