#==============================================================================
# Contains utility procedures for mega-widgets.
#
# Structure of the module:
#   - Namespace initialization
#   - Public utility procedures
#
# Copyright (c) 2000-2006  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================

package require Tcl 8
package require Tk  8

#
# Namespace initialization
# ========================
#

namespace eval mwutil {
    #
    # Public variables:
    #
    variable version	2.2
    variable library	[file dirname [info script]]

    #
    # Public procedures:
    #
    namespace export	wrongNumArgs getAncestorByClass convEventFields \
			defineKeyNav processTraversal configureWidget \
			fullConfigOpt fullOpt enumOpts configureSubCmd \
			attribSubCmd getScrollInfo
}

#
# Public utility procedures
# =========================
#

#------------------------------------------------------------------------------
# mwutil::wrongNumArgs
#
# Generates a "wrong # args" error message.
#------------------------------------------------------------------------------
proc mwutil::wrongNumArgs args {
    set optList {}
    foreach arg $args {
	lappend optList \"$arg\"
    }
    return -code error "wrong # args: should be [enumOpts $optList]"
}

#------------------------------------------------------------------------------
# mwutil::getAncestorByClass
#
# Gets the path name of the widget of the specified class from the path name w
# of one of its descendants.  It is assumed that all of the ancestors of w
# exist (but w itself needn't exist).
#------------------------------------------------------------------------------
proc mwutil::getAncestorByClass {w class} {
    regexp {^(.+)\..+$} $w dummy win
    while {[string compare [winfo class $win] $class] != 0} {
	set win [winfo parent $win]
    }

    return $win
}

#------------------------------------------------------------------------------
# mwutil::convEventFields
#
# Gets the path name of the widget of the specified class and the x and y
# coordinates relative to the latter from the path name w of one of its
# descendants and from the x and y coordinates relative to the latter.
#------------------------------------------------------------------------------
proc mwutil::convEventFields {w x y class} {
    set win [getAncestorByClass $w $class]
    set _x  [expr {$x + [winfo rootx $w] - [winfo rootx $win]}]
    set _y  [expr {$y + [winfo rooty $w] - [winfo rooty $win]}]

    return [list $win $_x $_y]
}

#------------------------------------------------------------------------------
# mwutil::defineKeyNav
#
# For a given mega-widget class, the procedure defines the binding tag
# ${class}KeyNav as a partial replacement for "all", by substituting the
# scripts bound to the events <Tab>, <Shift-Tab>, and <<PrevWindow>> with new
# ones which propagate these events to the mega-widget of the given class
# containing the widget to which the event was reported.  (The event
# <Shift-Tab> was replaced with <<PrevWindow>> in Tk 8.3.0.)  This tag is
# designed to be inserted before "all" in the list of binding tags of a
# descendant of a mega-widget of the specified class.
#------------------------------------------------------------------------------
proc mwutil::defineKeyNav class {
    foreach event {<Tab> <Shift-Tab> <<PrevWindow>>} {
	bind ${class}KeyNav $event \
	     [list mwutil::processTraversal %W $class $event]
    }

    bind Entry   <<TraverseIn>> { %W selection range 0 end; %W icursor end }
    bind Spinbox <<TraverseIn>> { %W selection range 0 end; %W icursor end }
}

#------------------------------------------------------------------------------
# mwutil::processTraversal
#
# Processes the given traversal event for the mega-widget of the specified
# class containing the widget w if that mega-widget is not the only widget
# receiving the focus during keyboard traversal within its top-level widget.
#------------------------------------------------------------------------------
proc mwutil::processTraversal {w class event} {
    set win [getAncestorByClass $w $class]

    if {[string compare $event "<Tab>"] == 0} {
	set target [tk_focusNext $win]
    } else {
	set target [tk_focusPrev $win]
    }

    if {[string compare $target $win] != 0} {
	focus $target
	event generate $target <<TraverseIn>>
    }

    return -code break ""
}

