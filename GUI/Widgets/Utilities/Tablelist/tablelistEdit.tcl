#==============================================================================
# Contains the implementation of interactive cell editing in tablelist widgets.
#
# Structure of the module:
#   - Namespace initialization
#   - Public procedures related to interactive cell editing
#   - Private procedures implementing the interactive cell editing
#   - Private procedures used in bindings related to interactive cell editing
#
# Copyright (c) 2003-2006  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================

#
# Namespace initialization
# ========================
#

namespace eval tablelist {
    #
    # Define the binding tag TablelistEdit
    #
    proc defineTablelistEdit {} {
	#
	# Get the supported modifier keys in the set {Alt, Meta, Command} on
	# the current windowing system ("x11", "win32", "classic", or "aqua")
	#
	variable winSys
	if {[catch {tk windowingsystem} winSys] != 0} {
	    switch $::tcl_platform(platform) {
		unix      { set winSys x11 }
		windows   { set winSys win32 }
		macintosh { set winSys classic }
	    }
	}
	switch $winSys {
	    x11		{ set modList {Alt Meta} }
	    win32	{ set modList {Alt} }
	    classic -
	    aqua	{ set modList {Command} }
	}

	#
	# Define some bindings for the binding tag TablelistEdit
	#
	bind TablelistEdit <Button-1> {
	    set tablelist::priv(clicked) 1
	    set tablelist::priv(clickedInEditWin) 1
	    focus %W
	}
	bind TablelistEdit <ButtonRelease-1> {
	    if {%t != 0} {			;# i.e., no generated event
		foreach {tablelist::W tablelist::x tablelist::y} \
		    [tablelist::convEventFields %W %x %y] {}

		set tablelist::priv(x) ""
		set tablelist::priv(y) ""
		set tablelist::priv(clicked) 0
		after cancel $tablelist::priv(afterId)
		set tablelist::priv(afterId) ""
		set tablelist::priv(releasedInEditWin) 1
		if {%t - $tablelist::priv(clickTime) < 300} {
		    tablelist::moveOrActivate $tablelist::W \
			$tablelist::priv(row) $tablelist::priv(col)
		} else {
		    tablelist::moveOrActivate $tablelist::W \
			[$tablelist::W nearest       $tablelist::y] \
			[$tablelist::W nearestcolumn $tablelist::x]
		}
		tablelist::condEvalInvokeCmd $tablelist::W
	    }
	}
	bind TablelistEdit <Control-i>    { tablelist::insertChar %W "\t" }
	bind TablelistEdit <Control-j>    { tablelist::insertChar %W "\n" }
	bind TablelistEdit <Escape>       { tablelist::cancelEditing %W }
	foreach key {Return KP_Enter} {
	    bind TablelistEdit <$key> {
		if {[string compare [winfo class %W] "Text"] == 0} {
		    tablelist::insertChar %W "\n"
		} else {
		    tablelist::finishEditing %W
		}
	    }
	    bind TablelistEdit <Control-$key> {
		tablelist::finishEditing %W
	    }
	}
	bind TablelistEdit <Tab>          { tablelist::goToNextPrevCell %W  1 }
	bind TablelistEdit <Shift-Tab>    { tablelist::goToNextPrevCell %W -1 }
	bind TablelistEdit <<PrevWindow>> { tablelist::goToNextPrevCell %W -1 }
	foreach modifier $modList {
	    bind TablelistEdit <$modifier-Left> {
		tablelist::goLeftRight %W -1
	    }
	    bind TablelistEdit <$modifier-Right> {
		tablelist::goLeftRight %W 1
	    }
	    bind TablelistEdit <$modifier-Up> {
		tablelist::goUpDown %W -1
	    }
	    bind TablelistEdit <$modifier-Down> {
		tablelist::goUpDown %W 1
	    }
	    bind TablelistEdit <$modifier-Prior> {
		tablelist::goToPriorNextPage %W -1
	    }
	    bind TablelistEdit <$modifier-Next> {
		tablelist::goToPriorNextPage %W 1
	    }
	    bind TablelistEdit <$modifier-Home> {
		tablelist::goToNextPrevCell %W 1 0 -1
	    }
	    bind TablelistEdit <$modifier-End> {
		tablelist::goToNextPrevCell %W -1 0 0
	    }
	}
	foreach direction {Left Right} amount {-1 1} {
	    bind TablelistEdit <$direction> [format {
		if {![tablelist::isKeyReserved %%W %%K]} {
		    tablelist::goLeftRight %%W %d
		}
	    } $amount]
	}
	foreach direction {Up Down} amount {-1 1} {
	    bind TablelistEdit <$direction> [format {
		if {![tablelist::isKeyReserved %%W %%K]} {
		    tablelist::goUpDown %%W %d
		}
	    } $amount]
	}
	foreach page {Prior Next} amount {-1 1} {
	    bind TablelistEdit <$page> [format {
		if {![tablelist::isKeyReserved %%W %%K]} {
		    tablelist::goToPriorNextPage %%W %d
		}
	    } $amount]
	}
	bind TablelistEdit <Control-Home> {
	    if {![tablelist::isKeyReserved %W Control-Home]} {
		tablelist::goToNextPrevCell %W 1 0 -1
	    }
	}
	bind TablelistEdit <Control-End> {
	    if {![tablelist::isKeyReserved %W Control-End]} {
		tablelist::goToNextPrevCell %W -1 0 0
	    }
	}
	foreach pattern {Tab Shift-Tab ISO_Left_Tab hpBackTab} {
	    catch {
		foreach modifier {Control Meta} {
		    bind TablelistEdit <$modifier-$pattern> [format {
			mwutil::processTraversal %%W Tablelist <%s>
		    } $pattern]
		}
	    }
	}
	bind TablelistEdit <FocusIn> {
	    set tablelist::W [tablelist::getTablelistPath %W]
	    set tablelist::ns${tablelist::W}::data(editFocus) %W
	}

	#
	# Define some emacs-like key bindings for the binding tag TablelistEdit
	#
	foreach pattern {Meta-b Meta-f} amount {-1 1} {
	    bind TablelistEdit <$pattern> [format {
		if {!$tk_strictMotif && ![tablelist::isKeyReserved %%W %s]} {
		    tablelist::goLeftRight %%W %d
		}
	    } $pattern $amount]
	}
	foreach pattern {Control-p Control-n} amount {-1 1} {
	    bind TablelistEdit <$pattern> [format {
		if {!$tk_strictMotif && ![tablelist::isKeyReserved %%W %s]} {
		    tablelist::goUpDown %%W %d
		}
	    } $pattern $amount]
	}
	bind TablelistEdit <Meta-less> {
	    if {!$tk_strictMotif &&
		![tablelist::isKeyReserved %W Meta-less]} {
		tablelist::goToNextPrevCell %W 1 0 -1
	    }
	}
	bind TablelistEdit <Meta-greater> {
	    if {!$tk_strictMotif &&
		![tablelist::isKeyReserved %W Meta-greater]} {
		tablelist::goToNextPrevCell %W -1 0 0
	    }
	}

	#
	# Define some bindings for the binding tag TablelistEdit that
	# propagate the mousewheel events to the tablelist's body
	#
	catch {
	    bind TablelistEdit <MouseWheel> {
		if {![tablelist::isComboTopMapped %W]} {
		    tablelist::genMouseWheelEvent \
			[[tablelist::getTablelistPath %W] bodypath] %D
		}
	    }
	}
	foreach detail {4 5} {
	    bind TablelistEdit <Button-$detail> [format {
		if {![tablelist::isComboTopMapped %%W]} {
		    event generate \
			[[tablelist::getTablelistPath %%W] bodypath] <Button-%s>
		}
	    } $detail]
	}
    }
    defineTablelistEdit 

