# circ2.tcl --
#
#  Test package for pkg_mkIndex. This package is required by circ1, and
#  requires circ3. Circ3, in turn, requires circ1 to give us a circularity.
#
# Copyright (c) 1998 by Scriptics Corporation.
# All rights reserved.
# 
# RCS: @(#) Id

package require circ3 1.0

package provide circ2 1.0

namespace eval circ2 {
    namespace export c2-1 c2-2
}

proc circ2::c2-1 { num } {
    return [expr $num * [circ3::c3-1]]
}

proc circ2::c2-2 { num } {
    return [expr $num * [circ3::c3-2]]
}
