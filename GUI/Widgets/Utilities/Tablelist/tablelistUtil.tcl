#==============================================================================
# Contains private utility procedures for tablelist widgets.
#
# Structure of the module:
#   - Namespace initialization
#   - Private utility procedures
#
# Copyright (c) 2000-2006  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================

#
# Namespace initialization
# ========================
#

namespace eval tablelist {
    #
    # Alignment -> anchor mapping
    #
    variable anchors
    array set anchors {
	left	w
	right	e
	center	center
    }

    #
    # <incrArrowType, sortOrder> -> direction mapping
    #
    variable directions
    array set directions {
	up,increasing	Up
	up,decreasing	Dn
	down,increasing	Dn
	down,decreasing	Up
    }

    variable auxWinClasses
    array set auxWinClasses {
	0	""
	1	Label
	2	Frame
    }
}

#
# Private utility procedures
# ==========================
#

#------------------------------------------------------------------------------
# tablelist::rowIndex
#
# Checks the row index idx and returns either its numerical value or an error.
# endIsSize must be a boolean value: if true, end refers to the number of items
# in the tablelist, i.e., to the element just after the last one; if false, end
# refers to 1 less than the number of items, i.e., to the last element in the
# tablelist.
#------------------------------------------------------------------------------
proc tablelist::rowIndex {win idx endIsSize} {
    upvar ::tablelist::ns${win}::data data

    set idxLen [string length $idx]
    if {[string first $idx "active"] == 0 && $idxLen >= 2} {
	return $data(activeRow)
    } elseif {[string first $idx "anchor"] == 0 && $idxLen >= 2} {
	return $data(anchorRow)
    } elseif {[string first $idx "end"] == 0} {
	if {$endIsSize} {
	    return $data(itemCount)
	} else {
	    return $data(lastRow)
	}
    } elseif {[string compare [string index $idx 0] "@"] == 0 &&
	      [catch {$data(body) index $idx}] == 0} {
	scan $idx "@%d,%d" x y
	incr x -[winfo x $data(body)]
	incr y -[winfo y $data(body)]
	set textIdx [$data(body) index @$x,$y]
	return [expr {int($textIdx) - 1}]
    } elseif {[string compare [string index $idx 0] "k"] == 0 &&
	      [set index [lsearch $data(itemList) "* $idx"]] >= 0} {
	return $index
    } elseif {[catch {format "%d" $idx} index] == 0} {
	return $index
    } else {
	for {set row 0} {$row < $data(itemCount)} {incr row} {
	    set key [lindex [lindex $data(itemList) $row] end]
	    set hasName [info exists data($key-name)]
	    if {$hasName && [string compare $idx $data($key-name)] == 0 ||
		!$hasName && [string compare $idx ""] == 0} {
		return $row
	    }
	}
	return -code error \
	       "bad row index \"$idx\": must be active, anchor,\
	        end, @x,y, a number, a full key, or a name"
    }
}

#------------------------------------------------------------------------------
# tablelist::colIndex
#
# Checks the column index idx and returns either its numerical value or an
# error.  checkRange must be a boolean value: if true, it is additionally
# checked whether the numerical value corresponding to idx is within the
# allowed range.
#------------------------------------------------------------------------------
proc tablelist::colIndex {win idx checkRange} {
    upvar ::tablelist::ns${win}::data data

    set idxLen [string length $idx]
    if {[string first $idx "active"] == 0 && $idxLen >= 2} {
	set index $data(activeCol)
    } elseif {[string first $idx "anchor"] == 0 && $idxLen >= 2} {
	set index $data(anchorCol)
    } elseif {[string first $idx "end"] == 0} {
	set index $data(lastCol)
    } elseif {[string compare [string index $idx 0] "@"] == 0 &&
	      [catch {$data(body) index $idx}] == 0} {
	scan $idx "@%d" x
	incr x -[winfo x $data(body)]
	set bodyWidth [winfo width $data(body)]
	if {$x >= $bodyWidth} {
	    set x [expr {$bodyWidth - 1}]
	} elseif {$x < 0} {
	    set x 0
	}
	set x [expr {$x + [winfo rootx $data(body)]}]

	set lastVisibleCol -1
	for {set col 0} {$col < $data(colCount)} {incr col} {
	    if {$data($col-hide) || $data($col-elide)} {
		continue
	    }

	    set lastVisibleCol $col
	    set w $data(hdrTxtFrLbl)$col
	    set wX [winfo rootx $w]
	    if {$x >= $wX && $x < $wX + [winfo width $w]} {
		return $col
	    }
	}
	set index $lastVisibleCol
    } elseif {[catch {format "%d" $idx} index] != 0} {
	for {set col 0} {$col < $data(colCount)} {incr col} {
	    set hasName [info exists data($col-name)]
	    if {$hasName && [string compare $idx $data($col-name)] == 0 ||
		!$hasName && [string compare $idx ""] == 0} {
		set index $col
		break
	    }
	}
	if {$col == $data(colCount)} {
	    return -code error \
		   "bad column index \"$idx\": must be active, anchor,\
		    end, @x,y, a number, or a name"
	}
    }

    if {$checkRange && ($index < 0 || $index > $data(lastCol))} {
	return -code error "column index \"$idx\" out of range"
    } else {
	return $index
    }
}

#------------------------------------------------------------------------------
# tablelist::cellIndex
#
# Checks the cell index idx and returns either its value in the form row,col or
# an error.  checkRange must be a boolean value: if true, it is additionally
# checked whether the two numerical values corresponding to idx are within the
# respective allowed ranges.
#------------------------------------------------------------------------------
proc tablelist::cellIndex {win idx checkRange} {
    upvar ::tablelist::ns${win}::data data

    set idxLen [string length $idx]
    if {[string first $idx "active"] == 0 && $idxLen >= 2} {
	set row $data(activeRow)
	set col $data(activeCol)
    } elseif {[string first $idx "anchor"] == 0 && $idxLen >= 2} {
	set row $data(anchorRow)
	set col $data(anchorCol)
    } elseif {[string first $idx "end"] == 0} {
	set row [rowIndex $win $idx 0]
	set col [colIndex $win $idx 0]
    } elseif {[string compare [string index $idx 0] "@"] == 0} {
	if {[catch {rowIndex $win $idx 0} row] != 0 ||
	    [catch {colIndex $win $idx 0} col] != 0} {
	    return -code error \
		   "bad cell index \"$idx\": must be active, anchor,\
		    end, @x,y, or row,col, where row must be active,\
		    anchor, end, a number, a full key, or a name, and\
		    col must be active, anchor, end, a number, or a name"
	}
    } else {
	set lst [split $idx ","]
	if {[llength $lst] != 2 ||
	    [catch {rowIndex $win [lindex $lst 0] 0} row] != 0 ||
	    [catch {colIndex $win [lindex $lst 1] 0} col] != 0} {
	    return -code error \
		   "bad cell index \"$idx\": must be active, anchor,\
		    end, @x,y, or row,col, where row must be active,\
		    anchor, end, a number, a full key, or a name, and\
		    col must be active, anchor, end, a number, or a name"
	}
    }

    if {$checkRange && ($row < 0 || $row > $data(lastRow) ||
	$col < 0 || $col > $data(lastCol))} {
	return -code error "cell index \"$idx\" out of range"
    } else {
	return $row,$col
    }
}

#------------------------------------------------------------------------------
# tablelist::adjustRowIndex
#
# Sets the row index specified by $rowName to the index of the nearest
# (non-hidden) row.
#------------------------------------------------------------------------------
proc tablelist::adjustRowIndex {win rowName {forceVisible 0}} {
    upvar ::tablelist::ns${win}::data data
    upvar $rowName row

    if {$row > $data(lastRow)} {
	set row $data(lastRow)
    }
    if {$row < 0} {
	set row 0
    }

    if {$forceVisible} {
	set origRow $row
	for {} {$row < $data(itemCount)} {incr row} {
	    set key [lindex [lindex $data(itemList) $row] end]
	    if {![info exists data($key-hide)]} {
		return ""
	    }
	}
	for {set row [expr {$origRow - 1}]} {$row >= 0} {incr row -1} {
	    set key [lindex [lindex $data(itemList) $row] end]
	    if {![info exists data($key-hide)]} {
		return ""
	    }
	}
	set row 0
    }
}

#------------------------------------------------------------------------------
# tablelist::adjustColIndex
#
# Sets the column index specified by $colName to the index of the nearest
# (non-hidden) column.
#------------------------------------------------------------------------------
proc tablelist::adjustColIndex {win colName {forceVisible 0}} {
    upvar ::tablelist::ns${win}::data data
    upvar $colName col

    if {$col > $data(lastCol)} {
	set col $data(lastCol)
    }
    if {$col < 0} {
	set col 0
    }

    if {$forceVisible} {
	set origCol $col
	for {} {$col < $data(colCount)} {incr col} {
	    if {!$data($col-hide)} {
		return ""
	    }
	}
	for {set col [expr {$origCol - 1}]} {$col >= 0} {incr col -1} {
	    if {!$data($col-hide)} {
		return ""
	    }
	}
	set col 0
    }
}

#------------------------------------------------------------------------------
# tablelist::findTabs
#
# Searches for the first and last occurrences of the tab character in the cell
# range specified by firstCol and lastCol in the given line of the body text
# child of the tablelist widget win.  Assigns the index of the first tab to
# $idx1Name and the index of the last tab to $idx2Name.  It is assumed that
# both columns are non-hidden (but there may be hidden ones between them).
#------------------------------------------------------------------------------
proc tablelist::findTabs {win line firstCol lastCol idx1Name idx2Name} {
    variable canElide
    variable elide
    upvar ::tablelist::ns${win}::data data
    upvar $idx1Name idx1 $idx2Name idx2

    set w $data(body)
    set endIdx $line.end

    set idx $line.1
    for {set col 0} {$col < $firstCol} {incr col} {
	if {!$data($col-hide) || $canElide} {
	    set idx [$w search $elide "\t" $idx $endIdx]+2c
	}
    }
    set idx1 [$w index $idx-1c]

    for {} {$col < $lastCol} {incr col} {
	if {!$data($col-hide) || $canElide} {
	    set idx [$w search $elide "\t" $idx $endIdx]+2c
	}
    }
    set idx2 [$w search $elide "\t" $idx $endIdx]
}

#------------------------------------------------------------------------------
# tablelist::sortStretchableColList
#
# Replaces the column indices different from end in the list of the stretchable
# columns of the tablelist widget win with their numerical equivalents and
# sorts the resulting list.
#------------------------------------------------------------------------------
proc tablelist::sortStretchableColList win {
    upvar ::tablelist::ns${win}::data data

    if {[llength $data(-stretch)] == 0 ||
	[string first $data(-stretch) "all"] == 0} {
	return ""
    }

    set containsEnd 0
    foreach elem $data(-stretch) {
	if {[string first $elem "end"] == 0} {
	    set containsEnd 1
	} else {
	    set tmp([colIndex $win $elem 0]) ""
	}
    }

    set data(-stretch) [lsort -integer [array names tmp]]
    if {$containsEnd} {
	lappend data(-stretch) end
    }
}

#------------------------------------------------------------------------------
# tablelist::deleteColData
#
# Cleans up the data associated with the col'th column of the tablelist widget
# win.
#------------------------------------------------------------------------------
proc tablelist::deleteColData {win col} {
    upvar ::tablelist::ns${win}::data data

    if {$data(editCol) == $col} {
	set data(editCol) -1
	set data(editRow) -1
    }

    #
    # Remove the elements with names of the form $col-*
    #
    if {[info exists data($col-redispId)]} {
	after cancel $data($col-redispId)
    }
    set w $data(body)
    foreach name [array names data $col-*] {
	unset data($name)
    }

    #
    # Remove the elements with names of the form k*,$col-*
    #
    foreach name [array names data k*,$col-*] {
	unset data($name)
	if {[string match "k*,$col-\[bf\]*" $name]} {
	    incr data(tagRefCount) -1
	} elseif {[string match "k*,$col-image" $name]} {
	    incr data(imgCount) -1
	} elseif {[string match "k*,$col-window" $name]} {
	    incr data(winCount) -1
	}
    }

    #
    # Remove col from the list of stretchable columns if explicitly specified
    #
    if {[string first $data(-stretch) "all"] != 0} {
	set stretchableCols {}
	foreach elem $data(-stretch) {
	    if {[string first $elem "end"] == 0 || $elem != $col} {
		lappend stretchableCols $elem
	    }
	}
	set data(-stretch) $stretchableCols
    }
}

#------------------------------------------------------------------------------
# tablelist::moveColData
#
# Moves the elements of oldArrName corresponding to oldCol to those of
# newArrName corresponding to newCol.
#------------------------------------------------------------------------------
proc tablelist::moveColData {win oldArrName newArrName imgArrName
			     oldCol newCol} {
    upvar $oldArrName oldArr $newArrName newArr $imgArrName imgArr

    foreach specialCol {activeCol anchorCol editCol} {
	if {$oldArr($specialCol) == $oldCol} {
	    set newArr($specialCol) $newCol
	}
    }

    if {$newCol < $newArr(colCount)} {
	foreach l [getSublabels $newArr(hdrTxtFrLbl)$newCol] {
	    destroy $l
	}
	set newArr(fmtCmdFlagList) \
	    [lreplace $newArr(fmtCmdFlagList) $newCol $newCol 0]
    }

    #
    # Move the elements of oldArr with names of the form $oldCol-*
    # to those of newArr with names of the form $newCol-*
    #
    set w $newArr(body)
    foreach newName [array names newArr $newCol-*] {
	unset newArr($newName)
    }
    foreach oldName [array names oldArr $oldCol-*] {
	regsub "$oldCol-" $oldName "$newCol-" newName
	set newArr($newName) $oldArr($oldName)
	unset oldArr($oldName)

	set tail [lindex [split $newName "-"] 1]
	switch $tail {
	    formatcommand {
		if {$newCol < $newArr(colCount)} {
		    set newArr(fmtCmdFlagList) \
			[lreplace $newArr(fmtCmdFlagList) $newCol $newCol 1]
		}
	    }
	    labelimage {
		set imgArr($newCol-$tail) $newArr($newName)
		unset newArr($newName)
	    }
	}
    }

    #
    # Move the elements of oldArr with names of the form k*,$oldCol-*
    # to those of newArr with names of the form k*,$newCol-*
    #
    foreach newName [array names newArr k*,$newCol-*] {
	unset newArr($newName)
    }
    foreach oldName [array names oldArr k*,$oldCol-*] {
	regsub -- ",$oldCol-" $oldName ",$newCol-" newName
	set newArr($newName) $oldArr($oldName)
	unset oldArr($oldName)
    }

    #
    # Replace oldCol with newCol in the list of
    # stretchable columns if explicitly specified
    #
    if {[info exists oldArr(-stretch)] &&
	[string first $oldArr(-stretch) "all"] != 0} {
	set stretchableCols {}
	foreach elem $oldArr(-stretch) {
	    if {[string first $elem "end"] != 0 && $elem == $oldCol} {
		lappend stretchableCols $newCol
	    } else {
		lappend stretchableCols $elem
	    }
	}
	set newArr(-stretch) $stretchableCols
    }
}

