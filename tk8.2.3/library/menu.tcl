# menu.tcl --
#
# This file defines the default bindings for Tk menus and menubuttons.
# It also implements keyboard traversal of menus and implements a few
# other utility procedures related to menus.
#
# RCS: @(#) Id
#
# Copyright (c) 1992-1994 The Regents of the University of California.
# Copyright (c) 1994-1997 Sun Microsystems, Inc.
# Copyright (c) 1998-1999 by Scriptics Corporation.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

#-------------------------------------------------------------------------
# Elements of tkPriv that are used in this file:
#
# cursor -		Saves the -cursor option for the posted menubutton.
# focus -		Saves the focus during a menu selection operation.
#			Focus gets restored here when the menu is unposted.
# grabGlobal -		Used in conjunction with tkPriv(oldGrab):  if
#			tkPriv(oldGrab) is non-empty, then tkPriv(grabGlobal)
#			contains either an empty string or "-global" to
#			indicate whether the old grab was a local one or
#			a global one.
# inMenubutton -	The name of the menubutton widget containing
#			the mouse, or an empty string if the mouse is
#			not over any menubutton.
# menuBar -		The name of the menubar that is the root
#			of the cascade hierarchy which is currently
#			posted. This is null when there is no menu currently
#			being pulled down from a menu bar.
# oldGrab -		Window that had the grab before a menu was posted.
#			Used to restore the grab state after the menu
#			is unposted.  Empty string means there was no
#			grab previously set.
# popup -		If a menu has been popped up via tk_popup, this
#			gives the name of the menu.  Otherwise this
#			value is empty.
# postedMb -		Name of the menubutton whose menu is currently
#			posted, or an empty string if nothing is posted
#			A grab is set on this widget.
# relief -		Used to save the original relief of the current
#			menubutton.
# window -		When the mouse is over a menu, this holds the
#			name of the menu;  it's cleared when the mouse
#			leaves the menu.
# tearoff -		Whether the last menu posted was a tearoff or not.
#			This is true always for unix, for tearoffs for Mac
#			and Windows.
# activeMenu -		This is the last active menu for use
#			with the <<MenuSelect>> virtual event.
# activeItem -		This is the last active menu item for
#			use with the <<MenuSelect>> virtual event.
#-------------------------------------------------------------------------

#-------------------------------------------------------------------------
# Overall note:
# This file is tricky because there are five different ways that menus
# can be used:
#
# 1. As a pulldown from a menubutton. In this style, the variable 
#    tkPriv(postedMb) identifies the posted menubutton.
# 2. As a torn-off menu copied from some other menu.  In this style
#    tkPriv(postedMb) is empty, and menu's type is "tearoff".
# 3. As an option menu, triggered from an option menubutton.  In this
#    style tkPriv(postedMb) identifies the posted menubutton.
# 4. As a popup menu.  In this style tkPriv(postedMb) is empty and
#    the top-level menu's type is "normal".
# 5. As a pulldown from a menubar. The variable tkPriv(menubar) has
#    the owning menubar, and the menu itself is of type "normal".
#
# The various binding procedures use the  state described above to
# distinguish the various cases and take different actions in each
# case.
#-------------------------------------------------------------------------

#-------------------------------------------------------------------------
# The code below creates the default class bindings for menus
# and menubuttons.
#-------------------------------------------------------------------------

bind Menubutton <FocusIn> {}
bind Menubutton <Enter> {
    tkMbEnter %W
}
bind Menubutton <Leave> {
    tkMbLeave %W
}
bind Menubutton <1> {
    if {[string compare $tkPriv(inMenubutton) ""]} {
	tkMbPost $tkPriv(inMenubutton) %X %Y
    }
}
bind Menubutton <Motion> {
    tkMbMotion %W up %X %Y
}
bind Menubutton <B1-Motion> {
    tkMbMotion %W down %X %Y
}
bind Menubutton <ButtonRelease-1> {
    tkMbButtonUp %W
}
bind Menubutton <space> {
    tkMbPost %W
    tkMenuFirstEntry [%W cget -menu]
}

# Must set focus when mouse enters a menu, in order to allow
# mixed-mode processing using both the mouse and the keyboard.
# Don't set the focus if the event comes from a grab release,
# though:  such an event can happen after as part of unposting
# a cascaded chain of menus, after the focus has already been
# restored to wherever it was before menu selection started.

bind Menu <FocusIn> {}

bind Menu <Enter> {
    set tkPriv(window) %W
    if {![string compare [%W cget -type] "tearoff"]} {
      if {[string compare "%m" "NotifyUngrab"]} {
          if {![string compare $tcl_platform(platform) "unix"]} {
		tk_menuSetFocus %W
	    }
	}
    }
    tkMenuMotion %W %x %y %s
}

