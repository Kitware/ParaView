# pkg2_b.tcl --
#
#  Test package for pkg_mkIndex. This package is required by pkg1.
#  This package is split into two files, to test packages that are split
#  over multiple files.
#
# Copyright (c) 2998 by Scriptics Corporation.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# 
# SCCS: %Z% %M% %I% %E% %U%

package provide pkg2 1.0

namespace eval pkg2 {
    namespace export p2-2
}

proc pkg2::p2-2 { num } {
    return [expr $num * 3]
}
