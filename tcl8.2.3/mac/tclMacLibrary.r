/* 
 * tclMacLibrary.r --
 *
 *	This file creates resources used by the Tcl shared library.
 *	Many thanks go to "Jay Lieske, Jr." <lieske@princeton.edu> who
 *	wrote the initial version of this file.
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
	"Tcl Library " TCL_PATCH_LEVEL " © 1996"
};

/*
 * Currently the creator for all Tcl/Tk libraries and extensions
 * should be 'TclL'.  This will allow those extension and libraries
 * to use the common icon for Tcl extensions.  However, this signature
 * still needs to be approved by the signature police at Apple and may
 * change.
 */
#define TCL_CREATOR 'TclL'
#define TCL_LIBRARY_RESOURCES 2000

/*
 * The 'BNDL' resource is the primary link between a file's
 * creator/type and its icon.  This resource acts for all Tcl shared
 * libraries; other libraries will not need one and ought to use
 * custom icons rather than new file types for a different appearance.
 */

resource 'BNDL' (TCL_LIBRARY_RESOURCES, "Tcl bundle", purgeable) 
{
	TCL_CREATOR,
	0,
	{	/* array TypeArray: 2 elements */
		/* [1] */
		'FREF',
		{	/* array IDArray: 1 elements */
			/* [1] */
			0, TCL_LIBRARY_RESOURCES
		},
		/* [2] */
		'ICN#',
		{	/* array IDArray: 1 elements */
			/* [1] */
			0, TCL_LIBRARY_RESOURCES
		}
	}
};

resource 'FREF' (TCL_LIBRARY_RESOURCES, purgeable) 
{
	'shlb', 0, ""
};

type TCL_CREATOR as 'STR ';
resource TCL_CREATOR (0, purgeable) {
	"Tcl Library " TCL_PATCH_LEVEL " © 1996"
};

/*
 * The 'kind' resource works with a 'BNDL' in Macintosh Easy Open
 * to affect the text the Finder displays in the "kind" column and
 * file info dialog.  This information will be applied to all files
 * with the listed creator and type.
 */

resource 'kind' (TCL_LIBRARY_RESOURCES, "Tcl kind", purgeable) {
	TCL_CREATOR,
	0, /* region = USA */
	{
		'shlb', "Tcl Library"
	}
};


/*
 * The -16397 string will be displayed by Finder when a user
 * tries to open the shared library. The string should
 * give the user a little detail about the library's capabilities
 * and enough information to install the library in the correct location.  
 * A similar string should be placed in all shared libraries.
 */