    #
    # Register the Tk core widgets entry, text, checkbutton,
    # and spinbox for interactive cell editing
    #
    proc addTkCoreWidgets {} {
	set name entry
	array set ::tablelist::editWin [list \
	    $name-creationCmd	"$name %W -width 0" \
	    $name-putValueCmd	"%W delete 0 end; %W insert 0 %T" \
	    $name-getValueCmd	"%W get" \
	    $name-putTextCmd	"%W delete 0 end; %W insert 0 %T" \
	    $name-getTextCmd	"%W get" \
	    $name-putListCmd	"" \
	    $name-getListCmd	"" \
	    $name-selectCmd	"" \
	    $name-invokeCmd	"" \
	    $name-fontOpt	-font \
	    $name-useFormat	1 \
	    $name-useReqWidth	0 \
	    $name-usePadX	0 \
	    $name-isEntryLike	1 \
	    $name-focusWin	%W \
	    $name-reservedKeys	{Left Right} \
	]

	set name text
	array set ::tablelist::editWin [list \
	    $name-creationCmd	"$name %W -padx 2 -pady 2 -wrap none" \
	    $name-putValueCmd	"%W delete 1.0 end; %W insert 1.0 %T" \
	    $name-getValueCmd	"%W get 1.0 end-1c" \
	    $name-putTextCmd	"%W delete 1.0 end; %W insert 1.0 %T" \
	    $name-getTextCmd	"%W get 1.0 end-1c" \
	    $name-putListCmd	"" \
	    $name-getListCmd	"" \
	    $name-selectCmd	"" \
	    $name-invokeCmd	"" \
	    $name-fontOpt	-font \
	    $name-useFormat	1 \
	    $name-useReqWidth	0 \
	    $name-usePadX	0 \
	    $name-isEntryLike	1 \
	    $name-focusWin	%W \
	    $name-reservedKeys	{Left Right Up Down Prior Next
				 Control-Home Control-End Meta-b Meta-f
				 Control-p Control-n Meta-less Meta-greater} \
	]

	set name checkbutton
	array set ::tablelist::editWin [list \
	    $name-creationCmd	"createCheckbutton %W" \
	    $name-putValueCmd	{set [%W cget -variable] %T} \
	    $name-getValueCmd	{set [%W cget -variable]} \
	    $name-putTextCmd	{set [%W cget -variable] %T} \
	    $name-getTextCmd	{set [%W cget -variable]} \
	    $name-putListCmd	"" \
	    $name-getListCmd	"" \
	    $name-selectCmd	"" \
	    $name-invokeCmd	"%W invoke" \
	    $name-fontOpt	"" \
	    $name-useFormat	0 \
	    $name-useReqWidth	1 \
	    $name-usePadX	0 \
	    $name-isEntryLike	0 \
	    $name-focusWin	%W \
	    $name-reservedKeys	{} \
	]

	if {$::tk_version < 8.4} {
	    return ""
	}

	set name spinbox
	array set ::tablelist::editWin [list \
	    $name-creationCmd	"$name %W -width 0" \
	    $name-putValueCmd	"%W delete 0 end; %W insert 0 %T" \
	    $name-getValueCmd	"%W get" \
	    $name-putTextCmd	"%W delete 0 end; %W insert 0 %T" \
	    $name-getTextCmd	"%W get" \
	    $name-putListCmd	"" \
	    $name-getListCmd	"" \
	    $name-selectCmd	"" \
	    $name-invokeCmd	"" \
	    $name-fontOpt	-font \
	    $name-useFormat	1 \
	    $name-useReqWidth	0 \
	    $name-usePadX	1 \
	    $name-isEntryLike	1 \
	    $name-focusWin	%W \
	    $name-reservedKeys	{Left Right Up Down} \
	]
    }
    addTkCoreWidgets 

    #
    # Register the tile widgets ttk::entry, ttk::combobox,
    # and ttk::checkbutton for interactive cell editing
    #
    proc addTileWidgets {} {
	set name ttk::entry
	array set ::tablelist::editWin [list \
	    $name-creationCmd	"createTileEntry %W" \
	    $name-putValueCmd	"%W delete 0 end; %W insert 0 %T" \
	    $name-getValueCmd	"%W get" \
	    $name-putTextCmd	"%W delete 0 end; %W insert 0 %T" \
	    $name-getTextCmd	"%W get" \
	    $name-putListCmd	"" \
	    $name-getListCmd	"" \
	    $name-selectCmd	"" \
	    $name-invokeCmd	"" \
	    $name-fontOpt	-font \
	    $name-useFormat	1 \
	    $name-useReqWidth	0 \
	    $name-usePadX	0 \
	    $name-isEntryLike	1 \
	    $name-focusWin	%W \
	    $name-reservedKeys	{Left Right} \
	]

	set name ttk::combobox
	array set ::tablelist::editWin [list \
	    $name-creationCmd	"createTileCombobox %W" \
	    $name-putValueCmd	"%W set %T" \
	    $name-getValueCmd	"%W get" \
	    $name-putTextCmd	"%W set %T" \
	    $name-getTextCmd	"%W get" \
	    $name-putListCmd	"" \
	    $name-getListCmd	"" \
	    $name-selectCmd	"" \
	    $name-invokeCmd	"event generate %W <Down>" \
	    $name-fontOpt	-font \
	    $name-useFormat	1 \
	    $name-useReqWidth	0 \
	    $name-usePadX	1 \
	    $name-isEntryLike	1 \
	    $name-focusWin	%W \
	    $name-reservedKeys	{Left Right Up Down} \
	]

	set name ttk::checkbutton
	array set ::tablelist::editWin [list \
	    $name-creationCmd	"createTileCheckbutton %W" \
	    $name-putValueCmd	{set [%W cget -variable] %T} \
	    $name-getValueCmd	{set [%W cget -variable]} \
	    $name-putTextCmd	{set [%W cget -variable] %T} \
	    $name-getTextCmd	{set [%W cget -variable]} \
	    $name-putListCmd	"" \
	    $name-getListCmd	"" \
	    $name-selectCmd	"" \
	    $name-invokeCmd	{%W instate !pressed {%W invoke}} \
	    $name-fontOpt	"" \
	    $name-useFormat	0 \
	    $name-useReqWidth	1 \
	    $name-usePadX	0 \
	    $name-isEntryLike	0 \
	    $name-focusWin	%W \
	    $name-reservedKeys	{} \
	]
    }
    if {$::tk_version >= 8.4 && [llength [package versions tile]] > 0} {
	addTileWidgets 
    }
}

#
# Public procedures related to interactive cell editing
# =====================================================
#

#------------------------------------------------------------------------------
# tablelist::addBWidgetEntry
#
# Registers the Entry widget from the BWidget package for interactive cell
# editing.
#------------------------------------------------------------------------------
proc tablelist::addBWidgetEntry {{name Entry}} {
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	"Entry %W -width 0" \
	$name-putValueCmd	"%W delete 0 end; %W insert 0 %T" \
	$name-getValueCmd	"%W get" \
	$name-putTextCmd	"%W delete 0 end; %W insert 0 %T" \
	$name-getTextCmd	"%W get" \
	$name-putListCmd	"" \
	$name-getListCmd	"" \
	$name-selectCmd		"" \
	$name-invokeCmd		"" \
	$name-fontOpt		-font \
	$name-useFormat		1 \
	$name-useReqWidth	0 \
	$name-usePadX		0 \
	$name-isEntryLike	1 \
	$name-focusWin		%W \
	$name-reservedKeys	{Left Right} \
    ]

    return $name
}

#------------------------------------------------------------------------------
# tablelist::addBWidgetSpinBox
#
# Registers the SpinBox widget from the BWidget package for interactive cell
# editing.
#------------------------------------------------------------------------------
proc tablelist::addBWidgetSpinBox {{name SpinBox}} {
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	"SpinBox %W -editable 1 -width 0" \
	$name-putValueCmd	"%W configure -text %T" \
	$name-getValueCmd	"%W cget -text" \
	$name-putTextCmd	"%W configure -text %T" \
	$name-getTextCmd	"%W cget -text" \
	$name-putListCmd	"" \
	$name-getListCmd	"" \
	$name-selectCmd		"" \
	$name-invokeCmd		"" \
	$name-fontOpt		-font \
	$name-useFormat		1 \
	$name-useReqWidth	0 \
	$name-usePadX		1 \
	$name-isEntryLike	1 \
	$name-focusWin		%W.e \
	$name-reservedKeys	{Left Right Up Down Prior Next} \
    ]

    return $name
}

#------------------------------------------------------------------------------
# tablelist::addBWidgetComboBox
#
# Registers the ComboBox widget from the BWidget package for interactive cell
# editing.
#------------------------------------------------------------------------------
proc tablelist::addBWidgetComboBox {{name ComboBox}} {
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	"ComboBox %W -editable 1 -width 0" \
	$name-putValueCmd	"%W configure -text %T" \
	$name-getValueCmd	"%W cget -text" \
	$name-putTextCmd	"%W configure -text %T" \
	$name-getTextCmd	"%W cget -text" \
	$name-putListCmd	"" \
	$name-getListCmd	"" \
	$name-selectCmd		"" \
	$name-invokeCmd		"%W.a invoke" \
	$name-fontOpt		-font \
	$name-useFormat		1 \
	$name-useReqWidth	0 \
	$name-usePadX		1 \
	$name-isEntryLike	1 \
	$name-focusWin		%W.e \
	$name-reservedKeys	{Left Right Up Down} \
    ]

    return $name
}

#------------------------------------------------------------------------------
# tablelist::addIncrEntryfield
#
# Registers the entryfield widget from the Iwidgets package for interactive
# cell editing.
#------------------------------------------------------------------------------
proc tablelist::addIncrEntryfield {{name entryfield}} {
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	"iwidgets::entryfield %W -width 0" \
	$name-putValueCmd	"%W clear; %W insert 0 %T" \
	$name-getValueCmd	"%W get" \
	$name-putTextCmd	"%W clear; %W insert 0 %T" \
	$name-getTextCmd	"%W get" \
	$name-putListCmd	"" \
	$name-getListCmd	"" \
	$name-selectCmd		"" \
	$name-invokeCmd		"" \
	$name-fontOpt		-textfont \
	$name-useFormat		1 \
	$name-useReqWidth	0 \
	$name-usePadX		0 \
	$name-isEntryLike	1 \
	$name-focusWin		{[%W component entry]} \
	$name-reservedKeys	{Left Right} \
    ]

    return $name
}

