# tkfbox.tcl --
#
#	Implements the "TK" standard file selection dialog box. This
#	dialog box is used on the Unix platforms whenever the tk_strictMotif
#	flag is not set.
#
#	The "TK" standard file selection dialog box is similar to the
#	file selection dialog box on Win95(TM). The user can navigate
#	the directories by clicking on the folder icons or by
#	selectinf the "Directory" option menu. The user can select
#	files by clicking on the file icons or by entering a filename
#	in the "Filename:" entry.
#
# RCS: @(#) Id
#
# Copyright (c) 1994-1998 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

#----------------------------------------------------------------------
#
#		      I C O N   L I S T
#
# This is a pseudo-widget that implements the icon list inside the 
# tkFDialog dialog box.
#
#----------------------------------------------------------------------

# tkIconList --
#
#	Creates an IconList widget.
#
proc tkIconList {w args} {
    upvar #0 $w data

    tkIconList_Config $w $args
    tkIconList_Create $w
}

# tkIconList_Config --
#
#	Configure the widget variables of IconList, according to the command
#	line arguments.
#
proc tkIconList_Config {w argList} {
    upvar #0 $w data

    # 1: the configuration specs
    #
    set specs {
	{-browsecmd "" "" ""}
	{-command "" "" ""}
    }

    # 2: parse the arguments
    #
    tclParseConfigSpec $w $specs "" $argList
}

# tkIconList_Create --
#
#	Creates an IconList widget by assembling a canvas widget and a
#	scrollbar widget. Sets all the bindings necessary for the IconList's
#	operations.
#
proc tkIconList_Create {w} {
    upvar #0 $w data

    frame $w
    set data(sbar)   [scrollbar $w.sbar -orient horizontal \
	-highlightthickness 0 -takefocus 0]
    set data(canvas) [canvas $w.canvas -bd 2 -relief sunken \
	-width 400 -height 120 -takefocus 1]
    pack $data(sbar) -side bottom -fill x -padx 2
    pack $data(canvas) -expand yes -fill both

    $data(sbar) config -command [list $data(canvas) xview]
    $data(canvas) config -xscrollcommand [list $data(sbar) set]

    # Initializes the max icon/text width and height and other variables
    #
    set data(maxIW) 1
    set data(maxIH) 1
    set data(maxTW) 1
    set data(maxTH) 1
    set data(numItems) 0
    set data(curItem)  {}
    set data(noScroll) 1

    # Creates the event bindings.
    #
    bind $data(canvas) <Configure>	[list tkIconList_Arrange $w]

    bind $data(canvas) <1>		[list tkIconList_Btn1 $w %x %y]
    bind $data(canvas) <B1-Motion>	[list tkIconList_Motion1 $w %x %y]
    bind $data(canvas) <B1-Leave>	[list tkIconList_Leave1 $w %x %y]
    bind $data(canvas) <B1-Enter>	[list tkCancelRepeat]
    bind $data(canvas) <ButtonRelease-1> [list tkCancelRepeat]
    bind $data(canvas) <Double-ButtonRelease-1> \
	    [list tkIconList_Double1 $w %x %y]

    bind $data(canvas) <Up>		[list tkIconList_UpDown $w -1]
    bind $data(canvas) <Down>		[list tkIconList_UpDown $w  1]
    bind $data(canvas) <Left>		[list tkIconList_LeftRight $w -1]
    bind $data(canvas) <Right>		[list tkIconList_LeftRight $w  1]
    bind $data(canvas) <Return>		[list tkIconList_ReturnKey $w]
    bind $data(canvas) <KeyPress>	[list tkIconList_KeyPress $w %A]
    bind $data(canvas) <Control-KeyPress> ";"
    bind $data(canvas) <Alt-KeyPress>	";"

    bind $data(canvas) <FocusIn>	[list tkIconList_FocusIn $w]

    return $w
}

# tkIconList_AutoScan --
#
# This procedure is invoked when the mouse leaves an entry window
# with button 1 down.  It scrolls the window up, down, left, or
# right, depending on where the mouse left the window, and reschedules
# itself as an "after" command so that the window continues to scroll until
# the mouse moves back into the window or the mouse button is released.
#
# Arguments:
# w -		The IconList window.
#
proc tkIconList_AutoScan {w} {
    upvar #0 $w data
    global tkPriv

    if {![winfo exists $w]} return
    set x $tkPriv(x)
    set y $tkPriv(y)

    if {$data(noScroll)} {
	return
    }
    if {$x >= [winfo width $data(canvas)]} {
	$data(canvas) xview scroll 1 units
    } elseif {$x < 0} {
	$data(canvas) xview scroll -1 units
    } elseif {$y >= [winfo height $data(canvas)]} {
	# do nothing
    } elseif {$y < 0} {
	# do nothing
    } else {
	return
    }

    tkIconList_Motion1 $w $x $y
    set tkPriv(afterId) [after 50 [list tkIconList_AutoScan $w]]
}

# Deletes all the items inside the canvas subwidget and reset the IconList's
# state.
#
proc tkIconList_DeleteAll {w} {
    upvar #0 $w data
    upvar #0 $w:itemList itemList

    $data(canvas) delete all
    catch {unset data(selected)}
    catch {unset data(rect)}
    catch {unset data(list)}
    catch {unset itemList}
    set data(maxIW) 1
    set data(maxIH) 1
    set data(maxTW) 1
    set data(maxTH) 1
    set data(numItems) 0
    set data(curItem)  {}
    set data(noScroll) 1
    $data(sbar) set 0.0 1.0
    $data(canvas) xview moveto 0
}

