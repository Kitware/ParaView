#==============================================================================
# Contains the implementation of the tablelist widget.
#
# Structure of the module:
#   - Namespace initialization
#   - Public procedure
#   - Private procedures implementing the tablelist widget command
#   - Private callback procedures
#
# Copyright (c) 2000-2006  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================

#
# Namespace initialization
# ========================
#

namespace eval tablelist {
    #
    # The array configSpecs is used to handle configuration options.  The
    # names of its elements are the configuration options for the Tablelist
    # class.  The value of an array element is either an alias name or a list
    # containing the database name and class as well as an indicator specifying
    # the widget(s) to which the option applies: c stands for all children
    # (text widgets and labels), b for the body text widget, l for the labels,
    # f for the frame, and w for the widget itself.
    #
    #	Command-Line Name	 {Database Name		  Database Class      W}
    #	------------------------------------------------------------------------
    #
    variable configSpecs
    array set configSpecs {
	-activestyle		 {activeStyle		  ActiveStyle	      w}
	-arrowcolor		 {arrowColor		  ArrowColor	      w}
	-arrowstyle		 {arrowStyle		  ArrowStyle	      w}
	-arrowdisabledcolor	 {arrowDisabledColor	  ArrowDisabledColor  w}
	-background		 {background		  Background	      b}
	-bg			 -background
	-borderwidth		 {borderWidth		  BorderWidth	      f}
	-bd			 -borderwidth
	-columns		 {columns		  Columns	      w}
	-cursor			 {cursor		  Cursor	      c}
	-disabledforeground	 {disabledForeground	  DisabledForeground  w}
	-editendcommand		 {editEndCommand	  EditEndCommand      w}
	-editstartcommand	 {editStartCommand	  EditStartCommand    w}
	-exportselection	 {exportSelection	  ExportSelection     w}
	-font			 {font			  Font		      b}
	-forceeditendcommand	 {forceEditEndCommand	  ForceEditEndCommand w}
	-foreground		 {foreground		  Foreground	      b}
	-fg			 -foreground
	-height			 {height		  Height	      w}
	-highlightbackground	 {highlightBackground	  HighlightBackground f}
	-highlightcolor		 {highlightColor	  HighlightColor      f}
	-highlightthickness	 {highlightThickness	  HighlightThickness  f}
	-incrarrowtype		 {incrArrowType		  IncrArrowType	      w}
	-labelactivebackground	 {labelActiveBackground	  Foreground          l}
	-labelactiveforeground	 {labelActiveForeground	  Background          l}
	-labelbackground	 {labelBackground	  Background	      l}
	-labelbg		 -labelbackground
	-labelborderwidth	 {labelBorderWidth	  BorderWidth	      l}
	-labelbd		 -labelborderwidth
	-labelcommand		 {labelCommand		  LabelCommand	      w}
	-labelcommand2		 {labelCommand2		  LabelCommand2	      w}
	-labeldisabledforeground {labelDisabledForeground DisabledForeground  l}
	-labelfont		 {labelFont		  Font		      l}
	-labelforeground	 {labelForeground	  Foreground	      l}
	-labelfg		 -labelforeground
	-labelheight		 {labelHeight		  Height	      l}
	-labelpady		 {labelPadY		  Pad		      l}
	-labelrelief		 {labelRelief		  Relief	      l}
	-listvariable		 {listVariable		  Variable	      w}
	-movablecolumns	 	 {movableColumns	  MovableColumns      w}
	-movablerows		 {movableRows		  MovableRows	      w}
	-movecolumncursor	 {moveColumnCursor	  MoveColumnCursor    w}
	-movecursor		 {moveCursor		  MoveCursor	      w}
	-protecttitlecolumns	 {protectTitleColumns	  ProtectTitleColumns w}
	-relief			 {relief		  Relief	      f}
	-resizablecolumns	 {resizableColumns	  ResizableColumns    w}
	-resizecursor		 {resizeCursor		  ResizeCursor	      w}
	-selectbackground	 {selectBackground	  Foreground	      w}
	-selectborderwidth	 {selectBorderWidth	  BorderWidth	      w}
	-selectforeground	 {selectForeground	  Background	      w}
	-selectmode		 {selectMode		  SelectMode	      w}
	-selecttype		 {selectType		  SelectType	      w}
	-setfocus		 {setFocus		  SetFocus	      w}
	-setgrid		 {setGrid		  SetGrid	      w}
	-showarrow		 {showArrow		  ShowArrow	      w}
	-showlabels		 {showLabels		  ShowLabels	      w}
	-showseparators		 {showSeparators	  ShowSeparators      w}
	-snipstring		 {snipString		  SnipString	      w}
	-sortcommand		 {sortCommand		  SortCommand	      w}
	-spacing		 {spacing		  Spacing	      w}
	-state			 {state			  State		      w}
	-stretch		 {stretch		  Stretch	      w}
	-stripebackground	 {stripeBackground	  Background	      w}
	-stripebg		 -stripebackground
	-stripeforeground	 {stripeForeground	  Foreground	      w}
	-stripefg		 -stripeforeground
	-stripeheight		 {stripeHeight		  StripeHeight	      w}
	-takefocus		 {takeFocus		  TakeFocus	      f}
	-targetcolor		 {targetColor		  TargetColor	      w}
	-titlecolumns		 {titleColumns	  	  TitleColumns	      w}
	-width			 {width			  Width		      w}
	-xscrollcommand		 {xScrollCommand	  ScrollCommand	      w}
	-yscrollcommand		 {yScrollCommand	  ScrollCommand	      w}
    }

    #
    # Get the current windowing system ("x11", "win32", "classic", or "aqua")
    #
    variable winSys
    if {[catch {tk windowingsystem} winSys] != 0} {
	switch $::tcl_platform(platform) {
	    unix	{ set winSys x11 }
	    windows	{ set winSys win32 }
	    macintosh	{ set winSys classic }
	}
    }

    #
    # Extend the elements of the array configSpecs
    #
    extendConfigSpecs 

    variable configOpts [lsort [array names configSpecs]]

    #
    # The array colConfigSpecs is used to handle column configuration options.
    # The names of its elements are the column configuration options for the
    # Tablelist widget class.  The value of an array element is either an alias
    # name or a list containing the database name and class.
    #
    #	Command-Line Name	{Database Name		Database Class	}
    #	-----------------------------------------------------------------
    #
    variable colConfigSpecs
    array set colConfigSpecs {
	-align			{align			Align		}
	-background		{background		Background	}
	-bg			-background
	-editable		{editable		Editable	}
	-editwindow		{editWindow		EditWindow	}
	-font			{font			Font		}
	-foreground		{foreground		Foreground	}
	-fg			-foreground
	-formatcommand		{formatCommand		FormatCommand	}
	-hide			{hide			Hide		}
	-labelalign		{labelAlign		Align		}
	-labelbackground	{labelBackground	Background	}
	-labelbg		-labelbackground
	-labelborderwidth	{labelBorderWidth	BorderWidth	}
	-labelbd		-labelborderwidth
	-labelcommand		{labelCommand		LabelCommand	}
	-labelcommand2		{labelCommand2		LabelCommand2	}
	-labelfont		{labelFont		Font		}
	-labelforeground	{labelForeground	Foreground	}
	-labelfg		-labelforeground
	-labelheight		{labelHeight		Height		}
	-labelimage		{labelImage		Image		}
	-labelpady		{labelPadY		Pad		}
	-labelrelief		{labelRelief		Relief		}
	-maxwidth		{maxWidth		MaxWidth	}
	-name			{name			Name		}
	-resizable		{resizable		Resizable	}
	-selectbackground	{selectBackground	Foreground	}
	-selectforeground	{selectForeground	Background	}
	-showarrow		{showArrow		ShowArrow	}
	-showlinenumbers	{showLineNumbers	ShowLineNumbers }
	-sortcommand		{sortCommand		SortCommand	}
	-sortmode		{sortMode		SortMode	}
	-stretchable		{stretchable		Stretchable	}
	-text			{text			Text		}
	-title			{title			Title		}
	-width			{width			Width		}
    }

    #
    # Extend some elements of the array colConfigSpecs
    #
    lappend colConfigSpecs(-align)		- left
    lappend colConfigSpecs(-editable)		- 0
    lappend colConfigSpecs(-editwindow)		- entry
    lappend colConfigSpecs(-hide)		- 0
    lappend colConfigSpecs(-maxwidth)		- 0
    lappend colConfigSpecs(-resizable)		- 1
    lappend colConfigSpecs(-showarrow)		- 1
    lappend colConfigSpecs(-showlinenumbers)	- 0
    lappend colConfigSpecs(-sortmode)		- ascii
    lappend colConfigSpecs(-stretchable)	- 0
    lappend colConfigSpecs(-width)		- 0

    if {$usingTile} {
	unset colConfigSpecs(-labelheight)
    }

    #
    # The array rowConfigSpecs is used to handle row configuration options.
    # The names of its elements are the row configuration options for the
    # Tablelist widget class.  The value of an array element is either an alias
    # name or a list containing the database name and class.
    #
    #	Command-Line Name	{Database Name		Database Class	}
    #	-----------------------------------------------------------------
    #
    variable rowConfigSpecs
    array set rowConfigSpecs {
	-background		{background		Background	}
	-bg			-background
	-font			{font			Font		}
	-foreground		{foreground		Foreground	}
	-fg			-foreground
	-hide			{hide			Hide		}
	-name			{name			Name		}
	-selectable		{selectable		Selectable	}
	-selectbackground	{selectBackground	Foreground	}
	-selectforeground	{selectForeground	Background	}
	-text			{text			Text		}
    }

    #
    # Check whether the -elide text widget tag option is available
    #
    variable canElide
    variable elide
    if {$::tk_version >= 8.3} {
	set canElide 1
	set elide -elide
    } else {
	set canElide 0
	set elide --
    }

    #
    # Extend some elements of the array rowConfigSpecs
    #
    if {$canElide} {
	lappend rowConfigSpecs(-hide)	- 0
    } else {
	unset rowConfigSpecs(-hide)
    }
    lappend rowConfigSpecs(-selectable)	- 1

    #
    # The array cellConfigSpecs is used to handle cell configuration options.
    # The names of its elements are the cell configuration options for the
    # Tablelist widget class.  The value of an array element is either an alias
    # name or a list containing the database name and class.
    #
    #	Command-Line Name	{Database Name		Database Class	}
    #	-----------------------------------------------------------------
    #
    variable cellConfigSpecs
    array set cellConfigSpecs {
	-background		{background		Background	}
	-bg			-background
	-editable		{editable		Editable	}
	-editwindow		{editWindow		EditWindow	}
	-font			{font			Font		}
	-foreground		{foreground		Foreground	}
	-fg			-foreground
	-image			{image			Image		}
	-selectbackground	{selectBackground	Foreground	}
	-selectforeground	{selectForeground	Background	}
	-text			{text			Text		}
	-window			{window			Window		}
	-windowdestroy		{windowDestroy		WindowDestroy	}
    }

    #
    # Extend some elements of the array cellConfigSpecs
    #
    lappend cellConfigSpecs(-editable)		- 0
    lappend cellConfigSpecs(-editwindow)	- entry

    #
    # Use a list to facilitate the handling of the command options 
    #
    variable cmdOpts [list \
	activate activatecell attrib bbox bodypath bodytag cancelediting \
	cellcget cellconfigure cellindex cellselection cget columncget \
	columnconfigure columncount columnindex configure containing \
	containingcell containingcolumn curcellselection curselection delete \
	deletecolumns editcell editwinpath entrypath fillcolumn finishediting \
	get getcells getcolumns getkeys imagelabelpath index insert \
	insertcolumnlist insertcolumns insertlist itemlistvar labelpath \
	labels move movecolumn nearest nearestcell nearestcolumn rejectinput \
	resetsortinfo rowcget rowconfigure scan see seecell seecolumn \
	selection separatorpath separators size sort sortbycolumn \
	sortbycolumnlist sortcolumn sortcolumnlist sortorder sortorderlist \
	togglecolumnhide togglerowhide windowpath xview yview]
    if {!$canElide} {
	set idx [lsearch -exact $cmdOpts togglerowhide]
	set cmdOpts [lreplace $cmdOpts $idx $idx]
    }

    #
    # Use lists to facilitate the handling of miscellaneous options
    #
    variable activeStyles	[list frame none underline]
    variable alignments		[list left right center]
    variable arrowStyles	[list flat7x4 flat7x5 flat7x7 flat8x5 flat9x5 \
				      sunken8x7 sunken10x9 sunken12x11]
    variable arrowTypes		[list up down]
    variable states		[list disabled normal]
    variable selectTypes	[list row cell]
    variable sortModes		[list ascii command dictionary integer real]
    variable sortOrders		[list increasing decreasing]
    variable _sortOrders	[list -increasing -decreasing]
    variable scanCmdOpts	[list mark dragto]
    variable selCmdOpts		[list anchor clear includes set]

    #
    # Define the procedure strToDispStr, which returns the string
    # obtained by replacing all \t characters in its argument with
    # \\t, as well as the procedure strMap, needed because the
    # "string map" command is not available in Tcl 8.0 and 8.1.0.
    #
    if {[catch {string map {} ""}] == 0} {
	proc strToDispStr str {
	    if {[string match "*\t*" $str]} {
		return [string map {"\t" "\\t"} $str]
	    } else {
		return $str
	    }
	}

	interp alias {} ::tablelist::strMap {} string map
    } else {
	proc strToDispStr str {
	    if {[string match "*\t*" $str]} {
		regsub -all "\t" $str "\\t" str
	    }

	    return $str
	}

	proc strMap {charMap str} {
	    foreach {key val} $charMap {
		#
		# We will only need this for noncritical key values
		#
		regsub -all $key $str $val str
	    }

	    return $str
	}
    }

    #
    # Define some Tablelist class bindings
    #
    bind Tablelist <KeyPress> continue
    bind Tablelist <FocusIn> {
	tablelist::addActiveTag %W

	if {[string compare [focus -lastfor %W] %W] == 0} {
	    if {[winfo exists [%W editwinpath]]} {
		focus [set tablelist::ns%W::data(editFocus)]
	    } else {
		focus [%W bodypath]
	    }
	}
    }
    bind Tablelist <FocusOut>		{ tablelist::removeActiveTag %W }
    bind Tablelist <<TablelistSelect>>	{ event generate %W <<ListboxSelect>> }
    bind Tablelist <Destroy>		{ tablelist::cleanup %W }
    if {$usingTile} {
	bind Tablelist <<ThemeChanged>>	{
	    after idle [list tablelist::updateConfigSpecs %W]
	}
    }

    #
    # Define some TablelistWindow class bindings
    #
    bind TablelistWindow <Destroy>	{ tablelist::cleanupWindow %W }