#------------------------------------------------------------------------------
# tablelist::addIncrDateTimeWidget
#
# Registers the datefield, dateentry, timefield, or timeentry widget from the
# Iwidgets package, with or without the -clicks option for its get subcommand,
# for interactive cell editing.
#------------------------------------------------------------------------------
proc tablelist::addIncrDateTimeWidget {widgetType args} {
    if {![regexp {^(datefield|dateentry|timefield|timeentry)$} $widgetType]} {
	return -code error \
	       "bad widget type \"$widgetType\": must be\
		datefield, dateentry, timefield, or timeentry"
    }

    switch [llength $args] {
	0 {
	    set useClicks 0
	    set name $widgetType
	}

	1 {
	    set arg [lindex $args 0]
	    if {[string compare $arg "-seconds"] == 0} {
		set useClicks 1
		set name $widgetType
	    } else {
		set useClicks 0
		set name $arg
	    }
	}

	2 {
	    set arg0 [lindex $args 0]
	    if {[string compare $arg0 "-seconds"] != 0} {
		return -code error "bad option \"$arg0\": must be -seconds"
	    }

	    set useClicks 1
	    set name [lindex $args 1]
	}

	default {
	    mwutil::wrongNumArgs "addIncrDateTimeWidget\
				  datefield|dateentry|timefield|timeentry\
				  ?-seconds? ?name?"
	}
    }
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	"iwidgets::$widgetType %W" \
	$name-putValueCmd	"%W show %T" \
	$name-getValueCmd	"%W get" \
	$name-putTextCmd	"%W show %T" \
	$name-getTextCmd	"%W get" \
	$name-putListCmd	"" \
	$name-getListCmd	"" \
	$name-selectCmd		"" \
	$name-invokeCmd		"" \
	$name-fontOpt		-textfont \
	$name-useReqWidth	1 \
	$name-usePadX		[string match "*entry" $widgetType] \
	$name-useFormat		1 \
	$name-isEntryLike	1 \
	$name-reservedKeys	{Left Right Up Down} \
    ]
    if {$useClicks} {
	lappend ::tablelist::editWin($name-getValueCmd) -clicks
	set ::tablelist::editWin($name-useFormat) 0
    }
    if {[string match "date*" $widgetType]} {
	set ::tablelist::editWin($name-focusWin) {[%W component date]}
    } else {
	set ::tablelist::editWin($name-focusWin) {[%W component time]}
    }

    return $name
}

#------------------------------------------------------------------------------
# tablelist::addIncrSpinner
#
# Registers the spinner widget from the Iwidgets package for interactive cell
# editing.
#------------------------------------------------------------------------------
proc tablelist::addIncrSpinner {{name spinner}} {
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	"iwidgets::spinner %W -width 0" \
	$name-putValueCmd	"%W clear; %W insert 0 %T" \
	$name-getValueCmd	"%W get" \
	$name-putTextCmd	"%W clear; %W insert 0 %T" \
	$name-getTextCmd	"%W get" \
	$name-putListCmd	"" \
	$name-getListCmd	"" \
	$name-selectCmd		"" \
	$name-invokeCmd		"" \
	$name-fontOpt		-textfont \
	$name-useFormat		1 \
	$name-useReqWidth	0 \
	$name-usePadX		1 \
	$name-isEntryLike	1 \
	$name-focusWin		{[%W component entry]} \
	$name-reservedKeys	{Left Right} \
    ]

    return $name
}

#------------------------------------------------------------------------------
# tablelist::addIncrSpinint
#
# Registers the spinint widget from the Iwidgets package for interactive cell
# editing.
#------------------------------------------------------------------------------
proc tablelist::addIncrSpinint {{name spinint}} {
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	"iwidgets::spinint %W -width 0" \
	$name-putValueCmd	"%W clear; %W insert 0 %T" \
	$name-getValueCmd	"%W get" \
	$name-putTextCmd	"%W clear; %W insert 0 %T" \
	$name-getTextCmd	"%W get" \
	$name-putListCmd	"" \
	$name-getListCmd	"" \
	$name-selectCmd		"" \
	$name-invokeCmd		"" \
	$name-fontOpt		-textfont \
	$name-useFormat		1 \
	$name-useReqWidth	0 \
	$name-usePadX		1 \
	$name-isEntryLike	1 \
	$name-focusWin		{[%W component entry]} \
	$name-reservedKeys	{Left Right} \
    ]

    return $name
}

#------------------------------------------------------------------------------
# tablelist::addIncrCombobox
#
# Registers the combobox widget from the Iwidgets package for interactive cell
# editing.
#------------------------------------------------------------------------------
proc tablelist::addIncrCombobox {{name combobox}} {
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	"createIncrCombobox %W" \
	$name-putValueCmd	"%W clear entry; %W insert entry 0 %T" \
	$name-getValueCmd	"%W get" \
	$name-putTextCmd	"%W clear entry; %W insert entry 0 %T" \
	$name-getTextCmd	"%W get" \
	$name-putListCmd	{eval [list %W insert list end] %L} \
	$name-getListCmd	"%W component list get 0 end" \
	$name-selectCmd		"%W selection set %I" \
	$name-invokeCmd		"%W invoke" \
	$name-fontOpt		-textfont \
	$name-useFormat		1 \
	$name-useReqWidth	0 \
	$name-usePadX		1 \
	$name-isEntryLike	1 \
	$name-focusWin		{[%W component entry]} \
	$name-reservedKeys	{Left Right Up Down Control-p Control-n} \
    ]

    return $name
}

#------------------------------------------------------------------------------
# tablelist::addOakleyCombobox
#
# Registers Bryan Oakley's combobox widget for interactive cell editing.
#------------------------------------------------------------------------------
proc tablelist::addOakleyCombobox {{name combobox}} {
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	"createOakleyCombobox %W" \
	$name-putValueCmd	"%W delete 0 end; %W insert 0 %T" \
	$name-getValueCmd	"%W get" \
	$name-putTextCmd	"%W delete 0 end; %W insert 0 %T" \
	$name-getTextCmd	"%W get" \
	$name-putListCmd	{eval [list %W list insert end] %L} \
	$name-getListCmd	"%W list get 0 end" \
	$name-selectCmd		"%W select %I" \
	$name-invokeCmd		"%W open" \
	$name-fontOpt		-font \
	$name-useFormat		1 \
	$name-useReqWidth	0 \
	$name-usePadX		1 \
	$name-isEntryLike	1 \
	$name-focusWin		%W.entry \
	$name-reservedKeys	{Left Right Up Down Prior Next} \
    ]

    #
    # Patch the ::combobox::UpdateVisualAttributes procedure to make sure it
    # won't change the background and trough colors of the vertical scrollbar
    #
    catch {combobox::combobox}	;# enforces the evaluation of "combobox.tcl"
    if {[catch {rename ::combobox::UpdateVisualAttributes \
		::combobox::_UpdateVisualAttributes}] == 0} {
	proc ::combobox::UpdateVisualAttributes w {
	    set vsbBackground [$w.top.vsb cget -background]
	    set vsbTroughColor [$w.top.vsb cget -troughcolor]

	    ::combobox::_UpdateVisualAttributes $w

	    $w.top.vsb configure -background $vsbBackground
	    $w.top.vsb configure -troughcolor $vsbTroughColor
	}
    }

    return $name
}

#------------------------------------------------------------------------------
# tablelist::addDateMentry
#
# Registers the widget created by the mentry::dateMentry command from the
# Mentry package, with a given format and separator and with or without the
# "-gmt 1" option for the mentry::putClockVal and mentry::getClockVal commands,
# for interactive cell editing.
#------------------------------------------------------------------------------
proc tablelist::addDateMentry {fmt sep args} {
    #
    # Parse the fmt argument
    #
    if {![regexp {^([dmyY])([dmyY])([dmyY])$} $fmt dummy \
		 fields(0) fields(1) fields(2)]} {
	return -code error \
	       "bad format \"$fmt\": must be a string of length 3,\
		consisting of the letters d, m, and y or Y"
    }

    #
    # Check whether all the three date components are represented in fmt
    #
    for {set n 0} {$n < 3} {incr n} {
	set lfields($n) [string tolower $fields($n)]
    }
    if {[string compare $lfields(0) $lfields(1)] == 0 ||
	[string compare $lfields(0) $lfields(2)] == 0 ||
	[string compare $lfields(1) $lfields(2)] == 0} {
	return -code error \
	       "bad format \"$fmt\": must have unique components for the\
		day, month, and year"
    }

    #
    # Parse the remaining arguments (if any)
    #
    switch [llength $args] {
	0 {
	    set useGMT 0
	    set name dateMentry
	}

	1 {
	    set arg [lindex $args 0]
	    if {[string compare $arg "-gmt"] == 0} {
		set useGMT 1
		set name dateMentry
	    } else {
		set useGMT 0
		set name $arg
	    }
	}

	2 {
	    set arg0 [lindex $args 0]
	    if {[string compare $arg0 "-gmt"] != 0} {
		return -code error "bad option \"$arg0\": must be -gmt"
	    }

	    set useGMT 1
	    set name [lindex $args 1]
	}

	default {
	    mwutil::wrongNumArgs "addDateMentry format separator ?-gmt? ?name?"
	}
    }
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	[list mentry::dateMentry %W $fmt $sep] \
	$name-putValueCmd	"mentry::putClockVal %T %W -gmt $useGMT" \
	$name-getValueCmd	"mentry::getClockVal %W -gmt $useGMT" \
	$name-putTextCmd	"" \
	$name-getTextCmd	"%W getstring" \
	$name-putListCmd	{eval [list %W put 0] %L} \
	$name-getListCmd	"%W getlist" \
	$name-selectCmd		"" \
	$name-invokeCmd		"" \
	$name-fontOpt		-font \
	$name-useFormat		0 \
	$name-useReqWidth	1 \
	$name-usePadX		1 \
	$name-isEntryLike	1 \
	$name-focusWin		"" \
	$name-reservedKeys	{Left Right Up Down Prior Next} \
    ]

    return $name
}

