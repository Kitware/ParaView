# tclets.tcl --
#
# Drag & Drop Tclets
# by Ray Johnson
#
# A simple way to create Tcl applications.  This applications will copy a droped Tcl file
# into a copy of a stub application (the user can pick).  The file is placed into the
# TEXT resource named "tclshrc" which is automatically executed on startup.
#
# RCS: @(#) Id
#
# Copyright (c) 1997 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

# tkOpenDocument --
#
#	This procedure is a called whenever Wish recieves an "Open" event.  The
#	procedure must be named tkOpenDocument for this to work.  Passed in files
#	are assumed to be Tcl files that the user wants to be made into Tclets.
#	(Only the first one is used.)  The procedure then creates a copy of the
#	stub app and places the Tcl file in the new application's resource fork.
#
# Parameters:
#	args		List of files
#
# Results:
# 	One success a new Tclet is created.

proc tkOpenDocument {args} {
    global droped_to_start
    
    # We only deal with the one file droped on the App
    set tclFile [lindex $args 0]
    set stub [GetStub]
    
    # Give a helper screen to guide user
    toplevel .helper -menu .bar
    unsupported1 style .helper dBoxProc
    message .helper.m -aspect 300 -text \
	"Select the name & location of your target Tcl application."
    pack .helper.m
    wm geometry .helper +20+40
    update idletasks
    
    # Get the target file from the end user
    set target [tk_getSaveFile]
    destroy .helper
    if {$target == ""} return
    
    # Copy stub, copy the droped file into the stubs text resource
    file copy $stub $target
    set id [open $tclFile r]
    set rid [resource open $target w]
    resource write -name tclshrc -file $rid TEXT [read $id]
    resource close $rid
    close $id
    
    # This is a hint to the start-up code - always set to true
    set droped_to_start true
}

# GetStub --
#
#	Get the location of our stub application.  The value may be cached,
#	in the preferences file, or we may need to ask the user.
#
# Parameters:
#	None.
#
# Results:
# 	A path to the stub application.

proc GetStub {} {
    global env stub_location
    
    if {[info exists stub_location]} {
	return $stub_location
    }
    
    set file $env(PREF_FOLDER)
    append file "D&D Tclet Preferences"
    
    
    if {[file exists $file]} {
	uplevel #0 [list source $file]
	if {[info exists stub_location] && [file exists $stub_location]} {
	    return $stub_location
	}
    }

    SelectStub

    if {[info exists stub_location]} {
	return $stub_location
    } else {
	exit
    }
}

# SelectStub --
#
#	This procedure uses tk_getOpenFile to allow the user to select
#	the copy of "Wish" that is used as the basis for Tclets.  The
#	result is stored in a preferences file.
#
# Parameters:
#	None.
#
# Results:
# 	None.  The prefernce file is updated.

proc SelectStub {} {
    global env stub_location

    # Give a helper screen to guide user
    toplevel .helper -menu .bar
    unsupported1 style .helper dBoxProc
    message .helper.m -aspect 300 -text \
        "Select \"Wish\" stub to clone.  A copy of this application will be made to create your Tclet." \
	
    pack .helper.m
    wm geometry .helper +20+40
    update idletasks

    set new_location [tk_getOpenFile]
    destroy .helper
    if {$new_location != ""} {
	set stub_location $new_location
	set file [file join $env(PREF_FOLDER) "D&D Tclet Preferences"]
    
	set id [open $file w]
	puts $id [list set stub_location $stub_location]
	close $id
    }
}

# CreateMenus --
#
#	Create the menubar for this application.
#
# Parameters:
#	None.
#
# Results:
# 	None.

proc CreateMenus {} {
    menu .bar
    .bar add cascade -menu .bar.file -label File
    .bar add cascade -menu .bar.apple
    . configure -menu .bar
    
    menu .bar.apple -tearoff 0
    .bar.apple add command -label "About Drag & Drop Tclets..." -command {ShowAbout}

    menu .bar.file -tearoff 0
    .bar.file add command -label "Show Console..." -command {console show}
    .bar.file add command -label "Select Wish Stub..." -command {SelectStub}
    .bar.file add separator
    .bar.file add command -label "Quit" -accel Command-Q -command exit
}

# ShowAbout --
#
#	Show the about box for Drag & Drop Tclets.
#
# Parameters:
#	None.
#
# Results:
# 	None.

proc ShowAbout {} {
    tk_messageBox -icon info -type ok -message \
"Drag & Drop Tclets
by Ray Johnson\n\n\
Copyright (c) 1997 Sun Microsystems, Inc."
}

# Start --
#
#	This procedure provides the main start-up code for the application.
#	It should be run first thing on start up.  It will create the UI
#	and set up the rest of the state of the application.
#
# Parameters:
#	None.
#
# Results:
# 	None.

proc Start {} {
    global droped_to_start

    # Hide . & console - see if we ran as a droped item
    wm geometry . 1x1-25000-25000
    console hide

    # Run update - if we get any drop events we know that we were
    # started by a drag & drop - if so, we quit automatically when done
    set droped_to_start false
    update
    if {$droped_to_start == "true"} {
	exit
    }
    
    # We were not started by a drag & drop - create the UI
    CreateMenus
}

# Now that everything is defined, lets start the app!
Start
