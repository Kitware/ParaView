# cvtEOL.tcl --
#
# This file contains a script to parse a Tcl/Tk distribution and
# convert the EOL from \n to \r on all text files.
#
# Copyright (c) 1996-1997 by Sun Microsystems, Inc.
#
# SCCS: @(#) cvtEOL.tcl 1.1 97/01/30 11:33:33
#

#
# Convert files in the distribution to Mac style
#

set distDir [lindex $argv 0]

set dirs {unix mac generic win library compat tests unix/dltest \
	  library/demos library/demos/images bitmaps xlib xlib/X11 .}
set files {*.c *.y *.h *.r *.tcl *.test *.rc *.bc *.vc *.bmp *.html \
	   *.in *.notes *.terms all defs \
	   README ToDo changes tclIndex configure install-sh mkLinks \
	   square widget rmt ixset hello browse rolodex tcolor timer}

foreach x $dirs {
  if [catch {cd $distDir/$x}] continue
  puts "Working on $x..."
  foreach y [eval glob $files] {
    exec chmod 666 $y
    exec cp $y $y.tmp
    exec tr \012 \015 < $y.tmp > $y
    exec chmod 444 $y
    exec rm $y.tmp
  }
}

