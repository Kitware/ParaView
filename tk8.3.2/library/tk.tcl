# tk.tcl --
#
# Initialization script normally executed in the interpreter for each
# Tk-based application.  Arranges class bindings for widgets.
#
# RCS: @(#) Id
#
# Copyright (c) 1992-1994 The Regents of the University of California.
# Copyright (c) 1994-1996 Sun Microsystems, Inc.
# Copyright (c) 1998-2000 Scriptics Corporation.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.

# Insist on running with compatible versions of Tcl and Tk.

package require -exact Tk 8.3
package require -exact Tcl 8.3

# Add Tk's directory to the end of the auto-load search path, if it
# isn't already on the path:

if {[info exists auto_path] && [string compare {} $tk_library] && \
	[lsearch -exact $auto_path $tk_library] < 0} {
    lappend auto_path $tk_library
}

# Turn off strict Motif look and feel as a default.

set tk_strictMotif 0

# Create a ::tk namespace

namespace eval ::tk {
}

# ::tk::PlaceWindow --
#   place a toplevel at a particular position
# Arguments:
#   toplevel	name of toplevel window
#   ?placement?	pointer ?center? ; places $w centered on the pointer
#		widget widgetPath ; centers $w over widget_name
#		defaults to placing toplevel in the middle of the screen
#   ?anchor?	center or widgetPath
# Results:
#   Returns nothing
#
proc ::tk::PlaceWindow {w {place ""} {anchor ""}} {
    wm withdraw $w
    update idletasks
    set checkBounds 1
    if {[string equal -len [string length $place] $place "pointer"]} {
	## place at POINTER (centered if $anchor == center)
	if {[string equal -len [string length $anchor] $anchor "center"]} {
	    set x [expr {[winfo pointerx $w]-[winfo reqwidth $w]/2}]
	    set y [expr {[winfo pointery $w]-[winfo reqheight $w]/2}]
	} else {
	    set x [winfo pointerx $w]
	    set y [winfo pointery $w]
	}
    } elseif {[string equal -len [string length $place] $place "widget"] && \
	    [winfo exists $anchor] && [winfo ismapped $anchor]} {
	## center about WIDGET $anchor, widget must be mapped
	set x [expr {[winfo rootx $anchor] + \
		([winfo width $anchor]-[winfo reqwidth $w])/2}]
	set y [expr {[winfo rooty $anchor] + \
		([winfo height $anchor]-[winfo reqheight $w])/2}]
    } else {
	set x [expr {([winfo screenwidth $w]-[winfo reqwidth $w])/2}]
	set y [expr {([winfo screenheight $w]-[winfo reqheight $w])/2}]
	set checkBounds 0
    }
    if {$checkBounds} {
	if {$x < 0} {
	    set x 0
	} elseif {$x > ([winfo screenwidth $w]-[winfo reqwidth $w])} {
	    set x [expr {[winfo screenwidth $w]-[winfo reqwidth $w]}]
	}
	if {$y < 0} {
	    set y 0
	} elseif {$y > ([winfo screenheight $w]-[winfo reqheight $w])} {
	    set y [expr {[winfo screenheight $w]-[winfo reqheight $w]}]
	}
    }
    wm geometry $w +$x+$y
    wm deiconify $w
}

# ::tk::SetFocusGrab --
#   swap out current focus and grab temporarily (for dialogs)
# Arguments:
#   grab	new window to grab
#   focus	window to give focus to
# Results:
#   Returns nothing
#
proc ::tk::SetFocusGrab {grab {focus {}}} {
    set index "$grab,$focus"
    upvar ::tk::FocusGrab($index) data

    lappend data [focus]
    set oldGrab [grab current $grab]
    lappend data $oldGrab
    if {[winfo exists $oldGrab]} {
	lappend data [grab status $oldGrab]
    }
    grab $grab
    if {[winfo exists $focus]} {
	focus $focus
    }
}

