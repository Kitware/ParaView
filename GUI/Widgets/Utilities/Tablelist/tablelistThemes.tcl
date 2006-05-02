#==============================================================================
# Contains procedures that populate the array themeDefaults with theme-specific
# values of some tablelist configuration options.
#
# Copyright (c) 2005-2006  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================

#------------------------------------------------------------------------------
# tablelist::altTheme
#------------------------------------------------------------------------------
proc tablelist::altTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		white \
	-foreground		black \
	-disabledforeground	#a3a3a3 \
	-selectbackground	#4a6984 \
	-selectforeground	#ffffff \
	-selectborderwidth	0 \
	-font			TkTextFont \
        -labelbackground	#d9d9d9 \
	-labeldisabledBg	#d9d9d9 \
	-labelactiveBg		#ececec \
	-labelpressedBg		#ececec \
	-labelforeground	black \
	-labeldisabledFg	#a3a3a3 \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkDefaultFont \
	-labelborderwidth	2 \
	-labelpady		1 \
	-arrowcolor		"" \
	-arrowstyle		sunken10x9 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::aquaTheme
#------------------------------------------------------------------------------
proc tablelist::aquaTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		white \
	-foreground		black \
	-disabledforeground	#a3a3a3 \
	-selectbackground	#3875d8 \
	-selectforeground	#ffffff \
	-selectborderwidth	0 \
	-font			TkTooltipFont \
        -labelbackground	#f4f4f4 \
	-labeldisabledBg	#f4f4f4 \
	-labelactiveBg		#f4f4f4 \
	-labelpressedBg		#e4e4e4 \
	-labelforeground	black \
	-labeldisabledFg	#a3a3a3 \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkHeadingFont \
	-labelborderwidth	1 \
	-labelpady		1 \
	-arrowcolor		#777777 \
	-arrowstyle		flat7x7 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::AquativoTheme
#------------------------------------------------------------------------------
proc tablelist::AquativoTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		white \
	-foreground		black \
	-disabledforeground	black \
	-selectbackground	#000000 \
	-selectforeground	#ffffff \
	-selectborderwidth	0 \
	-font			TkTextFont \
        -labelbackground	#fafafa \
	-labeldisabledBg	#fafafa \
	-labelactiveBg		#fafafa \
	-labelpressedBg		#fafafa \
	-labelforeground	black \
	-labeldisabledFg	black \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkDefaultFont \
	-labelborderwidth	2 \
	-labelpady		1 \
	-arrowcolor		#777777 \
	-arrowstyle		flat7x7 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::blueTheme
#------------------------------------------------------------------------------
proc tablelist::blueTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		#e6f3ff \
	-foreground		black \
	-disabledforeground	#666666 \
	-selectbackground	#ffff33 \
	-selectforeground	#000000 \
	-selectborderwidth	1 \
	-font			TkTextFont \
        -labelbackground	#6699cc \
	-labeldisabledBg	#6699cc \
	-labelactiveBg		#6699cc \
	-labelpressedBg		#6699cc \
	-labelforeground	black \
	-labeldisabledFg	#666666 \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkDefaultFont \
	-labelborderwidth	2 \
	-labelpady		1 \
	-arrowcolor		"" \
	-arrowstyle		sunken10x9 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::clamTheme
#------------------------------------------------------------------------------
proc tablelist::clamTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		white \
	-foreground		black \
	-disabledforeground	#999999 \
	-selectbackground	#4a6984 \
	-selectforeground	#ffffff \
	-selectborderwidth	0 \
	-font			TkTextFont \
        -labelbackground	#dcdad5 \
	-labeldisabledBg	#dcdad5 \
	-labelactiveBg		#eeebe7 \
	-labelpressedBg		#eeebe7 \
	-labelforeground	black \
	-labeldisabledFg	#999999 \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkDefaultFont \
	-labelborderwidth	2 \
	-labelpady		1 \
	-arrowcolor		"" \
	-arrowstyle		sunken10x9 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::classicTheme
#------------------------------------------------------------------------------
proc tablelist::classicTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		white \
	-foreground		black \
	-disabledforeground	#a3a3a3 \
	-selectbackground	#c3c3c3 \
	-selectforeground	#000000 \
	-selectborderwidth	1 \
	-font			TkClassicDefaultFont \
        -labelbackground	#d9d9d9 \
	-labeldisabledBg	#d9d9d9 \
	-labelactiveBg		#ececec \
	-labelpressedBg		#ececec \
	-labelforeground	black \
	-labeldisabledFg	#a3a3a3 \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkClassicDefaultFont \
	-labelborderwidth	2 \
	-labelpady		1 \
	-arrowcolor		"" \
	-arrowstyle		sunken10x9 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::defaultTheme
#------------------------------------------------------------------------------
proc tablelist::defaultTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		white \
	-foreground		black \
	-disabledforeground	#a3a3a3 \
	-selectbackground	#4a6984 \
	-selectforeground	#ffffff \
	-selectborderwidth	1 \
	-font			TkTextFont \
        -labelbackground	#d9d9d9 \
	-labeldisabledBg	#d9d9d9 \
	-labelactiveBg		#ececec \
	-labelpressedBg		#ececec \
	-labelforeground	black \
	-labeldisabledFg	#a3a3a3 \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkDefaultFont \
	-labelborderwidth	1 \
	-labelpady		1 \
	-arrowcolor		"" \
	-arrowstyle		sunken10x9 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::keramikTheme
#------------------------------------------------------------------------------
proc tablelist::keramikTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		white \
	-foreground		black \
	-disabledforeground	#aaaaaa \
	-selectbackground	#000000 \
	-selectforeground	#ffffff \
	-selectborderwidth	0 \
	-font			TkTextFont \
        -labelbackground	#cccccc \
	-labeldisabledBg	#cccccc \
	-labelactiveBg		#cccccc \
	-labelpressedBg		#cccccc \
	-labelforeground	black \
	-labeldisabledFg	#aaaaaa \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkDefaultFont \
	-labelborderwidth	2 \
	-labelpady		1 \
	-arrowcolor		black \
	-arrowstyle		flat8x5 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::krocTheme