bind Menu <Leave> {
    tkMenuLeave %W %X %Y %s
}
bind Menu <Motion> {
    tkMenuMotion %W %x %y %s
}
bind Menu <ButtonPress> {
    tkMenuButtonDown %W
}
bind Menu <ButtonRelease> {
   tkMenuInvoke %W 1
}
bind Menu <space> {
    tkMenuInvoke %W 0
}
bind Menu <Return> {
    tkMenuInvoke %W 0
}
bind Menu <Escape> {
    tkMenuEscape %W
}
bind Menu <Left> {
    tkMenuLeftArrow %W
}
bind Menu <Right> {
    tkMenuRightArrow %W
}
bind Menu <Up> {
    tkMenuUpArrow %W
}
bind Menu <Down> {
    tkMenuDownArrow %W
}
bind Menu <KeyPress> {
    tkTraverseWithinMenu %W %A
}

# The following bindings apply to all windows, and are used to
# implement keyboard menu traversal.

if {![string compare $tcl_platform(platform) "unix"]} {
    bind all <Alt-KeyPress> {
	tkTraverseToMenu %W %A
    }

    bind all <F10> {
	tkFirstMenu %W
    }
} else {
    bind Menubutton <Alt-KeyPress> {
	tkTraverseToMenu %W %A
    }

    bind Menubutton <F10> {
	tkFirstMenu %W
    }
}

# tkMbEnter --
# This procedure is invoked when the mouse enters a menubutton
# widget.  It activates the widget unless it is disabled.  Note:
# this procedure is only invoked when mouse button 1 is *not* down.
# The procedure tkMbB1Enter is invoked if the button is down.
#
# Arguments:
# w -			The  name of the widget.

proc tkMbEnter w {
    global tkPriv

    if {[string compare $tkPriv(inMenubutton) ""]} {
	tkMbLeave $tkPriv(inMenubutton)
    }
    set tkPriv(inMenubutton) $w
    if {[string compare [$w cget -state] "disabled"]} {
	$w configure -state active
    }
}

# tkMbLeave --
# This procedure is invoked when the mouse leaves a menubutton widget.
# It de-activates the widget, if the widget still exists.
#
# Arguments:
# w -			The  name of the widget.

proc tkMbLeave w {
    global tkPriv

    set tkPriv(inMenubutton) {}
    if {![winfo exists $w]} {
	return
    }
    if {![string compare [$w cget -state] "active"]} {
	$w configure -state normal
    }
}

# tkMbPost --
# Given a menubutton, this procedure does all the work of posting
# its associated menu and unposting any other menu that is currently
# posted.
#
# Arguments:
# w -			The name of the menubutton widget whose menu
#			is to be posted.
# x, y -		Root coordinates of cursor, used for positioning
#			option menus.  If not specified, then the center
#			of the menubutton is used for an option menu.

proc tkMbPost {w {x {}} {y {}}} {
    global tkPriv errorInfo
    global tcl_platform

    if {![string compare [$w cget -state] "disabled"] ||
      ![string compare $w $tkPriv(postedMb)]} {
	return
    }
    set menu [$w cget -menu]
    if {![string compare $menu ""]} {
	return
    }
    set tearoff [expr {![string compare $tcl_platform(platform) "unix"] \
          || ![string compare [$menu cget -type] "tearoff"]}]
    if {[string first $w $menu] != 0} {
	error "can't post $menu:  it isn't a descendant of $w (this is a new requirement in Tk versions 3.0 and later)"
    }
    set cur $tkPriv(postedMb)
    if {[string compare $cur ""]} {
	tkMenuUnpost {}
    }
    set tkPriv(cursor) [$w cget -cursor]
    set tkPriv(relief) [$w cget -relief]
    $w configure -cursor arrow
    $w configure -relief raised

    set tkPriv(postedMb) $w
    set tkPriv(focus) [focus]
    $menu activate none
    tkGenerateMenuSelect $menu

    # If this looks like an option menubutton then post the menu so
    # that the current entry is on top of the mouse.  Otherwise post
    # the menu just below the menubutton, as for a pull-down.

    update idletasks
    if {[catch {
    	 switch [$w cget -direction] {
    	    above {
    	    	set x [winfo rootx $w]
    	    	set y [expr {[winfo rooty $w] - [winfo reqheight $menu]}]
    	    	$menu post $x $y
    	    }
    	    below {
    	    	set x [winfo rootx $w]
    	    	set y [expr {[winfo rooty $w] + [winfo height $w]}]
    	    	$menu post $x $y
    	    }
    	    left {
    	    	set x [expr {[winfo rootx $w] - [winfo reqwidth $menu]}]
    	    	set y [expr {(2 * [winfo rooty $w] + [winfo height $w]) / 2}]
    	    	set entry [tkMenuFindName $menu [$w cget -text]]
    	    	if {[$w cget -indicatoron]} {
		    if {$entry == [$menu index last]} {
		    	incr y [expr {-([$menu yposition $entry] \
			    	+ [winfo reqheight $menu])/2}]
		    } else {
		    	incr y [expr {-([$menu yposition $entry] \
			        + [$menu yposition [expr {$entry+1}]])/2}]
		    }
    	    	}
    	    	$menu post $x $y
              if {[string compare $entry {}] && [string compare [$menu entrycget $entry -state] "disabled"]} {
    	    	    $menu activate $entry
		    tkGenerateMenuSelect $menu
    	    	}
    	    }
    	    right {
    	    	set x [expr {[winfo rootx $w] + [winfo width $w]}]
    	    	set y [expr {(2 * [winfo rooty $w] + [winfo height $w]) / 2}]
    	    	set entry [tkMenuFindName $menu [$w cget -text]]
    	    	if {[$w cget -indicatoron]} {
		    if {$entry == [$menu index last]} {
		    	incr y [expr {-([$menu yposition $entry] \
			    	+ [winfo reqheight $menu])/2}]
		    } else {
		    	incr y [expr {-([$menu yposition $entry] \
			        + [$menu yposition [expr {$entry+1}]])/2}]
		    }
    	    	}
    	    	$menu post $x $y
              if {[string compare $entry {}] && [string compare [$menu entrycget $entry -state] "disabled"]} {
    	    	    $menu activate $entry
		    tkGenerateMenuSelect $menu
    	    	}
    	    }
    	    default {
    	    	if {[$w cget -indicatoron]} {
                  if {![string compare $y {}]} {
			set x [expr {[winfo rootx $w] + [winfo width $w]/2}]
			set y [expr {[winfo rooty $w] + [winfo height $w]/2}]
	    	    }
	            tkPostOverPoint $menu $x $y [tkMenuFindName $menu [$w cget -text]]
		} else {
	    	    $menu post [winfo rootx $w] [expr {[winfo rooty $w]+[winfo height $w]}]
    	    	}  
    	    }
    	 }
     } msg]} {
	# Error posting menu (e.g. bogus -postcommand). Unpost it and
	# reflect the error.
	
	set savedInfo $errorInfo
	tkMenuUnpost {}
	error $msg $savedInfo

    }

    set tkPriv(tearoff) $tearoff
    if {$tearoff != 0} {
    	focus $menu
    	tkSaveGrabInfo $w
    	grab -global $w
    }
}