# ::tk::RestoreFocusGrab --
#   restore old focus and grab (for dialogs)
# Arguments:
#   grab	window that had taken grab
#   focus	window that had taken focus
#   destroy	destroy|withdraw - how to handle the old grabbed window
# Results:
#   Returns nothing
#
proc ::tk::RestoreFocusGrab {grab focus {destroy destroy}} {
    set index "$grab,$focus"
    foreach {oldFocus oldGrab oldStatus} $::tk::FocusGrab($index) { break }
    unset ::tk::FocusGrab($index)

    catch {focus $oldFocus}
    grab release $grab
    if {[string equal $destroy "withdraw"]} {
	wm withdraw $grab
    } else {
	destroy $grab
    }
    if {[winfo exists $oldGrab] && [winfo ismapped $oldGrab]} {
	if {[string equal $oldStatus "global"]} {
	    grab -global $oldGrab
	} else {
	    grab $oldGrab
	}
    }
}

# tkScreenChanged --
# This procedure is invoked by the binding mechanism whenever the
# "current" screen is changing.  The procedure does two things.
# First, it uses "upvar" to make global variable "tkPriv" point at an
# array variable that holds state for the current display.  Second,
# it initializes the array if it didn't already exist.
#
# Arguments:
# screen -		The name of the new screen.

proc tkScreenChanged screen {
    set x [string last . $screen]
    if {$x > 0} {
	set disp [string range $screen 0 [expr {$x - 1}]]
    } else {
	set disp $screen
    }

    uplevel #0 upvar #0 tkPriv.$disp tkPriv
    global tkPriv
    global tcl_platform

    if {[info exists tkPriv]} {
	set tkPriv(screen) $screen
	return
    }
    array set tkPriv {
	activeMenu	{}
	activeItem	{}
	afterId		{}
	buttons		0
	buttonWindow	{}
	dragging	0
	focus		{}
	grab		{}
	initPos		{}
	inMenubutton	{}
	listboxPrev	{}
	menuBar		{}
	mouseMoved	0
	oldGrab		{}
	popup		{}
	postedMb	{}
	pressX		0
	pressY		0
	prevPos		0
	selectMode	char
    }
    set tkPriv(screen) $screen
    set tkPriv(tearoff) [string equal $tcl_platform(platform) "unix"]
    set tkPriv(window) {}
}

# Do initial setup for tkPriv, so that it is always bound to something
# (otherwise, if someone references it, it may get set to a non-upvar-ed
# value, which will cause trouble later).

tkScreenChanged [winfo screen .]

# tkEventMotifBindings --
# This procedure is invoked as a trace whenever tk_strictMotif is
# changed.  It is used to turn on or turn off the motif virtual
# bindings.
#
# Arguments:
# n1 - the name of the variable being changed ("tk_strictMotif").

proc tkEventMotifBindings {n1 dummy dummy} {
    upvar $n1 name
    
    if {$name} {
	set op delete
    } else {
	set op add
    }

    event $op <<Cut>> <Control-Key-w>
    event $op <<Copy>> <Meta-Key-w> 
    event $op <<Paste>> <Control-Key-y>
}

#----------------------------------------------------------------------
# Define common dialogs on platforms where they are not implemented 
# using compiled code.
#----------------------------------------------------------------------

if {[string equal [info commands tk_chooseColor] ""]} {
    proc tk_chooseColor {args} {
	return [eval tkColorDialog $args]
    }
}
if {[string equal [info commands tk_getOpenFile] ""]} {
    proc tk_getOpenFile {args} {
	if {$::tk_strictMotif} {
	    return [eval tkMotifFDialog open $args]
	} else {
	    return [eval ::tk::dialog::file::tkFDialog open $args]
	}
    }
}
if {[string equal [info commands tk_getSaveFile] ""]} {
    proc tk_getSaveFile {args} {
	if {$::tk_strictMotif} {
	    return [eval tkMotifFDialog save $args]
	} else {
	    return [eval ::tk::dialog::file::tkFDialog save $args]
	}
    }
}
if {[string equal [info commands tk_messageBox] ""]} {
    proc tk_messageBox {args} {
	return [eval tkMessageBox $args]
    }
}
if {[string equal [info command tk_chooseDirectory] ""]} {
    proc tk_chooseDirectory {args} {
	return [eval ::tk::dialog::file::chooseDir::tkChooseDirectory $args]
    }
}
	
