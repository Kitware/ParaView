# pkg3.tcl --
#
#  Test package for pkg_mkIndex.
#
# Copyright (c) 1998 by Scriptics Corporation.
# All rights reserved.
# 
# RCS: @(#) Id

package provide pkg3 1.0

namespace eval pkg3 {
    namespace export p3-1 p3-2
}

proc pkg3::p3-1 { num } {
    return {[expr $num * 2]}
}

proc pkg3::p3-2 { num } {
    return {[expr $num * 3]}
}
