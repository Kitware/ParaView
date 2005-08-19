#==============================================================================
# Main Tablelist and Tablelist_tile package module.
#
# Copyright (c) 2000-2005  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================

namespace eval tablelist {
    #
    # Public variables:
    #
    variable version	4.1
    if {[string compare $::tcl_platform(platform) "macintosh"] != 0} {
	#
	# On the Macintosh, the tablelist::library variable is
	# set in the file pkgIndex.tcl, because of a bug in
	# [info script] in some Tcl releases for that platform.
	#
	variable library	[file dirname [info script]]
    }
    variable usingTile

    #
    # Creates a new tablelist widget:
    #
    namespace export	tablelist

    #
    # Sorts the items of a tablelist widget based on one of its columns:
    #
    namespace export	sortByColumn

    #
    # Helper procedures used in binding scripts:
    #
    namespace export	getTablelistPath convEventFields

    #
    # Register various widgets for interactive cell editing:
    #
    namespace export	addBWidgetEntry addBWidgetSpinBox addBWidgetComboBox
    namespace export    addIncrEntryfield addIncrDateTimeWidget \
			addIncrSpinner addIncrSpinint addIncrCombobox
    namespace export	addOakleyCombobox
    namespace export	addDateMentry addTimeMentry addFixedPointMentry \
    			addIPAddrMentry
}

lappend auto_path [file join $tablelist::library scripts]
