# console.tcl --
#
# This code constructs the console window for an application.  It
# can be used by non-unix systems that do not have built-in support
# for shells.
#
# RCS: @(#) Id
#
# Copyright (c) 1998-1999 Scriptics Corp.
# Copyright (c) 1995-1997 Sun Microsystems, Inc.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
#

# TODO: history - remember partially written command

# tkConsoleInit --
# This procedure constructs and configures the console windows.
#
# Arguments:
# 	None.

proc tkConsoleInit {} {
    global tcl_platform

    if {![consoleinterp eval {set tcl_interactive}]} {
	wm withdraw .
    }

    if {[string compare $tcl_platform(platform) "macintosh"]} {
	set mod "Ctrl"
    } else {
      set mod "Cmd"
    }

    menu .menubar
    .menubar add cascade -label File -menu .menubar.file -underline 0
    .menubar add cascade -label Edit -menu .menubar.edit -underline 0

    menu .menubar.file -tearoff 0
    .menubar.file add command -label "Source..." -underline 0 \
	-command tkConsoleSource
    .menubar.file add command -label "Hide Console" -underline 0 \
	-command {wm withdraw .}
    if {[string compare $tcl_platform(platform) "macintosh"]} {
	.menubar.file add command -label "Exit" -underline 1 -command exit
    } else {
      .menubar.file add command -label "Quit" -command exit -accel Cmd-Q
    }

    menu .menubar.edit -tearoff 0
    .menubar.edit add command -label "Cut" -underline 2 \
	-command { event generate .console <<Cut>> } -accel "$mod+X"
    .menubar.edit add command -label "Copy" -underline 0 \
	-command { event generate .console <<Copy>> } -accel "$mod+C"
    .menubar.edit add command -label "Paste" -underline 1 \
	-command { event generate .console <<Paste>> } -accel "$mod+V"

    if {[string compare $tcl_platform(platform) "windows"]} {
      .menubar.edit add command -label "Clear" -underline 2 \
          -command { event generate .console <<Clear>> }
    } else {
	.menubar.edit add command -label "Delete" -underline 0 \
	    -command { event generate .console <<Clear>> } -accel "Del"

	.menubar add cascade -label Help -menu .menubar.help -underline 0
	menu .menubar.help -tearoff 0
	.menubar.help add command -label "About..." -underline 0 \
	    -command tkConsoleAbout
    }

    . configure -menu .menubar

    text .console  -yscrollcommand ".sb set" -setgrid true 
    scrollbar .sb -command ".console yview"
    pack .sb -side right -fill both
    pack .console -fill both -expand 1 -side left
    if {![string compare $tcl_platform(platform) "macintosh"]} {
        .console configure -font {Monaco 9 normal} -highlightthickness 0
    }

    tkConsoleBind .console

    .console tag configure stderr -foreground red
    .console tag configure stdin -foreground blue

    focus .console
    
    wm protocol . WM_DELETE_WINDOW { wm withdraw . }
    wm title . "Console"
    flush stdout
    .console mark set output [.console index "end - 1 char"]
    tkTextSetCursor .console end
    .console mark set promptEnd insert
    .console mark gravity promptEnd left
}

# tkConsoleSource --
#
# Prompts the user for a file to source in the main interpreter.
#
# Arguments:
# None.

proc tkConsoleSource {} {
    set filename [tk_getOpenFile -defaultextension .tcl -parent . \
		      -title "Select a file to source" \
		      -filetypes {{"Tcl Scripts" .tcl} {"All Files" *}}]
    if {[string compare $filename ""]} {
    	set cmd [list source $filename]
	if {[catch {consoleinterp eval $cmd} result]} {
	    tkConsoleOutput stderr "$result\n"
	}
    }
}

# tkConsoleInvoke --
# Processes the command line input.  If the command is complete it
# is evaled in the main interpreter.  Otherwise, the continuation
# prompt is added and more input may be added.
#
# Arguments:
# None.

proc tkConsoleInvoke {args} {
    set ranges [.console tag ranges input]
    set cmd ""
    if {[llength $ranges]} {
	set pos 0
      while {[string compare [lindex $ranges $pos] ""]} {
	    set start [lindex $ranges $pos]
	    set end [lindex $ranges [incr pos]]
	    append cmd [.console get $start $end]
	    incr pos
	}
    }
    if {![string compare $cmd ""]} {
	tkConsolePrompt
    } elseif {[info complete $cmd]} {
	.console mark set output end
	.console tag delete input
	set result [consoleinterp record $cmd]
      if {[string compare $result ""]} {
	    puts $result
	}
	tkConsoleHistory reset
	tkConsolePrompt
    } else {
	tkConsolePrompt partial
    }
    .console yview -pickplace insert
}

