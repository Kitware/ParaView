# clrpick.tcl --
#
#	Color selection dialog for platforms that do not support a
#	standard color selection dialog.
#
# RCS: @(#) Id
#
# Copyright (c) 1996 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#
# ToDo:
#
#	(1): Find out how many free colors are left in the colormap and
#	     don't allocate too many colors.
#	(2): Implement HSV color selection. 
#

# tkColorDialog --
#
#	Create a color dialog and let the user choose a color. This function
#	should not be called directly. It is called by the tk_chooseColor
#	function when a native color selector widget does not exist
#
proc tkColorDialog {args} {
    global tkPriv
    set w .__tk__color
    upvar #0 $w data

    # The lines variables track the start and end indices of the line
    # elements in the colorbar canvases.
    set data(lines,red,start)   0
    set data(lines,red,last)   -1
    set data(lines,green,start) 0
    set data(lines,green,last) -1
    set data(lines,blue,start)  0
    set data(lines,blue,last)  -1

    # This is the actual number of lines that are drawn in each color strip.
    # Note that the bars may be of any width.
    # However, NUM_COLORBARS must be a number that evenly divides 256.
    # Such as 256, 128, 64, etc.
    set data(NUM_COLORBARS) 8

    # BARS_WIDTH is the number of pixels wide the color bar portion of the
    # canvas is. This number must be a multiple of NUM_COLORBARS
    set data(BARS_WIDTH) 128

    # PLGN_WIDTH is the number of pixels wide of the triangular selection
    # polygon. This also results in the definition of the padding on the 
    # left and right sides which is half of PLGN_WIDTH. Make this number even.
    set data(PLGN_HEIGHT) 10

    # PLGN_HEIGHT is the height of the selection polygon and the height of the 
    # selection rectangle at the bottom of the color bar. No restrictions.
    set data(PLGN_WIDTH) 10

    tkColorDialog_Config $w $args
    tkColorDialog_InitValues $w

    if {![winfo exists $w]} {
	toplevel $w -class tkColorDialog
	tkColorDialog_BuildDialog $w
    }
    wm transient $w $data(-parent)


    # 5. Withdraw the window, then update all the geometry information
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
    wm title $w $data(-title)

    # 6. Set a grab and claim the focus too.

    set oldFocus [focus]
    set oldGrab [grab current $w]
    if {[string compare $oldGrab ""]} {
	set grabStatus [grab status $oldGrab]
    }
    grab $w
    focus $data(okBtn)

    # 7. Wait for the user to respond, then restore the focus and
    # return the index of the selected button.  Restore the focus
    # before deleting the window, since otherwise the window manager
    # may take the focus away so we can't redirect it.  Finally,
    # restore any grab that was in effect.

    tkwait variable tkPriv(selectColor)
    catch {focus $oldFocus}
    grab release $w
    destroy $w
    unset data
    if {[string compare $oldGrab ""]} {
	if {![string compare $grabStatus "global"]} {
	    grab -global $oldGrab
	} else {
	    grab $oldGrab
	}
    }
    return $tkPriv(selectColor)
}

# tkColorDialog_InitValues --
#
#	Get called during initialization or when user resets NUM_COLORBARS
#
proc tkColorDialog_InitValues {w} {
    upvar #0 $w data

    # IntensityIncr is the difference in color intensity between a colorbar
    # and its neighbors.
    set data(intensityIncr) [expr {256 / $data(NUM_COLORBARS)}]

    # ColorbarWidth is the width of each colorbar
    set data(colorbarWidth) \
	    [expr {$data(BARS_WIDTH) / $data(NUM_COLORBARS)}]

    # Indent is the width of the space at the left and right side of the
    # colorbar. It is always half the selector polygon width, because the
    # polygon extends into the space.
    set data(indent) [expr {$data(PLGN_WIDTH) / 2}]

    set data(colorPad) 2
    set data(selPad)   [expr {$data(PLGN_WIDTH) / 2}]

    #
    # minX is the x coordinate of the first colorbar
    #
    set data(minX) $data(indent)

    #
    # maxX is the x coordinate of the last colorbar
    #
    set data(maxX) [expr {$data(BARS_WIDTH) + $data(indent)-1}]

    #
    # canvasWidth is the width of the entire canvas, including the indents
    #
    set data(canvasWidth) [expr {$data(BARS_WIDTH) + \
	    $data(PLGN_WIDTH)}]

    # Set the initial color, specified by -initialcolor, or the
    # color chosen by the user the last time.
    set data(selection) $data(-initialcolor)
    set data(finalColor)  $data(-initialcolor)
    set rgb [winfo rgb . $data(selection)]

    set data(red,intensity)   [expr {[lindex $rgb 0]/0x100}]
    set data(green,intensity) [expr {[lindex $rgb 1]/0x100}]
    set data(blue,intensity)  [expr {[lindex $rgb 2]/0x100}]
}