    #
    # Define the binding tags TablelistKeyNav and TablelistBody
    #
    mwutil::defineKeyNav Tablelist
    defineTablelistBody 

    #
    # Define the virtual events <<Button3>> and <<ShiftButton3>>
    #
    event add <<Button3>> <Button-3>
    event add <<ShiftButton3>> <Shift-Button-3>
    if {[string compare $winSys "classic"] == 0 ||
	[string compare $winSys "aqua"] == 0} {
	event add <<Button3>> <Control-Button-1>
	event add <<ShiftButton3>> <Shift-Control-Button-1>
    }

    #
    # Define some mouse bindings for the binding tag TablelistLabel
    #
    bind TablelistLabel <Enter>		{ tablelist::labelEnter    %W %x }
    bind TablelistLabel <Motion>	{ tablelist::labelEnter    %W %x }
    bind TablelistLabel <Leave>		{ tablelist::labelLeave    %W %X %x %y }
    bind TablelistLabel <Button-1>	{ tablelist::labelB1Down   %W %x 0 }
    bind TablelistLabel <Shift-Button-1>  { tablelist::labelB1Down %W %x 1 }
    bind TablelistLabel <B1-Motion>	{ tablelist::labelB1Motion %W %X %x %y }
    bind TablelistLabel <B1-Enter>	{ tablelist::labelB1Enter  %W }
    bind TablelistLabel <B1-Leave>	{ tablelist::labelB1Leave  %W %x %y }
    bind TablelistLabel <ButtonRelease-1> { tablelist::labelB1Up   %W %X}
    bind TablelistLabel <<Button3>>	  { tablelist::labelB3Down %W 0 }
    bind TablelistLabel <<ShiftButton3>>  { tablelist::labelB3Down %W 1 }

    #
    # Define the binding tags TablelistSubLabel and TablelistArrow
    #
    defineTablelistSubLabel 
    defineTablelistArrow 

    #
    # Pre-register some widgets for interactive cell editing
    #
    variable editWin
    array set editWin {
	entry-registered			1
	text-registered				1
	checkbutton-registered			1
    }
    if {$::tk_version >= 8.4} {
	array set editWin {
	    spinbox-registered			1
	}
	if {[llength [package versions tile]] > 0} {
	    array set editWin {
		ttk::entry-registered		1
		ttk::combobox-registered	1
		ttk::checkbutton-registered	1
	    }
	}
    }
}

#
# Public procedure
# ================
#

#------------------------------------------------------------------------------
# tablelist::tablelist
#
# Creates a new tablelist widget whose name is specified as the first command-
# line argument, and configures it according to the options and their values
# given on the command line.  Returns the name of the newly created widget.
#------------------------------------------------------------------------------
proc tablelist::tablelist args {
    variable usingTile
    variable configSpecs
    variable configOpts
    variable canElide

    if {[llength $args] == 0} {
	mwutil::wrongNumArgs "tablelist pathName ?options?"
    }

    #
    # Create a frame of the class Tablelist
    #
    set win [lindex $args 0]
    if {[catch {
	if {$usingTile} {
	    ttk::frame $win -style Frame$win.TFrame -class Tablelist \
			    -height 0 -width 0
	} else {
	    tk::frame $win -class Tablelist -container 0 -height 0 -width 0
	}
    } result] != 0} {
	return -code error $result
    }

    #
    # Create a namespace within the current one to hold the data of the widget
    #
    namespace eval ns$win {
	#
	# The folowing array holds various data for this widget
	#
	variable data
	array set data {
	    arrowWidth		 9
	    hasListVar		 0
	    isDisabled		 0
	    ownsFocus		 0
	    charWidth		 1
	    hdrPixels		 0
	    activeRow		 0
	    activeCol		 0
	    anchorRow		 0
	    anchorCol		 0
	    seqNum		-1
	    freeKeyList		 {}
	    itemList		 {}
	    itemCount		 0
	    lastRow		-1
	    colList		 {}
	    colCount		 0
	    lastCol		-1
	    tagRefCount		 0
	    imgCount		 0
	    winCount		 0
	    afterId		 {}
	    labelClicked	 0
	    arrowColList	 {}
	    sortColList		 {}
	    sortOrder		 {}
	    editRow		-1
	    editCol		-1
	    forceAdjust		 0
	    fmtCmdFlagList	 {}
	    scrlColOffset	 0
	    cellsToReconfig	 {}
	    hiddenRowCount	 0
	    nonHiddenRowList	 {-1}
	    hiddenColCount	 0
	}

	#
	# The following array is used to hold arbitrary
	# attributes and their values for this widget
	#
	variable attribVals
    }

    #
    # Initialize some further components of data
    #
    upvar ::tablelist::ns${win}::data data
    foreach opt $configOpts {
	set data($opt) [lindex $configSpecs($opt) 3]
    }
    if {$usingTile} {
	variable themeDefaults
	set data(currentTheme) $tile::currentTheme
	set data(themeDefaults) [array get themeDefaults]
	if {[string compare $tile::currentTheme "tileqt"] == 0} {
	    set data(widgetStyle) [tile::theme::tileqt::currentThemeName]
	    set data(colorScheme) [getKdeConfigVal "KDE" "colorScheme"]
	} else {
	    set data(widgetStyle) ""
	    set data(colorScheme) ""
	}
    }
    set data(-titlecolumns)	0		;# for Tk versions < 8.3
    set data(colFontList)	[list $data(-font)]
    set data(listVarTraceCmd)	[list tablelist::listVarTrace $win]
    set data(bodyTag)		body$win
    set data(body)		$win.body
    set data(bodyFr)		$data(body).f
    set data(bodyFrEd)		$data(bodyFr).e
    set data(rowGap)		$data(body).g
    set data(hdr)		$win.hdr
    set data(hdrTxt)		$data(hdr).t
    set data(hdrTxtFr)		$data(hdrTxt).f
    set data(hdrTxtFrCanv)	$data(hdrTxtFr).c
    set data(hdrTxtFrLbl)	$data(hdrTxtFr).l
    set data(hdrLbl)		$data(hdr).l
    set data(colGap)		$data(hdr).g
    set data(lb)		$win.lb
    set data(sep)		$win.sep

    #
    # Create a child hierarchy used to hold the column labels.  The
    # labels will be created as children of the frame data(hdrTxtFr),
    # which is embedded into the text widget data(hdrTxt) (in order
    # to make it scrollable), which in turn fills the frame data(hdr)
    # (whose width and height can be set arbitrarily in pixels).
    #
    set w $data(hdr)			;# header frame
    tk::frame $w -borderwidth 0 -container 0 -height 0 -highlightthickness 0 \
		 -relief flat -takefocus 0 -width 0
    bind $w <Configure> {
	set tablelist::W [winfo parent %W]
	tablelist::stretchColumnsWhenIdle $tablelist::W
	tablelist::updateScrlColOffsetWhenIdle $tablelist::W
	tablelist::updateHScrlbarWhenIdle $tablelist::W
    }
    pack $w -fill x
    set w $data(hdrTxt)			;# text widget within the header frame
    text $w -borderwidth 0 -highlightthickness 0 -insertwidth 0 \
	    -padx 0 -pady 0 -state normal -takefocus 0 -wrap none
    place $w -relheight 1.0 -relwidth 1.0
    bindtags $w [lreplace [bindtags $w] 1 1]
    tk::frame $data(hdrTxtFr) -borderwidth 0 -container 0 -height 0 \
			      -highlightthickness 0 -relief flat \
			      -takefocus 0 -width 0
    $w window create 1.0 -window $data(hdrTxtFr)
    set w $data(hdrLbl)			;# filler label within the header frame
    if {$usingTile} {
	ttk::label $data(hdrTxtFrLbl)0 -style TablelistHeader.TLabel
	ttk::label $w -style TablelistHeader.TLabel -image "" \
		      -padding {1 1 1 1} -takefocus 0 -text "" \
		      -textvariable "" -underline -1 -wraplength 0
    } else {
	tk::label $data(hdrTxtFrLbl)0 
	tk::label $w -bitmap "" -highlightthickness 0 -image "" \
		     -takefocus 0 -text "" -textvariable "" -underline -1 \
		     -wraplength 0
    }
    place $w -relheight 1.0 -relwidth 1.0

    #
    # Create the body text widget within the main frame
    #
    set w $data(body)
    text $w -borderwidth 0 -exportselection 0 -highlightthickness 0 \
	    -insertwidth 0 -padx 0 -pady 0 -state normal -takefocus 0 -wrap none
    bind $w <Configure> {
	set tablelist::W [winfo parent %W]
	tablelist::adjustElidedTextWhenIdle $tablelist::W
	tablelist::updateColorsWhenIdle $tablelist::W
	tablelist::adjustSepsWhenIdle $tablelist::W
	tablelist::updateVScrlbarWhenIdle $tablelist::W
    }
    pack $w -expand 1 -fill both

    #
    # Modify the list of binding tags of the body text widget
    #
    bindtags $w [list $w $data(bodyTag) TablelistBody [winfo toplevel $w] \
		 TablelistKeyNav all]

    #
    # Create the "stripe", "select", "active", "disabled", "hiddenRow",
    # "hiddenCol", and "elidedCol" tags in the body text widget.  Don't
    # use the built-in "sel" tag because on Windows the selection in a
    # text widget only becomes visible when the window gets the input
    # focus.  DO NOT CHANGE the order of creation of these tags!
    #
    $w tag configure stripe -background "" -foreground ""    ;# will be changed
    $w tag configure select -relief raised
    $w tag configure active -borderwidth ""		     ;# will be changed
    $w tag configure disabled -foreground ""		     ;# will be changed
    if {$canElide} {
	$w tag configure hiddenRow -elide 1
	$w tag configure hiddenCol -elide 1
	$w tag configure elidedCol -elide 1
    }

    #
    # Create two frames used to display a gap between two consecutive
    # rows/columns when moving a row/column interactively
    #
    tk::frame $data(rowGap) -borderwidth 1 -container 0 -highlightthickness 0 \
			    -relief sunken -takefocus 0 -height 4
    tk::frame $data(colGap) -borderwidth 1 -container 0 -highlightthickness 0 \
			    -relief sunken -takefocus 0 -width 4

    #
    # Create an unmanaged listbox child, used to handle the -setgrid option
    #
    listbox $data(lb)

    #
    # Create the bitmaps needed to display the sort ranks
    #
    createSortRankImgs $win

    #
    # Configure the widget according to the command-line
    # arguments and to the available database options
    #
    if {[catch {
	mwutil::configureWidget $win configSpecs tablelist::doConfig \
				tablelist::doCget [lrange $args 1 end] 1
    } result] != 0} {
	destroy $win
	return -code error $result
    }

    #
    # Move the original widget command into the current namespace
    # and build a new widget procedure in the global one
    #
    rename ::$win $win
    proc ::$win args [format {
	if {[catch {tablelist::tablelistWidgetCmd %s $args} result] == 0} {
	    return $result
	} else {
	    return -code error $result
	}
    } [list $win]]

    #
    # Register a callback to be invoked whenever the PRIMARY
    # selection is owned by the window win and someone
    # attempts to retrieve it as a UTF8_STRING or STRING
    #
    selection handle -type UTF8_STRING $win \
	[list ::tablelist::fetchSelection $win]
    selection handle -type STRING $win \
	[list ::tablelist::fetchSelection $win]

    #
    # Set a trace on the array elements data(activeRow),
    # data(avtiveCol), and data(-selecttype)
    #
    foreach name {activeRow activeCol -selecttype} {
	trace variable data($name) w [list tablelist::activeTrace $win]
    }

    return $win
}

#
# Private procedures implementing the tablelist widget command
# ============================================================
#