#------------------------------------------------------------------------------
# tablelist::addTimeMentry
#
# Registers the widget created by the mentry::timeMentry command from the
# Mentry package, with a given format and separator and with or without the
# "-gmt 1" option for the mentry::putClockVal and mentry::getClockVal commands,
# for interactive cell editing.
#------------------------------------------------------------------------------
proc tablelist::addTimeMentry {fmt sep args} {
    #
    # Parse the fmt argument
    #
    if {![regexp {^(H|I)(M)(S?)$} $fmt dummy fields(0) fields(1) fields(2)]} {
	return -code error \
	       "bad format \"$fmt\": must be a string of length 2 or 3\
		starting with H or I, followed by M and optionally by S"
    }

    #
    # Parse the remaining arguments (if any)
    #
    switch [llength $args] {
	0 {
	    set useGMT 0
	    set name timeMentry
	}

	1 {
	    set arg [lindex $args 0]
	    if {[string compare $arg "-gmt"] == 0} {
		set useGMT 1
		set name timeMentry
	    } else {
		set useGMT 0
		set name $arg
	    }
	}

	2 {
	    set arg0 [lindex $args 0]
	    if {[string compare $arg0 "-gmt"] != 0} {
		return -code error "bad option \"$arg0\": must be -gmt"
	    }

	    set useGMT 1
	    set name [lindex $args 1]
	}

	default {
	    mwutil::wrongNumArgs "addTimeMentry format separator ?-gmt? ?name?"
	}
    }
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	[list mentry::timeMentry %W $fmt $sep] \
	$name-putValueCmd	"mentry::putClockVal %T %W -gmt $useGMT" \
	$name-getValueCmd	"mentry::getClockVal %W -gmt $useGMT" \
	$name-putTextCmd	"" \
	$name-getTextCmd	"%W getstring" \
	$name-putListCmd	{eval [list %W put 0] %L} \
	$name-getListCmd	"%W getlist" \
	$name-selectCmd		"" \
	$name-invokeCmd		"" \
	$name-fontOpt		-font \
	$name-useFormat		0 \
	$name-useReqWidth	1 \
	$name-usePadX		1 \
	$name-isEntryLike	1 \
	$name-focusWin		"" \
	$name-reservedKeys	{Left Right Up Down Prior Next} \
    ]

    return $name
}

#------------------------------------------------------------------------------
# tablelist::addFixedPointMentry
#
# Registers the widget created by the mentry::fixedPointMentry command from the
# Mentry package, with a given number of characters before and a given number
# of digits after the decimal point, with or without the -comma option, for
# interactive cell editing.
#------------------------------------------------------------------------------
proc tablelist::addFixedPointMentry {cnt1 cnt2 args} {
    #
    # Check the arguments cnt1 and cnt2
    #
    if {[catch {format %d $cnt1}] != 0 || $cnt1 <= 0} {
	return -code error "expected positive integer but got \"$cnt1\""
    }
    if {[catch {format %d $cnt2}] != 0 || $cnt2 <= 0} {
	return -code error "expected positive integer but got \"$cnt2\""
    }

    #
    # Parse the remaining arguments (if any)
    #
    switch [llength $args] {
	0 {
	    set useComma 0
	    set name fixedPointMentry_$cnt1.$cnt2
	}

	1 {
	    set arg [lindex $args 0]
	    if {[string compare $arg "-comma"] == 0} {
		set useComma 1
		set name fixedPointMentry_$cnt1,$cnt2
	    } else {
		set useComma 0
		set name $arg
	    }
	}

	2 {
	    set arg0 [lindex $args 0]
	    if {[string compare $arg0 "-comma"] != 0} {
		return -code error "bad option \"$arg0\": must be -comma"
	    }

	    set useComma 1
	    set name [lindex $args 1]
	}

	default {
	    mwutil::wrongNumArgs "addFixedPointMentry count1 count2\
				  ?-comma? ?name?"
	}
    }
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	[list mentry::fixedPointMentry %W $cnt1 $cnt2] \
	$name-putValueCmd	"mentry::putReal %T %W" \
	$name-getValueCmd	"mentry::getReal %W" \
	$name-putTextCmd	"" \
	$name-getTextCmd	"%W getstring" \
	$name-putListCmd	{eval [list %W put 0] %L} \
	$name-getListCmd	"%W getlist" \
	$name-selectCmd		"" \
	$name-invokeCmd		"" \
	$name-fontOpt		-font \
	$name-useFormat		0 \
	$name-useReqWidth	1 \
	$name-usePadX		1 \
	$name-isEntryLike	1 \
	$name-focusWin		"" \
	$name-reservedKeys	{Left Right} \
    ]
    if {$useComma} {
	lappend ::tablelist::editWin($name-creationCmd) -comma
    }

    return $name
}

#------------------------------------------------------------------------------
# tablelist::addIPAddrMentry
#
# Registers the widget created by the mentry::ipAddrMentry command from the
# Mentry package for interactive cell editing.
#------------------------------------------------------------------------------
proc tablelist::addIPAddrMentry {{name ipAddrMentry}} {
    checkEditWinName $name

    array set ::tablelist::editWin [list \
	$name-creationCmd	"mentry::ipAddrMentry %W" \
	$name-putValueCmd	"mentry::putIPAddr %T %W" \
	$name-getValueCmd	"mentry::getIPAddr %W" \
	$name-putTextCmd	"" \
	$name-getTextCmd	"%W getstring" \
	$name-putListCmd	{eval [list %W put 0] %L} \
	$name-getListCmd	"%W getlist" \
	$name-selectCmd		"" \
	$name-invokeCmd		"" \
	$name-fontOpt		-font \
	$name-useFormat		0 \
	$name-useReqWidth	1 \
	$name-usePadX		1 \
	$name-isEntryLike	1 \
	$name-focusWin		"" \
	$name-reservedKeys	{Left Right Up Down Prior Next} \
    ]

    return $name
}

#
# Private procedures implementing the interactive cell editing
# ============================================================
#

#------------------------------------------------------------------------------
# tablelist::checkEditWinName
#
# Generates an error if the given edit window name is one of "entry", "text",
# "spinbox", "checkbutton", "ttk::entry", "ttk::combobox", or
# "ttk::checkbutton".
#------------------------------------------------------------------------------
proc tablelist::checkEditWinName name {
    if {[regexp {^(entry|text|spinbox|checkbutton)$} $name]} {
	return -code error \
	       "edit window name \"$name\" is reserved for Tk $name widgets"
    }

    if {[regexp {^ttk::(entry|combobox|checkbutton)$} $name dummy name]} {
	return -code error \
	       "edit window name \"$name\" is reserved for tile $name widgets"
    }
}

#------------------------------------------------------------------------------
# tablelist::createCheckbutton
#
# Creates a checkbutton widget with the given path name for interactive cell
# editing in a tablelist widget.
#------------------------------------------------------------------------------
proc tablelist::createCheckbutton {w args} {
    variable winSys
    switch $winSys {
	x11 {
	    variable checkedImg
	    variable uncheckedImg
	    if {![info exists checkedImg]} {
		createCheckbuttonImgs 
	    }

	    checkbutton $w -borderwidth 2 -indicatoron 0 -image $uncheckedImg \
			   -selectimage $checkedImg -selectcolor ""
	    if {$::tk_version >= 8.4} {
		$w configure -offrelief sunken
	    }
	    pack $w
	}

	win32 {
	    checkbutton $w -borderwidth 0 -font {"MS Sans Serif" 8} \
			   -padx 0 -pady 0
	    [winfo parent $w] configure -width 13 -height 13
	    place $w -x -1 -y -1
	}

	classic {
	    checkbutton $w -borderwidth 0 -font "system" -padx 0 -pady 0
	    [winfo parent $w] configure -width 16 -height 14
	    place $w -x 0 -y -1
	}

	aqua {
	    checkbutton $w -borderwidth 0 -font "system" -padx 0 -pady 0
	    [winfo parent $w] configure -width 16 -height 17
	    place $w -x -3 -y -1
	}
    }

    foreach {opt val} $args {
	switch -- $opt {
	    -font  {}
	    -state { $w configure $opt $val }
	}
    }

    set win [getTablelistPath $w]
    $w configure -variable ::tablelist::ns${win}::data(editText)
}