# tkColorDialog_Config  --
#
#	Parses the command line arguments to tk_chooseColor
#
proc tkColorDialog_Config {w argList} {
    global tkPriv
    upvar #0 $w data

    # 1: the configuration specs
    #
    set specs {
	{-initialcolor "" "" ""}
	{-parent "" "" "."}
	{-title "" "" "Color"}
    }

    # 2: parse the arguments
    #
    tclParseConfigSpec $w $specs "" $argList

    if {![string compare $data(-title) ""]} {
	set data(-title) " "
    }
    if {![string compare $data(-initialcolor) ""]} {
	if {[info exists tkPriv(selectColor)] && \
		[string compare $tkPriv(selectColor) ""]} {
	    set data(-initialcolor) $tkPriv(selectColor)
	} else {
	    set data(-initialcolor) [. cget -background]
	}
    } else {
	if {[catch {winfo rgb . $data(-initialcolor)} err]} {
	    error $err
	}
    }

    if {![winfo exists $data(-parent)]} {
	error "bad window path name \"$data(-parent)\""
    }
}

# tkColorDialog_BuildDialog --
#
#	Build the dialog.
#
proc tkColorDialog_BuildDialog {w} {
    upvar #0 $w data

    # TopFrame contains the color strips and the color selection
    #
    set topFrame [frame $w.top -relief raised -bd 1]

    # StripsFrame contains the colorstrips and the individual RGB entries
    set stripsFrame [frame $topFrame.colorStrip]

    foreach c { Red Green Blue } {
	set color [string tolower $c]

	# each f frame contains an [R|G|B] entry and the equiv. color strip.
	set f [frame $stripsFrame.$color]

	# The box frame contains the label and entry widget for an [R|G|B]
	set box [frame $f.box]

	label $box.label -text $c: -width 6 -under 0 -anchor ne
	entry $box.entry -textvariable [format %s $w]($color,intensity) \
	    -width 4
	pack $box.label -side left -fill y -padx 2 -pady 3
	pack $box.entry -side left -anchor n -pady 0
	pack $box -side left -fill both

	set height [expr \
	    {[winfo reqheight $box.entry] - \
	    2*([$box.entry cget -highlightthickness] + [$box.entry cget -bd])}]

	canvas $f.color -height $height\
	    -width $data(BARS_WIDTH) -relief sunken -bd 2
	canvas $f.sel -height $data(PLGN_HEIGHT) \
	    -width $data(canvasWidth) -highlightthickness 0
	pack $f.color -expand yes -fill both
	pack $f.sel -expand yes -fill both

	pack $f -side top -fill x -padx 0 -pady 2

	set data($color,entry) $box.entry
	set data($color,col) $f.color
	set data($color,sel) $f.sel

	bind $data($color,col) <Configure> \
	    "tkColorDialog_DrawColorScale $w $color 1"
	bind $data($color,col) <Enter> \
	    "tkColorDialog_EnterColorBar $w $color"
	bind $data($color,col) <Leave> \
	    "tkColorDialog_LeaveColorBar $w $color"

	bind $data($color,sel) <Enter> \
	    "tkColorDialog_EnterColorBar $w $color"
	bind $data($color,sel) <Leave> \
	    "tkColorDialog_LeaveColorBar $w $color"
	
	bind $box.entry <Return> "tkColorDialog_HandleRGBEntry $w"
    }

    pack $stripsFrame -side left -fill both -padx 4 -pady 10

    # The selFrame contains a frame that demonstrates the currently
    # selected color
    #
    set selFrame [frame $topFrame.sel]
    set lab [label $selFrame.lab -text "Selection:" -under 0 -anchor sw]
    set ent [entry $selFrame.ent -textvariable [format %s $w](selection) \
	-width 16]
    set f1  [frame $selFrame.f1 -relief sunken -bd 2]
    set data(finalCanvas) [frame $f1.demo -bd 0 -width 100 -height 70]

    pack $lab $ent -side top -fill x -padx 4 -pady 2
    pack $f1 -expand yes -anchor nw -fill both -padx 6 -pady 10
    pack $data(finalCanvas) -expand yes -fill both

    bind $ent <Return> "tkColorDialog_HandleSelEntry $w"

    pack $selFrame -side left -fill none -anchor nw
    pack $topFrame -side top -expand yes -fill both -anchor nw

    # the botFrame frame contains the buttons
    #
    set botFrame [frame $w.bot -relief raised -bd 1]
    button $botFrame.ok     -text OK            -width 8 -under 0 \
	-command "tkColorDialog_OkCmd $w"
    button $botFrame.cancel -text Cancel        -width 8 -under 0 \
	-command "tkColorDialog_CancelCmd $w"

    set data(okBtn)      $botFrame.ok
    set data(cancelBtn)  $botFrame.cancel
 
    pack $botFrame.ok $botFrame.cancel \
	-padx 10 -pady 10 -expand yes -side left
    pack $botFrame -side bottom -fill x


    # Accelerator bindings

    bind $w <Alt-r> "focus $data(red,entry)"
    bind $w <Alt-g> "focus $data(green,entry)"
    bind $w <Alt-b> "focus $data(blue,entry)"
    bind $w <Alt-s> "focus $ent"
    bind $w <KeyPress-Escape> "tkButtonInvoke $data(cancelBtn)"
    bind $w <Alt-c> "tkButtonInvoke $data(cancelBtn)"
    bind $w <Alt-o> "tkButtonInvoke $data(okBtn)"

    wm protocol $w WM_DELETE_WINDOW "tkColorDialog_CancelCmd $w"
}

