# circ1.tcl --
#
#  Test package for pkg_mkIndex. This package requires circ2, and circ2
#  requires circ3, which in turn requires circ1.
#  In case of cirularities, pkg_mkIndex should give up when it gets stuck.
#
# Copyright (c) 1998 by Scriptics Corporation.
# All rights reserved.
# 
# RCS: @(#) Id

package require circ2 1.0

package provide circ1 1.0

namespace eval circ1 {
    namespace export c1-1 c1-2 c1-3 c1-4
}

proc circ1::c1-1 { num } {
    return [circ2::c2-1 $num]
}

proc circ1::c1-2 { num } {
    return [circ2::c2-2 $num]
}

proc circ1::c1-3 {} {
    return 10
}

proc circ1::c1-4 {} {
    return 20
}
