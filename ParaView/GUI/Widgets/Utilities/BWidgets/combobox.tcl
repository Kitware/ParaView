# ----------------------------------------------------------------------------
#  combobox.tcl
#  This file is part of Unifix BWidget Toolkit
#  Id
# ----------------------------------------------------------------------------
#  Index of commands:
#     - ComboBox::create
#     - ComboBox::configure
#     - ComboBox::cget
#     - ComboBox::setvalue
#     - ComboBox::getvalue
#     - ComboBox::_create_popup
#     - ComboBox::_mapliste
#     - ComboBox::_unmapliste
#     - ComboBox::_select
#     - ComboBox::_modify_value
# ----------------------------------------------------------------------------

# ComboBox uses the 8.3 -listvariable listbox option
package require Tk 8.3

namespace eval ComboBox {
    Widget::define ComboBox combobox ArrowButton Entry ListBox

    Widget::tkinclude ComboBox frame :cmd \
	include {-relief -borderwidth -bd -background} \
	initialize {-relief sunken -borderwidth 2} \

    Widget::bwinclude ComboBox Entry .e \
	remove {-relief -bd -borderwidth -bg} \
	rename {-background -entrybg}

    Widget::declare ComboBox {
	{-height       TkResource 0    0 listbox}
	{-values       String	  ""   0}
	{-images       String	  ""   0}
	{-indents      String	  ""   0}
	{-modifycmd    String	  ""   0}
	{-postcommand  String	  ""   0}
	{-expand       Enum	  none 0 {none tab}}
	{-autocomplete Boolean	  0    0}
        {-bwlistbox    Boolean    0    0}
        {-listboxwidth Int        0    0}
        {-hottrack     Boolean    0    0}
    }

    Widget::addmap ComboBox ArrowButton .a {
	-background {} -foreground {} -disabledforeground {} -state {}
    }

    Widget::syncoptions ComboBox Entry .e {-text {}}

    ::bind BwComboBox <FocusIn> [list after idle {BWidget::refocus %W %W.e}]
    ::bind BwComboBox <Destroy> [list Widget::destroy %W]

    ::bind ListBoxHotTrack <Motion> {
        %W selection clear 0 end
        %W activate @%x,%y
        %W selection set @%x,%y
    }
}


# ComboBox::create --
#
#	Create a combobox widget with the given options.
#
# Arguments:
#	path	name of the new widget.
#	args	optional arguments to the widget.
#
# Results:
#	path	name of the new widget.

proc ComboBox::create { path args } {
    array set maps [list ComboBox {} :cmd {} .e {} .a {}]
    array set maps [Widget::parseArgs ComboBox $args]

    eval [list frame $path] $maps(:cmd) \
	[list -highlightthickness 0 -takefocus 0 -class ComboBox]
    Widget::initFromODB ComboBox $path $maps(ComboBox)

    bindtags $path [list $path BwComboBox [winfo toplevel $path] all]

    set entry [eval [list Entry::create $path.e] $maps(.e) \
		   [list -relief flat -borderwidth 0 -takefocus 1]]

    ::bind $path.e <FocusOut>      [list $path _focus_out]
    ::bind $path   <<TraverseIn>>  [list $path _traverse_in]

    if {[Widget::cget $path -autocomplete]} {
	::bind $path.e <KeyRelease> [list $path _auto_complete %K]
    }

    if {[string equal $::tcl_platform(platform) "unix"]} {
	set ipadx 0
	set width 11
    } else {
	set ipadx 2
	set width 15
    }
    set height [winfo reqheight $entry]
    set arrow [eval [list ArrowButton::create $path.a] $maps(.a) \
		   -width $width -height $height \
		   -highlightthickness 0 -borderwidth 1 -takefocus 0 \
		   -dir	  bottom \
		   -type  button \
		   -ipadx $ipadx \
		   -command [list [list ComboBox::_mapliste $path]]]

    pack $arrow -side right -fill y
    pack $entry -side left  -fill both -expand yes

    set editable [Widget::cget $path -editable]
    Entry::configure $path.e -editable $editable
    if {$editable} {
	::bind $entry <ButtonPress-1> [list ComboBox::_unmapliste $path]
    } else {
	::bind $entry <ButtonPress-1> [list ArrowButton::invoke $path.a]
	if { ![string equal [Widget::cget $path -state] "disabled"] } {
	    Entry::configure $path.e -takefocus 1
	}
    }

    ::bind $path  <ButtonPress-1> [list ComboBox::_unmapliste $path]
    ::bind $entry <Key-Up>	  [list ComboBox::_unmapliste $path]
    ::bind $entry <Key-Down>	  [list ComboBox::_mapliste $path]
    ::bind $entry <Control-Up>	  [list ComboBox::_modify_value $path previous]
    ::bind $entry <Control-Down>  [list ComboBox::_modify_value $path next]
    ::bind $entry <Control-Prior> [list ComboBox::_modify_value $path first]
    ::bind $entry <Control-Next>  [list ComboBox::_modify_value $path last]

    if {$editable} {
	set expand [Widget::cget $path -expand]
	if {[string equal "tab" $expand]} {
	    # Expand entry value on Tab (from -values)
	    ::bind $entry <Tab> "[list ComboBox::_expand $path]; break"
	} elseif {[string equal "auto" $expand]} {
	    # Expand entry value anytime (from -values)
	    #::bind $entry <Key> "[list ComboBox::_expand $path]; break"
	}
    }

    ## If we have images, we have to use a BWidget ListBox.
    set bw [Widget::cget $path -bwlistbox]
    if {[llength [Widget::cget $path -images]]} {
        Widget::configure $path [list -bwlistbox 1]
    } else {
        Widget::configure $path [list -bwlistbox $bw]
    }

    return [Widget::create ComboBox $path]
}


