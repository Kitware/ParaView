#!/bin/sh
#
# Synthesize tkConfig.sh for Mac OS X based
# on tclConfig.sh and tkConfig.sh.in
#
# RCS: @(#) Id
#
# the next line restarts using tclsh \
exec tclsh8.4 "$0" "$@"

proc main {tclConfigFile tkConfigFileIn tkConfigFile} {
	set in [open $tclConfigFile]
	set tclConfig [read $in]
	close $in
	set in [open $tkConfigFileIn]
	set tkConfig [read $in]
	close $in
	set tclconfvars [regexp -all -inline -line -- {^TCL_([^=]*)=(.*)$} $tclConfig]
	lappend tclconfvars {} {XINCLUDES} \
	[lindex [regexp -inline -line -- {^TCL_INCLUDE_SPEC=(.*)$} $tclConfig] 1]
	set tkconfvars [regexp -all -inline -line -- {^TK_([^=]*)=} $tkConfig]
	foreach {-> var val} $tclconfvars {
		regsub -all -- {([Tt])cl((?![[:alnum:]])|stub)} $val {\1k\2} val
		foreach {-> tkvar} $tkconfvars {
			regsub -all -- "TCL_$tkvar" $val "TK_$tkvar" val
		}
		regsub -line -- "^TK_$var=.*\$" $tkConfig "TK_$var=$val" tkConfig
	}
	regsub -line -all -- {@[^@]+@} $tkConfig {} tkConfig
	regsub -line -all -- {(/tk)/(?:Development|Deployment)} $tkConfig {\1} tkConfig
	regsub -line {^(TK_DEFS=')} $tkConfig {\1 -DMAC_OSX_TK} tkConfig
	
	set out [open $tkConfigFile w]
	puts $out $tkConfig
	close $out
}

if {$argc != 3} {
	puts stderr "usage: $argv0 /path/to/tclConfig.sh \
/path/to/tkConfig.sh.in /path/to/tkConfig.sh"
	exit 1
}

main [lindex $argv 0] [lindex $argv 1] [lindex $argv 2]