# tkMenuUnpost --
# This procedure unposts a given menu, plus all of its ancestors up
# to (and including) a menubutton, if any.  It also restores various
# values to what they were before the menu was posted, and releases
# a grab if there's a menubutton involved.  Special notes:
# 1. It's important to unpost all menus before releasing the grab, so
#    that any Enter-Leave events (e.g. from menu back to main
#    application) have mode NotifyGrab.
# 2. Be sure to enclose various groups of commands in "catch" so that
#    the procedure will complete even if the menubutton or the menu
#    or the grab window has been deleted.
#
# Arguments:
# menu -		Name of a menu to unpost.  Ignored if there
#			is a posted menubutton.

proc tkMenuUnpost menu {
    global tcl_platform
    global tkPriv
    set mb $tkPriv(postedMb)

    # Restore focus right away (otherwise X will take focus away when
    # the menu is unmapped and under some window managers (e.g. olvwm)
    # we'll lose the focus completely).

    catch {focus $tkPriv(focus)}
    set tkPriv(focus) ""

    # Unpost menu(s) and restore some stuff that's dependent on
    # what was posted.

    catch {
      if {[string compare $mb ""]} {
	    set menu [$mb cget -menu]
	    $menu unpost
	    set tkPriv(postedMb) {}
	    $mb configure -cursor $tkPriv(cursor)
	    $mb configure -relief $tkPriv(relief)
      } elseif {[string compare $tkPriv(popup) ""]} {
	    $tkPriv(popup) unpost
	    set tkPriv(popup) {}
      } elseif {[string compare [$menu cget -type] "menubar"]
              && [string compare [$menu cget -type] "tearoff"]} {
	    # We're in a cascaded sub-menu from a torn-off menu or popup.
	    # Unpost all the menus up to the toplevel one (but not
	    # including the top-level torn-off one) and deactivate the
	    # top-level torn off menu if there is one.

	    while 1 {
		set parent [winfo parent $menu]
              if {[string compare [winfo class $parent] "Menu"]
			|| ![winfo ismapped $parent]} {
		    break
		}
		$parent activate none
		$parent postcascade none
		tkGenerateMenuSelect $parent
		set type [$parent cget -type]
              if {![string compare $type "menubar"] ||
                  ![string compare $type "tearoff"]} {
		    break
		}
		set menu $parent
	    }
          if {[string compare [$menu cget -type] "menubar"]} {
		$menu unpost
	    }
	}
    }

    if {($tkPriv(tearoff) != 0) || [string compare $tkPriv(menuBar) ""]} {
    	# Release grab, if any, and restore the previous grab, if there
    	# was one.
      if {[string compare $menu ""]} {
	    set grab [grab current $menu]
          if {[string compare $grab ""]} {
		grab release $grab
	    }
	}
	tkRestoreOldGrab
      if {[string compare $tkPriv(menuBar) ""]} {
	    $tkPriv(menuBar) configure -cursor $tkPriv(cursor)
	    set tkPriv(menuBar) {}
	}
      if {[string compare $tcl_platform(platform) "unix"]} {
	    set tkPriv(tearoff) 0
	}
    }
}

