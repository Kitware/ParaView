/* 
 * tclInitScript.h --
 *
 *	This file contains Unix & Windows common init script
 *      It is not used on the Mac. (the mac init script is in tclMacInit.c)
 *
 * Copyright (c) 1998 Sun Microsystems, Inc.
 * Copyright (c) 1999 by Scriptics Corporation.
 * All rights reserved.
 *
 * RCS: @(#) Id
 */

/*
 * In order to find init.tcl during initialization, the following script
 * is invoked by Tcl_Init().  It looks in several different directories:
 *
 *	$tcl_library		- can specify a primary location, if set
 *				  no other locations will be checked
 *
 *	$env(TCL_LIBRARY)	- highest priority so user can always override
 *				  the search path unless the application has
 *				  specified an exact directory above
 *
 *	$tclDefaultLibrary	- this value is initialized by TclPlatformInit
 *				  from a static C variable that was set at
 *				  compile time
 *
 *	$tcl_libPath		- this value is initialized by a call to
 *				  TclGetLibraryPath called from Tcl_Init.
 *
 * The first directory on this path that contains a valid init.tcl script
 * will be set as the value of tcl_library.
 *
 * Note that this entire search mechanism can be bypassed by defining an
 * alternate tclInit procedure before calling Tcl_Init().
 */

static char initScript[] = "if {[info proc tclInit]==\"\"} {\n\
  proc tclInit {} {\n\
    global tcl_libPath tcl_library errorInfo\n\
    global env tclDefaultLibrary\n\
    rename tclInit {}\n\
    set errors {}\n\
    set dirs {}\n\
    if {[info exists tcl_library]} {\n\
	lappend dirs $tcl_library\n\
    } else {\n\
	if {[info exists env(TCL_LIBRARY)]} {\n\
	    lappend dirs $env(TCL_LIBRARY)\n\
	}\n\
	lappend dirs $tclDefaultLibrary\n\
	unset tclDefaultLibrary\n\
        set dirs [concat $dirs $tcl_libPath]\n\
    }\n\
    foreach i $dirs {\n\
	set tcl_library $i\n\
	set tclfile [file join $i init.tcl]\n\
	if {[file exists $tclfile]} {\n\
	    if {![catch {uplevel #0 [list source $tclfile]} msg]} {\n\
	        return\n\
	    } else {\n\
		append errors \"$tclfile: $msg\n$errorInfo\n\"\n\
	    }\n\
	}\n\
    }\n\
    set msg \"Can't find a usable init.tcl in the following directories: \n\"\n\
    append msg \"    $dirs\n\n\"\n\
    append msg \"$errors\n\n\"\n\
    append msg \"This probably means that Tcl wasn't installed properly.\n\"\n\
    error $msg\n\
  }\n\
}\n\
tclInit";


/*
 * A pointer to a string that holds an initialization script that if non-NULL
 * is evaluated in Tcl_Init() prior to the the built-in initialization script
 * above.  This variable can be modified by the procedure below.
 */
 
static char *          tclPreInitScript = NULL;


/*
 *----------------------------------------------------------------------
 *
 * TclSetPreInitScript --
 *
 *	This routine is used to change the value of the internal
 *	variable, tclPreInitScript.
 *
 * Results:
 *	Returns the current value of tclPreInitScript.
 *
 * Side effects:
 *	Changes the way Tcl_Init() routine behaves.
 *
 *----------------------------------------------------------------------
 */

char *
TclSetPreInitScript (string)
    char *string;		/* Pointer to a script. */
{
    char *prevString = tclPreInitScript;
    tclPreInitScript = string;
    return(prevString);
}