#------------------------------------------------------------------------------
# tablelist::deleteColFromCellList
#
# Returns the list obtained from a given list of cell indices by removing the
# elements whose column component equals a given column number.
#------------------------------------------------------------------------------
proc tablelist::deleteColFromCellList {cellList col} {
    set newCellList {}
    foreach cellIdx $cellList {
	scan $cellIdx "%d,%d" cellRow cellCol
	if {$cellCol != $col} {
	    lappend newCellList $cellIdx
	}
    }

    return $newCellList
}

#------------------------------------------------------------------------------
# tablelist::extractColFromCellList
#
# Returns the list of row indices obtained from those elements of a given list
# of cell indices whose column component equals a given column number.
#------------------------------------------------------------------------------
proc tablelist::extractColFromCellList {cellList col} {
    set rowList {}
    foreach cellIdx $cellList {
	scan $cellIdx "%d,%d" cellRow cellCol
	if {$cellCol == $col} {
	    lappend rowList $cellRow
	}
    }

    return $rowList
}

#------------------------------------------------------------------------------
# tablelist::replaceColInCellList
#
# Returns the list obtained from a given list of cell indices by replacing the
# occurrences of oldCol in the column components with newCol.
#------------------------------------------------------------------------------
proc tablelist::replaceColInCellList {cellList oldCol newCol} {
    set cellList [deleteColFromCellList $cellList $newCol]
    set newCellList {}
    foreach cellIdx $cellList {
	scan $cellIdx "%d,%d" cellRow cellCol
	if {$cellCol == $oldCol} {
	    lappend newCellList $cellRow,$newCol
	} else {
	    lappend newCellList $cellIdx
	}
    }

    return $newCellList
}

#------------------------------------------------------------------------------
# tablelist::condUpdateListVar
#
# Updates the list variable of the tablelist widget win if present.
#------------------------------------------------------------------------------
proc tablelist::condUpdateListVar win {
    upvar ::tablelist::ns${win}::data data

    if {$data(hasListVar)} {
	upvar #0 $data(-listvariable) var
	trace vdelete var wu $data(listVarTraceCmd)
	set var {}
	foreach item $data(itemList) {
	    lappend var [lrange $item 0 $data(lastCol)]
	}
	trace variable var wu $data(listVarTraceCmd)
    }
}

#------------------------------------------------------------------------------
# tablelist::reconfigColLabels
#
# Reconfigures the labels of the col'th column of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::reconfigColLabels {win imgArrName col} {
    variable usingTile
    upvar ::tablelist::ns${win}::data data
    upvar $imgArrName imgArr

    set optList {-labelalign -labelbackground -labelborderwidth -labelfont
		 -labelforeground -labelpady -labelrelief}
    if {!$usingTile} {
	lappend optList -labelheight
    }

    foreach opt $optList {
	if {[info exists data($col$opt)]} {
	    doColConfig $col $win $opt $data($col$opt)
	} else {
	    doColConfig $col $win $opt ""
	}
    }

    if {[info exists imgArr($col-labelimage)]} {
	doColConfig $col $win -labelimage $imgArr($col-labelimage)
    }
}

#------------------------------------------------------------------------------
# tablelist::charsToPixels
#
# Returns the width in pixels of the string consisting of a given number of "0"
# characters.
#------------------------------------------------------------------------------
proc tablelist::charsToPixels {win font charCount} {
    ### set str [string repeat "0" $charCount]
    set str ""
    for {set n 0} {$n < $charCount} {incr n} {
	append str 0
    }
    return [font measure $font -displayof $win $str]
}