# tkMbMotion --
# This procedure handles mouse motion events inside menubuttons, and
# also outside menubuttons when a menubutton has a grab (e.g. when a
# menu selection operation is in progress).
#
# Arguments:
# w -			The name of the menubutton widget.
# upDown - 		"down" means button 1 is pressed, "up" means
#			it isn't.
# rootx, rooty -	Coordinates of mouse, in (virtual?) root window.

proc tkMbMotion {w upDown rootx rooty} {
    global tkPriv

    if {![string compare $tkPriv(inMenubutton) $w]} {
	return
    }
    set new [winfo containing $rootx $rooty]
    if {[string compare $new $tkPriv(inMenubutton)]
          && (![string compare $new ""]
          || ![string compare [winfo toplevel $new] [winfo toplevel $w]])} {
      if {[string compare $tkPriv(inMenubutton) ""]} {
	    tkMbLeave $tkPriv(inMenubutton)
	}
      if {[string compare $new ""]
              && ![string compare [winfo class $new] "Menubutton"]
		&& ([$new cget -indicatoron] == 0)
		&& ([$w cget -indicatoron] == 0)} {
          if {![string compare $upDown "down"]} {
		tkMbPost $new $rootx $rooty
	    } else {
		tkMbEnter $new
	    }
	}
    }
}

# tkMbButtonUp --
# This procedure is invoked to handle button 1 releases for menubuttons.
# If the release happens inside the menubutton then leave its menu
# posted with element 0 activated.  Otherwise, unpost the menu.
#
# Arguments:
# w -			The name of the menubutton widget.

proc tkMbButtonUp w {
    global tkPriv
    global tcl_platform

    set menu [$w cget -menu]
    set tearoff [expr {($tcl_platform(platform) == "unix") \
	    || (($menu != {}) && ([$menu cget -type] == "tearoff"))}]
    if {($tearoff != 0) && ($tkPriv(postedMb) == $w) 
	    && ($tkPriv(inMenubutton) == $w)} {
	tkMenuFirstEntry [$tkPriv(postedMb) cget -menu]
    } else {
	tkMenuUnpost {}
    }
}

# tkMenuMotion --
# This procedure is called to handle mouse motion events for menus.
# It does two things.  First, it resets the active element in the
# menu, if the mouse is over the menu.  Second, if a mouse button
# is down, it posts and unposts cascade entries to match the mouse
# position.
#
# Arguments:
# menu -		The menu window.
# x -			The x position of the mouse.
# y -			The y position of the mouse.
# state -		Modifier state (tells whether buttons are down).

proc tkMenuMotion {menu x y state} {
    global tkPriv
    if {![string compare $menu $tkPriv(window)]} {
      if {![string compare [$menu cget -type] "menubar"]} {
	    if {[info exists tkPriv(focus)] && \
                  [string compare $menu $tkPriv(focus)]} {
		$menu activate @$x,$y
		tkGenerateMenuSelect $menu
	    }
	} else {
	    $menu activate @$x,$y
	    tkGenerateMenuSelect $menu
	}
    }
    if {($state & 0x1f00) != 0} {
	$menu postcascade active
    }
}

# tkMenuButtonDown --
# Handles button presses in menus.  There are a couple of tricky things
# here:
# 1. Change the posted cascade entry (if any) to match the mouse position.
# 2. If there is a posted menubutton, must grab to the menubutton;  this
#    overrrides the implicit grab on button press, so that the menu
#    button can track mouse motions over other menubuttons and change
#    the posted menu.
# 3. If there's no posted menubutton (e.g. because we're a torn-off menu
#    or one of its descendants) must grab to the top-level menu so that
#    we can track mouse motions across the entire menu hierarchy.
#
# Arguments:
# menu -		The menu window.

proc tkMenuButtonDown menu {
    global tkPriv
    global tcl_platform

    if {![winfo viewable $menu]} {
        return
    }
    $menu postcascade active
    if {[string compare $tkPriv(postedMb) ""]} {
	grab -global $tkPriv(postedMb)
    } else {
      while {![string compare [$menu cget -type] "normal"]
              && ![string compare [winfo class [winfo parent $menu]] "Menu"]
		&& [winfo ismapped [winfo parent $menu]]} {
	    set menu [winfo parent $menu]
	}

      if {![string compare $tkPriv(menuBar) {}]} {
	    set tkPriv(menuBar) $menu
	    set tkPriv(cursor) [$menu cget -cursor]
	    $menu configure -cursor arrow
        }

	# Don't update grab information if the grab window isn't changing.
	# Otherwise, we'll get an error when we unpost the menus and
	# restore the grab, since the old grab window will not be viewable
	# anymore.

      if {[string compare $menu [grab current $menu]]} {
	    tkSaveGrabInfo $menu
	}

	# Must re-grab even if the grab window hasn't changed, in order
	# to release the implicit grab from the button press.

      if {![string compare $tcl_platform(platform) "unix"]} {
	    grab -global $menu
	}
    }
}

