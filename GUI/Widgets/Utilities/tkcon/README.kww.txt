The changes are:

- in proc tkcon_puts args 
  comment 'update idletaks':
    ## WARNING: This update should behave well because it uses idletasks,
    ## however, if there are weird looping problems with events, or
    ## hanging in waits, try commenting this out.
    if {$len} {
        tkcon console see output
        #update idletasks
    }
- in proc ::tkcon::Bindings
  comment exit code:
    #        <<TkCon_Exit>>                <Control-q>
  add:
        <<TkCon_BracketPair>>          <Control-]>
        <<TkCon_ExpandSubcommand>>        <Shift-Tab>
  add:
    bind TkConsole <<TkCon_BracketPair>> {
            tkTextSetCursor %W {limit}
            ::tkcon::Insert %W {[}
            tkTextSetCursor %W {end}
            ::tkcon::Insert %W {] }
    }
    bind TkConsole <<TkCon_ExpandSubcommand>> {
        if {[%W compare insert > limit]} {::tkcon::ExpandSubcommand %W}
    }
- at the end of file, add support proc for code above:  

## ::tkcon::VTKMethods - helper to convert the output of ListMethods
## into a form that Expand needs
# ARGS: methods - the output of ListMethods class
#       pattern - the substring to match
# Calls:
# Returns: a tcl list of methods
## 
proc ::tkcon::VTKMethods {methods pattern} {

    set mm [split $methods "\n"]
    set methods ""
    if { [llength $mm] > 0 } {
        set mm [concat $mm Print ListMethods New]
    }
    foreach m $mm {
        set m [string trim $m]
        if { [string length $m] == 0 } { continue }
        if { [string match "Methods from*" $m] } { continue }
        if { [string match $pattern $m] } { lappend methods [lindex $m 0] } 
    }
    return $methods
}

## show methods for currently pending command string
## ::tkcon::ExpandSubcommand - look at the current string on the 
##  command line and see if it is a complete command, and if so 
## try to figure out the subcommands for that command
# ARGS: w - the text window to get the command from
# Calls: 
# Returns: nothing.
# Side Effects: the best substring is inserted, and the remaining
#   options are printed
## 
proc ::tkcon::ExpandSubcommand w {

    # get the obj whose methods we want to probe and the 
    # partial subcommand (sub) to complete
    set cmd [::tkcon::CmdGet $::tkcon::PRIV(console)]
    if { [string index $cmd end] == " " } {
        set sub ""
    } else {
        set sub [lindex $cmd end]
    }
    set obj [string trim [string range $cmd 0 end-[string length $sub]]]

    # evaluate the obj in the global space (in case it's a variable 
    # reference or a command in brackets) and then get the list
    # of methods that match the current partial subcommand 
    # - note VTK specific subcommand 'ListMethods'
    #eval set substres [EvalSlave subst [list $obj]]
    set retcode [catch "eval EvalSlave subst [list $obj]" substres]
    if { $retcode || $substres == "" || [info command $substres] == "" } return
    set vtklist [EvalSlave $substres ListMethods]
    set match [::tkcon::VTKMethods $vtklist $sub*]

    # create the longest common substring of methods
    # that match the substring and insert it in the command
    if {[llength $match] > 1} {
        set best [ExpandBestMatch $match $sub]
    } else {
        set best $match
    }
    set new [string range $best [string length $sub] end]
    $w insert end $new

    # show the possible completion options
    if {[llength $match] > 1} {
        if {$::tkcon::OPT(showmultiple) } {
            puts stdout [lsort -dictionary $match]
        }
    }
}