# Adds an icon into the IconList with the designated image and text
#
proc tkIconList_Add {w image text} {
    upvar #0 $w data
    upvar #0 $w:itemList itemList
    upvar #0 $w:textList textList

    set iTag [$data(canvas) create image 0 0 -image $image -anchor nw]
    set tTag [$data(canvas) create text  0 0 -text  $text  -anchor nw \
	-font $data(font)]
    set rTag [$data(canvas) create rect  0 0 0 0 -fill "" -outline ""]
    
    set b [$data(canvas) bbox $iTag]
    set iW [expr {[lindex $b 2]-[lindex $b 0]}]
    set iH [expr {[lindex $b 3]-[lindex $b 1]}]
    if {$data(maxIW) < $iW} {
	set data(maxIW) $iW
    }
    if {$data(maxIH) < $iH} {
	set data(maxIH) $iH
    }
    
    set b [$data(canvas) bbox $tTag]
    set tW [expr {[lindex $b 2]-[lindex $b 0]}]
    set tH [expr {[lindex $b 3]-[lindex $b 1]}]
    if {$data(maxTW) < $tW} {
	set data(maxTW) $tW
    }
    if {$data(maxTH) < $tH} {
	set data(maxTH) $tH
    }
    
    lappend data(list) [list $iTag $tTag $rTag $iW $iH $tW $tH $data(numItems)]
    set itemList($rTag) [list $iTag $tTag $text $data(numItems)]
    set textList($data(numItems)) [string tolower $text]
    incr data(numItems)
}

# Places the icons in a column-major arrangement.
#
proc tkIconList_Arrange {w} {
    upvar #0 $w data

    if {![info exists data(list)]} {
	if {[info exists data(canvas)] && [winfo exists $data(canvas)]} {
	    set data(noScroll) 1
	    $data(sbar) config -command ""
	}
	return
    }

    set W [winfo width  $data(canvas)]
    set H [winfo height $data(canvas)]
    set pad [expr {[$data(canvas) cget -highlightthickness] + \
	    [$data(canvas) cget -bd]}]
    if {$pad < 2} {
	set pad 2
    }

    incr W -[expr {$pad*2}]
    incr H -[expr {$pad*2}]

    set dx [expr {$data(maxIW) + $data(maxTW) + 8}]
    if {$data(maxTH) > $data(maxIH)} {
	set dy $data(maxTH)
    } else {
	set dy $data(maxIH)
    }
    incr dy 2
    set shift [expr {$data(maxIW) + 4}]

    set x [expr {$pad * 2}]
    set y [expr {$pad * 1}] ; # Why * 1 ?
    set usedColumn 0
    foreach sublist $data(list) {
	set usedColumn 1
	set iTag [lindex $sublist 0]
	set tTag [lindex $sublist 1]
	set rTag [lindex $sublist 2]
	set iW   [lindex $sublist 3]
	set iH   [lindex $sublist 4]
	set tW   [lindex $sublist 5]
	set tH   [lindex $sublist 6]

	set i_dy [expr {($dy - $iH)/2}]
	set t_dy [expr {($dy - $tH)/2}]

	$data(canvas) coords $iTag $x                    [expr {$y + $i_dy}]
	$data(canvas) coords $tTag [expr {$x + $shift}]  [expr {$y + $t_dy}]
	$data(canvas) coords $tTag [expr {$x + $shift}]  [expr {$y + $t_dy}]
	$data(canvas) coords $rTag $x $y [expr {$x+$dx}] [expr {$y+$dy}]

	incr y $dy
	if {($y + $dy) > $H} {
	    set y [expr {$pad * 1}] ; # *1 ?
	    incr x $dx
	    set usedColumn 0
	}
    }

    if {$usedColumn} {
	set sW [expr {$x + $dx}]
    } else {
	set sW $x
    }

    if {$sW < $W} {
	$data(canvas) config -scrollregion [list $pad $pad $sW $H]
	$data(sbar) config -command ""
	$data(canvas) xview moveto 0
	set data(noScroll) 1
    } else {
	$data(canvas) config -scrollregion [list $pad $pad $sW $H]
	$data(sbar) config -command [list $data(canvas) xview]
	set data(noScroll) 0
    }

    set data(itemsPerColumn) [expr {($H-$pad)/$dy}]
    if {$data(itemsPerColumn) < 1} {
	set data(itemsPerColumn) 1
    }

    if {$data(curItem) != ""} {
	tkIconList_Select $w [lindex [lindex $data(list) $data(curItem)] 2] 0
    }
}

# Gets called when the user invokes the IconList (usually by double-clicking
# or pressing the Return key).
#
proc tkIconList_Invoke {w} {
    upvar #0 $w data

    if {$data(-command) != "" && [info exists data(selected)]} {
	uplevel #0 $data(-command)
    }
}

# tkIconList_See --
#
#	If the item is not (completely) visible, scroll the canvas so that
#	it becomes visible.
proc tkIconList_See {w rTag} {
    upvar #0 $w data
    upvar #0 $w:itemList itemList

    if {$data(noScroll)} {
	return
    }
    set sRegion [$data(canvas) cget -scrollregion]
    if {[string equal $sRegion {}]} {
	return
    }

    if {![info exists itemList($rTag)]} {
	return
    }


    set bbox [$data(canvas) bbox $rTag]
    set pad [expr {[$data(canvas) cget -highlightthickness] + \
	    [$data(canvas) cget -bd]}]

    set x1 [lindex $bbox 0]
    set x2 [lindex $bbox 2]
    incr x1 -[expr {$pad * 2}]
    incr x2 -[expr {$pad * 1}] ; # *1 ?

    set cW [expr {[winfo width $data(canvas)] - $pad*2}]

    set scrollW [expr {[lindex $sRegion 2]-[lindex $sRegion 0]+1}]
    set dispX [expr {int([lindex [$data(canvas) xview] 0]*$scrollW)}]
    set oldDispX $dispX

    # check if out of the right edge
    #
    if {($x2 - $dispX) >= $cW} {
	set dispX [expr {$x2 - $cW}]
    }
    # check if out of the left edge
    #
    if {($x1 - $dispX) < 0} {
	set dispX $x1
    }

    if {$oldDispX != $dispX} {
	set fraction [expr {double($dispX)/double($scrollW)}]
	$data(canvas) xview moveto $fraction
    }
}

