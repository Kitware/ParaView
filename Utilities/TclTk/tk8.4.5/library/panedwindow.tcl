# panedwindow.tcl --
#
# This file defines the default bindings for Tk panedwindow widgets and
# provides procedures that help in implementing those bindings.
#
# RCS: @(#) Id
#

bind Panedwindow <Button-1> { ::tk::panedwindow::MarkSash %W %x %y 1 }
bind Panedwindow <Button-2> { ::tk::panedwindow::MarkSash %W %x %y 0 }

bind Panedwindow <B1-Motion> { ::tk::panedwindow::DragSash %W %x %y 1 }
bind Panedwindow <B2-Motion> { ::tk::panedwindow::DragSash %W %x %y 0 }

bind Panedwindow <ButtonRelease-1> {::tk::panedwindow::ReleaseSash %W 1}
bind Panedwindow <ButtonRelease-2> {::tk::panedwindow::ReleaseSash %W 0}

bind Panedwindow <Motion> { ::tk::panedwindow::Motion %W %x %y }

bind Panedwindow <Leave> { ::tk::panedwindow::Leave %W }

# Initialize namespace
namespace eval ::tk::panedwindow {}

# ::tk::panedwindow::MarkSash --
#
#   Handle marking the correct sash for possible dragging
#
# Arguments:
#   w		the widget
#   x		widget local x coord
#   y		widget local y coord
#   proxy	whether this should be a proxy sash
# Results:
#   None
#
proc ::tk::panedwindow::MarkSash {w x y proxy} {
    set what [$w identify $x $y]
    if { [llength $what] == 2 } {
	foreach {index which} $what break
	if { !$::tk_strictMotif || [string equal $which "handle"] } {
	    if {!$proxy} { $w sash mark $index $x $y }
	    set ::tk::Priv(sash) $index
	    foreach {sx sy} [$w sash coord $index] break
	    set ::tk::Priv(dx) [expr {$sx-$x}]
	    set ::tk::Priv(dy) [expr {$sy-$y}]
	}
    }
}

# ::tk::panedwindow::DragSash --
#
#   Handle dragging of the correct sash
#
# Arguments:
#   w		the widget
#   x		widget local x coord
#   y		widget local y coord
#   proxy	whether this should be a proxy sash
# Results:
#   Moves sash
#
proc ::tk::panedwindow::DragSash {w x y proxy} {
    if { [info exists ::tk::Priv(sash)] } {
	if {$proxy} {
	    $w proxy place \
		    [expr {$x+$::tk::Priv(dx)}] [expr {$y+$::tk::Priv(dy)}]
	} else {
	    $w sash place $::tk::Priv(sash) \
		    [expr {$x+$::tk::Priv(dx)}] [expr {$y+$::tk::Priv(dy)}]
	}
    }
}

# ::tk::panedwindow::ReleaseSash --
#
#   Handle releasing of the sash
#
# Arguments:
#   w		the widget
#   proxy	whether this should be a proxy sash
# Results:
#   Returns ...
#
proc ::tk::panedwindow::ReleaseSash {w proxy} {
    if { [info exists ::tk::Priv(sash)] } {
	if {$proxy} {
	    foreach {x y} [$w proxy coord] break
	    $w sash place $::tk::Priv(sash) $x $y
	    $w proxy forget
	}
	unset ::tk::Priv(sash) ::tk::Priv(dx) ::tk::Priv(dy)
    }
}

# ::tk::panedwindow::Motion --
#
#   Handle motion on the widget.  This is used to change the cursor
#   when the user moves over the sash area.
#
# Arguments:
#   w		the widget
#   x		widget local x coord
#   y		widget local y coord
# Results:
#   May change the cursor.  Sets up a timer to verify that we are still
#   over the widget.
#
proc ::tk::panedwindow::Motion {w x y} {
    variable ::tk::Priv
    set id [$w identify $x $y]
    if {([llength $id] == 2) && \
	    (!$::tk_strictMotif || [string equal [lindex $id 1] "handle"])} {
	if { ![info exists Priv($w,panecursor)] } {
	    set Priv($w,panecursor) [$w cget -cursor]
	    if { [string equal [$w cget -sashcursor] ""] } {
		if { [string equal [$w cget -orient] "horizontal"] } {
		    $w configure -cursor sb_h_double_arrow
		} else {
		    $w configure -cursor sb_v_double_arrow
		}
	    } else {
		$w configure -cursor [$w cget -sashcursor]
	    }
	    if {[info exists Priv($w,pwAfterId)]} {
		after cancel $Priv($w,pwAfterId)
	    }
	    set Priv($w,pwAfterId) [after 150 \
		    [list ::tk::panedwindow::Cursor $w]]
	}
	return
    }
    if { [info exists Priv($w,panecursor)] } {
	$w configure -cursor $Priv($w,panecursor)
	unset Priv($w,panecursor)
    }
}

# ::tk::panedwindow::Cursor --
#
#   Handles returning the normal cursor when we are no longer over the
#   sash area.  This needs to be done this way, because the panedwindow
#   won't see Leave events when the mouse moves from the sash to a
#   paned child, although the child does receive an Enter event.
#
# Arguments:
#   w		the widget
# Results:
#   May restore the default cursor, or schedule a timer to do it.
#
proc ::tk::panedwindow::Cursor {w} {
    variable ::tk::Priv
    if {[info exists Priv($w,panecursor)]} {
	if {[winfo containing [winfo pointerx $w] [winfo pointery $w]] eq $w} {
	    set Priv($w,pwAfterId) [after 150 \
		    [list ::tk::panedwindow::Cursor $w]]
	} else {
	    $w configure -cursor $Priv($w,panecursor)
	    unset Priv($w,panecursor)
	    if {[info exists Priv($w,pwAfterId)]} {
		after cancel $Priv($w,pwAfterId)
		unset Priv($w,pwAfterId)
	    }
	}
    }
}

# ::tk::panedwindow::Leave --
#
#   Return to default cursor when leaving the pw widget.
#
# Arguments:
#   w		the widget
# Results:
#   Restores the default cursor
#
proc ::tk::panedwindow::Leave {w} {
    if {[info exists ::tk::Priv($w,panecursor)]} {
        $w configure -cursor $::tk::Priv($w,panecursor)
        unset ::tk::Priv($w,panecursor)
    }
}