# tkConsoleHistory --
# This procedure implements command line history for the
# console.  In general is evals the history command in the
# main interpreter to obtain the history.  The global variable
# histNum is used to store the current location in the history.
#
# Arguments:
# cmd -	Which action to take: prev, next, reset.

set histNum 1
proc tkConsoleHistory {cmd} {
    global histNum
    
    switch $cmd {
    	prev {
	    incr histNum -1
	    if {$histNum == 0} {
		set cmd {history event [expr {[history nextid] -1}]}
	    } else {
		set cmd "history event $histNum"
	    }
    	    if {[catch {consoleinterp eval $cmd} cmd]} {
    	    	incr histNum
    	    	return
    	    }
	    .console delete promptEnd end
    	    .console insert promptEnd $cmd {input stdin}
    	}
    	next {
	    incr histNum
	    if {$histNum == 0} {
		set cmd {history event [expr {[history nextid] -1}]}
	    } elseif {$histNum > 0} {
		set cmd ""
		set histNum 1
	    } else {
		set cmd "history event $histNum"
	    }
          if {[string compare $cmd ""]} {
		catch {consoleinterp eval $cmd} cmd
	    }
	    .console delete promptEnd end
	    .console insert promptEnd $cmd {input stdin}
    	}
    	reset {
    	    set histNum 1
    	}
    }
}

# tkConsolePrompt --
# This procedure draws the prompt.  If tcl_prompt1 or tcl_prompt2
# exists in the main interpreter it will be called to generate the 
# prompt.  Otherwise, a hard coded default prompt is printed.
#
# Arguments:
# partial -	Flag to specify which prompt to print.

proc tkConsolePrompt {{partial normal}} {
    if {![string compare $partial "normal"]} {
	set temp [.console index "end - 1 char"]
	.console mark set output end
    	if {[consoleinterp eval "info exists tcl_prompt1"]} {
    	    consoleinterp eval "eval \[set tcl_prompt1\]"
    	} else {
    	    puts -nonewline "% "
    	}
    } else {
	set temp [.console index output]
	.console mark set output end
    	if {[consoleinterp eval "info exists tcl_prompt2"]} {
    	    consoleinterp eval "eval \[set tcl_prompt2\]"
    	} else {
	    puts -nonewline "> "
    	}
    }
    flush stdout
    .console mark set output $temp
    tkTextSetCursor .console end
    .console mark set promptEnd insert
    .console mark gravity promptEnd left
}

# tkConsoleBind --
# This procedure first ensures that the default bindings for the Text
# class have been defined.  Then certain bindings are overridden for
# the class.
#
# Arguments:
# None.

