/*
 * tkMacOSA.r --
 *
 *	This file creates resources used by the AppleScript package.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
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

#define SCRIPT_MAJOR_VERSION 1		/* Major number */
#define SCRIPT_MINOR_VERSION  1		/* Minor number */
#define SCRIPT_RELEASE_SERIAL  0	/* Really minor number! */
#define RELEASE_LEVEL final		/* alpha, beta, or final */
#define SCRIPT_VERSION "1.1"
#define SCRIPT_PATCH_LEVEL "1.1.0"
#define FINAL 1				/* Change to 1 if final version. */

#if FINAL
#   define MINOR_VERSION (SCRIPT_MINOR_VERSION * 16) + SCRIPT_RELEASE_SERIAL
#   define RELEASE_CODE 0x00
#else
#   define MINOR_VERSION SCRIPT_MINOR_VERSION * 16
#   define RELEASE_CODE SCRIPT_RELEASE_SERIAL
#endif

#define RELEASE_CODE 0x00

resource 'vers' (1) {
	SCRIPT_MAJOR_VERSION, MINOR_VERSION,
	RELEASE_LEVEL, RELEASE_CODE, verUS,
	SCRIPT_PATCH_LEVEL,
	SCRIPT_PATCH_LEVEL ", by Jim Ingham © Cygnus Solutions" "\n" "© 2001 Tcl Core Team"
};

resource 'vers' (2) {
	SCRIPT_MAJOR_VERSION, MINOR_VERSION,
	RELEASE_LEVEL, RELEASE_CODE, verUS,
	SCRIPT_PATCH_LEVEL,
	"Tclapplescript " SCRIPT_PATCH_LEVEL " © 1996-2001"
};

/*
 * The -16397 string will be displayed by Finder when a user
 * tries to open the shared library. The string should
 * give the user a little detail about the library's capabilities
 * and enough information to install the library in the correct location.  
 * A similar string should be placed in all shared libraries.
 */
resource 'STR ' (-16397, purgeable) {
	"TclAppleScript Library\n\n"
	"This library provides the ability to run AppleScript "
	" commands from Tcl/Tk programs.  To work properly, it "
	"should be placed in the ŒTool Command Language¹ folder "
	"within the Extensions folder."
};


/* 
 * We now load the Tk library into the resource fork of the library.
 */

data 'TEXT' (4000,"pkgIndex",purgeable, preload) {
	"# Tcl package index file, version 1.0\n"
	"package ifneeded Tclapplescript 1.1 [list tclPkgSetup $dir Tclapplescript 1.1 {{Tclapplescript" 
	".shlb load AppleScript}}]\n"
};
