# comdlg.tcl --
#
#	Some functions needed for the common dialog boxes. Probably need to go
#	in a different file.
#
# RCS: @(#) Id
#
# Copyright (c) 1996 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

# tclParseConfigSpec --
#
#	Parses a list of "-option value" pairs. If all options and
#	values are legal, the values are stored in
#	$data($option). Otherwise an error message is returned. When
#	an error happens, the data() array may have been partially
#	modified, but all the modified members of the data(0 array are
#	guaranteed to have valid values. This is different than
#	Tk_ConfigureWidget() which does not modify the value of a
#	widget record if any error occurs.
#
# Arguments:
#
# w = widget record to modify. Must be the pathname of a widget.
#
# specs = {
#    {-commandlineswitch resourceName ResourceClass defaultValue verifier}
#    {....}
# }
#
# flags = currently unused.
#
# argList = The list of  "-option value" pairs.
#
proc tclParseConfigSpec {w specs flags argList} {
    upvar #0 $w data

    # 1: Put the specs in associative arrays for faster access
    #
    foreach spec $specs {
	if {[llength $spec] < 4} {
	    error "\"spec\" should contain 5 or 4 elements"
	}
	set cmdsw [lindex $spec 0]
	set cmd($cmdsw) ""
	set rname($cmdsw)   [lindex $spec 1]
	set rclass($cmdsw)  [lindex $spec 2]
	set def($cmdsw)     [lindex $spec 3]
	set verproc($cmdsw) [lindex $spec 4]
    }

    if {[llength $argList] & 1} {
	set cmdsw [lindex $argList end]
	if {![info exists cmd($cmdsw)]} {
	    error "bad option \"$cmdsw\": must be [tclListValidFlags cmd]"
	}
	error "value for \"$cmdsw\" missing"
    }

    # 2: set the default values
    #
    foreach cmdsw [array names cmd] {
	set data($cmdsw) $def($cmdsw)
    }

    # 3: parse the argument list
    #
    foreach {cmdsw value} $argList {
	if {![info exists cmd($cmdsw)]} {
	    error "bad option \"$cmdsw\": must be [tclListValidFlags cmd]"
	}
	set data($cmdsw) $value
    }

    # Done!
}

proc tclListValidFlags {v} {
    upvar $v cmd

    set len [llength [array names cmd]]
    set i 1
    set separator ""
    set errormsg ""
    foreach cmdsw [lsort [array names cmd]] {
	append errormsg "$separator$cmdsw"
	incr i
	if {$i == $len} {
	    set separator ", or "
	} else {
	    set separator ", "
	}
    }
    return $errormsg
}

# This procedure is used to sort strings in a case-insenstive mode.
#
proc tclSortNoCase {str1 str2} {
    return [string compare [string toupper $str1] [string toupper $str2]]
}


# Gives an error if the string does not contain a valid integer
# number
#
proc tclVerifyInteger {string} {
    lindex {1 2 3} $string
}


#----------------------------------------------------------------------
#
#			Focus Group
#
# Focus groups are used to handle the user's focusing actions inside a
# toplevel.
#
# One example of using focus groups is: when the user focuses on an
# entry, the text in the entry is highlighted and the cursor is put to
# the end of the text. When the user changes focus to another widget,
# the text in the previously focused entry is validated.
#
#----------------------------------------------------------------------


# tkFocusGroup_Create --
#
#	Create a focus group. All the widgets in a focus group must be
#	within the same focus toplevel. Each toplevel can have only
#	one focus group, which is identified by the name of the
#	toplevel widget.
#
proc tkFocusGroup_Create {t} {
    global tkPriv
    if {[string compare [winfo toplevel $t] $t]} {
	error "$t is not a toplevel window"
    }
    if {![info exists tkPriv(fg,$t)]} {
	set tkPriv(fg,$t) 1
	set tkPriv(focus,$t) ""
	bind $t <FocusIn>  "tkFocusGroup_In  $t %W %d"
	bind $t <FocusOut> "tkFocusGroup_Out $t %W %d"
	bind $t <Destroy>  "tkFocusGroup_Destroy $t %W"
    }
}

