# man2help.tcl --
#
# This file defines procedures that work in conjunction with the
# man2tcl program to generate a Windows help file from Tcl manual
# entries.
#
# Copyright (c) 1996 by Sun Microsystems, Inc.
#
# RCS: @(#) Id
# 

#
# PASS 1
#

proc generateContents {basename version files} {
    global curID topics
    set curID 0
    foreach f $files {
	regsub -all -- {-} [file tail $f] {} curFile
	puts "Pass 1 -- $f"
	flush stdout
	doFile $f
    }
    set fd [open "$basename$version.cnt" w]
    fconfigure $fd -translation crlf
    puts $fd ":Base $basename$version.hlp"
    foreach package [getPackages] {
	foreach section [getSections $package] {
	    puts $fd "1 $section"
	    set lastTopic {}
	    foreach topic [getTopics $package $section] {
		if {[string compare $lastTopic $topic] != 0} {
		    set id $topics($package,$section,$topic) 
		    puts $fd "2 $topic=$id"
		    set lastTopic $topic
		}
	    }
	}
    }
    close $fd
}


#
# PASS 2
#

proc generateHelp {basename files} {
    global curID topics keywords file id_keywords
    set curID 0

    foreach key [array names keywords] {
	foreach id $keywords($key) {
	    lappend id_keywords($id) $key
	}
    }
	    
    set file [open "$basename.rtf" w]
    fconfigure $file -translation crlf
    puts $file "\{\\rtf1\\ansi \\deff0\\deflang1033\{\\fonttbl\{\\f0\\froman\\fcharset0\\fprq2 Times New Roman\;\}\}"
    foreach f $files {
	regsub -all -- {-} [file tail $f] {} curFile
	puts "Pass 2 -- $f"
	flush stdout
	initGlobals
	doFile $f
	pageBreak
    }
    puts $file "\}"
    close $file
}

# doFile --
#
# Given a file as argument, translate the file to a tcl script and
# evaluate it.
#
# Arguments:
# file -		Name of file to translate.

proc doFile {file} {
    if {[catch {eval [exec man2tcl [glob $file]]} msg] &&
	    [catch {eval [exec ./man2tcl [glob $file]]} msg]} {
	global errorInfo
	puts stderr $msg
	puts "in"
	puts $errorInfo
	exit 1
    }
}

# doDir --
#
# Given a directory as argument, translate all the man pages in
# that directory.
#
# Arguments:
# dir -			Name of the directory.

proc doDir dir {
    puts "Generating man pages for $dir..."
    foreach f [lsort [glob [file join $dir *.\[13n\]]]] {
	do $f
    }
}

# process command line arguments

if {$argc < 3} {
    puts stderr "usage: $argv0 projectName version manFiles..."
    exit 1
}

set baseName [lindex $argv 0]
set version [lindex $argv 1]
set files {}
foreach i [lrange $argv 2 end] {
    set i [file join $i]
    if [file isdir $i] {
	foreach f [lsort [glob [file join $i *.\[13n\]]]] {
	    lappend files $f
	}
    } elseif [file exists $i] {
	lappend files $i
    }
}

source [file join [file dir $argv0] index.tcl]
generateContents $baseName $version $files
source [file join [file dir $argv0] man2help2.tcl]
generateHelp $baseName $files