# tkColorDialog_SetRGBValue --
#
#	Sets the current selection of the dialog box
#
proc tkColorDialog_SetRGBValue {w color} {
    upvar #0 $w data 

    set data(red,intensity)   [lindex $color 0]
    set data(green,intensity) [lindex $color 1]
    set data(blue,intensity)  [lindex $color 2]
    
    tkColorDialog_RedrawColorBars $w all

    # Now compute the new x value of each colorbars pointer polygon
    foreach color { red green blue } {
	set x [tkColorDialog_RgbToX $w $data($color,intensity)]
	tkColorDialog_MoveSelector $w $data($color,sel) $color $x 0
    }
}

# tkColorDialog_XToRgb --
#
#	Converts a screen coordinate to intensity
#
proc tkColorDialog_XToRgb {w x} {
    upvar #0 $w data
    
    return [expr {($x * $data(intensityIncr))/ $data(colorbarWidth)}]
}

# tkColorDialog_RgbToX
#
#	Converts an intensity to screen coordinate.
#
proc tkColorDialog_RgbToX {w color} {
    upvar #0 $w data
    
    return [expr {($color * $data(colorbarWidth)/ $data(intensityIncr))}]
}


# tkColorDialog_DrawColorScale --
# 
#	Draw color scale is called whenever the size of one of the color
#	scale canvases is changed.
#
proc tkColorDialog_DrawColorScale {w c {create 0}} {
    global lines
    upvar #0 $w data

    # col: color bar canvas
    # sel: selector canvas
    set col $data($c,col)
    set sel $data($c,sel)

    # First handle the case that we are creating everything for the first time.
    if {$create} {
	# First remove all the lines that already exist.
	if { $data(lines,$c,last) > $data(lines,$c,start)} {
	    for {set i $data(lines,$c,start)} \
		{$i <= $data(lines,$c,last)} { incr i} {
		$sel delete $i
	    }
	}
	# Delete the selector if it exists
	if {[info exists data($c,index)]} {
	    $sel delete $data($c,index)
	}
	
	# Draw the selection polygons
	tkColorDialog_CreateSelector $w $sel $c
	$sel bind $data($c,index) <ButtonPress-1> \
	    "tkColorDialog_StartMove $w $sel $c %x $data(selPad) 1"
	$sel bind $data($c,index) <B1-Motion> \
	    "tkColorDialog_MoveSelector $w $sel $c %x $data(selPad)"
	$sel bind $data($c,index) <ButtonRelease-1> \
	    "tkColorDialog_ReleaseMouse $w $sel $c %x $data(selPad)"

	set height [winfo height $col]
	# Create an invisible region under the colorstrip to catch mouse clicks
	# that aren't on the selector.
	set data($c,clickRegion) [$sel create rectangle 0 0 \
	    $data(canvasWidth) $height -fill {} -outline {}]

	bind $col <ButtonPress-1> \
	    "tkColorDialog_StartMove $w $sel $c %x $data(colorPad)"
	bind $col <B1-Motion> \
	    "tkColorDialog_MoveSelector $w $sel $c %x $data(colorPad)"
	bind $col <ButtonRelease-1> \
	    "tkColorDialog_ReleaseMouse $w $sel $c %x $data(colorPad)"

	$sel bind $data($c,clickRegion) <ButtonPress-1> \
	    "tkColorDialog_StartMove $w $sel $c %x $data(selPad)"
	$sel bind $data($c,clickRegion) <B1-Motion> \
	    "tkColorDialog_MoveSelector $w $sel $c %x $data(selPad)"
	$sel bind $data($c,clickRegion) <ButtonRelease-1> \
	    "tkColorDialog_ReleaseMouse $w $sel $c %x $data(selPad)"
    } else {
	# l is the canvas index of the first colorbar.
	set l $data(lines,$c,start)
    }
    
    # Draw the color bars.
    set highlightW [expr \
	    {[$col cget -highlightthickness] + [$col cget -bd]}]
    for {set i 0} { $i < $data(NUM_COLORBARS)} { incr i} {
	set intensity [expr {$i * $data(intensityIncr)}]
	set startx [expr {$i * $data(colorbarWidth) + $highlightW}]
	if { $c == "red" } {
	    set color [format "#%02x%02x%02x" \
			   $intensity \
			   $data(green,intensity) \
			   $data(blue,intensity)]
	} elseif { $c == "green" } {
	    set color [format "#%02x%02x%02x" \
			   $data(red,intensity) \
			   $intensity \
			   $data(blue,intensity)]
	} else {
	    set color [format "#%02x%02x%02x" \
			   $data(red,intensity) \
			   $data(green,intensity) \
			   $intensity]
	}

	if {$create} {
	    set index [$col create rect $startx $highlightW \
		    [expr {$startx +$data(colorbarWidth)}] \
		    [expr {[winfo height $col] + $highlightW}]\
	        -fill $color -outline $color]
	} else {
	    $col itemconfigure $l -fill $color -outline $color
	    incr l
	}
    }
    $sel raise $data($c,index)

    if {$create} {
	set data(lines,$c,last) $index
	set data(lines,$c,start) [expr {$index - $data(NUM_COLORBARS) + 1}]
    }

    tkColorDialog_RedrawFinalColor $w
}

