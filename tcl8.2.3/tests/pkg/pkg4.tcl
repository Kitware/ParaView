# pkg4.tcl --
#
#  Test package for pkg_mkIndex. This package requires pkg3, and it calls
#  a pkg3 proc in the code that is executed by the file
#
# Copyright (c) 1998 by Scriptics Corporation.
# All rights reserved.
# 
# RCS: @(#) Id

package require pkg3 1.0

package provide pkg4 1.0

namespace eval pkg4 {
    namespace export p4-1 p4-2
    variable m2 [pkg3::p3-1 10]
}

proc pkg4::p4-1 { num } {
    variable m2
    return [expr {$m2 * $num}]
}

proc pkg4::p4-2 { num } {
    return [pkg3::p3-2 $num]
}
