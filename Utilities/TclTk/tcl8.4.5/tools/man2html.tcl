#!/proj/tcl/install/5.x-sparc/bin/tclsh7.5

if [catch {

# man2html.tcl --
#
# This file contains procedures that work in conjunction with the
# man2tcl program to generate a HTML files from Tcl manual entries.
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
#
# SCCS: @(#) man2html.tcl 1.5 96/04/11 20:21:43
#

set homeDir /home/rjohnson/Projects/tools/generic

# sarray -
#
# Save an array to a file so that it can be sourced.
#
# Arguments:
# file -		Name of the output file
# args -		Name of the arrays to save
#
proc sarray {file args} {
    set file [open $file w]
    foreach a $args {
	upvar $a array
	if ![array exists array] {
	    puts "sarray: \"$a\" isn't an array"
	    break
	}	
    
	foreach name [lsort [array names array]] {
	    regsub -all " " $name "\\ " name1
	    puts $file "set ${a}($name1) \{$array($name)\}"
	}
    }
    close $file
}



# footer --
#
# Builds footer info for HTML pages
#
# Arguments:
# None

proc footer {packages} {
    lappend f "<HR>"
    set h {[}
    foreach package $packages {
	lappend h "<A HREF=\"../$package/contents.html\">$package</A>"
	lappend h "|"
    }
    lappend f [join [lreplace $h end end {]} ] " "]
    lappend f "<HR>"
    lappend f "<PRE>Copyright &#169; 1989-1994 The Regents of the University of California."
    lappend f "Copyright &#169; 1994-1996 Sun Microsystems, Inc."
    lappend f "</PRE>"
    return [join $f "\n"]
}




# doDir --
#
# Given a directory as argument, translate all the man pages in
# that directory.
#
# Arguments:
# dir -			Name of the directory.

proc doDir dir {
    foreach f [lsort [glob -directory $dir "*.\[13n\]"]] {
	do $f	;# defined in man2html1.tcl & man2html2.tcl
    }
}


if {$argc < 2} {
    puts stderr "usage: $argv0 html_dir tcl_dir packages..."
    puts stderr "usage: $argv0 -clean html_dir"
    exit 1
}
	
if {[lindex $argv 0] == "-clean"} {
    set html_dir [lindex $argv 1]
    puts -nonewline "recursively remove: $html_dir? "
    flush stdout
    if {[gets stdin] == "y"} {
	puts "removing: $html_dir"
	exec rm -r $html_dir
    }
    exit 0
}

set html_dir [lindex $argv 0]
set tcl_dir  [lindex $argv 1]
set packages [lrange $argv 2 end]

#### need to add glob capability to packages ####

# make sure there are doc directories for each package

foreach i $packages {
    if ![file exists $tcl_dir/$i/doc] {
	puts stderr "Error: doc directory for package $i is missing"
	exit 1
    }
    if ![file isdirectory $tcl_dir/$i/doc] {
	puts stderr "Error: $tcl_dir/$i/doc is not a directory"
	exit 1
    }
}


# we want to start with a clean sheet

if [file exists $html_dir] {
    puts stderr "Error: HTML directory already exists"
    exit 1
} else {
    exec mkdir $html_dir
}

set footer [footer $packages]


# make the hyperlink arrays and contents.html for all packages
	
foreach package $packages {
    global homeDir
    exec mkdir $html_dir/$package
    
    # build hyperlink database arrays: NAME_file and KEY_file
    #
    puts "\nScanning man pages in $tcl_dir/$package/doc..."
    source $homeDir/man2html1.tcl
    
    doDir $tcl_dir/$package/doc

    # clean up the NAME_file and KEY_file database arrays
    #
    catch {unset KEY_file()}
    foreach name [lsort [array names NAME_file]] {
	set file_name $NAME_file($name)
	if {[llength $file_name] > 1} {
	    set file_name [lsort $file_name]
	    puts stdout "Warning: '$name' multiply defined in: $file_name; using last"
	    set NAME_file($name) [lindex $file_name end]
	}
    }
#   sarray $html_dir/$package/xref.tcl NAME_file KEY_file

    # build the contents file from NAME_file
    #
    puts "\nGenerating contents.html for $package"
    doContents $html_dir/$package/contents.html $lib ;# defined in man2html1.tcl

    # now translate the man pages to HTML pages
    #
    source $homeDir/man2html2.tcl
    puts "\nBuilding html pages from man pages in $tcl_dir/$package/doc..."
    doDir $tcl_dir/$package/doc

    unset NAME_file
}

	

} result] {
    global errorInfo
    puts stderr $result
    puts stderr "in"
    puts stderr $errorInfo
}