#------------------------------------------------------------------------------
# tablelist::strRange
#
# Gets the largest initial (for alignment = left or center) or final (for
# alignment = right) range of characters from str whose width, when displayed
# in the given font, is no greater than pixels decremented by the width of
# snipStr.  Returns a string obtained from this substring by appending (for
# alignment = left or center) or prepending (for alignment = right) (part of)
# snipStr to it.
#------------------------------------------------------------------------------
proc tablelist::strRange {win str font pixels alignment snipStr} {
    if {$pixels < 0} {
	return ""
    }

    set width [font measure $font -displayof $win $str]
    if {$width <= $pixels} {
	return $str
    }

    set snipWidth [font measure $font -displayof $win $snipStr]
    if {$pixels <= $snipWidth} {
	set str $snipStr
	set snipStr ""
    } else {
	incr pixels -$snipWidth
    }

    if {[string compare $alignment "right"] == 0} {
	set idx [expr {[string length $str]*($width - $pixels)/$width}]
	set subStr [string range $str $idx end]
	set width [font measure $font -displayof $win $subStr]
	if {$width < $pixels} {
	    while 1 {
		incr idx -1
		set subStr [string range $str $idx end]
		set width [font measure $font -displayof $win $subStr]
		if {$width > $pixels} {
		    incr idx
		    set subStr [string range $str $idx end]
		    return $snipStr$subStr
		} elseif {$width == $pixels} {
		    return $snipStr$subStr
		}
	    }
	} elseif {$width == $pixels} {
	    return $snipStr$subStr
	} else {
	    while 1 {
		incr idx
		set subStr [string range $str $idx end]
		set width [font measure $font -displayof $win $subStr]
		if {$width <= $pixels} {
		    return $snipStr$subStr
		}
	    }
	}

    } else {
	set idx [expr {[string length $str]*$pixels/$width - 1}]
	set subStr [string range $str 0 $idx]
	set width [font measure $font -displayof $win $subStr]
	if {$width < $pixels} {
	    while 1 {
		incr idx
		set subStr [string range $str 0 $idx]
		set width [font measure $font -displayof $win $subStr]
		if {$width > $pixels} {
		    incr idx -1
		    set subStr [string range $str 0 $idx]
		    return $subStr$snipStr
		} elseif {$width == $pixels} {
		    return $subStr$snipStr
		}
	    }
	} elseif {$width == $pixels} {
	    return $subStr$snipStr
	} else {
	    while 1 {
		incr idx -1
		set subStr [string range $str 0 $idx]
		set width [font measure $font -displayof $win $subStr]
		if {$width <= $pixels} {
		    return $subStr$snipStr
		}
	    }
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::adjustItem
#
# Returns the list obtained by adjusting the list specified by item to the
# length expLen.
#------------------------------------------------------------------------------
proc tablelist::adjustItem {item expLen} {
    set len [llength $item]
    if {$len < $expLen} {
	for {set n $len} {$n < $expLen} {incr n} {
	    lappend item ""
	}
	return $item
    } else {
	return [lrange $item 0 [expr {$expLen - 1}]]
    }
}

#------------------------------------------------------------------------------
# tablelist::formatItem
#
# Returns the list obtained by formatting the elements of the item argument.
#------------------------------------------------------------------------------
proc tablelist::formatItem {win item} {
    upvar ::tablelist::ns${win}::data data

    set formattedItem {}
    set col 0
    foreach text $item fmtCmdFlag $data(fmtCmdFlagList) {
	if {$fmtCmdFlag} {
	    set text [uplevel #0 $data($col-formatcommand) [list $text]]
	}
	lappend formattedItem $text
	incr col
    }

    return $formattedItem
}

#------------------------------------------------------------------------------
# tablelist::hasChars
#
# Checks whether at least one element of the given list is a nonempty string.
#------------------------------------------------------------------------------
proc tablelist::hasChars list {
    foreach str $list {
	if {[string compare $str ""] != 0} {
	    return 1
	}
    }

    return 0
}

#------------------------------------------------------------------------------
# tablelist::getListWidth
#
# Returns the max. number of pixels that the elements of the given list would
# use in the specified font when displayed in the window win.
#------------------------------------------------------------------------------
proc tablelist::getListWidth {win list font} {
    set width 0
    foreach str $list {
	set strWidth [font measure $font -displayof $win $str]
	if {$strWidth > $width} {
	    set width $strWidth
	}
    }

    return $width
}

#------------------------------------------------------------------------------
# tablelist::joinList
#
# Returns the string formed by joining together with "\n" the strings obtained 
# by applying strRange to the elements of the given list, with the specified
# specified arguments.
#------------------------------------------------------------------------------
proc tablelist::joinList {win list font pixels alignment snipStr} {
    set list2 {}
    foreach str $list {
	lappend list2 [strRange $win $str $font $pixels $alignment $snipStr]
    }

    return [join $list2 "\n"]
}

#------------------------------------------------------------------------------
# tablelist::displayText
#
# Displays the given text in a message widget to be embedded into the specified
# cell of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::displayText {win key col text font alignment} {
    variable anchors
    upvar ::tablelist::ns${win}::data data

    set w $data(body).m$key,$col
    if {![winfo exists $w]} {
	#
	# Create a message widget and replace the binding tag Message with
	# $data(bodyTag) and TablelistBody in the list of its binding tags
	#
	message $w -background $data(-background) -borderwidth 0 \
		   -foreground $data(-foreground) -highlightthickness 0 \
		   -padx 0 -pady 0 -relief flat -takefocus 0 -width 1000000
	bindtags $w [lreplace [bindtags $w] 1 1 $data(bodyTag) TablelistBody]
    }

    $w configure -anchor $anchors($alignment) -font $font \
		 -justify $alignment -text $text
    updateColorsWhenIdle $win

    return $w
}

#------------------------------------------------------------------------------
# tablelist::displayImage
#
# Displays an image in a label widget to be embedded into the specified cell of
# the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::displayImage {win key col width} {
    upvar ::tablelist::ns${win}::data data

    set w $data(body).l$key,$col
    if {![winfo exists $w]} {
	#
	# Create a label widget and replace the binding tag Label with
	# $data(bodyTag) and TablelistBody in the list of its binding tags
	#
	tk::label $w -borderwidth 0 -height 0 -highlightthickness 0 \
		     -padx 0 -pady 0 -relief flat -takefocus 0
	bindtags $w [lreplace [bindtags $w] 1 1 $data(bodyTag) TablelistBody]
    }

    $w configure -image $data($key,$col-image) -width $width
    updateColorsWhenIdle $win

    return $w
}

#------------------------------------------------------------------------------
# tablelist::getAuxData
#
# Gets the name, type, and width of the image or window associated with the
# specified cell of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::getAuxData {win key col auxTypeName auxWidthName} {
    upvar ::tablelist::ns${win}::data data
    upvar $auxTypeName auxType $auxWidthName auxWidth

    if {[info exists data($key,$col-window)]} {
	set aux $data(body).f$key,$col
	set auxWidth $data($key,$col-reqWidth)
	set auxType 2
    } elseif {[info exists data($key,$col-image)]} {
	set aux [list ::tablelist::displayImage $win $key $col 0]
	set auxWidth [image width $data($key,$col-image)]
	set auxType 1
    } else {
	set aux ""
	set auxWidth 0
	set auxType 0
    }

    return $aux
}

#------------------------------------------------------------------------------
# tablelist::adjustElem
#
# Prepares the text specified by $textName and the auxiliary object width
# specified by $auxWidthName for insertion into a cell of the tablelist widget
# win.
#------------------------------------------------------------------------------
proc tablelist::adjustElem {win textName auxWidthName font pixels alignment
			    snipStr} {
    upvar $textName text $auxWidthName auxWidth

    if {$pixels == 0} {				;# convention: dynamic width
	if {$auxWidth != 0 && [string compare $text ""] != 0} {
	    if {[string compare $alignment "right"] == 0} {
		set text "$text "
	    } else {
		set text " $text"
	    }
	}
    } elseif {$auxWidth == 0} {			;# no image or window
	set text [strRange $win $text $font $pixels $alignment $snipStr]
    } elseif {[string compare $text ""] == 0} {	;# aux. object w/o text
	if {$auxWidth > $pixels} {
	    set auxWidth $pixels
	}
    } else {					;# both aux. object and text
	set gap [font measure $font -displayof $win " "]
	if {$auxWidth + $gap <= $pixels} {
	    incr pixels -[expr {$auxWidth + $gap}]
	    set text [strRange $win $text $font $pixels $alignment $snipStr]
	    if {[string compare $alignment "right"] == 0} {
		set text "$text "
	    } else {
		set text " $text"
	    }
	} elseif {$auxWidth <= $pixels} {
	    set text ""				;# can't display the text
	} else {
	    set auxWidth $pixels
	    set text ""				;# can't display the text
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::adjustMlElem
#
# Prepares the list specified by $listName and the auxiliary object width
# specified by $auxWidthName for insertion into a multiline cell of the
# tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::adjustMlElem {win listName auxWidthName font pixels alignment
			      snipStr} {
    upvar $listName list $auxWidthName auxWidth

    set list2 {}
    if {$pixels == 0} {				;# convention: dynamic width
	if {$auxWidth != 0 && [hasChars $list]} {
	    foreach str $list {
		if {[string compare $alignment "right"] == 0} {
		    lappend list2 "$str "
		} else {
		    lappend list2 " $str"
		}
	    }
	    set list $list2
	}
    } elseif {$auxWidth == 0} {			;# no image or window
	foreach str $list {
	    lappend list2 [strRange $win $str $font $pixels $alignment $snipStr]
	}
	set list $list2
    } elseif {![hasChars $list]} {		;# aux. object w/o text
	if {$auxWidth > $pixels} {
	    set auxWidth $pixels
	}
    } else {					;# both aux. object and text
	set gap [font measure $font -displayof $win " "]
	if {$auxWidth + $gap <= $pixels} {
	    incr pixels -[expr {$auxWidth + $gap}]
	    foreach str $list {
		set str [strRange $win $str $font $pixels $alignment $snipStr]
		if {[string compare $alignment "right"] == 0} {
		    lappend list2 "$str "
		} else {
		    lappend list2 " $str"
		}
	    }
	    set list $list2
	} elseif {$auxWidth <= $pixels} {
	    foreach str $list {
		lappend list2 ""
	    }
	    set list $list2			;# can't display the text
	} else {
	    set auxWidth $pixels
	    foreach str $list {
		lappend list2 ""
	    }
	    set list $list2			;# can't display the text
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::getCellTextWidth
#
# Returns the number of pixels that the given text would use when displayed in
# a cell of a dynamic-width column of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::getCellTextWidth {win text auxWidth cellFont} {
    if {[string match "*\n*" $text]} {
	set list [split $text "\n"]
	if {$auxWidth != 0 && [hasChars $list]} {
	    foreach str $list {
		lappend list2 " $str"
	    }
	    set list $list2
	}
	return [getListWidth $win $list $cellFont]
    } else {
	if {$auxWidth != 0 && [string compare $text ""] != 0} {
	    set text " $text"
	}
	return [font measure $cellFont -displayof $win $text]
    }
}

#------------------------------------------------------------------------------
# tablelist::insertElem
#
# Inserts the given text and auxiliary object (image or window) into the text
# widget w, just before the character position specified by index.  The object
# will follow the text if alignment is "right", and will precede it otherwise.
#------------------------------------------------------------------------------
proc tablelist::insertElem {w index text aux auxType alignment} {
    set index [$w index $index]

    if {$auxType == 0} {				;# no image or window
	$w insert $index $text
    } elseif {[string compare $alignment "right"] == 0} {
	if {$auxType == 1} {					;# image
	    $w window create $index -pady 1 -create $aux
	} else {						;# window
	    place $aux.w -relx 1.0 -anchor ne
	    $w window create $index -pady 1 -window $aux
	}
	$w insert $index $text
    } else {
	$w insert $index $text
	if {$auxType == 1} {					;# image
	    $w window create $index -pady 1 -create $aux
	} else {						;# window
	    place $aux.w -relx 0.0 -anchor nw
	    $w window create $index -pady 1 -window $aux
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::insertMlElem
#
# Inserts the given message widget and auxiliary object (image or window) into
# the text widget w, just before the character position specified by index.
# The object will follow the message widget if alignment is "right", and will
# precede it otherwise.
#------------------------------------------------------------------------------
proc tablelist::insertMlElem {w index msgScript aux auxType alignment} {
    set index [$w index $index]

    if {$auxType == 0} {				;# no image or window
	$w window create $index -pady 1 -create $msgScript
    } elseif {[string compare $alignment "right"] == 0} {
	if {$auxType == 1} {					;# image
	    $w window create $index -pady 1 -create $aux
	} else {						;# window
	    place $aux.w -relx 1.0 -anchor ne
	    $w window create $index -pady 1 -window $aux
	}
	$w window create $index -pady 1 -create $msgScript
    } else {
	$w window create $index -pady 1 -create $msgScript
	if {$auxType == 1} {					;# image
	    $w window create $index -pady 1 -create $aux
	} else {						;# window
	    place $aux.w -relx 0.0 -anchor nw
	    $w window create $index -pady 1 -window $aux
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::updateCell
#
# Updates the contents of the text widget w starting at index1 and ending just
# before index2 by keeping the auxiliary object (image or window) (if any) and
# replacing only the text between the two character positions.
#------------------------------------------------------------------------------
proc tablelist::updateCell {w index1 index2 text aux auxType auxWidth
			    alignment} {
    if {$auxType == 0} {				;# no image or window
	$w delete $index1 $index2
	$w insert $index1 $text
    } else {
	#
	# Check whether the label containing an image or the frame containing
	# a window is mapped at the first or last position of the cell
	#
	if {$auxType == 1} {					;# image
	    if {[setImgLabelWidth $w $index1 $auxWidth]} {
		set auxFound 1
		$w delete $index1+1c $index2
	    } elseif {[setImgLabelWidth $w $index2-1c $auxWidth]} {
		set auxFound 1
		$w delete $index1 $index2-1c
	    } else {
		set auxFound 0
		$w delete $index1 $index2
	    }
	} else {						;# window
	    $aux configure -width $auxWidth

	    if {[string compare [lindex [$w dump -window $index1] 1] $aux]
		== 0} {
		set auxFound 1
		$w delete $index1+1c $index2
	    } elseif {[string compare [lindex [$w dump -window $index2-1c] 1]
		       $aux] == 0} {
		set auxFound 1
		$w delete $index1 $index2-1c
	    } else {
		set auxFound 0
		$w delete $index1 $index2
	    }
	}

	if {$auxFound} {
	    #
	    # Insert the text
	    #
	    if {[string compare $alignment "right"] == 0} {
		if {$auxType == 2} {				;# window
		    place $aux.w -relx 1.0 -anchor ne
		}
		set index $index1
	    } else {
		if {$auxType == 2} {				;# window
		    place $aux.w -relx 0.0 -anchor nw
		}
		set index $index1+1c
	    }
	    $w insert $index $text
	} else {
	    #
	    # Insert the text and the aux. window
	    #
	    if {$auxType == 1} {				;# image
		set aux [lreplace $aux end end $auxWidth]
	    } else {						;# window
		$aux configure -width $auxWidth
	    }
	    insertElem $w $index1 $text $aux $auxType $alignment
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::updateMlCell
#
# Updates the contents of the text widget w starting at index1 and ending just
# before index2 by keeping the auxiliary object (image or window) (if any) and
# replacing only the multiline text between the two character positions.
#------------------------------------------------------------------------------
proc tablelist::updateMlCell {w index1 index2 msgScript aux auxType auxWidth
			      alignment} {
    if {$auxType == 0} {				;# no image or window
	set path [lindex [$w dump -window $index1] 1]
	if {[string compare $path ""] != 0 &&
	    [string compare [winfo class $path] "Message"] == 0} {
	    eval $msgScript
	} elseif {[catch {
	    $w window configure $index1 -pady 1 -create $msgScript
	}] != 0} {
	    $w delete $index1 $index2
	    $w window create $index1 -pady 1 -create $msgScript
	}
    } else {
	#
	# Check whether the label containing an image or the frame containing
	# a window is mapped at the first or last position of the cell
	#
	if {$auxType == 1} {					;# image
	    if {[setImgLabelWidth $w $index1 $auxWidth]} {
		set auxFound 1
		$w delete $index1+1c $index2
	    } elseif {[setImgLabelWidth $w $index2-1c $auxWidth]} {
		set auxFound 1
		$w delete $index1 $index2-1c
	    } else {
		set auxFound 0
		$w delete $index1 $index2
	    }
	} else {						;# window
	    $aux configure -width $auxWidth

	    if {[string compare [lindex [$w dump -window $index1] 1] $aux]
		== 0} {
		set auxFound 1
		$w delete $index1+1c $index2
	    } elseif {[string compare [lindex [$w dump -window $index2-1c] 1]
		       $aux] == 0} {
		set auxFound 1
		$w delete $index1 $index2-1c
	    } else {
		set auxFound 0
		$w delete $index1 $index2
	    }
	}

	if {$auxFound} {
	    #
	    # Insert the message window
	    #
	    if {[string compare $alignment "right"] == 0} {
		if {$auxType == 2} {				;# window
		    place $aux.w -relx 1.0 -anchor ne
		}
		set index $index1
	    } else {
		if {$auxType == 2} {				;# window
		    place $aux.w -relx 0.0 -anchor nw
		}
		set index $index1+1c
	    }
	    set path [lindex [$w dump -window $index] 1]
	    if {[string compare $path ""] != 0 &&
		[string compare [winfo class $path] "Message"] == 0} {
		eval $msgScript
	    } elseif {[catch {
		$w window configure $index -pady 1 -create $msgScript
	    }] != 0} {
		$w window create $index -pady 1 -create $msgScript
	    }
	} else {
	    #
	    # Insert the message and aux. windows
	    #
	    if {$auxType == 1} {				;# image
		set aux [lreplace $aux end end $auxWidth]
	    } else {						;# window
		$aux configure -width $auxWidth
	    }
	    insertMlElem $w $index1 $msgScript $aux $auxType $alignment
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::setImgLabelWidth
#
# Sets the width of the image label embedded into the text widget w at the
# given index to the specified value.
#------------------------------------------------------------------------------
proc tablelist::setImgLabelWidth {w index width} {
    set path [lindex [$w dump -window $index] 1]
    if {[string compare $path ""] != 0 &&
	[string compare [winfo class $path] "Label"] == 0} {
	$path configure -width $width
	return 1
    } elseif {[catch {$w window cget $index -create} createScript] == 0 &&
	      [string match "::tablelist::displayImage*" $createScript]} {
	set createScript [lreplace $createScript end end $width]
	$w window configure $index -pady 1 -create $createScript
	return 1
    } else {
	return 0
    }
}

#------------------------------------------------------------------------------
# tablelist::appendComplexElem
#
# Adjusts the given text and the width of the auxiliary object (image or
# window) corresponding to the specified cell of the tablelist widget win, and
# inserts the text and the auxiliary object (if any) just before the newline
# character at the end of the specified line of the tablelist's body.
#------------------------------------------------------------------------------
proc tablelist::appendComplexElem {win key row col text pixels alignment
				   snipStr cellTags line} {
    upvar ::tablelist::ns${win}::data data

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
    set cellFont [getCellFont $win $key $col]
    if {$pixels == 0} {		;# convention: dynamic width
	if {$data($col-maxPixels) > 0} {
	    if {$data($col-reqPixels) > $data($col-maxPixels)} {
		set pixels $data($col-maxPixels)
	    }
	}
    }
    if {$pixels != 0} {
	incr pixels $data($col-delta)
    }
    if {$multiline} {
	adjustMlElem $win list auxWidth $cellFont $pixels $alignment $snipStr
	set msgScript [list ::tablelist::displayText $win $key $col \
		       [join $list "\n"] $cellFont $alignment]
    } else {
	adjustElem $win text auxWidth $cellFont $pixels $alignment $snipStr
    }

    #
    # Insert the text and the auxiliary object (if any) just before the newline
    #
    set w $data(body)
    if {$auxType == 0} {				;# no image or window
	if {$multiline} {
	    $w insert $line.end "\t\t" $cellTags
	    $w window create $line.end-1c -pady 1 -create $msgScript
	} else {
	    $w insert $line.end "\t$text\t" $cellTags
	}
    } else {
	$w insert $line.end "\t\t" $cellTags
	if {$auxType == 1} {					;# image
	    #
	    # Update the creation script for the image label
	    #
	    set aux [lreplace $aux end end $auxWidth]
	} else {						;# window
	    #
	    # Create a frame and evaluate the script that
	    # creates a child window within the frame
	    #
	    tk::frame $aux -borderwidth 0 -class TablelistWindow -container 0 \
			   -height $data($key,$col-reqHeight) \
			   -highlightthickness 0 -relief flat -takefocus 0 \
			   -width $auxWidth
	    uplevel #0 $data($key,$col-window) [list $win $row $col $aux.w]
	}
	if {$multiline} {
	    insertMlElem $w $line.end-1c $msgScript $aux $auxType $alignment
	} else {
	    insertElem $w $line.end-1c $text $aux $auxType $alignment
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::makeColFontAndTagLists
#
# Builds the lists data(colFontList) of the column fonts and data(colTagsList)
# of the column tag names for the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::makeColFontAndTagLists win {
    upvar ::tablelist::ns${win}::data data

    set widgetFont $data(-font)
    set data(colFontList) {}
    set data(colTagsList) {}
    set data(hasColTags) 0

    for {set col 0} {$col < $data(colCount)} {incr col} {
	set tagNames {}

	if {[info exists data($col-font)]} {
	    lappend data(colFontList) $data($col-font)
	    lappend tagNames col-font-$data($col-font)
	    set data(hasColTags) 1
	} else {
	    lappend data(colFontList) $widgetFont
	}

	foreach opt {-background -foreground} {
	    if {[info exists data($col$opt)]} {
		lappend tagNames col$opt-$data($col$opt)
		set data(hasColTags) 1
	    }
	}

	lappend data(colTagsList) $tagNames
    }
}

#------------------------------------------------------------------------------
# tablelist::makeSortAndArrowColLists
#
# Builds the lists data(sortColList) of the sort columns and data(arrowColList)
# of the arrow columns for the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::makeSortAndArrowColLists win {
    upvar ::tablelist::ns${win}::data data

    set data(sortColList) {}
    set data(arrowColList) {}

    #
    # Build a list of {col sortRank} pairs and sort it based on sortRank
    #
    set pairList {}
    for {set col 0} {$col < $data(colCount)} {incr col} {
	if {$data($col-sortRank) > 0} {
	    lappend pairList [list $col $data($col-sortRank)]
	}
    }
    set pairList [lsort -integer -index 1 $pairList]

    #
    # Build data(sortColList) and data(arrowColList), and update
    # the sort ranks to have values from 1 to [llength $pairList]
    #
    set sortRank 1
    foreach pair $pairList {
	set col [lindex $pair 0]
	lappend data(sortColList) $col
	set data($col-sortRank) $sortRank
	if {$sortRank < 10 && $data(-showarrow) && $data($col-showarrow)} {
	    lappend data(arrowColList) $col
	    configCanvas $win $col
	    raiseArrow $win $col
	}
	incr sortRank
    }
}

#------------------------------------------------------------------------------
# tablelist::setupColumns
#
# Updates the value of the -colums configuration option for the tablelist
# widget win by using the width, title, and alignment specifications given in
# the columns argument, and creates the corresponding label (and separator)
# widgets if createLabels is true.
#------------------------------------------------------------------------------
proc tablelist::setupColumns {win columns createLabels} {
    variable usingTile
    variable configSpecs
    variable configOpts
    variable alignments
    upvar ::tablelist::ns${win}::data data

    set argCount [llength $columns]
    set colConfigVals {}

    #
    # Check the syntax of columns before performing any changes
    #
    for {set n 0} {$n < $argCount} {incr n} {
	#
	# Get the column width
	#
	set width [lindex $columns $n]
	set width [format "%d" $width]	;# integer check with error message

	#
	# Get the column title
	#
	if {[incr n] == $argCount} {
	    return -code error "column title missing"
	}
	set title [lindex $columns $n]

	#
	# Get the column alignment
	#
	set alignment left
	if {[incr n] < $argCount} {
	    set next [lindex $columns $n]
	    if {[catch {format "%d" $next}] == 0} {	;# integer check
		incr n -1
	    } else {
		set alignment [mwutil::fullOpt "alignment" $next $alignments]
	    }
	}

	#
	# Append the properly formatted values of width,
	# title, and alignment to the list colConfigVals
	#
	lappend colConfigVals $width $title $alignment
    }

    #
    # Save the value of colConfigVals in data(-columns)
    #
    set data(-columns) $colConfigVals

    #
    # Delete the labels, canvases, and separators if requested
    #
    if {$createLabels} {
	foreach w [winfo children $data(hdrTxtFr)] {
	    destroy $w
	}
	foreach w [winfo children $win] {
	    if {[regexp {^sep[0-9]+$} [winfo name $w]]} {
		destroy $w
	    }
	}
	set data(fmtCmdFlagList) {}
    }

    #
    # Build the list data(colList), and create
    # the labels and canvases if requested
    #
    set widgetFont $data(-font)
    set oldColCount $data(colCount)
    set data(colList) {}
    set data(colCount) 0
    set data(lastCol) -1
    set col 0
    foreach {width title alignment} $data(-columns) {
	#
	# Append the width in pixels and the
	# alignment to the list data(colList)
	#
	if {$width > 0} {		;# convention: width in characters
	    set pixels [charsToPixels $win $widgetFont $width]
	    set data($col-lastStaticWidth) $pixels
	} elseif {$width < 0} {		;# convention: width in pixels
	    set pixels [expr {(-1)*$width}]
	    set data($col-lastStaticWidth) $pixels
	} else {			;# convention: dynamic width
	    set pixels 0
	}
	lappend data(colList) $pixels $alignment
	incr data(colCount)
	set data(lastCol) $col

	if {$createLabels} {
	    set data($col-elide) 0
	    foreach {name val} {delta 0  lastStaticWidth 0  maxPixels 0
				sortOrder ""  sortRank 0  editable 0
				editwindow entry  hide 0  maxwidth 0
				resizable 1  showarrow 1  showlinenumbers 0
				sortmode ascii} {
		if {![info exists data($col-$name)]} {
		    set data($col-$name) $val
		}
	    }
	    lappend data(fmtCmdFlagList) [info exists data($col-formatcommand)]

	    #
	    # Create the label
	    #
	    set w $data(hdrTxtFrLbl)$col
	    if {$usingTile} {
		ttk::label $w -style TablelistHeader.TLabel -image "" \
			      -padding {1 1 1 1} -takefocus 0 -text "" \
			      -textvariable "" -underline -1 -wraplength 0
	    } else {
		tk::label $w -bitmap "" -highlightthickness 0 -image "" \
			     -takefocus 0 -text "" -textvariable "" \
			     -underline -1 -wraplength 0
	    }

	    #
	    # Apply to it the current configuration options
	    #
	    foreach opt $configOpts {
		set optGrp [lindex $configSpecs($opt) 2]
		if {[string compare $optGrp "l"] == 0} {
		    set optTail [string range $opt 6 end]
		    if {[info exists data($col$opt)]} {
			configLabel $w -$optTail $data($col$opt)
		    } else {
			configLabel $w -$optTail $data($opt)
		    }
		} elseif {[string compare $optGrp "c"] == 0} {
		    configLabel $w $opt $data($opt)
		}
	    }
	    catch {configLabel $w -state $data(-state)}

	    #
	    # Replace the binding tag Label with TablelistLabel
	    # in the list of binding tags of the label
	    #
	    bindtags $w [lreplace [bindtags $w] 1 1 TablelistLabel]

	    #
	    # Create a canvas containing the sort arrows
	    #
	    set w $data(hdrTxtFrCanv)$col
	    canvas $w -borderwidth 0 -highlightthickness 0 \
		      -relief flat -takefocus 0
	    regexp {^(flat|sunken)([0-9]+)x([0-9]+)$} $data(-arrowstyle) \
		   dummy relief width height
	    createArrows $w $width $height $relief

	    #
	    # Apply to it the current configuration options
	    #
	    foreach opt $configOpts {
		if {[string compare [lindex $configSpecs($opt) 2] "c"] == 0} {
		    $w configure $opt $data($opt)
		}
	    }
	    
	    #
	    # Replace the binding tag Canvas with TablelistArrow
	    # in the list of binding tags of the canvas
	    #
	    bindtags $w [lreplace [bindtags $w] 1 1 TablelistArrow]

	    if {[info exists data($col-labelimage)]} {
		doColConfig $col $win -labelimage $data($col-labelimage)
	    }
	}

	#
	# Configure the edit window if present
	#
	if {$col == $data(editCol) &&
	    [string compare [winfo class $data(bodyFrEd)] "Mentry"] != 0} {
	    catch {$data(bodyFrEd) configure -justify $alignment}
	}

	incr col
    }

    #
    # Clean up the data associated with the deleted columns
    #
    for {set col $data(colCount)} {$col < $oldColCount} {incr col} {
	deleteColData $win $col
    }

    #
    # Create the separators if needed
    #
    if {$createLabels && $data(-showseparators)} {
	createSeps $win
    }
}

#------------------------------------------------------------------------------
# tablelist::createSeps
#
# Creates and manages the separator frames in the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::createSeps win {
    variable usingTile
    upvar ::tablelist::ns${win}::data data

    set x 1
    if {$usingTile} {
	if {[string compare $tile::currentTheme "xpnative"] == 0 &&
	    $::tablelist::xpStyle} {
	    set x 0
	} elseif {[string compare $tile::currentTheme "tileqt"] == 0 &&
		  [string compare [string tolower $tile::theme::tileqt::theme] \
		   "qtcurve"] == 0} {
	    set x 2
	}
    }

    for {set col 0} {$col < $data(colCount)} {incr col} {
	#
	# Create the col'th separator frame and attach it
	# to the right edge of the col'th header label
	#
	set w $data(sep)$col
	if {$usingTile} {
	    ttk::separator $w -style Seps$win.TSeparator \
			      -cursor $data(-cursor) -orient vertical \
			      -takefocus 0
	} else {
	    tk::frame $w -background $data(-background) -borderwidth 1 \
			 -container 0 -cursor $data(-cursor) \
			 -highlightthickness 0 -relief sunken \
			 -takefocus 0 -width 2
	}
	place $w -in $data(hdrTxtFrLbl)$col -anchor ne -bordermode outside \
		 -relx 1.0 -x $x

	#
	# Replace the binding tag Frame with $data(bodyTag) and
	# TablelistBody in the list of binding tags of the frame
	#
	bindtags $w [lreplace [bindtags $w] 1 1 $data(bodyTag) TablelistBody]
    }
    
    adjustSepsWhenIdle $win
}

#------------------------------------------------------------------------------
# tablelist::adjustSepsWhenIdle
#
# Arranges for the height and vertical position of each separator frame in the
# tablelist widget win to be adjusted at idle time.
#------------------------------------------------------------------------------
proc tablelist::adjustSepsWhenIdle win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(sepsId)]} {
	return ""
    }

    set data(sepsId) [after idle [list tablelist::adjustSeps $win]]
}

#------------------------------------------------------------------------------
# tablelist::adjustSeps
#
# Adjusts the height and vertical position of each separator frame in the
# tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::adjustSeps win {
    variable usingTile
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(sepsId)]} {
	after cancel $data(sepsId)
	unset data(sepsId)
    }

    #
    # Get the height to be applied to the separator frames
    #
    set w $data(body)
    set textIdx [$w index @0,[expr {[winfo height $w] - 1}]]
    set dlineinfo [$w dlineinfo $textIdx]
    if {$data(itemCount) == 0 || [string compare $dlineinfo ""] == 0} {
	set sepHeight 1
    } else {
	foreach {x y width height baselinePos} $dlineinfo {}
	set sepHeight [expr {$y + $height}]
    }

    #
    # Set the height of the main separator frame (if any) and attach
    # the latter to the right edge of the last non-hidden title column
    #
    set startCol [expr {$data(-titlecolumns) - 1}]
    if {$startCol > $data(lastCol)} {
	set startCol $data(lastCol)
    }
    for {set col $startCol} {$col >= 0} {incr col -1} {
	if {!$data($col-hide)} {
	    break
	}
    }
    set w $data(sep)
    if {$col < 0} {
	if {[winfo exists $w]} {
	    place forget $w
	}
    } else {
	if {$usingTile &&
	    [string compare $tile::currentTheme "xpnative"] == 0 &&
	    $::tablelist::xpStyle} {
	    set x 0
	} else {
	    set x 1
	}
	place $w -in $data(hdrTxtFrLbl)$col -anchor ne -bordermode outside \
		 -height [expr {$sepHeight + [winfo height $data(hdr)] - 1}] \
		 -relx 1.0 -x $x -y 1
	raise $w
    }

    #
    # Set the height and vertical position of each separator frame
    #
    if {!$usingTile && $data(-showlabels)} {
	incr sepHeight
    }
    foreach w [winfo children $win] {
	if {[regexp {^sep[0-9]+$} [winfo name $w]]} {
	    if {$data(-showlabels)} {
		if {$usingTile} {
		    place configure $w -height $sepHeight -rely 1.0 -y 0
		} else {
		    place configure $w -height $sepHeight -rely 1.0 -y -1
		}
	    } else {
		if {$usingTile} {
		    place configure $w -height $sepHeight -rely 0.0 -y 1
		} else {
		    place configure $w -height $sepHeight -rely 0.0 -y 0
		}
	    }
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::adjustColumns
#
# Applies some configuration options to the labels of the tablelist widget win,
# places them in the header frame, computes and sets the tab stops for the body
# text widget, and adjusts the width and height of the header frame.  The
# whichWidths argument specifies the dynamic-width columns or labels whose
# widths are to be computed when performing these operations.  The stretchCols
# argument specifies whether to stretch the stretchable columns.
#------------------------------------------------------------------------------
proc tablelist::adjustColumns {win whichWidths stretchCols} {
    variable canElide
    upvar ::tablelist::ns${win}::data data

    set compAllColWidths [expr {[string compare $whichWidths "allCols"] == 0}]
    set compAllLabelWidths \
	[expr {[string compare $whichWidths "allLabels"] == 0}]

    #
    # Configure the labels and compute the positions of
    # the tab stops to be set in the body text widget
    #
    set data(hdrPixels) 0
    set tabs {}
    set col 0
    set x 0
    foreach {pixels alignment} $data(colList) {
	set w $data(hdrTxtFrLbl)$col
	if {$data($col-hide) && !$canElide} {
	    place forget $w
	    incr col
	    continue
	}

	#
	# Adjust the col'th label
	#
	if {[info exists data($col-labelalign)]} {
	    set labelAlignment $data($col-labelalign)
	} else {
	    set labelAlignment $alignment
	}
	if {$pixels != 0} {			;# convention: static width
	    incr pixels $data($col-delta)
	}
	adjustLabel $win $col $pixels $labelAlignment

	if {$pixels == 0} {			;# convention: dynamic width
	    #
	    # Compute the column or label width if requested
	    #
	    if {$compAllColWidths} {
		computeColWidth $win $col
	    } elseif {$compAllLabelWidths} {
		computeLabelWidth $win $col
	    } elseif {[lsearch -exact $whichWidths $col] >= 0} {
		computeColWidth $win $col
	    } elseif {[lsearch -exact $whichWidths l$col] >= 0} {
		computeLabelWidth $win $col
	    }

	    set pixels $data($col-reqPixels)
	    if {$data($col-maxPixels) > 0 && $pixels > $data($col-maxPixels)} {
		set pixels $data($col-maxPixels)
		incr pixels $data($col-delta)
		adjustLabel $win $col $pixels $labelAlignment
	    } else {
		incr pixels $data($col-delta)
	    }
	}

	if {$col == $data(editCol) &&
	    ![string match "*Checkbutton" [winfo class $data(bodyFrEd)]]} {
	    adjustEditWindow $win $pixels
	}

	set canvas $data(hdrTxtFrCanv)$col
	if {[lsearch -exact $data(arrowColList) $col] >= 0 &&
	    !$data($col-elide) && !$data($col-hide)} {
	    #
	    # Place the canvas to the left side of the label if the
	    # latter is right-justified and to its right side otherwise
	    #
	    if {[string compare $labelAlignment "right"] == 0} {
		place $canvas -in $w -anchor w -bordermode outside \
			      -relx 0.0 -x $data(charWidth) -rely 0.499 -y -1
	    } else {
		place $canvas -in $w -anchor e -bordermode outside \
			      -relx 1.0 -x -$data(charWidth) -rely 0.499 -y -1
	    }
	    raise $canvas
	} else {
	    place forget $canvas
	}

	#
	# Place the label in the header frame
	#
	if {$data($col-elide) || $data($col-hide)} {
	    foreach l [getSublabels $w] {
		place forget $l
	    }
	    place $w -x [expr {$x - 1}] -relheight 1.0 -width 1
	    lower $w
	} else {
	    set labelPixels [expr {$pixels + 2*$data(charWidth)}]
	    place $w -x $x -relheight 1.0 -width $labelPixels
	}

	#
	# Append a tab stop and the alignment to the tabs list
	#
	if {!$data($col-elide) && !$data($col-hide)} {
	    incr x $data(charWidth)
	    switch $alignment {
		left {
		    lappend tabs $x left
		    incr x $pixels
		}
		right {
		    incr x $pixels
		    lappend tabs $x right
		}
		center {
		    lappend tabs [expr {$x + $pixels/2}] center
		    incr x $pixels
		}
	    }
	    incr x $data(charWidth)
	    lappend tabs $x left
	}

	incr col
    }
    place $data(hdrLbl) -x $x

    #
    # Apply the value of tabs to the body text widget
    #
    $data(body) configure -tabs $tabs

    #
    # Adjust the width and height of the frames data(hdrTxtFr) and data(hdr)
    #
    set data(hdrPixels) $x
    $data(hdrTxtFr) configure -width $data(hdrPixels)
    if {$data(-width) <= 0} {
	if {$stretchCols} {
	    $data(hdr) configure -width $data(hdrPixels)
	    $data(lb) configure -width \
		      [expr {$data(hdrPixels) / $data(charWidth)}]
	}
    } else {
	$data(hdr) configure -width 0
    }
    adjustHeaderHeight $win

    #
    # Stretch the stretchable columns if requested, and update
    # the scrolled column offset and the horizontal scrollbar
    #
    if {$stretchCols} {
	stretchColumnsWhenIdle $win
    }
    if {![info exists data(colBeingResized)]} {
	updateScrlColOffsetWhenIdle $win
    }
    updateHScrlbarWhenIdle $win
}

#------------------------------------------------------------------------------
# tablelist::adjustLabel
#
# Applies some configuration options to the col'th label of the tablelist
# widget win as well as to the label's sublabels (if any), and places the
# sublabels.
#------------------------------------------------------------------------------
proc tablelist::adjustLabel {win col pixels alignment} {
    variable anchors
    variable usingTile
    upvar ::tablelist::ns${win}::data data

    #
    # Apply some configuration options to the label and its sublabels (if any)
    #
    set w $data(hdrTxtFrLbl)$col
    set anchor $anchors($alignment)
    set borderWidth [winfo pixels $w [$w cget -borderwidth]]
    if {$borderWidth < 0} {
	set borderWidth 0
    }
    set padX [expr {$data(charWidth) - $borderWidth}]
    configLabel $w -anchor $anchor -justify $alignment -padx $padX
    if {[info exists data($col-labelimage)]} {
	set imageWidth [image width $data($col-labelimage)]
	$w-tl configure -anchor $anchor -justify $alignment
    } else {
	set imageWidth 0
    }

    #
    # Make room for the canvas displaying an up- or down-arrow if needed
    #
    set title [lindex $data(-columns) [expr {3*$col + 1}]]
    set labelFont [$w cget -font]
    if {[lsearch -exact $data(arrowColList) $col] >= 0} {
	set spaceWidth [font measure $labelFont -displayof $w " "]
	set canvas $data(hdrTxtFrCanv)$col
	set canvasWidth $data(arrowWidth)
	if {[llength $data(arrowColList)] > 1} {
	    incr canvasWidth 6
	    $canvas itemconfigure sortRank \
		    -image sortRank$data($col-sortRank)$win
	}
	$canvas configure -width $canvasWidth
	set spaces "  "
	set n 2
	while {$n*$spaceWidth < $canvasWidth + $data(charWidth)} {
	    append spaces " "
	    incr n
	}
	set spacePixels [expr {$n * $spaceWidth}]
    } else {
	set spaces ""
	set spacePixels 0
    }

    if {$pixels == 0} {				;# convention: dynamic width
	#
	# Set the label text
	#
	if {$imageWidth == 0} {				;# no image
	    if {[string compare $title ""] == 0} {
		set text $spaces
	    } else {
		set lines {}
		foreach line [split $title "\n"] {
		    if {[string compare $alignment "right"] == 0} {
			lappend lines $spaces$line
		    } else {
			lappend lines $line$spaces
		    }
		}
		set text [join $lines "\n"]
	    }
	    $w configure -text $text
	} elseif {[string compare $title ""] == 0} {	;# image w/o text
	    $w configure -text ""
	    set text $spaces
	    $w-tl configure -text $text
	    $w-il configure -width $imageWidth
	} else {					;# both image and text
	    $w configure -text ""
	    set lines {}
	    foreach line [split $title "\n"] {
		if {[string compare $alignment "right"] == 0} {
		    lappend lines "$spaces$line "
		} else {
		    lappend lines " $line$spaces"
		}
	    }
	    set text [join $lines "\n"]
	    $w-tl configure -text $text
	    $w-il configure -width $imageWidth
	}
    } else {
	#
	# Clip each line of title according to pixels and alignment
	#
	set lessPixels [expr {$pixels - $spacePixels}]
	if {$imageWidth == 0} {				;# no image
	    if {[string compare $title ""] == 0} {
		set text $spaces
	    } else {
		set lines {}
		foreach line [split $title "\n"] {
		    set line [strRange $win $line $labelFont \
			      $lessPixels $alignment $data(-snipstring)]
		    if {[string compare $alignment "right"] == 0} {
			lappend lines $spaces$line
		    } else {
			lappend lines $line$spaces
		    }
		}
		set text [join $lines "\n"]
	    }
	    $w configure -text $text
	} elseif {[string compare $title ""] == 0} {	;# image w/o text
	    $w configure -text ""
	    if {$imageWidth + $spacePixels <= $pixels} {
		set text $spaces
		$w-tl configure -text $text
		$w-il configure -width $imageWidth
	    } elseif {$spacePixels < $pixels} {
		set text $spaces
		$w-tl configure -text $text
		$w-il configure -width [expr {$pixels - $spacePixels}]
	    } else {
		set imageWidth 0			;# can't disp. the image
		set text ""
	    }
	} else {					;# both image and text
	    $w configure -text ""
	    set gap [font measure $labelFont -displayof $win " "]
	    if {$imageWidth + $gap + $spacePixels <= $pixels} {
		incr lessPixels -[expr {$imageWidth + $gap}]
		set lines {}
		foreach line [split $title "\n"] {
		    set line [strRange $win $line $labelFont \
			      $lessPixels $alignment $data(-snipstring)]
		    if {[string compare $alignment "right"] == 0} {
			lappend lines "$spaces$line "
		    } else {
			lappend lines " $line$spaces"
		    }
		}
		set text [join $lines "\n"]
		$w-tl configure -text $text
		$w-il configure -width $imageWidth
	    } elseif {$imageWidth + $spacePixels <= $pixels} {	
		set text $spaces		;# can't display the orig. text
		$w-tl configure -text $text
		$w-il configure -width $imageWidth
	    } elseif {$spacePixels < $pixels} {
		set text $spaces		;# can't display the orig. text
		$w-tl configure -text $text
		$w-il configure -width [expr {$pixels - $spacePixels}]
	    } else {
		set imageWidth 0		;# can't display the image
		set text ""			;# can't display the text
	    }
	}
    }

    #
    # Place the label's sublabels (if any)
    #
    if {$imageWidth == 0} {
	if {[info exists data($col-labelimage)]} {
	    place forget $w-il
	    place forget $w-tl
	}
    } else {
	if {[string compare $text ""] == 0} {
	    place forget $w-tl
	}

	set margin $data(charWidth)
	switch $alignment {
	    left {
		place $w-il -in $w -anchor w -bordermode outside \
			    -relx 0.0 -x $margin -rely 0.499
		if {[string compare $text ""] != 0} {
		    if {$usingTile} {
			set padding [$w cget -padding]
			lset padding 0 [expr {$padX + [winfo reqwidth $w-il]}]
			$w configure -padding $padding -text $text
		    } else {
			set textX [expr {$margin + [winfo reqwidth $w-il]}]
			place $w-tl -in $w -anchor w -bordermode outside \
				    -relx 0.0 -x $textX -rely 0.499
		    }
		}
	    }

	    right {
		place $w-il -in $w -anchor e -bordermode outside \
			    -relx 1.0 -x -$margin -rely 0.499
		if {[string compare $text ""] != 0} {
		    if {$usingTile} {
			set padding [$w cget -padding]
			lset padding 2 [expr {$padX + [winfo reqwidth $w-il]}]
			$w configure -padding $padding -text $text
		    } else {
			set textX [expr {-$margin - [winfo reqwidth $w-il]}]
			place $w-tl -in $w -anchor e -bordermode outside \
				    -relx 1.0 -x $textX -rely 0.499
		    }
		}
	    }

	    center {
		if {[string compare $text ""] == 0} {
		    place $w-il -in $w -anchor center -relx 0.5 -x 0 -rely 0.499
		} else {
		    set reqWidth [expr {[winfo reqwidth $w-il] +
					[winfo reqwidth $w-tl]}]
		    set iX [expr {-$reqWidth/2}]
		    place $w-il -in $w -anchor w -relx 0.5 -x $iX -rely 0.499
		    if {$usingTile} {
			set padding [$w cget -padding]
			lset padding 0 [expr {$padX + [winfo reqwidth $w-il]}]
			$w configure -padding $padding -text $text
		    } else {
			set tX [expr {$reqWidth + $iX}]
			place $w-tl -in $w -anchor e -relx 0.5 -x $tX \
				    -rely 0.499
		    }
		}
	    }
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::computeColWidth
#
# Computes the width of the col'th column of the tablelist widget win to be just
# large enough to hold all the elements of the column (including its label).
#------------------------------------------------------------------------------
proc tablelist::computeColWidth {win col} {
    upvar ::tablelist::ns${win}::data data

    set fmtCmdFlag [info exists data($col-formatcommand)]

    set data($col-elemWidth) 0
    set data($col-widestCount) 0

    #
    # Column elements
    #
    foreach item $data(itemList) {
	if {$col >= [llength $item] - 1} {
	    continue
	}

	set key [lindex $item end]
	if {[info exists data($key-hide)]} {
	    continue
	}

	set text [lindex $item $col]
	if {$fmtCmdFlag} {
	    set text [uplevel #0 $data($col-formatcommand) [list $text]]
	}
	set text [strToDispStr $text]
	getAuxData $win $key $col auxType auxWidth
	set cellFont [getCellFont $win $key $col]
	set textWidth [getCellTextWidth $win $text $auxWidth $cellFont]
	set elemWidth [expr {$auxWidth + $textWidth}]
	if {$elemWidth == $data($col-elemWidth)} {
	    incr data($col-widestCount)
	} elseif {$elemWidth > $data($col-elemWidth)} {
	    set data($col-elemWidth) $elemWidth
	    set data($col-widestCount) 1
	}
    }
    set data($col-reqPixels) $data($col-elemWidth)

    #
    # Column label
    #
    computeLabelWidth $win $col
}

#------------------------------------------------------------------------------
# tablelist::computeLabelWidth
#
# Computes the width of the col'th label of the tablelist widget win and
# adjusts the column's width accordingly.
#------------------------------------------------------------------------------
proc tablelist::computeLabelWidth {win col} {
    upvar ::tablelist::ns${win}::data data

    set w $data(hdrTxtFrLbl)$col
    if {[info exists data($col-labelimage)]} {
	set netLabelWidth \
	    [expr {[winfo reqwidth $w-il] + [winfo reqwidth $w-tl]}]
    } else {							;# no image
	set netLabelWidth [expr {[winfo reqwidth $w] - 2*$data(charWidth)}]
    }

    if {$netLabelWidth < $data($col-elemWidth)} {
	set data($col-reqPixels) $data($col-elemWidth)
    } else {
	set data($col-reqPixels) $netLabelWidth
    }
}

#------------------------------------------------------------------------------
# tablelist::adjustHeaderHeight
#
# Sets the height of the header frame of the tablelist widget win to the max.
# height of its children.
#------------------------------------------------------------------------------
proc tablelist::adjustHeaderHeight win {
    upvar ::tablelist::ns${win}::data data

    #
    # Compute the max. label height
    #
    set maxLabelHeight [winfo reqheight $data(hdrLbl)]
    for {set col 0} {$col < $data(colCount)} {incr col} {
	set w $data(hdrTxtFrLbl)$col
	if {[string compare [winfo manager $w] ""] == 0} {
	    continue
	}

	set reqHeight [winfo reqheight $w]
	if {$reqHeight > $maxLabelHeight} {
	    set maxLabelHeight $reqHeight
	}

	foreach l [getSublabels $w] {
	    if {[string compare [winfo manager $l] ""] == 0} {
		continue
	    }

	    set borderWidth [winfo pixels $w [$w cget -borderwidth]]
	    if {$borderWidth < 0} {
		set borderWidth 0
	    }
	    set reqHeight [expr {[winfo reqheight $l] + 2*$borderWidth}]
	    if {$reqHeight > $maxLabelHeight} {
		set maxLabelHeight $reqHeight
	    }
	}
    }

    #
    # Set the height of the header frame, update
    # the colors, and adjust the separators
    #
    $data(hdrTxtFr) configure -height $maxLabelHeight
    if {$data(-showlabels)} {
	$data(hdr) configure -height $maxLabelHeight
    } else {
	$data(hdr) configure -height 1
    }
    updateColorsWhenIdle $win
    adjustSepsWhenIdle $win
}

#------------------------------------------------------------------------------
# tablelist::stretchColumnsWhenIdle
#
# Arranges for the stretchable columns of the tablelist widget win to be
# stretched at idle time.
#------------------------------------------------------------------------------
proc tablelist::stretchColumnsWhenIdle win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(stretchId)]} {
	return ""
    }

    set data(stretchId) [after idle [list tablelist::stretchColumns $win -1]]
}

#------------------------------------------------------------------------------
# tablelist::stretchColumns
#
# Stretches the stretchable columns to fill the tablelist window win
# horizontally.  The colOfFixedDelta argument specifies the column for which
# the stretching is to be made using a precomputed amount of pixels.
#------------------------------------------------------------------------------
proc tablelist::stretchColumns {win colOfFixedDelta} {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(stretchId)]} {
	after cancel $data(stretchId)
	unset data(stretchId)
    }

    set forceAdjust $data(forceAdjust)
    set data(forceAdjust) 0

    if {$data(hdrPixels) == 0 || $data(-width) <= 0} {
	return ""
    }

    #
    # Get the list data(stretchableCols) of the
    # numerical indices of the stretchable columns
    #
    set data(stretchableCols) {}
    if {[string first $data(-stretch) "all"] == 0} {
	for {set col 0} {$col < $data(colCount)} {incr col} {
	    lappend data(stretchableCols) $col
	}
    } else {
	foreach col $data(-stretch) {
	    lappend data(stretchableCols) [colIndex $win $col 0]
	}
    }

    #
    # Compute the total number data(delta) of pixels by which the
    # columns are to be stretched and the total amount
    # data(stretchablePixels) of stretchable column widths in pixels
    #
    set data(delta) [winfo width $data(hdr)]
    set data(stretchablePixels) 0
    set lastColToStretch -1
    set col 0
    foreach {pixels alignment} $data(colList) {
	if {$data($col-hide)} {
	    incr col
	    continue
	}

	if {$pixels == 0} {			;# convention: dynamic width
	    set pixels $data($col-reqPixels)
	    if {$data($col-maxPixels) > 0} {
		if {$pixels > $data($col-maxPixels)} {
		    set pixels $data($col-maxPixels)
		}
	    }
	}
	incr data(delta) -[expr {$pixels + 2*$data(charWidth)}]
	if {[lsearch -exact $data(stretchableCols) $col] >= 0} {
	    incr data(stretchablePixels) $pixels
	    set lastColToStretch $col
	}

	incr col
    }
    if {$data(delta) < 0} {
	set delta 0
    } else {
	set delta $data(delta)
    }
    if {$data(stretchablePixels) == 0 && !$forceAdjust} {
	return ""
    }

    #
    # Distribute the value of delta to the stretchable
    # columns, proportionally to their widths in pixels
    #
    set rest $delta
    set col 0
    foreach {pixels alignment} $data(colList) {
	if {$data($col-hide) ||
	    [lsearch -exact $data(stretchableCols) $col] < 0} {
	    set data($col-delta) 0
	} else {
	    set oldDelta $data($col-delta)
	    if {$pixels == 0} {			;# convention: dynamic width
		set dynamic 1
		set pixels $data($col-reqPixels)
		if {$data($col-maxPixels) > 0} {
		    if {$pixels > $data($col-maxPixels)} {
			set pixels $data($col-maxPixels)
			set dynamic 0
		    }
		}
	    } else {
		set dynamic 0
	    }
	    if {$data(stretchablePixels) == 0} {
		set data($col-delta) 0
	    } else {
		if {$col != $colOfFixedDelta} {
		    set data($col-delta) \
			[expr {$delta*$pixels/$data(stretchablePixels)}]
		}
		incr rest -$data($col-delta)
	    }
	    if {$col == $lastColToStretch} {
		incr data($col-delta) $rest
	    }
	    if {!$dynamic && $data($col-delta) != $oldDelta} {
		redisplayColWhenIdle $win $col
	    }
	}

	incr col
    }

    #
    # Adjust the columns
    #
    adjustColumns $win {} 0
}

#------------------------------------------------------------------------------
# tablelist::updateColorsWhenIdle
#
# Arranges for the background and foreground colors of the label and message
# widgets containing the currently visible images and multiline elements of the
# tablelist widget win to be updated at idle time.
#------------------------------------------------------------------------------
proc tablelist::updateColorsWhenIdle win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(colorId)]} {
	return ""
    }

    set data(colorId) [after idle [list tablelist::updateColors $win]]
}

#------------------------------------------------------------------------------
# tablelist::updateColors
#
# Updates the background and foreground colors of the label and message widgets
# containing the currently visible images and multiline elements of the
# tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::updateColors win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(colorId)]} {
	after cancel $data(colorId)
	unset data(colorId)
    }

    set w $data(body)
    set topLeftIdx [$w index @0,0]
    set btmRightIdx "[$w index @0,[expr {[winfo height $w] - 1}]] lineend"
    foreach {dummy path textIdx} [$w dump -window $topLeftIdx $btmRightIdx] {
	if {[string compare $path ""] == 0} {
	    continue
	}

	set class [winfo class $path]
	if {[string compare $class "Label"] != 0 &&
	    [string compare $class "Message"] != 0} {
	    continue
	}

	set name [winfo name $path]
	foreach {key col} [split [string range $name 1 end] ","] {}
	set tagNames [$w tag names $textIdx]

	#
	# Set the widget's background and foreground
	# colors to those of the containing cell
	#
	if {$data(isDisabled)} {
	    set bg $data(-background)
	    set fg $data(-disabledforeground)
	} elseif {[lsearch -exact $tagNames select] < 0} {	;# not selected
	    if {[info exists data($key,$col-background)]} {
		set bg $data($key,$col-background)
	    } elseif {[info exists data($key-background)]} {
		set bg $data($key-background)
	    } elseif {[lsearch -exact $tagNames stripe] < 0 ||
		      [string compare $data(-stripebackground) ""] == 0} {
		if {[info exists data($col-background)]} {
		    set bg $data($col-background)
		} else {
		    set bg $data(-background)
		}
	    } else {
		set bg $data(-stripebackground)
	    }

	    if {[info exists data($key,$col-foreground)]} {
		set fg $data($key,$col-foreground)
	    } elseif {[info exists data($key-foreground)]} {
		set fg $data($key-foreground)
	    } elseif {[lsearch -exact $tagNames stripe] < 0 ||
		      [string compare $data(-stripeforeground) ""] == 0} {
		if {[info exists data($col-foreground)]} {
		    set fg $data($col-foreground)
		} else {
		    set fg $data(-foreground)
		}
	    } else {
		set fg $data(-stripeforeground)
	    }
	} else {						;# selected
	    if {[info exists data($key,$col-selectbackground)]} {
		set bg $data($key,$col-selectbackground)
	    } elseif {[info exists data($key-selectbackground)]} {
		set bg $data($key-selectbackground)
	    } elseif {[info exists data($col-selectbackground)]} {
		set bg $data($col-selectbackground)
	    } else {
		set bg $data(-selectbackground)
	    }

	    if {[info exists data($key,$col-selectforeground)]} {
		set fg $data($key,$col-selectforeground)
	    } elseif {[info exists data($key-selectforeground)]} {
		set fg $data($key-selectforeground)
	    } elseif {[info exists data($col-selectforeground)]} {
		set fg $data($col-selectforeground)
	    } else {
		set fg $data(-selectforeground)
	    }
	}
	if {[string compare [$path cget -background] $bg] != 0} {
	    $path configure -background $bg
	}
	if {[string compare [$path cget -foreground] $fg] != 0} {
	    $path configure -foreground $fg
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::updateScrlColOffsetWhenIdle
#
# Arranges for the scrolled column offset of the tablelist widget win to be
# updated at idle time.
#------------------------------------------------------------------------------
proc tablelist::updateScrlColOffsetWhenIdle win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(offsetId)]} {
	return ""
    }

    set data(offsetId) [after idle [list tablelist::updateScrlColOffset $win]]
}

#------------------------------------------------------------------------------
# tablelist::updateScrlColOffset
#
# Updates the scrolled column offset of the tablelist widget win to fit into
# the allowed range.
#------------------------------------------------------------------------------
proc tablelist::updateScrlColOffset win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(offsetId)]} {
	after cancel $data(offsetId)
	unset data(offsetId)
    }

    set maxScrlColOffset [getMaxScrlColOffset $win]
    if {$data(scrlColOffset) > $maxScrlColOffset} {
	set data(scrlColOffset) $maxScrlColOffset
	adjustElidedTextWhenIdle $win
    }
}

#------------------------------------------------------------------------------
# tablelist::updateHScrlbarWhenIdle
#
# Arranges for the horizontal scrollbar associated with the tablelist widget
# win to be updated at idle time.
#------------------------------------------------------------------------------
proc tablelist::updateHScrlbarWhenIdle win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(hScrlbarId)]} {
	return ""
    }

    set data(hScrlbarId) [after idle [list tablelist::updateHScrlbar $win]]
}

#------------------------------------------------------------------------------
# tablelist::updateHScrlbar
#
# Updates the horizontal scrollbar associated with the tablelist widget win by
# invoking the command specified as the value of the -xscrollcommand option.
#------------------------------------------------------------------------------
proc tablelist::updateHScrlbar win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(hScrlbarId)]} {
	after cancel $data(hScrlbarId)
	unset data(hScrlbarId)
    }

    if {$data(-titlecolumns) > 0 &&
	[string compare $data(-xscrollcommand) ""] != 0} {
	eval $data(-xscrollcommand) [xviewSubCmd $win {}]
    }
}

