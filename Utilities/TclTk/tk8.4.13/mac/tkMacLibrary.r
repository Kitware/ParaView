/*
 * tkMacLibrary.r --
 *
 *	This file creates resources for use in most Tk applications.
 *	This is designed to be an example of using the Tcl/Tk 
 *	libraries in a Macintosh Application.
 *
 * Copyright (c) 1996 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#include <Types.r>
#include <SysTypes.r>
#include <AEUserTermTypes.r>

/*
 * The folowing include and defines help construct
 * the version string for Tcl.
 */

#define RC_INVOKED
#include <tcl.h>
#include "tk.h"

#if (TK_RELEASE_LEVEL == 0)
#   define RELEASE_LEVEL alpha
#elif (TK_RELEASE_LEVEL == 1)
#   define RELEASE_LEVEL beta
#elif (TK_RELEASE_LEVEL == 2)
#   define RELEASE_LEVEL final
#endif

#if (TK_RELEASE_LEVEL == 2)
#   define MINOR_VERSION (TK_MINOR_VERSION * 16) + TK_RELEASE_SERIAL
#   define RELEASE_CODE 0x00
#else
#   define MINOR_VERSION TK_MINOR_VERSION * 16
#   define RELEASE_CODE TK_RELEASE_SERIAL
#endif

#define RELEASE_CODE 0x00

resource 'vers' (1) {
	TK_MAJOR_VERSION, MINOR_VERSION,
	RELEASE_LEVEL, RELEASE_CODE, verUS,
	TK_PATCH_LEVEL,
	TK_PATCH_LEVEL ", by Ray Johnson & Jim Ingham" "\n" "© 2001 Tcl Core Team"
};

resource 'vers' (2) {
	TK_MAJOR_VERSION, MINOR_VERSION,
	RELEASE_LEVEL, RELEASE_CODE, verUS,
	TK_PATCH_LEVEL,
	"Tk Library " TK_PATCH_LEVEL " © 1993-2001"
};

/*
 * The -16397 string will be displayed by Finder when a user
 * tries to open the shared library. The string should
 * give the user a little detail about the library's capabilities
 * and enough information to install the library in the correct location.  
 * A similar string should be placed in all shared libraries.
 */
resource 'STR ' (-16397, purgeable) {
	"Tk Library\n\n"
	"This is the library needed to run Tcl/Tk programs. "
	"To work properly, it should be placed in the Tool Command Language folder "
	"within the Extensions folder."
};
