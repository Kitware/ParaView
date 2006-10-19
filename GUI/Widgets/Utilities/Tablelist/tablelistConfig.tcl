#==============================================================================
# Contains private configuration procedures for tablelist widgets.
#
# Copyright (c) 2000-2006  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================


#------------------------------------------------------------------------------
# tablelist::extendConfigSpecs
#
# Extends the elements of the array configSpecs.
#------------------------------------------------------------------------------
proc tablelist::extendConfigSpecs {} {
    variable usingTile
    variable helpLabel
    variable configSpecs
    variable winSys


    #
    # Extend some elements of the array configSpecs
    #
    lappend configSpecs(-activestyle)           underline
    lappend configSpecs(-columns)               {}
    lappend configSpecs(-editendcommand)        {}
    lappend configSpecs(-editstartcommand)      {}
    lappend configSpecs(-forceeditendcommand)   0
    lappend configSpecs(-incrarrowtype)         up
    lappend configSpecs(-labelcommand)          {}
    lappend configSpecs(-labelcommand2)         {}
    lappend configSpecs(-labelrelief)           raised
    lappend configSpecs(-listvariable)          {}
    lappend configSpecs(-movablecolumns)        0
    lappend configSpecs(-movablerows)           0
    lappend configSpecs(-movecolumncursor)      icon
    lappend configSpecs(-movecursor)            hand2
    lappend configSpecs(-protecttitlecolumns)   0
    lappend configSpecs(-resizablecolumns)      1
    lappend configSpecs(-resizecursor)          sb_h_double_arrow
    lappend configSpecs(-selecttype)            row
    lappend configSpecs(-setfocus)              0
    lappend configSpecs(-showarrow)             1
    lappend configSpecs(-showlabels)            1
    lappend configSpecs(-showseparators)        0
    lappend configSpecs(-snipstring)            ...
    lappend configSpecs(-sortcommand)           {}
    lappend configSpecs(-spacing)               0
    lappend configSpecs(-stretch)               {}
    lappend configSpecs(-stripebackground)      {}
    lappend configSpecs(-stripeforeground)      {}
    lappend configSpecs(-stripeheight)          1
    lappend configSpecs(-targetcolor)           black
    lappend configSpecs(-titlecolumns)          0


    #
    # Append the default values of the configuration options
    # of a temporary, invisible listbox widget to the values
    # of the corresponding elements of the array configSpecs
    #
    set helpListbox .__helpListbox
    for {set n 0} {[winfo exists $helpListbox]} {incr n} {
        set helpListbox .__helpListbox$n
    }
    listbox $helpListbox
    foreach configSet [$helpListbox configure] {
        if {[llength $configSet] != 2} {
            set opt [lindex $configSet 0]
            if {[info exists configSpecs($opt)]} {
                lappend configSpecs($opt) [lindex $configSet 3]
            }
        }
    }
    destroy $helpListbox


    set helpLabel .__helpLabel
    for {set n 0} {[winfo exists $helpLabel]} {incr n} {
        set helpLabel .__helpLabel$n
    }


    if {$usingTile} {
        foreach opt {-highlightbackground -highlightcolor -highlightthickness
                     -labelactivebackground -labelactiveforeground
                     -labeldisabledforeground -labelheight} {
            unset configSpecs($opt)
        }


        #
        # Append theme-specific values to some elements of the array configSpecs
        #
        ttk::label $helpLabel -takefocus 0
        variable themeDefaults
        setThemeDefaults                ;# pupulates the array themeDefaults
        foreach opt {-labelbackground -labelforeground -labelfont
                     -labelborderwidth -labelpady
                     -arrowcolor -arrowdisabledcolor -arrowstyle} {
            lappend configSpecs($opt) $themeDefaults($opt)
        }
        foreach opt {-background -foreground -disabledforeground
                     -stripebackground -selectbackground -selectforeground
                     -selectborderwidth -font} {
            lset configSpecs($opt) 3 $themeDefaults($opt)
        }


        #
        # Define the header label layout
        #
        style theme settings "default" {
            style layout TablelistHeader.TLabel {
                Treeheading.border -children {
                    Label.padding -children {
                        Label.label
                    }
                }
            }
        }
        if {[string compare [package provide tile::theme::aqua] ""] != 0} {
            style theme settings "aqua" {
                if {[string compare $tile::patchlevel "0.6.4"] < 0} {
                    style layout TablelistHeader.TLabel {
                        Treeheading.cell -children {
                            Label.padding -children {
                                Label.label -side top
                                Separator.hseparator -side bottom
                            }
                        }
                    }
                } else {
                    style layout TablelistHeader.TLabel {
                        Treeheading.cell -children {
                            Label.padding -children {
                                Label.label -side top
                            }
                        }
                    }
                }
                style map TablelistHeader.TLabel -foreground [list \
                    {disabled background} #a3a3a3 disabled #a3a3a3 \
                    background black]
            }
        }


        #
        # Another tile backward compatibility issue
        #
        if {[string compare $tile::version 0.7] < 0} {
            interp alias {} ::tablelist::styleConfig {} style default
        } else {
            interp alias {} ::tablelist::styleConfig {} style configure
        }
    } else {
        if {$::tk_version < 8.3} {
            unset configSpecs(-titlecolumns)
        }


        #
        # Append the default values of some configuration options
        # of an invisible label widget to the values of the
        # corresponding -label* elements of the array configSpecs
        #
        tk::label $helpLabel -takefocus 0
        foreach optTail {font height} {
            set configSet [$helpLabel configure -$optTail]
            lappend configSpecs(-label$optTail) [lindex $configSet 3]
        }
        if {[catch {$helpLabel configure -activebackground} configSet1] == 0 &&
            [catch {$helpLabel configure -activeforeground} configSet2] == 0} {
            lappend configSpecs(-labelactivebackground) [lindex $configSet1 3]
            lappend configSpecs(-labelactiveforeground) [lindex $configSet2 3]
        } else {
            unset configSpecs(-labelactivebackground)
            unset configSpecs(-labelactiveforeground)
        }
        if {[catch {$helpLabel configure -disabledforeground} configSet] == 0} {
            lappend configSpecs(-labeldisabledforeground) [lindex $configSet 3]
        } else {
            unset configSpecs(-labeldisabledforeground)
        }
        if {[string compare $winSys "win32"] == 0 &&
            $::tcl_platform(osVersion) < 5.1} {
            lappend configSpecs(-labelpady) 0
        } else {
            set configSet [$helpLabel configure -pady]
            lappend configSpecs(-labelpady) [lindex $configSet 3]
        }


        #
        # Steal the default values of some configuration
        # options from a temporary, invisible button widget
        #
        set helpButton .__helpButton
        for {set n 0} {[winfo exists $helpButton]} {incr n} {
            set helpButton .__helpButton$n
        }
        button $helpButton
        foreach opt {-disabledforeground -state} {
            if {[llength $configSpecs($opt)] == 3} {
                set configSet [$helpButton configure $opt]
                lappend configSpecs($opt) [lindex $configSet 3]
            }
        }
        foreach optTail {background foreground} {
            set configSet [$helpButton configure -$optTail]
            lappend configSpecs(-label$optTail) [lindex $configSet 3]
        }
        if {[string compare $winSys "classic"] == 0 ||
            [string compare $winSys "aqua"] == 0} {
            lappend configSpecs(-labelborderwidth) 1
        } else {
            set configSet [$helpButton configure -borderwidth]
            lappend configSpecs(-labelborderwidth) [lindex $configSet 3]
        }
        destroy $helpButton


        #
        # Set the default values of the -arrowcolor,
        # -arrowdisabledcolor, and -arrowstyle options
        #
        switch $winSys {
            x11 {
                lappend configSpecs(-arrowcolor)              {}
                lappend configSpecs(-arrowdisabledcolor)      {}
                lappend configSpecs(-arrowstyle)              sunken10x9
            }


            win32 {
                if {$::tcl_platform(osVersion) < 5.1} {
                    lappend configSpecs(-arrowcolor)          {}
                    lappend configSpecs(-arrowdisabledcolor)  {}
                    lappend configSpecs(-arrowstyle)          sunken8x7
                } else {
                    lappend configSpecs(-arrowcolor)          #aca899
                    lappend configSpecs(-arrowdisabledcolor)  SystemDisabledText
                    lappend configSpecs(-arrowstyle)          flat9x5
                }
            }


            classic -
            aqua {
                lappend configSpecs(-arrowcolor)              #777777
                lappend configSpecs(-arrowdisabledcolor)      #a3a3a3
                lappend configSpecs(-arrowstyle)              flat7x7
            }
        }
        lappend configSpecs(-arrowdisabledcolor) \
                [lindex $configSpecs(-arrowcolor) 3]
    }
}