# tkMenuLeave --
# This procedure is invoked to handle Leave events for a menu.  It
# deactivates everything unless the active element is a cascade element
# and the mouse is now over the submenu.
#
# Arguments:
# menu -		The menu window.
# rootx, rooty -	Root coordinates of mouse.
# state -		Modifier state.

proc tkMenuLeave {menu rootx rooty state} {
    global tkPriv
    set tkPriv(window) {}
    if {![string compare [$menu index active] "none"]} {
	return
    }
    if {![string compare [$menu type active] "cascade"]
          && ![string compare [winfo containing $rootx $rooty] \
                  [$menu entrycget active -menu]]} {
	return
    }
    $menu activate none
    tkGenerateMenuSelect $menu
}

# tkMenuInvoke --
# This procedure is invoked when button 1 is released over a menu.
# It invokes the appropriate menu action and unposts the menu if
# it came from a menubutton.
#
# Arguments:
# w -			Name of the menu widget.
# buttonRelease -	1 means this procedure is called because of
#			a button release;  0 means because of keystroke.

proc tkMenuInvoke {w buttonRelease} {
    global tkPriv

    if {$buttonRelease && ![string compare $tkPriv(window) {}]} {
	# Mouse was pressed over a menu without a menu button, then
	# dragged off the menu (possibly with a cascade posted) and
	# released.  Unpost everything and quit.

	$w postcascade none
	$w activate none
	event generate $w <<MenuSelect>>
	tkMenuUnpost $w
	return
    }
    if {![string compare [$w type active] "cascade"]} {
	$w postcascade active
	set menu [$w entrycget active -menu]
	tkMenuFirstEntry $menu
    } elseif {![string compare [$w type active] "tearoff"]} {
	tkMenuUnpost $w
	tkTearOffMenu $w
    } elseif {![string compare [$w cget -type] "menubar"]} {
	$w postcascade none
	$w activate none
	event generate $w <<MenuSelect>>
	tkMenuUnpost $w
    } else {
	tkMenuUnpost $w
	uplevel #0 [list $w invoke active]
    }
}

# tkMenuEscape --
# This procedure is invoked for the Cancel (or Escape) key.  It unposts
# the given menu and, if it is the top-level menu for a menu button,
# unposts the menu button as well.
#
# Arguments:
# menu -		Name of the menu window.

proc tkMenuEscape menu {
    set parent [winfo parent $menu]
    if {[string compare [winfo class $parent] "Menu"]} {
	tkMenuUnpost $menu
    } elseif {![string compare [$parent cget -type] "menubar"]} {
	tkMenuUnpost $menu
	tkRestoreOldGrab
    } else {
	tkMenuNextMenu $menu left
    }
}

# The following routines handle arrow keys. Arrow keys behave
# differently depending on whether the menu is a menu bar or not.

proc tkMenuUpArrow {menu} {
    if {![string compare [$menu cget -type] "menubar"]} {
	tkMenuNextMenu $menu left
    } else {
	tkMenuNextEntry $menu -1
    }
}

proc tkMenuDownArrow {menu} {
    if {![string compare [$menu cget -type] "menubar"]} {
	tkMenuNextMenu $menu right
    } else {
	tkMenuNextEntry $menu 1
    }
}

proc tkMenuLeftArrow {menu} {
    if {![string compare [$menu cget -type] "menubar"]} {
	tkMenuNextEntry $menu -1
    } else {
	tkMenuNextMenu $menu left
    }
}

proc tkMenuRightArrow {menu} {
    if {![string compare [$menu cget -type] "menubar"]} {
	tkMenuNextEntry $menu 1
    } else {
	tkMenuNextMenu $menu right
    }
}

# tkMenuNextMenu --
# This procedure is invoked to handle "left" and "right" traversal
# motions in menus.  It traverses to the next menu in a menu bar,
# or into or out of a cascaded menu.
#
# Arguments:
# menu -		The menu that received the keyboard
#			event.
# direction -		Direction in which to move: "left" or "right"