#------------------------------------------------------------------------------
# tablelist::createTileEntry
#
# Creates a tile entry widget with the given path name for interactive cell
# editing in a tablelist widget.
#------------------------------------------------------------------------------
proc tablelist::createTileEntry {w args} {
    package require tile 0.6

    #
    # The style of the tile entry widget should have -borderwidth
    # 2 and -padding 1.  For those themes that don't honor the
    # -borderwidth 2 setting, set the padding to another value.
    #
    set win [getTablelistPath $w]
    switch $tile::currentTheme {
	aqua {
	    set padding {0 0 0 -1}
	}

	tileqt {
	    set padding 3
	}

	xpnative {
	    switch [winfo rgb . SystemButtonFace] {
		"60652 59881 55512" -
		"57568 57311 58339"	{ set padding 2 }
		default			{ set padding 1 }
	    }
	}

	default {
	    set padding 1
	}
    }
    styleConfig Tablelist.TEntry -borderwidth 2 -highlightthickness 0 \
				 -padding $padding

    ttk::entry $w -style Tablelist.TEntry

    foreach {opt val} $args {
	$w configure $opt $val
    }
}

#------------------------------------------------------------------------------
# tablelist::createTileCombobox
#
# Creates a tile combobox widget with the given path name for interactive cell
# editing in a tablelist widget.
#------------------------------------------------------------------------------
proc tablelist::createTileCombobox {w args} {
    package require tile 0.6

    set win [getTablelistPath $w]
    if {[string compare $tile::currentTheme "aqua"] == 0} {
	styleConfig Tablelist.TCombobox -borderwidth 2 -padding {0 0 0 -1}
    } else {
	styleConfig Tablelist.TCombobox -borderwidth 2 -padding 1
    }

    ttk::combobox $w -style Tablelist.TCombobox

    foreach {opt val} $args {
	$w configure $opt $val
    }
}

