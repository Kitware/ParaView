package require vtkcommon

if {[package require -exact KWWidgets $::kwwidgets::init::version]} {
    package provide kwwidgets $::kwwidgets::init::version
}
