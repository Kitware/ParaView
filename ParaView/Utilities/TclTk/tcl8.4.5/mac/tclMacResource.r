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

#define RC_INVOKED
#include "tcl.h"

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

#ifndef TCLTK_NO_LIBRARY_TEXT_RESOURCES
#include "tclMacTclCode.r"
#endif