#------------------------------------------------------------------------------
# tablelist::updateVScrlbarWhenIdle
#
# Arranges for the vertical scrollbar associated with the tablelist widget win
# to be updated at idle time.
#------------------------------------------------------------------------------
proc tablelist::updateVScrlbarWhenIdle win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(vScrlbarId)]} {
	return ""
    }

    set data(vScrlbarId) [after idle [list tablelist::updateVScrlbar $win]]
}

#------------------------------------------------------------------------------
# tablelist::updateVScrlbar
#
# Updates the vertical scrollbar associated with the tablelist widget win by
# invoking the command specified as the value of the -yscrollcommand option.
#------------------------------------------------------------------------------
proc tablelist::updateVScrlbar win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(vScrlbarId)]} {
	after cancel $data(vScrlbarId)
	unset data(vScrlbarId)
    }

    if {[string compare $data(-yscrollcommand) ""] != 0} {
	eval $data(-yscrollcommand) [yviewSubCmd $win {}]
    }
}

#------------------------------------------------------------------------------
# tablelist::adjustElidedTextWhenIdle
#
# Arranges for the elided text ranges of the body text child of the tablelist
# widget win to be updated at idle time.
#------------------------------------------------------------------------------
proc tablelist::adjustElidedTextWhenIdle win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(elidedId)]} {
	return ""
    }

    set data(elidedId) [after idle [list tablelist::adjustElidedText $win]]
}