#------------------------------------------------------------------------------
# tablelist::createTileCheckbutton
#
# Creates a tile checkbutton widget with the given path name for interactive
# cell editing in a tablelist widget.
#------------------------------------------------------------------------------
proc tablelist::createTileCheckbutton {w args} {
    package require tile 0.6

    #
    # Define the checkbutton layout; use catch to suppress
    # the error message in case the layout already exists
    #
    if {[string compare $tile::currentTheme "aqua"] == 0} {
	catch { style layout Tablelist.TCheckbutton { Checkbutton.button } }
    } else {
	catch { style layout Tablelist.TCheckbutton { Checkbutton.indicator } }
	styleConfig Tablelist.TCheckbutton -indicatormargin 0
    }

    set win [getTablelistPath $w]
    ttk::checkbutton $w -style Tablelist.TCheckbutton \
			-variable ::tablelist::ns${win}::data(editText)

    foreach {opt val} $args {
	switch -- $opt {
	    -font  {}
	    -state { $w configure $opt $val }
	}
    }

    #
    # Adjust the dimensions of the tile checkbutton's parent
    # and manage the checkbutton, depending on the current theme
    #
    switch $tile::currentTheme {
	aqua {
	    [winfo parent $w] configure -width 16 -height 17
	    place $w -x -3 -y -2
	}

	Aquativo {
	    [winfo parent $w] configure -width 14 -height 14
	    place $w -x -1 -y -1
	}

	blue -
	winxpblue {
	    set height [winfo reqheight $w]
	    [winfo parent $w] configure -width $height -height $height
	    place $w -x 0
	}

	keramik {
	    [winfo parent $w] configure -width 16 -height 16
	    place $w -x -1 -y -1
	}

	sriv -
	srivlg {
	    [winfo parent $w] configure -width 15 -height 16
	    place $w -x -1
	}

	tileqt {
	    switch -- [string tolower [tile::theme::tileqt::currentThemeName]] {
		acqua {
		    [winfo parent $w] configure -width 17 -height 18
		    place $w -x -1 -y -2
		}
		kde_xp {
		    [winfo parent $w] configure -width 13 -height 13
		    place $w -x 0
		}
		keramik -
		thinkeramik {
		    [winfo parent $w] configure -width 16 -height 16
		    place $w -x 0
		}
		default {
		    set height [winfo reqheight $w]
		    [winfo parent $w] configure -width $height -height $height
		    place $w -x 0
		}
	    }
	}

	winnative -
	xpnative {
	    set height [winfo reqheight $w]
	    [winfo parent $w] configure -width $height -height $height
	    place $w -x -2
	}

	default {
	    pack $w
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::createIncrCombobox
#
# Creates an [incr Widgets] combobox with the given path name for interactive
# cell editing in a tablelist widget.
#------------------------------------------------------------------------------
proc tablelist::createIncrCombobox {w args} {
    eval [list iwidgets::combobox $w -dropdown 1 -editable 1 -width 0] $args

    #
    # Make sure that the entry component will receive the input focus
    # whenever the list component (a scrolledlistbox widget) gets unmapped
    #
    bind [$w component list] <Unmap> +[list focus [$w component entry]]
}

#------------------------------------------------------------------------------
# tablelist::createOakleyCombobox
#
# Creates an Oakley combobox widget with the given path name for interactive
# cell editing in a tablelist widget.
#------------------------------------------------------------------------------
proc tablelist::createOakleyCombobox {w args} {
    eval [list combobox::combobox $w -editable 1 -width 0] $args

    #
    # Repack the widget's components, to make sure that the
    # button will remain visible when shrinking the combobox.
    # This patch is needed for combobox versions earlier than 2.3.
    #
    pack forget $w.entry $w.button
    pack $w.button -side right -fill y    -expand 0
    pack $w.entry  -side left  -fill both -expand 1
}

#------------------------------------------------------------------------------
# tablelist::editcellSubCmd
#
# This procedure is invoked to process the tablelist editcell subcommand.  cmd
# may be an empty string, condChangeSelection, or changeSelection.  charPos
# stands for the character position component of the index in the body text
# widget of the character underneath the mouse cursor if this command was
# invoked by clicking mouse button 1 in the body of the tablelist widget.
#------------------------------------------------------------------------------
proc tablelist::editcellSubCmd {win row col restore {cmd ""} {charPos -1}} {
    variable editWin
    upvar ::tablelist::ns${win}::data data

    if {$data(isDisabled) || [doRowCget $row $win -hide] || $data($col-hide) ||
	![isCellEditable $win $row $col]} {
	return ""
    }
    if {$data(editRow) == $row && $data(editCol) == $col} {
	return ""
    }
    if {$data(editRow) >= 0 && ![finisheditingSubCmd $win]} {
	return ""
    }

    #
    # Create a frame to be embedded into the tablelist's body, together
    # with a child of column-specific type; replace the binding tag
    # Frame with TablelistEdit in the list of binding tags of the frame
    #
    seecellSubCmd $win $row $col
    set netRowHeight [lindex [bboxSubCmd $win $row] 3]
    set frameHeight [expr {$netRowHeight + 6}]	;# + 6 because of -pady -3 below
    set f $data(bodyFr)
    tk::frame $f -borderwidth 0 -container 0 -height $frameHeight \
		 -highlightthickness 0 -relief flat -takefocus 0
    bindtags $f [lreplace [bindtags $f] 1 1 TablelistEdit]
    bind $f <Destroy> {
	array set tablelist::ns[winfo parent [winfo parent %W]]::data \
		  {editRow -1  editCol -1}
	if {[catch {tk::CancelRepeat}] != 0} {
	    tkCancelRepeat 
	}
	catch {tile::CancelRepeat}
    }
    set name [getEditWindow $win $row $col]
    set creationCmd [strMap {"%W" "$w"} $editWin($name-creationCmd)]
    set item [lindex $data(itemList) $row]
    set key [lindex $item end]
    append creationCmd { $editWin($name-fontOpt) [getCellFont $win $key $col]} \
		       { -state normal}
    set w $data(bodyFrEd)
    if {[catch {eval $creationCmd} result] != 0} {
	destroy $f
	return -code error $result
    }
    set class [winfo class $w]
    set isCheckbtn [string match "*Checkbutton" $class]
    set isText [expr {[string compare $class "Text"] == 0}]
    set isMentry [expr {[string compare $class "Mentry"] == 0}]
    catch {$w configure -relief ridge}
    catch {$w configure -highlightthickness 0}
    if {!$isCheckbtn} {
	catch {$w configure -borderwidth 2}
    }
    set alignment [lindex $data(colList) [expr {2*$col + 1}]]
    if {!$isText && !$isMentry} {
	catch {$w configure -justify $alignment}
    }
    clearTakefocusOpt $w

    #
    # Replace the cell contents between the two tabs with the above frame
    #
    set b $data(body)
    set data(editKey) $key
    set data(editRow) $row
    set data(editCol) $col
    findTabs $win [expr {$row + 1}] $col $col tabIdx1 tabIdx2
    set editIdx [$b index $tabIdx1+1c]
    $b delete $editIdx $tabIdx2
    $b window create $editIdx -padx -3 -pady -3 -window $f
    $b mark set editMark $editIdx

    #
    # Insert the binding tag TablelistEdit in the list of binding tags
    # of some components of w, just before the respective path names
    #
    if {$isMentry} {
	set compList [$w entries]
    } else {
	set comp [subst [strMap {"%W" "$w"} $editWin($name-focusWin)]]
	set compList [list $comp]
	set data(editFocus) $comp
    }
    foreach comp $compList {
	set bindTags [bindtags $comp]
	set idx [lsearch -exact $bindTags $comp]
	bindtags $comp [linsert $bindTags $idx TablelistEdit]
    }

    #
    # Restore or initialize some of the edit window's data
    #
    if {$restore} {
	restoreEditData $win
    } else {
	#
	# Put the cell's contents to the edit window
	#
	set data(canceled) 0
	set data(invoked) 0
	set text [lindex $item $col]
	if {$editWin($name-useFormat) &&
	    [info exists data($col-formatcommand)]} {
	    set text [uplevel #0 $data($col-formatcommand) [list $text]]
	}
	catch {
	    eval [strMap {"%W" "$w"  "%T" "$text"} $editWin($name-putValueCmd)]
	}
	if {[string compare $data(-editstartcommand) ""] != 0} {
	    set text [uplevel #0 $data(-editstartcommand) \
		      [list $win $row $col $text]]
	    if {$data(canceled)} {
		return ""
	    }
	    catch {
		eval [strMap {"%W" "$w"  "%T" "$text"} \
		      $editWin($name-putValueCmd)]
	    }
	}

	#
	# Save the edit window's text
	#
	set data(origEditText) \
	    [eval [strMap {"%W" "$w"} $editWin($name-getTextCmd)]]
	set data(rejected) 0

	if {$isText} {
	    #
	    # Adjust the edit window's height
	    #
	    scan [$w index end-1c] "%d" numLines
	    $w configure -height $numLines
	    if {[info exists ::wcb::version]} {
		wcb::callback $w after insert tablelist::adjustTextHeight
		wcb::callback $w after delete tablelist::adjustTextHeight
	    }
	}

	if {[string compare $editWin($name-getListCmd) ""] != 0 &&
	    [string compare $editWin($name-selectCmd) ""] != 0} {
	    #
	    # Select the edit window's item corresponding to text
	    #
	    set itemList [eval [strMap {"%W" "$w"} $editWin($name-getListCmd)]]
	    if {[set idx [lsearch -exact $itemList $text]] >= 0} {
		eval [strMap {"%W" "$w"  "%I" "$idx"} $editWin($name-selectCmd)]
	    }
	}

	#
	# Evaluate the optional command passed as argument
	#
	if {[string compare $cmd ""] != 0} {
	    eval [list $cmd $win $row $col]
	}

	#
	# Set the focus and the insertion cursor
	#
	if {$charPos >= 0} {
	    if {$isText || !$editWin($name-isEntryLike)} {
		focus $w
	    } else {
		set hasAuxObject [expr {[info exists data($key,$col-image)] ||
					[info exists data($key,$col-window)]}]
		if {[string compare $alignment "right"] == 0} {
		    scan $tabIdx2 "%d.%d" line tabCharIdx2
		    if {$isMentry} {
			set len [string length [$w getstring]]
		    } else {
			set len [$comp index end]
		    }
		    set number [expr {$len - $tabCharIdx2 + $charPos}]
		    if {$hasAuxObject} {
			incr number 2
		    }
		} else {
		    scan $tabIdx1 "%d.%d" line tabCharIdx1
		    set number [expr {$charPos - $tabCharIdx1 - 1}]
		    if {$hasAuxObject} {
			incr number -2
		    }
		}
		if {$isMentry} {
		    setMentryCursor $w $number
		} else {
		    focus $comp
		    $comp icursor $number
		}
	    }
	} else {
	    if {$isText || $isMentry || !$editWin($name-isEntryLike)} {
		focus $w
	    } else {
		focus $comp
		$comp icursor end
		$comp selection range 0 end
	    }
	}
    }

    #
    # Adjust the frame's dimensions and paddings
    #
    update idletasks
    if {!$isCheckbtn} {
	$f configure -height [winfo reqheight $w]
	place $w -relwidth 1.0 -relheight 1.0
	set pixels [lindex $data(colList) [expr {2*$col}]]
	if {$pixels == 0} {			;# convention: dynamic width
	    set pixels $data($col-reqPixels)
	    if {$data($col-maxPixels) > 0} {
		if {$pixels > $data($col-maxPixels)} {
		    set pixels $data($col-maxPixels)
		}
	    }
	}
	incr pixels $data($col-delta)
	adjustEditWindow $win $pixels
	update idletasks
    }

    adjustElidedTextWhenIdle $win
    updateColorsWhenIdle $win
    adjustSepsWhenIdle $win
    return ""
}

#------------------------------------------------------------------------------
# tablelist::canceleditingSubCmd
#
# This procedure is invoked to process the tablelist cancelediting subcommand.
# Aborts the interactive cell editing and restores the cell's contents after
# destroying the edit window.
#------------------------------------------------------------------------------
proc tablelist::canceleditingSubCmd win {
    upvar ::tablelist::ns${win}::data data

    if {[set row $data(editRow)] < 0} {
	return ""
    }
    set col $data(editCol)

    #
    # Invoke the command specified by the -editendcommand option if needed
    #
    if {$data(-forceeditendcommand) &&
	[string compare $data(-editendcommand) ""] != 0} {
	uplevel #0 $data(-editendcommand) \
		[list $win $row $col $data(origEditText)]
    }

    if {[winfo exists $data(bodyFr)]} {
	destroy $data(bodyFr)
	set item [lindex $data(itemList) $row]
	set key [lindex $item end]
	if {[info exists data($key,$col-window)]} {
	    doCellConfig $row $col $win -window $data($key,$col-window)
	} elseif {[info exists data($key,$col-image)]} {
	    doCellConfig $row $col $win -image $data($key,$col-image)
	}
	doCellConfig $row $col $win -text [lindex $item $col]
    }

    focus $data(body)
    set data(canceled) 1
    event generate $win <<TablelistCellRestored>>

    adjustElidedTextWhenIdle $win
    updateColorsWhenIdle $win
    adjustSepsWhenIdle $win
    return ""
}

#------------------------------------------------------------------------------
# tablelist::finisheditingSubCmd
#
# This procedure is invoked to process the tablelist finishediting subcommand.
# Invokes the command specified by the -editendcommand option if needed, and
# updates the element just edited after destroying the edit window if the
# latter's content was not rejected.  Returns 1 on normal termination and 0
# otherwise.
#------------------------------------------------------------------------------
proc tablelist::finisheditingSubCmd win {
    variable editWin
    upvar ::tablelist::ns${win}::data data

    if {[set row $data(editRow)] < 0} {
	return 1
    }
    set col $data(editCol)

    #
    # Get the edit window's text, and invoke the command
    # specified by the -editendcommand option if needed
    #
    set w $data(bodyFrEd)
    set name [getEditWindow $win $row $col]
    set text [eval [strMap {"%W" "$w"} $editWin($name-getTextCmd)]]
    set item [lindex $data(itemList) $row]
    if {!$data(-forceeditendcommand) &&
	[string compare $text $data(origEditText)] == 0} {
	set text [lindex $item $col]
    } else {
	if {[catch {
	    eval [strMap {"%W" "$w"} $editWin($name-getValueCmd)]
	} text] != 0} {
	    set data(rejected) 1
	}
	if {[string compare $data(-editendcommand) ""] != 0} {
	    set text \
		[uplevel #0 $data(-editendcommand) [list $win $row $col $text]]
	}
    }

    #
    # Check whether the input was rejected (by the above "set data(rejected) 1"
    # statement or within the command specified by the -editendcommand option)
    #
    if {$data(rejected)} {
	if {[winfo exists $data(bodyFr)]} {
	    seecellSubCmd $win $row $col
	    if {[string compare [winfo class $w] "Mentry"] != 0} {
		focus $data(editFocus)
	    }
	} else {
	    focus $data(body)
	}

	set data(rejected) 0
	set result 0
    } else {
	if {[winfo exists $data(bodyFr)]} {
	    destroy $data(bodyFr)
	    set key [lindex $item end]
	    if {[info exists data($key,$col-window)]} {
		doCellConfig $row $col $win -window $data($key,$col-window)
	    } elseif {[info exists data($key,$col-image)]} {
		doCellConfig $row $col $win -image $data($key,$col-image)
	    }
	    doCellConfig $row $col $win -text $text
	    set result 1
	} else {
	    set result 0
	}

	focus $data(body)
	event generate $win <<TablelistCellUpdated>>
    }

    adjustElidedTextWhenIdle $win
    updateColorsWhenIdle $win
    adjustSepsWhenIdle $win
    return $result
}

#------------------------------------------------------------------------------
# tablelist::clearTakefocusOpt
#
# Sets the -takefocus option of all members of the widget hierarchy starting
# with w to 0.
#------------------------------------------------------------------------------
proc tablelist::clearTakefocusOpt w {
    catch {$w configure -takefocus 0}
    foreach c [winfo children $w] {
	clearTakefocusOpt $c
    }
}

#------------------------------------------------------------------------------
# tablelist::adjustTextHeight
#
# This procedure is an after-insert and after-delete callback asociated with a
# text widget used for interactive cell editing.  It sets the height of the
# edit window to the number of lines currently contained in it.
#------------------------------------------------------------------------------
proc tablelist::adjustTextHeight {w args} {
    scan [$w index end-1c] "%d" numLines
    $w configure -height $numLines

    set path [wcb::pathname $w]
    [winfo parent $path] configure -height [winfo reqheight $path]
}

#------------------------------------------------------------------------------
# tablelist::setMentryCursor
#
# Sets the focus to the entry child of the mentry widget w that contains the
# global character position specified by number, and sets the insertion cursor
# in that entry to the relative character position corresponding to number.  If
# that entry is not enabled then the procedure sets the focus to the last
# enabled entry child preceding the found one and sets the insertion cursor to
# its end.
#------------------------------------------------------------------------------
proc tablelist::setMentryCursor {w number} {
    #
    # Find the entry child containing the given character
    # position; if the latter is contained in a label child
    # then take the entry immediately preceding that label
    #
    set entryIdx -1
    set childIdx 0
    set childCount [llength [$w cget -body]]
    foreach c [winfo children $w] {
	set class [winfo class $c]
	switch $class {
	    Entry {
		set str [$c get]
		set entry $c
		incr entryIdx
	    }
	    Frame {
		set str [$c.e get]
		set entry $c.e
		incr entryIdx
	    }
	    Label { set str [$c cget -text] }
	}
	set len [string length $str]

	if {$number < $len} {
	    break
	} elseif {$childIdx < $childCount - 1} {
	    incr number -$len
	}

	incr childIdx
    }

    #
    # If the entry's state is normal then set the focus to this entry and
    # the insertion cursor to the relative character position corresponding
    # to number; otherwise set the focus to the last enabled entry child
    # preceding the found one and set the insertion cursor to its end
    #
    switch $class {
	Entry -
	Frame { set relIdx $number }
	Label { set relIdx end }
    }
    if {[string compare [$entry cget -state] "normal"] == 0} {
	focus $entry
	$entry icursor $relIdx
    } else {
	for {incr entryIdx -1} {$entryIdx >= 0} {incr entryIdx -1} {
	    set entry [$w entrypath $entryIdx]
	    if {[string compare [$entry cget -state] "normal"] == 0} {
		focus $entry
		$entry icursor end
		return ""
	    }
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::adjustEditWindow
#
# Adjusts the width and the horizontal padding of the frame containing the edit
# window associated with the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::adjustEditWindow {win pixels} {
    variable editWin
    upvar ::tablelist::ns${win}::data data

    set name [getEditWindow $win $data(editRow) $data(editCol)]
    if {$editWin($name-useReqWidth) &&
	[set reqWidth [winfo reqwidth $data(bodyFrEd)]] <=
	$pixels + 2*$data(charWidth)} {
	set width $reqWidth
	set padX [expr {$reqWidth <= $pixels ? -3 : ($pixels - $reqWidth) / 2}]
    } else {
	if {$editWin($name-usePadX)} {
	    set amount $data(charWidth)
	} else {
	    switch -- $name {
		text { set amount 4 }
		ttk::entry {
		    if {[string compare $tile::currentTheme "aqua"] == 0} {
			set amount 5
		    } else {
			set amount 3
		    }
		}
		default { set amount 3 }
	    }
	}
	set width [expr {$pixels + 2*$amount}]
	set padX -$amount
    }

    $data(bodyFr) configure -width $width
    $data(body) window configure editMark -padx $padX
}

#------------------------------------------------------------------------------
# tablelist::setEditWinFont
#
# Sets the font of the edit window associated with the tablelist widget win to
# that of the cell currently being edited.
#------------------------------------------------------------------------------
proc tablelist::setEditWinFont win {
    variable editWin
    upvar ::tablelist::ns${win}::data data

    set name [getEditWindow $win $data(editRow) $data(editCol)]
    if {[string compare $editWin($name-fontOpt) ""] == 0} {
	return ""
    }

    set key [lindex [lindex $data(itemList) $data(editRow)] end] 
    set cellFont [getCellFont $win $key $data(editCol)]
    $data(bodyFrEd) configure $editWin($name-fontOpt) $cellFont

    $data(bodyFr) configure -height [winfo reqheight $data(bodyFrEd)]
}

#------------------------------------------------------------------------------
# tablelist::saveEditData
#
# Saves some data of the edit window associated with the tablelist widget win.
#------------------------------------------------------------------------------
proc tablelist::saveEditData win {
    variable editWin
    upvar ::tablelist::ns${win}::data data

    set w $data(bodyFrEd)
    set entry $data(editFocus)
    set class [winfo class $w]
    set isText [expr {[string compare $class "Text"] == 0}]
    set isMentry [expr {[string compare $class "Mentry"] == 0}]

    #
    # Miscellaneous data
    #
    set name [getEditWindow $win $data(editRow) $data(editCol)]
    set data(editText) [eval [strMap {"%W" "$w"} $editWin($name-getTextCmd)]]
    if {[string compare $editWin($name-getListCmd) ""] != 0} {
	set data(editList) \
	    [eval [strMap {"%W" "$w"} $editWin($name-getListCmd)]]
    }
    if {$isText} {
	set data(editPos) [$w index insert]
	set data(textSelRanges) [$w tag ranges sel]
    } elseif {$editWin($name-isEntryLike)} {
	set data(editPos) [$entry index insert]
	if {[set data(entryHadSel) [$entry selection present]]} {
	    set data(entrySelFrom) [$entry index sel.first]
	    set data(entrySelTo)   [$entry index sel.last]
	}
    }
    set data(editHadFocus) \
	[expr {[string compare [focus -lastfor $entry] $entry] == 0}]

    #
    # Configuration options and widget callbacks
    #
    saveEditConfigOpts $w
    if {[info exists ::wcb::version] &&
	$editWin($name-isEntryLike) && !$isMentry} {
	foreach when {before after} {
	    foreach opt {insert delete motion} {
		set data(entryCb-$when-$opt) \
		    [::wcb::callback $entry $when $opt]
	    }
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::saveEditConfigOpts
#
# Saves the non-default values of the configuration options of the edit window
# w associated with a tablelist widget, as well as those of its descendants.
#------------------------------------------------------------------------------
proc tablelist::saveEditConfigOpts w {
    regexp {^(.+)\.body\.f\.(e.*)$} $w dummy win tail
    upvar ::tablelist::ns${win}::data data

    foreach configSet [$w configure] {
	if {[llength $configSet] != 2} {
	    set default [lindex $configSet 3]
	    set current [lindex $configSet 4]
	    if {[string compare $default $current] != 0} {
		set opt [lindex $configSet 0]
		set data($tail$opt) [lindex $configSet 4]
	    }
	}
    }

    foreach c [winfo children $w] {
	saveEditConfigOpts $c
    }
}

#------------------------------------------------------------------------------
# tablelist::restoreEditData
#
# Restores some data of the edit window associated with the tablelist widget
# win.
#------------------------------------------------------------------------------
proc tablelist::restoreEditData win {
    variable editWin
    upvar ::tablelist::ns${win}::data data

    set w $data(bodyFrEd)
    set entry $data(editFocus)
    set class [winfo class $w]
    set isText [expr {[string compare $class "Text"] == 0}]
    set isMentry [expr {[string compare $class "Mentry"] == 0}]
    set isIncrDateTimeWidget [regexp {^(Date.+|Time.+)$} $class]

    #
    # Miscellaneous data
    #
    set name [getEditWindow $win $data(editRow) $data(editCol)]
    if {[string compare $editWin($name-putTextCmd) ""] != 0} {
	eval [strMap {"%W" "$w"  "%T" "$data(editText)"} \
	      $editWin($name-putTextCmd)]
    }
    if {[string compare $editWin($name-putListCmd) ""] != 0 &&
	[string compare $data(editList) ""] != 0} {
	eval [strMap {"%W" "$w"  "%L" "$data(editList)"} \
	      $editWin($name-putListCmd)]
    }
    if {[string compare $editWin($name-selectCmd) ""] != 0 &&
	[set idx [lsearch -exact $data(editList) $data(editText)]] >= 0} {
	eval [strMap {"%W" "$w"  "%I" "$idx"} $editWin($name-selectCmd)]
    }
    if {$isText} {
	$w mark set insert $data(editPos)
	if {[llength $data(textSelRanges)] != 0} {
	    eval [list $w tag add sel] $data(textSelRanges)
	}
    } elseif {$editWin($name-isEntryLike)} {
	$entry icursor $data(editPos)
	if {$data(entryHadSel)} {
	    $entry selection range $data(entrySelFrom) $data(entrySelTo)
	}
    }
    if {$data(editHadFocus)} {
	focus $entry
    }

    #
    # Configuration options and widget callbacks
    #
    restoreEditConfigOpts $w
    if {[info exists ::wcb::version] &&
	$editWin($name-isEntryLike) && !$isMentry} {
	foreach when {before after} {
	    foreach opt {insert delete motion} {
		eval [list ::wcb::callback $entry $when $opt] \
		     $data(entryCb-$when-$opt)
	    }
	}
    }

    #
    # If the edit window is a datefield, dateentry, timefield, or timeentry
    # widget then restore its text here, because otherwise it would be
    # overridden when the above invocation of restoreEditConfigOpts sets
    # the widget's -format option.  Note that this is a special case; in
    # general we must restore the text BEFORE the configuration options.
    #
    if {$isIncrDateTimeWidget} {
	eval [strMap {"%W" "$w"  "%T" "$data(editText)"} \
	      $editWin($name-putTextCmd)]
    }
}

#------------------------------------------------------------------------------
# tablelist::restoreEditConfigOpts
#
# Restores the non-default values of the configuration options of the edit
# window w associated with a tablelist widget, as well as those of its
# descendants.
#------------------------------------------------------------------------------
proc tablelist::restoreEditConfigOpts w {
    regexp {^(.+)\.body\.f\.(e.*)$} $w dummy win tail
    upvar ::tablelist::ns${win}::data data

    set isMentry [expr {[string compare [winfo class $w] "Mentry"] == 0}]

    foreach name [array names data $tail-*] {
	set opt [string range $name [string last "-" $name] end]
	if {!$isMentry || [string compare $opt "-body"] != 0} {
	    $w configure $opt $data($name)
	}
	unset data($name)
    }

    foreach c [winfo children $w] {
	restoreEditConfigOpts $c
    }
}

#
# Private procedures used in bindings related to interactive cell editing
# =======================================================================
#

#------------------------------------------------------------------------------
# tablelist::insertChar
#
# Inserts the string str ("\t" or "\n") into the entry-like widget w at the
# point of the insertion cursor.
#------------------------------------------------------------------------------
proc tablelist::insertChar {w str} {
    set class [winfo class $w]
    if {[string compare $class "Text"] == 0} {
	if {[string compare $str "\n"] == 0} {
	    eval [strMap {"%W" "$w"} [bind Text <Return>]]
	} else {
	    eval [strMap {"%W" "$w"} [bind Text <Control-i>]]
	}
	return -code break ""
    } elseif {[regexp {^(T?Entry|TCombobox|Spinbox)$} $class]} {
	if {[string match "T*" $class]} {
	    tile::entry::Insert $w $str
	} elseif {[string compare [info procs "::tkEntryInsert"] ""] == 0} {
	    tk::EntryInsert $w $str
	} else {
	    tkEntryInsert $w $str
	}
	return -code break ""
    }
}

#------------------------------------------------------------------------------
# tablelist::cancelEditing
#
# Invokes the canceleditingSubCmd procedure.
#------------------------------------------------------------------------------
proc tablelist::cancelEditing w {
    if {[isComboTopMapped $w]} {
	return ""
    }

    set win [getTablelistPath $w]
    upvar ::tablelist::ns${win}::data data
    if {[info exists data(sourceRow)]} {	;# move operation in progress
	return ""
    }

    canceleditingSubCmd $win
    return -code break ""
}

#------------------------------------------------------------------------------
# tablelist::finishEditing
#
# Invokes the finisheditingSubCmd procedure.
#------------------------------------------------------------------------------
proc tablelist::finishEditing w {
    if {[isComboTopMapped $w]} {
	return ""
    }

    finisheditingSubCmd [getTablelistPath $w]
    return -code break ""
}

#------------------------------------------------------------------------------
# tablelist::goToNextPrevCell
#
# Moves the edit window into the next or previous editable cell different from
# the one indicated by the given row and column, if there is such a cell.
#------------------------------------------------------------------------------
proc tablelist::goToNextPrevCell {w amount args} {
    if {[isComboTopMapped $w]} {
	return ""
    }

    set win [getTablelistPath $w]
    upvar ::tablelist::ns${win}::data data

    if {[llength $args] == 0} {
	set row $data(editRow)
	set col $data(editCol)
	set cmd condChangeSelection
    } else {
	set row [lindex $args 0]
	set col [lindex $args 1]
	set cmd changeSelection
    }

    set oldRow $row
    set oldCol $col

    while 1 {
	incr col $amount
	if {$col < 0} {
	    incr row $amount
	    if {$row < 0} {
		set row $data(lastRow)
	    }
	    set col $data(lastCol)
	} elseif {$col > $data(lastCol)} {
	    incr row $amount
	    if {$row > $data(lastRow)} {
		set row 0
	    }
	    set col 0
	}

	if {$row == $oldRow && $col == $oldCol} {
	    return -code break ""
	} elseif {![doRowCget $row $win -hide] && !$data($col-hide) &&
		  [isCellEditable $win $row $col]} {
	    editcellSubCmd $win $row $col 0 $cmd
	    return -code break ""
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::goLeftRight
#
# Moves the edit window into the previous or next editable cell of the current
# row if the cell being edited is not the first/last editable one within that
# row.
#------------------------------------------------------------------------------
proc tablelist::goLeftRight {w amount} {
    if {[isComboTopMapped $w]} {
	return ""
    }

    set win [getTablelistPath $w]
    upvar ::tablelist::ns${win}::data data

    set row $data(editRow)
    set col $data(editCol)

    while 1 {
	incr col $amount
	if {$col < 0 || $col > $data(lastCol)} {
	    return -code break ""
	} elseif {!$data($col-hide) && [isCellEditable $win $row $col]} {
	    editcellSubCmd $win $row $col 0 condChangeSelection
	    return -code break ""
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::goUpDown
#
# Invokes the goToPrevNextLine procedure.
#------------------------------------------------------------------------------
proc tablelist::goUpDown {w amount} {
    if {[isComboTopMapped $w]} {
	return ""
    }

    set win [getTablelistPath $w]
    upvar ::tablelist::ns${win}::data data

    goToPrevNextLine $w $amount $data(editRow) $data(editCol) \
	condChangeSelection
    return -code break ""
}

#------------------------------------------------------------------------------
# tablelist::goToPrevNextLine
#
# Moves the edit window into the last or first editable cell that is located in
# the specified column and has a row index less/greater than the given one, if
# there is such a cell.
#------------------------------------------------------------------------------
proc tablelist::goToPrevNextLine {w amount row col cmd} {
    set win [getTablelistPath $w]
    upvar ::tablelist::ns${win}::data data

    while 1 {
	incr row $amount
	if {$row < 0 || $row > $data(lastRow)} {
	    return 0
	} elseif {![doRowCget $row $win -hide] &&
		  [isCellEditable $win $row $col]} {
	    editcellSubCmd $win $row $col 0 $cmd
	    return 1
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::goToPriorNextPage
#
# Moves the edit window up or down by one page within the current column if the
# cell being edited is not the first/last editable one within that column.
#------------------------------------------------------------------------------
proc tablelist::goToPriorNextPage {w amount} {
    if {[isComboTopMapped $w]} {
	return ""
    }

    set win [getTablelistPath $w]
    upvar ::tablelist::ns${win}::data data

    #
    # Check whether there is any non-hidden editable cell
    # above/below the current one, in the same column
    #
    set row $data(editRow)
    set col $data(editCol)
    while 1 {
	incr row $amount
	if {$row < 0 || $row > $data(lastRow)} {
	    return -code break ""
	} elseif {![doRowCget $row $win -hide] &&
		  [isCellEditable $win $row $col]} {
	    break
	}
    }

    #
    # Scroll up/down the view by one page and get the corresponding row index
    #
    set row $data(editRow)
    seeSubCmd $win $row
    set bbox [bboxSubCmd $win $row]
    yviewSubCmd $win [list scroll $amount pages]
    set newRow [rowIndex $win @0,[lindex $bbox 1] 0]

    if {$amount < 0} {
	if {$newRow < $row} {
	    if {![goToPrevNextLine $w -1 [expr {$newRow + 1}] $col \
		  changeSelection]} {
		goToPrevNextLine $w 1 $newRow $col changeSelection
	    }
	} else {
	    goToPrevNextLine $w 1 -1 $col changeSelection
	}
    } else {
	if {$newRow > $row} {
	    if {![goToPrevNextLine $w 1 [expr {$newRow - 1}] $col \
		  changeSelection]} {
		goToPrevNextLine $w -1 $newRow $col changeSelection
	    }
	} else {
	    goToPrevNextLine $w -1 $data(itemCount) $col changeSelection
	}
    }

    return -code break ""
}

#------------------------------------------------------------------------------
# tablelist::genMouseWheelEvent
#
# Generates a <MouseWheel> event with the given delta on the widget w.
#------------------------------------------------------------------------------
proc tablelist::genMouseWheelEvent {w delta} {
    set focus [focus -displayof $w]
    focus $w
    event generate $w <MouseWheel> -delta $delta
    focus $focus
}

#------------------------------------------------------------------------------
# tablelist::isKeyReserved
#
# Checks whether the given keysym is used in the standard binding scripts
# associated with the widget w, which is assumed to be the edit window or one
# of its descendants.
#------------------------------------------------------------------------------
proc tablelist::isKeyReserved {w keySym} {
    variable editWin
    set win [getTablelistPath $w]
    upvar ::tablelist::ns${win}::data data

    set name [getEditWindow $win $data(editRow) $data(editCol)]
    return [expr {[lsearch -exact $editWin($name-reservedKeys) $keySym] >= 0}]
}

#------------------------------------------------------------------------------
# tablelist::isComboTopMapped
#
# Checks whether the given widget is a component of an Oakley combobox having
# its toplevel child mapped.  This is needed in our binding scripts to make
# sure that the interactive cell editing won't be terminated prematurely,
# because Bryan Oakley's combobox keeps the focus on its entry child even if
# its toplevel component is mapped.
#------------------------------------------------------------------------------
proc tablelist::isComboTopMapped w {
    set par [winfo parent $w]
    if {[string compare [winfo class $par] "Combobox"] == 0 &&
	[winfo exists $par.top] && [winfo ismapped $par.top]} {
	return 1
    } else {
	return 0
    }
}
