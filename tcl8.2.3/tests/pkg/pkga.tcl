# pkga.tcl --
#
#  Test package for pkg_mkIndex. This package provides Pkga,
#  which is also provided by a DLL.
#
# Copyright (c) 1998 by Scriptics Corporation.
# All rights reserved.
# 
# RCS: @(#) Id

package provide Pkga 1.0

proc pkga_neq { x } {
    return [expr {! [pkgq_eq $x]}]
}