# ComboBox::configure --
#
#	Configure subcommand for ComboBox widgets.  Works like regular
#	widget configure command.
#
# Arguments:
#	path	Name of the ComboBox widget.
#	args	Additional optional arguments:
#			?-option?
#			?-option value ...?
#
# Results:
#	Depends on arguments.  If no arguments are given, returns a complete
#	list of configuration information.  If one argument is given, returns
#	the configuration information for that option.  If more than one
#	argument is given, returns nothing.

proc ComboBox::configure { path args } {
    set res [Widget::configure $path $args]
    set entry $path.e


    set list [list -images -values -bwlistbox -hottrack]
    foreach {ci cv cb ch} [eval Widget::hasChangedX $path $list] { break }

    if { $ci } {
        set images [Widget::cget $path -images]
        if {[llength $images]} {
            Widget::configure $path [list -bwlistbox 1]
        } else {
            Widget::configure $path [list -bwlistbox 0]
        }
    }

    set bw [Widget::cget $path -bwlistbox]

    ## If the images, bwlistbox, hottrack or values have changed,
    ## destroy the shell so that it will re-create itself the next
    ## time around.
    if { $ci || $cb || $ch || ($bw && $cv) } {
        destroy $path.shell
    }

    set chgedit [Widget::hasChangedX $path -editable]
    if {$chgedit} {
        if {[Widget::cget $path -editable]} {
            ::bind $entry <ButtonPress-1> [list ComboBox::_unmapliste $path]
	    Entry::configure $entry -editable true
	} else {
	    ::bind $entry <ButtonPress-1> [list ArrowButton::invoke $path.a]
	    Entry::configure $entry -editable false

	    # Make sure that non-editable comboboxes can still be tabbed to.

	    if { ![string equal [Widget::cget $path -state] "disabled"] } {
		Entry::configure $entry -takefocus 1
	    }
        }
    }

    if {$chgedit || [Widget::hasChangedX $path -expand]} {
	# Unset what we may have created.
	::bind $entry <Tab> {}
	if {[Widget::cget $path -editable]} {
	    set expand [Widget::cget $path -expand]
	    if {[string equal "tab" $expand]} {
		# Expand entry value on Tab (from -values)
		::bind $entry <Tab> "[list ComboBox::_expand $path]; break"
	    } elseif {[string equal "auto" $expand]} {
		# Expand entry value anytime (from -values)
		#::bind $entry <Key> "[list ComboBox::_expand $path]; break"
	    }
	}
    }

    # if the dropdown listbox is shown, simply force the actual entry
    #  colors into it. If it is not shown, the next time the dropdown
    #  is shown it'll get the actual colors anyway
    if {[winfo exists $path.shell.listb]} {
	$path.shell.listb configure \
		-bg [Widget::cget $path -entrybg] \
		-fg [Widget::cget $path -foreground] \
		-selectbackground [Widget::cget $path -selectbackground] \
		-selectforeground [Widget::cget $path -selectforeground]
    }

    return $res
}


# ----------------------------------------------------------------------------
#  Command ComboBox::cget
# ----------------------------------------------------------------------------
proc ComboBox::cget { path option } {
    return [Widget::cget $path $option]
}