#------------------------------------------------------------------------------
proc tablelist::krocTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		white \
	-foreground		black \
	-disabledforeground	#b2b2b2 \
	-selectbackground	#000000 \
	-selectforeground	#ffffff \
	-selectborderwidth	1 \
	-font			TkTextFont \
        -labelbackground	#fcb64f \
	-labeldisabledBg	#fcb64f \
	-labelactiveBg		#694418 \
	-labelpressedBg		#694418 \
	-labelforeground	black \
	-labeldisabledFg	#b2b2b2 \
	-labelactiveFg		#ffe7cb \
	-labelpressedFg		#ffe7cb \
	-labelfont		TkDefaultFont \
	-labelborderwidth	2 \
	-labelpady		1 \
	-arrowcolor		"" \
	-arrowstyle		sunken10x9 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::plastikTheme
#------------------------------------------------------------------------------
proc tablelist::plastikTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		white \
	-foreground		black \
	-disabledforeground	#aaaaaa \
	-selectbackground	#657a9e \
	-selectforeground	#ffffff \
	-selectborderwidth	0 \
	-font			TkTextFont \
        -labelbackground	#cccccc \
	-labeldisabledBg	#cccccc \
	-labelactiveBg		#cccccc \
	-labelpressedBg		#cccccc \
	-labelforeground	black \
	-labeldisabledFg	#aaaaaa \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkDefaultFont \
	-labelborderwidth	2 \
	-labelpady		1 \
	-arrowcolor		black \
	-arrowstyle		flat7x4 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::srivTheme
#------------------------------------------------------------------------------
proc tablelist::srivTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		#e6f3ff \
	-foreground		black \
	-disabledforeground	#666666 \
	-selectbackground	#ffff33 \
	-selectforeground	#000000 \
	-selectborderwidth	1 \
	-font			TkTextFont \
        -labelbackground	#a0a0a0 \
	-labeldisabledBg	#a0a0a0 \
	-labelactiveBg		#a0a0a0 \
	-labelpressedBg		#a0a0a0 \
	-labelforeground	black \
	-labeldisabledFg	#666666 \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkDefaultFont \
	-labelborderwidth	2 \
	-labelpady		1 \
	-arrowcolor		"" \
	-arrowstyle		sunken10x9 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::srivlgTheme
#------------------------------------------------------------------------------
proc tablelist::srivlgTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		#e6f3ff \
	-foreground		black \
	-disabledforeground	#666666 \
	-selectbackground	#ffff33 \
	-selectforeground	#000000 \
	-selectborderwidth	1 \
	-font			TkTextFont \
        -labelbackground	#6699cc \
	-labeldisabledBg	#6699cc \
	-labelactiveBg		#6699cc \
	-labelpressedBg		#6699cc \
	-labelforeground	black \
	-labeldisabledFg	#666666 \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkDefaultFont \
	-labelborderwidth	2 \
	-labelpady		1 \
	-arrowcolor		"" \
	-arrowstyle		sunken10x9 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::stepTheme
#------------------------------------------------------------------------------
proc tablelist::stepTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		white \
	-foreground		black \
	-disabledforeground	#808080 \
	-selectbackground	#fdcd00 \
	-selectforeground	#ffffff \
	-selectborderwidth	0 \
	-font			TkTextFont \
        -labelbackground	#a0a0a0 \
	-labeldisabledBg	#a0a0a0 \
	-labelactiveBg		#aeb2c3 \
	-labelpressedBg		#aeb2c3 \
	-labelforeground	black \
	-labeldisabledFg	#808080 \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkDefaultFont \
	-labelborderwidth	2 \
	-labelpady		1 \
	-arrowcolor		"" \
	-arrowstyle		sunken10x9 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::tileqtTheme
