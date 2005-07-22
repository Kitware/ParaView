#==============================================================================
# Contains private utility procedures for tablelist widgets.
#
# Copyright (c) 2000-2005  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================

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
    } elseif {[string compare [string index $idx 0] "@"] == 0} {
	if {[catch {$data(body) index $idx}] == 0} {
	    if {$data(itemCount) == 0} {
		return -1
	    } else {
		scan $idx "@%d,%d" x y
		incr x -[winfo x $data(body)]
		incr y -[winfo y $data(body)]
		set textIdx [$data(body) index @$x,$y]
		return [expr {int($textIdx) - 1}]
	    }
	} else {
	    return -code error \
		   "bad row index \"$idx\": must be active, anchor,\
		    end, @x,y, a number, or a full key"
	}
    } elseif {[string compare [string index $idx 0] "k"] == 0} {
	if {[set index [lsearch $data(itemList) "* $idx"]] >= 0} {
	    return $index
	} else {
	    return -code error \
		   "bad row index \"$idx\": must be active, anchor,\
		    end, @x,y, a number, or a full key"
	}
    } elseif {[catch {format "%d" $idx} index] == 0} {
	return $index
    } else {
	return -code error \
	       "bad row index \"$idx\": must be active, anchor,\
	        end, @x,y, a number, or a full key"
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
	    if {$data($col-hide) || $data($col-elided)} {
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
		    anchor, end, a number, or a full key, and col\
		    must be active, anchor, end, a number, or a name"
	}
    } else {
	set lst [split $idx ","]
	if {[llength $lst] != 2 ||
	    [catch {rowIndex $win [lindex $lst 0] 0} row] != 0 ||
	    [catch {colIndex $win [lindex $lst 1] 0} col] != 0} {
	    return -code error \
		   "bad cell index \"$idx\": must be active, anchor,\
		    end, @x,y, or row,col, where row must be active,\
		    anchor, end, a number, or a full key, and col\
		    must be active, anchor, end, a number, or a name"
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
# Sets the row index specified by $rowName to the index of the nearest row.
#------------------------------------------------------------------------------
proc tablelist::adjustRowIndex {win rowName} {
    upvar ::tablelist::ns${win}::data data
    upvar $rowName row

    if {$row > $data(lastRow)} {
	set row $data(lastRow)
    }
    if {$row < 0} {
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
    variable elide
    upvar ::tablelist::ns${win}::data data
    upvar $idx1Name idx1 $idx2Name idx2

    set w $data(body)
    set endIdx $line.end

    set idx1 $line.0
    for {set col 0} {$col < $firstCol} {incr col} {
	if {!$data($col-hide)} {
	    set idx1 [$w search $elide "\t" $idx1+1c $endIdx]+1c
	}
    }
    set idx1 [$w index $idx1]

    set idx2 $idx1
    for {} {$col < $lastCol} {incr col} {
	if {!$data($col-hide)} {
	    set idx2 [$w search $elide "\t" $idx2+1c $endIdx]+1c
	}
    }
    set idx2 [$w search $elide "\t" $idx2+1c $endIdx]

    return ""
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
    if {$data(arrowCol) == $col} {
	set data(arrowCol) -1
    }
    if {$data(sortCol) == $col} {
	set data(sortCol) -1
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
    # Remove the elements with names of the form k*-$col-*
    #
    foreach name [array names data k*-$col-*] {
	unset data($name)
	if {[string match "k*-$col-\[bf\]*" $name]} {
	    incr data(tagRefCount) -1
	} elseif {[string match "k*-$col-image" $name]} {
	    incr data(imgCount) -1
	} elseif {[string match "k*-$col-window" $name]} {
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

    foreach specialCol {activeCol anchorCol editCol arrowCol sortCol} {
	if {$oldArr($specialCol) == $oldCol} {
	    set newArr($specialCol) $newCol
	}
    }

    if {$newCol < $newArr(colCount)} {
	foreach c [winfo children $newArr(hdrTxtFrLbl)$newCol] {
	    destroy $c
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
    # Move the elements of oldArr with names of the form k*-$oldCol-*
    # to those of newArr with names of the form k*-$newCol-*
    #
    foreach newName [array names newArr k*-$newCol-*] {
	unset newArr($newName)
    }
    foreach oldName [array names oldArr k*-$oldCol-*] {
	regsub -- "-$oldCol-" $oldName "-$newCol-" newName
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
# Returns the largest initial (for alignment = left or center) or final (for
# alignment = right) range of characters from str whose width, when displayed
# in the given font, is no greater than pixels.
#------------------------------------------------------------------------------
proc tablelist::strRange {win str font pixels alignment} {
    if {[font measure $font -displayof $win $str] <= $pixels} {
	return $str
    }

    set halfLen [expr {[string length $str] / 2}]
    if {$halfLen == 0} {
	return ""
    }

    if {[string compare $alignment "right"] == 0} {
	set rightStr [string range $str $halfLen end]
	set width [font measure $font -displayof $win $rightStr]
	if {$width == $pixels} {
	    return $rightStr
	} elseif {$width > $pixels} {
	    return [strRange $win $rightStr $font $pixels $alignment]
	} else {
	    set str [string range $str 0 [expr {$halfLen - 1}]]
	    return [strRange $win $str $font \
		    [expr {$pixels - $width}] $alignment]$rightStr
	}
    } else {
	set leftStr [string range $str 0 [expr {$halfLen - 1}]]
	set width [font measure $font -displayof $win $leftStr]
	if {$width == $pixels} {
	    return $leftStr
	} elseif {$width > $pixels} {
	    return [strRange $win $leftStr $font $pixels $alignment]
	} else {
	    set str [string range $str $halfLen end]
	    return $leftStr[strRange $win $str $font \
			    [expr {$pixels - $width}] $alignment]
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::strRangeExt
#
# Invokes strRange with the given arguments and returns a string obtained by
# appending (for alignment = left or center) or prepending (for alignment =
# right) (part of) the snip string to (part of) its result.
#------------------------------------------------------------------------------
proc tablelist::strRangeExt {win str font pixels alignment snipStr} {
    set subStr [strRange $win $str $font $pixels $alignment]
    set len [string length $subStr]
    if {$pixels < 0 || $len == [string length $str] ||
	[string compare $snipStr ""] == 0} {
	return $subStr
    }

    if {[string compare $alignment "right"] == 0} {
	set extSubStr $snipStr$subStr
	while {[font measure $font -displayof $win $extSubStr] > $pixels} {
	    if {$len > 0} {
		set subStr [string range $subStr 1 end]
		incr len -1
		set extSubStr $snipStr$subStr
	    } else {
		set extSubStr [string range $extSubStr 1 end]
	    }
	}
    } else {
	set last [expr {$len - 1}]
	set extSubStr $subStr$snipStr
	while {[font measure $font -displayof $win $extSubStr] > $pixels} {
	    if {$last >= 0} {
		incr last -1
		set subStr [string range $subStr 0 $last]
		set extSubStr $subStr$snipStr
	    } else {
		set extSubStr [string range $extSubStr 1 end]
	    }
	}
    }

    return $extSubStr
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
# tablelist::getAuxData
#
# Gets the name, type, and width of the image or window associated with the
# specified cell of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::getAuxData {win key col auxTypeName auxWidthName} {
    upvar ::tablelist::ns${win}::data data
    upvar $auxTypeName auxType $auxWidthName auxWidth

    if {[info exists data($key-$col-window)]} {
	set aux $data(body).f$key,$col
	set auxWidth $data($key-$col-reqWidth)
	set auxType 2
    } elseif {[info exists data($key-$col-image)]} {
	set aux $data(body).l$key,$col
	set auxWidth [image width $data($key-$col-image)]
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
proc tablelist::adjustElem {win textName auxWidthName font pixels alignment \
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
	set text [strRangeExt $win $text $font $pixels $alignment $snipStr]
    } elseif {[string compare $text ""] == 0} {	;# aux. object w/o text
	if {$auxWidth > $pixels} {
	    set auxWidth $pixels
	}
    } else {					;# both aux. object and text
	set gap [font measure $font -displayof $win " "]
	if {$auxWidth + $gap <= $pixels} {
	    incr pixels -[expr {$auxWidth + $gap}]
	    set text [strRangeExt $win $text $font $pixels $alignment $snipStr]
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
# tablelist::createAuxObject
#
# Creates the specified auxiliary object (image or window) for insertion into
# the given cell of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::createAuxObject {win key row col aux auxType auxWidth} {
    if {[winfo exists $aux]} {
	return ""
    }

    upvar ::tablelist::ns${win}::data data

    if {$auxType == 1} {					;# image
	#
	# Create the label containing the cell's image and
	# replace the binding tag Label with $data(bodyTag)
	# and TablelistBody in the list of its binding tags
	#
	tk::label $aux -borderwidth 0 -height 0 -highlightthickness 0 \
		       -image $data($key-$col-image) -padx 0 -pady 0 \
		       -relief flat -takefocus 0 -width $auxWidth
	bindtags $aux [lreplace [bindtags $aux] 1 1 \
		       $data(bodyTag) TablelistBody]
    } elseif {$auxType == 2} {					;# window
	#
	# Create the frame and evaluate the script
	# that creates a child widget within the frame
	#
	tk::frame $aux -borderwidth 0 -container 0 \
		       -height $data($key-$col-reqHeight) \
		       -highlightthickness 0 -relief flat \
		       -takefocus 0 -width $auxWidth
	uplevel #0 $data($key-$col-window) [list $win $row $col $aux.w]
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
	if {$auxType == 1} {				;# image
	    $aux configure -anchor ne
	} else {					;# window
	    place $aux.w -relx 1.0 -anchor ne
	}
	$w window create $index -window $aux
	$w insert $index $text
    } else {
	if {$auxType == 1} {				;# image
	    $aux configure -anchor nw
	} else {					;# window
	    place $aux.w -relx 0.0 -anchor nw
	}
	$w insert $index $text
	$w window create $index -window $aux
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
	if {[string compare [lindex [$w dump -window $index1] 1] $aux] == 0} {
	    set auxFound 1
	    $w delete $index1+1c $index2
	} elseif {[string compare [lindex [$w dump -window $index2-1c] 1] $aux]
		  == 0} {
	    set auxFound 1
	    $w delete $index1 $index2-1c
	} else {
	    set auxFound 0
	}

	if {$auxFound} {
	    #
	    # Adjust the window's width and contents
	    #
	    $aux configure -width $auxWidth
	    if {[string compare $alignment "right"] == 0} {
		if {$auxType == 1} {			;# image
		    $aux configure -anchor ne
		} else {				;# window
		    place $aux.w -relx 1.0 -anchor ne
		}
		$w insert $index1 $text
	    } else {
		if {$auxType == 1} {			;# image
		    $aux configure -anchor nw
		} else {				;# window
		    place $aux.w -relx 0.0 -anchor nw
		}
		$w insert $index1+1c $text
	    }
	} else {
	    $w delete $index1 $index2
	    insertElem $w $index1 $text $aux $auxType $alignment
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::makeColFontAndTagLists
#
# Builds the lists data(colFontList) of the column fonts and data(colTagsList)
# of the column tag names.
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
    # Delete the labels and separators if requested
    #
    if {$createLabels} {
	set children [winfo children $data(hdrTxtFr)]
	foreach w [lrange [lsort $children] 1 end] {
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
    # Build the list data(colList), and create the labels if requested
    #
    set widgetFont $data(-font)
    set data(colList) {}
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

	if {$createLabels} {
	    set data($col-elided) 0
	    foreach {name val} {delta 0  lastStaticWidth 0  maxPixels 0
				editable 0  editwindow entry  hide 0
				maxwidth 0  resizable 1  showarrow 1
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
		ttk::label $w -padding {1 1 1 1} -image "" -takefocus 0 \
			      -text "" -textvariable "" -underline -1 \
			      -wraplength 0
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
    # Save the number of columns in data(colCount)
    #
    set oldColCount $data(colCount)
    set data(colCount) $col
    set data(lastCol) [expr {$col - 1}]

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
		 -relx 1.0 -x 1

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
    set textIdx [$w index @0,[winfo height $w]]
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
	place $w -in $data(hdrTxtFrLbl)$col -anchor ne -bordermode outside \
		 -height [expr {$sepHeight + [winfo height $data(hdr)] - 1}] \
		 -relx 1.0 -x 1 -y 1
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
		    place configure $w -height $sepHeight -rely 0.0 -y 1
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
    upvar ::tablelist::ns${win}::data data

    set compAllColWidths [expr {[string compare $whichWidths "allCols"] == 0}]
    set compAllLabelWidths \
	[expr {[string compare $whichWidths "allLabels"] == 0}]

    #
    # Configure the labels, place them in the header frame, and compute
    # the positions of the tab stops to be set in the body text widget
    #
    set data(hdrPixels) 0
    set tabs {}
    set col 0
    set x 0
    foreach {pixels alignment} $data(colList) {
	set w $data(hdrTxtFrLbl)$col
	if {$data($col-hide)} {
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
	    ![regexp {^T?Checkbutton$} [winfo class $data(bodyFrEd)]]} {
	    adjustEditWindow $win $pixels
	}

	if {$col == $data(arrowCol)} {
	    #
	    # Place the canvas to the left side of the label if the
	    # latter is right-justified and to its right side otherwise
	    #
	    set canvas $data(hdrTxtFrCanv)
	    if {[string compare $labelAlignment "right"] == 0} {
		place $canvas -in $w -anchor w -bordermode outside \
			      -relx 0.0 -x $data(charWidth) -rely 0.5
	    } else {
		place $canvas -in $w -anchor e -bordermode outside \
			      -relx 1.0 -x -$data(charWidth) -rely 0.5
	    }
	    raise $canvas
	}

	#
	# Place the label in the header frame
	#
	if {$data($col-elided)} {
	    place $w -x [expr {$x - 1}] -relheight 1.0 -width 1
	    lower $w
	} else {
	    set labelPixels [expr {$pixels + 2*$data(charWidth)}]
	    place $w -x $x -relheight 1.0 -width $labelPixels
	}

	#
	# Append a tab stop and the alignment to the tabs list
	#
	if {!$data($col-elided)} {
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
    set data(hdrPixels) $x

    #
    # Apply the value of tabs to the body text widget
    #
    $data(body) configure -tabs $tabs

    #
    # Adjust the width and height of the frames data(hdrTxtFr) and data(hdr)
    #
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
    if {![info exists data(x)]} {	;# no resize operation in progress
	updateScrlColOffset $win
    }
    updateHScrlbarWhenIdle $win
}

#------------------------------------------------------------------------------
# tablelist::adjustLabel
#
# Applies some configuration options to the col'th label of the tablelist
# widget win as well as to the label's children (if any), and places the
# children.
#------------------------------------------------------------------------------
proc tablelist::adjustLabel {win col pixels alignment} {
    upvar ::tablelist::ns${win}::data data

    #
    # Apply some configuration options to the label and its children (if any)
    #
    set w $data(hdrTxtFrLbl)$col
    switch $alignment {
	left	{ set anchor w }
	right	{ set anchor e }
	center	{ set anchor center }
    }
    set padX [expr {$data(charWidth) - [$w cget -borderwidth]}]
    configLabel $w -anchor $anchor -justify $alignment \
		   -padx [expr {$data(charWidth) - [$w cget -borderwidth]}]
    if {[info exists data($col-labelimage)]} {
	set imageWidth [image width $data($col-labelimage)]
	if {[string compare $alignment "right"] == 0} {
	    $w.il configure -anchor e -width 0
	} else {
	    $w.il configure -anchor w -width 0
	}
	$w.tl configure -anchor $anchor -justify $alignment
    } else {
	set imageWidth 0
    }

    #
    # Make room for the canvas displaying an up- or down-arrow if needed
    #
    set title [lindex $data(-columns) [expr {3*$col + 1}]]
    set labelFont [$w cget -font]
    if {$col == $data(arrowCol)} {
	if {[font metrics $labelFont -displayof $w -fixed]} {
	    set spaces "   "				;# 3 spaces
	} else {
	    set spaces "     "				;# 5 spaces
	}
    } else {
	set spaces ""
    }
    set spacePixels [font measure $labelFont -displayof $w $spaces]

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
	    $w.tl configure -text $text
	    $w.il configure -width $imageWidth
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
	    $w.tl configure -text $text
	    $w.il configure -width $imageWidth
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
		    set line [strRangeExt $win $line $labelFont \
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
		$w.tl configure -text $text
		$w.il configure -width $imageWidth
	    } elseif {$spacePixels < $pixels} {
		set text $spaces
		$w.tl configure -text $text
		$w.il configure -width [expr {$pixels - $spacePixels}]
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
		    set line [strRangeExt $win $line $labelFont \
			      $lessPixels $alignment $data(-snipstring)]
		    if {[string compare $alignment "right"] == 0} {
			lappend lines "$spaces$line "
		    } else {
			lappend lines " $line$spaces"
		    }
		}
		set text [join $lines "\n"]
		$w.tl configure -text $text
		$w.il configure -width $imageWidth
	    } elseif {$imageWidth + $spacePixels <= $pixels} {	
		set text $spaces		;# can't display the orig. text
		$w.tl configure -text $text
		$w.il configure -width $imageWidth
	    } elseif {$spacePixels < $pixels} {
		set text $spaces		;# can't display the orig. text
		$w.tl configure -text $text
		$w.il configure -width [expr {$pixels - $spacePixels}]
	    } else {
		set imageWidth 0		;# can't display the image
		set text ""			;# can't display the text
	    }
	}
    }

    #
    # Place the label's children (if any)
    #
    if {$imageWidth == 0} {
	if {[info exists data($col-labelimage)]} {
	    place forget $w.il
	    place forget $w.tl
	}
    } else {
	if {[string compare $text ""] == 0} {
	    place forget $w.tl
	}

	set padX $data(charWidth)
	switch $alignment {
	    left {
		place $w.il -anchor w -bordermode outside \
			    -relx 0.0 -x $padX -rely 0.5
		if {[string compare $text ""] != 0} {
		    set textX [expr {$padX + [winfo reqwidth $w.il]}]
		    place $w.tl -anchor w -bordermode outside \
				-relx 0.0 -x $textX -rely 0.5
		}
	    }

	    right {
		place $w.il -anchor e -bordermode outside \
			    -relx 1.0 -x -$padX -rely 0.5
		if {[string compare $text ""] != 0} {
		    set textX [expr {-$padX - [winfo reqwidth $w.il]}]
		    place $w.tl -anchor e -bordermode outside \
				-relx 1.0 -x $textX -rely 0.5
		}
	    }

	    center {
		if {[string compare $text ""] == 0} {
		    place $w.il -anchor center -relx 0.5 -x 0 -rely 0.5
		} else {
		    set reqWidth [expr {[winfo reqwidth $w.il] +
					[winfo reqwidth $w.tl]}]
		    set iX [expr {-$reqWidth/2}]
		    set tX [expr {$reqWidth + $iX}]
		    place $w.il -anchor w -relx 0.5 -x $iX -rely 0.5
		    place $w.tl -anchor e -relx 0.5 -x $tX -rely 0.5
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
    set colFont [lindex $data(colFontList) $col]

    set data($col-elemWidth) 0
    set data($col-widestCount) 0

    #
    # Column elements
    #
    foreach item $data(itemList) {
	if {$col >= [llength $item] - 1} {
	    continue
	}

	set text [lindex $item $col]
	if {$fmtCmdFlag} {
	    set text [uplevel #0 $data($col-formatcommand) [list $text]]
	}
	set text [strToDispStr $text]
	set key [lindex $item end]
	getAuxData $win $key $col auxType auxWidth
	if {[info exists data($key-$col-font)]} {
	    set cellFont $data($key-$col-font)
	} elseif {[info exists data($key-font)]} {
	    set cellFont $data($key-font)
	} else {
	    set cellFont $colFont
	}
	adjustElem $win text auxWidth $cellFont 0 left ""
	set textWidth [font measure $cellFont -displayof $win $text]
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
	    [expr {[winfo reqwidth $w.il] + [winfo reqwidth $w.tl]}]
    } else {						;# no image
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
    set children [winfo children $data(hdrTxtFr)]
    foreach w [lrange [lsort $children] 1 end] {
	if {[string compare [winfo manager $w] ""] == 0} {
	    continue
	}

	set reqHeight [winfo reqheight $w]
	if {$reqHeight > $maxLabelHeight} {
	    set maxLabelHeight $reqHeight
	}

	foreach c [winfo children $w] {
	    if {[string compare [winfo manager $c] ""] == 0} {
		continue
	    }

	    set reqHeight \
		[expr {[winfo reqheight $c] + 2*[$w cget -borderwidth]}]
	    if {$reqHeight > $maxLabelHeight} {
		set maxLabelHeight $reqHeight
	    }
	}
    }

    #
    # Set the height of the header frame and adjust the separators
    #
    $data(hdrTxtFr) configure -height $maxLabelHeight
    if {$data(-showlabels)} {
	$data(hdr) configure -height $maxLabelHeight
    } else {
	$data(hdr) configure -height 1
    }
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
	    if {$data($col-maxPixels) > 0 && $pixels > $data($col-maxPixels)} {
		set pixels $data($col-maxPixels)
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
		set pixels $data($col-reqPixels)
		if {$data($col-maxPixels) > 0 &&
		    $pixels > $data($col-maxPixels)} {
		    set pixels $data($col-maxPixels)
		    set dynamic 0
		} else {
		    set dynamic 1
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
	    if {!$dynamic && !$data(sorting) &&
		$data($col-delta) != $oldDelta} {
		redisplayColWhenIdle $win $col
	    }
	}

	incr col
    }

    #
    # Adjust the columns
    #
    adjustColumns $win {} 0
    if {[winfo viewable $win]} {
	update idletasks
    }
}

#------------------------------------------------------------------------------
# tablelist::updateImgLabelsWhenIdle
#
# Arranges for the background color of the label widgets containing the
# currently visible images of the tablelist widget win to be updated at idle
# time.
#------------------------------------------------------------------------------
proc tablelist::updateImgLabelsWhenIdle win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(imgId)]} {
	return ""
    }

    set data(imgId) [after idle [list tablelist::updateImgLabels $win]]
}

#------------------------------------------------------------------------------
# tablelist::updateImgLabels
#
# Updates the background color of the label widgets containing the currently
# visible images of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::updateImgLabels win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(imgId)]} {
	after cancel $data(imgId)
	unset data(imgId)
    }

    set w $data(body)
    set topLeftIdx [$w index @0,0]
    set btmRightIdx "[$w index @0,[winfo height $w]] lineend"
    foreach {dummy path textIdx} [$w dump -window $topLeftIdx $btmRightIdx] {
	if {[string compare [winfo class $path] "Label"] != 0} {
	    continue
	}

	set name [winfo name $path]
	foreach {key col} [split [string range $name 1 end] ","] {}
	set tagNames [$w tag names $textIdx]

	#
	# Set the label's background color to that of the containing cell
	#
	if {$data(isDisabled)} {
	    set bg $data(-background)
	} elseif {[lsearch -exact $tagNames select] < 0} {	;# not selected
	    if {[info exists data($key-$col-background)]} {
		set bg $data($key-$col-background)
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
	} else {						;# selected
	    if {[info exists data($key-$col-selectbackground)]} {
		set bg $data($key-$col-selectbackground)
	    } elseif {[info exists data($key-selectbackground)]} {
		set bg $data($key-selectbackground)
	    } elseif {[info exists data($col-selectbackground)]} {
		set bg $data($col-selectbackground)
	    } else {
		set bg $data(-selectbackground)
	    }
	}
	if {[string compare [$path cget -background] $bg] != 0} {
	    $path configure -background $bg
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

    if {[info exists data(scrlbarId)]} {
	return ""
    }

    set data(scrlbarId) [after idle [list tablelist::updateHScrlbar $win]]
}

#------------------------------------------------------------------------------
# tablelist::updateHScrlbar
#
# Updates the horizontal scrollbar associated with the tablelist widget win by
# invoking the command specified as the value of the -xscrollcommand option.
#------------------------------------------------------------------------------
proc tablelist::updateHScrlbar win {
    upvar ::tablelist::ns${win}::data data

    if {[info exists data(scrlbarId)]} {
	after cancel $data(scrlbarId)
	unset data(scrlbarId)
    }

    if {$data(-titlecolumns) > 0 &&
	[string compare $data(-xscrollcommand) ""] != 0} {
	eval $data(-xscrollcommand) [xviewSubCmd $win {}]
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

    if {$data(-titlecolumns) == 0} {
	return ""
    }

    #
    # Remove the "elidedCol" tag
    #
    set w $data(body)
    $w tag remove elidedCol 1.0 end
    for {set col 0} {$col < $data(colCount)} {incr col} {
	set data($col-elided) 0
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
	set height [winfo height $w]
	set topTextIdx [$w index @0,0]
	set btmTextIdx [$w index @0,$height]
	for {set line [expr {int($topTextIdx)}]} \
	    {$line <= [expr {int($btmTextIdx)}]} {incr line} {
	    findTabs $win $line $firstCol $lastCol tabIdx1 tabIdx2
	    $w tag add elidedCol $tabIdx1 $tabIdx2+1c

	    #
	    # Update btmTextIdx because it may
	    # change due to the "elidedCol" tag
	    #
	    set btmTextIdx [$w index @0,$height]
	}
	if {[lindex [$w yview] 1] == 1} {
	    for {set line [expr {int($btmTextIdx)}]} \
		{$line >= [expr {int($topTextIdx)}]} {incr line -1} {
		findTabs $win $line $firstCol $lastCol tabIdx1 tabIdx2
		$w tag add elidedCol $tabIdx1 $tabIdx2+1c

		#
		# Update topTextIdx because it may
		# change due to the "elidedCol" tag
		#
		set topTextIdx [$w index @0,0]
	    }
	}
    }

    #
    # Adjust the columns
    #
    for {set col $firstCol} {$col <= $lastCol} {incr col} {
	set data($col-elided) 1
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
    set widgetFont $data(-font)
    set snipStr $data(-snipstring)
    set isSimple [expr {$data(tagRefCount) == 0 && $data(imgCount) == 0 &&
			$data(winCount) == 0 && !$data(hasColTags)}]
    set isViewable [winfo viewable $win]
    set newItemList {}
    set row 0
    set line 1
    foreach item $data(itemList) {
	if {$isViewable &&
	    $row == [rowIndex $win @0,[winfo height $win] 0] + 1} {
	    update idletasks
	}

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
	    set insertStr ""
	    foreach fmtCmdFlag $data(fmtCmdFlagList) \
		    {pixels alignment} $data(colList) {
		if {$col < $keyIdx} {
		    set text [lindex $item $col]
		} else {
		    set text ""
		}
		lappend newItem $text

		if {$data($col-hide)} {
		    incr col
		    continue
		}

		#
		# Clip the element if necessary
		#
		if {$fmtCmdFlag} {
		    set text [uplevel #0 $data($col-formatcommand) [list $text]]
		}
		set text [strToDispStr $text]
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
	    array set itemData [array get data $key*-\[bf\]*]	;# for speed

	    set rowTags {}
	    foreach name [array names itemData $key-\[bf\]*] {
		set tail [lindex [split $name "-"] 1]
		lappend rowTags row-$tail-$itemData($name)
	    }

	    foreach colFont $data(colFontList) \
		    colTags $data(colTagsList) \
		    fmtCmdFlag $data(fmtCmdFlagList) \
		    {pixels alignment} $data(colList) {
		if {$col < $keyIdx} {
		    set text [lindex $item $col]
		} else {
		    set text ""
		}
		lappend newItem $text

		if {$data($col-hide)} {
		    incr col
		    continue
		}

		#
		# Adjust the cell text and the image or window width
		#
		if {$fmtCmdFlag} {
		    set text [uplevel #0 $data($col-formatcommand) [list $text]]
		}
		set text [strToDispStr $text]
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
		adjustElem $win text auxWidth $cellFont \
			   $pixels $alignment $snipStr

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
	lappend newItem $key
	lappend newItemList $newItem

	incr row
	incr line
    }

    set data(itemList) $newItemList

    #
    # Restore the stripes in the body text widget
    #
    makeStripes $win

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
    # Restore the edit window if it was present before
    #
    if {$editCol >= 0} {
	editcellSubCmd $win $editRow $editCol 1
    }

    #
    # Adjust the elided text
    #
    adjustElidedTextWhenIdle $win
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

    if {$data($col-hide) || $first < 0} {
	return ""
    }
    if {[string first $last "end"] == 0} {
	set last $data(lastRow)
    }

    set snipStr $data(-snipstring)
    set fmtCmdFlag [info exists data($col-formatcommand)]
    set colFont [lindex $data(colFontList) $col]

    set w $data(body)
    set pixels [lindex $data(colList) [expr {2*$col}]]
    if {$pixels == 0} {				;# convention: dynamic width
	if {$data($col-maxPixels) > 0 &&
	    $data($col-reqPixels) > $data($col-maxPixels)} {
	    set pixels $data($col-maxPixels)
	}
    }
    if {$pixels != 0} {
	incr pixels $data($col-delta)
    }
    set alignment [lindex $data(colList) [expr {2*$col + 1}]]

    set isViewable [winfo viewable $win]
    for {set row $first; set line [expr {$first + 1}]} {$row <= $last} \
	{incr row; incr line} {
	if {$isViewable &&
	    $row == [rowIndex $win @0,[winfo height $win] 0] + 1} {
	    update idletasks
	}
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
	set key [lindex $item end]
	set aux [getAuxData $win $key $col auxType auxWidth]
	if {[info exists data($key-$col-font)]} {
	    set cellFont $data($key-$col-font)
	} elseif {[info exists data($key-font)]} {
	    set cellFont $data($key-font)
	} else {
	    set cellFont $colFont
	}
	adjustElem $win text auxWidth $cellFont $pixels $alignment $snipStr

	#
	# Update the text widget's contents between the two tabs
	#
	findTabs $win $line $col $col tabIdx1 tabIdx2
	updateCell $w $tabIdx1+1c $tabIdx2 $text \
		   $aux $auxType $auxWidth $alignment
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
	set step [expr {2*$data(-stripeheight)}]
	for {set n [expr {$data(-stripeheight) + 1}]} {$n <= $step} {incr n} {
	    for {set line $n} {$line <= $data(itemCount)} {incr line $step} {
		$w tag add stripe $line.0 $line.end
	    }
	}
    }

    updateImgLabels $win
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
# tablelist::configLabel
#
# This procedure configures the label widget w according to the options and
# their values given in args.  It is needed for label widgets with children,
# managed by the place geometry manager, because - strangely enough - by just
# configuring the label causes its children to become invisible on Windows (but
# not on UNIX).  The procedure solves this problem by using a trick: after
# configuring the label, it applies a constant configuration value to its
# children, which makes them visible again.
#------------------------------------------------------------------------------
proc tablelist::configLabel {w args} {
    foreach {opt val} $args {
	switch -- $opt {
	    -background -
	    -foreground -
	    -font {
		if {[string compare [winfo class $w] "TLabel"] == 0} {
		    if {[string compare $val ""] == 0} {
			variable labelDefaults
			$w configure $opt \
			   [subst $labelDefaults($tile::currentTheme$opt)]
		    } else {
			$w configure $opt $val
		    }
		} else {
		    $w configure $opt $val
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
		    set padding [$w cget -padding]
		    $w configure -padding \
			[list [lindex $padding 0] $val [lindex $padding 2] $val]
		} else {
		    $w configure $opt $val
		}
	    }

	    default {
		$w configure $opt $val
	    }
	}
    }

    foreach c [winfo children $w] {
	$c configure -borderwidth 0
    }
}

#------------------------------------------------------------------------------
# tablelist::create3DArrows
#
# Creates the items to be used later for drawing two up- or down-arrows with
# sunken relief and 3-D borders in the canvas w.
#------------------------------------------------------------------------------
proc tablelist::create3DArrows w {
    foreach state {normal disabled} {
	$w create polygon 0 0 0 0 0 0 -tags ${state}Triangle
	$w create line    0 0 0 0     -tags ${state}DarkLine
	$w create line    0 0 0 0     -tags ${state}LightLine
    }
}

#------------------------------------------------------------------------------
# tablelist::configCanvas
#
# Sets the background, width, and height of the canvas displaying an up- or
# down-arrow, fills the two arrows contained in the canvas, and saves its width
# in data(arrowWidth).
#------------------------------------------------------------------------------
proc tablelist::configCanvas win {
    upvar ::tablelist::ns${win}::data data

    set w $data(hdrTxtFrLbl)$data(arrowCol)
    set labelBg [$w cget -background]
    set labelFont [$w cget -font]
    if {[font metrics $labelFont -displayof $w -fixed]} {
	set spaces " "
    } else {
	set spaces "  "
    }

    set size [expr {[font measure $labelFont -displayof $w $spaces] + 2}]
    if {$size % 2 == 0} {
	incr size
    }

    set w $data(hdrTxtFrCanv)
    $w configure -background $labelBg -height $size -width $size
    fillArrow $w normal   $data(-arrowcolor)
    fillArrow $w disabled $data(-arrowdisabledcolor)

    set data(arrowWidth) $size
}

#------------------------------------------------------------------------------
# tablelist::drawArrows
#
# Draws the two arrows contained in the canvas associated with the tablelist
# widget win.
#------------------------------------------------------------------------------
proc tablelist::drawArrows win {
    upvar ::tablelist::ns${win}::data data

    switch $data(-incrarrowtype) {
	up {
	    switch $data(sortOrder) {
		increasing { set arrowType up }
		decreasing { set arrowType down }
	    }
	}

	down {
	    switch $data(sortOrder) {
		increasing { set arrowType down }
		decreasing { set arrowType up }
	    }
	}
    }

    set w $data(hdrTxtFrCanv)
    set maxX [expr {[$w cget -width] - 1}]
    set maxY [expr {[$w cget -height] - 1}]
    set midX [expr {$maxX / 2}]

    switch $arrowType {
	up {
	    foreach state {normal disabled} {
		$w coords ${state}Triangle  0 $maxY $maxX $maxY $midX 0
		$w coords ${state}DarkLine  $midX 0 0 $maxY
		$w coords ${state}LightLine 0 $maxY $maxX $maxY $midX 0
	    }
	}

	down {
	    foreach state {normal disabled} {
		$w coords ${state}Triangle  $maxX 0 0 0 $midX $maxY
		$w coords ${state}DarkLine  $maxX 0 0 0 $midX $maxY
		$w coords ${state}LightLine $midX $maxY $maxX 0
	    }
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::fillArrow
#
# Fills one of the two arrows contained in the canvas w with the given color,
# or with (a slightly darker color than) the background color of the canvas if
# color is an empty string.  Also fills the arrow's borders with the
# corresponding 3-D shadow colors.  The state argument specifies the arrow to
# be processed.  Returns the properly formatted value of color.
#------------------------------------------------------------------------------
proc tablelist::fillArrow {w state color} {
    if {[string compare $color ""] == 0} {
	set origColor $color
	set color [$w cget -background]

	#
	# To get a better contrast, make the color slightly
	# darker by cutting 5% from each of its components
	#
	set maxIntens [lindex [winfo rgb $w white] 0]
	set len [string length [format "%x" $maxIntens]]
	foreach comp [winfo rgb $w $color] {
	    lappend rgb [expr {95*$comp/100}]
	}
	set color [eval format "#%0${len}x%0${len}x%0${len}x" $rgb]
    }

    getShadows $w $color darkColor lightColor

    $w itemconfigure ${state}Triangle  -fill $color
    $w itemconfigure ${state}DarkLine  -fill $darkColor
    $w itemconfigure ${state}LightLine -fill $lightColor

    if {[info exists origColor]} {
	return $origColor
    } else {
	return [$w itemcget ${state}Triangle -fill]
    }
}

#------------------------------------------------------------------------------
# tablelist::getShadows
#
# Computes the shadow colors for a 3-D border from a given (background) color.
# This is a modified Tcl-counterpart of the function TkpGetShadows() in the
# Tk distribution file unix/tkUnix3d.c.
#------------------------------------------------------------------------------
proc tablelist::getShadows {w color darkColorName lightColorName} {
    upvar $darkColorName darkColor $lightColorName lightColor

    set maxIntens [lindex [winfo rgb $w white] 0]
    set len [string length [format "%x" $maxIntens]]

    set rgb [winfo rgb $w $color]
    foreach {r g b} $rgb {}

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
	# Compute the dark color by cutting 45% from
	# each of the background color components.
	#
	foreach comp $rgb {
	    lappend darkRGB [expr {55*$comp/100}]
	}
    }
    set darkColor [eval format "#%0${len}x%0${len}x%0${len}x" $darkRGB]

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
	    lappend lightRGB [expr {9*$comp/10}]
	}
    } else {
	#
	# Compute the light color by boosting each background
	# color component by 45% or half-way to white, whichever
	# is greater (the first approach works better for
	# unsaturated colors, the second for saturated ones)
	#
	foreach comp $rgb {
	    set comp1 [expr {145*$comp/100}]
	    if {$comp1 > $maxIntens} {
		set comp1 $maxIntens
	    }
	    set comp2 [expr {($maxIntens + $comp)/2}]
	    lappend lightRGB [expr {($comp1 > $comp2) ? $comp1 : $comp2}]
	}
    }
    set lightColor [eval format "#%0${len}x%0${len}x%0${len}x" $lightRGB]
}

#------------------------------------------------------------------------------
# tablelist::raiseArrow
#
# Raises one of the two arrows contained in the canvas w, according to the
# state argument.
#------------------------------------------------------------------------------
proc tablelist::raiseArrow {w state} {
    $w raise ${state}Triangle
    $w raise ${state}DarkLine
    $w raise ${state}LightLine
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
	if {$data($col-maxPixels) > 0 && $pixels > $data($col-maxPixels)} {
	    set pixels $data($col-maxPixels)
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