proc tkIconList_SelectAtXY {w x y} {
    upvar #0 $w data

    tkIconList_Select $w [$data(canvas) find closest \
	    [$data(canvas) canvasx $x] [$data(canvas) canvasy $y]]
}

proc tkIconList_Select {w rTag {callBrowse 1}} {
    upvar #0 $w data
    upvar #0 $w:itemList itemList

    if {![info exists itemList($rTag)]} {
	return
    }
    set iTag   [lindex $itemList($rTag) 0]
    set tTag   [lindex $itemList($rTag) 1]
    set text   [lindex $itemList($rTag) 2]
    set serial [lindex $itemList($rTag) 3]

    if {![info exists data(rect)]} {
        set data(rect) [$data(canvas) create rect 0 0 0 0 \
		-fill #a0a0ff -outline #a0a0ff]
    }
    $data(canvas) lower $data(rect)
    set bbox [$data(canvas) bbox $tTag]
    eval [list $data(canvas) coords $data(rect)] $bbox

    set data(curItem) $serial
    set data(selected) $text

    if {$callBrowse && $data(-browsecmd) != ""} {
	eval $data(-browsecmd) [list $text]
    }
}

proc tkIconList_Unselect {w} {
    upvar #0 $w data

    if {[info exists data(rect)]} {
	$data(canvas) delete $data(rect)
	unset data(rect)
    }
    if {[info exists data(selected)]} {
	unset data(selected)
    }
    #set data(curItem)  {}
}

# Returns the selected item
#
proc tkIconList_Get {w} {
    upvar #0 $w data

    if {[info exists data(selected)]} {
	return $data(selected)
    } else {
	return ""
    }
}


proc tkIconList_Btn1 {w x y} {
    upvar #0 $w data

    focus $data(canvas)
    tkIconList_SelectAtXY $w $x $y
}

# Gets called on button-1 motions
#
proc tkIconList_Motion1 {w x y} {
    global tkPriv
    set tkPriv(x) $x
    set tkPriv(y) $y

    tkIconList_SelectAtXY $w $x $y
}

proc tkIconList_Double1 {w x y} {
    upvar #0 $w data

    if {[string compare $data(curItem) {}]} {
	tkIconList_Invoke $w
    }
}

proc tkIconList_ReturnKey {w} {
    tkIconList_Invoke $w
}

proc tkIconList_Leave1 {w x y} {
    global tkPriv

    set tkPriv(x) $x
    set tkPriv(y) $y
    tkIconList_AutoScan $w
}

proc tkIconList_FocusIn {w} {
    upvar #0 $w data

    if {![info exists data(list)]} {
	return
    }

    if {[string compare $data(curItem) {}]} {
	tkIconList_Select $w [lindex [lindex $data(list) $data(curItem)] 2] 1
    }
}

# tkIconList_UpDown --
#
# Moves the active element up or down by one element
#
# Arguments:
# w -		The IconList widget.
# amount -	+1 to move down one item, -1 to move back one item.
#
proc tkIconList_UpDown {w amount} {
    upvar #0 $w data

    if {![info exists data(list)]} {
	return
    }

    if {[string equal $data(curItem) {}]} {
	set rTag [lindex [lindex $data(list) 0] 2]
    } else {
	set oldRTag [lindex [lindex $data(list) $data(curItem)] 2]
	set rTag [lindex [lindex $data(list) [expr {$data(curItem)+$amount}]] 2]
	if {[string equal $rTag ""]} {
	    set rTag $oldRTag
	}
    }

    if {[string compare $rTag ""]} {
	tkIconList_Select $w $rTag
	tkIconList_See $w $rTag
    }
}

# tkIconList_LeftRight --
#
# Moves the active element left or right by one column
#
# Arguments:
# w -		The IconList widget.
# amount -	+1 to move right one column, -1 to move left one column.
#
proc tkIconList_LeftRight {w amount} {
    upvar #0 $w data

    if {![info exists data(list)]} {
	return
    }
    if {[string equal $data(curItem) {}]} {
	set rTag [lindex [lindex $data(list) 0] 2]
    } else {
	set oldRTag [lindex [lindex $data(list) $data(curItem)] 2]
	set newItem [expr {$data(curItem)+($amount*$data(itemsPerColumn))}]
	set rTag [lindex [lindex $data(list) $newItem] 2]
	if {[string equal $rTag ""]} {
	    set rTag $oldRTag
	}
    }

    if {[string compare $rTag ""]} {
	tkIconList_Select $w $rTag
	tkIconList_See $w $rTag
    }
}

#----------------------------------------------------------------------
#		Accelerator key bindings
#----------------------------------------------------------------------

# tkIconList_KeyPress --
#
#	Gets called when user enters an arbitrary key in the listbox.
#
proc tkIconList_KeyPress {w key} {
    global tkPriv

    append tkPriv(ILAccel,$w) $key
    tkIconList_Goto $w $tkPriv(ILAccel,$w)
    catch {
	after cancel $tkPriv(ILAccel,$w,afterId)
    }
    set tkPriv(ILAccel,$w,afterId) [after 500 [list tkIconList_Reset $w]]
}

proc tkIconList_Goto {w text} {
    upvar #0 $w data
    upvar #0 $w:textList textList
    global tkPriv
    
    if {![info exists data(list)]} {
	return
    }

    if {[string equal {} $text]} {
	return
    }

    if {$data(curItem) == "" || $data(curItem) == 0} {
	set start  0
    } else {
	set start  $data(curItem)
    }

    set text [string tolower $text]
    set theIndex -1
    set less 0
    set len [string length $text]
    set len0 [expr {$len-1}]
    set i $start

    # Search forward until we find a filename whose prefix is an exact match
    # with $text
    while {1} {
	set sub [string range $textList($i) 0 $len0]
	if {[string equal $text $sub]} {
	    set theIndex $i
	    break
	}
	incr i
	if {$i == $data(numItems)} {
	    set i 0
	}
	if {$i == $start} {
	    break
	}
    }

    if {$theIndex > -1} {
	set rTag [lindex [lindex $data(list) $theIndex] 2]
	tkIconList_Select $w $rTag
	tkIconList_See $w $rTag
    }
}

proc tkIconList_Reset {w} {
    global tkPriv

    catch {unset tkPriv(ILAccel,$w)}
}

#----------------------------------------------------------------------
#
#		      F I L E   D I A L O G
#
#----------------------------------------------------------------------

namespace eval ::tk::dialog {}
namespace eval ::tk::dialog::file {}

# ::tk::dialog::file::tkFDialog --
#
#	Implements the TK file selection dialog. This dialog is used when
#	the tk_strictMotif flag is set to false. This procedure shouldn't
#	be called directly. Call tk_getOpenFile or tk_getSaveFile instead.
#
# Arguments:
#	type		"open" or "save"
#	args		Options parsed by the procedure.
#

proc ::tk::dialog::file::tkFDialog {type args} {
    global tkPriv
    set dataName __tk_filedialog
    upvar ::tk::dialog::file::$dataName data

    ::tk::dialog::file::Config $dataName $type $args

    if {[string equal $data(-parent) .]} {
        set w .$dataName
    } else {
        set w $data(-parent).$dataName
    }

    # (re)create the dialog box if necessary
    #
    if {![winfo exists $w]} {
	::tk::dialog::file::Create $w TkFDialog
    } elseif {[string compare [winfo class $w] TkFDialog]} {
	destroy $w
	::tk::dialog::file::Create $w TkFDialog
    } else {
	set data(dirMenuBtn) $w.f1.menu
	set data(dirMenu) $w.f1.menu.menu
	set data(upBtn) $w.f1.up
	set data(icons) $w.icons
	set data(ent) $w.f2.ent
	set data(typeMenuLab) $w.f3.lab
	set data(typeMenuBtn) $w.f3.menu
	set data(typeMenu) $data(typeMenuBtn).m
	set data(okBtn) $w.f2.ok
	set data(cancelBtn) $w.f3.cancel
    }
    wm transient $w $data(-parent)

    # Add traces on the selectPath variable
    #

    trace variable data(selectPath) w "::tk::dialog::file::SetPath $w"
    $data(dirMenuBtn) configure \
	    -textvariable ::tk::dialog::file::${dataName}(selectPath)

    # Initialize the file types menu
    #
    if {[llength $data(-filetypes)]} {
	$data(typeMenu) delete 0 end
	foreach type $data(-filetypes) {
	    set title  [lindex $type 0]
	    set filter [lindex $type 1]
	    $data(typeMenu) add command -label $title \
		-command [list ::tk::dialog::file::SetFilter $w $type]
	}
	::tk::dialog::file::SetFilter $w [lindex $data(-filetypes) 0]
	$data(typeMenuBtn) config -state normal
	$data(typeMenuLab) config -state normal
    } else {
	set data(filter) "*"
	$data(typeMenuBtn) config -state disabled -takefocus 0
	$data(typeMenuLab) config -state disabled
    }
    ::tk::dialog::file::UpdateWhenIdle $w

    # Withdraw the window, then update all the geometry information
    # so we know how big it wants to be, then center the window in the
    # display and de-iconify it.

    ::tk::PlaceWindow $w widget $data(-parent)
    wm title $w $data(-title)

    # Set a grab and claim the focus too.

    ::tk::SetFocusGrab $w $data(ent)
    $data(ent) delete 0 end
    $data(ent) insert 0 $data(selectFile)
    $data(ent) selection range 0 end
    $data(ent) icursor end

    # Wait for the user to respond, then restore the focus and
    # return the index of the selected button.  Restore the focus
    # before deleting the window, since otherwise the window manager
    # may take the focus away so we can't redirect it.  Finally,
    # restore any grab that was in effect.

    tkwait variable tkPriv(selectFilePath)

    ::tk::RestoreFocusGrab $w $data(ent) withdraw

    # Cleanup traces on selectPath variable
    #

    foreach trace [trace vinfo data(selectPath)] {
	trace vdelete data(selectPath) [lindex $trace 0] [lindex $trace 1]
    }
    $data(dirMenuBtn) configure -textvariable {}

    return $tkPriv(selectFilePath)
}

# ::tk::dialog::file::Config --
#
#	Configures the TK filedialog according to the argument list
#
proc ::tk::dialog::file::Config {dataName type argList} {
    upvar ::tk::dialog::file::$dataName data

    set data(type) $type

    # 0: Delete all variable that were set on data(selectPath) the
    # last time the file dialog is used. The traces may cause troubles
    # if the dialog is now used with a different -parent option.

    foreach trace [trace vinfo data(selectPath)] {
	trace vdelete data(selectPath) [lindex $trace 0] [lindex $trace 1]
    }

    # 1: the configuration specs
    #
    set specs {
	{-defaultextension "" "" ""}
	{-filetypes "" "" ""}
	{-initialdir "" "" ""}
	{-initialfile "" "" ""}
	{-parent "" "" "."}
	{-title "" "" ""}
    }

    # 2: default values depending on the type of the dialog
    #
    if {![info exists data(selectPath)]} {
	# first time the dialog has been popped up
	set data(selectPath) [pwd]
	set data(selectFile) ""
    }

    # 3: parse the arguments
    #
    tclParseConfigSpec ::tk::dialog::file::$dataName $specs "" $argList

    if {$data(-title) == ""} {
	if {[string equal $type "open"]} {
	    set data(-title) "Open"
	} else {
	    set data(-title) "Save As"
	}
    }

    # 4: set the default directory and selection according to the -initial
    #    settings
    #
    if {$data(-initialdir) != ""} {
	# Ensure that initialdir is an absolute path name.
	if {[file isdirectory $data(-initialdir)]} {
	    set old [pwd]
	    cd $data(-initialdir)
	    set data(selectPath) [pwd]
	    cd $old
	} else {
	    set data(selectPath) [pwd]
	}
    }
    set data(selectFile) $data(-initialfile)

    # 5. Parse the -filetypes option
    #
    set data(-filetypes) [tkFDGetFileTypes $data(-filetypes)]

    if {![winfo exists $data(-parent)]} {
	error "bad window path name \"$data(-parent)\""
    }
}

proc ::tk::dialog::file::Create {w class} {
    set dataName [lindex [split $w .] end]
    upvar ::tk::dialog::file::$dataName data
    global tk_library tkPriv

    toplevel $w -class $class

    # f1: the frame with the directory option menu
    #
    set f1 [frame $w.f1]
    label $f1.lab -text "Directory:" -under 0
    set data(dirMenuBtn) $f1.menu
    set data(dirMenu) [tk_optionMenu $f1.menu [format %s(selectPath) ::tk::dialog::file::$dataName] ""]
    set data(upBtn) [button $f1.up]
    if {![info exists tkPriv(updirImage)]} {
	set tkPriv(updirImage) [image create bitmap -data {
#define updir_width 28
#define updir_height 16
static char updir_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x80, 0x1f, 0x00, 0x00, 0x40, 0x20, 0x00, 0x00,
   0x20, 0x40, 0x00, 0x00, 0xf0, 0xff, 0xff, 0x01, 0x10, 0x00, 0x00, 0x01,
   0x10, 0x02, 0x00, 0x01, 0x10, 0x07, 0x00, 0x01, 0x90, 0x0f, 0x00, 0x01,
   0x10, 0x02, 0x00, 0x01, 0x10, 0x02, 0x00, 0x01, 0x10, 0x02, 0x00, 0x01,
   0x10, 0xfe, 0x07, 0x01, 0x10, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0x01,
   0xf0, 0xff, 0xff, 0x01};}]
    }
    $data(upBtn) config -image $tkPriv(updirImage)

    $f1.menu config -takefocus 1 -highlightthickness 2
 
    pack $data(upBtn) -side right -padx 4 -fill both
    pack $f1.lab -side left -padx 4 -fill both
    pack $f1.menu -expand yes -fill both -padx 4

    # data(icons): the IconList that list the files and directories.
    #
    if { [string equal $class TkFDialog] } {
	set fNameCaption "File name:"
	set fNameUnder 5
	set iconListCommand [list ::tk::dialog::file::OkCmd $w]
    } else {
	set fNameCaption "Selection:"
	set fNameUnder 0
	set iconListCommand [list ::tk::dialog::file::chooseDir::DblClick $w]
    }
    set data(icons) [tkIconList $w.icons \
	-browsecmd [list ::tk::dialog::file::ListBrowse $w] \
	-command   $iconListCommand]

    # f2: the frame with the OK button and the "file name" field
    #
    set f2 [frame $w.f2 -bd 0]
    label $f2.lab -text $fNameCaption -anchor e -width 14 \
	    -under $fNameUnder -pady 0
    set data(ent) [entry $f2.ent]

    # The font to use for the icons. The default Canvas font on Unix
    # is just deviant.
    global $w.icons
    set $w.icons(font) [$data(ent) cget -font]

    # f3: the frame with the cancel button and the file types field
    #
    set f3 [frame $w.f3 -bd 0]

    # Make the file types bits only if this is a File Dialog
    if { [string equal $class TkFDialog] } {
	# The "File of types:" label needs to be grayed-out when
	# -filetypes are not specified. The label widget does not support
	# grayed-out text on monochrome displays. Therefore, we have to
	# use a button widget to emulate a label widget (by setting its
	# bindtags)
	
	set data(typeMenuLab) [button $f3.lab -text "Files of type:" \
		-anchor e -width 14 -under 9 \
		-bd [$f2.lab cget -bd] \
		-highlightthickness [$f2.lab cget -highlightthickness] \
		-relief [$f2.lab cget -relief] \
		-padx [$f2.lab cget -padx] \
		-pady [$f2.lab cget -pady]]
	bindtags $data(typeMenuLab) [list $data(typeMenuLab) Label \
		[winfo toplevel $data(typeMenuLab)] all]
	
	set data(typeMenuBtn) [menubutton $f3.menu -indicatoron 1 \
		-menu $f3.menu.m]
	set data(typeMenu) [menu $data(typeMenuBtn).m -tearoff 0]
	$data(typeMenuBtn) config -takefocus 1 -highlightthickness 2 \
		-relief raised -bd 2 -anchor w
    }

    # the okBtn is created after the typeMenu so that the keyboard traversal
    # is in the right order
    set data(okBtn)     [button $f2.ok     -text OK     -under 0 -width 6 \
	-default active -pady 3]
    set data(cancelBtn) [button $f3.cancel -text Cancel -under 0 -width 6\
	-default normal -pady 3]

    # pack the widgets in f2 and f3
    #
    pack $data(okBtn) -side right -padx 4 -anchor e
    pack $f2.lab -side left -padx 4
    pack $f2.ent -expand yes -fill x -padx 2 -pady 0
    
    pack $data(cancelBtn) -side right -padx 4 -anchor w
    if { [string equal $class TkFDialog] } {
	pack $data(typeMenuLab) -side left -padx 4
	pack $data(typeMenuBtn) -expand yes -fill x -side right
    }

    # Pack all the frames together. We are done with widget construction.
    #
    pack $f1 -side top -fill x -pady 4
    pack $f3 -side bottom -fill x
    pack $f2 -side bottom -fill x
    pack $data(icons) -expand yes -fill both -padx 4 -pady 1

    # Set up the event handlers that are common to Directory and File Dialogs
    #

    wm protocol $w WM_DELETE_WINDOW [list ::tk::dialog::file::CancelCmd $w]
    $data(upBtn)     config -command [list ::tk::dialog::file::UpDirCmd $w]
    $data(cancelBtn) config -command [list ::tk::dialog::file::CancelCmd $w]
    bind $w <KeyPress-Escape> [list tkButtonInvoke $data(cancelBtn)]
    bind $w <Alt-c> [list tkButtonInvoke $data(cancelBtn)]
    bind $w <Alt-d> [list focus $data(dirMenuBtn)]

    # Set up event handlers specific to File or Directory Dialogs
    #

    if { [string equal $class TkFDialog] } {
	bind $data(ent) <Return> [list ::tk::dialog::file::ActivateEnt $w]
	$data(okBtn)     config -command [list ::tk::dialog::file::OkCmd $w]
	bind $w <Alt-t> [format {
	    if {[string equal [%s cget -state] "normal"]} {
		focus %s
	    }
	} $data(typeMenuBtn) $data(typeMenuBtn)]
	bind $w <Alt-n> [list focus $data(ent)]
	bind $w <Alt-o> [list ::tk::dialog::file::InvokeBtn $w Open]
	bind $w <Alt-s> [list ::tk::dialog::file::InvokeBtn $w Save]
    } else {
	set okCmd [list ::tk::dialog::file::chooseDir::OkCmd $w]
	bind $data(ent) <Return> $okCmd
	$data(okBtn) config -command $okCmd
	bind $w <Alt-s> [list focus $data(ent)]
	bind $w <Alt-o> [list tkButtonInvoke $data(okBtn)]
    }

    # Build the focus group for all the entries
    #
    tkFocusGroup_Create $w
    tkFocusGroup_BindIn $w  $data(ent) [list ::tk::dialog::file::EntFocusIn $w]
    tkFocusGroup_BindOut $w $data(ent) [list ::tk::dialog::file::EntFocusOut $w]
}

