/* 
 * tkInitScript.h --
 *
 *	This file contains Unix & Windows common init script
 *      It is not used on the Mac. (the mac init script is in tkMacInit.c)
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */



/*
 * In order to find tk.tcl during initialization, the following script
 * is invoked by Tk_Init().  It looks in several different directories:
 *
 *	$tk_library		- can specify a primary location, if set
 *				  no other locations will be checked
 *
 *	$env(TK_LIBRARY)	- highest priority so user can always override
 *				  the search path unless the application has
 *				  specified an exact directory above
 *
 *	$tcl_library/../tk$tk_version
 *				- look relative to init.tcl in an installed
 *				  lib directory (e.g. /usr/local)
 *
 *	<executable directory>/../lib/tk$tk_version
 *				- look for a lib/tk<ver> in a sibling of
 *				  the bin directory (e.g. /usr/local)
 *
 *	<executable directory>/../library
 *				- look in Tk build directory
 *
 *	<executable directory>/../../tk$tk_patchLevel/library
 *				- look for Tk build directory relative
 *				  to a parallel build directory
 *
 * The first directory on this path that contains a valid tk.tcl script
 * will be set ast the value of tk_library.
 *
 * Note that this entire search mechanism can be bypassed by defining an
 * alternate tkInit procedure before calling Tk_Init().
 */

static char initScript[] = "if {[info proc tkInit]==\"\"} {\n\
  proc tkInit {} {\n\
    global tk_library tk_version tk_patchLevel\n\
      rename tkInit {}\n\
    tcl_findLibrary tk $tk_version $tk_patchLevel tk.tcl TK_LIBRARY tk_library\n\
  }\n\
}\n\
tkInit";

