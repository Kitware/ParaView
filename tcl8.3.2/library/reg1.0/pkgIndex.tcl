if {[info exists tcl_platform(debug)]} {
    package ifneeded registry 1.0 \
            [list load [file join $dir tclreg83d.dll] registry]
} else {
    package ifneeded registry 1.0 \
            [list load [file join $dir tclreg83.dll] registry]
}
