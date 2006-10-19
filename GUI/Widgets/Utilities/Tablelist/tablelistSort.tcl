#==============================================================================
# Contains the implementation of the tablelist::sortByColumn and
# tablelist::addToSortColumns commands, as well as of the tablelist sort,
# sortbycolumn, and sortbycolumnlist subcommands.
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
    set col $result
    if {[::$win columncget $col -showlinenumbers]} {
	return ""
    }

    #
    # Determine the sort order
    #
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
    set col $result
    if {[::$win columncget $col -showlinenumbers]} {
	return ""
    }

    #
    # Update the lists of sort columns and orders
    #
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
    variable canElide
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
    # Save the keys corresponding to anchorRow and activeRow,
    # as well as the indices of the selected cells
    #
    foreach type {anchor active} {
	set ${type}Key [lindex [lindex $data(itemList) $data(${type}Row)] end]
    }
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
	    return -code error "value of the -sortcommand option is empty"
	}

	#
	# Update the sort info
	#
	for {set col 0} {$col < $data(colCount)} {incr col} {
	    set data($col-sortRank) 0
	    set data($col-sortOrder) ""
	}
	set data(sortColList) {}
	set data(arrowColList) {}
	set order [lindex $sortOrderList 0]
	set data(sortOrder) $order

	#
	# Sort the item list
	#
	set data(itemList) \
	    [lsort -$order -command $data(-sortcommand) $data(itemList)]
    } else {					;# sorting by a column (list)
	#
	# Check the specified column indices
	#
	set sortColCount2 $sortColCount
	foreach col $sortColList {
	    if {$data($col-showlinenumbers)} {
		incr sortColCount2 -1
	    }
	}
	if {$sortColCount2 == 0} {
	    return ""
	}

	#
	# Update the sort info
	#
	for {set col 0} {$col < $data(colCount)} {incr col} {
	    set data($col-sortRank) 0
	    set data($col-sortOrder) ""
	}
	set rank 1
	foreach col $sortColList order $sortOrderList {
	    if {$data($col-showlinenumbers)} {
		continue
	    }

	    set data($col-sortRank) $rank
	    set data($col-sortOrder) $order
	    incr rank
	}
	makeSortAndArrowColLists $win

	#
	# Sort the item list based on the specified columns
	#
	for {set idx [expr {$sortColCount - 1}]} {$idx >= 0} {incr idx -1} {
	    set col [lindex $sortColList $idx]
	    if {$data($col-showlinenumbers)} {
		continue
	    }

	    set order $data($col-sortOrder)
	    if {[string compare $data($col-sortmode) "command"] == 0} {
		if {![info exists data($col-sortcommand)]} {
		    return -code error "value of the -sortcommand option for\
					column $col is missing or empty"
		}

		set data(itemList) [lsort -$order -index $col \
		    -command $data($col-sortcommand) $data(itemList)]
	    } else {
		set data(itemList) [lsort -$order -index $col \
		    -$data($col-sortmode) $data(itemList)]
	    }
	}
    }

    #
    # Update the line numbers (if any)
    #
    for {set col 0} {$col < $data(colCount)} {incr col} {
	if {!$data($col-showlinenumbers)} {
	    continue
	}

	set newItemList {}
	set line 1
	foreach item $data(itemList) {
	    set item [lreplace $item $col $col $line]
	    lappend newItemList $item
	    set key [lindex $item end]
	    if {![info exists data($key-hide)]} {
		incr line
	    }
	}
	set data(itemList) $newItemList
    }

    #
    # Replace the contents of the list variable if present
    #
    condUpdateListVar $win

    #
    # Update anchorRow and activeRow
    #
    foreach type {anchor active} {
	upvar 0 ${type}Key key2
	if {[string compare $key2 ""] != 0} {
	    set data(${type}Row) [lsearch $data(itemList) "* $key2"]
	}
    }

    #
    # Cancel the execution of all delayed redisplay and redisplayCol commands
    #
    foreach name [array names data *redispId] {
	after cancel $data($name)
	unset data($name)
    }

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
    $w tag remove hiddenRow 1.0 end
    for {set line 1} {$line <= $data(itemCount)} {incr line} {
	$w delete $line.0 $line.end
    }
    set snipStr $data(-snipstring)
    set tagRefCount $data(tagRefCount)
    set isSimple [expr {$data(imgCount) == 0 && $data(winCount) == 0}]
    set hasFmtCmds [expr {[lsearch -exact $data(fmtCmdFlagList) 1] >= 0}]
    set row 0
    set line 1
    foreach item $data(itemList) {
	if {$hasFmtCmds} {
	    set formattedItem [formatItem $win [lrange $item 0 $data(lastCol)]]
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
	    set insertArgs {}
	    set multilineData {}
	    foreach text [strToDispStr $formattedItem] \
		    colFont $data(colFontList) \
		    colTags $data(colTagsList) \
		    {pixels alignment} $data(colList) {
		if {$data($col-hide) && !$canElide} {
		    incr col
		    continue
		}

		#
		# Build the list of tags to be applied to the cell
		#
		set cellFont $colFont
		set cellTags $colTags
		if {$tagRefCount != 0} {
		    set cellFont [getCellFont $win $key $col]
		    foreach opt {-background -foreground -font} {
			if {[info exists data($key,$col$opt)]} {
			    lappend cellTags cell$opt-$data($key,$col$opt)
			}
		    }
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
		    if {$data($col-maxPixels) > 0} {
			if {$data($col-reqPixels) > $data($col-maxPixels)} {
			    set pixels $data($col-maxPixels)
			}
		    }
		}
		if {$pixels != 0} {
		    incr pixels $data($col-delta)
		    if {$multiline} {
			set text [joinList $win $list $cellFont \
				  $pixels $alignment $snipStr]
		    } else {
			set text [strRange $win $text $cellFont \
				  $pixels $alignment $snipStr]
		    }
		}

		if {$multiline} {
		    lappend insertArgs "\t\t" $cellTags
		    lappend multilineData $col $text $colFont $alignment
		} else {
		    lappend insertArgs "\t$text\t" $cellTags
		}

		incr col
	    }

	    #
	    # Insert the item into the body text widget
	    #
	    if {[llength $insertArgs] != 0} {
		eval [list $w insert $line.0] $insertArgs
	    }

	    #
	    # Embed the message widgets displaying multiline elements
	    #
	    foreach {col text font alignment} $multilineData {
		findTabs $win $line $col $col tabIdx1 tabIdx2
		set msgScript [list ::tablelist::displayText $win $key \
			       $col $text $font $alignment]
		$w window create $tabIdx2 -pady 1 -create $msgScript
	    }

	} else {
	    foreach text [strToDispStr $formattedItem] \
		    colTags $data(colTagsList) \
		    {pixels alignment} $data(colList) {
		if {$data($col-hide) && !$canElide} {
		    incr col
		    continue
		}

		#
		# Build the list of tags to be applied to the cell
		#
		set cellTags $colTags
		if {$tagRefCount != 0} {
		    foreach opt {-background -foreground -font} {
			if {[info exists data($key,$col$opt)]} {
			    lappend cellTags cell$opt-$data($key,$col$opt)
			}
		    }
		}

		#
		# Insert the text and the label or window
		# (if any) into the body text widget
		#
		appendComplexElem $win $key $row $col $text $pixels \
				  $alignment $snipStr $cellTags $line

		incr col
	    }
	}

	if {$tagRefCount != 0} {
	    foreach opt {-background -foreground -font} {
		if {[info exists data($key$opt)]} {
		    $w tag add row$opt-$data($key$opt) $line.0 $line.end
		}
	    }
	}

	if {[info exists data($key-hide)]} {
	    $w tag add hiddenRow $line.0 $line.end+1c
	}

	set row $line
	incr line
    }

    #
    # Invalidate the list of the row indices indicating the non-hidden rows
    #
    set data(nonHiddenRowList) {-1}

    #
    # Select the cells that were selected before
    #
    foreach {key col} $selCells {
	set row [lsearch $data(itemList) "* $key"]
	cellselectionSubCmd $win set $row $col $row $col
    }

    #
    # Disable the body text widget if it was disabled before
    #
    if {$data(isDisabled)} {
	$w tag add disabled 1.0 end
	$w tag configure select -borderwidth 0
    }

    #
    # Bring the "most important" row into view
    #
    if {$editCol >= 0} {
	set editRow [lsearch $data(itemList) "* $editKey"]
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
    # Adjust the elided text and restore the stripes in the body text widget
    #
    adjustElidedText $win
    makeStripes $win

    #
    # Restore the edit window if it was present before
    #
    if {$editCol >= 0} {
	editcellSubCmd $win $editRow $editCol 1
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

    return ""
}