#------------------------------------------------------------------------------
# mwutil::configureWidget
#
# Configures the widget win by processing the command-line arguments specified
# in optValPairs and, if the value of initialize is true, also those database
# options that don't match any command-line arguments.
#------------------------------------------------------------------------------
proc mwutil::configureWidget {win configSpecsName configCmd cgetCmd \
			      optValPairs initialize} {
    upvar $configSpecsName configSpecs

    #
    # Process the command-line arguments
    #
    set cmdLineOpts {}
    set savedVals {}
    set failed 0
    set count [llength $optValPairs]
    foreach {opt val} $optValPairs {
	if {[catch {fullConfigOpt $opt configSpecs} result] != 0} {
	    set failed 1
	    break
	}
	if {$count == 1} {
	    set result "value for \"$opt\" missing"
	    set failed 1
	    break
	}
	set opt $result
	lappend cmdLineOpts $opt
	lappend savedVals [eval $cgetCmd [list $win $opt]]
	if {[catch {eval $configCmd [list $win $opt $val]} result] != 0} {
	    set failed 1
	    break
	}
	incr count -2
    }

    if {$failed} {
	#
	# Restore the saved values
	#
	foreach opt $cmdLineOpts val $savedVals {
	    eval $configCmd [list $win $opt $val]
	}

	return -code error $result
    }

    if {$initialize} {
	#
	# Process those configuration options that were not
	# given as command-line arguments; use the corresponding
	# values from the option database if available
	#
	foreach opt [lsort [array names configSpecs]] {
	    if {[llength $configSpecs($opt)] == 1 ||
		[lsearch -exact $cmdLineOpts $opt] >= 0} {
		continue
	    }
	    set dbName [lindex $configSpecs($opt) 0]
	    set dbClass [lindex $configSpecs($opt) 1]
	    set dbValue [option get $win $dbName $dbClass]
	    if {[string compare $dbValue ""] == 0} {
		set default [lindex $configSpecs($opt) 3]
		eval $configCmd [list $win $opt $default]
	    } else {
		if {[catch {
		    eval $configCmd [list $win $opt $dbValue]
		} result] != 0} {
		    return -code error $result
		}
	    }
	}
    }

    return ""
}

#------------------------------------------------------------------------------
# mwutil::fullConfigOpt
#
# Returns the full configuration option corresponding to the possibly
# abbreviated option opt.
#------------------------------------------------------------------------------
proc mwutil::fullConfigOpt {opt configSpecsName} {
    upvar $configSpecsName configSpecs

    if {[info exists configSpecs($opt)]} {
	if {[llength $configSpecs($opt)] == 1} {
	    return $configSpecs($opt)
	} else {
	    return $opt
	}
    }

    set optList [lsort [array names configSpecs]]
    set count 0
    foreach elem $optList {
	if {[string first $opt $elem] == 0} {
	    incr count
	    if {$count == 1} {
		set option $elem
	    } else {
		break
	    }
	}
    }

    switch $count {
	0 {
	    ### return -code error "unknown option \"$opt\""
	    return -code error \
		   "bad option \"$opt\": must be [enumOpts $optList]"
	}

	1 {
	    if {[llength $configSpecs($option)] == 1} {
		return $configSpecs($option)
	    } else {
		return $option
	    }
	}

	default {
	    ### return -code error "unknown option \"$opt\""
	    return -code error \
		   "ambiguous option \"$opt\": must be [enumOpts $optList]"
	}
    }
}

#------------------------------------------------------------------------------
# mwutil::fullOpt
#
# Returns the full option corresponding to the possibly abbreviated option opt.
#------------------------------------------------------------------------------
proc mwutil::fullOpt {kind opt optList} {
    if {[lsearch -exact $optList $opt] >= 0} {
	return $opt
    }

    set count 0
    foreach elem $optList {
	if {[string first $opt $elem] == 0} {
	    incr count
	    if {$count == 1} {
		set option $elem
	    } else {
		break
	    }
	}
    }

    switch $count {
	0 {
	    return -code error \
		   "bad $kind \"$opt\": must be [enumOpts $optList]"
	}

	1 {
	    return $option
	}

	default {
	    return -code error \
		   "ambiguous $kind \"$opt\": must be [enumOpts $optList]"
	}
    }
}

#------------------------------------------------------------------------------
# mwutil::enumOpts
#
# Returns a string consisting of the elements of the given list, separated by
# commas and spaces.
#------------------------------------------------------------------------------
proc mwutil::enumOpts optList {
    set optCount [llength $optList]
    set n 1
    foreach opt $optList {
	if {$n == 1} {
	    set str $opt
	} elseif {$n < $optCount} {
	    append str ", $opt"
	} else {
	    if {$optCount > 2} {
		append str ","
	    }
	    append str " or $opt"
	}

	incr n
    }

    return $str
}