#------------------------------------------------------------------------------
# tablelist::doConfig
#
# Applies the value val of the configuration option opt to the tablelist widget
# win.
#------------------------------------------------------------------------------
proc tablelist::doConfig {win opt val} {
    variable usingTile
    variable helpLabel
    variable configSpecs
    upvar ::tablelist::ns${win}::data data


    #
    # Apply the value to the widget(s) corresponding to the given option
    #
    switch [lindex $configSpecs($opt) 2] {
        c {
            #
            # Apply the value to all children and save the
            # properly formatted value of val in data($opt)
            #
            foreach w [winfo children $win] {
                if {[regexp {^(body|hdr|sep([0-9]+)?)$} [winfo name $w]]} {
                    $w configure $opt $val
                }
            }
            $data(hdrTxt) configure $opt $val
            $data(hdrLbl) configure $opt $val
            foreach w [winfo children $data(hdrTxtFr)] {
                $w configure $opt $val
            }
            set data($opt) [$data(hdrLbl) cget $opt]
        }


        b {
            #
            # Apply the value to the body text widget and save
            # the properly formatted value of val in data($opt)
            #
            set w $data(body)
            $w configure $opt $val
            set data($opt) [$w cget $opt]


            switch -- $opt {
                -background {
                    #
                    # Apply the value to the frame (because of
                    # the shadow colors of its 3-D border), to
                    # the separators, and to the "disabled" tag
                    #
                    if {$usingTile} {
                        styleConfig Frame$win.TFrame $opt $val
                        styleConfig Seps$win.TSeparator $opt $val
                    } else {
                        $win configure $opt $val
                        foreach c [winfo children $win] {
                            if {[regexp {^sep[0-9]+$} [winfo name $c]]} {
                                $c configure $opt $val
                            }
                        }
                    }
                    $w tag configure disabled $opt $val
                    updateColorsWhenIdle $win
                }
                -font {
                    #
                    # Apply the value to the header text widget and to
                    # the listbox child, rebuild the lists of the column
                    # fonts and tag names, configure the edit window if
                    # present, set up and adjust the columns, and make
                    # sure the items will be redisplayed at idle time
                    #
                    $data(hdrTxt) configure $opt $val
                    $data(lb) configure $opt $val
                    set data(charWidth) [font measure $val -displayof $win 0]
                    makeColFontAndTagLists $win
                    if {$data(editRow) >= 0} {
                        setEditWinFont $win
                    }
                    for {set col 0} {$col < $data(colCount)} {incr col} {
                        if {$data($col-maxwidth) > 0} {
                            set data($col-maxPixels) \
                                [charsToPixels $win $val $data($col-maxwidth)]
                        }
                    }
                    setupColumns $win $data(-columns) 0
                    adjustColumns $win allCols 1
                    redisplayWhenIdle $win
                }
                -foreground {
                    #
                    # Set the background color of the main separator
                    # frame (if any) to the specified value, and apply
                    # this value to the "disabled" tag if needed
                    #
                    if {$usingTile} {
                        styleConfig Sep$win.TSeparator -background $val
                    } else {
                        if {[winfo exists $data(sep)]} {
                            $data(sep) configure -background $val
                        }
                    }
                    if {[string compare $data(-disabledforeground) ""] == 0} {
                        $w tag configure disabled $opt $val
                    }
                    updateColorsWhenIdle $win
                }
            }
        }


        l {
            #
            # Apply the value to all not individually configured labels
            # and save the properly formatted value of val in data($opt)
            #
            set optTail [string range $opt 6 end]       ;# remove the -label
            configLabel $data(hdrLbl) -$optTail $val
            for {set col 0} {$col < $data(colCount)} {incr col} {
                set w $data(hdrTxtFrLbl)$col
                if {![info exists data($col$opt)]} {
                    configLabel $w -$optTail $val
                }
            }
            if {$usingTile && [string compare $opt "-labelpady"] == 0} {
                set data($opt) $val
            } else {
                set data($opt) [$data(hdrLbl) cget -$optTail]
            }


            switch -- $opt {
                -labelbackground -
                -labelforeground {
                    #
                    # Apply the value to $data(hdrTxt) and conditionally
                    # to the canvases displaying up- or down-arrows
                    #
                    $data(hdrTxt) configure -$optTail $data($opt)
                    foreach col $data(arrowColList) {
                        if {![info exists data($col$opt)]} {
                            configCanvas $win $col
                        }
                    }
                }
                -labelborderwidth {
                    #
                    # Adjust the columns (including
                    # the height of the header frame)
                    #
                    adjustColumns $win allLabels 1
                }
                -labeldisabledforeground {
                    #
                    # Conditionally apply the value to the
                    # canvases displaying up- or down-arrows
                    #
                    foreach col $data(arrowColList) {
                        if {![info exists data($col$opt)]} {
                            configCanvas $win $col
                        }
                    }
                }
                -labelfont {
                    #
                    # Adjust the columns (including
                    # the height of the header frame)
                    #
                    adjustColumns $win allLabels 1
                }
                -labelheight -
                -labelpady {
                    #
                    # Adjust the height of the header frame
                    #
                    adjustHeaderHeight $win
                }
            }
        }


        f {
            #
            # Apply the value to the frame and save the
            # properly formatted value of val in data($opt)
            #
            $win configure $opt $val
            set data($opt) [$win cget $opt]
        }


        w {
            switch -- $opt {
                -activestyle {
                    #
                    # Configure the "active" tag and save the
                    # properly formatted value of val in data($opt)
                    #
                    variable activeStyles
                    set val [mwutil::fullOpt "active style" $val $activeStyles]
                    set w $data(body)
                    switch $val {
                        frame {
                            $w tag configure active \
                                   -borderwidth 1 -relief solid -underline ""
                        }
                        none {
                            $w tag configure active \
                                   -borderwidth "" -relief "" -underline ""
                        }
                        underline {
                            $w tag configure active \
                                   -borderwidth "" -relief "" -underline 1
                        }
                    }
                    set data($opt) $val
                }
                -arrowcolor -
                -arrowdisabledcolor {
                    #
                    # Save the properly formatted value of val in data($opt)
                    # and set the color of the normal or disabled arrows
                    #
                    if {[string compare $val ""] == 0} {
                        set data($opt) ""
                    } else {
                        $helpLabel configure -foreground $val
                        set data($opt) [$helpLabel cget -foreground]
                    }
                    if {([string compare $opt "-arrowcolor"] == 0 &&
                         !$data(isDisabled)) ||
                        ([string compare $opt "-arrowdisabledcolor"] == 0 &&
                         $data(isDisabled))} {
                        foreach w [info commands $data(hdrTxtFrCanv)*] {
                            fillArrows $w $val
                        }
                    }
                }
                -arrowstyle {
                    #
                    # Save the properly formatted value of val in data($opt)
                    # and draw the corresponding arrows in the canvas widgets
                    #
                    variable arrowStyles
                    set data($opt) \
                        [mwutil::fullOpt "arrow style" $val $arrowStyles]
                    regexp {^(flat|sunken)([0-9]+)x([0-9]+)$} $data($opt) \
                           dummy relief width height
                    set data(arrowWidth) $width
                    foreach w [info commands $data(hdrTxtFrCanv)*] {
                        createArrows $w $width $height $relief
                        if {$data(isDisabled)} {
                            fillArrows $w $data(-arrowdisabledcolor)
                        } else {
                            fillArrows $w $data(-arrowcolor)
                        }
                    }
                    if {[llength $data(arrowColList)] > 0} {
                        foreach col $data(arrowColList) {
                            raiseArrow $win $col
                            lappend whichWidths l$col
                        }
                        adjustColumns $win $whichWidths 1
                    }
                }
                -columns {
                    #
                    # Set up and adjust the columns, rebuild
                    # the lists of the column fonts and tag
                    # names, and redisplay the items
                    #
                    set selCells [curcellselectionSubCmd $win]
                    setupColumns $win $val 1
                    adjustColumns $win allCols 1
                    adjustColIndex $win data(anchorCol) 1
                    adjustColIndex $win data(activeCol) 1
                    makeColFontAndTagLists $win
                    redisplay $win 0 $selCells
                }
                -disabledforeground {
                    #
                    # Configure the "disabled" tag in the body text widget and
                    # save the properly formatted value of val in data($opt)
                    #
                    set w $data(body)
                    if {[string compare $val ""] == 0} {
                        $w tag configure disabled -fgstipple gray50 \
                                -foreground $data(-foreground)
                        set data($opt) ""
                    } else {
                        $w tag configure disabled -fgstipple "" \
                                -foreground $val
                        set data($opt) [$w tag cget disabled -foreground]
                    }
                    if {$data(isDisabled)} {
                        updateColorsWhenIdle $win
                    }
                }
                -editendcommand -
                -editstartcommand -
                -labelcommand -
                -labelcommand2 -
                -selectmode -
                -sortcommand -
                -yscrollcommand {
                    set data($opt) $val
                }
                -exportselection {
                    #
                    # Save the boolean value specified by val in
                    # data($opt).  In addition, if the selection is
                    # exported and there are any selected rows in the
                    # widget then make win the new owner of the PRIMARY
                    # selection and register a callback to be invoked
                    # when it loses ownership of the PRIMARY selection
                    #
                    set data($opt) [expr {$val ? 1 : 0}]
                    if {$val &&
                        [llength [$data(body) tag nextrange select 1.0]] != 0} {
                        selection own -command \
                                [list ::tablelist::lostSelection $win] $win
                    }
                }
                -forceeditendcommand -
                -movablecolumns -
                -movablerows -
                -protecttitlecolumns -
                -resizablecolumns -
                -setfocus {
                    #
                    # Save the boolean value specified by val in data($opt)
                    #
                    set data($opt) [expr {$val ? 1 : 0}]
                }
                -height {
                    #
                    # Adjust the heights of the body text widget
                    # and of the listbox child, and save the
                    # properly formatted value of val in data($opt)
                    #
                    set val [format "%d" $val]  ;# integer check with error msg
                    if {$val <= 0} {
                        set nonHiddenRowCount \
                            [expr {$data(itemCount) - $data(hiddenRowCount)}]
                        $data(body) configure $opt $nonHiddenRowCount
                        $data(lb) configure $opt $nonHiddenRowCount
                    } else {
                        $data(body) configure $opt $val
                        $data(lb) configure $opt $val
                    }
                    set data($opt) $val
                }
                -incrarrowtype {
                    #
                    # Save the properly formatted value of val in
                    # data($opt) and raise the corresponding arrows
                    # if the currently mapped canvas widgets
                    #
                    variable arrowTypes
                    set data($opt) \
                        [mwutil::fullOpt "arrow type" $val $arrowTypes]
                    foreach col $data(arrowColList) {
                        raiseArrow $win $col
                    }
                }
                -listvariable {
                    #
                    # Associate val as list variable with the
                    # given widget and save it in data($opt)
                    #
                    makeListVar $win $val
                    set data($opt) $val
                    if {[string compare $val ""] == 0} {
                        set data(hasListVar) 0
                    } else {
                        set data(hasListVar) 1
                    }
                }
                -movecolumncursor -
                -movecursor -
                -resizecursor {
                    #
                    # Save the properly formatted value of val in data($opt)
                    #
                    $helpLabel configure -cursor $val
                    set data($opt) [$helpLabel cget -cursor]
                }
                -selectbackground -
                -selectforeground {
                    #
                    # Configure the "select" tag in the body text widget
                    # and save the properly formatted value of val in
                    # data($opt).  Don't use the built-in "sel" tag
                    # because on Windows the selection in a text widget only
                    # becomes visible when the window gets the input focus.
                    #
                    set w $data(body)
                    set optTail [string range $opt 7 end] ;# remove the -select
                    $w tag configure select -$optTail $val
                    set data($opt) [$w tag cget select -$optTail]
                    if {!$data(isDisabled)} {
                        updateColorsWhenIdle $win
                    }
                }
                -selecttype {
                    #
                    # Save the properly formatted value of val in data($opt)
                    #
                    variable selectTypes
                    set val [mwutil::fullOpt "selection type" $val $selectTypes]
                    set data($opt) $val
                }
                -selectborderwidth {
                    #
                    # Configure the "select" tag in the body text widget
                    # and save the properly formatted value of val in
                    # data($opt).  Don't use the built-in "sel" tag
                    # because on Windows the selection in a text widget only
                    # becomes visible when the window gets the input focus.
                    # In addition, adjust the line spacing accordingly and
                    # apply the value to the listbox child, too.
                    #
                    set w $data(body)
                    set optTail [string range $opt 7 end] ;# remove the -select
                    $w tag configure select -$optTail $val
                    set data($opt) [$w tag cget select -$optTail]
                    set pixVal [winfo pixels $w $val]
                    if {$pixVal < 0} {
                        set pixVal 0
                    }
                    set spacing [winfo pixels $w $data(-spacing)]
                    if {$spacing < 0} {
                        set spacing 0
                    }
                    $w configure -spacing1 [expr {$spacing + $pixVal}] \
                                 -spacing3 [expr {$spacing + $pixVal + 1}]
                    $data(lb) configure $opt $val
                    updateColorsWhenIdle $win
                    adjustSepsWhenIdle $win
                }
                -setgrid {
                    #
                    # Apply the value to the listbox child and save
                    # the properly formatted value of val in data($opt)
                    #
                    $data(lb) configure $opt $val
                    set data($opt) [$data(lb) cget $opt]
                }
                -showarrow {
                    #
                    # Save the boolean value specified by val in
                    # data($opt) and manage or unmanage the
                    # canvases displaying up- or down-arrows
                    #
                    set data($opt) [expr {$val ? 1 : 0}]
                    makeSortAndArrowColLists $win
                    adjustColumns $win allLabels 1
                }
                -showlabels {
                    #
                    # Save the boolean value specified by val in data($opt)
                    # and adjust the height of the header frame
                    #
                    set data($opt) [expr {$val ? 1 : 0}]
                    adjustHeaderHeight $win
                }
                -showseparators {
                    #
                    # Save the boolean value specified by val in data($opt),
                    # and create or destroy the separators if needed
                    #
                    set oldVal $data($opt)
                    set data($opt) [expr {$val ? 1 : 0}]
                    if {!$oldVal && $data($opt)} {
                        createSeps $win
                    } elseif {$oldVal && !$data($opt)} {
                        foreach w [winfo children $win] {
                            if {[regexp {^sep[0-9]+$} [winfo name $w]]} {
                                destroy $w
                            }
                        }
                    }
                }
                -snipstring {
                    #
                    # Save val in data($opt), adjust the columns, and make
                    # sure the items will be redisplayed at idle time
                    #
                    set data($opt) $val
                    adjustColumns $win {} 0
                    redisplayWhenIdle $win
                }
                -spacing {
                    #
                    # Adjust the line spacing and save val in data($opt)
                    #
                    set w $data(body)
                    set pixVal [winfo pixels $w $val]
                    if {$pixVal < 0} {
                        set pixVal 0
                    }
                    set selectBd [winfo pixels $w $data(-selectborderwidth)]
                    if {$selectBd < 0} {
                        set selectBd 0
                    }
                    $w configure -spacing1 [expr {$pixVal + $selectBd}] \
                                 -spacing3 [expr {$pixVal + $selectBd + 1}]
                    set data($opt) $val
                    updateColorsWhenIdle $win
                    adjustSepsWhenIdle $win
                }
                -state {
                    #
                    # Apply the value to all labels and their sublabels
                    # (if any), as well as to the edit window (if present),
                    # add/remove the "disabled" tag to/from the contents
                    # of the body text widget, configure the borderwidth
                    # of the "active" and "select" tags, save the
                    # properly formatted value of val in data($opt),
                    # and raise the corresponding arrow in the canvas
                    #
                    variable states
                    set val [mwutil::fullOpt "state" $val $states]
                    catch {
                        configLabel $data(hdrLbl) $opt $val
                        for {set col 0} {$col < $data(colCount)} {incr col} {
                            configLabel $data(hdrTxtFrLbl)$col $opt $val
                        }
                    }
                    if {$data(editRow) >= 0} {
                        catch {$data(bodyFrEd) configure $opt $val}
                    }
                    set w $data(body)
                    switch $val {
                        disabled {
                            $w tag add disabled 1.0 end
                            $w tag configure select -relief flat
                            set data(isDisabled) 1
                        }
                        normal {
                            $w tag remove disabled 1.0 end
                            $w tag configure select -relief raised
                            set data(isDisabled) 0
                        }
                    }
                    set data($opt) $val
                    foreach col $data(arrowColList) {
                        configCanvas $win $col
                        raiseArrow $win $col
                    }
                    updateColorsWhenIdle $win
                }
                -stretch {
                    #
                    # Save the properly formatted value of val in
                    # data($opt) and stretch the stretchable columns
                    #
                    if {[string first $val "all"] == 0} {
                        set data($opt) all
                    } else {
                        set data($opt) $val
                        sortStretchableColList $win
                    }
                    set data(forceAdjust) 1
                    stretchColumnsWhenIdle $win
                }
                -stripebackground -
                -stripeforeground {
                    #
                    # Configure the "stripe" tag in the body text
                    # widget, save the properly formatted value of val
                    # in data($opt), and draw the stripes if necessary
                    #
                    set w $data(body)
                    set optTail [string range $opt 7 end] ;# remove the -stripe
                    $w tag configure stripe -$optTail $val
                    set data($opt) [$w tag cget stripe -$optTail]
                    makeStripesWhenIdle $win
                }
                -stripeheight {
                    #
                    # Save the properly formatted value of val in
                    # data($opt) and draw the stripes if necessary
                    #
                    set val [format "%d" $val]  ;# integer check with error msg
                    set data($opt) $val
                    makeStripesWhenIdle $win
                }
                -targetcolor {
                    #
                    # Set the color of the row and column gaps, and save
                    # the properly formatted value of val in data($opt)
                    #
                    $data(rowGap) configure -background $val
                    $data(colGap) configure -background $val
                    set data($opt) [$data(rowGap) cget -background]
                }
                -titlecolumns {
                    #
                    # Update the value of the -xscrollcommand option, save
                    # the properly formatted value of val in data($opt), and
                    # create or destroy the main separator frame if needed
                    #
                    set oldVal $data($opt)
                    set val [format "%d" $val]  ;# integer check with error msg
                    if {$val < 0} {
                        set val 0
                    }
                    xviewSubCmd $win 0
                    set w $data(sep)
                    if {$val == 0} {
                        $data(hdrTxt) configure -xscrollcommand \
                                      $data(-xscrollcommand)
                        if {$oldVal > 0} {
                            destroy $w
                        }
                    } else {
                        $data(hdrTxt) configure -xscrollcommand ""
                        if {$oldVal == 0} {
                            if {$usingTile} {
                                ttk::separator $w -style Sep$win.TSeparator \
                                                   -cursor $data(-cursor) \
                                                   -orient vertical -takefocus 0
                            } else {
                                tk::frame $w -background $data(-foreground) \
                                             -borderwidth 1 -container 0 \
                                             -cursor $data(-cursor) \
                                             -highlightthickness 0 \
                                             -relief sunken -takefocus 0 \
                                             -width 2
                            }
                            bindtags $w [lreplace [bindtags $w] 1 1 \
                                         $data(bodyTag) TablelistBody]
                        }
                        adjustSepsWhenIdle $win
                    }
                    set data($opt) $val
                    xviewSubCmd $win 0
                    updateHScrlbarWhenIdle $win
                }
                -width {
                    #
                    # Adjust the widths of the body text widget,
                    # header frame, and listbox child, and save the
                    # properly formatted value of val in data($opt)
                    #
                    set val [format "%d" $val]  ;# integer check with error msg
                    $data(body) configure $opt $val
                    if {$val <= 0} {
                        $data(hdr) configure $opt $data(hdrPixels)
                        $data(lb) configure $opt \
                                  [expr {$data(hdrPixels) / $data(charWidth)}]
                    } else {
                        $data(hdr) configure $opt 0
                        $data(lb) configure $opt $val
                    }
                    set data($opt) $val
                }
                -xscrollcommand {
                    #
                    # Save val in data($opt), and apply it to the header text
                    # widget if (and only if) no title columns are being used
                    #
                    set data($opt) $val
                    if {$data(-titlecolumns) == 0} {
                        $data(hdrTxt) configure $opt $val
                    } else {
                        $data(hdrTxt) configure $opt ""
                    }
                }
            }
        }
    }
}