# tkFocusGroup_BindIn --
#
# Add a widget into the "FocusIn" list of the focus group. The $cmd will be
# called when the widget is focused on by the user.
#
proc tkFocusGroup_BindIn {t w cmd} {
    global tkFocusIn tkPriv
    if {![info exists tkPriv(fg,$t)]} {
	error "focus group \"$t\" doesn't exist"
    }
    set tkFocusIn($t,$w) $cmd
}


# tkFocusGroup_BindOut --
#
#	Add a widget into the "FocusOut" list of the focus group. The
#	$cmd will be called when the widget loses the focus (User
#	types Tab or click on another widget).
#
proc tkFocusGroup_BindOut {t w cmd} {
    global tkFocusOut tkPriv
    if {![info exists tkPriv(fg,$t)]} {
	error "focus group \"$t\" doesn't exist"
    }
    set tkFocusOut($t,$w) $cmd
}

# tkFocusGroup_Destroy --
#
#	Cleans up when members of the focus group is deleted, or when the
#	toplevel itself gets deleted.
#
proc tkFocusGroup_Destroy {t w} {
    global tkPriv tkFocusIn tkFocusOut

    if {![string compare $t $w]} {
	unset tkPriv(fg,$t)
	unset tkPriv(focus,$t) 

	foreach name [array names tkFocusIn $t,*] {
	    unset tkFocusIn($name)
	}
	foreach name [array names tkFocusOut $t,*] {
	    unset tkFocusOut($name)
	}
    } else {
	if {[info exists tkPriv(focus,$t)]} {
	    if {![string compare $tkPriv(focus,$t) $w]} {
		set tkPriv(focus,$t) ""
	    }
	}
	catch {
	    unset tkFocusIn($t,$w)
	}
	catch {
	    unset tkFocusOut($t,$w)
	}
    }
}

# tkFocusGroup_In --
#
#	Handles the <FocusIn> event. Calls the FocusIn command for the newly
#	focused widget in the focus group.
#
proc tkFocusGroup_In {t w detail} {
    global tkPriv tkFocusIn

    if {![info exists tkFocusIn($t,$w)]} {
	set tkFocusIn($t,$w) ""
	return
    }
    if {![info exists tkPriv(focus,$t)]} {
	return
    }
    if {![string compare $tkPriv(focus,$t) $w]} {
	# This is already in focus
	#
	return
    } else {
	set tkPriv(focus,$t) $w
	eval $tkFocusIn($t,$w)
    }
}

# tkFocusGroup_Out --
#
#	Handles the <FocusOut> event. Checks if this is really a lose
#	focus event, not one generated by the mouse moving out of the
#	toplevel window.  Calls the FocusOut command for the widget
#	who loses its focus.
#
proc tkFocusGroup_Out {t w detail} {
    global tkPriv tkFocusOut

    if {[string compare $detail NotifyNonlinear] &&
	[string compare $detail NotifyNonlinearVirtual]} {
	# This is caused by mouse moving out of the window
	return
    }
    if {![info exists tkPriv(focus,$t)]} {
	return
    }
    if {![info exists tkFocusOut($t,$w)]} {
	return
    } else {
	eval $tkFocusOut($t,$w)
	set tkPriv(focus,$t) ""
    }
}

# tkFDGetFileTypes --
#
#	Process the string given by the -filetypes option of the file
#	dialogs. Similar to the C function TkGetFileFilters() on the Mac
#	and Windows platform.
#
proc tkFDGetFileTypes {string} {
    foreach t $string {
	if {[llength $t] < 2 || [llength $t] > 3} {
	    error "bad file type \"$t\", should be \"typeName {extension ?extensions ...?} ?{macType ?macTypes ...?}?\""
	}
	eval lappend [list fileTypes([lindex $t 0])] [lindex $t 1]
    }

    set types {}
    foreach t $string {
	set label [lindex $t 0]
	set exts {}

	if {[info exists hasDoneType($label)]} {
	    continue
	}

	set name "$label ("
	set sep ""
	foreach ext $fileTypes($label) {
	    if {![string compare $ext ""]} {
		continue
	    }
	    regsub {^[.]} $ext "*." ext
	    if {![info exists hasGotExt($label,$ext)]} {
		append name $sep$ext
		lappend exts $ext
		set hasGotExt($label,$ext) 1
	    }
	    set sep ,
	}
	append name ")"
	lappend types [list $name $exts]

	set hasDoneType($label) 1
    }

    return $types
}
