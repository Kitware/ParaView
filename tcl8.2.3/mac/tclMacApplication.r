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
	"Tcl Shell " TCL_PATCH_LEVEL " © 1996"
};

#define TCL_APP_CREATOR 'Tcl '

type TCL_APP_CREATOR as 'STR ';
resource TCL_APP_CREATOR (0, purgeable) {
	"Tcl Shell " TCL_PATCH_LEVEL " © 1996"
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