#----------------------------------------------------------------------
# Define the set of common virtual events.
#----------------------------------------------------------------------

switch $tcl_platform(platform) {
    "unix" {
	event add <<Cut>> <Control-Key-x> <Key-F20> 
	event add <<Copy>> <Control-Key-c> <Key-F16>
	event add <<Paste>> <Control-Key-v> <Key-F18>
	event add <<PasteSelection>> <ButtonRelease-2>
	# Some OS's define a goofy (as in, not <Shift-Tab>) keysym
	# that is returned when the user presses <Shift-Tab>.  In order for
	# tab traversal to work, we have to add these keysyms to the 
	# PrevWindow event.
	# The info exists is necessary, because tcl_platform(os) doesn't
	# exist in safe interpreters.
	if {[info exists tcl_platform(os)]} {
	    switch $tcl_platform(os) {
		"IRIX"  -
		"Linux" { event add <<PrevWindow>> <ISO_Left_Tab> }
		"HP-UX" { event add <<PrevWindow>> <hpBackTab> }
	    }
	}
	trace variable tk_strictMotif w tkEventMotifBindings
	set tk_strictMotif $tk_strictMotif
    }
    "windows" {
	event add <<Cut>> <Control-Key-x> <Shift-Key-Delete>
	event add <<Copy>> <Control-Key-c> <Control-Key-Insert>
	event add <<Paste>> <Control-Key-v> <Shift-Key-Insert>
	event add <<PasteSelection>> <ButtonRelease-2>
    }
    "macintosh" {
	event add <<Cut>> <Control-Key-x> <Key-F2> 
	event add <<Copy>> <Control-Key-c> <Key-F3>
	event add <<Paste>> <Control-Key-v> <Key-F4>
	event add <<PasteSelection>> <ButtonRelease-2>
	event add <<Clear>> <Clear>
    }
}

# ----------------------------------------------------------------------
# Read in files that define all of the class bindings.
# ----------------------------------------------------------------------

if {[string compare $tcl_platform(platform) "macintosh"] && \
	[string compare {} $tk_library]} {
    source [file join $tk_library button.tcl]
    source [file join $tk_library entry.tcl]
    source [file join $tk_library listbox.tcl]
    source [file join $tk_library menu.tcl]
    source [file join $tk_library scale.tcl]
    source [file join $tk_library scrlbar.tcl]
    source [file join $tk_library text.tcl]
}

# ----------------------------------------------------------------------
# Default bindings for keyboard traversal.
# ----------------------------------------------------------------------

event add <<PrevWindow>> <Shift-Tab>
bind all <Tab> {tkTabToWindow [tk_focusNext %W]}
bind all <<PrevWindow>> {tkTabToWindow [tk_focusPrev %W]}

# tkCancelRepeat --
# This procedure is invoked to cancel an auto-repeat action described
# by tkPriv(afterId).  It's used by several widgets to auto-scroll
# the widget when the mouse is dragged out of the widget with a
# button pressed.
#
# Arguments:
# None.

proc tkCancelRepeat {} {
    global tkPriv
    after cancel $tkPriv(afterId)
    set tkPriv(afterId) {}
}

# tkTabToWindow --
# This procedure moves the focus to the given widget.  If the widget
# is an entry, it selects the entire contents of the widget.
#
# Arguments:
# w - Window to which focus should be set.

proc tkTabToWindow {w} {
    if {[string equal [winfo class $w] Entry]} {
	$w selection range 0 end
	$w icursor end
    }
    focus $w
}
