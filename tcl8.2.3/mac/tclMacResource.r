/* 
 * tclMacResource.r --
 *
 *	This file creates resources for use in a simple shell.
 *	This is designed to be an example of using the Tcl libraries
 *	statically in a Macintosh Application.  For an example of
 *	of using the dynamic libraries look at tclMacApplication.r.
 *
 * Copyright (c) 1993-94 Lockheed Missle & Space Company
 * Copyright (c) 1994-97 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <Types.r>
#include <SysTypes.r>

/*
 * The folowing include and defines help construct
 * the version string for Tcl.
 */

#define RESOURCE_INCLUDED
#include "tcl.h"

#if (TCL_RELEASE_LEVEL == 0)
#   define RELEASE_LEVEL alpha
#elif (TCL_RELEASE_LEVEL == 1)
#   define RELEASE_LEVEL beta
#elif (TCL_RELEASE_LEVEL == 2)
#   define RELEASE_LEVEL final
#endif

#if (TCL_RELEASE_LEVEL == 2)
#   define MINOR_VERSION (TCL_MINOR_VERSION * 16) + TCL_RELEASE_SERIAL
#else
#   define MINOR_VERSION TCL_MINOR_VERSION * 16
#endif

resource 'vers' (1) {
	TCL_MAJOR_VERSION, MINOR_VERSION,
	RELEASE_LEVEL, 0x00, verUS,
	TCL_PATCH_LEVEL,
	TCL_PATCH_LEVEL ", by Ray Johnson © Sun Microsystems"
};

resource 'vers' (2) {
	TCL_MAJOR_VERSION, MINOR_VERSION,
	RELEASE_LEVEL, 0x00, verUS,
	TCL_PATCH_LEVEL,
	"Simple Tcl Shell " TCL_PATCH_LEVEL " © 1996"
};


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

#include "tclMacTclCode.r"

/*
 * The following resource is used when creating the 'env' variable in
 * the Macintosh environment.  The creation mechanisim looks for the
 * 'STR#' resource named "Tcl Environment Variables" rather than a
 * specific resource number.  (In other words, feel free to change the
 * resource id if it conflicts with your application.)  Each string in
 * the resource must be of the form "KEYWORD=SOME STRING".  See Tcl
 * documentation for futher information about the env variable.
 *
 * A good example of something you may want to set is: "TCL_LIBRARY=My
 * disk:etc."
 */
 
resource 'STR#' (128, "Tcl Environment Variables") {
	{	"SCHEDULE_NAME=Agent Controller Schedule",
		"SCHEDULE_PATH=Lozoya:System Folder:Tcl Lib:Tcl-Scheduler"
	};
};

