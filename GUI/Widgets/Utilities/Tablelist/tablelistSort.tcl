#==============================================================================
# Contains the implementation of the tablelist::sortByColumn command as well as
# of the tablelist sort and sortbycolumn subcommands.
#
# Copyright (c) 2000-2005  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================

#------------------------------------------------------------------------------
# tablelist::sortByColumn
#
# Sorts the contents of the tablelist widget win by its col'th column.  Returns
# the sorting order (increasing or decreasing).
#------------------------------------------------------------------------------
proc tablelist::sortByColumn {win col} {
    #
    # Check the arguments
    #
    if {![winfo exists $win]} {
	return -code error "bad window path name \"$win\""
    }
    if {[string compare [winfo class $win] "Tablelist"] != 0} {
	return -code error "window \"$win\" is not a tablelist widget"
    }
    if {[catch {::$win columnindex $col} result] != 0} {
	return -code error $result
    }
    if {$result < 0 || $result >= [::$win columncount]} {
	return -code error "column index \"$col\" out of range"
    }

    #
    # Determine the sorting order
    #
    set col $result
    if {$col == [::$win sortcolumn] &&
	[string compare [::$win sortorder] "increasing"] == 0} {
	set sortOrder decreasing
    } else {
	set sortOrder increasing
    }

    #
    # Sort the widget's contents based on the given column
    #
    if {[catch {::$win sortbycolumn $col -$sortOrder} result] == 0} {
	event generate $win <<TablelistColumnSorted>>
	return $sortOrder
    } else {
	return -code error $result
    }
}

