#==============================================================================
# Tablelist package index file.
#
# Copyright (c) 2000-2004  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================

if {[string compare $::tcl_platform(platform) "macintosh"] == 0} {
    #
    # We need to do this here instead of in tablelist.tcl, because of
    # a bug in [info script] in some Tcl releases for the Macintosh.
    #
    namespace eval tablelist {}
    set tablelist::library $dir
}

package ifneeded Tablelist 3.7 [list source [file join $dir tablelist.tcl]]
package ifneeded tablelist 3.7 [list source [file join $dir tablelist.tcl]]