#------------------------------------------------------------------------------
# tablelist::adjustElidedText
#
# Updates the elided text ranges of the body text child of the tablelist widget
# win.
#------------------------------------------------------------------------------
proc tablelist::adjustElidedText win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(elidedId)]} {
	after cancel $data(elidedId)
	unset data(elidedId)
    }

    #
    # Remove the "hiddenCol" tag
    #
    set w $data(body)
    $w tag remove hiddenCol 1.0 end

    #
    # Add the "hiddenCol" tag to the contents of the hidden
    # columns from the top to the bottom window line
    #
    variable canElide
    if {$canElide && $data(hiddenColCount) > 0 && $data(itemCount) > 0} {
	set btmY [expr {[winfo height $w] - 1}]
	set topLine [expr {int([$w index @0,0])}]
	set btmLine [expr {int([$w index @0,$btmY])}]
	for {set line $topLine; set row [expr {$line - 1}]} \
	    {$line <= $btmLine} {set row $line; incr line} {
	    set key [lindex [lindex $data(itemList) $row] end]
	    if {[info exists data($key-hide)]} {
		continue
	    }

	    set textIdx1 $line.0
	    for {set col 0; set count 0} \
		{$col < $data(colCount) && $count < $data(hiddenColCount)} \
		{incr col} {
		set textIdx2 \
		    [$w search -elide "\t" $textIdx1+1c $line.end]+1c
		if {$data($col-hide)} {
		    incr count
		    $w tag add hiddenCol $textIdx1 $textIdx2
		}
		set textIdx1 $textIdx2
	    }

	    #
	    # Update btmLine because it may
	    # change due to the "hiddenCol" tag
	    #
	    set btmLine [expr {int([$w index @0,$btmY])}]
	}

	if {[lindex [$w yview] 1] == 1} {
	    for {set line $btmLine; set row [expr {$line - 1}]} \
		{$line >= $topLine} {set line $row; incr row -1} {
		set key [lindex [lindex $data(itemList) $row] end]
		if {[info exists data($key-hide)]} {
		    continue
		}

		set textIdx1 $line.0
		for {set col 0; set count 0} \
		    {$col < $data(colCount) && $count < $data(hiddenColCount)} \
		    {incr col} {
		    set textIdx2 \
			[$w search -elide "\t" $textIdx1+1c $line.end]+1c
		    if {$data($col-hide)} {
			incr count
			$w tag add hiddenCol $textIdx1 $textIdx2
		    }
		    set textIdx1 $textIdx2
		}

		#
		# Update topLine because it may
		# change due to the "hiddenCol" tag
		#
		set topLine [expr {int([$w index @0,0])}]
	    }
	}
    }

    if {$data(-titlecolumns) == 0} {
	return ""
    }

    #
    # Remove the "elidedCol" tag
    #
    $w tag remove elidedCol 1.0 end
    for {set col 0} {$col < $data(colCount)} {incr col} {
	set data($col-elide) 0
    }

    if {$data(scrlColOffset) == 0} {
	adjustColumns $win {} 0
	return ""
    }

    #
    # Find max. $data(scrlColOffset) non-hidden columns with indices >=
    # $data(-titlecolumns) and retain the first and last of these indices
    #
    set firstCol $data(-titlecolumns)
    while {$firstCol < $data(colCount) && $data($firstCol-hide)} {
	incr firstCol
    }
    if {$firstCol >= $data(colCount)} {
	return ""
    }
    set lastCol $firstCol
    set nonHiddenCount 1
    while {$nonHiddenCount < $data(scrlColOffset) &&
	   $lastCol < $data(colCount)} {
	incr lastCol
	if {!$data($lastCol-hide)} {
	    incr nonHiddenCount
	}
    }

    #
    # Add the "elidedCol" tag to the contents of these
    # columns from the top to the bottom window line
    #
    if {$data(itemCount) > 0} {
	set btmY [expr {[winfo height $w] - 1}]
	set topLine [expr {int([$w index @0,0])}]
	set btmLine [expr {int([$w index @0,$btmY])}]
	for {set line $topLine; set row [expr {$line - 1}]} \
	    {$line <= $btmLine} {set row $line; incr line} {
	    set key [lindex [lindex $data(itemList) $row] end]
	    if {![info exists data($key-hide)]} {
		findTabs $win $line $firstCol $lastCol tabIdx1 tabIdx2
		$w tag add elidedCol $tabIdx1 $tabIdx2+1c
	    }

	    #
	    # Update btmLine because it may
	    # change due to the "elidedCol" tag
	    #
	    set btmLine [expr {int([$w index @0,$btmY])}]
	}

	if {[lindex [$w yview] 1] == 1} {
	    for {set line $btmLine; set row [expr {$line - 1}]} \
		{$line >= $topLine} {set line $row; incr row -1} {
		set key [lindex [lindex $data(itemList) $row] end]
		if {![info exists data($key-hide)]} {
		    findTabs $win $line $firstCol $lastCol tabIdx1 tabIdx2
		    $w tag add elidedCol $tabIdx1 $tabIdx2+1c
		}

		#
		# Update topLine because it may
		# change due to the "elidedCol" tag
		#
		set topLine [expr {int([$w index @0,0])}]
	    }
	}
    }

    #
    # Adjust the columns
    #
    for {set col $firstCol} {$col <= $lastCol} {incr col} {
	set data($col-elide) 1
    }
    adjustColumns $win {} 0
}

