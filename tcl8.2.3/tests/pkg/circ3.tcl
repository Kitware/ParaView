# circ3.tcl --
#
#  Test package for pkg_mkIndex. This package is required by circ2, and in
#  turn requires circ1. This closes the circularity.
#
# Copyright (c) 1998 by Scriptics Corporation.
# All rights reserved.
# 
# RCS: @(#) Id

package require circ1 1.0

package provide circ3 1.0

namespace eval circ3 {
    namespace export c3-1 c3-4
}

proc circ3::c3-1 {} {
    return [circ1::c1-3]
}

proc circ3::c3-2 {} {
    return [circ1::c1-4]
}
