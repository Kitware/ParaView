# pkg1.tcl --
#
#  Test package for pkg_mkIndex. This package requires pkg3, but it does
#  not use any of pkg3's procs in the code that is executed by the file
#  (i.e. references to pkg3's procs are in the proc bodies only).
#
# Copyright (c) 1998 by Scriptics Corporation.
# All rights reserved.
# 
# RCS: @(#) Id

package require pkg3 1.0

package provide pkg1 1.0

namespace eval pkg1 {
    namespace export p1-1 p1-2
}

proc pkg1::p1-1 { num } {
    return [pkg3::p3-1 $num]
}

proc pkg1::p1-2 { num } {
    return [pkg3::p3-2 $num]
}