# ::tk::dialog::file::UpdateWhenIdle --
#
#	Creates an idle event handler which updates the dialog in idle
#	time. This is important because loading the directory may take a long
#	time and we don't want to load the same directory for multiple times
#	due to multiple concurrent events.
#
proc ::tk::dialog::file::UpdateWhenIdle {w} {
    upvar ::tk::dialog::file::[winfo name $w] data

    if {[info exists data(updateId)]} {
	return
    } else {
	set data(updateId) [after idle [list ::tk::dialog::file::Update $w]]
    }
}

# ::tk::dialog::file::Update --
#
#	Loads the files and directories into the IconList widget. Also
#	sets up the directory option menu for quick access to parent
#	directories.
#
proc ::tk::dialog::file::Update {w} {

    # This proc may be called within an idle handler. Make sure that the
    # window has not been destroyed before this proc is called
    if {![winfo exists $w]} {
	return
    }
    set class [winfo class $w]
    if { [string compare $class TkFDialog] && \
	    [string compare $class TkChooseDir] } {
	return
    }

    set dataName [winfo name $w]
    upvar ::tk::dialog::file::$dataName data
    global tk_library tkPriv
    catch {unset data(updateId)}

    if {![info exists tkPriv(folderImage)]} {
	set tkPriv(folderImage) [image create photo -data {
R0lGODlhEAAMAKEAAAD//wAAAPD/gAAAACH5BAEAAAAALAAAAAAQAAwAAAIghINhyycvVFsB
QtmS3rjaH1Hg141WaT5ouprt2HHcUgAAOw==}]
	set tkPriv(fileImage)   [image create photo -data {
R0lGODlhDAAMAKEAALLA3AAAAP//8wAAACH5BAEAAAAALAAAAAAMAAwAAAIgRI4Ha+IfWHsO
rSASvJTGhnhcV3EJlo3kh53ltF5nAhQAOw==}]
    }
    set folder $tkPriv(folderImage)
    set file   $tkPriv(fileImage)

    set appPWD [pwd]
    if {[catch {
	cd $data(selectPath)
    }]} {
	# We cannot change directory to $data(selectPath). $data(selectPath)
	# should have been checked before ::tk::dialog::file::Update is called, so
	# we normally won't come to here. Anyways, give an error and abort
	# action.
	tk_messageBox -type ok -parent $w -message \
	    "Cannot change to the directory \"$data(selectPath)\".\nPermission denied."\
	    -icon warning
	cd $appPWD
	return
    }

    # Turn on the busy cursor. BUG?? We haven't disabled X events, though,
    # so the user may still click and cause havoc ...
    #
    set entCursor [$data(ent) cget -cursor]
    set dlgCursor [$w         cget -cursor]
    $data(ent) config -cursor watch
    $w         config -cursor watch
    update idletasks
    
    tkIconList_DeleteAll $data(icons)

    # Make the dir list
    #
    foreach f [lsort -dictionary [glob -nocomplain .* *]] {
	if {[string equal $f .]} {
	    continue
	}
	if {[string equal $f ..]} {
	    continue
	}
	if {[file isdir ./$f]} {
	    if {![info exists hasDoneDir($f)]} {
		tkIconList_Add $data(icons) $folder $f
		set hasDoneDir($f) 1
	    }
	}
    }
    if { [string equal $class TkFDialog] } {
	# Make the file list if this is a File Dialog
	#
	if {[string equal $data(filter) *]} {
	    set files [lsort -dictionary \
		    [glob -nocomplain .* *]]
	} else {
	    set files [lsort -dictionary \
		    [eval glob -nocomplain $data(filter)]]
	}
	
	foreach f $files {
	    if {![file isdir ./$f]} {
		if {![info exists hasDoneFile($f)]} {
		    tkIconList_Add $data(icons) $file $f
		    set hasDoneFile($f) 1
		}
	    }
	}
    }

    tkIconList_Arrange $data(icons)

    # Update the Directory: option menu
    #
    set list ""
    set dir ""
    foreach subdir [file split $data(selectPath)] {
	set dir [file join $dir $subdir]
	lappend list $dir
    }

    $data(dirMenu) delete 0 end
    set var [format %s(selectPath) ::tk::dialog::file::$dataName]
    foreach path $list {
	$data(dirMenu) add command -label $path -command [list set $var $path]
    }

    # Restore the PWD to the application's PWD
    #
    cd $appPWD

    if { [string equal $class TkFDialog] } {
	# Restore the Open/Save Button if this is a File Dialog
	#
	if {[string equal $data(type) open]} {
	    $data(okBtn) config -text "Open"
	} else {
	    $data(okBtn) config -text "Save"
	}
    }

    # turn off the busy cursor.
    #
    $data(ent) config -cursor $entCursor
    $w         config -cursor $dlgCursor
}