#------------------------------------------------------------------------------
# tablelist::sortSubCmd
#
# This procedure is invoked to process the tablelist sort and sortbycolumn
# subcommands.
#------------------------------------------------------------------------------
proc tablelist::sortSubCmd {win col order} {
    upvar ::tablelist::ns${win}::data data

    set data(sorting) 1

    #
    # Cancel the execution of all delayed redisplay and redisplayCol commands
    #
    foreach name [array names data *redispId] {
	after cancel $data($name)
	unset data($name)
    }

    #
    # Save the keys corresponding to anchorRow and activeRow 
    #
    foreach type {anchor active} {
	set item [lindex $data(itemList) $data(${type}Row)]
	set ${type}Key [lindex $item end]
    }

    #
    # Save the indices of the selected cells
    #
    set selCells [curcellselectionSubCmd $win 1]

    #
    # Save some data of the edit window if present
    #
    if {[set editCol $data(editCol)] >= 0} {
	set editKey $data(editKey)
	saveEditData $win
    }

    #
    # Sort the item list and update the sort info
    #
    if {$col < 0} {				;# not sorting by a column
	if {[string compare $data(-sortcommand) ""] == 0} {
	    return -code error \
		   "value of the -sortcommand option is empty"
	}

	set data(itemList) \
	    [lsort $order -command $data(-sortcommand) $data(itemList)]
    } else {					;# sorting by a column
	if {[string compare $data($col-sortmode) "command"] == 0} {
	    if {[info exists data($col-sortcommand)]} {
		set data(itemList) \
		    [lsort $order -index $col \
		     -command $data($col-sortcommand) $data(itemList)]
	    } else {
		return -code error \
		       "value of the -sortcommand option for\
			column $col is missing or empty"
	    }
	} else {
	    set data(itemList) \
		[lsort $order -index $col \
		 -$data($col-sortmode) $data(itemList)]
	}
    }
    set data(sortCol) $col
    set data(sortOrder) [string range $order 1 end]

    #
    # Replace the contents of the list variable if present
    #
    if {$data(hasListVar)} {
	upvar #0 $data(-listvariable) var
	trace vdelete var wu $data(listVarTraceCmd)
	set var {}
	foreach item $data(itemList) {
	    lappend var [lrange $item 0 $data(lastCol)]
	}
	trace variable var wu $data(listVarTraceCmd)
    }

    #
    # Update anchorRow and activeRow
    #
    foreach type {anchor active} {
	upvar 0 ${type}Key key
	if {[string compare $key ""] != 0} {
	    set data(${type}Row) [lsearch $data(itemList) "* $key"]
	}
    }

    #
    # Check whether an up- or down-arrow is to be displayed
    #
    set oldArrowCol $data(arrowCol)
    if {$col >= 0 && $data(-showarrow) && $data($col-showarrow)} {
	#
	# Configure the canvas and draw the arrows
	#
	set data(arrowCol) $col
	configCanvas $win
	drawArrows $win

	#
	# Make sure the arrow will fit into the column
	#
	set idx [expr {2*$col}]
	set pixels [lindex $data(colList) $idx]
	if {$pixels == 0 && $data($col-maxPixels) > 0 &&
	    $data($col-reqPixels) > $data($col-maxPixels) &&
	    $data($col-maxPixels) < $data(arrowWidth)} {
	    set data($col-maxPixels) $data(arrowWidth)
	    set data($col-maxwidth) -$data(arrowWidth)
	}
	if {$pixels != 0 && $pixels < $data(arrowWidth)} {
	    set data(colList) \
		[lreplace $data(colList) $idx $idx $data(arrowWidth)]
	    set idx [expr {3*$col}]
	    set data(-columns) \
		[lreplace $data(-columns) $idx $idx -$data(arrowWidth)]
	}

	#
	# Adjust the columns; this will also place the canvas into the label
	#
	adjustColumns $win [list l$oldArrowCol l$col] 1
    } else {
	#
	# Unmanage the canvas and adjust the columns
	#
	place forget $data(hdrTxtFrCanv)
	set data(arrowCol) -1
	adjustColumns $win l$oldArrowCol 1
    }

    #
    # Delete the items from the body text widget and insert the sorted ones.
    # Interestingly, for a large number of items it is much more efficient
    # to empty each line individually than to invoke a global delete command.
    #
    set widgetFont $data(-font)
    set snipStr $data(-snipstring)
    set isSimple [expr {$data(tagRefCount) == 0 && $data(imgCount) == 0 &&
			$data(winCount) == 0 && !$data(hasColTags)}]
    set isViewable [winfo viewable $win]
    set w $data(body)
    set row 0
    set line 1
    foreach item $data(itemList) {
	if {$isViewable &&
	    $row == [expr {[rowIndex $win @0,[winfo height $win] 0] + 1}]} {
	    update idletasks
	}

	#
	# Empty the line, clip the elements if necessary,
	# and insert them with the corresponding tags
	#
	$w delete $line.0 $line.end
	set dispItem [strToDispStr $item]
	set col 0
	if {$isSimple} {
	    set insertStr ""
	    foreach text [lrange $dispItem 0 $data(lastCol)] \
		    fmtCmdFlag $data(fmtCmdFlagList) \
		    {pixels alignment} $data(colList) {
		if {$data($col-hide)} {
		    incr col
		    continue
		}

		#
		# Clip the element if necessary
		#
		if {$fmtCmdFlag} {
		    set text [uplevel #0 $data($col-formatcommand) \
			      [list [lindex $item $col]]]
		    set text [strToDispStr $text]
		}
		if {$pixels == 0} {		;# convention: dynamic width
		    if {$data($col-maxPixels) > 0 &&
			$data($col-reqPixels) > $data($col-maxPixels)} {
			set pixels $data($col-maxPixels)
		    }
		}
		if {$pixels != 0} {
		    incr pixels $data($col-delta)
		    set text [strRangeExt $win $text $widgetFont \
			      $pixels $alignment $snipStr]
		}

		append insertStr "\t$text\t"
		incr col
	    }

	    #
	    # Insert the item into the body text widget
	    #
	    $w insert $line.0 $insertStr

	} else {
	    set key [lindex $item end]
	    array set itemData [array get data $key*-\[bf\]*]	;# for speed

	    set rowTags {}
	    foreach name [array names itemData $key-\[bf\]*] {
		set tail [lindex [split $name "-"] 1]
		lappend rowTags row-$tail-$itemData($name)
	    }

	    foreach text [lrange $dispItem 0 $data(lastCol)] \
		    colFont $data(colFontList) \
		    colTags $data(colTagsList) \
		    fmtCmdFlag $data(fmtCmdFlagList) \
		    {pixels alignment} $data(colList) {
		if {$data($col-hide)} {
		    incr col
		    continue
		}

		#
		# Adjust the cell text and the image or window width
		#
		if {$fmtCmdFlag} {
		    set text [uplevel #0 $data($col-formatcommand) \
			      [list [lindex $item $col]]]
		    set text [strToDispStr $text]
		}
		set aux [getAuxData $win $key $col auxType auxWidth]
		if {[info exists data($key-$col-font)]} {
		    set cellFont $data($key-$col-font)
		} elseif {[info exists data($key-font)]} {
		    set cellFont $data($key-font)
		} else {
		    set cellFont $colFont
		}
		if {$pixels == 0} {		;# convention: dynamic width
		    if {$data($col-maxPixels) > 0 &&
			$data($col-reqPixels) > $data($col-maxPixels)} {
			set pixels $data($col-maxPixels)
		    }
		}
		if {$pixels != 0} {
		    incr pixels $data($col-delta)
		}
		adjustElem $win text auxWidth $cellFont $pixels \
			   $alignment $snipStr

		#
		# Insert the text and the auxiliary object
		#
		set cellTags {}
		foreach name [array names itemData $key-$col-\[bf\]*] {
		    set tail [lindex [split $name "-"] 2]
		    lappend cellTags cell-$tail-$itemData($name)
		}
		set tagNames [concat $colTags $rowTags $cellTags]
		if {$auxType == 0} {
		    $w insert $line.end "\t$text\t" $tagNames
		} else {
		    $w insert $line.end "\t\t" $tagNames
		    createAuxObject $win $key $row $col $aux $auxType $auxWidth
		    insertElem $w $line.end-1c $text $aux $auxType $alignment
		}

		incr col
	    }

	    unset itemData
	}

	incr row
	incr line
    }

    #
    # Restore the stripes in the body text widget
    #
    makeStripes $win

    #
    # Select the cells that were selected before
    #
    foreach {key col} $selCells {
	set row [lsearch $data(itemList) "* $key"]
	cellselectionSubCmd $win set $row $col $row $col
    }

    #
    # Restore the edit window if it was present before
    #
    if {$editCol >= 0} {
	set editRow [lsearch $data(itemList) "* $editKey"]
	editcellSubCmd $win $editRow $editCol 1
    }

    #
    # Disable the body text widget if it was disabled before
    #
    if {$data(isDisabled)} {
	$w tag add disabled 1.0 end
	$w tag configure select -borderwidth 0
    }

    #
    # Adjust the elided text
    #
    adjustElidedTextWhenIdle $win

    #
    # Bring the "most important" row into view
    #
    if {$editCol >= 0} {
	seeSubCmd $win $editRow
    } else {
	set selRows [curselectionSubCmd $win]
	if {[llength $selRows] == 1} {
	    seeSubCmd $win $selRows
	} elseif {[string compare [focus -lastfor $w] $w] == 0} {
	    seeSubCmd $win $data(activeRow)
	}
    }

    #
    # Work around a Tk bug on Mac OS X Aqua
    #
    variable winSys
    if {[string compare $winSys "aqua"] == 0} {
	set canvas [list $data(hdrTxtFrCanv)]
	after idle "lower $canvas; raise $canvas"
    }

    set data(sorting) 0
    return ""
}