#------------------------------------------------------------------------------
# tablelist::doCget
#
# Returns the value of the configuration option opt for the tablelist widget
# win.
#------------------------------------------------------------------------------
proc tablelist::doCget {win opt} {
    upvar ::tablelist::ns${win}::data data


    return $data($opt)
}


#------------------------------------------------------------------------------
# tablelist::doColConfig
#
# Applies the value val of the column configuration option opt to the col'th
# column of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::doColConfig {col win opt val} {
    variable canElide
    upvar ::tablelist::ns${win}::data data


    switch -- $opt {
        -align {
            #
            # Set up and adjust the columns, and make sure the
            # given column will be redisplayed at idle time
            #
            set idx [expr {3*$col + 2}]
            setupColumns $win [lreplace $data(-columns) $idx $idx $val] 0
            adjustColumns $win {} 0
            redisplayColWhenIdle $win $col
        }


        -background -
        -foreground {
            set w $data(body)
            set name $col$opt


            if {[info exists data($name)] &&
                (!$data($col-hide) || $canElide)} {
                #
                # Remove the tag col$opt-$data($name)
                # from the elements of the given column
                #
                set tag col$opt-$data($name)
                for {set line 1} {$line <= $data(itemCount)} {incr line} {
                    findTabs $win $line $col $col tabIdx1 tabIdx2
                    $w tag remove $tag $tabIdx1 $tabIdx2+1c
                }
            }


            if {[string compare $val ""] == 0} {
                if {[info exists data($name)]} {
                    unset data($name)
                }
            } else {
                #
                # Configure the tag col$opt-$val in the body text widget
                #
                set tag col$opt-$val
                $w tag configure $tag $opt $val
                $w tag lower $tag


                if {!$data($col-hide) || $canElide} {
                    #
                    # Apply the tag to the elements of the given column
                    #
                    for {set line 1} {$line <= $data(itemCount)} {incr line} {
                        findTabs $win $line $col $col tabIdx1 tabIdx2
                        if {[lsearch -exact [$w tag names $tabIdx1] select]
                            < 0} {
                            $w tag add $tag $tabIdx1 $tabIdx2+1c
                        }
                    }
                }


                #
                # Save val in data($name)
                #
                set data($name) $val
            }


            if {!$data(isDisabled)} {
                updateColorsWhenIdle $win
            }


            #
            # Rebuild the lists of the column fonts and tag names
            #
            makeColFontAndTagLists $win
        }


        -editable -
        -resizable {
            #
            # Save the boolean value specified by val in data($col$opt)
            #
            set data($col$opt) [expr {$val ? 1 : 0}]
        }


        -editwindow {
            variable editWin
            if {[info exists editWin($val-registered)] ||
                [info exists editWin($val-creationCmd)]} {
                set data($col$opt) $val
            } else {
                return -code error "name \"$val\" is not registered\
                                    for interactive cell editing"
            }
        }


        -font {
            set w $data(body)
            set name $col$opt


            if {[info exists data($name)] &&
                (!$data($col-hide) || $canElide)} {
                #
                # Remove the tag col$opt-$data($name)
                # from the elements of the given column
                #
                set tag col$opt-$data($name)
                for {set line 1} {$line <= $data(itemCount)} {incr line} {
                    findTabs $win $line $col $col tabIdx1 tabIdx2
                    $w tag remove $tag $tabIdx1 $tabIdx2+1c
                }
            }


            if {[string compare $val ""] == 0} {
                if {[info exists data($name)]} {
                    unset data($name)
                }
            } else {
                #
                # Configure the tag col$opt-$val in the body text widget
                #
                set tag col$opt-$val
                $w tag configure $tag $opt $val
                $w tag lower $tag


                if {!$data($col-hide) || $canElide} {
                    #
                    # Apply the tag to the elements of the given column
                    #
                    for {set line 1} {$line <= $data(itemCount)} {incr line} {
                        findTabs $win $line $col $col tabIdx1 tabIdx2
                        $w tag add $tag $tabIdx1 $tabIdx2+1c
                    }
                }


                #
                # Save val in data($name)
                #
                set data($name) $val
            }


            #
            # Rebuild the lists of the column fonts and tag names
            #
            makeColFontAndTagLists $win


            #
            # Adjust the columns, and make sure the specified
            # column will be redisplayed at idle time
            #
            adjustColumns $win $col 1
            redisplayColWhenIdle $win $col


            adjustElidedTextWhenIdle $win


            if {$col == $data(editCol)} {
                #
                # Configure the edit window
                #
                setEditWinFont $win
            }
        }


        -formatcommand {
            if {[string compare $val ""] == 0} {
                if {[info exists data($col$opt)]} {
                    unset data($col$opt)
                }
                set fmtCmdFlag 0
            } else {
                set data($col$opt) $val
                set fmtCmdFlag 1
            }


            #
            # Update the corresponding element of the list data(fmtCmdFlagList)
            #
            set data(fmtCmdFlagList) \
                [lreplace $data(fmtCmdFlagList) $col $col $fmtCmdFlag]


            #
            # Adjust the columns and make sure the specified
            # column will be redisplayed at idle time
            #
            adjustColumns $win $col 1
            redisplayColWhenIdle $win $col
        }


        -hide {
            #
            # Save the boolean value specified by val in data($col$opt),
            # adjust the columns, and redisplay the items
            #
            set oldVal $data($col$opt)
            set newVal [expr {$val ? 1 : 0}]
            if {$newVal != $oldVal} {
                if {!$canElide} {
                    set selCells [curcellselectionSubCmd $win]
                } elseif {$newVal} {
                    cellselectionSubCmd $win clear 0 $col $data(lastRow) $col
                }
                set data($col$opt) $newVal
                if {$newVal} {                          ;# hiding the column
                    incr data(hiddenColCount)
                    adjustColIndex $win data(anchorCol) 1
                    adjustColIndex $win data(activeCol) 1
                    if {$col == $data(editCol)} {
                        canceleditingSubCmd $win
                    }
                } else {
                    incr data(hiddenColCount) -1
                }
                adjustColumns $win $col 1
                if {$canElide} {
                    adjustElidedTextWhenIdle $win
                } else {
                    redisplay $win 0 $selCells
                }
            }
        }


        -labelalign {
            if {[string compare $val ""] == 0} {
                #
                # Unset data($col$opt)
                #
                set alignment [lindex $data(colList) [expr {2*$col + 1}]]
                if {[info exists data($col$opt)]} {
                    unset data($col$opt)
                }
            } else {
                #
                # Save the properly formatted value of val in data($col$opt)
                #
                variable alignments
                set val [mwutil::fullOpt "label alignment" $val $alignments]
                set alignment $val
                set data($col$opt) $val
            }


            #
            # Adjust the col'th label
            #
            set pixels [lindex $data(colList) [expr {2*$col}]]
            if {$pixels == 0} {                 ;# convention: dynamic width
                if {$data($col-maxPixels) > 0} {
                    if {$data($col-reqPixels) > $data($col-maxPixels)} {
                        set pixels $data($col-maxPixels)
                    }
                }
            }
            if {$pixels != 0} { 
                incr pixels $data($col-delta)
            }
            adjustLabel $win $col $pixels $alignment
        }


        -labelbackground -
        -labelforeground {
            set w $data(hdrTxtFrLbl)$col
            set optTail [string range $opt 6 end]       ;# remove the -label
            if {[string compare $val ""] == 0} {
                #
                # Apply the value of the corresponding widget
                # configuration option to the col'th label and
                # its sublabels (if any), and unset data($col$opt)
                #
                configLabel $w -$optTail $data($opt)
                if {[info exists data($col$opt)]} {
                    unset data($col$opt)
                }
            } else {
                #
                # Apply the given value to the col'th label and
                # its sublabels (if any), and save the properly
                # formatted value of val in data($col$opt)
                #
                configLabel $w -$optTail $val
                set data($col$opt) [$w cget -$optTail]
            }


            if {[lsearch -exact $data(arrowColList) $col] >= 0} {
                configCanvas $win $col
            }
        }


        -labelborderwidth {
            set w $data(hdrTxtFrLbl)$col
            set optTail [string range $opt 6 end]       ;# remove the -label
            if {[string compare $val ""] == 0} {
                #
                # Apply the value of the corresponding widget configuration
                # option to the col'th label and unset data($col$opt)
                #
                configLabel $w -$optTail $data($opt)
                if {[info exists data($col$opt)]} {
                    unset data($col$opt)
                }
            } else {
                #
                # Apply the given value to the col'th label and save the
                # properly formatted value of val in data($col$opt)
                #
                configLabel $w -$optTail $val
                set data($col$opt) [$w cget -$optTail]
            }


            #
            # Adjust the columns (including the height of the header frame)
            #
            adjustColumns $win l$col 1
        }


        -labelcommand -
        -labelcommand2 -
        -name -
        -sortcommand {
            if {[string compare $val ""] == 0} {
                if {[info exists data($col$opt)]} {
                    unset data($col$opt)
                }
            } else {
                set data($col$opt) $val
            }
        }


        -labelfont {
            set w $data(hdrTxtFrLbl)$col
            set optTail [string range $opt 6 end]       ;# remove the -label
            if {[string compare $val ""] == 0} {
                #
                # Apply the value of the corresponding widget
                # configuration option to the col'th label and
                # its sublabels (if any), and unset data($col$opt)
                #
                configLabel $w -$optTail $data($opt)
                if {[info exists data($col$opt)]} {
                    unset data($col$opt)
                }
            } else {
                #
                # Apply the given value to the col'th label and
                # its sublabels (if any), and save the properly
                # formatted value of val in data($col$opt)
                #
                configLabel $w -$optTail $val
                set data($col$opt) [$w cget -$optTail]
            }


            #
            # Adjust the columns (including the height of the header frame)
            #
            adjustColumns $win l$col 1
        }


        -labelheight -
        -labelpady {
            set w $data(hdrTxtFrLbl)$col
            set optTail [string range $opt 6 end]       ;# remove the -label
            if {[string compare $val ""] == 0} {
                #
                # Apply the value of the corresponding widget configuration
                # option to the col'th label and unset data($col$opt)
                #
                configLabel $w -$optTail $data($opt)
                if {[info exists data($col$opt)]} {
                    unset data($col$opt)
                }
            } else {
                #
                # Apply the given value to the col'th label and save the
                # properly formatted value of val in data($col$opt)
                #
                configLabel $w -$optTail $val
                variable usingTile
                if {$usingTile} {
                    set data($col$opt) $val
                } else {
                    set data($col$opt) [$w cget -$optTail]
                }
            }


            #
            # Adjust the height of the header frame
            #
            adjustHeaderHeight $win
        }


        -labelimage {
            set w $data(hdrTxtFrLbl)$col
            if {[string compare $val ""] == 0} {
                foreach l [getSublabels $w] {
                    destroy $l
                }
                if {[info exists data($col$opt)]} {
                    unset data($col$opt)
                }
            } else {
                if {![winfo exists $w-il]} {
                    variable configSpecs
                    variable configOpts
                    foreach l [list $w-il $w-tl] {      ;# image and text labels
                        #
                        # Create the label $l
                        #
                        tk::label $l -borderwidth 0 -height 0 \
                                     -highlightthickness 0 -padx 0 \
                                     -pady 0 -takefocus 0 -width 0


                        #
                        # Apply to it the current configuration options
                        #
                        foreach opt2 $configOpts {
                            if {[string compare \
                                 [lindex $configSpecs($opt2) 2] "c"] == 0} {
                                $l configure $opt2 $data($opt2)
                            }
                        }
                        foreach opt2 {-background -foreground -font} {
                            $l configure $opt2 [$w cget $opt2]
                        }
                        foreach opt2 {-activebackground -activeforeground
                                      -disabledforeground -state} {
                            catch {$l configure $opt2 [$w cget $opt2]}
                        }


                        #
                        # Replace the binding tag Label with
                        # $w and TablelistSubLabel in the
                        # list of binding tags of the label $l
                        #
                        bindtags $l [lreplace [bindtags $l] 1 1 \
                                     $w TablelistSubLabel]
                    }
                }


                #
                # Display the specified image in the label
                # $w-il and save val in data($col$opt)
                #
                $w-il configure -image $val
                set data($col$opt) $val
            }


            #
            # Adjust the columns (including the height of the header frame)
            #
            adjustColumns $win l$col 1
        }


        -labelrelief {
            set w $data(hdrTxtFrLbl)$col
            set optTail [string range $opt 6 end]       ;# remove the -label
            if {[string compare $val ""] == 0} {
                #
                # Apply the value of the corresponding widget configuration
                # option to the col'th label and unset data($col$opt)
                #
                configLabel $w -$optTail $data($opt)
                if {[info exists data($col$opt)]} {
                    unset data($col$opt)
                }
            } else {
                #
                # Apply the given value to the col'th label and save the
                # properly formatted value of val in data($col$opt)
                #
                configLabel $w -$optTail $val
                set data($col$opt) [$w cget -$optTail]
            }
        }


        -maxwidth {
            #
            # Save the properly formatted value of val in
            # data($col$opt), adjust the columns, and make sure
            # the specified column will be redisplayed at idle time
            #
            set val [format "%d" $val]  ;# integer check with error message
            set data($col$opt) $val
            if {$val > 0} {             ;# convention: max. width in characters
                set pixels [charsToPixels $win $data(-font) $val]
            } elseif {$val < 0} {       ;# convention: max. width in pixels
                set pixels [expr {(-1)*$val}]
            } else {                    ;# convention: no max. width
                set pixels 0
            }
            set data($col-maxPixels) $pixels
            adjustColumns $win $col 1
            redisplayColWhenIdle $win $col
        }


        -selectbackground -
        -selectforeground {
            set w $data(body)
            set name $col$opt


            if {[info exists data($name)] &&
                (!$data($col-hide) || $canElide)} {
                #
                # Remove the tag col$opt-$data($name)
                # from the elements of the given column
                #
                set tag col$opt-$data($name)
                for {set line 1} {$line <= $data(itemCount)} {incr line} {
                    findTabs $win $line $col $col tabIdx1 tabIdx2
                    $w tag remove $tag $tabIdx1 $tabIdx2+1c
                }
            }


            if {[string compare $val ""] == 0} {
                if {[info exists data($name)]} {
                    unset data($name)
                }
            } else {
                #
                # Configure the tag col$opt-$val in the body text widget
                #
                set tag col$opt-$val
                set optTail [string range $opt 7 end]   ;# remove the -select
                $w tag configure $tag -$optTail $val
                $w tag raise $tag select


                if {!$data($col-hide) || $canElide} {
                    #
                    # Apply the tag to the selected elements of the given column
                    #
                    set selRange [$w tag nextrange select 1.0]
                    while {[llength $selRange] != 0} {
                        set selStart [lindex $selRange 0]
                        set line [expr {int($selStart)}]
                        findTabs $win $line $col $col tabIdx1 tabIdx2
                        if {[lsearch -exact [$w tag names $tabIdx1] select]
                            >= 0} {
                            $w tag add $tag $tabIdx1 $tabIdx2+1c
                        }


                        set selRange \
                            [$w tag nextrange select "$selStart lineend"]
                    }
                }


                #
                # Save val in data($name)
                #
                set data($name) $val
            }


            if {!$data(isDisabled)} {
                updateColorsWhenIdle $win
            }
        }


        -showarrow {
            #
            # Save the boolean value specified by val in data($col$opt) and
            # manage or unmanage the canvas displaying an up- or down-arrow
            #
            set data($col$opt) [expr {$val ? 1 : 0}]
            makeSortAndArrowColLists $win
            adjustColumns $win l$col 1
        }


        -showlinenumbers {
            #
            # Save the boolean value specified by val in
            # data($col$opt), and make sure the line numbers
            # will be redisplayed at idle time if needed
            #
            set val [expr {$val ? 1 : 0}]
            if {!$data($col$opt) && $val} {
                showLineNumbersWhenIdle $win
            }
            set data($col$opt) $val
        }


        -sortmode {
            #
            # Save the properly formatted value of val in data($col$opt)
            #
            variable sortModes
            set data($col$opt) [mwutil::fullOpt "sort mode" $val $sortModes]
        }


        -stretchable {
            set flag [expr {$val ? 1 : 0}]
            if {$flag} {
                if {[string compare $data(-stretch) "all"] != 0 &&
                    [lsearch -exact $data(-stretch) $col] < 0} {
                    #
                    # col was not found in data(-stretch): add it to the list
                    #
                    lappend data(-stretch) $col
                    sortStretchableColList $win
                    set data(forceAdjust) 1
                    stretchColumnsWhenIdle $win
                }
            } elseif {[string compare $data(-stretch) "all"] == 0} {
                #
                # Replace the value "all" of data(-stretch) with
                # the list of all column indices different from col
                #
                set data(-stretch) {}
                for {set n 0} {$n < $data(colCount)} {incr n} {
                    if {$n != $col} {
                        lappend data(-stretch) $n
                    }
                }
                set data(forceAdjust) 1
                stretchColumnsWhenIdle $win
            } else {
                #
                # If col is contained in data(-stretch)
                # then remove it from the list
                #
                if {[set n [lsearch -exact $data(-stretch) $col]] >= 0} {
                    set data(-stretch) [lreplace $data(-stretch) $n $n]
                    set data(forceAdjust) 1
                    stretchColumnsWhenIdle $win
                }


                #
                # If col indicates the last column and data(-stretch)
                # contains "end" then remove "end" from the list
                #
                if {$col == $data(lastCol) &&
                    [string compare [lindex $data(-stretch) end] "end"] == 0} {
                    set data(-stretch) [lreplace $data(-stretch) end end]
                    set data(forceAdjust) 1
                    stretchColumnsWhenIdle $win
                }
            }
        }


        -text {
            if {$data(isDisabled)} {
                return ""
            }


            #
            # Replace the column's contents in the internal list
            #
            set newItemList {}
            set row 0
            foreach item $data(itemList) text [lrange $val 0 $data(itemCount)] {
                set item [lreplace $item $col $col $text]
                lappend newItemList $item
            }
            set data(itemList) $newItemList


            #
            # Update the list variable if present
            #
            condUpdateListVar $win


            #
            # Adjust the columns and make sure the specified
            # column will be redisplayed at idle time
            #
            adjustColumns $win $col 1
            redisplayColWhenIdle $win $col
        }


        -title {
            #
            # Save the given value in the corresponding
            # element of data(-columns) and adjust the columns
            #
            set idx [expr {3*$col + 1}]
            set data(-columns) [lreplace $data(-columns) $idx $idx $val]
            adjustColumns $win l$col 1
        }


        -width {
            #
            # Set up and adjust the columns, and make sure the
            # given column will be redisplayed at idle time
            #
            set idx [expr {3*$col}]
            if {$val != [lindex $data(-columns) $idx]} {
                setupColumns $win [lreplace $data(-columns) $idx $idx $val] 0
                adjustColumns $win $col 1
                redisplayColWhenIdle $win $col
            }
        }
    }
}