#
# Tested with the following Qt styles:
#
#   .NET style	       KDE_XP			  MS Windows 9x	 RISC OS
#   Acqua	       Keramik			  Plastik	 SGI
#   B3/KDE	       Light Style, 2nd revision  Plastik	 System-Series
#   CDE		       Light Style, 3rd revision  Platinum	 System++
#   HighColor Classic  Marble			  QtCurve	 ThinKeramik
#   HighContrast       Motif			  QtCurve V2
#   KDE Classic	       Motif Plus		  QtCurve V3
#
# Supported color schemes:
#
#   Atlas Green			  Ice (FreddyK)	     Pumpkin
#   BeOS			  KDE 1		     Redmond 2000
#   Blue Slate			  KDE 2		     Redmond 95
#   CDE				  Keramik	     Redmond XP
#   Dark Blue			  Keramik Emerald    Solaris
#   Desert Red			  Keramik White	     Storm
#   Digital CDE			  Media Peach	     SuSE, old & new
#   EveX			  Next		     System
#   High Contrast Black Text	  Pale Gray	     Thin Keramik, old & new
#   High Contrast Yellow on Blue  Plastik	     Thin Keramik II
#   High Contrast White Text	  Point Reyes Green
#
# NOTE:  The implementation below reflects the current development state of the
#	 tile-qt extension.  Future tile-qt releases should make it possible to
#	 implement this procedure in a much more straight-forward way.
#------------------------------------------------------------------------------
proc tablelist::tileqtTheme {} {
    set bg       [tile::theme::tileqt::currentThemeColour -background]
    set fg       [tile::theme::tileqt::currentThemeColour -foreground]
    set selectBg [tile::theme::tileqt::currentThemeColour -selectbackground]
    set selectFg [tile::theme::tileqt::currentThemeColour -selectforeground]
    set style    [string tolower [tile::theme::tileqt::currentThemeName]]

    #
    # For most Qt styles the colors depend on the color scheme:
    #
    switch $bg {
	#afb49f {		;# color scheme "Atlas Green"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #afb49f;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {  
		"highcolor"	{ set labelBg #b2b6a1 }
		"sgi"		{ set labelBg #929684 }
		"dotnet"			      { set pressedBg #6f7a63 }
		"light, 3rd revision"		      { set pressedBg #c1c6af }
		"platinum"			      { set pressedBg #929684 }
		"*curve*"	{ set labelBg #b7bba8;  set pressedBg #8c917a }
		"keramik"	{ set labelBg #c7cabb;  set pressedBg #adb1a1 }
		"phase"		{ set labelBg #a7b49f;  set pressedBg #929684 }
		"plastik"	{ set labelBg #acb19c;  set pressedBg #959987 }
		"thinkeramik"	{ set labelBg #c1c4b6;  set pressedBg #a5a999 }
	    }
	}

	#d9d9d9 {		;# color scheme "BeOS"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #d9d9d9;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {  
		"highcolor"	{ set labelBg #dcdcdc }
		"sgi"		{ set labelBg #b4b4b4 }
		"dotnet"			      { set pressedBg #a8a8a8 }
		"light, 3rd revision"		      { set pressedBg #eeeeee }
		"platinum"			      { set pressedBg #b4b4b4 }
		"*curve*"	{ set labelBg #e3e3e3;  set pressedBg #ababab }
		"keramik"	{ set labelBg #e5e5e5;  set pressedBg #cdcdcd }
		"phase"		{ set labelBg #dadada;  set pressedBg #b4b4b4 }
		"plastik"	{ set labelBg #d6d6d6;  set pressedBg #b6b6b6 }
		"thinkeramik"	{ set labelBg #dddddd;  set pressedBg #c5c5c5 }
	    }
	}

	#9db9c8 {		;# color scheme "Blue Slate"
	    set tableBg #c3c3c3;  set tableFg #000000
	    set labelBg #9db9c8;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {  
		"highcolor"	{ set labelBg #9fbbcb }
		"sgi"		{ set labelBg #8299a6 }
		"dotnet"			      { set pressedBg #558097 }
		"light, 3rd revision"		      { set pressedBg #adcbdc }
		"platinum"			      { set pressedBg #8299a6 }
		"*curve*"	{ set labelBg #a7c1cf;  set pressedBg #7394a6 }
		"keramik"	{ set labelBg #baced9;  set pressedBg #a0b5c1 }
		"phase"		{ set labelBg #9db9c9;  set pressedBg #8299a6 }
		"plastik"	{ set labelBg #99b6c5;  set pressedBg #869fab }
		"thinkeramik"	{ set labelBg #b5c8d2;  set pressedBg #98adb8 }
	    }
	}

	#999999 {		;# color scheme "CDE"
	    set tableBg #818181;  set tableFg #ffffff
	    set labelBg #999999;  set labelFg #ffffff;  set pressedBg $labelBg
	    switch -glob -- $style {  
		"highcolor"	{ set labelBg #9b9b9b }
		"sgi"		{ set labelBg #7f7f7f }
		"dotnet"			      { set pressedBg #326284 }
		"light, 3rd revision"		      { set pressedBg #a8a8a8 }
		"platinum"			      { set pressedBg #7f7f7f }
		"*curve*"	{ set labelBg #a0a0a0;  set pressedBg #787878 }
		"keramik"	{ set labelBg #b7b7b7;  set pressedBg #9d9d9d }
		"phase"		{ set labelBg #999999;  set pressedBg #7f7f7f }
		"plastik"	{ set labelBg #979797;  set pressedBg #808080 }
		"thinkeramik"	{ set labelBg #b3b3b3;  set pressedBg #959595 }
	    }
	}

	#426794 {		;# color scheme "Dark Blue"
	    set tableBg #002a4e;  set tableFg #dcdcdc
	    set labelBg #426794;  set labelFg #ffffff;  set pressedBg $labelBg
	    switch -glob -- $style {  
		"highcolor"	{ set labelBg #436895 }
		"sgi"		{ set labelBg #37567b }
		"dotnet"			      { set pressedBg #5cb3ff }
		"light, 3rd revision"		      { set pressedBg #4871a2 }
		"platinum"			      { set pressedBg #37567b }
		"*curve*"	{ set labelBg #436b9d;  set pressedBg #3a526e }
		"keramik"	{ set labelBg #7994b4;  set pressedBg #5b7799 }
		"phase"		{ set labelBg #426795;  set pressedBg #37567b }
		"plastik"	{ set labelBg #406592;  set pressedBg #36547a }
		"thinkeramik"	{ set labelBg #7991af;  set pressedBg #546f91 }
	    }
	}

	#d6cdbb {		;# color scheme "Desert Red"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #d6cdbb;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {  
		"highcolor"	{ set labelBg #d9d0be }
		"sgi"		{ set labelBg #b2ab9c }
		"dotnet"			      { set pressedBg #800000 }
		"light, 3rd revision"		      { set pressedBg #ebe1ce }
		"platinum"			      { set pressedBg #b2ab9c }
		"*curve*"	{ set labelBg #ded6c6;  set pressedBg #b1a48b }
		"keramik"	{ set labelBg #e3dcd0;  set pressedBg #cbc5b7 }
		"phase"		{ set labelBg #d6cdbb;  set pressedBg #b2ab9c }
		"plastik"	{ set labelBg #d3cbb8;  set pressedBg #bab3a3 }
		"thinkeramik"	{ set labelBg #dbd5ca;  set pressedBg #c2bbae }
	    }
	}

	#4b7b82 {		;# color scheme "Digital CDE"
	    set tableBg #374d4e;  set tableFg #ffffff
	    set labelBg #4b7b82;  set labelFg #ffffff;  set pressedBg $labelBg
	    switch -glob -- $style {  
		"highcolor"	{ set labelBg #4b7d84 }
		"sgi"		{ set labelBg #3e666c }
		"dotnet"			      { set pressedBg #526674 }
		"light, 3rd revision"		      { set pressedBg #52878f }
		"platinum"			      { set pressedBg #3e666c }
		"*curve*"	{ set labelBg #4d8289;  set pressedBg #3f5d62 }
		"keramik"	{ set labelBg #80a2a7;  set pressedBg #62868c }
		"phase"		{ set labelBg #4b7b82;  set pressedBg #3e666c }
		"plastik"	{ set labelBg #49787f;  set pressedBg #3d666c }
		"thinkeramik"	{ set labelBg #7f97a3;  set pressedBg #5a7e83 }
	    }
	}

	#e6dedc {		;# color scheme "EveX"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #e4e4e4;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {  
		"highcolor"	{ set labelBg #e7e7e7 }
		"sgi"		{ set labelBg #e4e4e4 }
		"dotnet"			      { set pressedBg #0a5f89 }
		"light, 3rd revision"		      { set pressedBg #fdf4f2 }
		"platinum"			      { set pressedBg #bfb8b7 }
		"*curve*"	{ set labelBg #efefef;  set pressedBg #b4b4b4 }
		"keramik"	{ set labelBg #ededed;  set pressedBg #d6d6d6 }
		"phase"		{ set labelBg #e7e0dd;  set pressedBg #bfb8b7 }
		"plastik"	{ set labelBg #e2e2e2;  set pressedBg #c0bfbf }
		"thinkeramik"	{ set labelBg #e6e1df;  set pressedBg #c7c9c7 }
	    }
	}

	#ffffff {		;# color scheme "High Contrast Black Text"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #ffffff;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {  
		"highcolor"	{ set labelBg #f5f5f5 }
		"sgi"		{ set labelBg #d4d4d4 }
		"dotnet"			      { set pressedBg #a5a5ff }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #d4d4d4 }
		"*curve*"	{ set labelBg #ffffff;  set pressedBg #c9c9c9 }
		"keramik"	{ set labelBg #fbfbfb;  set pressedBg #e8e8e8 }
		"phase"		{ set labelBg #f7f7f7;  set pressedBg #d4d4d4 }
		"plastik"	{ set labelBg #f8f8f8;  set pressedBg #d8d8d8 }
		"thinkeramik"	{ set labelBg #f4f4f4;  set pressedBg #e2e2e2 }
	    }
	}

	#0000ff {		;# color scheme "High Contrast Yellow on Blue"
	    set tableBg #0000ff;  set tableFg #ffff00
	    set labelBg #0000ff;  set labelFg #ffff00;  set pressedBg $labelBg
	    switch -glob -- $style {  
		"highcolor"	{ set labelBg #0e0ef5 }
		"sgi"		{ set labelBg #0000d4 }
		"dotnet"			      { set pressedBg #0000b4 }
		"light, 3rd revision"		      { set pressedBg #1919ff }
		"platinum"			      { set pressedBg #0000d4 }
		"*curve*"	{ set labelBg #0b0bff;  set pressedBg #1515b4 }
		"keramik"	{ set labelBg #4949fb;  set pressedBg #2929e8 }
		"phase"		{ set labelBg #0909f7;  set pressedBg #0000d4 }
		"plastik"	{ set labelBg #0505f8;  set pressedBg #0000d8 }
		"thinkeramik"	{ set labelBg #5151f4;  set pressedBg #2222e2 }
	    }
	}

	#000000 {		;# color scheme "High Contrast White Text"
	    set tableBg #000000;  set tableFg #ffffff
	    set labelBg #000000;  set labelFg #ffffff;  set pressedBg $labelBg
	    switch -glob -- $style {  
		"highcolor"	{ set labelBg #000000 }
		"sgi"		{ set labelBg #000000 }
		"dotnet"			      { set pressedBg #00005a }
		"light, 3rd revision"		      { set pressedBg #000000 }
		"platinum"			      { set pressedBg #000000 }
		"*curve*"	{ set labelBg #000000;  set pressedBg #000000 }
		"keramik"	{ set labelBg #494949;  set pressedBg #292929 }
		"phase"		{ set labelBg #000000;  set pressedBg #000000 }
		"plastik"	{ set labelBg #000000;  set pressedBg #000000 }
		"thinkeramik"	{ set labelBg #4d4d4d;  set pressedBg #222222 }
	    }
	}

	#f6f6ff {		;# color scheme "Ice (FreddyK)"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #e4eeff;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #e8edf5 }
		"sgi"		{ set labelBg #e4eeff }
		"dotnet"			      { set pressedBg #9bd2ff }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #cdcdd4 }
		"*curve*"	{ set labelBg #fbfcff;  set pressedBg #8eb2e0 }
		"keramik"	{ set labelBg #edf3fb;  set pressedBg #d6dde8 }
		"phase"		{ set labelBg #f3f3f7;  set pressedBg #cdcdd4 }
		"plastik"	{ set labelBg #e3eaf8;  set pressedBg #c0c9d8 }
		"thinkeramik"	{ set labelBg #f1f1f4;  set pressedBg #dbdbe2 }
	    }
	}

	#c0c0c0 {		;# color schemes "KDE 1" and "Storm"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #c0c0c0;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #c2c2c2 }
		"sgi"		{ set labelBg #a0a0a0 }
		"dotnet"			      { set pressedBg #000080 }
		"light, 3rd revision"		      { set pressedBg #d3d3d3 }
		"platinum"			      { set pressedBg #a0a0a0 }
		"*curve*"	{ set labelBg #c9c9c9;  set pressedBg #979797 }
		"keramik"	{ set labelBg #d3d3d3;  set pressedBg #bababa }
		"phase"		{ set labelBg #c1c1c1;  set pressedBg #a0a0a0 }
		"plastik"	{ set labelBg #bebebe;  set pressedBg #a2a2a2 }
		"thinkeramik"	{ set labelBg #cccccc;  set pressedBg #b2b2b2 }
	    }
	}

	#dcdcdc {		;# color scheme "KDE 2"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #e4e4e4;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #e7e7e7 }
		"sgi"		{ set labelBg #e4e4e4 }
		"dotnet"			      { set pressedBg #0a5f89 }
		"light, 3rd revision"		      { set pressedBg #d3d3d3 }
		"platinum"			      { set pressedBg #b7b7b7 }
		"*curve*"	{ set labelBg #efefef;  set pressedBg #b4b4b4 }
		"keramik"	{ set labelBg #ededed;  set pressedBg #d6d6d6 }
		"phase"		{ set labelBg #dddddd;  set pressedBg #b7b7b7 }
		"plastik"	{ set labelBg #e2e2e2;  set pressedBg #c0c0c0 }
		"thinkeramik"	{ set labelBg #dfdfdf;  set pressedBg #c7c7c7 }
	    }
	}

	#eae9e8 {		;# color scheme "Keramik"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #e6f0f9;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #eaeef2 }
		"sgi"		{ set labelBg #e6f0f9 }
		"dotnet"			      { set pressedBg #a9d1ff }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c3c2c1 }
		"*curve*"	{ set labelBg #f8fbfe;  set pressedBg #9ebedb }
		"keramik"	{ set labelBg #eef4f8;  set pressedBg #d7dfe5 }
		"phase"		{ set labelBg #ebeae9;  set pressedBg #c3c2c1 }
		"plastik"	{ set labelBg #e3ecf3;  set pressedBg #c0c9d2 }
		"thinkeramik"	{ set labelBg #e8e8e7;  set pressedBg #d2d1d0 }
	    }
	}

	#eeeee6 {		;# color scheme "Keramik Emerald"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #eeeade;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #eeeae1 }
		"sgi"		{ set labelBg #eeeade }
		"dotnet"			      { set pressedBg #86cc86 }
		"light, 3rd revision"		      { set pressedBg #fffffc }
		"platinum"			      { set pressedBg #c6c6bf }
		"*curve*"	{ set labelBg #f6f3ec;  set pressedBg #c7bea3 }
		"keramik"	{ set labelBg #f3f1e8;  set pressedBg #dddad1 }
		"phase"		{ set labelBg #efefef;  set pressedBg #c6c6bf }
		"plastik"	{ set labelBg #ebe7dc;  set pressedBg #c9c6bc }
		"thinkeramik"	{ set labelBg #ebebe5;  set pressedBg #d5d5cf }
	    }
	}

	#e9e9e9 {		;# color scheme "Keramik White"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #f6f6f6;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #f1f1f1 }
		"sgi"		{ set labelBg #f6f6f6 }
		"dotnet"			      { set pressedBg #ffddf6 }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c2c2c2 }
		"*curve*"	{ set labelBg #ffffff;  set pressedBg #c2c2c2 }
		"keramik"	{ set labelBg #f7f7f7;  set pressedBg #e3e3e3 }
		"phase"		{ set labelBg #eaeaea;  set pressedBg #c2c2c2 }
		"plastik"	{ set labelBg #f1f1f1;  set pressedBg #cfcfcf }
		"thinkeramik"	{ set labelBg #e8e8e8;  set pressedBg #d1d1d1 }
	    }
	}

	#f4ddb2 {		;# color scheme "Media Peach"
	    set tableBg #ffe7ba;  set tableFg #000000
	    set labelBg #f4ddb2;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #f0dbb6 }
		"sgi"		{ set labelBg #cbb894 }
		"dotnet"			      { set pressedBg #800000 }
		"light, 3rd revision"		      { set pressedBg #ffebc7 }
		"platinum"			      { set pressedBg #cbb894 }
		"*curve*"	{ set labelBg #f8e5c3;  set pressedBg #dab672 }
		"keramik"	{ set labelBg #f6e8c9;  set pressedBg #e1d0b0 }
		"phase"		{ set labelBg #f4ddb2;  set pressedBg #cbb894 }
		"plastik"	{ set labelBg #ffdbaf;  set pressedBg #d5c19c }
		"thinkeramik"	{ set labelBg #efe0c3;  set pressedBg #d9c8a7 }
	    }
	}

	#a8a8a8 {		;# color scheme "Next"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #a8a8a8;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #aaaaaa }
		"sgi"		{ set labelBg #8c8c8c }
		"dotnet"			      { set pressedBg #000000 }
		"light, 3rd revision"		      { set pressedBg #b8b8b8 }
		"platinum"			      { set pressedBg #8c8c8c }
		"*curve*"	{ set labelBg #b0b0b0;  set pressedBg #848484 }
		"keramik"	{ set labelBg #c2c2c2;  set pressedBg #a8a8a8 }
		"phase"		{ set labelBg #a9a9a9;  set pressedBg #8c8c8c }
		"plastik"	{ set labelBg #a5a5a5;  set pressedBg #898989 }
		"thinkeramik"	{ set labelBg #bdbdbd;  set pressedBg #a0a0a0 }
	    }
	}

	#d6d6d6 {		;# color scheme "Pale Gray"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #d6d6d6;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #d9d9d9 }
		"sgi"		{ set labelBg #b2b2b2 }
		"dotnet"			      { set pressedBg #000000 }
		"light, 3rd revision"		      { set pressedBg #ebebeb }
		"platinum"			      { set pressedBg #b2b2b2 }
		"*curve*"	{ set labelBg #e0e0e0;  set pressedBg #a9a9a9 }
		"keramik"	{ set labelBg #e3e3e3;  set pressedBg #cbcbcb }
		"phase"		{ set labelBg #d6d6d6;  set pressedBg #b2b2b2 }
		"plastik"	{ set labelBg #d3d3d3;  set pressedBg #bababa }
		"thinkeramik"	{ set labelBg #dbdbdb;  set pressedBg #c2c2c2 }
	    }
	}

	#efefef {		;# color scheme "Plastik"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #dddfe4;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #e0e1e7 }
		"sgi"		{ set labelBg #dddfe4 }
		"dotnet"			      { set pressedBg #6784b2 }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c7c7c7 }
		"*curve*"	{ set labelBg #e9eaee;  set pressedBg #aaaeb8 }
		"keramik"	{ set labelBg #e8e9ed;  set pressedBg #d0d2d6 }
		"phase"		{ set labelBg #f0f0f0;  set pressedBg #c7c7c7 }
		"plastik"	{ set labelBg #dbdde2;  set pressedBg #babcc0 }
		"thinkeramik"	{ set labelBg #ececec;  set pressedBg #d6d6d6 }
	    }
	}

	#d3c5be {		;# color scheme "Point Reyes Green"
	    set tableBg #ffffff;  set tableFg #532113
	    set labelBg #aba09a;  set labelFg #264b2c;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #ada29d }
		"sgi"		{ set labelBg #aba09a }
		"dotnet"			      { set pressedBg #417f4b }
		"light, 3rd revision"		      { set pressedBg #e8d9d1 }
		"platinum"			      { set pressedBg #afa49e }
		"*curve*"	{ set labelBg #b2a8a2;  set pressedBg #897d77 }
		"keramik"	{ set labelBg #c4bcb8;  set pressedBg #aba29e }
		"phase"		{ set labelBg #d3c5be;  set pressedBg #afa49e }
		"plastik"	{ set labelBg #ab9f99;  set pressedBg #9b908a }
		"thinkeramik"	{ set labelBg #d9d0cc;  set pressedBg #c0b6b1 }
	    }
	}

	#eed8ae {		;# color scheme "Pumpkin"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #eed8ae;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #eed8b1 }
		"sgi"		{ set labelBg #c6b390 }
		"dotnet"			      { set pressedBg #cd853f }
		"light, 3rd revision"		      { set pressedBg #ffe0c0 }
		"platinum"			      { set pressedBg #c6b390 }
		"*curve*"	{ set labelBg #f2e0bd;  set pressedBg #d1b173 }
		"keramik"	{ set labelBg #f3e4c6;  set pressedBg #ddcdad }
		"phase"		{ set labelBg #eed8ae;  set pressedBg #c6b390 }
		"plastik"	{ set labelBg #ebd5ac;  set pressedBg #cfbc96 }
		"thinkeramik"	{ set labelBg #ebdcc0;  set pressedBg #d5c4a4 }
	    }
	}

	#d4d0c8 {		;# color scheme "Redmond 2000"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #d4d0c8;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #d7d3cb }
		"sgi"		{ set labelBg #b0ada6 }
		"dotnet"			      { set pressedBg #002468 }
		"light, 3rd revision"		      { set pressedBg #e9e5dc }
		"platinum"			      { set pressedBg #b0ada6 }
		"*curve*"	{ set labelBg #ddd9d3;  set pressedBg #aba599 }
		"keramik"	{ set labelBg #e1ded9;  set pressedBg #cac7c1 }
		"phase"		{ set labelBg #d5d1c9;  set pressedBg #b0ada6 }
		"plastik"	{ set labelBg #d2cdc5;  set pressedBg #b2afa7 }
		"thinkeramik"	{ set labelBg #dad7d2;  set pressedBg #c1beb8 }
	    }
	}

	#c3c3c3 {		;# color scheme "Redmond 95"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #c3c3c3;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #c5c5c5 }
		"sgi"		{ set labelBg #a2a2a2 }
		"dotnet"			      { set pressedBg #000080 }
		"light, 3rd revision"		      { set pressedBg #d6d6d6 }
		"platinum"			      { set pressedBg #a2a2a2 }
		"*curve*"	{ set labelBg #cccccc;  set pressedBg #9a9a9a }
		"keramik"	{ set labelBg #d5d5d5;  set pressedBg #bdbdbd }
		"phase"		{ set labelBg #c4c4c4;  set pressedBg #a2a2a2 }
		"plastik"	{ set labelBg #c1c1c1;  set pressedBg #a3a3a3 }
		"thinkeramik"	{ set labelBg #cecece;  set pressedBg #b5b5b5 }
	    }
	}

	#eeeee6 {		;# color scheme "Redmond XP"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #eeeade;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #eeeae1 }
		"sgi"		{ set labelBg #eeeade }
		"dotnet"			      { set pressedBg #4a79cd }
		"light, 3rd revision"		      { set pressedBg #fffffc }
		"platinum"			      { set pressedBg #c6c6bf }
		"*curve*"	{ set labelBg #f6f3ec;  set pressedBg #c7bea3 }
		"keramik"	{ set labelBg #f3f1e8;  set pressedBg #dddad1 }
		"phase"		{ set labelBg #efefe7;  set pressedBg #c6c6bf }
		"plastik"	{ set labelBg #ebe7dc;  set pressedBg #c9c6bc }
		"thinkeramik"	{ set labelBg #ebebe5;  set pressedBg #d5d5cf }
	    }
	}

	#aeb2c3 {		;# color scheme "Solaris"
	    set tableBg #9397a5;  set tableFg #000000
	    set labelBg #aeb2c3;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #b0b4c5 }
		"sgi"		{ set labelBg #9194a2 }
		"dotnet"			      { set pressedBg #718ba5 }
		"light, 3rd revision"		      { set pressedBg #bfc3d6 }
		"platinum"			      { set pressedBg #9194a2 }
		"*curve*"	{ set labelBg #b8bbcb;  set pressedBg #84899e }
		"keramik"	{ set labelBg #c6c9d5;  set pressedBg #adb0bd }
		"phase"		{ set labelBg #aeb2c3;  set pressedBg #9194a2 }
		"plastik"	{ set labelBg #abafc0;  set pressedBg #969aa9 }
		"thinkeramik"	{ set labelBg #c0c3ce;  set pressedBg #a5a7b5 }
	    }
	}

	#eeeaee {		;# color scheme "SuSE" old
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #e6f0f9;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #eaeef2 }
		"sgi"		{ set labelBg #e6f0f9 }
		"dotnet"			      { set pressedBg #447bcd }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c6c3c6 }
		"*curve*"	{ set labelBg #f8fbfe;  set pressedBg #9ebedb }
		"keramik"	{ set labelBg #eef4f8;  set pressedBg #d7dfe5 }
		"phase"		{ set labelBg #efecef;  set pressedBg #c6c3c6 }
		"plastik"	{ set labelBg #e3ecf3;  set pressedBg #c0c9d2 }
		"thinkeramik"	{ set labelBg #ebe8eb;  set pressedBg #d5d2d5 }
	    }
	}

	#eeeeee {		;# color scheme "SuSE" new
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #f4f4f4;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #f0f0f0 }
		"sgi"		{ set labelBg #f4f4f4 }
		"dotnet"			      { set pressedBg #44fbcd }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c6c6c6 }
		"*curve*"	{ set labelBg #ffffff;  set pressedBg #c0c0c0 }
		"keramik"	{ set labelBg #f6f6f6;  set pressedBg #e1e1e1 }
		"phase"		{ set labelBg #efefef;  set pressedBg #c6c6c6 }
		"plastik"	{ set labelBg #f0f0f0;  set pressedBg #cdcdcd }
		"thinkeramik"	{ set labelBg #ebebeb;  set pressedBg #d5d5d5 }
	    }
	}

	#d3d3d3 {		;# color scheme "System"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #d3d3d3;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #d6d6d6 }
		"sgi"		{ set labelBg #afafaf }
		"dotnet"			      { set pressedBg #5a2400 }
		"light, 3rd revision"		      { set pressedBg #e8e8e8 }
		"platinum"			      { set pressedBg #afafaf }
		"*curve*"	{ set labelBg #dddddd;  set pressedBg #a6a6a6 }
		"keramik"	{ set labelBg #e1e1e1;  set pressedBg #c9c9c9 }
		"phase"		{ set labelBg #d2d2d2;  set pressedBg #afafaf }
		"plastik"	{ set labelBg #d0d0d0;  set pressedBg #b9b9b9 }
		"thinkeramik"	{ set labelBg #d9d9d9;  set pressedBg #c0c0c0 }
	    }
	}

	#e6e6de {		;# color scheme "Thin Keramik" old
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #f0f0ef;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #eeeeee }
		"sgi"		{ set labelBg #f0f0ef }
		"dotnet"			      { set pressedBg #88cb88 }
		"light, 3rd revision"		      { set pressedBg #fdfdf4 }
		"platinum"			      { set pressedBg #bfbfb8 }
		"*curve*"	{ set labelBg #fbfbfb;  set pressedBg #bebebb }
		"keramik"	{ set labelBg #f4f4f4;  set pressedBg #dfdfde }
		"phase"		{ set labelBg #Re7e7df  set pressedBg #bfbfb8 }
		"plastik"	{ set labelBg #ededeb;  set pressedBg #cbcbc9 }
		"thinkeramik"	{ set labelBg #e6e6e1;  set pressedBg #cfcfc9 }
	    }
	}

	#edede1 {		;# color scheme "Thin Keramik" new
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #f6f6e9;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #f1f1ec }
		"sgi"		{ set labelBg #f6f6e9 }
		"dotnet"			      { set pressedBg #76884c }
		"light, 3rd revision"		      { set pressedBg #fffff7 }
		"platinum"			      { set pressedBg #c5c5bb }
		"*curve*"	{ set labelBg #fdfdf9;  set pressedBg #d1d1a8 }
		"keramik"	{ set labelBg #f7f7f0;  set pressedBg #e3e3da }
		"phase"		{ set labelBg #edede1;  set pressedBg #c5c5bb }
		"plastik"	{ set labelBg #f4f4e6;  set pressedBg #ddddd0 }
		"thinkeramik"	{ set labelBg #eaeae3;  set pressedBg #d4d4cb }
	    }
	}

	#f6f5e8 {		;# color scheme "Thin Keramik II"
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #eeeade;  set labelFg #000000;  set pressedBg $labelBg
	    switch -glob -- $style {
		"highcolor"	{ set labelBg #eeeae1 }
		"sgi"		{ set labelBg #eeeade }
		"dotnet"			      { set pressedBg #edc967 }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #cdccc1 }
		"*curve*"	{ set labelBg #f6f3ec;  set pressedBg #c7bea3 }
		"keramik"	{ set labelBg #f3f1e8;  set pressedBg #dddad1 }
		"phase"		{ set labelBg #f3f2e9;  set pressedBg #cdccc1 }
		"plastik"	{ set labelBg #ebe7dc;  set pressedBg #c9c6bc }
		"thinkeramik"	{ set labelBg #f1f1e8;  set pressedBg #dbdad0 }
	    }
	}
    }

    #
    # For some Qt styles the colors are independent of the color scheme:
    #
    switch -- $style {
	"acqua" {
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #e7e7e7;  set labelFg #000000;  set pressedBg #8fbeec
	}

	"kde_xp" {
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #ebeadb;  set labelFg #000000;  set pressedBg #faf8f3
	}

	"marble" {
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #cccccc;  set labelFg $fg;      set pressedBg $labelBg
	}

	"riscos" {
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #dddddd;  set labelFg #000000;  set pressedBg $labelBg
	}

	"system" -
	"systemalt" {
	    set tableBg #ffffff;  set tableFg #000000
	    set labelBg #cccccc;  set labelFg #000000;  set pressedBg $labelBg
	}
    }

    #
    # The arrow color and style depend mainly on the current Qt style:
    #
    switch -glob -- $style {
	"*curve*" -
	"dotnet" -
	"highcontrast" -
	"light, 2nd revision" -
	"light, 3rd revision" -
	"phase" -
	"plastik"	{ set arrowColor $labelFg;  set arrowStyle flat7x4 }

	"keramik" -
	"thinkeramik"	{ set arrowColor $labelFg;  set arrowStyle flat8x5 }

	default		{ set arrowColor "";	    set arrowStyle sunken12x11 }
    }

    #
    # The values corresponding to the keys -disabledforeground,
    # -labeldisabledBg, and -labeldisabledFg below are not yet
    # correct, due to the current development state of tile-qt:
    #
    variable themeDefaults
    array set themeDefaults [list \
	-background		$tableBg \
	-foreground		$tableFg \
	-disabledforeground	$tableFg \
	-selectbackground	$selectBg \
	-selectforeground	$selectFg \
	-selectborderwidth	0 \
	-font			TkTextFont \
        -labelbackground	$labelBg \
	-labeldisabledBg	$labelBg \
	-labelactiveBg		$labelBg \
	-labelpressedBg		$pressedBg \
	-labelforeground	$labelFg \
	-labeldisabledFg	$labelFg \
	-labelactiveFg		$labelFg \
	-labelpressedFg		$labelFg \
	-labelfont		TkDefaultFont \
	-labelborderwidth	4 \
	-labelpady		0 \
	-arrowcolor		$arrowColor \
	-arrowstyle		$arrowStyle \
    ]
}

#------------------------------------------------------------------------------
# tablelist::winnativeTheme
#------------------------------------------------------------------------------
proc tablelist::winnativeTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		SystemWindow \
	-foreground		SystemWindowText \
	-disabledforeground	SystemDisabledText \
	-selectbackground	SystemHighlight \
	-selectforeground	SystemHighlightText \
	-selectborderwidth	0 \
	-font			TkTextFont \
        -labelbackground	SystemButtonFace \
	-labeldisabledBg	SystemButtonFace \
	-labelactiveBg		SystemButtonFace \
	-labelpressedBg		SystemButtonFace \
	-labelforeground	SystemButtonText \
	-labeldisabledFg	SystemDisabledText \
	-labelactiveFg		SystemButtonText \
	-labelpressedFg		SystemButtonText \
	-labelfont		TkDefaultFont \
	-labelborderwidth	2 \
	-labelpady		0 \
	-arrowcolor		"" \
	-arrowstyle		sunken8x7 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::winxpblueTheme
#------------------------------------------------------------------------------
proc tablelist::winxpblueTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		white \
	-foreground		black \
	-disabledforeground	#565248 \
	-selectbackground	#4a6984 \
	-selectforeground	#ffffff \
	-selectborderwidth	0 \
	-font			TkTextFont \
        -labelbackground	#ece9d8 \
	-labeldisabledBg	#e3e1dd \
	-labelactiveBg		#c1d2ee \
	-labelpressedBg		#bab5ab \
	-labelforeground	black \
	-labeldisabledFg	#565248 \
	-labelactiveFg		black \
	-labelpressedFg		black \
	-labelfont		TkDefaultFont \
	-labelborderwidth	2 \
	-labelpady		1 \
	-arrowcolor		#aca899 \
	-arrowstyle		flat9x5 \
    ]
}

#------------------------------------------------------------------------------
# tablelist::xpnativeTheme
#------------------------------------------------------------------------------
proc tablelist::xpnativeTheme {} {
    variable xpStyle
    switch [winfo rgb . SystemButtonFace] {
	"60652 59881 55512" {
	    set xpStyle		1
	    set labelBg		#ebeadb
	    set activeBg	#faf8f3
	    set pressedBg	#dedfd8
	    set labelBd		4
	    set labelPadY	4
	    set arrowColor	#aca899
	    set arrowStyle	flat9x5

	    if {[string compare $tile::version 0.7] < 0} {
		set labelBd 0
	    }
	}

	"57568 57311 58339" {
	    set xpStyle		1
	    set labelBg		#f9fafd
	    set activeBg	#fefefe
	    set pressedBg	#ececf3
	    set labelBd		4
	    set labelPadY	4
	    set arrowColor	#aca899
	    set arrowStyle	flat9x5

	    if {[string compare $tile::version 0.7] < 0} {
		set labelBd 0
	    }
	}

	default {
	    set xpStyle		0
	    set labelBg		SystemButtonFace
	    set activeBg	SystemButtonFace
	    set pressedBg	SystemButtonFace
	    set labelBd		2
	    set labelPadY	0
	    set arrowColor	SystemButtonShadow
	    set arrowStyle	flat7x4
	}
    }

    variable themeDefaults
    array set themeDefaults [list \
	-background		SystemWindow \
	-foreground		SystemWindowText \
	-disabledforeground	SystemDisabledText \
	-selectbackground	SystemHighlight \
	-selectforeground	SystemHighlightText \
	-selectborderwidth	0 \
	-font			TkTextFont \
        -labelbackground	$labelBg \
	-labeldisabledBg	$labelBg \
	-labelactiveBg		$activeBg \
	-labelpressedBg		$pressedBg \
	-labelforeground	SystemButtonText \
	-labeldisabledFg	SystemDisabledText \
	-labelactiveFg		SystemButtonText \
	-labelpressedFg		SystemButtonText \
	-labelfont		TkDefaultFont \
	-labelborderwidth	$labelBd \
	-labelpady		$labelPadY \
	-arrowcolor		$arrowColor \
	-arrowstyle		$arrowStyle \
    ]
}