# ----------------------------------------------------------------------------
#  Command ComboBox::setvalue
# ----------------------------------------------------------------------------
proc ComboBox::setvalue { path index } {
    set values [Widget::getMegawidgetOption $path -values]
    set value  [Entry::cget $path.e -text]
    switch -- $index {
        next {
            if { [set idx [lsearch -exact $values $value]] != -1 } {
                incr idx
            } else {
                set idx [lsearch -exact $values "$value*"]
            }
        }
        previous {
            if { [set idx [lsearch -exact $values $value]] != -1 } {
                incr idx -1
            } else {
                set idx [lsearch -exact $values "$value*"]
            }
        }
        first {
            set idx 0
        }
        last {
            set idx [expr {[llength $values]-1}]
        }
        default {
            if { [string index $index 0] == "@" } {
                set idx [string range $index 1 end]
		if { ![string is integer -strict $idx] } {
                    return -code error "bad index \"$index\""
                }
            } else {
                return -code error "bad index \"$index\""
            }
        }
    }
    if { $idx >= 0 && $idx < [llength $values] } {
        set newval [lindex $values $idx]
	Entry::configure $path.e -text $newval
        return 1
    }
    return 0
}


proc ComboBox::icursor { path idx } {
    return [$path.e icursor $idx]
}


proc ComboBox::get { path } {
    return [$path.e get]
}


# ----------------------------------------------------------------------------
#  Command ComboBox::getvalue
# ----------------------------------------------------------------------------
proc ComboBox::getvalue { path } {
    set values [Widget::getMegawidgetOption $path -values]
    set value  [Entry::cget $path.e -text]

    return [lsearch -exact $values $value]
}


proc ComboBox::getlistbox { path } {
    _create_popup $path
    return $path.shell.listb
}


# ----------------------------------------------------------------------------
#  Command ComboBox::post
# ----------------------------------------------------------------------------
proc ComboBox::post { path } {
    _mapliste $path
    return
}


proc ComboBox::unpost { path } {
    _unmapliste $path
    return
}


# ----------------------------------------------------------------------------
#  Command ComboBox::bind
# ----------------------------------------------------------------------------
proc ComboBox::bind { path args } {
    return [eval [list ::bind $path.e] $args]
}