#------------------------------------------------------------------------------
# tablelist::redisplayWhenIdle
#
# Arranges for the items of the tablelist widget win to be redisplayed at idle
# time.
#------------------------------------------------------------------------------
proc tablelist::redisplayWhenIdle win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(redispId)] || $data(itemCount) == 0} {
	return ""
    }

    set data(redispId) [after idle [list tablelist::redisplay $win]]

    #
    # Cancel the execution of all delayed redisplayCol commands
    #
    foreach name [array names data *-redispId] {
	after cancel $data($name)
	unset data($name)
    }
}

#------------------------------------------------------------------------------
# tablelist::redisplay
#
# Redisplays the items of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::redisplay {win {getSelCells 1} {selCells {}}} {
    variable canElide
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(redispId)]} {
	after cancel $data(redispId)
	unset data(redispId)
    }

    #
    # Save the indices of the selected cells
    #
    if {$getSelCells} {
	set selCells [curcellselectionSubCmd $win]
    }

    #
    # Save some data of the edit window if present
    #
    if {[set editCol $data(editCol)] >= 0} {
	set editRow $data(editRow)
	saveEditData $win
    }

    set w $data(body)
    set snipStr $data(-snipstring)
    set tagRefCount $data(tagRefCount)
    set isSimple [expr {$data(imgCount) == 0 && $data(winCount) == 0}]
    set newItemList {}
    set row 0
    set line 1
    foreach item $data(itemList) {
	#
	# Empty the line, clip the elements if necessary,
	# and insert them with the corresponding tags
	#
	$w delete $line.0 $line.end
	set keyIdx [expr {[llength $item] - 1}]
	set key [lindex $item end]
	set newItem {}
	set col 0
	if {$isSimple} {
	    set insertArgs {}
	    set multilineData {}
	    foreach fmtCmdFlag $data(fmtCmdFlagList) \
		    colFont $data(colFontList) \
		    colTags $data(colTagsList) \
		    {pixels alignment} $data(colList) {
		if {$col < $keyIdx} {
		    set text [lindex $item $col]
		} else {
		    set text ""
		}
		lappend newItem $text

		if {$data($col-hide) && !$canElide} {
		    incr col
		    continue
		}

		if {$fmtCmdFlag} {
		    set text [uplevel #0 $data($col-formatcommand) [list $text]]
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
		set text [strToDispStr $text]
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
		    lappend multilineData $col $text $cellFont $alignment
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
	    foreach fmtCmdFlag $data(fmtCmdFlagList) \
		    colTags $data(colTagsList) \
		    {pixels alignment} $data(colList) {
		if {$col < $keyIdx} {
		    set text [lindex $item $col]
		} else {
		    set text ""
		}
		lappend newItem $text

		if {$data($col-hide) && !$canElide} {
		    incr col
		    continue
		}

		if {$fmtCmdFlag} {
		    set text [uplevel #0 $data($col-formatcommand) [list $text]]
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

	lappend newItem $key
	lappend newItemList $newItem

	set row $line
	incr line
    }

    set data(itemList) $newItemList

    #
    # Select the cells that were selected before
    #
    foreach cellIdx $selCells {
	scan $cellIdx "%d,%d" row col
	if {$col < $data(colCount)} {
	    cellselectionSubCmd $win set $row $col $row $col
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
}

#------------------------------------------------------------------------------
# tablelist::redisplayColWhenIdle
#
# Arranges for the elements of the col'th column of the tablelist widget win to
# be redisplayed at idle time.
#------------------------------------------------------------------------------
proc tablelist::redisplayColWhenIdle {win col} {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data($col-redispId)] || [info exists data(redispId)] ||
	$data(itemCount) == 0} {
	return ""
    }

    set data($col-redispId) \
	[after idle [list tablelist::redisplayCol $win $col 0 end]]
}

#------------------------------------------------------------------------------
# tablelist::redisplayCol
#
# Redisplays the elements of the col'th column of the tablelist widget win, in
# the range specified by first and last.
#------------------------------------------------------------------------------
proc tablelist::redisplayCol {win col first last} {
    upvar ::tablelist::ns${win}::data data

    if {$first == 0 && [string first $last "end"] == 0 &&
	[info exists data($col-redispId)]} {
	after cancel $data($col-redispId)
	unset data($col-redispId)
    }

    if {$data(itemCount) == 0 || $data($col-hide) || $first < 0} {
	return ""
    }
    if {[string first $last "end"] == 0} {
	set last $data(lastRow)
    }

    set snipStr $data(-snipstring)
    set fmtCmdFlag [info exists data($col-formatcommand)]

    set w $data(body)
    set pixels [lindex $data(colList) [expr {2*$col}]]
    if {$pixels == 0} {				;# convention: dynamic width
	if {$data($col-maxPixels) > 0} {
	    if {$data($col-reqPixels) > $data($col-maxPixels)} {
		set pixels $data($col-maxPixels)
	    }
	}
    }
    if {$pixels != 0} {
	incr pixels $data($col-delta)
    }
    set alignment [lindex $data(colList) [expr {2*$col + 1}]]

    for {set row $first; set line [expr {$first + 1}]} {$row <= $last} \
	{set row $line; incr line} {
	if {$row == $data(editRow) && $col == $data(editCol)} {
	    continue
	}

	#
	# Adjust the cell text and the image or window width
	#
	set item [lindex $data(itemList) $row]
	set text [lindex $item $col]
	if {$fmtCmdFlag} {
	    set text [uplevel #0 $data($col-formatcommand) [list $text]]
	}
	set text [strToDispStr $text]
	if {[string match "*\n*" $text]} {
	    set multiline 1
	    set list [split $text "\n"]
	} else {
	    set multiline 0
	}
	set key [lindex $item end]
	set aux [getAuxData $win $key $col auxType auxWidth]
	set cellFont [getCellFont $win $key $col]
	if {$multiline} {
	    adjustMlElem $win list auxWidth $cellFont \
			 $pixels $alignment $snipStr
	    set msgScript [list ::tablelist::displayText $win $key \
			   $col [join $list "\n"] $cellFont $alignment]
	} else {
	    adjustElem $win text auxWidth $cellFont $pixels $alignment $snipStr
	}

	#
	# Update the text widget's contents between the two tabs
	#
	findTabs $win $line $col $col tabIdx1 tabIdx2
	if {$multiline} {
	    updateMlCell $w $tabIdx1+1c $tabIdx2 $msgScript \
			 $aux $auxType $auxWidth $alignment
	} else {
	    updateCell $w $tabIdx1+1c $tabIdx2 $text \
		       $aux $auxType $auxWidth $alignment
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::makeStripesWhenIdle
#
# Arranges for the stripes in the body of the tablelist widget win to be
# redrawn at idle time.
#------------------------------------------------------------------------------
proc tablelist::makeStripesWhenIdle win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(stripesId)] || $data(itemCount) == 0} {
	return ""
    }

    set data(stripesId) [after idle [list tablelist::makeStripes $win]]
}

#------------------------------------------------------------------------------
# tablelist::makeStripes
#
# Redraws the stripes in the body of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::makeStripes win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(stripesId)]} {
	after cancel $data(stripesId)
	unset data(stripesId)
    }

    set w $data(body)
    $w tag remove stripe 1.0 end
    if {[string compare $data(-stripebackground) ""] != 0 ||
	[string compare $data(-stripeforeground) ""] != 0} {
	set count 0
	set inStripe 0
	for {set row 0; set line 1} {$row < $data(itemCount)} \
	    {set row $line; incr line} {
	    set key [lindex [lindex $data(itemList) $row] end]
	    if {![info exists data($key-hide)]} {
		if {$inStripe} {
		    $w tag add stripe $line.0 $line.end
		}

		if {[incr count] == $data(-stripeheight)} {
		    set count 0
		    set inStripe [expr {!$inStripe}]
		}
	    }
	}
    }

    updateColors $win
}

#------------------------------------------------------------------------------
# tablelist::showLineNumbersWhenIdle
#
# Arranges for the line numbers in the tablelist widget win to be redisplayed
# at idle time.
#------------------------------------------------------------------------------
proc tablelist::showLineNumbersWhenIdle win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(lineNumsId)]} {
	return ""
    }

    set data(lineNumsId) [after idle [list tablelist::showLineNumbers $win]]
}

#------------------------------------------------------------------------------
# tablelist::showLineNumbers
#
# Redisplays the line numbers (if any) in the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::showLineNumbers win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(lineNumsId)]} {
	after cancel $data(lineNumsId)
	unset data(lineNumsId)
    }

    #
    # Update the item list
    #
    set colIdxList {}
    for {set col 0} {$col < $data(colCount)} {incr col} {
	if {!$data($col-showlinenumbers)} {
	    continue
	}

	lappend colIdxList $col

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

	redisplayColWhenIdle $win $col
    }

    if {[llength $colIdxList] == 0} {
	return ""
    }

    #
    # Update the list variable if present
    #
    condUpdateListVar $win

    #
    # Adjust the columns
    #
    adjustColumns $win $colIdxList 1
    return ""
}