# ::tk::dialog::file::SetPathSilently --
#
# 	Sets data(selectPath) without invoking the trace procedure
#
proc ::tk::dialog::file::SetPathSilently {w path} {
    upvar ::tk::dialog::file::[winfo name $w] data
    
    trace vdelete  data(selectPath) w [list ::tk::dialog::file::SetPath $w]
    set data(selectPath) $path
    trace variable data(selectPath) w [list ::tk::dialog::file::SetPath $w]
}


# This proc gets called whenever data(selectPath) is set
#
proc ::tk::dialog::file::SetPath {w name1 name2 op} {
    if {[winfo exists $w]} {
	upvar ::tk::dialog::file::[winfo name $w] data
	::tk::dialog::file::UpdateWhenIdle $w
	# On directory dialogs, we keep the entry in sync with the currentdir.
	if { [string equal [winfo class $w] TkChooseDir] } {
	    $data(ent) delete 0 end
	    $data(ent) insert end $data(selectPath)
	}
    }
}

# This proc gets called whenever data(filter) is set
#
proc ::tk::dialog::file::SetFilter {w type} {
    upvar ::tk::dialog::file::[winfo name $w] data
    upvar \#0 $data(icons) icons

    set data(filter) [lindex $type 1]
    $data(typeMenuBtn) config -text [lindex $type 0] -indicatoron 1

    $icons(sbar) set 0.0 0.0
    
    ::tk::dialog::file::UpdateWhenIdle $w
}