proc ComboBox::insert { path idx args } {
    upvar #0 [Widget::varForOption $path -values] values

    if {[Widget::cget $path -bwlistbox]} {
        set l [$path getlistbox]
        set i [eval $l insert $idx #auto $args]
        set text [$l itemcget $i -text]
        if {$idx == "end"} {
            lappend values $text
        } else {
            set values [linsert $values $idx $text]
        }
    } else {
        set values [eval linsert [list $values] $idx $args]
    }
}

# ----------------------------------------------------------------------------
#  Command ComboBox::_create_popup
# ----------------------------------------------------------------------------
proc ComboBox::_create_popup { path } {
    set shell $path.shell

    if {[winfo exists $shell]} { return }

    set lval   [Widget::cget $path -values]
    set h      [Widget::cget $path -height]
    set bw     [Widget::cget $path -bwlistbox]

    if { $h <= 0 } {
	set len [llength $lval]
	if { $len < 3 } {
	    set h 3
	} elseif { $len > 10 } {
	    set h 10
	} else {
	    set h $len
	}
    }

    if { $::tcl_platform(platform) == "unix" } {
	set sbwidth 11
    } else {
	set sbwidth 15
    }

    toplevel            $shell -relief solid -bd 1
    wm withdraw         $shell
    update idletasks
    wm overrideredirect $shell 1
    wm transient        $shell [winfo toplevel $path]
    wm withdraw         $shell
    catch { wm attributes $shell -topmost 1 }

    set sw [ScrolledWindow $shell.sw -managed 0 -size $sbwidth -ipad 0]
    
    if {$bw} {
        set listb  [ListBox $shell.listb \
                -relief flat -borderwidth 0 -highlightthickness 0 \
                -selectmode single -selectfill 1 -autofocus 0 -height $h \
                -font [Widget::cget $path -font]  \
                -bg [Widget::cget $path -entrybg] \
                -fg [Widget::cget $path -foreground] \
                -selectbackground [Widget::cget $path -selectbackground] \
                -selectforeground [Widget::cget $path -selectforeground]]

        set values [Widget::cget $path -values]
        set images [Widget::cget $path -images]
        foreach value $values image $images {
            $listb insert end #auto -text $value -image $image
        }
	$listb bindText  <1> "ComboBox::_select $path"
	$listb bindImage <1> "ComboBox::_select $path"
        if {[Widget::cget $path -hottrack]} {
            $listb bindText  <Enter> [list $listb selection set]
            $listb bindImage <Enter> [list $listb selection set]
        }
    } else {
        set listb  [listbox $shell.listb \
                -relief flat -borderwidth 0 -highlightthickness 0 \
                -exportselection false \
                -font	[Widget::cget $path -font]  \
                -height $h \
                -bg [Widget::cget $path -entrybg] \
                -fg [Widget::cget $path -foreground] \
                -selectbackground [Widget::cget $path -selectbackground] \
                -selectforeground [Widget::cget $path -selectforeground] \
                -listvariable [Widget::varForOption $path -values]]
        ::bind $listb <ButtonRelease-1> [list ComboBox::_select $path @%x,%y]

        if {[Widget::cget $path -hottrack]} {
            bindtags $listb [concat [bindtags $listb] ListBoxHotTrack]
        }
    }
    pack $sw -fill both -expand yes
    $sw setwidget $listb

    ::bind $listb <Return>   "ComboBox::_select $path \[%W curselection]"
    ::bind $listb <Escape>   [list ComboBox::_unmapliste $path]
    ::bind $listb <FocusOut> [list ComboBox::_focus_out $path]
}


proc ComboBox::_recreate_popup { path } {
    variable background
    variable foreground

    set shell $path.shell
    set lval  [Widget::cget $path -values]
    set h     [Widget::cget $path -height]
    set bw    [Widget::cget $path -bwlistbox]

    if { $h <= 0 } {
	set len [llength $lval]
	if { $len < 3 } {
	    set h 3
	} elseif { $len > 10 } {
	    set h 10
	} else {
	    set h $len
	}
    }

    if { $::tcl_platform(platform) == "unix" } {
	set sbwidth 11
    } else {
	set sbwidth 15
    }

    _create_popup $path

    if {![Widget::cget $path -editable]} {
        if {[info exists background]} {
            $path.e configure -bg $background
            $path.e configure -fg $foreground
            unset background
            unset foreground
        }
    }

    set listb $shell.listb
    destroy $shell.sw
    set sw [ScrolledWindow $shell.sw -managed 0 -size $sbwidth -ipad 0]
    $listb configure \
            -height $h \
            -font   [Widget::cget $path -font] \
            -bg     [Widget::cget $path -entrybg] \
            -fg     [Widget::cget $path -foreground] \
            -selectbackground [Widget::cget $path -selectbackground] \
            -selectforeground [Widget::cget $path -selectforeground]
    pack $sw -fill both -expand yes
    $sw setwidget $listb
    raise $listb
}


# ----------------------------------------------------------------------------
#  Command ComboBox::_mapliste
# ----------------------------------------------------------------------------
proc ComboBox::_mapliste { path } {
    set listb $path.shell.listb
    if {[winfo exists $path.shell] &&
        [string equal [wm state $path.shell] "normal"]} {
	_unmapliste $path
        return
    }

    if { [Widget::cget $path -state] == "disabled" } {
        return
    }
    if { [set cmd [Widget::getMegawidgetOption $path -postcommand]] != "" } {
        uplevel \#0 $cmd
    }
    if { ![llength [Widget::getMegawidgetOption $path -values]] } {
        return
    }

    _recreate_popup $path

    ArrowButton::configure $path.a -relief sunken
    update

    set bw [Widget::cget $path -bwlistbox]

    $listb selection clear 0 end
    set values [Widget::getMegawidgetOption $path -values]
    set curval [Entry::cget $path.e -text]
    if { [set idx [lsearch -exact $values $curval]] != -1 ||
         [set idx [lsearch -exact $values "$curval*"]] != -1 } {
        if {$bw} {
            set idx [$listb items $idx]
        } else {
            $listb activate $idx
        }
        $listb selection set $idx
        $listb see $idx
    } else {
        set idx 0
        if {$bw} {
            set idx [$listb items 0]
        } else {
            $listb activate $idx
        }
	$listb selection set $idx
        $listb see $idx
    }

    set width [Widget::cget $path -listboxwidth]
    if {!$width} { set width [winfo width $path] }
    BWidget::place $path.shell $width 0 below $path
    wm deiconify $path.shell
    raise $path.shell
    BWidget::focus set $listb
    BWidget::grab global $path
}


# ----------------------------------------------------------------------------
#  Command ComboBox::_unmapliste
# ----------------------------------------------------------------------------
proc ComboBox::_unmapliste { path {refocus 1} } {
    if {[winfo exists $path.shell] && \
	    [string equal [wm state $path.shell] "normal"]} {
        BWidget::grab release $path
        BWidget::focus release $path.shell.listb $refocus
	# Update now because otherwise [focus -force...] makes the app hang!
	if {$refocus} {
	    update
	    focus -force $path.e
	}
        wm withdraw $path.shell
        ArrowButton::configure $path.a -relief raised
    }
}


# ----------------------------------------------------------------------------
#  Command ComboBox::_select
# ----------------------------------------------------------------------------
proc ComboBox::_select { path index } {
    set index [$path.shell.listb index $index]
    _unmapliste $path
    if { $index != -1 } {
        if { [setvalue $path @$index] } {
	    set cmd [Widget::getMegawidgetOption $path -modifycmd]
            if { $cmd != "" } {
                uplevel \#0 $cmd
            }
        }
    }
    $path.e selection clear
    $path.e selection range 0 end
}


# ----------------------------------------------------------------------------
#  Command ComboBox::_modify_value
# ----------------------------------------------------------------------------
proc ComboBox::_modify_value { path direction } {
    if { [setvalue $path $direction] } {
        if { [set cmd [Widget::getMegawidgetOption $path -modifycmd]] != "" } {
            uplevel \#0 $cmd
        }
    }
}

# ----------------------------------------------------------------------------
#  Command ComboBox::_expand
# ----------------------------------------------------------------------------
proc ComboBox::_expand {path} {
    set values [Widget::getMegawidgetOption $path -values]
    if {![llength $values]} {
	bell
	return 0
    }

    set found  {}
    set curval [Entry::cget $path.e -text]
    set curlen [$path.e index insert]
    if {$curlen < [string length $curval]} {
	# we are somewhere in the middle of a string.
	# if the full value matches some string in the listbox,
	# reorder values to start matching after that string.
	set idx [lsearch -exact $values $curval]
	if {$idx >= 0} {
	    set values [concat [lrange $values [expr {$idx+1}] end] \
			    [lrange $values 0 $idx]]
	}
    }
    if {$curlen == 0} {
	set found $values
    } else {
	foreach val $values {
	    if {[string equal -length $curlen $curval $val]} {
		lappend found $val
	    }
	}
    }
    if {[llength $found]} {
	Entry::configure $path.e -text [lindex $found 0]
	if {[llength $found] > 1} {
	    set best [_best_match $found [string range $curval 0 $curlen]]
	    set blen [string length $best]
	    $path.e icursor $blen
	    $path.e selection range $blen end
	}
    } else {
	bell
    }
    return [llength $found]
}

# best_match --
#   finds the best unique match in a list of names
#   The extra $e in this argument allows us to limit the innermost loop a
#   little further.
# Arguments:
#   l		list to find best unique match in
#   e		currently best known unique match
# Returns:
#   longest unique match in the list
#
proc ComboBox::_best_match {l {e {}}} {
    set ec [lindex $l 0]
    if {[llength $l]>1} {
	set e  [string length $e]; incr e -1
	set ei [string length $ec]; incr ei -1
	foreach l $l {
	    while {$ei>=$e && [string first $ec $l]} {
		set ec [string range $ec 0 [incr ei -1]]
	    }
	}
    }
    return $ec
}
# possibly faster
#proc match {string1 string2} {
#   set i 1
#   while {[string equal -length $i $string1 $string2]} { incr i }
#   return [string range $string1 0 [expr {$i-2}]]
#}
#proc matchlist {list} {
#   set list [lsort $list]
#   return [match [lindex $list 0] [lindex $list end]]
#}


# ----------------------------------------------------------------------------
#  Command ComboBox::_traverse_in
#  Called when widget receives keyboard focus due to keyboard traversal.
# ----------------------------------------------------------------------------
proc ComboBox::_traverse_in { path } {
    if {[$path.e selection present] != 1} {
	# Autohighlight the selection, but not if one existed
	$path.e selection range 0 end
    }
}


# ----------------------------------------------------------------------------
#  Command ComboBox::_focus_out
# ----------------------------------------------------------------------------
proc ComboBox::_focus_out { path } {
    if {[focus] == ""} {
	# we lost focus to some other app, make sure we drop the listbox
	return [_unmapliste $path 0]
    }
}

proc ComboBox::_auto_complete { path key } {
    ## Anything that is all lowercase is either a letter, number
    ## or special key we're ok with.  Everything else is a
    ## functional key of some kind.
    if {[string tolower $key] != $key} { return }

    set text [string map [list {[} {\[} {]} {\]}] [$path.e get]]
    if {[string equal $text ""]} { return }
    set values [Widget::cget $path -values]
    set x [lsearch $values $text*]
    if {$x < 0} { return }

    set idx [$path.e index insert]
    $path.e configure -text [lindex $values $x]
    $path.e icursor $idx
    $path.e select range insert end
}