#------------------------------------------------------------------------------
# tablelist::synchronize
#
# This procedure is invoked either as an idle callback after the list variable
# associated with the tablelist widget win was written, or directly, upon
# execution of some widget commands.  It makes sure that the content of the
# widget is synchronized with the value of the list variable.
#------------------------------------------------------------------------------
proc tablelist::synchronize win {
    upvar ::tablelist::ns${win}::data data

    #
    # Nothing to do if the list variable was not written
    #
    if {![info exists data(syncId)]} {
	return ""
    }

    #
    # Here we are in the case that the procedure was scheduled for
    # execution at idle time.  However, it might have been invoked
    # directly, before the idle time occured; in this case we should
    # cancel the execution of the previously scheduled idle callback.
    #
    after cancel $data(syncId)	;# no harm if data(syncId) is no longer valid
    unset data(syncId)

    upvar #0 $data(-listvariable) var
    set newCount [llength $var]
    if {$newCount < $data(itemCount)} {
	#
	# Delete the items with indices >= newCount from the widget
	#
	set updateCount $newCount
	deleteRows $win $newCount $data(lastRow) 0
    } elseif {$newCount > $data(itemCount)} {
	#
	# Insert the items of var with indices
	# >= data(itemCount) into the widget
	#
	set updateCount $data(itemCount)
	insertSubCmd $win $data(itemCount) [lrange $var $data(itemCount) end] 0
    } else {
	set updateCount $newCount
    }

    #
    # Update the first updateCount items of the internal list
    #
    set itemsChanged 0
    for {set row 0} {$row < $updateCount} {incr row} {
	set oldItem [lindex $data(itemList) $row]
	set newItem [adjustItem [lindex $var $row] $data(colCount)]
	lappend newItem [lindex $oldItem end]

	if {[string compare $oldItem $newItem] != 0} {
	    set data(itemList) [lreplace $data(itemList) $row $row $newItem]
	    set itemsChanged 1
	}
    }

    #
    # If necessary, adjust the columns and make sure
    # that the items will be redisplayed at idle time
    #
    if {$itemsChanged} {
	adjustColumns $win allCols 1
	redisplayWhenIdle $win
    }
}

#------------------------------------------------------------------------------
# tablelist::getSublabels
#
# Returns the list of the existing sublabels $w-il and $w-tl associated with
# the label widget w.
#------------------------------------------------------------------------------
proc tablelist::getSublabels w {
    set lst {}
    foreach lbl [list $w-il $w-tl] {
	if {[winfo exists $lbl]} {
	    lappend lst $lbl
	}
    }

    return $lst
}

#------------------------------------------------------------------------------
# tablelist::parseLabelPath
#
# Extracts the path name of the tablelist widget as well as the column number
# from the path name w of a header label.
#------------------------------------------------------------------------------
proc tablelist::parseLabelPath {w winName colName} {
    upvar $winName win $colName col
    return [regexp {^(.+)\.hdr\.t\.f\.l([0-9]+)$} $w dummy win col]
}