# tk::dialog::file::ResolveFile --
#
#	Interpret the user's text input in a file selection dialog.
#	Performs:
#
#	(1) ~ substitution
#	(2) resolve all instances of . and ..
#	(3) check for non-existent files/directories
#	(4) check for chdir permissions
#
# Arguments:
#	context:  the current directory you are in
#	text:	  the text entered by the user
#	defaultext: the default extension to add to files with no extension
#
# Return vaue:
#	[list $flag $directory $file]
#
#	 flag = OK	: valid input
#	      = PATTERN	: valid directory/pattern
#	      = PATH	: the directory does not exist
#	      = FILE	: the directory exists by the file doesn't
#			  exist
#	      = CHDIR	: Cannot change to the directory
#	      = ERROR	: Invalid entry
#
#	 directory      : valid only if flag = OK or PATTERN or FILE
#	 file           : valid only if flag = OK or PATTERN
#
#	directory may not be the same as context, because text may contain
#	a subdirectory name
#
proc ::tk::dialog::file::ResolveFile {context text defaultext} {

    set appPWD [pwd]

    set path [::tk::dialog::file::JoinFile $context $text]

    # If the file has no extension, append the default.  Be careful not
    # to do this for directories, otherwise typing a dirname in the box
    # will give back "dirname.extension" instead of trying to change dir.
    if {![file isdirectory $path] && [string equal [file ext $path] ""]} {
	set path "$path$defaultext"
    }


    if {[catch {file exists $path}]} {
	# This "if" block can be safely removed if the following code
	# stop generating errors.
	#
	#	file exists ~nonsuchuser
	#
	return [list ERROR $path ""]
    }

    if {[file exists $path]} {
	if {[file isdirectory $path]} {
	    if {[catch {cd $path}]} {
		return [list CHDIR $path ""]
	    }
	    set directory [pwd]
	    set file ""
	    set flag OK
	    cd $appPWD
	} else {
	    if {[catch {cd [file dirname $path]}]} {
		return [list CHDIR [file dirname $path] ""]
	    }
	    set directory [pwd]
	    set file [file tail $path]
	    set flag OK
	    cd $appPWD
	}
    } else {
	set dirname [file dirname $path]
	if {[file exists $dirname]} {
	    if {[catch {cd $dirname}]} {
		return [list CHDIR $dirname ""]
	    }
	    set directory [pwd]
	    set file [file tail $path]
	    if {[regexp {[*]|[?]} $file]} {
		set flag PATTERN
	    } else {
		set flag FILE
	    }
	    cd $appPWD
	} else {
	    set directory $dirname
	    set file [file tail $path]
	    set flag PATH
	}
    }

    return [list $flag $directory $file]
}