proc tkMenuNextMenu {menu direction} {
    global tkPriv

    # First handle traversals into and out of cascaded menus.

    if {![string compare $direction "right"]} {
	set count 1
	set parent [winfo parent $menu]
	set class [winfo class $parent]
      if {![string compare [$menu type active] "cascade"]} {
	    $menu postcascade active
	    set m2 [$menu entrycget active -menu]
          if {[string compare $m2 ""]} {
		tkMenuFirstEntry $m2
	    }
	    return
	} else {
	    set parent [winfo parent $menu]
          while {[string compare $parent "."]} {
              if {![string compare [winfo class $parent] "Menu"]
                      && ![string compare [$parent cget -type] "menubar"]} {
		    tk_menuSetFocus $parent
		    tkMenuNextEntry $parent 1
		    return
		}
		set parent [winfo parent $parent]
	    }
	}
    } else {
	set count -1
	set m2 [winfo parent $menu]
      if {![string compare [winfo class $m2] "Menu"]} {
          if {[string compare [$m2 cget -type] "menubar"]} {
		$menu activate none
		tkGenerateMenuSelect $menu
		tk_menuSetFocus $m2
		
		# This code unposts any posted submenu in the parent.
		
		set tmp [$m2 index active]
		$m2 activate none
		$m2 activate $tmp
		return
	    }
	}
    }

    # Can't traverse into or out of a cascaded menu.  Go to the next
    # or previous menubutton, if that makes sense.

    set m2 [winfo parent $menu]
    if {![string compare [winfo class $m2] "Menu"]} {
      if {![string compare [$m2 cget -type] "menubar"]} {
	    tk_menuSetFocus $m2
	    tkMenuNextEntry $m2 -1
	    return
	}
    }

    set w $tkPriv(postedMb)
    if {![string compare $w ""]} {
	return
    }
    set buttons [winfo children [winfo parent $w]]
    set length [llength $buttons]
    set i [expr {[lsearch -exact $buttons $w] + $count}]
    while 1 {
	while {$i < 0} {
	    incr i $length
	}
	while {$i >= $length} {
	    incr i -$length
	}
	set mb [lindex $buttons $i]
      if {![string compare [winfo class $mb] "Menubutton"]
              && [string compare [$mb cget -state] "disabled"]
              && [string compare [$mb cget -menu] ""]
              && [string compare [[$mb cget -menu] index last] "none"]} {
	    break
	}
      if {![string compare $mb $w]} {
	    return
	}
	incr i $count
    }
    tkMbPost $mb
    tkMenuFirstEntry [$mb cget -menu]
}

# tkMenuNextEntry --
# Activate the next higher or lower entry in the posted menu,
# wrapping around at the ends.  Disabled entries are skipped.
#
# Arguments:
# menu -			Menu window that received the keystroke.
# count -			1 means go to the next lower entry,
#				-1 means go to the next higher entry.

proc tkMenuNextEntry {menu count} {
    global tkPriv

    if {![string compare [$menu index last] "none"]} {
	return
    }
    set length [expr {[$menu index last]+1}]
    set quitAfter $length
    set active [$menu index active]
    if {![string compare $active "none"]} {
	set i 0
    } else {
	set i [expr {$active + $count}]
    }
    while 1 {
	if {$quitAfter <= 0} {
	    # We've tried every entry in the menu.  Either there are
	    # none, or they're all disabled.  Just give up.

	    return
	}
	while {$i < 0} {
	    incr i $length
	}
	while {$i >= $length} {
	    incr i -$length
	}
	if {[catch {$menu entrycget $i -state} state] == 0} {
	    if {$state != "disabled"} {
		break
	    }
	}
	if {$i == $active} {
	    return
	}
	incr i $count
	incr quitAfter -1
    }
    $menu activate $i
    tkGenerateMenuSelect $menu
    if {![string compare [$menu type $i] "cascade"]} {
	set cascade [$menu entrycget $i -menu]
      if {[string compare $cascade ""]} {
	    $menu postcascade $i
	    tkMenuFirstEntry $cascade
	}
    }
}

# tkMenuFind --
# This procedure searches the entire window hierarchy under w for
# a menubutton that isn't disabled and whose underlined character
# is "char" or an entry in a menubar that isn't disabled and whose
# underlined character is "char".
# It returns the name of that window, if found, or an
# empty string if no matching window was found.  If "char" is an
# empty string then the procedure returns the name of the first
# menubutton found that isn't disabled.
#
# Arguments:
# w -				Name of window where key was typed.
# char -			Underlined character to search for;
#				may be either upper or lower case, and
#				will match either upper or lower case.

proc tkMenuFind {w char} {
    global tkPriv
    set char [string tolower $char]
    set windowlist [winfo child $w]

    foreach child $windowlist {
	# Don't descend into other toplevels.
        if {[winfo toplevel [focus]] != [winfo toplevel $child] } {
	    continue
	}
	switch [winfo class $child] {
	    Menu {
              if {![string compare [$child cget -type] "menubar"]} {
                  if {![string compare $char ""]} {
			return $child
		    }
		    set last [$child index last]
		    for {set i [$child cget -tearoff]} {$i <= $last} {incr i} {
                      if {![string compare [$child type $i] "separator"]} {
			    continue
			}
			set char2 [string index [$child entrycget $i -label] \
				[$child entrycget $i -underline]]
                      if {![string compare $char [string tolower $char2]] \
                              || ![string compare $char ""]} {
                          if {[string compare [$child entrycget $i -state] "disabled"]} {
				return $child
			    }
			}
		    }
		}
	    }
	}
    }

    foreach child $windowlist {
	# Don't descend into other toplevels.
        if {[winfo toplevel [focus]] != [winfo toplevel $child] } {
	    continue
	}
	switch [winfo class $child] {
	    Menubutton {
		set char2 [string index [$child cget -text] \
			[$child cget -underline]]
              if {![string compare $char [string tolower $char2]]
                      || ![string compare $char ""]} {
                  if {[string compare [$child cget -state] "disabled"]} {
			return $child
		    }
		}
	    }

	    default {
		set match [tkMenuFind $child $char]
              if {[string compare $match ""]} {
		    return $match
		}
	    }
	}
    }
    return {}
}