#------------------------------------------------------------------------------
# tablelist::configLabel
#
# This procedure configures the label widget w according to the options and
# their values given in args.  It is needed for label widgets with sublabels.
#------------------------------------------------------------------------------
proc tablelist::configLabel {w args} {
    foreach {opt val} $args {
	switch -- $opt {
	    -active {
		if {[string compare [winfo class $w] "TLabel"] == 0} {
		    set state [expr {$val ? "active" : "!active"}]
		    $w state $state
		    if {$val} {
			variable themeDefaults
			set bg $themeDefaults(-labelactiveBg)
		    } else {
			set bg [$w cget -background]
		    }
		    foreach l [getSublabels $w] {
			$l configure -background $bg
		    }
		} else {
		    set state [expr {$val ? "active" : "normal"}]
		    catch {
			$w configure -state $state
			foreach l [getSublabels $w] {
			    $l configure -state $state
			}
		    }
		}

		parseLabelPath $w win col
		upvar ::tablelist::ns${win}::data data
		if {[lsearch -exact $data(arrowColList) $col] >= 0} {
		    configCanvas $win $col
		}
	    }

	    -activebackground -
	    -activeforeground -
	    -disabledforeground {
		$w configure $opt $val
		foreach l [getSublabels $w] {
		    $l configure $opt $val
		}
	    }

	    -background -
	    -foreground -
	    -font {
		if {[string compare $val ""] == 0 &&
		    [string compare [winfo class $w] "TLabel"] == 0} {
		    variable themeDefaults
		    set val $themeDefaults(-label[string range $opt 1 end])
		}
		$w configure $opt $val
		foreach l [getSublabels $w] {
		    $l configure $opt $val
		}
	    }

	    -padx {
		if {[string compare [winfo class $w] "TLabel"] == 0} {
		    set padding [$w cget -padding]
		    $w configure -padding \
			[list $val [lindex $padding 1] $val [lindex $padding 3]]
		} else {
		    $w configure $opt $val
		}
	    }

	    -pady {
		if {[string compare [winfo class $w] "TLabel"] == 0} {
		    set val [winfo pixels $w $val]
		    set padding [$w cget -padding]
		    $w configure -padding \
			[list [lindex $padding 0] $val [lindex $padding 2] $val]
		} else {
		    $w configure $opt $val
		}
	    }

	    -pressed {
		if {[string compare [winfo class $w] "TLabel"] == 0} {
		    set state [expr {$val ? "pressed" : "!pressed"}]
		    $w state $state
		    variable themeDefaults
		    if {$val} {
			set bg $themeDefaults(-labelpressedBg)
		    } else {
			set bg $themeDefaults(-labelactiveBg)
		    }
		    foreach l [getSublabels $w] {
			$l configure -background $bg
		    }

		    parseLabelPath $w win col
		    upvar ::tablelist::ns${win}::data data
		    if {[lsearch -exact $data(arrowColList) $col] >= 0} {
			configCanvas $win $col
		    }
		}
	    }

	    -state {
		$w configure $opt $val
		if {[string compare [winfo class $w] "TLabel"] == 0} {
		    if {[string compare $val "disabled"] == 0} {
			variable themeDefaults
			set bg $themeDefaults(-labeldisabledBg)
		    } else {
			set bg [$w cget -background]
		    }
		    foreach l [getSublabels $w] {
			$l configure -background $bg
		    }
		} else {
		    foreach l [getSublabels $w] {
			$l configure $opt $val
		    }
		}
	    }

	    default {
		if {[string compare $val [$w cget $opt]] != 0} {
		    $w configure $opt $val
		}
	    }
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::createArrows
#
# Creates two arrows in the canvas w.
#------------------------------------------------------------------------------
proc tablelist::createArrows {w width height relief} {
    if {$height < 6} {
	set wHeight 6
	set y 1
    } else {
	set wHeight $height
	set y 0
    }

    $w configure -width $width -height $wHeight

    #
    # Delete any existing arrow image items from
    # the canvas and the corresponding images
    #
    foreach shape {triangleUp darkLineUp lightLineUp
		   triangleDn darkLineDn lightLineDn} {
	$w delete $shape
	catch {image delete $shape$w}
    }

    #
    # Create the arrow images and canvas image items
    # corresponding to the procedure's arguments
    #
    $relief${width}x${height}Arrows $w
    foreach shape {triangleUp darkLineUp lightLineUp
		   triangleDn darkLineDn lightLineDn} {
	catch {$w create image 0 $y -anchor nw -image $shape$w -tags $shape}
    }

    #
    # Create the sort rank image item
    #
    $w delete sortRank
    set x [expr {$width + 2}]
    set y [expr {$wHeight - 6}]
    $w create image $x $y -anchor nw -tags sortRank
}

#------------------------------------------------------------------------------
# tablelist::configCanvas
#
# Sets the background color of the canvas displaying an up- or down-arrow for
# the given column, and fills the two arrows contained in the canvas.
#------------------------------------------------------------------------------
proc tablelist::configCanvas {win col} {
    upvar ::tablelist::ns${win}::data data

    set w $data(hdrTxtFrLbl)$col
    set labelBg [$w cget -background]
    set labelFg [$w cget -foreground]

    if {[string compare [winfo class $w] "TLabel"] == 0} {
	variable themeDefaults
	foreach state {disabled active pressed} {
	    $w instate $state {
		set labelBg $themeDefaults(-label${state}Bg)
		set labelFg $themeDefaults(-label${state}Fg)
	    }
	}
    } else {
	catch {
	    set state [$w cget -state]
	    variable winSys
	    if {[string compare $state "disabled"] == 0} {
		set labelFg [$w cget -disabledforeground]
	    } elseif {[string compare $state "active"] == 0 &&
		      [string compare $winSys "classic"] != 0 &&
		      [string compare $winSys "aqua"] != 0} {
		set labelBg [$w cget -activebackground]
		set labelFg [$w cget -activeforeground]
	    }
	}
    }

    set w $data(hdrTxtFrCanv)$col
    $w configure -background $labelBg
    sortRank$data($col-sortRank)$win configure -foreground $labelFg

    if {$data(isDisabled)} {
	fillArrows $w $data(-arrowdisabledcolor)
    } else {
	fillArrows $w $data(-arrowcolor)
    }
}

#------------------------------------------------------------------------------
# tablelist::fillArrows
#
# Fills the two arrows contained in the canvas w with the given color, or with
# the background color of the canvas if color is an empty string.  Also fills
# the arrow's borders with the corresponding 3-D shadow colors.
#------------------------------------------------------------------------------
proc tablelist::fillArrows {w color} {
    set bgColor [$w cget -background]
    if {[string compare $color ""] == 0} {
	set color $bgColor
    }

    getShadows $w $color darkColor lightColor

    foreach dir {Up Dn} {
	triangle$dir$w configure -foreground $color -background $bgColor
	catch {
	    darkLine$dir$w  configure -foreground $darkColor
	    lightLine$dir$w configure -foreground $lightColor
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::getShadows
#
# Computes the shadow colors for a 3-D border from a given (background) color.
# This is the Tcl-counterpart of the function TkpGetShadows() in the Tk
# distribution file unix/tkUnix3d.c.
#------------------------------------------------------------------------------
proc tablelist::getShadows {w color darkColorName lightColorName} {
    upvar $darkColorName darkColor $lightColorName lightColor

    set rgb [winfo rgb $w $color]
    foreach {r g b} $rgb {}
    set maxIntens [lindex [winfo rgb $w white] 0]

    #
    # Compute the dark shadow color
    #
    if {[string compare $::tk_patchLevel "8.3.1"] >= 0 &&
	$r*0.5*$r + $g*1.0*$g + $b*0.28*$b < $maxIntens*0.05*$maxIntens} {
	#
	# The background is already very dark: make the dark
	# color a little lighter than the background by increasing
	# each color component 1/4th of the way to $maxIntens
	#
	foreach comp $rgb {
	    lappend darkRGB [expr {($maxIntens + 3*$comp)/4}]
	}
    } else {
	#
	# Compute the dark color by cutting 40% from
	# each of the background color components.
	#
	foreach comp $rgb {
	    lappend darkRGB [expr {60*$comp/100}]
	}
    }
    set darkColor [eval format "#%04x%04x%04x" $darkRGB]

    #
    # Compute the light shadow color
    #
    if {[string compare $::tk_patchLevel "8.3.1"] >= 0 &&
	$g > $maxIntens*0.95} {
	#
	# The background is already very bright: make the
	# light color a little darker than the background
	# by reducing each color component by 10%
	#
	foreach comp $rgb {
	    lappend lightRGB [expr {90*$comp/100}]
	}
    } else {
	#
	# Compute the light color by boosting each background
	# color component by 40% or half-way to white, whichever
	# is greater (the first approach works better for
	# unsaturated colors, the second for saturated ones)
	#
	foreach comp $rgb {
	    set comp1 [expr {140*$comp/100}]
	    if {$comp1 > $maxIntens} {
		set comp1 $maxIntens
	    }
	    set comp2 [expr {($maxIntens + $comp)/2}]
	    lappend lightRGB [expr {($comp1 > $comp2) ? $comp1 : $comp2}]
	}
    }
    set lightColor [eval format "#%04x%04x%04x" $lightRGB]
}

#------------------------------------------------------------------------------
# tablelist::raiseArrow
#
# Raises one of the two arrows contained in the canvas associated with the
# given column of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::raiseArrow {win col} {
    variable directions
    upvar ::tablelist::ns${win}::data data

    set w $data(hdrTxtFrCanv)$col
    set dir $directions($data(-incrarrowtype),$data($col-sortOrder))

    $w raise triangle$dir
    $w raise darkLine$dir
    $w raise lightLine$dir
}

#------------------------------------------------------------------------------
# tablelist::isHdrTxtFrXPosVisible
#
# Checks whether the given x position in the header text child of the tablelist
# widget win is visible.
#------------------------------------------------------------------------------
proc tablelist::isHdrTxtFrXPosVisible {win x} {
    upvar ::tablelist::ns${win}::data data

    foreach {fraction1 fraction2} [$data(hdrTxt) xview] {}
    return [expr {$x >= $fraction1 * $data(hdrPixels) &&
		  $x <  $fraction2 * $data(hdrPixels)}]
}

#------------------------------------------------------------------------------
# tablelist::getColWidth
#
# Returns the displayed width of the specified column of the tablelist widget
# win.
#------------------------------------------------------------------------------
proc tablelist::getColWidth {win col} {
    upvar ::tablelist::ns${win}::data data

    set pixels [lindex $data(colList) [expr {2*$col}]]
    if {$pixels == 0} {				;# convention: dynamic width
	set pixels $data($col-reqPixels)
	if {$data($col-maxPixels) > 0} {
	    if {$pixels > $data($col-maxPixels)} {
		set pixels $data($col-maxPixels)
	    }
	}
    }

    return [expr {$pixels + $data($col-delta) + 2*$data(charWidth)}]
}

#------------------------------------------------------------------------------
# tablelist::getScrlContentWidth
#
# Returns the total width of the non-hidden scrollable columns of the tablelist
# widget win, in the specified range.
#------------------------------------------------------------------------------
proc tablelist::getScrlContentWidth {win scrlColOffset lastCol} {
    upvar ::tablelist::ns${win}::data data

    set scrlContentWidth 0
    set nonHiddenCount 0
    for {set col $data(-titlecolumns)} {$col <= $lastCol} {incr col} {
	if {!$data($col-hide) && [incr nonHiddenCount] > $scrlColOffset} {
	    incr scrlContentWidth [getColWidth $win $col]
	}
    }

    return $scrlContentWidth
}

#------------------------------------------------------------------------------
# tablelist::getScrlWindowWidth
#
# Returns the number of pixels obtained by subtracting the widths of the non-
# hidden title columns from the width of the header frame of the tablelist
# widget win.
#------------------------------------------------------------------------------
proc tablelist::getScrlWindowWidth win {
    upvar ::tablelist::ns${win}::data data

    set scrlWindowWidth [winfo width $data(hdr)]
    for {set col 0} {$col < $data(-titlecolumns) && $col < $data(colCount)} \
	{incr col} {
	if {!$data($col-hide)} {
	    incr scrlWindowWidth -[getColWidth $win $col]
	}
    }

    return $scrlWindowWidth
}

#------------------------------------------------------------------------------
# tablelist::getMaxScrlColOffset
#
# Returns the max. scrolled column offset of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::getMaxScrlColOffset win {
    upvar ::tablelist::ns${win}::data data

    #
    # Get the number of non-hidden scrollable columns
    #
    set maxScrlColOffset 0
    for {set col $data(-titlecolumns)} {$col < $data(colCount)} {incr col} {
	if {!$data($col-hide)} {
	    incr maxScrlColOffset
	}
    }

    #
    # Decrement maxScrlColOffset while the total width of the
    # non-hidden scrollable columns starting with this offset
    # is less than the width of the window's scrollable part
    #
    set scrlWindowWidth [getScrlWindowWidth $win]
    if {$scrlWindowWidth > 0} {
	while {$maxScrlColOffset > 0} {
	    incr maxScrlColOffset -1
	    set scrlContentWidth \
		[getScrlContentWidth $win $maxScrlColOffset $data(lastCol)]
	    if {$scrlContentWidth == $scrlWindowWidth} {
		break
	    } elseif {$scrlContentWidth > $scrlWindowWidth} {
		incr maxScrlColOffset
		break
	    }
	}
    }

    return $maxScrlColOffset
}

#------------------------------------------------------------------------------
# tablelist::changeScrlColOffset
#
# Changes the scrolled column offset of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::changeScrlColOffset {win scrlColOffset} {
    upvar ::tablelist::ns${win}::data data

    #
    # Make sure the offset is non-negative and no
    # greater than the max. scrolled column offset
    #
    if {$scrlColOffset < 0} {
	set scrlColOffset 0
    } else {
	set maxScrlColOffset [getMaxScrlColOffset $win]
	if {$scrlColOffset > $maxScrlColOffset} {
	    set scrlColOffset $maxScrlColOffset
	}
    }

    #
    # Update data(scrlColOffset) and adjust the
    # elided text in the tablelist's body if necessary
    #
    if {$scrlColOffset != $data(scrlColOffset)} {
	set data(scrlColOffset) $scrlColOffset
	adjustElidedText $win
    }
}

#------------------------------------------------------------------------------
# tablelist::scrlXOffsetToColOffset
#
# Returns the scrolled column offset of the tablelist widget win, corresponding
# to the desired x offset.
#------------------------------------------------------------------------------
proc tablelist::scrlXOffsetToColOffset {win scrlXOffset} {
    upvar ::tablelist::ns${win}::data data

    set scrlColOffset 0
    set scrlContentWidth 0
    for {set col $data(-titlecolumns)} {$col < $data(colCount)} {incr col} {
	if {$data($col-hide)} {
	    continue
	}

	incr scrlContentWidth [getColWidth $win $col]
	if {$scrlContentWidth > $scrlXOffset} {
	    break
	} else {
	    incr scrlColOffset
	}
    }

    return $scrlColOffset
}

#------------------------------------------------------------------------------
# tablelist::scrlColOffsetToXOffset
#
# Returns the x offset corresponding to the specified scrolled column offset of
# the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::scrlColOffsetToXOffset {win scrlColOffset} {
    upvar ::tablelist::ns${win}::data data

    set scrlXOffset 0
    set nonHiddenCount 0
    for {set col $data(-titlecolumns)} {$col < $data(colCount)} {incr col} {
	if {$data($col-hide)} {
	    continue
	}

	if {[incr nonHiddenCount] > $scrlColOffset} {
	    break
	} else {
	    incr scrlXOffset [getColWidth $win $col]
	}
    }

    return $scrlXOffset
}

#------------------------------------------------------------------------------
# tablelist::getNonHiddenRowCount
#
# Returns the number of non-hidden rows of the tablelist widget win in the
# specified range.
#------------------------------------------------------------------------------
proc tablelist::getNonHiddenRowCount {win first last} {
    upvar ::tablelist::ns${win}::data data

    if {$data(hiddenRowCount) == 0} {
	return [expr {$last - $first + 1}]
    } else {
	set count 0
	for {set row $first} {$row <= $last} {incr row} {
	    set key [lindex [lindex $data(itemList) $row] end]
	    if {![info exists data($key-hide)]} {
		incr count
	    }
	}
    }

    return $count
}

#------------------------------------------------------------------------------
# tablelist::nonHiddenRowOffsetToRowIndex
#
# Returns the row index corresponding to the given non-hidden row offset in the
# tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::nonHiddenRowOffsetToRowIndex {win offset} {
    upvar ::tablelist::ns${win}::data data

    if {$data(hiddenRowCount) == 0} {
	return $offset
    } else {
	#
	# Rebuild the list data(nonHiddenRowList) of the row
	# indices indicating the non-hidden rows if needed
	#
	if {[lindex $data(nonHiddenRowList) 0] == -1} {
	    set data(nonHiddenRowList) {}
	    for {set row 0} {$row < $data(itemCount)} {incr row} {
		set key [lindex [lindex $data(itemList) $row] end]
		if {![info exists data($key-hide)]} {
		    lappend data(nonHiddenRowList) $row
		}
	    }
	}

	set nonHiddenCount [llength $data(nonHiddenRowList)]
	if {$nonHiddenCount == 0} {
	    return 0
	} else {
	    if {$offset >= $nonHiddenCount} {
		set offset [expr {$nonHiddenCount - 1}]
	    }
	    if {$offset < 0} {
		set offset 0
	    }
	    return [lindex $data(nonHiddenRowList) $offset]
	}
    }
}
