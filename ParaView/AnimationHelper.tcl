


# a generic interactor for tcl and vtk
#

# This file contains a procedure for helping animation objects
# set parameters.  For vector set/get methods, this procedure
# will change just one of them.

proc SetComponent {object var idx val} {
  set tmp [$object Get$var]
  set tmp [lreplace $tmp $idx $idx $val]
  eval $object Set$var $tmp
}