resource 'STR ' (-16397, purgeable) {
	"Tcl Library\n\n"
	"This is the core library needed to run Tool Command Language programs. "
	"To work properly, it should be placed in the ‘Tool Command Language’ folder "
	"within the Extensions folder."
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
 * The following are icons for the shared library.
 */

data 'icl4' (2000, "Tcl Shared Library", purgeable) {
	$"0FFF FFFF FFFF FFFF FFFF FFFF FFFF 0000"
	$"F000 0000 0000 0000 0000 0000 000C F000"
	$"F0CC CFFF CCCC CCC6 66CC CCCC CCCC F000"
	$"F0CC CFFF FFFF FF66 F6CC CCCC CCCC F000"
	$"F0CC CFFF 2000 0D66 6CCC CCCC CCCC F000"
	$"F0CC CFFF 0202 056F 6E5C CCCC CCCC F000"
	$"F0CC CFFF 2020 C666 F66F CCCC CCCC F000"
	$"F0CC CFFF 0200 B66F 666B FCCC CCCC F000"
	$"F0FC CFFF B020 55F6 6F52 BFCC CCCC F000"
	$"FF0F 0CCC FB02 5665 66D0 2FCC CCCC F0F0"
	$"F00F 0CCC CFB0 BF55 F6CF FFCC CCCC FFCF"
	$"000F 0CCC CCFB 06C9 66CC CCCC CCCC F0CF"
	$"000F 0CCC CCCF 56C6 6CCC CCCC CCCC CCCF"
	$"000F 0CCC CCCC 6FC6 FCCC CCCC CCCC CCCF"
	$"000F 0CCC CCCC 65C5 65CC CCCC CCCC CCCF"
	$"000F 0CCC CCCC 55D6 57CC CCCC CCCC CCCF"
	$"000F 0CCC CCCC 65CF 6CCC CCCC CCCC CCCF"
	$"000F 0CCC CCCC 5AC6 6CFF CCCC CCCC CCCF"
	$"000F 0CCC CCCC 65C5 6CF0 FCCC CCCC CCCF"
	$"000F 0CCC CCCC CECF CCF0 0FCC CCCC CCCF"
	$"000F 0CCC CCCC C5C6 CCCF 20FC CCCC FCCF"
	$"F00F 0CCC CCCF FFD5 CCCC F20F CCCC FFCF"
	$"FF0F 0CCC CCCF 20CF CCCC F020 FCCC F0F0"
	$"F0F0 CCCC CCCF B2C2 FFFF 0002 0FFC F000"
	$"F00C CCCC CCCC FBC0 2000 0020 2FFC F000"
	$"F0CC CCCC CCCC CFCB 0202 0202 0FFC F000"
	$"F0CC CCCC CCCC CCCF B020 2020 2FFC F000"
	$"F0CC CCCC CCCC CCDC FBBB BBBB BFFC F000"
	$"F0CC CCCC CCCC CCCC CFFF FFFF FFFC F000"
	$"F0CC CCCC CCCC CCCC CCCC CCCC CFFC F000"
	$"FCCC CCCC CCCC CCCC CCCC CCCC CCCC F000"
	$"0FFF FFFF FFFF FFFF FFFF FFFF FFFF 0000"
};

data 'ICN#' (2000, "Tcl Shared Library", purgeable) {
	$"7FFF FFF0 8000 0008 8701 C008 87FF C008"
	$"8703 8008 8707 E008 8707 F008 870F F808"
	$"A78F EC08 D0CF C40A 906F DC0D 1035 C009"
	$"101D 8001 100D 8001 100D C001 100D C001"
	$"100D 8001 100D B001 100D A801 1005 2401"
	$"1005 1209 901D 090D D011 088A A018 F068"
	$"800C 0068 8005 0068 8001 8068 8000 FFE8"
	$"8000 7FE8 8000 0068 8000 0008 7FFF FFF0"
	$"7FFF FFF0 FFFF FFF8 FFFF FFF8 FFFF FFF8"
	$"FFFF FFF8 FFFF FFF8 FFFF FFF8 FFFF FFF8"
	$"FFFF FFF8 DFFF FFFA 9FFF FFFF 1FFF FFFF"
	$"1FFF FFFF 1FFF FFFF 1FFF FFFF 1FFF FFFF"
	$"1FFF FFFF 1FFF FFFF 1FFF FFFF 1FFF FFFF"
	$"1FFF FFFF 9FFF FFFF DFFF FFFA FFFF FFF8"
	$"FFFF FFF8 FFFF FFF8 FFFF FFF8 FFFF FFF8"
	$"FFFF FFF8 FFFF FFF8 FFFF FFF8 7FFF FFF0"
};

data 'ics#' (2000, "Tcl Shared Library", purgeable) {
	$"FFFE B582 BB82 B3C2 BFA2 43C3 4381 4381"
	$"4381 4763 4392 856E 838E 81AE 811E FFFE"
	$"FFFE FFFE FFFE FFFE FFFE FFFF 7FFF 7FFF"
	$"7FFF 7FFF 7FFF FFFE FFFE FFFE FFFE FFFE"
};

data 'ics4' (2000, "Tcl Shared Library", purgeable) {
	$"FFFF FFFF FFFF FFF0 FCFF DED5 6CCC CCF0"
	$"FCFF C0D6 ECCC CCF0 FCFF 2056 65DC CCF0"
	$"FDFE D256 6DAC CCFF FFCC DDDE 5DDC CCEF"
	$"0FCC CD67 5CCC CCCF 0FCC CC5D 6CCC CCCF"
	$"0FCC CC5D 5CCC CCCF 0FCC CCD5 5CCC CCCF"
	$"FFCC CFFD CCFF CCFF FCCC CF2D DF20 FCFC"
	$"FCCC CCFD D202 FEF0 FCCC CC0D 2020 FEF0"
	$"FCCC CCCD FBBB FEF0 FFFF FFFF FFFF FFE0"
};

