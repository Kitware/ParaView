if {[info exists tcl_platform(debug)]} {
    package ifneeded dde 1.1 [list load [file join $dir tcldde83d.dll] dde]
} else {
    package ifneeded dde 1.1 [list load [file join $dir tcldde83.dll] dde]
}