#------------------------------------------------------------------------------
# tablelist::doColCget
#
# Returns the value of the column configuration option opt for the col'th
# column of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::doColCget {col win opt} {
    upvar ::tablelist::ns${win}::data data


    switch -- $opt {
        -align {
            return [lindex $data(-columns) [expr {3*$col + 2}]]
        }


        -stretchable {
            return [expr {
                [string compare $data(-stretch) "all"] == 0 ||
                [lsearch -exact $data(-stretch) $col] >= 0 ||
                ($col == $data(lastCol) && \
                 [string compare [lindex $data(-stretch) end] "end"] == 0)
            }]
        }


        -text {
            set result {}
            foreach item $data(itemList) {
                lappend result [lindex $item $col]
            }
            return $result
        }


        -title {
            return [lindex $data(-columns) [expr {3*$col + 1}]]
        }


        -width {
            return [lindex $data(-columns) [expr {3*$col}]]
        }


        default {
            if {[info exists data($col$opt)]} {
                return $data($col$opt)
            } else {
                return ""
            }
        }
    }
}


#------------------------------------------------------------------------------
# tablelist::doRowConfig
#
# Applies the value val of the row configuration option opt to the row'th row
# of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::doRowConfig {row win opt val} {
    variable canElide
    variable elide
    upvar ::tablelist::ns${win}::data data


    set w $data(body)


    switch -- $opt {
        -background -
        -foreground {
            set key [lindex [lindex $data(itemList) $row] end]
            set name $key$opt


            if {[info exists data($name)]} {
                #
                # Remove the tag row$opt-$data($name) from the given row
                #
                set tag row$opt-$data($name)
                set line [expr {$row + 1}]
                $w tag remove $tag $line.0 $line.end
            }


            if {[string compare $val ""] == 0} {
                if {[info exists data($name)]} {
                    unset data($name)
                    incr data(tagRefCount) -1
                }
            } else {
                #
                # Configure the tag row$opt-$val in the body text widget and
                # apply it to the non-selected elements of the given row
                #
                set tag row$opt-$val
                $w tag configure $tag $opt $val
                $w tag lower $tag active
                set line [expr {$row + 1}]
                set textIdx1 [expr {double($line)}]
                for {set col 0} {$col < $data(colCount)} {incr col} {
                    if {$data($col-hide) && !$canElide} {
                        continue
                    }


                    set textIdx2 \
                        [$w search $elide "\t" $textIdx1+1c $line.end]+1c
                    if {[lsearch -exact [$w tag names $textIdx1] select] < 0} {
                        $w tag add $tag $textIdx1 $textIdx2
                    }
                    set textIdx1 $textIdx2
                }


                #
                # Save val in data($name)
                #
                if {![info exists data($name)]} {
                    incr data(tagRefCount)
                }
                set data($name) $val
            }


            if {!$data(isDisabled)} {
                updateColorsWhenIdle $win
            }
        }


        -font {
            #
            # Save the current cell fonts in a temporary array
            #
            set item [lindex $data(itemList) $row]
            set key [lindex $item end]
            for {set col 0} {$col < $data(colCount)} {incr col} {
                set oldCellFonts($col) [getCellFont $win $key $col]
            }


            set name $key$opt
            if {[info exists data($name)]} {
                #
                # Remove the tag row$opt-$data($name) from the given row
                #
                set tag row$opt-$data($name)
                set line [expr {$row + 1}]
                $w tag remove $tag $line.0 $line.end
            }


            if {[string compare $val ""] == 0} {
                if {[info exists data($name)]} {
                    unset data($name)
                    incr data(tagRefCount) -1
                }
            } else {
                #
                # Configure the tag row$opt-$val in the body
                # text widget and apply it to the given row
                #
                set tag row$opt-$val
                $w tag configure $tag $opt $val
                $w tag lower $tag active
                set line [expr {$row + 1}]
                $w tag add $tag $line.0 $line.end


                #
                # Save val in data($name)
                #
                if {![info exists data($name)]} {
                    incr data(tagRefCount)
                }
                set data($name) $val
            }


            if {[lsearch -exact $data(fmtCmdFlagList) 1] >= 0} {
                set formattedItem \
                    [formatItem $win [lrange $item 0 $data(lastCol)]]
            } else {
                set formattedItem [lrange $item 0 $data(lastCol)]
            }
            set colWidthsChanged 0
            set colIdxList {}
            set line [expr {$row + 1}]
            set textIdx1 $line.1
            set col 0
            foreach text [strToDispStr $formattedItem] \
                    {pixels alignment} $data(colList) {
                if {$data($col-hide) && !$canElide} {
                    incr col
                    continue
                }


                #
                # Adjust the cell text and the image or window width
                #
                if {[string match "*\n*" $text]} {
                    set multiline 1
                    set list [split $text "\n"]
                } else {
                    set multiline 0
                }
                set aux [getAuxData $win $key $col auxType auxWidth]
                set cellFont [getCellFont $win $key $col]
                set workPixels $pixels
                if {$pixels == 0} {             ;# convention: dynamic width
                    if {$data($col-maxPixels) > 0} {
                        if {$data($col-reqPixels) > $data($col-maxPixels)} {
                            set workPixels $data($col-maxPixels)
                            set textSav $text
                            set auxWidthSav $auxWidth
                        }
                    }
                }
                if {$workPixels != 0} {
                    incr workPixels $data($col-delta)
                }
                if {$multiline} {
                    adjustMlElem $win list auxWidth $cellFont $workPixels \
                                 $alignment $data(-snipstring)
                    set msgScript [list ::tablelist::displayText $win $key \
                                   $col [join $list "\n"] $cellFont $alignment]
                } else {
                    adjustElem $win text auxWidth $cellFont $workPixels \
                               $alignment $data(-snipstring)
                }


                if {$row == $data(editRow) && $col == $data(editCol)} {
                    #
                    # Configure the edit window
                    #
                    setEditWinFont $win
                } else {
                    #
                    # Update the text widget's contents between the two tabs
                    #
                    set textIdx2 [$w search $elide "\t" $textIdx1 $line.end]
                    if {$multiline} {
                        updateMlCell $w $textIdx1 $textIdx2 $msgScript \
                                     $aux $auxType $auxWidth $alignment
                    } else {
                        updateCell $w $textIdx1 $textIdx2 $text \
                                   $aux $auxType $auxWidth $alignment
                    }
                }


                if {$pixels == 0} {             ;# convention: dynamic width
                    #
                    # Check whether the width of the current column has changed
                    #
                    if {$workPixels > 0} {
                        set text $textSav
                        set auxWidth $auxWidthSav
                    }
                    set textWidth \
                        [getCellTextWidth $win $text $auxWidth $cellFont]
                    set newElemWidth [expr {$auxWidth + $textWidth}]
                    if {$newElemWidth > $data($col-elemWidth)} {
                        set data($col-elemWidth) $newElemWidth
                        set data($col-widestCount) 1
                        if {$newElemWidth > $data($col-reqPixels)} {
                            set data($col-reqPixels) $newElemWidth
                            set colWidthsChanged 1
                        }
                    } else {
                        set oldTextWidth [getCellTextWidth $win $text \
                                          $auxWidth $oldCellFonts($col)]
                        set oldElemWidth [expr {$auxWidth + $oldTextWidth}]
                        if {$oldElemWidth < $data($col-elemWidth) &&
                            $newElemWidth == $data($col-elemWidth)} {
                            incr data($col-widestCount)
                        } elseif {$oldElemWidth == $data($col-elemWidth) &&
                                  $newElemWidth < $oldElemWidth &&
                                  [incr data($col-widestCount) -1] == 0} {
                            set colWidthsChanged 1
                            lappend colIdxList $col
                        }
                    }
                }


                set textIdx1 [$w search $elide "\t" $textIdx1 $line.end]+2c
                incr col
            }


            #
            # Adjust the columns if necessary and schedule
            # some operations for execution at idle time
            #
            if {$colWidthsChanged} {
                adjustColumns $win $colIdxList 1
            }
            adjustElidedTextWhenIdle $win
            updateColorsWhenIdle $win
            adjustSepsWhenIdle $win
        }


        -hide {
            set val [expr {$val ? 1 : 0}]
            set item [lindex $data(itemList) $row]
            set key [lindex $item end]
            set name $key$opt
            set line [expr {$row + 1}]
            set viewChanged 0


            if {$val} {                                 ;# hiding the row
                if {![info exists data($name)]} {
                    selectionSubCmd $win clear $row $row
                    set data($name) 1
                    incr data(hiddenRowCount)
                    $w tag add hiddenRow $line.0 $line.end+1c
                    set viewChanged 1
                    adjustRowIndex $win data(anchorRow) 1
                    adjustRowIndex $win data(activeRow) 1
                    if {$row == $data(editRow)} {
                        canceleditingSubCmd $win
                    }
                }
            } else {                                    ;# unhiding the row
                if {[info exists data($name)]} {
                    unset data($name)
                    incr data(hiddenRowCount) -1
                    $w tag remove hiddenRow $line.0 $line.end+1c
                    set viewChanged 1
                }
            }


            if {$viewChanged} {
                #
                # Adjust the heights of the body text widget
                # and of the listbox child, if necessary
                #
                if {$data(-height) <= 0} {
                    set nonHiddenRowCount \
                        [expr {$data(itemCount) - $data(hiddenRowCount)}]
                    $w configure -height $nonHiddenRowCount
                    $data(lb) configure -height $nonHiddenRowCount
                }


                #
                # Build the list of those dynamic-width columns
                # whose widths are affected by (un)hiding the row
                #
                set colWidthsChanged 0
                set colIdxList {}
                if {[lsearch -exact $data(fmtCmdFlagList) 1] >= 0} {
                    set formattedItem \
                        [formatItem $win [lrange $item 0 $data(lastCol)]]
                } else {
                    set formattedItem [lrange $item 0 $data(lastCol)]
                }
                set col 0
                foreach text [strToDispStr $formattedItem] \
                        {pixels alignment} $data(colList) {
                    if {($data($col-hide) && !$canElide) || $pixels != 0} {
                        incr col
                        continue
                    }


                    getAuxData $win $key $col auxType auxWidth
                    set cellFont [getCellFont $win $key $col]
                    set textWidth \
                        [getCellTextWidth $win $text $auxWidth $cellFont]
                    set elemWidth [expr {$auxWidth + $textWidth}]
                    if {$val} {                         ;# hiding the row
                        if {$elemWidth == $data($col-elemWidth) &&
                            [incr data($col-widestCount) -1] == 0} {
                            set colWidthsChanged 1
                            lappend colIdxList $col
                        }
                    } else {                            ;# unhiding the row
                        if {$elemWidth == $data($col-elemWidth)} {
                            incr data($col-widestCount)
                        } elseif {$elemWidth > $data($col-elemWidth)} {
                            set data($col-elemWidth) $elemWidth
                            set data($col-widestCount) 1
                            if {$elemWidth > $data($col-reqPixels)} {
                                set data($col-reqPixels) $elemWidth
                                set colWidthsChanged 1
                            }
                        }
                    }


                    incr col
                }


                #
                # Invalidate the list of the row indices indicating the
                # non-hidden rows, adjust the columns if necessary, and
                # schedule some operations for execution at idle time
                #
                set data(nonHiddenRowList) {-1}
                if {$colWidthsChanged} {
                    adjustColumns $win $colIdxList 1
                }
                adjustElidedTextWhenIdle $win
                makeStripesWhenIdle $win
                adjustSepsWhenIdle $win
                updateVScrlbarWhenIdle $win
                showLineNumbersWhenIdle $win
            }
        }


        -name {
            set key [lindex [lindex $data(itemList) $row] end]
            if {[string compare $val ""] == 0} {
                if {[info exists data($key$opt)]} {
                    unset data($key$opt)
                }
            } else {
                set data($key$opt) $val
            }
        }


        -selectable {
            set val [expr {$val ? 1 : 0}]
            set key [lindex [lindex $data(itemList) $row] end]


            if {$val} {
                if {[info exists data($key$opt)]} {
                    unset data($key$opt)
                }
            } else {
                #
                # Set data($key$opt) to 0 and deselect the row
                #
                set data($key$opt) 0
                selectionSubCmd $win clear $row $row
            }
        }


        -selectbackground -
        -selectforeground {
            set key [lindex [lindex $data(itemList) $row] end]
            set name $key$opt


            if {[info exists data($name)]} {
                #
                # Remove the tag row$opt-$data($name) from the given row
                #
                set tag row$opt-$data($name)
                set line [expr {$row + 1}]
                $w tag remove $tag $line.0 $line.end
            }


            if {[string compare $val ""] == 0} {
                if {[info exists data($name)]} {
                    unset data($name)
                }
            } else {
                #
                # Configure the tag row$opt-$val in the body text widget
                # and apply it to the selected elements of the given row
                #
                set tag row$opt-$val
                set optTail [string range $opt 7 end]   ;# remove the -select
                $w tag configure $tag -$optTail $val
                $w tag lower $tag active
                set line [expr {$row + 1}]
                set textIdx1 [expr {double($line)}]
                for {set col 0} {$col < $data(colCount)} {incr col} {
                    if {$data($col-hide) && !$canElide} {
                        continue
                    }


                    set textIdx2 \
                        [$w search $elide "\t" $textIdx1+1c $line.end]+1c
                    if {[lsearch -exact [$w tag names $textIdx1] select] >= 0} {
                        $w tag add $tag $textIdx1 $textIdx2
                    }
                    set textIdx1 $textIdx2
                }


                #
                # Save val in data($name)
                #
                set data($name) [$w tag cget $tag -$optTail]
            }


            if {!$data(isDisabled)} {
                updateColorsWhenIdle $win
            }
        }


        -text {
            if {$data(isDisabled)} {
                return ""
            }


            set colWidthsChanged 0
            set colIdxList {}
            set oldItem [lindex $data(itemList) $row]
            set key [lindex $oldItem end]
            set newItem [adjustItem $val $data(colCount)]
            if {[lsearch -exact $data(fmtCmdFlagList) 1] >= 0} {
                set formattedItem [formatItem $win $newItem]
            } else {
                set formattedItem $newItem
            }
            set line [expr {$row + 1}]
            set textIdx1 $line.1
            set col 0
            foreach text [strToDispStr $formattedItem] \
                    {pixels alignment} $data(colList) {
                if {$data($col-hide) && !$canElide} {
                    incr col
                    continue
                }


                #
                # Adjust the cell text and the image or window width
                #
                if {[string match "*\n*" $text]} {
                    set multiline 1
                    set list [split $text "\n"]
                } else {
                    set multiline 0
                }
                set aux [getAuxData $win $key $col auxType auxWidth]
                set cellFont [getCellFont $win $key $col]
                set workPixels $pixels
                if {$pixels == 0} {             ;# convention: dynamic width
                    if {$data($col-maxPixels) > 0} {
                        if {$data($col-reqPixels) > $data($col-maxPixels)} {
                            set workPixels $data($col-maxPixels)
                            set textSav $text
                            set auxWidthSav $auxWidth
                        }
                    }
                }
                if {$workPixels != 0} {
                    incr workPixels $data($col-delta)
                }
                if {$multiline} {
                    adjustMlElem $win list auxWidth $cellFont $workPixels \
                                 $alignment $data(-snipstring)
                    set msgScript [list ::tablelist::displayText $win $key \
                                   $col [join $list "\n"] $cellFont $alignment]
                } else {
                    adjustElem $win text auxWidth $cellFont $workPixels \
                               $alignment $data(-snipstring)
                }


                if {$row != $data(editRow) || $col != $data(editCol)} {
                    #
                    # Update the text widget's contents between the two tabs
                    #
                    set textIdx2 [$w search $elide "\t" $textIdx1 $line.end]
                    if {$multiline} {
                        updateMlCell $w $textIdx1 $textIdx2 $msgScript \
                                     $aux $auxType $auxWidth $alignment
                    } else {
                        updateCell $w $textIdx1 $textIdx2 $text \
                                   $aux $auxType $auxWidth $alignment
                    }
                }


                if {$pixels == 0} {             ;# convention: dynamic width
                    #
                    # Check whether the width of the current column has changed
                    #
                    if {$workPixels > 0} {
                        set text $textSav
                        set auxWidth $auxWidthSav
                    }
                    set textWidth \
                        [getCellTextWidth $win $text $auxWidth $cellFont]
                    set newElemWidth [expr {$auxWidth + $textWidth}]
                    if {$newElemWidth > $data($col-elemWidth)} {
                        set data($col-elemWidth) $newElemWidth
                        set data($col-widestCount) 1
                        if {$newElemWidth > $data($col-reqPixels)} {
                            set data($col-reqPixels) $newElemWidth
                            set colWidthsChanged 1
                        }
                    } else {
                        set oldText [lindex $oldItem $col]
                        if {[info exists data($col-formatcommand)]} {
                            set oldText [uplevel #0 $data($col-formatcommand) \
                                         [list $oldText]]
                        }
                        set oldText [strToDispStr $oldText]
                        set oldTextWidth \
                            [getCellTextWidth $win $oldText $auxWidth $cellFont]
                        set oldElemWidth [expr {$auxWidth + $oldTextWidth}]
                        if {$oldElemWidth < $data($col-elemWidth) &&
                            $newElemWidth == $data($col-elemWidth)} {
                            incr data($col-widestCount)
                        } elseif {$oldElemWidth == $data($col-elemWidth) &&
                                  $newElemWidth < $oldElemWidth &&
                                  [incr data($col-widestCount) -1] == 0} {
                            set colWidthsChanged 1
                            lappend colIdxList $col
                        }
                    }
                }


                set textIdx1 [$w search $elide "\t" $textIdx1 $line.end]+2c
                incr col
            }


            #
            # Replace the row contents in the list variable if present
            #
            if {$data(hasListVar)} {
                upvar #0 $data(-listvariable) var
                trace vdelete var wu $data(listVarTraceCmd)
                set var [lreplace $var $row $row $newItem]
                trace variable var wu $data(listVarTraceCmd)
            }


            #
            # Replace the row contents in the internal list
            #
            lappend newItem [lindex $oldItem end]
            set data(itemList) [lreplace $data(itemList) $row $row $newItem]


            #
            # Adjust the columns if necessary and schedule
            # some operations for execution at idle time
            #
            if {$colWidthsChanged} {
                adjustColumns $win $colIdxList 1
            }
            adjustElidedTextWhenIdle $win
            updateColorsWhenIdle $win
            adjustSepsWhenIdle $win
            showLineNumbersWhenIdle $win
        }
    }
}