# tkColorDialog_CreateSelector --
#
#	Creates and draws the selector polygon at the position
#	$data($c,intensity).
#
proc tkColorDialog_CreateSelector {w sel c } {
    upvar #0 $w data
    set data($c,index) [$sel create polygon \
	0 $data(PLGN_HEIGHT) \
	$data(PLGN_WIDTH) $data(PLGN_HEIGHT) \
	$data(indent) 0]
    set data($c,x) [tkColorDialog_RgbToX $w $data($c,intensity)]
    $sel move $data($c,index) $data($c,x) 0
}

# tkColorDialog_RedrawFinalColor
#
#	Combines the intensities of the three colors into the final color
#
proc tkColorDialog_RedrawFinalColor {w} {
    upvar #0 $w data

    set color [format "#%02x%02x%02x" $data(red,intensity) \
	$data(green,intensity) $data(blue,intensity)]
    
    $data(finalCanvas) configure -bg $color
    set data(finalColor) $color
    set data(selection) $color
    set data(finalRGB) [list \
	$data(red,intensity) \
	$data(green,intensity) \
	$data(blue,intensity)]
}

# tkColorDialog_RedrawColorBars --
#
# Only redraws the colors on the color strips that were not manipulated.
# Params: color of colorstrip that changed. If color is not [red|green|blue]
#         Then all colorstrips will be updated
#
proc tkColorDialog_RedrawColorBars {w colorChanged} {
    upvar #0 $w data

    switch $colorChanged {
	red { 
	    tkColorDialog_DrawColorScale $w green
	    tkColorDialog_DrawColorScale $w blue
	}
	green {
	    tkColorDialog_DrawColorScale $w red
	    tkColorDialog_DrawColorScale $w blue
	}
	blue {
	    tkColorDialog_DrawColorScale $w red
	    tkColorDialog_DrawColorScale $w green
	}
	default {
	    tkColorDialog_DrawColorScale $w red
	    tkColorDialog_DrawColorScale $w green
	    tkColorDialog_DrawColorScale $w blue
	}
    }
    tkColorDialog_RedrawFinalColor $w
}

#----------------------------------------------------------------------
#			Event handlers
#----------------------------------------------------------------------