proc tkConsoleBind {win} {
    bindtags $win "$win Text . all"

    # Ignore all Alt, Meta, and Control keypresses unless explicitly bound.
    # Otherwise, if a widget binding for one of these is defined, the
    # <KeyPress> class binding will also fire and insert the character,
    # which is wrong.  Ditto for <Escape>.

    bind $win <Alt-KeyPress> {# nothing }
    bind $win <Meta-KeyPress> {# nothing}
    bind $win <Control-KeyPress> {# nothing}
    bind $win <Escape> {# nothing}
    bind $win <KP_Enter> {# nothing}

    bind $win <Tab> {
	tkConsoleInsert %W \t
	focus %W
	break
    }
    bind $win <Return> {
	%W mark set insert {end - 1c}
	tkConsoleInsert %W "\n"
	tkConsoleInvoke
	break
    }
    bind $win <Delete> {
      if {[string compare [%W tag nextrange sel 1.0 end] ""]} {
	    %W tag remove sel sel.first promptEnd
	} else {
	    if {[%W compare insert < promptEnd]} {
		break
	    }
	}
    }
    bind $win <BackSpace> {
      if {[string compare [%W tag nextrange sel 1.0 end] ""]} {
	    %W tag remove sel sel.first promptEnd
	} else {
	    if {[%W compare insert <= promptEnd]} {
		break
	    }
	}
    }
    foreach left {Control-a Home} {
	bind $win <$left> {
	    if {[%W compare insert < promptEnd]} {
		tkTextSetCursor %W {insert linestart}
	    } else {
		tkTextSetCursor %W promptEnd
            }
	    break
	}
    }
    foreach right {Control-e End} {
	bind $win <$right> {
	    tkTextSetCursor %W {insert lineend}
	    break
	}
    }
    bind $win <Control-d> {
	if {[%W compare insert < promptEnd]} {
	    break
	}
    }
    bind $win <Control-k> {
	if {[%W compare insert < promptEnd]} {
	    %W mark set insert promptEnd
	}
    }
    bind $win <Control-t> {
	if {[%W compare insert < promptEnd]} {
	    break
	}
    }
    bind $win <Meta-d> {
	if {[%W compare insert < promptEnd]} {
	    break
	}
    }
    bind $win <Meta-BackSpace> {
	if {[%W compare insert <= promptEnd]} {
	    break
	}
    }
    bind $win <Control-h> {
	if {[%W compare insert <= promptEnd]} {
	    break
	}
    }
    foreach prev {Control-p Up} {
	bind $win <$prev> {
	    tkConsoleHistory prev
	    break
	}
    }
    foreach prev {Control-n Down} {
	bind $win <$prev> {
	    tkConsoleHistory next
	    break
	}
    }
    bind $win <Insert> {
	catch {tkConsoleInsert %W [selection get -displayof %W]}
	break
    }
    bind $win <KeyPress> {
	tkConsoleInsert %W %A
	break
    }
    foreach left {Control-b Left} {
	bind $win <$left> {
	    if {[%W compare insert == promptEnd]} {
		break
	    }
	    tkTextSetCursor %W insert-1c
	    break
	}
    }
    foreach right {Control-f Right} {
	bind $win <$right> {
	    tkTextSetCursor %W insert+1c
	    break
	}
    }
    bind $win <F9> {
	eval destroy [winfo child .]
      if {![string compare $tcl_platform(platform) "macintosh"]} {
	    source -rsrc Console
	} else {
	    source [file join $tk_library console.tcl]
	}
    }
    bind $win <<Cut>> {
        # Same as the copy event
 	if {![catch {set data [%W get sel.first sel.last]}]} {
	    clipboard clear -displayof %W
	    clipboard append -displayof %W $data
	}
	break
    }
    bind $win <<Copy>> {
 	if {![catch {set data [%W get sel.first sel.last]}]} {
	    clipboard clear -displayof %W
	    clipboard append -displayof %W $data
	}
	break
    }
    bind $win <<Paste>> {
	catch {
	    set clip [selection get -displayof %W -selection CLIPBOARD]
	    set list [split $clip \n\r]
	    tkConsoleInsert %W [lindex $list 0]
	    foreach x [lrange $list 1 end] {
		%W mark set insert {end - 1c}
		tkConsoleInsert %W "\n"
		tkConsoleInvoke
		tkConsoleInsert %W $x
	    }
	}
	break
    }
}

# tkConsoleInsert --
# Insert a string into a text at the point of the insertion cursor.
# If there is a selection in the text, and it covers the point of the
# insertion cursor, then delete the selection before inserting.  Insertion
# is restricted to the prompt area.
#
# Arguments:
# w -		The text window in which to insert the string
# s -		The string to insert (usually just a single character)

proc tkConsoleInsert {w s} {
    if {![string compare $s ""]} {
	return
    }
    catch {
	if {[$w compare sel.first <= insert]
		&& [$w compare sel.last >= insert]} {
	    $w tag remove sel sel.first promptEnd
	    $w delete sel.first sel.last
	}
    }
    if {[$w compare insert < promptEnd]} {
	$w mark set insert end	
    }
    $w insert insert $s {input stdin}
    $w see insert
}

# tkConsoleOutput --
#
# This routine is called directly by ConsolePutsCmd to cause a string
# to be displayed in the console.
#
# Arguments:
# dest -	The output tag to be used: either "stderr" or "stdout".
# string -	The string to be displayed.

proc tkConsoleOutput {dest string} {
    .console insert output $string $dest
    .console see insert
}

# tkConsoleExit --
#
# This routine is called by ConsoleEventProc when the main window of
# the application is destroyed.  Don't call exit - that probably already
# happened.  Just delete our window.
#
# Arguments:
# None.

proc tkConsoleExit {} {
    destroy .
}

# tkConsoleAbout --
#
# This routine displays an About box to show Tcl/Tk version info.
#
# Arguments:
# None.

proc tkConsoleAbout {} {
    global tk_patchLevel
    tk_messageBox -type ok -message "Tcl for Windows
Copyright \251 1999 Scriptics Corporation

Tcl [info patchlevel]
Tk $tk_patchLevel"
}

# now initialize the console

tkConsoleInit