# tkTraverseToMenu --
# This procedure implements keyboard traversal of menus.  Given an
# ASCII character "char", it looks for a menubutton with that character
# underlined.  If one is found, it posts the menubutton's menu
#
# Arguments:
# w -				Window in which the key was typed (selects
#				a toplevel window).
# char -			Character that selects a menu.  The case
#				is ignored.  If an empty string, nothing
#				happens.

proc tkTraverseToMenu {w char} {
    global tkPriv
    if {![string compare $char ""]} {
	return
    }
    while {![string compare [winfo class $w] "Menu"]} {
      if {[string compare [$w cget -type] "menubar"]
              && ![string compare $tkPriv(postedMb) ""]} {
	    return
	}
      if {![string compare [$w cget -type] "menubar"]} {
	    break
	}
	set w [winfo parent $w]
    }
    set w [tkMenuFind [winfo toplevel $w] $char]
    if {[string compare $w ""]} {
      if {![string compare [winfo class $w] "Menu"]} {
	    tk_menuSetFocus $w
	    set tkPriv(window) $w
	    tkSaveGrabInfo $w
	    grab -global $w
	    tkTraverseWithinMenu $w $char
	} else {
	    tkMbPost $w
	    tkMenuFirstEntry [$w cget -menu]
	}
    }
}

# tkFirstMenu --
# This procedure traverses to the first menubutton in the toplevel
# for a given window, and posts that menubutton's menu.
#
# Arguments:
# w -				Name of a window.  Selects which toplevel
#				to search for menubuttons.

proc tkFirstMenu w {
    set w [tkMenuFind [winfo toplevel $w] ""]
    if {[string compare $w ""]} {
      if {![string compare [winfo class $w] "Menu"]} {
	    tk_menuSetFocus $w
	    set tkPriv(window) $w
	    tkSaveGrabInfo $w
	    grab -global $w
	    tkMenuFirstEntry $w
	} else {
	    tkMbPost $w
	    tkMenuFirstEntry [$w cget -menu]
	}
    }
}

# tkTraverseWithinMenu
# This procedure implements keyboard traversal within a menu.  It
# searches for an entry in the menu that has "char" underlined.  If
# such an entry is found, it is invoked and the menu is unposted.
#
# Arguments:
# w -				The name of the menu widget.
# char -			The character to look for;  case is
#				ignored.  If the string is empty then
#				nothing happens.

proc tkTraverseWithinMenu {w char} {
    if {![string compare $char ""]} {
	return
    }
    set char [string tolower $char]
    set last [$w index last]
    if {![string compare $last "none"]} {
	return
    }
    for {set i 0} {$i <= $last} {incr i} {
	if {[catch {set char2 [string index \
		[$w entrycget $i -label] \
		[$w entrycget $i -underline]]}]} {
	    continue
	}
      if {![string compare $char [string tolower $char2]]} {
          if {![string compare [$w type $i] "cascade"]} {
		$w activate $i
		$w postcascade active
		event generate $w <<MenuSelect>>
		set m2 [$w entrycget $i -menu]
              if {[string compare $m2 ""]} {
		    tkMenuFirstEntry $m2
		}
	    } else {
		tkMenuUnpost $w
		uplevel #0 [list $w invoke $i]
	    }
	    return
	}
    }
}

# tkMenuFirstEntry --
# Given a menu, this procedure finds the first entry that isn't
# disabled or a tear-off or separator, and activates that entry.
# However, if there is already an active entry in the menu (e.g.,
# because of a previous call to tkPostOverPoint) then the active
# entry isn't changed.  This procedure also sets the input focus
# to the menu.
#
# Arguments:
# menu -		Name of the menu window (possibly empty).

proc tkMenuFirstEntry menu {
    if {![string compare $menu ""]} {
	return
    }
    tk_menuSetFocus $menu
    if {[string compare [$menu index active] "none"]} {
	return
    }
    set last [$menu index last]
    if {![string compare $last "none"]} {
	return
    }
    for {set i 0} {$i <= $last} {incr i} {
	if {([catch {set state [$menu entrycget $i -state]}] == 0)
              && [string compare $state "disabled"]
              && [string compare [$menu type $i] "tearoff"]} {
	    $menu activate $i
	    tkGenerateMenuSelect $menu
          if {![string compare [$menu type $i] "cascade"]} {
		set cascade [$menu entrycget $i -menu]
              if {[string compare $cascade ""]} {
		    $menu postcascade $i
		    tkMenuFirstEntry $cascade
		}
	    }
	    return
	}
    }
}