# Gets called when the entry box gets keyboard focus. We clear the selection
# from the icon list . This way the user can be certain that the input in the 
# entry box is the selection.
#
proc ::tk::dialog::file::EntFocusIn {w} {
    upvar ::tk::dialog::file::[winfo name $w] data

    if {[string compare [$data(ent) get] ""]} {
	$data(ent) selection range 0 end
	$data(ent) icursor end
    } else {
	$data(ent) selection clear
    }

    tkIconList_Unselect $data(icons)

    if { [string equal [winfo class $w] TkFDialog] } {
	# If this is a File Dialog, make sure the buttons are labeled right.
	if {[string equal $data(type) open]} {
	    $data(okBtn) config -text "Open"
	} else {
	    $data(okBtn) config -text "Save"
	}
    }
}

proc ::tk::dialog::file::EntFocusOut {w} {
    upvar ::tk::dialog::file::[winfo name $w] data

    $data(ent) selection clear
}


# Gets called when user presses Return in the "File name" entry.
#
proc ::tk::dialog::file::ActivateEnt {w} {
    upvar ::tk::dialog::file::[winfo name $w] data

    set text [string trim [$data(ent) get]]
    set list [::tk::dialog::file::ResolveFile $data(selectPath) $text \
		  $data(-defaultextension)]
    set flag [lindex $list 0]
    set path [lindex $list 1]
    set file [lindex $list 2]

    switch -- $flag {
	OK {
	    if {[string equal $file ""]} {
		# user has entered an existing (sub)directory
		set data(selectPath) $path
		$data(ent) delete 0 end
	    } else {
		::tk::dialog::file::SetPathSilently $w $path
		set data(selectFile) $file
		::tk::dialog::file::Done $w
	    }
	}
	PATTERN {
	    set data(selectPath) $path
	    set data(filter) $file
	}
	FILE {
	    if {[string equal $data(type) open]} {
		tk_messageBox -icon warning -type ok -parent $w \
		    -message "File \"[file join $path $file]\" does not exist."
		$data(ent) selection range 0 end
		$data(ent) icursor end
	    } else {
		::tk::dialog::file::SetPathSilently $w $path
		set data(selectFile) $file
		::tk::dialog::file::Done $w
	    }
	}
	PATH {
	    tk_messageBox -icon warning -type ok -parent $w \
		-message "Directory \"$path\" does not exist."
	    $data(ent) selection range 0 end
	    $data(ent) icursor end
	}
	CHDIR {
	    tk_messageBox -type ok -parent $w -message \
	       "Cannot change to the directory \"$path\".\nPermission denied."\
		-icon warning
	    $data(ent) selection range 0 end
	    $data(ent) icursor end
	}
	ERROR {
	    tk_messageBox -type ok -parent $w -message \
	       "Invalid file name \"$path\"."\
		-icon warning
	    $data(ent) selection range 0 end
	    $data(ent) icursor end
	}
    }
}

