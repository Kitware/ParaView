/* 
 * tclMacApplication.r --
 *
 *	This file creates resources for use Tcl Shell application.
 *	It should be viewed as an example of how to create a new
 *	Tcl application using the shared Tcl libraries.
 *
 * Copyright (c) 1996-1997 Sun Microsystems, Inc.
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

#if (TCL_RELEASE_LEVEL == 0)
#   define RELEASE_LEVEL alpha
#elif (TCL_RELEASE_LEVEL == 1)
#   define RELEASE_LEVEL beta
#elif (TCL_RELEASE_LEVEL == 2)
#   define RELEASE_LEVEL final
#endif

#if (TCL_RELEASE_LEVEL == 2)
#   define MINOR_VERSION (TCL_MINOR_VERSION * 16) + TCL_RELEASE_SERIAL
#   define RELEASE_CODE 0x00
#else
#   define MINOR_VERSION TCL_MINOR_VERSION * 16
#   define RELEASE_CODE TCL_RELEASE_SERIAL
#endif

resource 'vers' (1) {
	TCL_MAJOR_VERSION, MINOR_VERSION,
	RELEASE_LEVEL, RELEASE_CODE, verUS,
	TCL_PATCH_LEVEL,
	TCL_PATCH_LEVEL ", by Ray Johnson & Jim Ingham" "\n" "© 2001 Tcl Core Team"
};

resource 'vers' (2) {
	TCL_MAJOR_VERSION, MINOR_VERSION,
	RELEASE_LEVEL, RELEASE_CODE, verUS,
	TCL_PATCH_LEVEL,
	"Tcl Shell " TCL_PATCH_LEVEL " © 1993-2001"
};

#define TCL_APP_CREATOR 'Tcl '

type TCL_APP_CREATOR as 'STR ';
resource TCL_APP_CREATOR (0, purgeable) {
	"Tcl Shell " TCL_PATCH_LEVEL " © 1993-2001"
};

/*
 * The 'kind' resource works with a 'BNDL' in Macintosh Easy Open
 * to affect the text the Finder displays in the "kind" column and
 * file info dialog.  This information will be applied to all files
 * with the listed creator and type.
 */

resource 'kind' (128, "Tcl kind", purgeable) {
	TCL_APP_CREATOR,
	0, /* region = USA */
	{
		'APPL', "Tcl Shell",
	}
};

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
	{	
		/*		
		"SCHEDULE_NAME=Agent Controller Schedule",
		"SCHEDULE_PATH=Lozoya:System Folder:Tcl Lib:Tcl-Scheduler"
		*/
	};
};

data 'alis' (1000, "Library Folder") {
	$"0000 0000 00BA 0002 0001 012F 0000 0000"            /* .....†...../.... */
	$"0000 0000 0000 0000 0000 0000 0000 0000"            /* ................ */
	$"0000 0000 0000 985C FB00 4244 0000 0000"            /* ......ς\.BD.... */
	$"0002 1328 5375 7070 6F72 7420 4C69 6272"            /* ...(Support Libr */
	$"6172 6965 7329 0000 0000 0000 0000 0000"            /* aries).......... */
	$"0000 0000 0000 0000 0000 0000 0000 0000"            /* ................ */
	$"0000 0000 0000 0000 0000 0000 0000 0000"            /* ................ */
	$"0000 0076 8504 B617 A796 003D 0027 025B"            /* ...vΦ..ίρ.=.'.[ */
	$"01E4 0001 0001 0000 0000 0000 0000 0000"            /* .”.............. */
	$"0000 0000 0000 0000 0001 2F00 0002 0015"            /* ........../..... */
	$"2F3A 2853 7570 706F 7274 204C 6962 7261"            /* /:(Support Libra */
	$"7269 6573 2900 FFFF 0000"                           /* ries)... */
};