# tkMenuFindName --
# Given a menu and a text string, return the index of the menu entry
# that displays the string as its label.  If there is no such entry,
# return an empty string.  This procedure is tricky because some names
# like "active" have a special meaning in menu commands, so we can't
# always use the "index" widget command.
#
# Arguments:
# menu -		Name of the menu widget.
# s -			String to look for.

proc tkMenuFindName {menu s} {
    set i ""
    if {![regexp {^active$|^last$|^none$|^[0-9]|^@} $s]} {
	catch {set i [$menu index $s]}
	return $i
    }
    set last [$menu index last]
    if {![string compare $last "none"]} {
	return
    }
    for {set i 0} {$i <= $last} {incr i} {
	if {![catch {$menu entrycget $i -label} label]} {
          if {![string compare $label $s]} {
		return $i
	    }
	}
    }
    return ""
}

# tkPostOverPoint --
# This procedure posts a given menu such that a given entry in the
# menu is centered over a given point in the root window.  It also
# activates the given entry.
#
# Arguments:
# menu -		Menu to post.
# x, y -		Root coordinates of point.
# entry -		Index of entry within menu to center over (x,y).
#			If omitted or specified as {}, then the menu's
#			upper-left corner goes at (x,y).

proc tkPostOverPoint {menu x y {entry {}}}  {
    global tcl_platform
    
    if {[string compare $entry {}]} {
	if {$entry == [$menu index last]} {
	    incr y [expr {-([$menu yposition $entry] \
		    + [winfo reqheight $menu])/2}]
	} else {
	    incr y [expr {-([$menu yposition $entry] \
		    + [$menu yposition [expr {$entry+1}]])/2}]
	}
	incr x [expr {-[winfo reqwidth $menu]/2}]
    }
    $menu post $x $y
    if {[string compare $entry {}]
          && [string compare [$menu entrycget $entry -state] "disabled"]} {
	$menu activate $entry
	tkGenerateMenuSelect $menu
    }
}

# tkSaveGrabInfo --
# Sets the variables tkPriv(oldGrab) and tkPriv(grabStatus) to record
# the state of any existing grab on the w's display.
#
# Arguments:
# w -			Name of a window;  used to select the display
#			whose grab information is to be recorded.

proc tkSaveGrabInfo w {
    global tkPriv
    set tkPriv(oldGrab) [grab current $w]
    if {[string compare $tkPriv(oldGrab) ""]} {
	set tkPriv(grabStatus) [grab status $tkPriv(oldGrab)]
    }
}

# tkRestoreOldGrab --
# Restores the grab to what it was before TkSaveGrabInfo was called.
#

proc tkRestoreOldGrab {} {
    global tkPriv

    if {[string compare $tkPriv(oldGrab) ""]} {

    	# Be careful restoring the old grab, since it's window may not
	# be visible anymore.

	catch {
          if {![string compare $tkPriv(grabStatus) "global"]} {
		grab set -global $tkPriv(oldGrab)
	    } else {
		grab set $tkPriv(oldGrab)
	    }
	}
	set tkPriv(oldGrab) ""
    }
}

proc tk_menuSetFocus {menu} {
    global tkPriv
    if {![info exists tkPriv(focus)] || ![string compare $tkPriv(focus) {}]} {
	set tkPriv(focus) [focus]
    }
    focus $menu
}
    
proc tkGenerateMenuSelect {menu} {
    global tkPriv

    if {![string compare $tkPriv(activeMenu) $menu] \
          && ![string compare $tkPriv(activeItem) [$menu index active]]} {
	return
    }

    set tkPriv(activeMenu) $menu
    set tkPriv(activeItem) [$menu index active]
    event generate $menu <<MenuSelect>>
}

# tk_popup --
# This procedure pops up a menu and sets things up for traversing
# the menu and its submenus.
#
# Arguments:
# menu -		Name of the menu to be popped up.
# x, y -		Root coordinates at which to pop up the
#			menu.
# entry -		Index of a menu entry to center over (x,y).
#			If omitted or specified as {}, then menu's
#			upper-left corner goes at (x,y).

proc tk_popup {menu x y {entry {}}} {
    global tkPriv
    global tcl_platform
    if {[string compare $tkPriv(popup) ""]
          || [string compare $tkPriv(postedMb) ""]} {
	tkMenuUnpost {}
    }
    tkPostOverPoint $menu $x $y $entry
    if {![string compare $tcl_platform(platform) "unix"] \
	    && [winfo viewable $menu]} {
        tkSaveGrabInfo $menu
	grab -global $menu
	set tkPriv(popup) $menu
	tk_menuSetFocus $menu
    }
}