# Gets called when user presses the Alt-s or Alt-o keys.
#
proc ::tk::dialog::file::InvokeBtn {w key} {
    upvar ::tk::dialog::file::[winfo name $w] data

    if {[string equal [$data(okBtn) cget -text] $key]} {
	tkButtonInvoke $data(okBtn)
    }
}

# Gets called when user presses the "parent directory" button
#
proc ::tk::dialog::file::UpDirCmd {w} {
    upvar ::tk::dialog::file::[winfo name $w] data

    if {[string compare $data(selectPath) "/"]} {
	set data(selectPath) [file dirname $data(selectPath)]
    }
}

# Join a file name to a path name. The "file join" command will break
# if the filename begins with ~
#
proc ::tk::dialog::file::JoinFile {path file} {
    if {[string match {~*} $file] && [file exists $path/$file]} {
	return [file join $path ./$file]
    } else {
	return [file join $path $file]
    }
}



# Gets called when user presses the "OK" button
#
proc ::tk::dialog::file::OkCmd {w} {
    upvar ::tk::dialog::file::[winfo name $w] data

    set text [tkIconList_Get $data(icons)]
    if {[string compare $text ""]} {
	set file [::tk::dialog::file::JoinFile $data(selectPath) $text]
	if {[file isdirectory $file]} {
	    ::tk::dialog::file::ListInvoke $w $text
	    return
	}
    }

    ::tk::dialog::file::ActivateEnt $w
}

# Gets called when user presses the "Cancel" button
#
proc ::tk::dialog::file::CancelCmd {w} {
    upvar ::tk::dialog::file::[winfo name $w] data
    global tkPriv

    set tkPriv(selectFilePath) ""
}

# Gets called when user browses the IconList widget (dragging mouse, arrow
# keys, etc)
#
proc ::tk::dialog::file::ListBrowse {w text} {
    upvar ::tk::dialog::file::[winfo name $w] data

    if {[string equal $text ""]} {
	return
    }

    set file [::tk::dialog::file::JoinFile $data(selectPath) $text]
    if {![file isdirectory $file]} {
	$data(ent) delete 0 end
	$data(ent) insert 0 $text

	if { [string equal [winfo class $w] TkFDialog] } {
	    if {[string equal $data(type) open]} {
		$data(okBtn) config -text "Open"
	    } else {
		$data(okBtn) config -text "Save"
	    }
	}
    } else {
	if { [string equal [winfo class $w] TkFDialog] } {
	    $data(okBtn) config -text "Open"
	}
    }
}

# Gets called when user invokes the IconList widget (double-click, 
# Return key, etc)
#
proc ::tk::dialog::file::ListInvoke {w text} {
    upvar ::tk::dialog::file::[winfo name $w] data

    if {[string equal $text ""]} {
	return
    }

    set file [::tk::dialog::file::JoinFile $data(selectPath) $text]
    set class [winfo class $w]
    if {[string equal $class TkChooseDir] || [file isdirectory $file]} {
	set appPWD [pwd]
	if {[catch {cd $file}]} {
	    tk_messageBox -type ok -parent $w -message \
	       "Cannot change to the directory \"$file\".\nPermission denied."\
		-icon warning
	} else {
	    cd $appPWD
	    set data(selectPath) $file
	}
    } else {
	set data(selectFile) $file
	::tk::dialog::file::Done $w
    }
}

# ::tk::dialog::file::Done --
#
#	Gets called when user has input a valid filename.  Pops up a
#	dialog box to confirm selection when necessary. Sets the
#	tkPriv(selectFilePath) variable, which will break the "tkwait"
#	loop in tkFDialog and return the selected filename to the
#	script that calls tk_getOpenFile or tk_getSaveFile
#
proc ::tk::dialog::file::Done {w {selectFilePath ""}} {
    upvar ::tk::dialog::file::[winfo name $w] data
    global tkPriv

    if {[string equal $selectFilePath ""]} {
	set selectFilePath [::tk::dialog::file::JoinFile $data(selectPath) \
		$data(selectFile)]
	set tkPriv(selectFile)     $data(selectFile)
	set tkPriv(selectPath)     $data(selectPath)

	if {[file exists $selectFilePath] && [string equal $data(type) save]} {
	    set reply [tk_messageBox -icon warning -type yesno\
		    -parent $w -message "File\
		    \"$selectFilePath\" already exists.\nDo\
		    you want to overwrite it?"]
	    if {[string equal $reply "no"]} {
		return
	    }
	}
    }
    set tkPriv(selectFilePath) $selectFilePath
}