# tkColorDialog_StartMove --
#
#	Handles a mousedown button event over the selector polygon.
#	Adds the bindings for moving the mouse while the button is
#	pressed.  Sets the binding for the button-release event.
# 
# Params: sel is the selector canvas window, color is the color of the strip.
#
proc tkColorDialog_StartMove {w sel color x delta {dontMove 0}} {
    upvar #0 $w data

    if {!$dontMove} {
	tkColorDialog_MoveSelector $w $sel $color $x $delta
    }
}

# tkColorDialog_MoveSelector --
# 
# Moves the polygon selector so that its middle point has the same
# x value as the specified x. If x is outside the bounds [0,255],
# the selector is set to the closest endpoint.
#
# Params: sel is the selector canvas, c is [red|green|blue]
#         x is a x-coordinate.
#
proc tkColorDialog_MoveSelector {w sel color x delta} {
    upvar #0 $w data

    incr x -$delta

    if { $x < 0 } {
	set x 0
    } elseif { $x >= $data(BARS_WIDTH)} {
	set x [expr {$data(BARS_WIDTH) - 1}]
    }
    set diff [expr {$x - $data($color,x)}]
    $sel move $data($color,index) $diff 0
    set data($color,x) [expr {$data($color,x) + $diff}]
    
    # Return the x value that it was actually set at
    return $x
}

# tkColorDialog_ReleaseMouse
#
# Removes mouse tracking bindings, updates the colorbars.
#
# Params: sel is the selector canvas, color is the color of the strip,
#         x is the x-coord of the mouse.
#
proc tkColorDialog_ReleaseMouse {w sel color x delta} {
    upvar #0 $w data 

    set x [tkColorDialog_MoveSelector $w $sel $color $x $delta]
    
    # Determine exactly what color we are looking at.
    set data($color,intensity) [tkColorDialog_XToRgb $w $x]

    tkColorDialog_RedrawColorBars $w $color
}

# tkColorDialog_ResizeColorbars --
#
#	Completely redraws the colorbars, including resizing the
#	colorstrips
#
proc tkColorDialog_ResizeColorBars {w} {
    upvar #0 $w data
    
    if { ($data(BARS_WIDTH) < $data(NUM_COLORBARS)) || 
	 (($data(BARS_WIDTH) % $data(NUM_COLORBARS)) != 0)} {
	set data(BARS_WIDTH) $data(NUM_COLORBARS)
    }
    tkColorDialog_InitValues $w
    foreach color { red green blue } {
	$data($color,col) configure -width $data(canvasWidth)
	tkColorDialog_DrawColorScale $w $color 1
    }
}

# tkColorDialog_HandleSelEntry --
#
#	Handles the return keypress event in the "Selection:" entry
#
proc tkColorDialog_HandleSelEntry {w} {
    upvar #0 $w data

    set text [string trim $data(selection)]
    # Check to make sure that the color is valid
    if {[catch {set color [winfo rgb . $text]} ]} {
	set data(selection) $data(finalColor)
	return
    }
    
    set R [expr {[lindex $color 0]/0x100}]
    set G [expr {[lindex $color 1]/0x100}]
    set B [expr {[lindex $color 2]/0x100}]

    tkColorDialog_SetRGBValue $w "$R $G $B"
    set data(selection) $text
}

# tkColorDialog_HandleRGBEntry --
#
#	Handles the return keypress event in the R, G or B entry
#
proc tkColorDialog_HandleRGBEntry {w} {
    upvar #0 $w data

    foreach c {red green blue} {
	if {[catch {
	    set data($c,intensity) [expr {int($data($c,intensity))}]
	}]} {
	    set data($c,intensity) 0
	}

	if {$data($c,intensity) < 0} {
	    set data($c,intensity) 0
	}
	if {$data($c,intensity) > 255} {
	    set data($c,intensity) 255
	}
    }

    tkColorDialog_SetRGBValue $w "$data(red,intensity) $data(green,intensity) \
	$data(blue,intensity)"
}    

# mouse cursor enters a color bar
#
proc tkColorDialog_EnterColorBar {w color} {
    upvar #0 $w data

    $data($color,sel) itemconfig $data($color,index) -fill red
}

# mouse leaves enters a color bar
#
proc tkColorDialog_LeaveColorBar {w color} {
    upvar #0 $w data

    $data($color,sel) itemconfig $data($color,index) -fill black
}

# user hits OK button
#
proc tkColorDialog_OkCmd {w} {
    global tkPriv
    upvar #0 $w data

    set tkPriv(selectColor) $data(finalColor)
}

# user hits Cancel button
#
proc tkColorDialog_CancelCmd {w} {
    global tkPriv

    set tkPriv(selectColor) ""
}