#------------------------------------------------------------------------------
# tablelist::doRowCget
#
# Returns the value of the row configuration option opt for the row'th row of
# the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::doRowCget {row win opt} {
    upvar ::tablelist::ns${win}::data data


    #
    # Return the value of the specified row configuration option
    #
    set item [lindex $data(itemList) $row]
    switch -- $opt {
        -text {
            return [lrange $item 0 $data(lastCol)]
        }


        -hide {
            set key [lindex $item end]
            if {[info exists data($key$opt)]} {
                return $data($key$opt)
            } else {
                return 0
            }
        }


        -selectable {
            set key [lindex $item end]
            if {[info exists data($key$opt)]} {
                return $data($key$opt)
            } else {
                return 1
            }
        }


        default {
            set key [lindex $item end]
            if {[info exists data($key$opt)]} {
                return $data($key$opt)
            } else {
                return ""
            }
        }
    }
}


#------------------------------------------------------------------------------
# tablelist::doCellConfig
#
# Applies the value val of the cell configuration option opt to the cell
# row,col of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::doCellConfig {row col win opt val} {
    variable canElide
    upvar ::tablelist::ns${win}::data data


    set w $data(body)


    switch -- $opt {
        -background -
        -foreground {
            set key [lindex [lindex $data(itemList) $row] end]
            set name $key,$col$opt


            if {[info exists data($name)] &&
                (!$data($col-hide) || $canElide)} {
                #
                # Remove the tag cell$opt-$data($name) from the given cell
                #
                set tag cell$opt-$data($name)
                findTabs $win [expr {$row + 1}] $col $col tabIdx1 tabIdx2
                $w tag remove $tag $tabIdx1 $tabIdx2+1c
            }


            if {[string compare $val ""] == 0} {
                if {[info exists data($name)]} {
                    unset data($name)
                    incr data(tagRefCount) -1
                }
            } else {
                #
                # Configure the tag cell$opt-$val in the body text widget
                #
                set tag cell$opt-$val
                $w tag configure $tag $opt $val
                $w tag lower $tag disabled


                if {!$data($col-hide) || $canElide} {
                    #
                    # Apply the tag to the given cell if it is not selected
                    #
                    findTabs $win [expr {$row + 1}] $col $col tabIdx1 tabIdx2
                    if {[lsearch -exact [$w tag names $tabIdx1] select] < 0} {
                        $w tag add $tag $tabIdx1 $tabIdx2+1c
                    }
                }


                #
                # Save val in data($name)
                #
                if {![info exists data($name)]} {
                    incr data(tagRefCount)
                }
                set data($name) $val
            }


            if {!$data(isDisabled)} {
                updateColorsWhenIdle $win
            }
        }


        -editable {
            #
            # Save the boolean value specified by val in data($key,$col$opt)
            #
            set key [lindex [lindex $data(itemList) $row] end]
            set data($key,$col$opt) [expr {$val ? 1 : 0}]
        }


        -editwindow {
            variable editWin
            if {[info exists editWin($val-registered)] ||
                [info exists editWin($val-creationCmd)]} {
                set key [lindex [lindex $data(itemList) $row] end]
                set data($key,$col$opt) $val
            } else {
                return -code error "name \"$val\" is not registered\
                                    for interactive cell editing"
            }
        }


        -font {
            #
            # Save the current cell font
            #
            set item [lindex $data(itemList) $row]
            set key [lindex $item end]
            set name $key,$col$opt
            set oldCellFont [getCellFont $win $key $col]


            if {[info exists data($name)] &&
                (!$data($col-hide) || $canElide)} {
                #
                # Remove the tag cell$opt-$data($name) from the given cell
                #
                set tag cell$opt-$data($name)
                findTabs $win [expr {$row + 1}] $col $col tabIdx1 tabIdx2
                $w tag remove $tag $tabIdx1 $tabIdx2+1c
            }


            if {[string compare $val ""] == 0} {
                if {[info exists data($name)]} {
                    unset data($name)
                    incr data(tagRefCount) -1
                }
            } else {
                #
                # Configure the tag cell$opt-$val in the body text widget
                #
                set tag cell$opt-$val
                $w tag configure $tag $opt $val
                $w tag lower $tag disabled


                if {!$data($col-hide) || $canElide} {
                    #
                    # Apply the tag to the given cell
                    #
                    findTabs $win [expr {$row + 1}] $col $col tabIdx1 tabIdx2
                    $w tag add $tag $tabIdx1 $tabIdx2+1c
                }


                #
                # Save val in data($name)
                #
                if {![info exists data($name)]} {
                    incr data(tagRefCount)
                }
                set data($name) $val
            }


            #
            # Adjust the cell text and the image or window width
            #
            set text [lindex $item $col]
            if {[info exists data($col-formatcommand)]} {
                set text [uplevel #0 $data($col-formatcommand) [list $text]]
            }
            set text [strToDispStr $text]
            if {[string match "*\n*" $text]} {
                set multiline 1
                set list [split $text "\n"]
            } else {
                set multiline 0
            }
            set aux [getAuxData $win $key $col auxType auxWidth]
            set cellFont [getCellFont $win $key $col]
            set pixels [lindex $data(colList) [expr {2*$col}]]
            set workPixels $pixels
            if {$pixels == 0} {                 ;# convention: dynamic width
                if {$data($col-maxPixels) > 0} {
                    if {$data($col-reqPixels) > $data($col-maxPixels)} {
                        set workPixels $data($col-maxPixels)
                        set textSav $text
                        set auxWidthSav $auxWidth
                    }
                }
            }
            if {$workPixels != 0} {
                incr workPixels $data($col-delta)
            }
            set alignment [lindex $data(colList) [expr {2*$col + 1}]]
            if {$multiline} {
                adjustMlElem $win list auxWidth $cellFont $workPixels \
                             $alignment $data(-snipstring)
                set msgScript [list ::tablelist::displayText $win $key \
                               $col [join $list "\n"] $cellFont $alignment]
            } else {
                adjustElem $win text auxWidth $cellFont $workPixels \
                           $alignment $data(-snipstring)
            }


            if {!$data($col-hide)} {
                if {$row == $data(editRow) && $col == $data(editCol)} {
                    #
                    # Configure the edit window
                    #
                    setEditWinFont $win
                } else {
                    #
                    # Update the text widget's contents between the two tabs
                    #
                    findTabs $win [expr {$row + 1}] $col $col tabIdx1 tabIdx2
                    if {$multiline} {
                        updateMlCell $w $tabIdx1+1c $tabIdx2 $msgScript \
                                     $aux $auxType $auxWidth $alignment
                    } else {
                        updateCell $w $tabIdx1+1c $tabIdx2 $text \
                                   $aux $auxType $auxWidth $alignment
                    }
                }
            }


            #
            # Adjust the columns if necessary
            #
            if {$pixels == 0} {                 ;# convention: dynamic width
                if {$workPixels > 0} {
                    set text $textSav
                    set auxWidth $auxWidthSav
                }
                set textWidth [getCellTextWidth $win $text $auxWidth $cellFont]
                set newElemWidth [expr {$auxWidth + $textWidth}]
                if {$newElemWidth > $data($col-elemWidth)} {
                    set data($col-elemWidth) $newElemWidth
                    set data($col-widestCount) 1
                    if {$newElemWidth > $data($col-reqPixels)} {
                        set data($col-reqPixels) $newElemWidth
                        adjustColumns $win {} 1
                    }
                } else {
                    set oldTextWidth \
                        [getCellTextWidth $win $text $auxWidth $oldCellFont]
                    set oldElemWidth [expr {$auxWidth + $oldTextWidth}]
                    if {$oldElemWidth < $data($col-elemWidth) &&
                        $newElemWidth == $data($col-elemWidth)} {
                        incr data($col-widestCount)
                    } elseif {$oldElemWidth == $data($col-elemWidth) &&
                              $newElemWidth < $oldElemWidth &&
                              [incr data($col-widestCount) -1] == 0} {
                        adjustColumns $win $col 1
                    }
                }
            }


            adjustElidedTextWhenIdle $win
            updateColorsWhenIdle $win
            adjustSepsWhenIdle $win
        }


        -image {
            if {$data(isDisabled)} {
                return ""
            }


            #
            # Save the old image or window width
            #
            set item [lindex $data(itemList) $row]
            set key [lindex $item end]
            set name $key,$col$opt
            getAuxData $win $key $col oldAuxWidth oldAuxType


            #
            # Delete data($name) or save the specified value in it
            #
            if {[string compare $val ""] == 0} {
                if {[info exists data($name)]} {
                    unset data($name)
                    incr data(imgCount) -1
                }
            } else {
                if {![info exists data($name)]} {
                    incr data(imgCount)
                }
                set aux $w.l$key,$col
                set existsAux [winfo exists $aux]
                if {$existsAux && [info exists data($name)] &&
                    [string compare $val $data($name)] == 0} {
                    set keepAux 1
                } else {
                    set keepAux 0
                    if {$existsAux} {
                        destroy $aux
                    }
                }
                set data($name) $val
            }


            #
            # Adjust the cell text and the image or window width
            #
            set text [lindex $item $col]
            if {[info exists data($col-formatcommand)]} {
                set text [uplevel #0 $data($col-formatcommand) [list $text]]
            }
            set text [strToDispStr $text]
            set oldText $text
            if {[string match "*\n*" $text]} {
                set multiline 1
                set list [split $text "\n"]
            } else {
                set multiline 0
            }
            set aux [getAuxData $win $key $col auxType auxWidth]
            set cellFont [getCellFont $win $key $col]
            set pixels [lindex $data(colList) [expr {2*$col}]]
            set workPixels $pixels
            if {$pixels == 0} {                 ;# convention: dynamic width
                if {$data($col-maxPixels) > 0} {
                    if {$data($col-reqPixels) > $data($col-maxPixels)} {
                        set workPixels $data($col-maxPixels)
                        set textSav $text
                        set auxWidthSav $auxWidth
                    }
                }
            }
            if {$workPixels != 0} {
                incr workPixels $data($col-delta)
            }
            set alignment [lindex $data(colList) [expr {2*$col + 1}]]
            if {$multiline} {
                adjustMlElem $win list auxWidth $cellFont $workPixels \
                             $alignment $data(-snipstring)
                set msgScript [list ::tablelist::displayText $win $key \
                               $col [join $list "\n"] $cellFont $alignment]
            } else {
                adjustElem $win text auxWidth $cellFont $workPixels \
                           $alignment $data(-snipstring)
            }


            if {(!$data($col-hide) || $canElide) &&
                !($row == $data(editRow) && $col == $data(editCol))} {
                #
                # Delete the old cell contents between the two tabs,
                # and insert the text and the auxiliary object
                #
                findTabs $win [expr {$row + 1}] $col $col tabIdx1 tabIdx2
                if {$auxType != 1 || $keepAux} {
                    if {$multiline} {
                        updateMlCell $w $tabIdx1+1c $tabIdx2 $msgScript \
                                     $aux $auxType $auxWidth $alignment
                    } else {
                        updateCell $w $tabIdx1+1c $tabIdx2 $text \
                                   $aux $auxType $auxWidth $alignment
                    }
                } else {
                    set aux [lreplace $aux end end $auxWidth]
                    $w delete $tabIdx1+1c $tabIdx2
                    if {$multiline} {
                        insertMlElem $w $tabIdx1+1c $msgScript \
                                     $aux $auxType $alignment
                    } else {
                        insertElem $w $tabIdx1+1c $text $aux $auxType $alignment
                    }
                }
            }


            #
            # Adjust the columns if necessary
            #
            if {$pixels == 0} {                 ;# convention: dynamic width
                if {$workPixels > 0} {
                    set text $textSav
                    set auxWidth $auxWidthSav
                }
                set textWidth [getCellTextWidth $win $text $auxWidth $cellFont]
                set newElemWidth [expr {$auxWidth + $textWidth}]
                if {$newElemWidth > $data($col-elemWidth)} {
                    set data($col-elemWidth) $newElemWidth
                    set data($col-widestCount) 1
                    if {$newElemWidth > $data($col-reqPixels)} {
                        set data($col-reqPixels) $newElemWidth
                        adjustColumns $win {} 1
                    }
                } else {
                    set oldTextWidth \
                        [getCellTextWidth $win $oldText $oldAuxWidth $cellFont]
                    set oldElemWidth [expr {$oldAuxWidth + $oldTextWidth}]
                    if {$oldElemWidth < $data($col-elemWidth) &&
                        $newElemWidth == $data($col-elemWidth)} {
                        incr data($col-widestCount)
                    } elseif {$oldElemWidth == $data($col-elemWidth) &&
                              $newElemWidth < $oldElemWidth &&
                              [incr data($col-widestCount) -1] == 0} {
                        adjustColumns $win $col 1
                    }
                }
            }


            adjustElidedTextWhenIdle $win
            updateColorsWhenIdle $win
            adjustSepsWhenIdle $win
        }


        -selectbackground -
        -selectforeground {
            set key [lindex [lindex $data(itemList) $row] end]
            set name $key,$col$opt


            if {[info exists data($name)] &&
                (!$data($col-hide) || $canElide)} {
                #
                # Remove the tag cell$opt-$data($name) from the given cell
                #
                set tag cell$opt-$data($name)
                findTabs $win [expr {$row + 1}] $col $col tabIdx1 tabIdx2
                $w tag remove $tag $tabIdx1 $tabIdx2+1c
            }


            if {[string compare $val ""] == 0} {
                if {[info exists data($name)]} {
                    unset data($name)
                }
            } else {
                #
                # Configure the tag cell$opt-$val in the body text widget
                #
                set tag cell$opt-$val
                set optTail [string range $opt 7 end]   ;# remove the -select
                $w tag configure $tag -$optTail $val
                $w tag lower $tag disabled


                if {!$data($col-hide) || $canElide} {
                    #
                    # Apply the tag to the given cell if it is selected
                    #
                    findTabs $win [expr {$row + 1}] $col $col tabIdx1 tabIdx2
                    if {[lsearch -exact [$w tag names $tabIdx1] select] >= 0} {
                        $w tag add $tag $tabIdx1 $tabIdx2+1c
                    }
                }


                #
                # Save val in data($name)
                #
                set data($name) $val
            }


            if {!$data(isDisabled)} {
                updateColorsWhenIdle $win
            }
        }


        -text {
            if {$data(isDisabled)} {
                return ""
            }


            set pixels [lindex $data(colList) [expr {2*$col}]]
            set workPixels $pixels
            if {$pixels == 0} {                 ;# convention: dynamic width
                if {$data($col-maxPixels) > 0} {
                    if {$data($col-reqPixels) > $data($col-maxPixels)} {
                        set workPixels $data($col-maxPixels)
                    }
                }
            }
            if {$workPixels != 0} {
                incr workPixels $data($col-delta)
            }
            set alignment [lindex $data(colList) [expr {2*$col + 1}]]


            #
            # Adjust the cell text and the image or window width
            #
            set text $val
            set fmtCmdFlag [info exists data($col-formatcommand)]
            if {$fmtCmdFlag} {
                set text [uplevel #0 $data($col-formatcommand) [list $text]]
            }
            set text [strToDispStr $text]
            set textSav $text
            if {[string match "*\n*" $text]} {
                set multiline 1
                set list [split $text "\n"]
            } else {
                set multiline 0
            }
            set oldItem [lindex $data(itemList) $row]
            set key [lindex $oldItem end]
            set aux [getAuxData $win $key $col auxType auxWidth]
            set auxWidthSav $auxWidth
            set cellFont [getCellFont $win $key $col]
            if {$multiline} {
                adjustMlElem $win list auxWidth $cellFont $workPixels \
                             $alignment $data(-snipstring)
                set msgScript [list ::tablelist::displayText $win $key \
                               $col [join $list "\n"] $cellFont $alignment]
            } else {
                adjustElem $win text auxWidth $cellFont $workPixels \
                           $alignment $data(-snipstring)
            }


            if {(!$data($col-hide) || $canElide) &&
                !($row == $data(editRow) && $col == $data(editCol))} {
                #
                # Update the text widget's contents between the two tabs
                #
                findTabs $win [expr {$row + 1}] $col $col tabIdx1 tabIdx2
                if {$multiline} {
                    updateMlCell $w $tabIdx1+1c $tabIdx2 $msgScript \
                                 $aux $auxType $auxWidth $alignment
                } else {
                    updateCell $w $tabIdx1+1c $tabIdx2 $text \
                               $aux $auxType $auxWidth $alignment
                }
            }


            #
            # Replace the cell contents in the internal list
            #
            set newItem [lreplace $oldItem $col $col $val]
            set data(itemList) [lreplace $data(itemList) $row $row $newItem]


            #
            # Replace the cell contents in the list variable if present
            #
            if {$data(hasListVar)} {
                upvar #0 $data(-listvariable) var
                trace vdelete var wu $data(listVarTraceCmd)
                set var [lreplace $var $row $row \
                         [lrange $newItem 0 $data(lastCol)]]
                trace variable var wu $data(listVarTraceCmd)
            }


            #
            # Adjust the columns if necessary
            #
            if {$pixels == 0} {                 ;# convention: dynamic width
                if {$workPixels > 0} {
                    set text $textSav
                    set auxWidth $auxWidthSav
                }
                set textWidth [getCellTextWidth $win $text $auxWidth $cellFont]
                set newElemWidth [expr {$auxWidth + $textWidth}]
                if {$newElemWidth > $data($col-elemWidth)} {
                    set data($col-elemWidth) $newElemWidth
                    set data($col-widestCount) 1
                    if {$newElemWidth > $data($col-reqPixels)} {
                        set data($col-reqPixels) $newElemWidth
                        adjustColumns $win {} 1
                    }
                } else {
                    set oldText [lindex $oldItem $col]
                    if {$fmtCmdFlag} {
                        set oldText [uplevel #0 $data($col-formatcommand) \
                                     [list $oldText]]
                    }
                    set oldText [strToDispStr $oldText]
                    set oldTextWidth \
                        [getCellTextWidth $win $oldText $auxWidth $cellFont]
                    set oldElemWidth [expr {$auxWidth + $oldTextWidth}]
                    if {$oldElemWidth < $data($col-elemWidth) &&
                        $newElemWidth == $data($col-elemWidth)} {
                        incr data($col-widestCount)
                    } elseif {$oldElemWidth == $data($col-elemWidth) &&
                              $newElemWidth < $oldElemWidth &&
                              [incr data($col-widestCount) -1] == 0} {
                        adjustColumns $win $col 1
                    }
                }
            }


            adjustElidedTextWhenIdle $win
            updateColorsWhenIdle $win
            adjustSepsWhenIdle $win
        }


        -window {
            if {$data(isDisabled)} {
                return ""
            }


            #
            # Save the old image or window width
            #
            set item [lindex $data(itemList) $row]
            set key [lindex $item end]
            set name $key,$col$opt
            getAuxData $win $key $col oldAuxWidth oldAuxType


            #
            # Delete data($name) or save the specified value in it
            #
            if {[string compare $val ""] == 0} {
                if {[info exists data($name)]} {
                    unset data($name)
                    unset data($key,$col-reqWidth)
                    unset data($key,$col-reqHeight)


                    #
                    # If the cell index is contained in the list
                    # data(cellsToReconfig) then remove it from the list
                    #
                    set n [lsearch -exact $data(cellsToReconfig) $row,$col]
                    if {$n >= 0} {
                        set data(cellsToReconfig) \
                            [lreplace $data(cellsToReconfig) $n $n]
                    }
                    incr data(winCount) -1
                }
            } else {
                if {![info exists data($name)]} {
                    incr data(winCount)
                }
                set aux $w.f$key,$col
                set existsAux [winfo exists $aux]
                if {$existsAux && [info exists data($name)] &&
                    [string compare $val $data($name)] == 0} {
                    set keepAux 1
                } else {
                    set keepAux 0
                    if {$existsAux} {
                        destroy $aux
                    }


                    #
                    # Create the frame and evaluate the specified script
                    # that creates a child widget within the frame
                    #
                    tk::frame $aux -borderwidth 0 -class TablelistWindow \
                                   -container 0 -highlightthickness 0 \
                                   -relief flat -takefocus 0
                    uplevel #0 $val [list $win $row $col $aux.w]
                }
                set data($name) $val
                set data($key,$col-reqWidth) [winfo reqwidth $aux.w]
                set data($key,$col-reqHeight) [winfo reqheight $aux.w]
                $aux configure -height $data($key,$col-reqHeight)


                #
                # Add the cell index to the list data(cellsToReconfig) if
                # the window's requested width or height is not yet known
                #
                if {$data($key,$col-reqWidth) == 1 ||
                    $data($key,$col-reqHeight) == 1} {
                    lappend data(cellsToReconfig) $row,$col
                    if {![info exists data(reconfigId)]} {
                        set data(reconfigId) \
                            [after idle [list tablelist::reconfigWindows $win]]
                    }
                }
            }


            #
            # Adjust the cell text and the image or window width
            #
            set text [lindex $item $col]
            if {[info exists data($col-formatcommand)]} {
                set text [uplevel #0 $data($col-formatcommand) [list $text]]
            }
            set text [strToDispStr $text]
            set oldText $text
            if {[string match "*\n*" $text]} {
                set multiline 1
                set list [split $text "\n"]
            } else {
                set multiline 0
            }
            set aux [getAuxData $win $key $col auxType auxWidth]
            set cellFont [getCellFont $win $key $col]
            set pixels [lindex $data(colList) [expr {2*$col}]]
            set workPixels $pixels
            if {$pixels == 0} {                 ;# convention: dynamic width
                if {$data($col-maxPixels) > 0} {
                    if {$data($col-reqPixels) > $data($col-maxPixels)} {
                        set workPixels $data($col-maxPixels)
                        set textSav $text
                        set auxWidthSav $auxWidth
                    }
                }
            }
            if {$workPixels != 0} {
                incr workPixels $data($col-delta)
            }
            set alignment [lindex $data(colList) [expr {2*$col + 1}]]
            if {$multiline} {
                adjustMlElem $win list auxWidth $cellFont $workPixels \
                             $alignment $data(-snipstring)
                set msgScript [list ::tablelist::displayText $win $key \
                               $col [join $list "\n"] $cellFont $alignment]
            } else {
                adjustElem $win text auxWidth $cellFont $workPixels \
                           $alignment $data(-snipstring)
            }


            if {(!$data($col-hide) || $canElide) &&
                !($row == $data(editRow) && $col == $data(editCol))} {
                #
                # Delete the old cell contents between the two tabs,
                # and insert the text and the auxiliary object
                #
                findTabs $win [expr {$row + 1}] $col $col tabIdx1 tabIdx2
                if {$auxType != 2 || $keepAux} {
                    if {$multiline} {
                        updateMlCell $w $tabIdx1+1c $tabIdx2 $msgScript \
                                     $aux $auxType $auxWidth $alignment
                    } else {
                        updateCell $w $tabIdx1+1c $tabIdx2 $text \
                                   $aux $auxType $auxWidth $alignment
                    }
                } else {
                    $aux configure -width $auxWidth
                    $w delete $tabIdx1+1c $tabIdx2
                    if {$multiline} {
                        insertMlElem $w $tabIdx1+1c $msgScript \
                                     $aux $auxType $alignment
                    } else {
                        insertElem $w $tabIdx1+1c $text $aux $auxType $alignment
                    }
                }
            }


            #
            # Adjust the columns if necessary
            #
            if {$pixels == 0} {                 ;# convention: dynamic width
                if {$workPixels > 0} {
                    set text $textSav
                    set auxWidth $auxWidthSav
                }
                set textWidth [getCellTextWidth $win $text $auxWidth $cellFont]
                set newElemWidth [expr {$auxWidth + $textWidth}]
                if {$newElemWidth > $data($col-elemWidth)} {
                    set data($col-elemWidth) $newElemWidth
                    set data($col-widestCount) 1
                    if {$newElemWidth > $data($col-reqPixels)} {
                        set data($col-reqPixels) $newElemWidth
                        adjustColumns $win {} 1
                    }
                } else {
                    set oldTextWidth \
                        [getCellTextWidth $win $oldText $oldAuxWidth $cellFont]
                    set oldElemWidth [expr {$oldAuxWidth + $oldTextWidth}]
                    if {$oldElemWidth < $data($col-elemWidth) &&
                        $newElemWidth == $data($col-elemWidth)} {
                        incr data($col-widestCount)
                    } elseif {$oldElemWidth == $data($col-elemWidth) &&
                              $newElemWidth < $oldElemWidth &&
                              [incr data($col-widestCount) -1] == 0} {
                        adjustColumns $win $col 1
                    }
                }
            }


            adjustElidedTextWhenIdle $win
            updateColorsWhenIdle $win
            adjustSepsWhenIdle $win
        }


        -windowdestroy {
            set key [lindex [lindex $data(itemList) $row] end]
            set name $key,$col$opt


            #
            # Delete data($name) or save the specified value in it
            #
            if {[string compare $val ""] == 0} {
                if {[info exists data($name)]} {
                    unset data($name)
                }
            } else {
                set data($name) $val
            }
        }
    }
}


#------------------------------------------------------------------------------
# tablelist::doCellCget
#
# Returns the value of the cell configuration option opt for the cell row,col
# of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::doCellCget {row col win opt} {
    upvar ::tablelist::ns${win}::data data


    #
    # Return the value of the specified cell configuration option
    #
    switch -- $opt {
        -editable {
            return [isCellEditable $win $row $col]
        }


        -editwindow {
            return [getEditWindow $win $row $col]
        }


        -text {
            return [lindex [lindex $data(itemList) $row] $col]
        }


        default {
            set key [lindex [lindex $data(itemList) $row] end]
            if {[info exists data($key,$col$opt)]} {
                return $data($key,$col$opt)
            } else {
                return ""
            }
        }
    }
}


#------------------------------------------------------------------------------
# tablelist::makeListVar
#
# Arranges for the global variable specified by varName to become the list
# variable associated with the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::makeListVar {win varName} {
    upvar ::tablelist::ns${win}::data data


    if {[string compare $varName ""] == 0} {
        #
        # If there is an old list variable associated with the
        # widget then remove the trace set on this variable
        #
        if {$data(hasListVar)} {
            synchronize $win
            upvar #0 $data(-listvariable) var
            trace vdelete var wu $data(listVarTraceCmd)
        }
        return ""
    }


    #
    # The list variable may be an array element but must not be an array
    #
    if {![regexp {^(.*)\((.*)\)$} $varName dummy name1 name2]} {
        if {[array exists $varName]} {
            return -code error "variable \"$varName\" is array"
        }
        set name1 $varName
        set name2 ""
    }


    #
    # If there is an old list variable associated with the
    # widget then remove the trace set on this variable
    #
    if {$data(hasListVar)} {
        synchronize $win
        upvar #0 $data(-listvariable) var
        trace vdelete var wu $data(listVarTraceCmd)
    }


    upvar #0 $varName var
    if {[info exists var]} {
        #
        # Invoke the trace procedure associated with the new list variable
        #
        listVarTrace $win $name1 $name2 w
    } else {
        #
        # Set $varName according to the value of data(itemList)
        #
        set var {}
        foreach item $data(itemList) {
            lappend var [lrange $item 0 $data(lastCol)]
        }
    }


    #
    # Set a trace on the new list variable
    #
    trace variable var wu $data(listVarTraceCmd)
}


#------------------------------------------------------------------------------
# tablelist::getCellFont
#
# Returns the font to be used in the tablelist cell specified by win, key, and
# col.
#------------------------------------------------------------------------------
proc tablelist::getCellFont {win key col} {
    upvar ::tablelist::ns${win}::data data


    if {[info exists data($key,$col-font)]} {
        return $data($key,$col-font)
    } elseif {[info exists data($key-font)]} {
        return $data($key-font)
    } else {
        return [lindex $data(colFontList) $col]
    }
}


#------------------------------------------------------------------------------
# tablelist::reconfigWindows
#
# Invoked as an after idle callback, this procedure forces any geometry manager
# calculations to be completed and then applies the -window option a second
# time to those cells whose embedded windows' requested widths or heights were
# still unknown.
#------------------------------------------------------------------------------
proc tablelist::reconfigWindows win {
    upvar ::tablelist::ns${win}::data data


    #
    # Force any geometry manager calculations to be completed first
    #
    update idletasks


    set cellsToReconfig $data(cellsToReconfig)
    set data(cellsToReconfig) {}
    unset data(reconfigId)


    #
    # Reconfigure the cells specified in the list cellsToReconfig
    #
    foreach cellIdx $cellsToReconfig {
        foreach {row col} [split $cellIdx ","] {}
        set key [lindex [lindex $data(itemList) $row] end]
        if {[info exists data($key,$col-window)]} {
            doCellConfig $row $col $win -window $data($key,$col-window)
        }
    }
}


#------------------------------------------------------------------------------
# tablelist::isCellEditable
#
# Checks whether the given cell of the tablelist widget win is editable.
#------------------------------------------------------------------------------
proc tablelist::isCellEditable {win row col} {
    upvar ::tablelist::ns${win}::data data


    set key [lindex [lindex $data(itemList) $row] end]
    if {[info exists data($key,$col-editable)]} {
        return $data($key,$col-editable)
    } else {
        return $data($col-editable)
    }
}


#------------------------------------------------------------------------------
# tablelist::getEditWindow
#
# Returns the value of the -editwindow option at cell or column level for the
# given cell of the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::getEditWindow {win row col} {
    upvar ::tablelist::ns${win}::data data


    set key [lindex [lindex $data(itemList) $row] end]
    if {[info exists data($key,$col-editwindow)]} {
        return $data($key,$col-editwindow)
    } elseif {[info exists data($col-editwindow)]} {
        return $data($col-editwindow)
    } else {
        return "entry"
    }
}
