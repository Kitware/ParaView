# dialog.tcl --
#
# This file defines the procedure tk_dialog, which creates a dialog
# box containing a bitmap, a message, and one or more buttons.
#
# RCS: @(#) Id
#
# Copyright (c) 1992-1993 The Regents of the University of California.
# Copyright (c) 1994-1997 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

#
# tk_dialog:
#
# This procedure displays a dialog box, waits for a button in the dialog
# to be invoked, then returns the index of the selected button.  If the
# dialog somehow gets destroyed, -1 is returned.
#
# Arguments:
# w -		Window to use for dialog top-level.
# title -	Title to display in dialog's decorative frame.
# text -	Message to display in dialog.
# bitmap -	Bitmap to display in dialog (empty string means none).
# default -	Index of button that is to display the default ring
#		(-1 means none).
# args -	One or more strings to display in buttons across the
#		bottom of the dialog box.

proc tk_dialog {w title text bitmap default args} {
    global tkPriv tcl_platform

    # 1. Create the top-level window and divide it into top
    # and bottom parts.

    catch {destroy $w}
    toplevel $w -class Dialog
    wm title $w $title
    wm iconname $w Dialog
    wm protocol $w WM_DELETE_WINDOW { }

    # The following command means that the dialog won't be posted if
    # [winfo parent $w] is iconified, but it's really needed;  otherwise
    # the dialog can become obscured by other windows in the application,
    # even though its grab keeps the rest of the application from being used.

    wm transient $w [winfo toplevel [winfo parent $w]]
    if {![string compare $tcl_platform(platform) "macintosh"]} {
	unsupported1 style $w dBoxProc
    }

    frame $w.bot
    frame $w.top
    if {![string compare $tcl_platform(platform) "unix"]} {
	$w.bot configure -relief raised -bd 1
	$w.top configure -relief raised -bd 1
    }
    pack $w.bot -side bottom -fill both
    pack $w.top -side top -fill both -expand 1

    # 2. Fill the top part with bitmap and message (use the option
    # database for -wraplength and -font so that they can be
    # overridden by the caller).

    option add *Dialog.msg.wrapLength 3i widgetDefault
    if {![string compare $tcl_platform(platform) "macintosh"]} {
	option add *Dialog.msg.font system widgetDefault
    } else {
	option add *Dialog.msg.font {Times 12} widgetDefault
    }

    label $w.msg -justify left -text $text
    pack $w.msg -in $w.top -side right -expand 1 -fill both -padx 3m -pady 3m
    if {[string compare $bitmap ""]} {
      if {![string compare $tcl_platform(platform) "macintosh"] && ![string compare $bitmap "error"]} {
	    set bitmap "stop"
	}
	label $w.bitmap -bitmap $bitmap
	pack $w.bitmap -in $w.top -side left -padx 3m -pady 3m
    }

    # 3. Create a row of buttons at the bottom of the dialog.

    set i 0
    foreach but $args {
	button $w.button$i -text $but -command "set tkPriv(button) $i"
	if {$i == $default} {
	    $w.button$i configure -default active
	} else {
	    $w.button$i configure -default normal
	}
	grid $w.button$i -in $w.bot -column $i -row 0 -sticky ew -padx 10
	grid columnconfigure $w.bot $i
	# We boost the size of some Mac buttons for l&f
      if {![string compare $tcl_platform(platform) "macintosh"]} {
	    set tmp [string tolower $but]
          if {![string compare $tmp "ok"] || ![string compare $tmp "cancel"]} {
		grid columnconfigure $w.bot $i -minsize [expr 59 + 20]
	    }
	}
	incr i
    }

    # 4. Create a binding for <Return> on the dialog if there is a
    # default button.

    if {$default >= 0} {
	bind $w <Return> "
          [list $w.button$default] configure -state active -relief sunken
	    update idletasks
	    after 100
	    set tkPriv(button) $default
	"
    }

    # 5. Create a <Destroy> binding for the window that sets the
    # button variable to -1;  this is needed in case something happens
    # that destroys the window, such as its parent window being destroyed.

    bind $w <Destroy> {set tkPriv(button) -1}

    # 6. Withdraw the window, then update all the geometry information
    # so we know how big it wants to be, then center the window in the
    # display and de-iconify it.

    wm withdraw $w
    update idletasks
    set x [expr {[winfo screenwidth $w]/2 - [winfo reqwidth $w]/2 \
	    - [winfo vrootx [winfo parent $w]]}]
    set y [expr {[winfo screenheight $w]/2 - [winfo reqheight $w]/2 \
	    - [winfo vrooty [winfo parent $w]]}]
    wm geom $w +$x+$y
    wm deiconify $w

    # 7. Set a grab and claim the focus too.

    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {[string compare $oldGrab ""]} {
	set grabStatus [grab status $oldGrab]
    }
    grab $w
    if {$default >= 0} {
	focus $w.button$default
    } else {
	focus $w
    }

    # 8. Wait for the user to respond, then restore the focus and
    # return the index of the selected button.  Restore the focus
    # before deleting the window, since otherwise the window manager
    # may take the focus away so we can't redirect it.  Finally,
    # restore any grab that was in effect.

    tkwait variable tkPriv(button)
    catch {focus $oldFocus}
    catch {
	# It's possible that the window has already been destroyed,
	# hence this "catch".  Delete the Destroy handler so that
	# tkPriv(button) doesn't get reset by it.

	bind $w <Destroy> {}
	destroy $w
    }
    if {[string compare $oldGrab ""]} {
      if {[string compare $grabStatus "global"]} {
	    grab $oldGrab
      } else {
          grab -global $oldGrab
	}
    }
    return $tkPriv(button)
}
