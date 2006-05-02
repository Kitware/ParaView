#==============================================================================
# Contains the implementation of the tablelist::sortByColumn command as well as
# of the tablelist sort and sortbycolumn subcommands.
#
# Copyright (c) 2000-2006  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================

#------------------------------------------------------------------------------
# tablelist::sortByColumn
#
# Sorts the contents of the tablelist widget win by its col'th column.  Returns
# the sort order (increasing or decreasing).
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
    # Determine the sort order
    #
    set col $result
    if {[set idx [lsearch -exact [::$win sortcolumnlist] $col]] >= 0 &&
	[string compare [lindex [::$win sortorderlist] $idx] "increasing"]
	== 0} {
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
# tablelist::addToSortColumns
#
# Adds the col'th column of the tablelist widget win to the latter's list of
# sort columns and sorts the contents of the widget by the modified column
# list.  Returns the specified column's sort order (increasing or decreasing).
#------------------------------------------------------------------------------
proc tablelist::addToSortColumns {win col} {
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
    # Update the lists of sort columns and orders
    #
    set col $result
    set sortColList [::$win sortcolumnlist]
    set sortOrderList [::$win sortorderlist]
    if {[set idx [lsearch -exact $sortColList $col]] >= 0} {
	if {[string compare [lindex $sortOrderList $idx] "increasing"] == 0} {
	    set sortOrder decreasing
	} else {
	    set sortOrder increasing
	}
	set sortOrderList [lreplace $sortOrderList $idx $idx $sortOrder]
    } else {
	lappend sortColList $col
	lappend sortOrderList increasing
	set sortOrder increasing
    }

    #
    # Sort the widget's contents according to the
    # modified lists of sort columns and orders
    #
    if {[catch {::$win sortbycolumnlist $sortColList $sortOrderList} result]
	== 0} {
	event generate $win <<TablelistColumnsSorted>>
	return $sortOrder
    } else {
	return -code error $result
    }
}

#------------------------------------------------------------------------------
# tablelist::sortSubCmd
#
# This procedure is invoked to process the tablelist sort, sortbycolumn, and
# sortbycolumnlist subcommands.
#------------------------------------------------------------------------------
proc tablelist::sortSubCmd {win sortColList sortOrderList} {
    upvar ::tablelist::ns${win}::data data

    #
    # Make sure sortOrderList has the same length as sortColList
    #
    set sortColCount [llength $sortColList]
    set sortOrderCount [llength $sortOrderList]
    if {$sortOrderCount < $sortColCount} {
	for {set n $sortOrderCount} {$n < $sortColCount} {incr n} {
	    lappend sortOrderList increasing
	}
    } else {
	set sortOrderList [lrange $sortOrderList 0 [expr {$sortColCount - 1}]]
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
    # Update the sort info and sort the item list
    #
    if {[llength $sortColList] == 1 && [lindex $sortColList 0] == -1} {
	if {[string compare $data(-sortcommand) ""] == 0} {
	    return -code error \
		   "value of the -sortcommand option is empty"
	}

	for {set col 0} {$col < $data(colCount)} {incr col} {
	    set data($col-sortRank) 0
	    set data($col-sortOrder) ""
	}
	set data(sortColList) {}
	set data(arrowColList) {}
	set order [lindex $sortOrderList 0]
	set data(sortOrder) $order

	set data(itemList) \
	    [lsort -$order -command $data(-sortcommand) $data(itemList)]
    } else {					;# sorting by a column (list)
	set sortColCount [llength $sortColList]
	if {$sortColCount == 0} {
	    return ""
	}

	for {set col 0} {$col < $data(colCount)} {incr col} {
	    set data($col-sortRank) 0
	    set data($col-sortOrder) ""
	}
	set rank 1
	foreach col $sortColList order $sortOrderList {
	    set data($col-sortRank) $rank
	    set data($col-sortOrder) $order
	    incr rank
	}
	makeSortAndArrowColLists $win

	for {set idx [expr {$sortColCount - 1}]} {$idx >= 0} {incr idx -1} {
	    set col [lindex $sortColList $idx]
	    set order $data($col-sortOrder)
	    if {[string compare $data($col-sortmode) "command"] == 0} {
		if {[info exists data($col-sortcommand)]} {
		    set data(itemList) \
			[lsort -$order -index $col \
			 -command $data($col-sortcommand) $data(itemList)]
		} else {
		    return -code error \
			   "value of the -sortcommand option for\
			    column $col is missing or empty"
		}
	    } else {
		set data(itemList) \
		    [lsort -$order -index $col \
		     -$data($col-sortmode) $data(itemList)]
	    }
	}
    }

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
    # Cancel the execution of all delayed redisplay and
    # redisplayCol commands, and make sure the stretchColumns
    # procedure won't schedule any invocation of redisplayCol
    #
    foreach name [array names data *redispId] {
	after cancel $data($name)
	unset data($name)
    }
    set data(sorting) 1

    set canvasWidth $data(arrowWidth)
    if {[llength $data(arrowColList)] > 1} {
	incr canvasWidth 6
    }
    foreach col $data(arrowColList) {
	#
	# Make sure the arrow will fit into the column
	#
	set idx [expr {2*$col}]
	set pixels [lindex $data(colList) $idx]
	if {$pixels == 0 && $data($col-maxPixels) > 0 &&
	    $data($col-reqPixels) > $data($col-maxPixels) &&
	    $data($col-maxPixels) < $canvasWidth} {
	    set data($col-maxPixels) $canvasWidth
	    set data($col-maxwidth) -$canvasWidth
	}
	if {$pixels != 0 && $pixels < $canvasWidth} {
	    set data(colList) [lreplace $data(colList) $idx $idx $canvasWidth]
	    set idx [expr {3*$col}]
	    set data(-columns) \
		[lreplace $data(-columns) $idx $idx -$canvasWidth]
	}
    }

    #
    # Adjust the columns; this will also place the
    # canvas widgets into the corresponding labels
    #
    adjustColumns $win allLabels 1

    #
    # Delete the items from the body text widget and insert the sorted ones.
    # Interestingly, for a large number of items it is much more efficient
    # to empty each line individually than to invoke a global delete command.
    #
    set w $data(body)
    for {set line 1} {$line <= $data(itemCount)} {incr line} {
	$w delete $line.0 $line.end
    }
    set widgetFont $data(-font)
    set snipStr $data(-snipstring)
    set isSimple [expr {$data(tagRefCount) == 0 && $data(imgCount) == 0 &&
			$data(winCount) == 0 && !$data(hasColTags)}]
    set isViewable [winfo viewable $win]
    set hasFmtCmds [expr {[lsearch -exact $data(fmtCmdFlagList) 1] >= 0}]
    set row 0
    set line 1
    foreach item $data(itemList) {
	if {$isViewable &&
	    $row == [rowIndex $win @0,[winfo height $win] 0] + 1} {
	    updateColors $win
	    update idletasks
	}

	if {$hasFmtCmds} {
	    set formattedItem {}
	    set col 0
	    foreach text [lrange $item 0 $data(lastCol)] \
		    fmtCmdFlag $data(fmtCmdFlagList) {
		if {$fmtCmdFlag} {
		    set text [uplevel #0 $data($col-formatcommand) [list $text]]
		}
		lappend formattedItem $text
		incr col
	    }
	} else {
	    set formattedItem [lrange $item 0 $data(lastCol)]
	}

	#
	# Clip the elements if necessary and
	# insert them with the corresponding tags
	#
	set key [lindex $item end]
	set col 0
	if {$isSimple} {
	    set insertStr ""
	    set multilineData {}
	    foreach text [strToDispStr $formattedItem] \
		    {pixels alignment} $data(colList) {
		if {$data($col-hide)} {
		    incr col
		    continue
		}

		#
		# Clip the element if necessary
		#
		if {[string match "*\n*" $text]} {
		    set multiline 1
		    set list [split $text "\n"]
		} else {
		    set multiline 0
		}
		if {$pixels == 0} {		;# convention: dynamic width
		    if {$data($col-maxPixels) > 0 &&
			$data($col-reqPixels) > $data($col-maxPixels)} {
			set pixels $data($col-maxPixels)
		    }
		}
		if {$pixels != 0} {
		    incr pixels $data($col-delta)
		    if {$multiline} {
			set text [joinList $win $list $widgetFont \
				  $pixels $alignment $snipStr]
		    } else {
			set text [strRangeExt $win $text $widgetFont \
				  $pixels $alignment $snipStr]
		    }
		}

		if {$multiline} {
		    append insertStr "\t\t"
		    lappend multilineData $col $text $alignment
		} else {
		    append insertStr "\t$text\t"
		}
		incr col
	    }

	    #
	    # Insert the item into the body text widget
	    #
	    $w insert $line.0 $insertStr

	    #
	    # Embed the message widgets displaying multiline elements
	    #
	    foreach {col text alignment} $multilineData {
		findTabs $win $line $col $col tabIdx1 tabIdx2
		set msgScript [list ::tablelist::displayText $win $key \
			       $col $text $widgetFont $alignment]
		$w window create $tabIdx2 -pady 1 -create $msgScript
	    }

	} else {
	    array set itemData [array get data $key*-\[bf\]*]	;# for speed

	    set rowTags {}
	    foreach name [array names itemData $key-\[bf\]*] {
		set tail [lindex [split $name "-"] 1]
		lappend rowTags row-$tail-$itemData($name)
	    }

	    foreach text [strToDispStr $formattedItem] \
		    colFont $data(colFontList) \
		    colTags $data(colTagsList) \
		    {pixels alignment} $data(colList) {
		if {$data($col-hide)} {
		    incr col
		    continue
		}

		#
		# Adjust the cell text and the image or window width
		#
		if {[string match "*\n*" $text]} {
		    set multiline 1
		    set list [split $text "\n"]
		} else {
		    set multiline 0
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
		if {$multiline} {
		    adjustMlElem $win list auxWidth $cellFont \
				 $pixels $alignment $snipStr
		    set msgScript [list ::tablelist::displayText $win $key \
				   $col [join $list "\n"] $cellFont $alignment]
		} else {
		    adjustElem $win text auxWidth $cellFont \
			       $pixels $alignment $snipStr
		}

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
		    if {$multiline} {
			$w insert $line.end "\t\t" $tagNames
			$w window create $line.end-1c -pady 1 -create $msgScript
		    } else {
			$w insert $line.end "\t$text\t" $tagNames
		    }
		} else {
		    $w insert $line.end "\t\t" $tagNames
		    createAuxObject $win $key $row $col $aux $auxType $auxWidth
		    if {$multiline} {
			insertMlElem $w $line.end-1c $msgScript \
				     $aux $auxType $alignment
		    } else {
			insertElem $w $line.end-1c $text \
				   $aux $auxType $alignment
		    }
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
	foreach col $data(arrowColList) {
	    set canvas [list $data(hdrTxtFrCanv)$col]
	    after idle "lower $canvas; raise $canvas"
	}
    }

    set data(sorting) 0
    return ""
}