#------------------------------------------------------------------------------
# tablelist::tablelistWidgetCmd
#
# This procedure is invoked to process the Tcl command corresponding to a
# tablelist widget.
#------------------------------------------------------------------------------
proc tablelist::tablelistWidgetCmd {win argList} {
    variable cmdOpts
    upvar ::tablelist::ns${win}::data data

    set argCount [llength $argList]
    if {$argCount == 0} {
	mwutil::wrongNumArgs "$win option ?arg arg ...?"
    }

    set cmd [mwutil::fullOpt "option" [lindex $argList 0] $cmdOpts]
    switch $cmd {
	activate -
	bbox -
	see {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd index"
	    }

	    synchronize $win
	    set index [rowIndex $win [lindex $argList 1] 0]
	    return [${cmd}SubCmd $win $index]
	}

	activatecell {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd cellIndex"
	    }

	    synchronize $win
	    scan [cellIndex $win [lindex $argList 1] 0] "%d,%d" row col
	    return [activatecellSubCmd $win $row $col]
	}

	attrib {
	    return [mwutil::attribSubCmd $win [lrange $argList 1 end]]
	}

	bodypath {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    return $data(body)
	}

	bodytag {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    return $data(bodyTag)
	}

	cancelediting -
	curcellselection -
	curselection -
	finishediting {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    synchronize $win
	    return [${cmd}SubCmd $win]
	}

	cellcget {
	    if {$argCount != 3} {
		mwutil::wrongNumArgs "$win $cmd cellIndex option"
	    }

	    synchronize $win
	    scan [cellIndex $win [lindex $argList 1] 1] "%d,%d" row col
	    variable cellConfigSpecs
	    set opt [mwutil::fullConfigOpt [lindex $argList 2] cellConfigSpecs]
	    return [doCellCget $row $col $win $opt]
	}

	cellconfigure {
	    if {$argCount < 2} {
		mwutil::wrongNumArgs "$win $cmd cellIndex ?option? ?value?\
				      ?option value ...?"
	    }

	    synchronize $win
	    scan [cellIndex $win [lindex $argList 1] 1] "%d,%d" row col
	    variable cellConfigSpecs
	    set argList [lrange $argList 2 end]
	    return [mwutil::configureSubCmd $win cellConfigSpecs \
		    "tablelist::doCellConfig $row $col" \
		    "tablelist::doCellCget $row $col" $argList]
	}

	cellindex {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd cellIndex"
	    }

	    synchronize $win
	    return [cellIndex $win [lindex $argList 1] 0]
	}

	cellselection {
	    if {$argCount < 3 || $argCount > 4} {
		mwutil::wrongNumArgs \
			"$win $cmd option firstCellIndex lastCellIndex" \
			"$win $cmd option cellIndexList"
	    }

	    synchronize $win
	    variable selCmdOpts
	    set opt [mwutil::fullOpt "option" [lindex $argList 1] $selCmdOpts]
	    set first [lindex $argList 2]
	    switch $opt {
		anchor -
		includes {
		    if {$argCount != 3} {
			mwutil::wrongNumArgs "$win cellselection $opt cellIndex"
		    }
		    scan [cellIndex $win $first 0] "%d,%d" row col
		    return [cellselectionSubCmd $win $opt $row $col $row $col]
		}
		clear -
		set {
		    if {$argCount == 3} {
			foreach elem $first {
			    scan [cellIndex $win $elem 0] "%d,%d" row col
			    cellselectionSubCmd $win $opt $row $col $row $col
			}
			return ""
		    } else {
			scan [cellIndex $win $first 0] "%d,%d" firstRow firstCol
			scan [cellIndex $win [lindex $argList 3] 0] "%d,%d" \
			     lastRow lastCol
			return [cellselectionSubCmd $win $opt \
				$firstRow $firstCol $lastRow $lastCol]
		    }
		}
	    }
	}

	cget {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd option"
	    }

	    #
	    # Return the value of the specified configuration option
	    #
	    variable configSpecs
	    set opt [mwutil::fullConfigOpt [lindex $argList 1] configSpecs]
	    return $data($opt)
	}

	columncget {
	    if {$argCount != 3} {
		mwutil::wrongNumArgs "$win $cmd columnIndex option"
	    }

	    synchronize $win
	    set col [colIndex $win [lindex $argList 1] 1]
	    variable colConfigSpecs
	    set opt [mwutil::fullConfigOpt [lindex $argList 2] colConfigSpecs]
	    return [doColCget $col $win $opt]
	}

	columnconfigure {
	    if {$argCount < 2} {
		mwutil::wrongNumArgs "$win $cmd columnIndex ?option? ?value?\
				      ?option value ...?"
	    }

	    synchronize $win
	    set col [colIndex $win [lindex $argList 1] 1]
	    variable colConfigSpecs
	    set argList [lrange $argList 2 end]
	    return [mwutil::configureSubCmd $win colConfigSpecs \
		    "tablelist::doColConfig $col" \
		    "tablelist::doColCget $col" $argList]
	}

	columncount {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    return $data(colCount)
	}

	columnindex {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd columnIndex"
	    }

	    synchronize $win
	    return [colIndex $win [lindex $argList 1] 0]
	}

	configure {
	    variable configSpecs
	    return [mwutil::configureSubCmd $win configSpecs \
		    tablelist::doConfig tablelist::doCget \
		    [lrange $argList 1 end]]
	}

	containing {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd y"
	    }

	    set y [format "%d" [lindex $argList 1]]
	    synchronize $win
	    return [containingSubCmd $win $y]
	}

	containingcell {
	    if {$argCount != 3} {
		mwutil::wrongNumArgs "$win $cmd x y"
	    }

	    set x [format "%d" [lindex $argList 1]]
	    set y [format "%d" [lindex $argList 2]]
	    synchronize $win
	    return [containingSubCmd $win $y],[containingcolumnSubCmd $win $x]
	}

	containingcolumn {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd x"
	    }

	    set x [format "%d" [lindex $argList 1]]
	    synchronize $win
	    return [containingcolumnSubCmd $win $x]
	}

	delete -
	get -
	getkeys -
	togglerowhide {
	    if {$argCount < 2 || $argCount > 3} {
		mwutil::wrongNumArgs "$win $cmd firstIndex lastIndex" \
				     "$win $cmd indexList"
	    }

	    synchronize $win
	    set first [lindex $argList 1]
	    if {$argCount == 3} {
		set last [lindex $argList 2]
	    } else {
		set last $first
	    }
	    incr argCount -1
	    return [${cmd}SubCmd $win $first $last $argCount]
	}

	deletecolumns -
	getcolumns -
	togglecolumnhide {
	    if {$argCount < 2 || $argCount > 3} {
		mwutil::wrongNumArgs \
			"$win $cmd firstColumnIndex lastColumnIndex" \
			"$win $cmd columnIndexList"
	    }

	    synchronize $win
	    set first [lindex $argList 1]
	    if {$argCount == 3} {
		set last [lindex $argList 2]
	    } else {
		set last $first
	    }
	    incr argCount -1
	    return [${cmd}SubCmd $win $first $last $argCount]
	}

	editcell {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd cellIndex"
	    }

	    synchronize $win
	    scan [cellIndex $win [lindex $argList 1] 1] "%d,%d" row col
	    return [editcellSubCmd $win $row $col 0]
	}

	editwinpath {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    if {[winfo exists $data(bodyFrEd)]} {
		return $data(bodyFrEd)
	    } else {
		return ""
	    }
	}

	entrypath {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    if {[winfo exists $data(bodyFrEd)]} {
		set class [winfo class $data(bodyFrEd)]
		if {[regexp {^(Mentry|T?Checkbutton)$} $class]} {
		    return ""
		} else {
		    return $data(editFocus)
		}
	    } else {
		return ""
	    }
	}

	fillcolumn {
	    if {$argCount != 3} {
		mwutil::wrongNumArgs "$win $cmd columnIndex text"
	    }

	    synchronize $win
	    set col [colIndex $win [lindex $argList 1] 1]
	    return [fillcolumnSubCmd $win $col [lindex $argList 2]]
	}

	getcells {
	    if {$argCount < 2 || $argCount > 3} {
		mwutil::wrongNumArgs \
			"$win $cmd firstCellIndex lastCellIndex" \
			"$win $cmd cellIndexList"
	    }

	    synchronize $win
	    set first [lindex $argList 1]
	    if {$argCount == 3} {
		set last [lindex $argList 2]
	    } else {
		set last $first
	    }
	    incr argCount -1
	    return [${cmd}SubCmd $win $first $last $argCount]
	}

	imagelabelpath {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd cellIndex"
	    }

	    synchronize $win
	    scan [cellIndex $win [lindex $argList 1] 1] "%d,%d" row col
	    set key [lindex [lindex $data(itemList) $row] end]
	    set w $data(body).l$key,$col
	    if {[winfo exists $w]} {
		return $w
	    } else {
		return ""
	    }
	}

	index {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd index"
	    }

	    synchronize $win
	    return [rowIndex $win [lindex $argList 1] 1]
	}

	insert {
	    if {$argCount < 2} {
		mwutil::wrongNumArgs "$win $cmd index ?item item ...?"
	    }

	    synchronize $win
	    set index [rowIndex $win [lindex $argList 1] 1]
	    return [insertSubCmd $win $index [lrange $argList 2 end] \
		    $data(hasListVar)]
	}

	insertcolumnlist {
	    if {$argCount != 3} {
		mwutil::wrongNumArgs "$win $cmd columnIndex columnList"
	    }

	    synchronize $win
	    set arg1 [lindex $argList 1]
	    if {[string first $arg1 "end"] == 0 || $arg1 == $data(colCount)} {
		set col $data(colCount)
	    } else {
		set col [colIndex $win $arg1 1]
	    }
	    return [insertcolumnsSubCmd $win $col [lindex $argList 2]]
	}

	insertcolumns {
	    if {$argCount < 2} {
		mwutil::wrongNumArgs "$win $cmd columnIndex\
			?width title ?alignment? width title ?alignment? ...?"
	    }

	    synchronize $win
	    set arg1 [lindex $argList 1]
	    if {[string first $arg1 "end"] == 0 || $arg1 == $data(colCount)} {
		set col $data(colCount)
	    } else {
		set col [colIndex $win $arg1 1]
	    }
	    return [insertcolumnsSubCmd $win $col [lrange $argList 2 end]]
	}

	insertlist {
	    if {$argCount != 3} {
		mwutil::wrongNumArgs "$win $cmd index list"
	    }

	    synchronize $win
	    set index [rowIndex $win [lindex $argList 1] 1]
	    return [insertSubCmd $win $index [lindex $argList 2] \
		    $data(hasListVar)]
	}

	itemlistvar {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    return ::tablelist::ns${win}::data(itemList)
	}

	labelpath {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd columnIndex"
	    }

	    synchronize $win
	    set col [colIndex $win [lindex $argList 1] 1]
	    return $data(hdrTxtFrLbl)$col
	}

	labels {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    set labelList {}
	    for {set col 0} {$col < $data(colCount)} {incr col} {
		lappend labelList $data(hdrTxtFrLbl)$col
	    }
	    return $labelList
	}

	move {
	    if {$argCount != 3} {
		mwutil::wrongNumArgs "$win $cmd sourceIndex targetIndex"
	    }

	    synchronize $win
	    set source [rowIndex $win [lindex $argList 1] 0]
	    set target [rowIndex $win [lindex $argList 2] 1]
	    return [moveSubCmd $win $source $target]
	}

	movecolumn {
	    if {$argCount != 3} {
		mwutil::wrongNumArgs "$win $cmd sourceColumnIndex\
				      targetColumnIndex"
	    }

	    synchronize $win
	    set arg1 [lindex $argList 1]
	    set source [colIndex $win $arg1 1]
	    set arg2 [lindex $argList 2]
	    if {[string first $arg2 "end"] == 0 || $arg2 == $data(colCount)} {
		set target $data(colCount)
	    } else {
		set target [colIndex $win $arg2 1]
	    }
	    return [movecolumnSubCmd $win $source $target]
	}

	nearest {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd y"
	    }

	    set y [format "%d" [lindex $argList 1]]
	    synchronize $win
	    return [rowIndex $win @0,$y 0]
	}

	nearestcell {
	    if {$argCount != 3} {
		mwutil::wrongNumArgs "$win $cmd x y"
	    }

	    set x [format "%d" [lindex $argList 1]]
	    set y [format "%d" [lindex $argList 2]]
	    synchronize $win
	    return [cellIndex $win @$x,$y 0]
	}

	nearestcolumn {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd x"
	    }

	    set x [format "%d" [lindex $argList 1]]
	    synchronize $win
	    return [colIndex $win @$x,0 0]
	}

	rejectinput {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    set data(rejected) 1
	}

	resetsortinfo {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    foreach col $data(sortColList) {
		set data($col-sortRank) 0
		set data($col-sortOrder) ""
	    }
	    set whichWidths {}
	    foreach col $data(arrowColList) {
		lappend whichWidths l$col
	    }
	    set data(sortColList) {}
	    set data(arrowColList) {}
	    set data(sortOrder) {}

	    if {[llength $whichWidths] > 0} {
		synchronize $win
		adjustColumns $win $whichWidths 1
	    }
	    return ""
	}

	rowcget {
	    if {$argCount != 3} {
		mwutil::wrongNumArgs "$win $cmd index option"
	    }

	    #
	    # Check the row index
	    #
	    synchronize $win
	    set rowArg [lindex $argList 1]
	    set row [rowIndex $win $rowArg 0]
	    if {$row < 0 || $row > $data(lastRow)} {
		return -code error "row index \"$rowArg\" out of range"
	    }

	    variable rowConfigSpecs
	    set opt [mwutil::fullConfigOpt [lindex $argList 2] rowConfigSpecs]
	    return [doRowCget $row $win $opt]
	}

	rowconfigure {
	    if {$argCount < 2} {
		mwutil::wrongNumArgs "$win $cmd index ?option? ?value?\
				      ?option value ...?"
	    }

	    #
	    # Check the row index
	    #
	    synchronize $win
	    set rowArg [lindex $argList 1]
	    set row [rowIndex $win $rowArg 0]
	    if {$row < 0 || $row > $data(lastRow)} {
		return -code error "row index \"$rowArg\" out of range"
	    }

	    variable rowConfigSpecs
	    set argList [lrange $argList 2 end]
	    return [mwutil::configureSubCmd $win rowConfigSpecs \
		    "tablelist::doRowConfig $row" \
		    "tablelist::doRowCget $row" $argList]
	}

	scan {
	    if {$argCount != 4} {
		mwutil::wrongNumArgs "$win $cmd mark|dragto x y"
	    }

	    set x [format "%d" [lindex $argList 2]]
	    set y [format "%d" [lindex $argList 3]]
	    variable scanCmdOpts
	    set opt [mwutil::fullOpt "option" [lindex $argList 1] $scanCmdOpts]
	    synchronize $win
	    return [scanSubCmd $win $opt $x $y]
	}

	seecell {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd cellIndex"
	    }

	    synchronize $win
	    scan [cellIndex $win [lindex $argList 1] 0] "%d,%d" row col
	    if {[winfo viewable $win]} {
		return [seecellSubCmd $win $row $col]
	    } else {
		after idle [list tablelist::seecellSubCmd $win $row $col]
		return ""
	    }
	}

	seecolumn {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd columnIndex"
	    }

	    synchronize $win
	    set col [colIndex $win [lindex $argList 1] 0]
	    if {[winfo viewable $win]} {
		return [seecellSubCmd $win [rowIndex $win @0,0 0] $col]
	    } else {
		after idle [list tablelist::seecellSubCmd \
			    $win [rowIndex $win @0,0 0] $col]
		return ""
	    }
	}

	selection {
	    if {$argCount < 3 || $argCount > 4} {
		mwutil::wrongNumArgs "$win $cmd option firstIndex lastIndex" \
				     "$win $cmd option indexList"
	    }

	    synchronize $win
	    variable selCmdOpts
	    set opt [mwutil::fullOpt "option" [lindex $argList 1] $selCmdOpts]
	    set first [lindex $argList 2]
	    switch $opt {
		anchor -
		includes {
		    if {$argCount != 3} {
			mwutil::wrongNumArgs "$win selection $opt index"
		    }
		    set index [rowIndex $win $first 0]
		    return [selectionSubCmd $win $opt $index $index]
		}
		clear -
		set {
		    if {$argCount == 3} {
			foreach elem $first {
			    set index [rowIndex $win $elem 0]
			    selectionSubCmd $win $opt $index $index
			}
			return ""
		    } else {
			set first [rowIndex $win $first 0]
			set last [rowIndex $win [lindex $argList 3] 0]
			return [selectionSubCmd $win $opt $first $last]
		    }
		}
	    }
	}

	separatorpath {
	    if {$argCount < 1 || $argCount > 2} {
		mwutil::wrongNumArgs "$win $cmd ?columnIndex?"
	    }

	    if {$argCount == 1} {
		if {[winfo exists $data(sep)]} {
		    return $data(sep)
		} else {
		    return ""
		}
	    } else {
		synchronize $win
		set col [colIndex $win [lindex $argList 1] 1]
		if {$data(-showseparators)} {
		    return $data(sep)$col
		} else {
		    return ""
		}
	    }
	}

	separators {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    set sepList {}
	    foreach w [winfo children $win] {
		if {[regexp {^sep([0-9]+)?$} [winfo name $w]]} {
		    lappend sepList $w
		}
	    }
	    return [lsort -dictionary $sepList]
	}

	size {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    synchronize $win
	    return $data(itemCount)
	}

	sort {
	    if {$argCount < 1 || $argCount > 2} {
		mwutil::wrongNumArgs "$win $cmd  ?-increasing|-decreasing?"
	    }

	    if {$argCount == 1} {
		set order -increasing
	    } else {
		variable _sortOrders
		set order [mwutil::fullOpt "option" \
			   [lindex $argList 2] $_sortOrders]
	    }
	    synchronize $win
	    return [sortSubCmd $win -1 [string range $order 1 end]]
	}

	sortbycolumn {
	    if {$argCount < 2 || $argCount > 3} {
		mwutil::wrongNumArgs "$win $cmd columnIndex\
				      ?-increasing|-decreasing?"
	    }

	    synchronize $win
	    set col [colIndex $win [lindex $argList 1] 1]
	    if {$argCount == 2} {
		set order -increasing
	    } else {
		variable _sortOrders
		set order [mwutil::fullOpt "option" \
			   [lindex $argList 2] $_sortOrders]
	    }
	    return [sortSubCmd $win $col [string range $order 1 end]]
	}

	sortbycolumnlist {
	    if {$argCount < 2 || $argCount > 3} {
		mwutil::wrongNumArgs "$win $cmd columnIndexList ?sortOrderList?"
	    }

	    synchronize $win
	    set sortColList {}
	    foreach elem [lindex $argList 1] {
		set col [colIndex $win $elem 1]
		if {[lsearch -exact $sortColList $col] >= 0} {
		    return -code error "duplicate column index \"$elem\""
		}
		lappend sortColList $col
	    }
	    set sortOrderList {}
	    if {$argCount == 3} {
		variable sortOrders
		foreach elem [lindex $argList 2] {
		    lappend sortOrderList \
			    [mwutil::fullOpt "option" $elem $sortOrders]
		}
	    }
	    return [sortSubCmd $win $sortColList $sortOrderList]
	}

	sortcolumn {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    if {[llength $data(sortColList)] == 0} {
		return -1
	    } else {
		return [lindex $data(sortColList) 0]
	    }
	}

	sortcolumnlist {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    return $data(sortColList)
	}

	sortorder {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    if {[llength $data(sortColList)] == 0} {
		return $data(sortOrder)
	    } else {
		set col [lindex $data(sortColList) 0]
		return $data($col-sortOrder)
	    }
	}

	sortorderlist {
	    if {$argCount != 1} {
		mwutil::wrongNumArgs "$win $cmd"
	    }

	    set sortOrderList {}
	    foreach col $data(sortColList) {
		lappend sortOrderList $data($col-sortOrder)
	    }
	    return $sortOrderList
	}

	windowpath {
	    if {$argCount != 2} {
		mwutil::wrongNumArgs "$win $cmd cellIndex"
	    }

	    synchronize $win
	    scan [cellIndex $win [lindex $argList 1] 1] "%d,%d" row col
	    set key [lindex [lindex $data(itemList) $row] end]
	    set w $data(body).f$key,$col.w
	    if {[winfo exists $w]} {
		return $w
	    } else {
		return ""
	    }
	}

	xview -
	yview {
	    synchronize $win
	    return [${cmd}SubCmd $win [lrange $argList 1 end]]
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::activateSubCmd
#
# This procedure is invoked to process the tablelist activate subcommand.
#------------------------------------------------------------------------------
proc tablelist::activateSubCmd {win index} {
    upvar ::tablelist::ns${win}::data data

    if {$data(isDisabled)} {
	return ""
    }

    #
    # Adjust the index to fit within the existing non-hidden items
    #
    adjustRowIndex $win index 1

    set data(activeRow) $index
    return ""
}

#------------------------------------------------------------------------------
# tablelist::activatecellSubCmd
#
# This procedure is invoked to process the tablelist activatecell subcommand.
#------------------------------------------------------------------------------
proc tablelist::activatecellSubCmd {win row col} {
    upvar ::tablelist::ns${win}::data data

    if {$data(isDisabled)} {
	return ""
    }

    #
    # Adjust the row and column indices to fit
    # within the existing non-hidden elements
    #
    adjustRowIndex $win row 1
    adjustColIndex $win col 1

    set data(activeRow) $row
    set data(activeCol) $col
    return ""
}

#------------------------------------------------------------------------------
# tablelist::bboxSubCmd
#
# This procedure is invoked to process the tablelist bbox subcommand.
#------------------------------------------------------------------------------
proc tablelist::bboxSubCmd {win index} {
    upvar ::tablelist::ns${win}::data data

    set w $data(body)
    set dlineinfo [$w dlineinfo [expr {double($index + 1)}]]
    if {$data(itemCount) == 0 || [string compare $dlineinfo ""] == 0} {
	return {}
    }

    set spacing1 [$w cget -spacing1]
    set spacing3 [$w cget -spacing3]
    foreach {x y width height baselinePos} $dlineinfo {}
    lappend bbox [expr {$x + [winfo x $w]}] \
		 [expr {$y + [winfo y $w] + $spacing1}] \
		 $width [expr {$height - $spacing1 - $spacing3}]
    return $bbox
}

#------------------------------------------------------------------------------
# tablelist::cellselectionSubCmd
#
# This procedure is invoked to process the tablelist cellselection subcommand.
#------------------------------------------------------------------------------
proc tablelist::cellselectionSubCmd {win opt firstRow firstCol \
				     lastRow lastCol} {
    upvar ::tablelist::ns${win}::data data

    if {$data(isDisabled) && [string compare $opt "includes"] != 0} {
	return ""
    }

    switch $opt {
	anchor {
	    #
	    # Adjust the row and column indices to fit
	    # within the existing non-hidden elements
	    #
	    adjustRowIndex $win firstRow 1
	    adjustColIndex $win firstCol 1

	    set data(anchorRow) $firstRow
	    set data(anchorCol) $firstCol
	    return ""
	}

	clear {
	    #
	    # Adjust the row and column indices
	    # to fit within the existing elements
	    #
	    if {$data(itemCount) == 0 || $data(colCount) == 0} {
		return ""
	    }
	    adjustRowIndex $win firstRow
	    adjustColIndex $win firstCol
	    adjustRowIndex $win lastRow
	    adjustColIndex $win lastCol

	    #
	    # Swap the indices if necessary
	    #
	    if {$lastRow < $firstRow} {
		set tmp $firstRow
		set firstRow $lastRow
		set lastRow $tmp
	    }
	    if {$lastCol < $firstCol} {
		set tmp $firstCol
		set firstCol $lastCol
		set lastCol $tmp
	    }

	    #
	    # Shrink the column range to be delimited by non-hidden columns
	    #
	    while {$firstCol <= $lastCol && $data($firstCol-hide)} {
		incr firstCol
	    }
	    if {$firstCol > $lastCol} {
		return ""
	    }
	    while {$lastCol >= $firstCol && $data($lastCol-hide)} {
		incr lastCol -1
	    }

	    set firstTextIdx [expr {$firstRow + 1}].0
	    set lastTextIdx [expr {$lastRow + 1}].end

	    #
	    # Find the (partly) selected lines of the body text
	    # widget in the text range specified by the two indices
	    #
	    set w $data(body)
	    variable canElide
	    variable elide
	    set selRange [$w tag nextrange select $firstTextIdx $lastTextIdx]
	    while {[llength $selRange] != 0} {
		set selStart [lindex $selRange 0]
		set line [expr {int($selStart)}]
		set row [expr {$line - 1}]
		set key [lindex [lindex $data(itemList) $row] end]

		#
		# Deselect the relevant elements of the row and handle
		# the -(select)background and -(select)foreground
		# cell and column configuration options for them
		#
		findTabs $win $line $firstCol $lastCol firstTabIdx lastTabIdx
		set textIdx1 $firstTabIdx
		for {set col $firstCol} {$col <= $lastCol} {incr col} {
		    if {$data($col-hide) && !$canElide} {
			continue
		    }

		    set textIdx2 \
			[$w search $elide "\t" $textIdx1+1c $lastTabIdx+1c]+1c
		    $w tag remove select $textIdx1 $textIdx2
		    foreach optTail {background foreground} {
			set opt -select$optTail
			foreach name  [list $col$opt $key$opt $key,$col$opt] \
				level [list col row cell] {
			    if {[info exists data($name)]} {
				set tag $level$opt-$data($name)
				$w tag remove $tag $textIdx1 $textIdx2
			    }
			}
			foreach name  [list $col-$optTail $key-$optTail \
				       $key,$col-$optTail] \
				level [list col row cell] {
			    if {[info exists data($name)]} {
				set tag $level-$optTail-$data($name)
				$w tag add $tag $textIdx1 $textIdx2
			    }
			}
		    }
		    set textIdx1 $textIdx2
		}

		set selRange \
		    [$w tag nextrange select "$selStart lineend" $lastTextIdx]
	    }

	    updateColorsWhenIdle $win
	    return ""
	}

	includes {
	    variable canElide
	    if {$firstRow < 0 || $firstRow > $data(lastRow) || \
		$firstCol < 0 || $firstCol > $data(lastCol) ||
		($data($firstCol-hide) && !$canElide)} {
		return 0
	    }

	    findTabs $win [expr {$firstRow + 1}] $firstCol $firstCol \
		     tabIdx1 tabIdx2
	    if {[lsearch -exact [$data(body) tag names $tabIdx1] select] < 0} {
		return 0
	    } else {
		return 1
	    }
	}

	set {
	    #
	    # Adjust the row and column indices
	    # to fit within the existing elements
	    #
	    if {$data(itemCount) == 0 || $data(colCount) == 0} {
		return ""
	    }
	    adjustRowIndex $win firstRow
	    adjustColIndex $win firstCol
	    adjustRowIndex $win lastRow
	    adjustColIndex $win lastCol

	    #
	    # Swap the indices if necessary
	    #
	    if {$lastRow < $firstRow} {
		set tmp $firstRow
		set firstRow $lastRow
		set lastRow $tmp
	    }
	    if {$lastCol < $firstCol} {
		set tmp $firstCol
		set firstCol $lastCol
		set lastCol $tmp
	    }

	    #
	    # Shrink the column range to be delimited by non-hidden columns
	    #
	    while {$firstCol <= $lastCol && $data($firstCol-hide)} {
		incr firstCol
	    }
	    if {$firstCol > $lastCol} {
		return ""
	    }
	    while {$lastCol >= $firstCol && $data($lastCol-hide)} {
		incr lastCol -1
	    }

	    set w $data(body)
	    variable canElide
	    variable elide
	    for {set row $firstRow; set line [expr {$firstRow + 1}]} \
		{$row <= $lastRow} {set row $line; incr line} {
		#
		# Check whether the row is selectable and non-hidden
		#
		set key [lindex [lindex $data(itemList) $row] end]
		if {[info exists data($key-selectable)] ||
		    [info exists data($key-hide)]} {
		    continue
		}

		#
		# Select the relevant non-hidden elements of the row and
		# handle the -(select)background and -(select)foreground
		# cell and column configuration options for them
		#
		findTabs $win $line $firstCol $lastCol firstTabIdx lastTabIdx
		set textIdx1 $firstTabIdx
		for {set col $firstCol} {$col <= $lastCol} {incr col} {
		    if {$data($col-hide) && !$canElide} {
			continue
		    }

		    set textIdx2 \
			[$w search $elide "\t" $textIdx1+1c $lastTabIdx+1c]+1c
		    if {$data($col-hide)} {
			set textIdx1 $textIdx2
			continue
		    }

		    $w tag add select $textIdx1 $textIdx2
		    foreach optTail {background foreground} {
			set opt -select$optTail
			foreach name  [list $col$opt $key$opt $key,$col$opt] \
				level [list col row cell] {
			    if {[info exists data($name)]} {
				set tag $level$opt-$data($name)
				$w tag add $tag $textIdx1 $textIdx2
			    }
			}
			foreach name  [list $col-$optTail $key-$optTail \
				       $key,$col-$optTail] \
				level [list col row cell] {
			    if {[info exists data($name)]} {
				set tag $level-$optTail-$data($name)
				$w tag remove $tag $textIdx1 $textIdx2
			    }
			}
		    }
		    set textIdx1 $textIdx2
		}
	    }

	    #
	    # If the selection is exported and there are any selected
	    # cells in the widget then make win the new owner of the
	    # PRIMARY selection and register a callback to be invoked
	    # when it loses ownership of the PRIMARY selection
	    #
	    if {$data(-exportselection) &&
		[llength [$w tag nextrange select 1.0]] != 0} {
		selection own -command \
			[list ::tablelist::lostSelection $win] $win
	    }

	    updateColorsWhenIdle $win
	    return ""
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::containingSubCmd
#
# This procedure is invoked to process the tablelist containing subcommand.
#------------------------------------------------------------------------------
proc tablelist::containingSubCmd {win y} {
    upvar ::tablelist::ns${win}::data data

    if {$data(itemCount) == 0} {
	return -1
    }

    set row [rowIndex $win @0,$y 0]

    set w $data(body)
    incr y -[winfo y $w]
    set dlineinfo [$w dlineinfo [expr {double($row + 1)}]]
    if {$y < [lindex $dlineinfo 1] + [lindex $dlineinfo 3]} {
	return $row
    } else {
	return -1
    }
}

#------------------------------------------------------------------------------
# tablelist::containingcolumnSubCmd
#
# This procedure is invoked to process the tablelist containingcolumn
# subcommand.
#------------------------------------------------------------------------------
proc tablelist::containingcolumnSubCmd {win x} {
    upvar ::tablelist::ns${win}::data data

    set col [colIndex $win @$x,0 0]
    if {$col < 0} {
	return -1
    }

    set lbl $data(hdrTxtFrLbl)$col
    if {$x + [winfo rootx $win] < [winfo width $lbl] + [winfo rootx $lbl]} {
	return $col
    } else {
	return -1
    }
}

#------------------------------------------------------------------------------
# tablelist::curcellselectionSubCmd
#
# This procedure is invoked to process the tablelist curcellselection
# subcommand.
#------------------------------------------------------------------------------
proc tablelist::curcellselectionSubCmd {win {getKeys 0}} {
    variable canElide
    variable elide
    upvar ::tablelist::ns${win}::data data

    #
    # Find the (partly) selected lines of the body text widget
    #
    set result {}
    set w $data(body)
    set selRange [$w tag nextrange select 1.0]
    while {[llength $selRange] != 0} {
	set selStart [lindex $selRange 0]
	set selEnd [lindex $selRange 1]
	set line [expr {int($selStart)}]
	set row [expr {$line - 1}]

	#
	# Get the index of the column starting at the text position selStart
	#
	set textIdx $line.0
	for {set col 0} {$col < $data(colCount)} {incr col} {
	    if {!$data($col-hide) || $canElide} {
		if {[$w compare $textIdx == $selStart]} {
		    set firstCol $col
		    break
		} else {
		    set textIdx [$w search $elide "\t" $textIdx+1c $selEnd]+1c
		}
	    }
	}

	#
	# Process the columns, starting at the found one
	# and ending just before the text position selEnd
	#
	if {$getKeys} {
	    set key [lindex [lindex $data(itemList) $row] end]
	}
	set textIdx [$w search $elide "\t" $textIdx+1c $selEnd]+1c
	for {set col $firstCol} {$col < $data(colCount)} {incr col} {
	    if {!$data($col-hide) || $canElide} {
		if {$getKeys} {
		    lappend result $key $col
		} else {
		    lappend result $row,$col
		}
		if {[$w compare $textIdx == $selEnd]} {
		    break
		} else {
		    set textIdx [$w search $elide "\t" $textIdx+1c $selEnd]+1c
		}
	    }
	}

	set selRange [$w tag nextrange select $selEnd]
    }
    return $result
}

#------------------------------------------------------------------------------
# tablelist::curselectionSubCmd
#
# This procedure is invoked to process the tablelist curselection subcommand.
#------------------------------------------------------------------------------
proc tablelist::curselectionSubCmd win {
    upvar ::tablelist::ns${win}::data data

    #
    # Find the (partly) selected lines of the body text widget
    #
    set result {}
    set w $data(body)
    set selRange [$w tag nextrange select 1.0]
    while {[llength $selRange] != 0} {
	set selStart [lindex $selRange 0]
	lappend result [expr {int($selStart) - 1}]

	set selRange [$w tag nextrange select "$selStart lineend"]
    }
    return $result
}

#------------------------------------------------------------------------------
# tablelist::deleteSubCmd
#
# This procedure is invoked to process the tablelist delete subcommand.
#------------------------------------------------------------------------------
proc tablelist::deleteSubCmd {win first last argCount} {
    upvar ::tablelist::ns${win}::data data

    if {$data(isDisabled)} {
	return ""
    }

    if {$argCount == 1} {
	if {[llength $first] == 1} {			;# just to save time
	    set index [rowIndex $win [lindex $first 0] 0]
	    return [deleteRows $win $index $index $data(hasListVar)]
	} elseif {$data(itemCount) == 0} {		;# no items present
	    return ""
	} else {					;# a bit more work
	    #
	    # Sort the numerical equivalents of the
	    # specified indices in decreasing order
	    #
	    set indexList {}
	    foreach elem $first {
		set index [rowIndex $win $elem 0]
		if {$index < 0} {
		    set index 0
		} elseif {$index > $data(lastRow)} {
		    set index $data(lastRow)
		}
		lappend indexList $index
	    }
	    set indexList [lsort -integer -decreasing $indexList]

	    #
	    # Traverse the sorted index list and ignore any duplicates
	    #
	    set prevIndex -1
	    foreach index $indexList {
		if {$index != $prevIndex} {
		    deleteRows $win $index $index $data(hasListVar)
		    set prevIndex $index
		}
	    }
	    return ""
	}
    } else {
	set first [rowIndex $win $first 0]
	set last [rowIndex $win $last 0]
	return [deleteRows $win $first $last $data(hasListVar)]
    }
}

#------------------------------------------------------------------------------
# tablelist::deleteRows
#
# Deletes a given range of rows of a tablelist widget.
#------------------------------------------------------------------------------
proc tablelist::deleteRows {win first last updateListVar} {
    upvar ::tablelist::ns${win}::data data

    #
    # Adjust the range to fit within the existing items
    #
    if {$first < 0} {
	set first 0
    }
    if {$last > $data(lastRow)} {
	set last $data(lastRow)
    }
    set count [expr {$last - $first + 1}]
    if {$count <= 0} {
	return ""
    }

    #
    # Check whether the width of any dynamic-width
    # column might be affected by the deletion
    #
    set w $data(body)
    set itemListRange [lrange $data(itemList) $first $last]
    if {$count == $data(itemCount)} {
	set colWidthsChanged 1				;# just to save time
	set data(seqNum) -1
	set data(freeKeyList) {}
    } else {
	variable canElide
	set colWidthsChanged 0
	set snipStr $data(-snipstring)
	set hasFmtCmds [expr {[lsearch -exact $data(fmtCmdFlagList) 1] >= 0}]
	foreach item $itemListRange {
	    #
	    # Format the item
	    #
	    if {$hasFmtCmds} {
		set formattedItem \
		    [formatItem $win [lrange $item 0 $data(lastCol)]]
	    } else {
		set formattedItem [lrange $item 0 $data(lastCol)]
	    }

	    set key [lindex $item end]
	    set col 0
	    foreach text [strToDispStr $formattedItem] \
		    {pixels alignment} $data(colList) {
		if {($data($col-hide) && !$canElide) || $pixels != 0} {
		    incr col
		    continue
		}

		getAuxData $win $key $col auxType auxWidth
		set cellFont [getCellFont $win $key $col]
		set textWidth [getCellTextWidth $win $text $auxWidth $cellFont]
		set elemWidth [expr {$auxWidth + $textWidth}]
		if {$elemWidth == $data($col-elemWidth) &&
		    [incr data($col-widestCount) -1] == 0} {
		    set colWidthsChanged 1
		    break
		}

		incr col
	    }

	    if {$colWidthsChanged} {
		break
	    }
	}
    }

    #
    # Delete the given items from the body text widget.  Interestingly,
    # for a large number of items it is much more efficient to delete
    # each line individually than to invoke a global delete command.
    #
    set textIdx1 [expr {double($first + 1)}]
    set textIdx2 [expr {double($first + 2)}]
    foreach item $itemListRange {
	$w delete $textIdx1 $textIdx2

	set key [lindex $item end]
	if {$count != $data(itemCount)} {
	    lappend data(freeKeyList) $key
	}

	foreach opt {-background -foreground -font} {
	    if {[info exists data($key$opt)]} {
		unset data($key$opt)
		incr data(tagRefCount) -1
	    }
	}
	if {[info exists data($key-hide)]} {
	    unset data($key-hide)
	    incr data(hiddenRowCount) -1
	}
	foreach opt {-name -selectable -selectbackground -selectforeground} {
	    if {[info exists data($key$opt)]} {
		unset data($key$opt)
	    }
	}

	for {set col 0} {$col < $data(colCount)} {incr col} {
	    foreach opt {-background -foreground -font} {
		if {[info exists data($key,$col$opt)]} {
		    unset data($key,$col$opt)
		    incr data(tagRefCount) -1
		}
	    }
	    foreach opt {-editable -editwindow -selectbackground
			 -selectforeground -windowdestroy} {
		if {[info exists data($key,$col$opt)]} {
		    unset data($key,$col$opt)
		}
	    }
	    if {[info exists data($key,$col-image)]} {
		unset data($key,$col-image)
		incr data(imgCount) -1
	    }
	    if {[info exists data($key,$col-window)]} {
		unset data($key,$col-window)
		unset data($key,$col-reqWidth)
		unset data($key,$col-reqHeight)
		incr data(winCount) -1
	    }
	}
    }

    #
    # Delete the given items from the internal list
    #
    set data(itemList) [lreplace $data(itemList) $first $last]
    incr data(itemCount) -$count
    incr data(lastRow) -$count

    #
    # Delete the given items from the list variable if needed
    #
    if {$updateListVar} {
	upvar #0 $data(-listvariable) var
	trace vdelete var wu $data(listVarTraceCmd)
	set var [lreplace $var $first $last]
	trace variable var wu $data(listVarTraceCmd)
    }

    #
    # Adjust the heights of the body text widget
    # and of the listbox child, if necessary
    #
    if {$data(-height) <= 0} {
	set nonHiddenRowCount [expr {$data(itemCount) - $data(hiddenRowCount)}]
	$w configure -height $nonHiddenRowCount
	$data(lb) configure -height $nonHiddenRowCount
    }

    #
    # Invalidate the list of the row indices indicating the
    # non-hidden rows, adjust the columns if necessary, and
    # schedule some operations for exection at idle time
    #
    set data(nonHiddenRowList) {-1}
    if {$colWidthsChanged} {
	adjustColumns $win allCols 1
    }
    adjustElidedTextWhenIdle $win
    makeStripesWhenIdle $win
    adjustSepsWhenIdle $win
    updateVScrlbarWhenIdle $win
    showLineNumbersWhenIdle $win

    #
    # Update the indices anchorRow and activeRow
    #
    if {$first <= $data(anchorRow)} {
	incr data(anchorRow) -$count
	if {$data(anchorRow) < $first} {
	    set data(anchorRow) $first
	}
	adjustRowIndex $win data(anchorRow) 1
    }
    if {$last < $data(activeRow)} {
	incr data(activeRow) -$count
	adjustRowIndex $win data(activeRow) 1
    } elseif {$first <= $data(activeRow)} {
	set data(activeRow) $first
	adjustRowIndex $win data(activeRow) 1
    }

    #
    # Update data(editRow) if the edit window is present
    #
    if {$data(editRow) >= 0} {
	set data(editRow) [lsearch $data(itemList) "* $data(editKey)"]
    }

    return ""
}

#------------------------------------------------------------------------------
# tablelist::deletecolumnsSubCmd
#
# This procedure is invoked to process the tablelist deletecolumns subcommand.
#------------------------------------------------------------------------------
proc tablelist::deletecolumnsSubCmd {win first last argCount} {
    upvar ::tablelist::ns${win}::data data

    if {$data(isDisabled)} {
	return ""
    }

    if {$argCount == 1} {
	if {[llength $first] == 1} {			;# just to save time
	    set col [colIndex $win [lindex $first 0] 1]
	    set selCells [curcellselectionSubCmd $win]
	    deleteCols $win $col $col selCells
	    redisplay $win 0 $selCells
	} elseif {$data(colCount) == 0} {		;# no columns present
	    return ""
	} else {					;# a bit more work
	    #
	    # Sort the numerical equivalents of the
	    # specified column indices in decreasing order
	    #
	    set colList {}
	    foreach elem $first {
		lappend colList [colIndex $win $elem 1]
	    }
	    set colList [lsort -integer -decreasing $colList]

	    #
	    # Traverse the sorted column index
	    # list and ignore any duplicates
	    #
	    set selCells [curcellselectionSubCmd $win]
	    set deleted 0
	    set prevCol -1
	    foreach col $colList {
		if {$col != $prevCol} {
		    deleteCols $win $col $col selCells
		    set deleted 1
		    set prevCol $col
		}
	    }
	    if {$deleted} {
		redisplay $win 0 $selCells
	    }
	}
    } else {
	set first [colIndex $win $first 1]
	set last [colIndex $win $last 1]
	if {$first <= $last} {
	    set selCells [curcellselectionSubCmd $win]
	    deleteCols $win $first $last selCells
	    redisplay $win 0 $selCells
	}
    }

    return ""
}

#------------------------------------------------------------------------------
# tablelist::deleteCols
#
# Deletes a given range of columns of a tablelist widget.
#------------------------------------------------------------------------------
proc tablelist::deleteCols {win first last selCellsName} {
    upvar ::tablelist::ns${win}::data data
    upvar $selCellsName selCells

    #
    # Delete the data corresponding to the given range
    #
    for {set col $first} {$col <= $last} {incr col} {
	if {$data($col-hide)} {
	    incr data(hiddenColCount) -1
	}
	deleteColData $win $col
	set selCells [deleteColFromCellList $selCells $col]
    }

    #
    # Shift the elements of data corresponding to the column
    # indices > last to the left by last - first + 1 positions
    #
    for {set oldCol [expr {$last + 1}]; set newCol $first} \
	{$oldCol < $data(colCount)} {incr oldCol; incr newCol} {
	moveColData $win data data imgs $oldCol $newCol
	set selCells [replaceColInCellList $selCells $oldCol $newCol]
    }

    #
    # Update the item list
    #
    set newItemList {}
    foreach item $data(itemList) {
	set item [lreplace $item $first $last]
	lappend newItemList $item
    }
    set data(itemList) $newItemList

    #
    # Update the list variable if present
    #
    condUpdateListVar $win

    #
    # Set up and adjust the columns, and rebuild some columns-related lists
    #
    setupColumns $win \
	[lreplace $data(-columns) [expr {3*$first}] [expr {3*$last + 2}]] 1
    makeColFontAndTagLists $win
    makeSortAndArrowColLists $win
    adjustColumns $win {} 1

    #
    # Reconfigure the relevant column labels
    #
    for {set col $first} {$col < $data(colCount)} {incr col} {
	reconfigColLabels $win imgs $col
    }

    #
    # Update the indices anchorCol and activeCol
    #
    set count [expr {$last - $first + 1}]
    if {$first <= $data(anchorCol)} {
	incr data(anchorCol) -$count
	if {$data(anchorCol) < $first} {
	    set data(anchorCol) $first
	}
	adjustColIndex $win data(anchorCol) 1
    }
    if {$last < $data(activeCol)} {
	incr data(activeCol) -$count
	adjustColIndex $win data(activeCol) 1
    } elseif {$first <= $data(activeCol)} {
	set data(activeCol) $first
	adjustColIndex $win data(activeCol) 1
    }
}

#------------------------------------------------------------------------------
# tablelist::fillcolumnSubCmd
#
# This procedure is invoked to process the tablelist fillcolumn subcommand.
#------------------------------------------------------------------------------
proc tablelist::fillcolumnSubCmd {win colIdx text} {
    upvar ::tablelist::ns${win}::data data

    if {$data(isDisabled)} {
	return ""
    }

    #
    # Update the item list
    #
    set newItemList {}
    foreach item $data(itemList) {
	set item [lreplace $item $colIdx $colIdx $text]
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
    adjustColumns $win $colIdx 1
    redisplayColWhenIdle $win $colIdx
    return ""
}

#------------------------------------------------------------------------------
# tablelist::getSubCmd
#
# This procedure is invoked to process the tablelist get subcommand.
#------------------------------------------------------------------------------
proc tablelist::getSubCmd {win first last argCount} {
    upvar ::tablelist::ns${win}::data data

    #
    # Get the specified items from the internal list
    #
    set result {}
    if {$argCount == 1} {
	foreach elem $first {
	    set index [rowIndex $win $elem 0]
	    if {$index >= 0 && $index < $data(itemCount)} {
		set item [lindex $data(itemList) $index]
		lappend result [lrange $item 0 $data(lastCol)]
	    }
	}

	if {[llength $first] == 1} {
	    return [lindex $result 0]
	} else {
	    return $result
	}
    } else {
	set first [rowIndex $win $first 0]
	set last [rowIndex $win $last 0]

	#
	# Adjust the range to fit within the existing items
	#
	if {$first > $data(lastRow)} {
	    return {}
	}
	if {$first < 0} {
	    set first 0
	}
	if {$last > $data(lastRow)} {
	    set last $data(lastRow)
	}

	foreach item [lrange $data(itemList) $first $last] {
	    lappend result [lrange $item 0 $data(lastCol)]
	}
	return $result
    }
}

#------------------------------------------------------------------------------
# tablelist::getcellsSubCmd
#
# This procedure is invoked to process the tablelist getcells subcommand.
#------------------------------------------------------------------------------
proc tablelist::getcellsSubCmd {win first last argCount} {
    upvar ::tablelist::ns${win}::data data

    #
    # Get the specified elements from the internal list
    #
    set result {}
    if {$argCount == 1} {
	foreach elem $first {
	    scan [cellIndex $win $elem 1] "%d,%d" row col
	    lappend result [lindex [lindex $data(itemList) $row] $col]
	}

	if {[llength $first] == 1} {
	    return [lindex $result 0]
	} else {
	    return $result
	}
    } else {
	scan [cellIndex $win $first 1] "%d,%d" firstRow firstCol
	scan [cellIndex $win $last 1] "%d,%d" lastRow lastCol

	foreach item [lrange $data(itemList) $firstRow $lastRow] {
	    foreach elem [lrange $item $firstCol $lastCol] {
		lappend result $elem
	    }
	}
	return $result
    }
}

#------------------------------------------------------------------------------
# tablelist::getcolumnsSubCmd
#
# This procedure is invoked to process the tablelist getcolumns subcommand.
#------------------------------------------------------------------------------
proc tablelist::getcolumnsSubCmd {win first last argCount} {
    upvar ::tablelist::ns${win}::data data

    #
    # Get the specified columns from the internal list
    #
    set result {}
    if {$argCount == 1} {
	foreach elem $first {
	    set col [colIndex $win $elem 1]
	    set colResult {}
	    foreach item $data(itemList) {
		lappend colResult [lindex $item $col]
	    }
	    lappend result $colResult
	}

	if {[llength $first] == 1} {
	    return [lindex $result 0]
	} else {
	    return $result
	}
    } else {
	set first [colIndex $win $first 1]
	set last [colIndex $win $last 1]

	for {set col $first} {$col <= $last} {incr col} {
	    set colResult {}
	    foreach item $data(itemList) {
		lappend colResult [lindex $item $col]
	    }
	    lappend result $colResult
	}
	return $result
    }
}

#------------------------------------------------------------------------------
# tablelist::getkeysSubCmd
#
# This procedure is invoked to process the tablelist getkeys subcommand.
#------------------------------------------------------------------------------
proc tablelist::getkeysSubCmd {win first last argCount} {
    upvar ::tablelist::ns${win}::data data

    #
    # Get the specified keys from the internal list
    #
    set result {}
    if {$argCount == 1} {
	foreach elem $first {
	    set index [rowIndex $win $elem 0]
	    if {$index >= 0 && $index < $data(itemCount)} {
		set item [lindex $data(itemList) $index]
		lappend result [string range [lindex $item end] 1 end]
	    }
	}

	if {[llength $first] == 1} {
	    return [lindex $result 0]
	} else {
	    return $result
	}
    } else {
	set first [rowIndex $win $first 0]
	set last [rowIndex $win $last 0]

	#
	# Adjust the range to fit within the existing items
	#
	if {$first > $data(lastRow)} {
	    return {}
	}
	if {$first < 0} {
	    set first 0
	}
	if {$last > $data(lastRow)} {
	    set last $data(lastRow)
	}

	foreach item [lrange $data(itemList) $first $last] {
	    lappend result [string range [lindex $item end] 1 end]
	}
	return $result
    }
}

#------------------------------------------------------------------------------
# tablelist::insertSubCmd
#
# This procedure is invoked to process the tablelist insert and insertlist
# subcommands.
#------------------------------------------------------------------------------
proc tablelist::insertSubCmd {win index argList updateListVar} {
    upvar ::tablelist::ns${win}::data data

    if {$data(isDisabled)} {
	return ""
    }

    set argCount [llength $argList]
    if {$argCount == 0} {
	return ""
    }

    if {$index < 0} {
	set index 0
    }

    #
    # Insert the items into the body text widget and into the internal list
    #
    variable canElide
    set w $data(body)
    set widgetFont $data(-font)
    set snipStr $data(-snipstring)
    set savedCount $data(itemCount)
    set colWidthsChanged 0
    set row $index
    set line [expr {$index + 1}]
    set hasFmtCmds [expr {[lsearch -exact $data(fmtCmdFlagList) 1] >= 0}]
    foreach item $argList {
	#
	# Adjust and format the item
	#
	set item [adjustItem $item $data(colCount)]
	if {$hasFmtCmds} {
	    set formattedItem [formatItem $win $item]
	} else {
	    set formattedItem $item
	}

	#
	# Get a free key for the new item
	#
	if {[llength $data(freeKeyList)] == 0} {
	    set key k[incr data(seqNum)]
	} else {
	    set key [lindex $data(freeKeyList) 0]
	    set data(freeKeyList) [lrange $data(freeKeyList) 1 end]
	}

	set multilineData {}
	if {$data(itemCount) != 0} {
	     $w insert $line.0 "\n"
	}

	set col 0
	if {$data(hasColTags)} {
	    set insertArgs {}
	    foreach text [strToDispStr $formattedItem] \
		    colFont $data(colFontList) \
		    colTags $data(colTagsList) \
		    {pixels alignment} $data(colList) {
		if {$data($col-hide) && !$canElide} {
		    incr col
		    continue
		}

		#
		# Update the column width or clip the element if necessary
		#
		if {[string match "*\n*" $text]} {
		    set multiline 1
		    set list [split $text "\n"]
		} else {
		    set multiline 0
		}
		if {$pixels == 0} {		;# convention: dynamic width
		    if {$multiline} {
			set textWidth [getListWidth $win $list $colFont]
		    } else {
			set textWidth \
			    [font measure $colFont -displayof $win $text]
		    }
		    if {$data($col-maxPixels) > 0} {
			if {$textWidth > $data($col-maxPixels)} {
			    set pixels $data($col-maxPixels)
			}
		    }
		    if {$textWidth == $data($col-elemWidth)} {
			incr data($col-widestCount)
		    } elseif {$textWidth > $data($col-elemWidth)} {
			set data($col-elemWidth) $textWidth
			set data($col-widestCount) 1
			if {$textWidth > $data($col-reqPixels)} {
			    set data($col-reqPixels) $textWidth
			    if {$pixels == 0} {
				set colWidthsChanged 1
			    }
			}
		    }
		}
		if {$pixels != 0} {
		    incr pixels $data($col-delta)
		    if {$multiline} {
			set text [joinList $win $list $colFont \
				  $pixels $alignment $snipStr]
		    } else {
			set text [strRange $win $text $colFont \
				  $pixels $alignment $snipStr]
		    }
		}

		if {$multiline} {
		    lappend insertArgs "\t\t" $colTags
		    lappend multilineData $col $text $colFont $alignment
		} else {
		    lappend insertArgs "\t$text\t" $colTags
		}
		incr col
	    }

	    #
	    # Insert the item into the body text widget
	    #
	    if {[llength $insertArgs] != 0} {
		eval [list $w insert $line.0] $insertArgs
	    }

	} else {
	    set insertStr ""
	    foreach text [strToDispStr $formattedItem] \
		    {pixels alignment} $data(colList) {
		if {$data($col-hide) && !$canElide} {
		    incr col
		    continue
		}

		#
		# Update the column width or clip the element if necessary
		#
		if {[string match "*\n*" $text]} {
		    set multiline 1
		    set list [split $text "\n"]
		} else {
		    set multiline 0
		}
		if {$pixels == 0} {		;# convention: dynamic width
		    if {$multiline} {
			set textWidth [getListWidth $win $list $widgetFont]
		    } else {
			set textWidth \
			    [font measure $widgetFont -displayof $win $text]
		    }
		    if {$data($col-maxPixels) > 0} {
			if {$textWidth > $data($col-maxPixels)} {
			    set pixels $data($col-maxPixels)
			}
		    }
		    if {$textWidth == $data($col-elemWidth)} {
			incr data($col-widestCount)
		    } elseif {$textWidth > $data($col-elemWidth)} {
			set data($col-elemWidth) $textWidth
			set data($col-widestCount) 1
			if {$textWidth > $data($col-reqPixels)} {
			    set data($col-reqPixels) $textWidth
			    if {$pixels == 0} {
				set colWidthsChanged 1
			    }
			}
		    }
		}
		if {$pixels != 0} {
		    incr pixels $data($col-delta)
		    if {$multiline} {
			set text [joinList $win $list $widgetFont \
				  $pixels $alignment $snipStr]
		    } else {
			set text [strRange $win $text $widgetFont \
				  $pixels $alignment $snipStr]
		    }
		}

		if {$multiline} {
		    append insertStr "\t\t"
		    lappend multilineData $col $text $widgetFont $alignment
		} else {
		    append insertStr "\t$text\t"
		}
		incr col
	    }

	    #
	    # Insert the item into the body text widget
	    #
	    $w insert $line.0 $insertStr
	}

	#
	# Embed the message widgets displaying multiline elements
	#
	foreach {col text font alignment} $multilineData {
	    findTabs $win $line $col $col tabIdx1 tabIdx2
	    set msgScript [list ::tablelist::displayText $win \
			   $key $col $text $font $alignment]
	    $w window create $tabIdx2 -pady 1 -create $msgScript
	}

	#
	# Insert the item into the list variable if needed
	#
	if {$updateListVar} {
	    upvar #0 $data(-listvariable) var
	    trace vdelete var wu $data(listVarTraceCmd)
	    if {$row == $data(itemCount)} {
		lappend var $item    		;# this works much faster
	    } else {
		set var [linsert $var $row $item]
	    }
	    trace variable var wu $data(listVarTraceCmd)
	}

	#
	# Insert the item into the internal list
	#
	lappend item $key
	if {$row == $data(itemCount)} {
	    lappend data(itemList) $item	;# this works much faster
	} else {
	    set data(itemList) [linsert $data(itemList) $row $item]
	}

	set row $line
	incr line
	incr data(itemCount)
    }
    set data(lastRow) [expr {$data(itemCount) - 1}]

    #
    # Adjust the heights of the body text widget
    # and of the listbox child, if necessary
    #
    if {$data(-height) <= 0} {
	set nonHiddenRowCount [expr {$data(itemCount) - $data(hiddenRowCount)}]
	$w configure -height $nonHiddenRowCount
	$data(lb) configure -height $nonHiddenRowCount
    }

    #
    # Adjust the horizontal view in the body text 
    # widget if the tablelist was previously empty
    #
    if {$savedCount == 0} {
	$w xview moveto [lindex [$data(hdrTxt) xview] 0]
    }

    #
    # Invalidate the list of the row indices indicating the
    # non-hidden rows, adjust the columns if necessary, and
    # schedule some operations for execution at idle time
    #
    set data(nonHiddenRowList) {-1}
    if {$colWidthsChanged} {
	adjustColumns $win {} 1
    }
    adjustElidedTextWhenIdle $win
    makeStripesWhenIdle $win
    adjustSepsWhenIdle $win
    updateVScrlbarWhenIdle $win
    showLineNumbersWhenIdle $win

    #
    # Update the indices anchorRow and activeRow
    #
    if {$index <= $data(anchorRow)} {
	incr data(anchorRow) $argCount
	adjustRowIndex $win data(anchorRow) 1
    }
    if {$index <= $data(activeRow)} {
	incr data(activeRow) $argCount
	adjustRowIndex $win data(activeRow) 1
    }

    #
    # Update data(editRow) if the edit window is present
    #
    if {$data(editRow) >= 0} {
	set data(editRow) [lsearch $data(itemList) "* $data(editKey)"]
    }

    return ""
}

#------------------------------------------------------------------------------
# tablelist::insertcolumnsSubCmd
#
# This procedure is invoked to process the tablelist insertcolumns and
# insertcolumnlist subcommands.
#------------------------------------------------------------------------------
proc tablelist::insertcolumnsSubCmd {win colIdx argList} {
    variable alignments
    upvar ::tablelist::ns${win}::data data

    if {$data(isDisabled)} {
	return ""
    }

    set argCount [llength $argList]
    if {$argCount == 0} {
	return ""
    }

    #
    # Check the syntax of argList and get the number of columns to be inserted
    #
    set count 0
    for {set n 0} {$n < $argCount} {incr n} {
	#
	# Check the column width
	#
	format "%d" [lindex $argList $n]    ;# integer check with error message

	#
	# Check whether the column title is present
	#
	if {[incr n] == $argCount} {
	    return -code error "column title missing"
	}

	#
	# Check the column alignment
	#
	set alignment left
	if {[incr n] < $argCount} {
	    set next [lindex $argList $n]
	    if {[catch {format "%d" $next}] == 0} {	;# integer check
		incr n -1
	    } else {
		mwutil::fullOpt "alignment" $next $alignments
	    }
	}

	incr count
    }

    #
    # Shift the elements of data corresponding to the column
    # indices >= colIdx to the right by count positions
    #
    set selCells [curcellselectionSubCmd $win]
    for {set oldCol $data(lastCol); set newCol [expr {$oldCol + $count}]} \
	{$oldCol >= $colIdx} {incr oldCol -1; incr newCol -1} {
	moveColData $win data data imgs $oldCol $newCol
	set selCells [replaceColInCellList $selCells $oldCol $newCol]
    }

    #
    # Update the item list
    #
    set emptyStrs {}
    for {set n 0} {$n < $count} {incr n} {
	lappend emptyStrs ""
    }
    set newItemList {}
    foreach item $data(itemList) {
	set item [eval [list linsert $item $colIdx] $emptyStrs]
	lappend newItemList $item
    }
    set data(itemList) $newItemList

    #
    # Update the list variable if present
    #
    condUpdateListVar $win

    #
    # Set up and adjust the columns, and rebuild some columns-related lists
    #
    setupColumns $win \
	[eval [list linsert $data(-columns) [expr {3*$colIdx}]] $argList] 1
    makeColFontAndTagLists $win
    makeSortAndArrowColLists $win
    set limit [expr {$colIdx + $count}]
    set colIdxList {}
    for {set col $colIdx} {$col < $limit} {incr col} {
	lappend colIdxList $col
    }
    adjustColumns $win $colIdxList 1

    #
    # Reconfigure the relevant column labels
    #
    for {set col $limit} {$col < $data(colCount)} {incr col} {
	reconfigColLabels $win imgs $col
    }

    #
    # Redisplay the items
    #
    redisplay $win 0 $selCells

    #
    # Update the indices anchorCol and activeCol
    #
    if {$colIdx <= $data(anchorCol)} {
	incr data(anchorCol) $argCount
	adjustColIndex $win data(anchorCol) 1
    }
    if {$colIdx <= $data(activeCol)} {
	incr data(activeCol) $argCount
	adjustColIndex $win data(activeCol) 1
    }

    return ""
}

#------------------------------------------------------------------------------
# tablelist::scanSubCmd
#
# This procedure is invoked to process the tablelist scan subcommand.
#------------------------------------------------------------------------------
proc tablelist::scanSubCmd {win opt x y} {
    upvar ::tablelist::ns${win}::data data

    set w $data(body)
    incr x -[winfo x $w]
    incr y -[winfo y $w]

    if {$data(-titlecolumns) == 0} {
	$w scan $opt $x $y
	$data(hdrTxt) scan $opt $x 0

	if {[string compare $opt "dragto"] == 0} {
	    adjustElidedText $win
	    updateColorsWhenIdle $win
	    adjustSepsWhenIdle $win
	    updateVScrlbarWhenIdle $win
	}
    } elseif {[string compare $opt "mark"] == 0} {
	$w scan mark 0 $y

	set data(scanMarkX) $x
	set data(scanMarkXOffset) \
	    [scrlColOffsetToXOffset $win $data(scrlColOffset)]
    } else {
	$w scan dragto 0 $y

	#
	# Compute the new scrolled x offset by amplifying the
	# difference between the current horizontal position and
	# the place where the scan started (the "mark" position)
	#
	set scrlXOffset \
	    [expr {$data(scanMarkXOffset) - 10*($x - $data(scanMarkX))}]
	set maxScrlXOffset [scrlColOffsetToXOffset $win \
			    [getMaxScrlColOffset $win]]
	if {$scrlXOffset > $maxScrlXOffset} {
	    set scrlXOffset $maxScrlXOffset
	    set data(scanMarkX) $x
	    set data(scanMarkXOffset) $maxScrlXOffset
	} elseif {$scrlXOffset < 0} {
	    set scrlXOffset 0
	    set data(scanMarkX) $x
	    set data(scanMarkXOffset) 0
	}

	#
	# Change the scrolled column offset and adjust the elided text
	#
	changeScrlColOffset $win [scrlXOffsetToColOffset $win $scrlXOffset]
	adjustElidedText $win
	updateColorsWhenIdle $win
	adjustSepsWhenIdle $win
	updateVScrlbarWhenIdle $win
    }

    return ""
}

#------------------------------------------------------------------------------
# tablelist::seeSubCmd
#
# This procedure is invoked to process the tablelist see subcommand.
#------------------------------------------------------------------------------
proc tablelist::seeSubCmd {win index} {
    upvar ::tablelist::ns${win}::data data

    #
    # Adjust the index to fit within the existing items
    #
    adjustRowIndex $win index
    set key [lindex [lindex $data(itemList) $index] end]
    if {$data(itemCount) == 0 || [info exists data($key-hide)]} {
	return ""
    }

    #
    # Bring the given row into the window and restore
    # the horizontal view in the body text widget
    #
    $data(body) see [expr {double($index + 1)}]
    $data(body) xview moveto [lindex [$data(hdrTxt) xview] 0]

    adjustElidedText $win
    updateColorsWhenIdle $win
    adjustSepsWhenIdle $win
    updateVScrlbarWhenIdle $win
    return ""
}

#------------------------------------------------------------------------------
# tablelist::seecellSubCmd
#
# This procedure is invoked to process the tablelist seecell subcommand.
#------------------------------------------------------------------------------
proc tablelist::seecellSubCmd {win row col} {
    #
    # This might be an "after idle" callback; check whether the window exists
    #
    if {![winfo exists $win]} {
	return ""
    }

    upvar ::tablelist::ns${win}::data data
    set h $data(hdrTxt)
    set b $data(body)

    #
    # Adjust the row and column indices to fit within the existing elements
    #
    adjustRowIndex $win row
    adjustColIndex $win col
    set key [lindex [lindex $data(itemList) $row] end]
    if {[info exists data($key-hide)]} {
	return ""
    }
    if {$data(colCount) == 0} {
	$b see [expr {double($row + 1)}]
	return ""
    } elseif {$data($col-hide)} {
	return ""
    }

    #
    # Force any geometry manager calculations to be completed first
    #
    update idletasks

    #
    # If the tablelist is empty then insert a temporary row
    #
    if {$data(itemCount) == 0} {
	variable canElide
	for {set n 0} {$n < $data(colCount)} {incr n} {
	    if {!$data($n-hide) || $canElide} {
		$b insert end "\t\t"
	    }
	}

	$b xview moveto [lindex [$h xview] 0]
    }

    if {$data(-titlecolumns) == 0} {
	findTabs $win [expr {$row + 1}] $col $col tabIdx1 tabIdx2
	set nextIdx [$b index $tabIdx2+1c]
	set alignment [lindex $data(colList) [expr {2*$col + 1}]]
	set lX [winfo x $data(hdrTxtFrLbl)$col]
	set rX [expr {$lX + [winfo width $data(hdrTxtFrLbl)$col] - 1}]

	switch $alignment {
	    left {
		#
		# Bring the cell's left edge into view
		#
		$b see $tabIdx1
		$h xview moveto [lindex [$b xview] 0]

		#
		# Shift the view in the header text widget until the right
		# edge of the cell becomes visible but finish the scrolling
		# before the cell's left edge would become invisible
		#
		while {![isHdrTxtFrXPosVisible $win $rX]} {
		    $h xview scroll 1 units
		    if {![isHdrTxtFrXPosVisible $win $lX]} {
			$h xview scroll -1 units
			break
		    }
		}
	    }

	    center {
		#
		# Bring the cell's left edge into view
		#
		$b see $tabIdx1
		set winWidth [winfo width $h]
		if {[winfo width $data(hdrTxtFrLbl)$col] > $winWidth} {
		    #
		    # The cell doesn't fit into the window:  Bring its
		    # center into the window's middle horizontal position
		    #
		    $h xview moveto \
		       [expr {double($lX + $rX - $winWidth)/2/$data(hdrPixels)}]
		} else {
		    #
		    # Shift the view in the header text widget until
		    # the right edge of the cell becomes visible
		    #
		    $h xview moveto [lindex [$b xview] 0]
		    while {![isHdrTxtFrXPosVisible $win $rX]} {
			$h xview scroll 1 units
		    }
		}
	    }

	    right {
		#
		# Bring the cell's right edge into view
		#
		$b see $nextIdx
		$h xview moveto [lindex [$b xview] 0]

		#
		# Shift the view in the header text widget until the left
		# edge of the cell becomes visible but finish the scrolling
		# before the cell's right edge would become invisible
		#
		while {![isHdrTxtFrXPosVisible $win $lX]} {
		    $h xview scroll -1 units
		    if {![isHdrTxtFrXPosVisible $win $rX]} {
			$h xview scroll 1 units
			break
		    }
		}
	    }
	}

	$b xview moveto [lindex [$h xview] 0]

    } else {
	#
	# Bring the cell's row into view
	#
	$b see [expr {double($row + 1)}]

	set scrlWindowWidth [getScrlWindowWidth $win]

	if {($col < $data(-titlecolumns)) ||
	    (!$data($col-elide) &&
	     [getScrlContentWidth $win $data(scrlColOffset) $col] <=
	     $scrlWindowWidth)} {
	    #
	    # The given column index specifies either a title column or
	    # one that is fully visible; restore the horizontal view
	    #
	    $b xview moveto [lindex [$h xview] 0]
	    adjustElidedText $win
	} elseif {$data($col-elide) ||
		  [winfo width $data(hdrTxtFrLbl)$col] > $scrlWindowWidth} {
	    #
	    # The given column index specifies either an elided column or one
	    # that doesn't fit into the window; shift the horizontal view to
	    # make the column the first visible one among all scrollable columns
	    #
	    set scrlColOffset 0
	    for {incr col -1} {$col >= $data(-titlecolumns)} {incr col -1} {
		if {!$data($col-hide)} {
		    incr scrlColOffset
		}
	    }
	    changeScrlColOffset $win $scrlColOffset
	} else {
	    #
	    # The given column index specifies a non-elided
	    # scrollable column; shift the horizontal view
	    # repeatedly until the column becomes visible
	    #
	    set scrlColOffset [expr {$data(scrlColOffset) + 1}]
	    while {[getScrlContentWidth $win $scrlColOffset $col] >
		   $scrlWindowWidth} {
		incr scrlColOffset
	    }
	    changeScrlColOffset $win $scrlColOffset
	}
    }

    #
    # Delete the temporary row if any
    #
    if {$data(itemCount) == 0} {
	$b delete 1.0 end
    }

    updateColorsWhenIdle $win
    adjustSepsWhenIdle $win
    updateVScrlbarWhenIdle $win
    return ""
}

#------------------------------------------------------------------------------
# tablelist::selectionSubCmd
#
# This procedure is invoked to process the tablelist selection subcommand.
#------------------------------------------------------------------------------
proc tablelist::selectionSubCmd {win opt first last} {
    upvar ::tablelist::ns${win}::data data

    if {$data(isDisabled) && [string compare $opt "includes"] != 0} {
	return ""
    }

    switch $opt {
	anchor {
	    #
	    # Adjust the index to fit within the existing non-hidden items
	    #
	    adjustRowIndex $win first 1

	    set data(anchorRow) $first
	    return ""
	}

	clear {
	    #
	    # Swap the indices if necessary
	    #
	    if {$last < $first} {
		set tmp $first
		set first $last
		set last $tmp
	    }

	    set firstTextIdx [expr {$first + 1}].0
	    set lastTextIdx [expr {$last + 1}].end

	    #
	    # Find the (partly) selected lines of the body text
	    # widget in the text range specified by the two indices
	    #
	    set w $data(body)
	    variable canElide
	    variable elide
	    set selRange [$w tag nextrange select $firstTextIdx $lastTextIdx]
	    while {[llength $selRange] != 0} {
		set selStart [lindex $selRange 0]

		$w tag remove select $selStart "$selStart lineend"

		#
		# Handle the -(select)background and -(select)foreground cell
		# and column configuration options for each element of the row
		#
		set row [expr {int($selStart) - 1}]
		set key [lindex [lindex $data(itemList) $row] end]
		set textIdx1 "$selStart linestart"
		for {set col 0} {$col < $data(colCount)} {incr col} {
		    if {$data($col-hide) && !$canElide} {
			continue
		    }

		    set textIdx2 [$w search $elide "\t" \
				  $textIdx1+1c "$selStart lineend"]+1c
		    foreach optTail {background foreground} {
			set opt -select$optTail
			foreach name  [list $col$opt $key$opt $key,$col$opt] \
				level [list col row cell] {
			    if {[info exists data($name)]} {
				set tag $level$opt-$data($name)
				$w tag remove $tag $textIdx1 $textIdx2
			    }
			}
			foreach name  [list $col-$optTail $key-$optTail \
				       $key,$col-$optTail] \
				level [list col row cell] {
			    if {[info exists data($name)]} {
				set tag $level-$optTail-$data($name)
				$w tag add $tag $textIdx1 $textIdx2
			    }
			}
		    }
		    set textIdx1 $textIdx2
		}

		set selRange \
		    [$w tag nextrange select "$selStart lineend" $lastTextIdx]
	    }

	    updateColorsWhenIdle $win
	    return ""
	}

	includes {
	    set w $data(body)
	    set textIdx [expr {double($first + 1)}]
	    set selRange [$w tag nextrange select $textIdx "$textIdx lineend"]
	    if {[llength $selRange] > 0} {
		return 1
	    } else {
		return 0
	    }
	}

	set {
	    #
	    # Swap the indices if necessary and adjust
	    # the range to fit within the existing items
	    #
	    if {$last < $first} {
		set tmp $first
		set first $last
		set last $tmp
	    }
	    if {$first < 0} {
		set first 0
	    }
	    if {$last > $data(lastRow)} {
		set last $data(lastRow)
	    }

	    set w $data(body)
	    variable canElide
	    variable elide
	    for {set row $first; set line [expr {$first + 1}]} \
		{$row <= $last} {set row $line; incr line} {
		#
		# Check whether the row is selectable and non-hidden
		#
		set key [lindex [lindex $data(itemList) $row] end]
		if {[info exists data($key-selectable)] ||
		    [info exists data($key-hide)]} {
		    continue
		}

		#
		# Select the non-hidden elements of the row and handle
		# the -(select)background and -(select)foreground
		# cell and column configuration options for them
		#
		set textIdx1 $line.0
		for {set col 0} {$col < $data(colCount)} {incr col} {
		    if {$data($col-hide) && !$canElide} {
			continue
		    }

		    set textIdx2 \
			[$w search $elide "\t" $textIdx1+1c $line.end]+1c
		    if {$data($col-hide)} {
			set textIdx1 $textIdx2
			continue
		    }

		    $w tag add select $textIdx1 $textIdx2
		    foreach optTail {background foreground} {
			set opt -select$optTail
			foreach name  [list $col$opt $key$opt $key,$col$opt] \
				level [list col row cell] {
			    if {[info exists data($name)]} {
				set tag $level$opt-$data($name)
				$w tag add $tag $textIdx1 $textIdx2
			    }
			}
			foreach name  [list $col-$optTail $key-$optTail \
				       $key,$col-$optTail] \
				level [list col row cell] {
			    if {[info exists data($name)]} {
				set tag $level-$optTail-$data($name)
				$w tag remove $tag $textIdx1 $textIdx2
			    }
			}
		    }
		    set textIdx1 $textIdx2
		}
	    }

	    #
	    # If the selection is exported and there are any selected
	    # cells in the widget then make win the new owner of the
	    # PRIMARY selection and register a callback to be invoked
	    # when it loses ownership of the PRIMARY selection
	    #
	    if {$data(-exportselection) &&
		[llength [$w tag nextrange select 1.0]] != 0} {
		selection own -command \
			[list ::tablelist::lostSelection $win] $win
	    }

	    updateColorsWhenIdle $win
	    return ""
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::togglecolumnhideSubCmd
#
# This procedure is invoked to process the tablelist togglecolumnhide
# subcommand.
#------------------------------------------------------------------------------
proc tablelist::togglecolumnhideSubCmd {win first last argCount} {
    variable canElide
    upvar ::tablelist::ns${win}::data data

    #
    # Toggle the value of the -hide option of the specified columns
    #
    if {!$canElide} {
	set selCells [curcellselectionSubCmd $win]
    }
    set colIdxList {}
    if {$argCount == 1} {
	foreach elem $first {
	    set col [colIndex $win $elem 1]
	    if {$canElide && !$data($col-hide)} {
		cellselectionSubCmd $win clear 0 $col $data(lastRow) $col
	    }
	    set data($col-hide) [expr {!$data($col-hide)}]
	    if {$data($col-hide)} {
		incr data(hiddenColCount)
		if {$col == $data(editCol)} {
		    canceleditingSubCmd $win
		}
	    } else {
		incr data(hiddenColCount) -1
	    }
	    lappend colIdxList $col
	}
    } else {
	set first [colIndex $win $first 1]
	set last [colIndex $win $last 1]

	for {set col $first} {$col <= $last} {incr col} {
	    if {$canElide && !$data($col-hide)} {
		cellselectionSubCmd $win clear 0 $col $data(lastRow) $col
	    }
	    set data($col-hide) [expr {!$data($col-hide)}]
	    if {$data($col-hide)} {
		incr data(hiddenColCount)
		if {$col == $data(editCol)} {
		    canceleditingSubCmd $win
		}
	    } else {
		incr data(hiddenColCount) -1
	    }
	    lappend colIdxList $col
	}
    }

    if {[llength $colIdxList] == 0} {
	return ""
    }

    #
    # Adjust the columns and redisplay the items
    #
    adjustColumns $win $colIdxList 1
    adjustColIndex $win data(anchorCol) 1
    adjustColIndex $win data(activeCol) 1
    if {$canElide} {
	adjustElidedTextWhenIdle $win
    } else {
	redisplay $win 0 $selCells
    }
    return ""
}

#------------------------------------------------------------------------------
# tablelist::togglerowhideSubCmd
#
# This procedure is invoked to process the tablelist togglerowhide subcommand.
#------------------------------------------------------------------------------
proc tablelist::togglerowhideSubCmd {win first last argCount} {
    upvar ::tablelist::ns${win}::data data

    #
    # Toggle the value of the -hide option of the specified rows
    #
    if {$argCount == 1} {
	foreach elem $first {
	    set row [rowIndex $win $elem 0]
	    if {$row < 0 || $row > $data(lastRow)} {
		return -code error "row index \"$elem\" out of range"
	    }

	    doRowConfig $row $win -hide [expr {![doRowCget $row $win -hide]}]
	}
    } else {
	set firstRow [rowIndex $win $first 0]
	if {$firstRow < 0 || $firstRow > $data(lastRow)} {
	    return -code error "row index \"$first\" out of range"
	}

	set lastRow [rowIndex $win $last 0]
	if {$lastRow < 0 || $lastRow > $data(lastRow)} {
	    return -code error "row index \"$last\" out of range"
	}

	for {set row $firstRow} {$row <= $lastRow} {incr row} {
	    doRowConfig $row $win -hide [expr {![doRowCget $row $win -hide]}]
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::xviewSubCmd
#
# This procedure is invoked to process the tablelist xview subcommand.
#------------------------------------------------------------------------------
proc tablelist::xviewSubCmd {win argList} {
    variable winSys
    upvar ::tablelist::ns${win}::data data

    switch [llength $argList] {
	0 {
	    #
	    # Command: $win xview
	    #
	    if {$data(-titlecolumns) == 0} {
		return [$data(hdrTxt) xview]
	    } else {
		set scrlWindowWidth [getScrlWindowWidth $win]
		if {$scrlWindowWidth <= 0} {
		    return [list 0 0]
		}

		set scrlContentWidth [getScrlContentWidth $win 0 $data(lastCol)]
		if {$scrlContentWidth == 0} {
		    return [list 0 1]
		}

		set scrlXOffset \
		    [scrlColOffsetToXOffset $win $data(scrlColOffset)]
		set fraction1 [expr {$scrlXOffset/double($scrlContentWidth)}]
		set fraction2 [expr {($scrlXOffset + $scrlWindowWidth)/
				     double($scrlContentWidth)}]
		if {$fraction2 > 1.0} {
		    set fraction2 1.0
		}
		return [list [format "%g" $fraction1] [format "%g" $fraction2]]
	    }
	}

	1 {
	    #
	    # Command: $win xview <units>
	    #
	    set units [format "%d" [lindex $argList 0]]
	    if {$data(-titlecolumns) == 0} {
		foreach w [list $data(hdrTxt) $data(body)] {
		    $w xview moveto 0
		    $w xview scroll $units units
		}
	    } else {
		changeScrlColOffset $win $units
		updateColorsWhenIdle $win
	    }
	    return ""
	}

	default {
	    #
	    # Command: $win xview moveto <fraction>
	    #	       $win xview scroll <number> units|pages
	    #
	    set argList [mwutil::getScrollInfo $argList]
	    if {$data(-titlecolumns) == 0} {
		foreach w [list $data(hdrTxt) $data(body)] {
		    eval [list $w xview] $argList
		}
	    } else {
		if {[string compare [lindex $argList 0] "moveto"] == 0} {
		    #
		    # Compute the new scrolled column offset
		    #
		    set fraction [lindex $argList 1]
		    set scrlContentWidth \
			[getScrlContentWidth $win 0 $data(lastCol)]
		    set pixels [expr {int($fraction*$scrlContentWidth + 0.5)}]
		    set scrlColOffset [scrlXOffsetToColOffset $win $pixels]

		    #
		    # Increase the new scrolled column offset if necessary
		    #
		    if {$pixels + [getScrlWindowWidth $win] >=
			$scrlContentWidth} {
			incr scrlColOffset
		    }

		    changeScrlColOffset $win $scrlColOffset
		} else {
		    set number [lindex $argList 1]
		    if {[string compare [lindex $argList 2] "units"] == 0} {
			changeScrlColOffset $win \
			    [expr {$data(scrlColOffset) + $number}]
		    } else {
			#
			# Compute the new scrolled column offset
			#
			set scrlXOffset \
			    [scrlColOffsetToXOffset $win $data(scrlColOffset)]
			set scrlWindowWidth [getScrlWindowWidth $win]
			set deltaPixels [expr {$number*$scrlWindowWidth}]
			set pixels [expr {$scrlXOffset + $deltaPixels}]
			set scrlColOffset [scrlXOffsetToColOffset $win $pixels]

			#
			# Adjust the new scrolled column offset if necessary
			#
			if {$number < 0 &&
			    [getScrlContentWidth $win $scrlColOffset \
			     $data(lastCol)] -
			    [getScrlContentWidth $win $data(scrlColOffset) \
			     $data(lastCol)] > -$deltaPixels} {
			    incr scrlColOffset
			}
			if {$scrlColOffset == $data(scrlColOffset)} {
			    if {$number < 0} {
				incr scrlColOffset -1
			    } elseif {$number > 0} {
				incr scrlColOffset
			    }
			}

			changeScrlColOffset $win $scrlColOffset
		    }
		}
		updateColorsWhenIdle $win
	    }
	    if {[string compare $winSys "aqua"] == 0 && [winfo viewable $win]} {
		#
		# Work around some Tk bugs on Mac OS X Aqua
		#
		if {[winfo exists $data(bodyFr)]} {
		    lower $data(bodyFr)
		    raise $data(bodyFr)
		}
		update 
	    }
	    return ""
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::yviewSubCmd
#
# This procedure is invoked to process the tablelist yview subcommand.
#------------------------------------------------------------------------------
proc tablelist::yviewSubCmd {win argList} {
    variable winSys
    upvar ::tablelist::ns${win}::data data

    set w $data(body)
    set argCount [llength $argList]
    switch $argCount {
	0 {
	    #
	    # Command: $win yview
	    #
	    set totalNonHiddenCount \
		[expr {$data(itemCount) - $data(hiddenRowCount)}]
	    if {$totalNonHiddenCount == 0} {
		return [list 0 1]
	    }
	    set btmY [expr {[winfo height $w] - 1}]
	    set topTextIdx [$w index @0,0]
	    set btmTextIdx [$w index @0,$btmY]
	    set topRow [expr {int($topTextIdx) - 1}]
	    set btmRow [expr {int($btmTextIdx) - 1}]
	    foreach {x y width height baselinePos} [$w dlineinfo $btmTextIdx] {}
	    set y2 [expr {$y + $height}]
	    if {[$w index @0,$y] == [$w index @0,$y2]} {;# row not fully visible
		incr btmRow -1
	    }
	    set upperNonHiddenCount \
		[getNonHiddenRowCount $win 0 [expr {$topRow - 1}]]
	    set winNonHiddenCount [getNonHiddenRowCount $win $topRow $btmRow]
	    set fraction1 [expr {$upperNonHiddenCount/
				 double($totalNonHiddenCount)}]
	    set fraction2 [expr {($upperNonHiddenCount + $winNonHiddenCount)/
				 double($totalNonHiddenCount)}]
	    return [list [format "%g" $fraction1] [format "%g" $fraction2]]
	}

	1 {
	    #
	    # Command: $win yview <units>
	    #
	    set units [format "%d" [lindex $argList 0]]
	    $w yview [nonHiddenRowOffsetToRowIndex $win $units]
	    adjustElidedText $win
	    updateColorsWhenIdle $win
	    adjustSepsWhenIdle $win
	    updateVScrlbarWhenIdle $win
	    return ""
	}

	default {
	    #
	    # Command: $win yview moveto <fraction>
	    #	       $win yview scroll <number> units|pages
	    #
	    set argList [mwutil::getScrollInfo $argList]
	    if {[string compare [lindex $argList 0] "moveto"] == 0} {
		set fraction [lindex $argList 1]
		set totalNonHiddenCount \
		    [expr {$data(itemCount) - $data(hiddenRowCount)}]
		set offset [expr {int($fraction*$totalNonHiddenCount + 0.5)}]
		$w yview [nonHiddenRowOffsetToRowIndex $win $offset]
	    } else {
		set number [lindex $argList 1]
		if {[string compare [lindex $argList 2] "units"] == 0} {
		    set topRow [expr {int([$w index @0,0]) - 1}]
		    set upperNonHiddenCount \
			[getNonHiddenRowCount $win 0 [expr {$topRow - 1}]]
		    set offset [expr {$upperNonHiddenCount + $number}]
		    $w yview [nonHiddenRowOffsetToRowIndex $win $offset]
		} else {
		    set absNumber [expr {abs($number)}]
		    set btmY [expr {[winfo height $w] - 1}]
		    for {set n 0} {$n < $absNumber} {incr n} {
			set topRow [expr {int([$w index @0,0]) - 1}]
			set btmRow [expr {int([$w index @0,$btmY]) - 1}]
			set upperNonHiddenCount \
			    [getNonHiddenRowCount $win 0 [expr {$topRow - 1}]]
			set winNonHiddenCount \
			    [getNonHiddenRowCount $win $topRow $btmRow]
			set delta [expr {$winNonHiddenCount - 2}]
			if {$number < 0} {
			    set delta [expr {(-1)*$delta}]
			}
			set offset [expr {$upperNonHiddenCount + $delta}]
			$w yview [nonHiddenRowOffsetToRowIndex $win $offset]
		    }
		}
	    }
	    adjustElidedText $win
	    updateColorsWhenIdle $win
	    adjustSepsWhenIdle $win
	    updateVScrlbarWhenIdle $win
	    if {[string compare $winSys "aqua"] == 0 && [winfo viewable $win]} {
		#
		# Work around some Tk bugs on Mac OS X Aqua
		#
		if {[winfo exists $data(bodyFr)]} {
		    lower $data(bodyFr)
		    raise $data(bodyFr)
		}
		update 
	    }
	    return ""
	}
    }
}

#
# Private callback procedures
# ===========================
#

#------------------------------------------------------------------------------
# tablelist::fetchSelection
#
# This procedure is invoked when the PRIMARY selection is owned by the
# tablelist widget win and someone attempts to retrieve it as a STRING.  It
# returns part or all of the selection, as given by offset and maxChars.  The
# string which is to be (partially) returned is built by joining all of the
# selected elements of the (partly) selected rows together with tabs and the
# rows themselves with newlines.
#------------------------------------------------------------------------------
proc tablelist::fetchSelection {win offset maxChars} {
    upvar ::tablelist::ns${win}::data data

    if {!$data(-exportselection)} {
	return ""
    }

    set selection ""
    set prevRow -1
    foreach cellIdx [curcellselectionSubCmd $win] {
	scan $cellIdx "%d,%d" row col
	if {$row != $prevRow} {
	    if {$prevRow != -1} {
		append selection "\n"
	    }

	    set prevRow $row
	    set item [lindex $data(itemList) $row]
	    set isFirstCol 1
	}

	set text [lindex $item $col]
	if {[info exists data($col-formatcommand)]} {
	    set text [uplevel #0 $data($col-formatcommand) [list $text]]
	}

	if {!$isFirstCol} {
	    append selection "\t"
	}
	append selection $text

	set isFirstCol 0
    }

    return [string range $selection $offset [expr {$offset + $maxChars - 1}]]
}

#------------------------------------------------------------------------------
# tablelist::lostSelection
#
# This procedure is invoked when the tablelist widget win loses ownership of
# the PRIMARY selection.  It deselects all items of the widget with the aid of
# the selectionSubCmd procedure if the selection is exported.
#------------------------------------------------------------------------------
proc tablelist::lostSelection win {
    upvar ::tablelist::ns${win}::data data

    if {$data(-exportselection)} {
	selectionSubCmd $win clear 0 $data(lastRow)
	event generate $win <<TablelistSelectionLost>>
    }
}

#------------------------------------------------------------------------------
# tablelist::activeTrace
#
# This procedure is executed whenever the array element data(activeRow),
# data(activeCol), or data(-selecttype) is written.  It moves the "active" tag
# to the line or cell that displays the active item or element of the widget in
# its body text child if the latter has the keyboard focus.
#------------------------------------------------------------------------------
proc tablelist::activeTrace {win varName index op} {
    upvar ::tablelist::ns${win}::data data

    set w $data(body)
    if {$data(ownsFocus)} {
	$w tag remove active 1.0 end

	set line [expr {$data(activeRow) + 1}]
	set col $data(activeCol)
	if {[string compare $data(-selecttype) "row"] == 0} {
	    $w tag add active $line.0 $line.end
	} elseif {$data(itemCount) > 0 && $data(colCount) > 0 &&
		  !$data($col-hide)} {
	    findTabs $win $line $data(activeCol) $data(activeCol) \
		     tabIdx1 tabIdx2
	    $w tag add active $tabIdx1 $tabIdx2+1c
	}
    }
}

#------------------------------------------------------------------------------
# tablelist::listVarTrace
#
# This procedure is executed whenever the global variable specified by varName
# is written or unset.  It makes sure that the contents of the widget will be
# synchronized with the value of the variable at idle time, and that the
# variable is recreated if it was unset.
#------------------------------------------------------------------------------
proc tablelist::listVarTrace {win varName index op} {
    upvar ::tablelist::ns${win}::data data

    switch $op {
	w {
	    if {![info exists data(syncId)]} {
		#
		# Arrange for the contents of the widget to be synchronized
		# with the value of the variable ::$varName at idle time
		#
		set data(syncId) [after idle [list tablelist::synchronize $win]]
	    }
	}

	u {
	    #
	    # Recreate the variable ::$varName by setting it according to
	    # the value of data(itemList), and set the trace on it again
	    #
	    if {[string compare $index ""] != 0} {
		set varName ${varName}($index)
	    }
	    set ::$varName {}
	    foreach item $data(itemList) {
		lappend ::$varName [lrange $item 0 $data(lastCol)]
	    }
	    trace variable ::$varName wu $data(listVarTraceCmd)
	}
    }
}
