/* 
 * tclMacTclCode.r --
 *
 *	This file creates resources from the Tcl code that is
 *	usually stored in the TCL_LiBRARY
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tclMacTclCode.r 1.1 98/01/21 22:22:38
 */

#include <Types.r>
#include <SysTypes.r>

#define TCL_LIBRARY_RESOURCES 2000

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

read 'TEXT' (TCL_LIBRARY_RESOURCES, "init", purgeable) "::library:init.tcl";
read 'TEXT' (TCL_LIBRARY_RESOURCES + 1, "auto", purgeable) "::library:auto.tcl";
read 'TEXT' (TCL_LIBRARY_RESOURCES + 2, "package", purgeable,preload) "::library:package.tcl";
read 'TEXT' (TCL_LIBRARY_RESOURCES + 3, "history", purgeable) "::library:history.tcl";
read 'TEXT' (TCL_LIBRARY_RESOURCES + 4, "word", purgeable,preload) "::library:word.tcl";
read 'TEXT' (TCL_LIBRARY_RESOURCES + 5, "parray", purgeable,preload) "::library:parray.tcl";