#------------------------------------------------------------------------------
# mwutil::configureSubCmd
#
# This procedure is invoked to process configuration subcommands.
#------------------------------------------------------------------------------
proc mwutil::configureSubCmd {win configSpecsName configCmd cgetCmd argList} {
    upvar $configSpecsName configSpecs

    switch [llength $argList] {
	0 {
	    #
	    # Return a list describing all available configuration options
	    #
	    foreach opt [lsort [array names configSpecs]] {
		if {[llength $configSpecs($opt)] == 1} {
		    set alias $configSpecs($opt)
		    if {$::tk_version < 8.1} {
			set dbName [lindex $configSpecs($alias) 0]
			lappend result [list $opt $dbName]
		    } else {
			lappend result [list $opt $alias]
		    }
		} else {
		    set dbName [lindex $configSpecs($opt) 0]
		    set dbClass [lindex $configSpecs($opt) 1]
		    set default [lindex $configSpecs($opt) 3]
		    lappend result [list $opt $dbName $dbClass $default \
				    [eval $cgetCmd [list $win $opt]]]
		}
	    }
	    return $result
	}

	1 {
	    #
	    # Return the description of the specified configuration option
	    #
	    set opt [fullConfigOpt [lindex $argList 0] configSpecs]
	    set dbName [lindex $configSpecs($opt) 0]
	    set dbClass [lindex $configSpecs($opt) 1]
	    set default [lindex $configSpecs($opt) 3]
	    return [list $opt $dbName $dbClass $default \
		    [eval $cgetCmd [list $win $opt]]]
	}

	default {
	    #
	    # Set the specified configuration options to the given values
	    #
	    return [configureWidget $win configSpecs $configCmd $cgetCmd \
		    $argList 0]
	}
    }
}

#------------------------------------------------------------------------------
# mwutil::attribSubCmd
#
# This procedure is invoked to process the attrib subcommand.
#------------------------------------------------------------------------------
proc mwutil::attribSubCmd {win argList} {
    set classNs [string tolower [winfo class $win]]
    upvar ::${classNs}::ns${win}::attribVals attribVals

    set argCount [llength $argList]
    switch $argCount {
	0 {
	    #
	    # Return the current list of attribute names and values
	    #
	    set result {}
	    foreach attr [lsort [array names attribVals]] {
		lappend result [list $attr $attribVals($attr)]
	    }
	    return $result
	}

	1 {
	    #
	    # Return the value of the specified attribute
	    #
	    set attr [lindex $argList 0]
	    if {[info exists attribVals($attr)]} {
		return $attribVals($attr)
	    } else {
		return ""
	    }
	}

	default {
	    #
	    # Set the specified attributes to the given values
	    #
	    if {$argCount % 2 != 0} {
		return -code error "value for \"[lindex $argList end]\" missing"
	    }
	    array set attribVals $argList
	    return ""
	}
    }
}

#------------------------------------------------------------------------------
# mwutil::getScrollInfo
#
# Parses a list of arguments of the form "moveto <fraction>" or "scroll
# <number> units|pages" and returns the corresponding list consisting of two or
# three properly formatted elements.
#------------------------------------------------------------------------------
proc mwutil::getScrollInfo argList {
    set argCount [llength $argList]
    set opt [lindex $argList 0]

    if {[string first $opt "moveto"] == 0} {
	if {$argCount != 2} {
	    wrongNumArgs "moveto fraction"
	}

	set fraction [format "%f" [lindex $argList 1]]
	return [list moveto $fraction]
    } elseif {[string first $opt "scroll"] == 0} {
	if {$argCount != 3} {
	    wrongNumArgs "scroll number units|pages"
	}

	set number [format "%d" [lindex $argList 1]]
	set what [lindex $argList 2]
	if {[string first $what "units"] == 0} {
	    return [list scroll $number units]
	} elseif {[string first $what "pages"] == 0} {
	    return [list scroll $number pages]
	} else {
	    return -code error "bad argument \"$what\": must be units or pages"
	}
    } else {
	return -code error "unknown option \"$opt\": must be moveto or scroll"
    }
}
