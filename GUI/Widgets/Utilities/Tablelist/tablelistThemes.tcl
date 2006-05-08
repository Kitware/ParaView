#==============================================================================
# Contains procedures that populate the array themeDefaults with theme-specific
# values of some tablelist configuration options.
#
# Copyright (c) 2005-2006  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================

#------------------------------------------------------------------------------
# tablelist::setThemeDefaults
#
# Populates the array themeDefaults with theme-specific values of some
# tablelist configuration options.
#------------------------------------------------------------------------------
proc tablelist::setThemeDefaults {} {
    if {[catch {${tile::currentTheme}Theme}] != 0} {
	return -code error "theme \"$tile::currentTheme\" not supported"
    }
}

#------------------------------------------------------------------------------
# tablelist::altTheme
#------------------------------------------------------------------------------
proc tablelist::altTheme {} {
    variable themeDefaults
    array set themeDefaults [list \
	-background		white \
	-foreground		black \
	-disabledforeground	#a3a3a3 \
	-stripebackground	"" \
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
	-stripebackground	"" \
	-selectbackground	systemHighlight \
	-selectforeground	systemHighlightText \
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
	-stripebackground	"" \
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
	-stripebackground	"" \
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
	-stripebackground	"" \
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
	-stripebackground	"" \
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
	-stripebackground	"" \
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
	-stripebackground	"" \
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
	-stripebackground	"" \
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
	-stripebackground	"" \
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
	-stripebackground	"" \
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
	-stripebackground	"" \
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
	-stripebackground	"" \
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
#   .NET style	       KDE Classic		  Motif		 QtCurve V2
#   Acqua	       KDE_XP			  Motif Plus	 QtCurve V3
#   B3/KDE	       Keramik			  MS Windows 9x	 RISC OS
#   Baghira	       Light Style, 2nd revision  Phase		 SGI
#   CDE		       Light Style, 3rd revision  Plastik	 System-Series
#   HighColor Classic  Lipstik			  Platinum	 System++
#   HighContrast       Marble			  QtCurve	 ThinKeramik
#
# Supported color schemes:
#
#   Aqua Blue			  Ice (FreddyK)	     Point Reyes Green
#   Aqua Graphite		  KDE 1		     Pumpkin
#   Atlas Green			  KDE 2		     Redmond 2000
#   BeOS			  Keramik	     Redmond 95
#   Blue Slate			  Keramik Emerald    Redmond XP
#   CDE				  Keramik White	     Solaris
#   Dark Blue			  Lipstik Noble	     Storm
#   Desert Red			  Lipstik Standard   SuSE, old & new
#   Digital CDE			  Lipstik White	     SUSE-kdm
#   EveX			  Media Peach	     System
#   High Contrast Black Text	  Next		     Thin Keramik, old & new
#   High Contrast Yellow on Blue  Pale Gray	     Thin Keramik II
#   High Contrast White Text	  Plastik
#------------------------------------------------------------------------------
proc tablelist::tileqtTheme {} {
    set bg         [tile::theme::tileqt::currentThemeColour -background]
    set fg         [tile::theme::tileqt::currentThemeColour -foreground]
    set tableBg    [tile::theme::tileqt::currentThemeColour -base]
    set tableFg    [tile::theme::tileqt::currentThemeColour -text]
    set tableDisFg [tile::theme::tileqt::currentThemeColour -disabled -text]
    set selectBg   [tile::theme::tileqt::currentThemeColour -highlight]
    set selectFg   [tile::theme::tileqt::currentThemeColour -highlightedText]
    set labelBg    [tile::theme::tileqt::currentThemeColour -button]
    set labelFg    [tile::theme::tileqt::currentThemeColour -buttonText]
    set labelDisFg [tile::theme::tileqt::currentThemeColour -disabled \
		    -buttonText]
    set style      [string tolower [tile::theme::tileqt::currentThemeName]]
    set pressedBg  $labelBg

    #
    # For most Qt styles the label colors depend on the color scheme:
    #
    switch "$bg $labelBg" {
	"#fafafa #6188d7" {	;# color scheme "Aqua Blue"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #396db5 }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #d0d0d0 }
		"*curve*"	{ set labelBg #6a90dc;  set pressedBg #4167b4 }
		"baghira"	{ set labelBg #f5f5f5;  set pressedBg #9ec2fa }
		"highcolor"	{ set labelBg #628ada;  set pressedBg #6188d7 }
		"keramik"	{ set labelBg #8fabe4;  set pressedBg #7390cc }
		"phase"		{ set labelBg #6188d7;  set pressedBg #d0d0d0 }
		"plastik"	{ set labelBg #666bd6;  set pressedBg #5c7ec2 }
		"thinkeramik"	{ set labelBg #f4f4f4;  set pressedBg #dedede }
	    }
	}

	"#ffffff #89919b" {	;# color scheme "Aqua Graphite"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #68798c }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #d4d4d4 }
		"*curve*"	{ set labelBg #9098a2;  set pressedBg #6b727a }
		"baghira"	{ set labelBg #f5f5f5;  set pressedBg #c3c7cd }
		"highcolor"	{ set labelBg #8b949e;  set pressedBg #89919b }
		"keramik"	{ set labelBg #acb1b8;  set pressedBg #91979e }
		"phase"		{ set labelBg #89919b;  set pressedBg #d4d4d4 }
		"plastik"	{ set labelBg #8c949d;  set pressedBg #7f868e }
		"thinkeramik"	{ set labelBg #f4f4f4;  set pressedBg #e2e2e2 }
	    }
	}

	"#afb49f #afb49f" {	;# color scheme "Atlas Green"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #6f7a63 }
		"light, 3rd revision"		      { set pressedBg #c1c6af }
		"platinum"			      { set pressedBg #929684 }
		"*curve*"	{ set labelBg #b7bba8;  set pressedBg #8c917a }
		"baghira"	{ set labelBg #e5e8dc;  set pressedBg #dadcd0 }
		"highcolor"	{ set labelBg #b2b6a1;  set pressedBg #afb49f }
		"keramik"	{ set labelBg #c7cabb;  set pressedBg #adb1a1 }
		"phase"		{ set labelBg #a7b49f;  set pressedBg #929684 }
		"plastik"	{ set labelBg #acb19c;  set pressedBg #959987 }
		"thinkeramik"	{ set labelBg #c1c4b6;  set pressedBg #a5a999 }
	    }
	}

	"#d9d9d9 #d9d9d9" {	;# color scheme "BeOS"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #a8a8a8 }
		"light, 3rd revision"		      { set pressedBg #eeeeee }
		"platinum"			      { set pressedBg #b4b4b4 }
		"*curve*"	{ set labelBg #e3e3e3;  set pressedBg #ababab }
		"baghira"	{ set labelBg #f2f2f2;  set pressedBg #e9e9e9 }
		"highcolor"	{ set labelBg #dcdcdc;  set pressedBg #d9d9d9 }
		"keramik"	{ set labelBg #e5e5e5;  set pressedBg #cdcdcd }
		"phase"		{ set labelBg #dadada;  set pressedBg #b4b4b4 }
		"plastik"	{ set labelBg #d6d6d6;  set pressedBg #b6b6b6 }
		"thinkeramik"	{ set labelBg #dddddd;  set pressedBg #c5c5c5 }
	    }
	}

	"#9db9c8 #9db9c8" {	;# color scheme "Blue Slate"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #558097 }
		"light, 3rd revision"		      { set pressedBg #adcbdc }
		"platinum"			      { set pressedBg #8299a6 }
		"*curve*"	{ set labelBg #a7c1cf;  set pressedBg #7394a6 }
		"baghira"	{ set labelBg #ddeff6;  set pressedBg #d0e1ea }
		"highcolor"	{ set labelBg #9fbbcb;  set pressedBg #9db9c8 }
		"keramik"	{ set labelBg #baced9;  set pressedBg #a0b5c1 }
		"phase"		{ set labelBg #9db9c9;  set pressedBg #8299a6 }
		"plastik"	{ set labelBg #99b6c5;  set pressedBg #869fab }
		"thinkeramik"	{ set labelBg #b5c8d2;  set pressedBg #98adb8 }
	    }
	}

	"#999999 #999999" {	;# color scheme "CDE"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #326284 }
		"light, 3rd revision"		      { set pressedBg #a8a8a8 }
		"platinum"			      { set pressedBg #7f7f7f }
		"*curve*"	{ set labelBg #a0a0a0;  set pressedBg #787878 }
		"baghira"	{ set labelBg #d5d5d5;  set pressedBg #cccccc }
		"highcolor"	{ set labelBg #9b9b9b;  set pressedBg #999999 }
		"keramik"	{ set labelBg #b7b7b7;  set pressedBg #9d9d9d }
		"phase"		{ set labelBg #999999;  set pressedBg #7f7f7f }
		"plastik"	{ set labelBg #979797;  set pressedBg #808080 }
		"thinkeramik"	{ set labelBg #b3b3b3;  set pressedBg #959595 }
	    }
	}

	"#426794 #426794" {	;# color scheme "Dark Blue"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #5cb3ff }
		"light, 3rd revision"		      { set pressedBg #4871a2 }
		"platinum"			      { set pressedBg #37567b }
		"*curve*"	{ set labelBg #436b9d;  set pressedBg #3a526e }
		"baghira"	{ set labelBg #8aafdc;  set pressedBg #82a3cc }
		"highcolor"	{ set labelBg #436895;  set pressedBg #426794 }
		"keramik"	{ set labelBg #7994b4;  set pressedBg #5b7799 }
		"phase"		{ set labelBg #426795;  set pressedBg #37567b }
		"plastik"	{ set labelBg #406592;  set pressedBg #36547a }
		"thinkeramik"	{ set labelBg #7991af;  set pressedBg #546f91 }
	    }
	}

	"#d6cdbb #d6cdbb" {	;# color scheme "Desert Red"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #800000 }
		"light, 3rd revision"		      { set pressedBg #ebe1ce }
		"platinum"			      { set pressedBg #b2ab9c }
		"*curve*"	{ set labelBg #ded6c6;  set pressedBg #b1a48b }
		"baghira"	{ set labelBg #f7f4ec;  set pressedBg #edeae0 }
		"highcolor"	{ set labelBg #d9d0be;  set pressedBg #d6cdbb }
		"keramik"	{ set labelBg #e3dcd0;  set pressedBg #cbc5b7 }
		"phase"		{ set labelBg #d6cdbb;  set pressedBg #b2ab9c }
		"plastik"	{ set labelBg #d3cbb8;  set pressedBg #bab3a3 }
		"thinkeramik"	{ set labelBg #dbd5ca;  set pressedBg #c2bbae }
	    }
	}

	"#4b7b82 #4b7b82" {	;# color scheme "Digital CDE"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #526674 }
		"light, 3rd revision"		      { set pressedBg #52878f }
		"platinum"			      { set pressedBg #3e666c }
		"*curve*"	{ set labelBg #4d8289;  set pressedBg #3f5d62 }
		"baghira"	{ set labelBg #97c3c9;  set pressedBg #8eb6bc }
		"highcolor"	{ set labelBg #4b7d84;  set pressedBg #4b7b82 }
		"keramik"	{ set labelBg #80a2a7;  set pressedBg #62868c }
		"phase"		{ set labelBg #4b7b82;  set pressedBg #3e666c }
		"plastik"	{ set labelBg #49787f;  set pressedBg #3d666c }
		"thinkeramik"	{ set labelBg #7f97a3;  set pressedBg #5a7e83 }
	    }
	}

	"#e6dedc #e4e4e4" {	;# color scheme "EveX"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #0a5f89 }
		"light, 3rd revision"		      { set pressedBg #fdf4f2 }
		"platinum"			      { set pressedBg #bfb8b7 }
		"*curve*"	{ set labelBg #efefef;  set pressedBg #b4b4b4 }
		"baghira"	{ set labelBg #f6f5f5;  set pressedBg #ededed }
		"highcolor"	{ set labelBg #e7e7e7;  set pressedBg #e4e4e4 }
		"keramik"	{ set labelBg #ededed;  set pressedBg #d6d6d6 }
		"phase"		{ set labelBg #e7e0dd;  set pressedBg #bfb8b7 }
		"plastik"	{ set labelBg #e2e2e2;  set pressedBg #c0bfbf }
		"thinkeramik"	{ set labelBg #e6e1df;  set pressedBg #c7c9c7 }
	    }
	}

	"#ffffff #ffffff" {	;# color scheme "High Contrast Black Text"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #a5a5ff }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #d4d4d4 }
		"*curve*"	{ set labelBg #ffffff;  set pressedBg #c9c9c9 }
		"baghira"	{ set labelBg #f5f5f5;  set pressedBg #f2f2f2 }
		"highcolor"	{ set labelBg #f5f5f5;  set pressedBg #ffffff }
		"keramik"	{ set labelBg #fbfbfb;  set pressedBg #e8e8e8 }
		"phase"		{ set labelBg #f7f7f7;  set pressedBg #d4d4d4 }
		"plastik"	{ set labelBg #f8f8f8;  set pressedBg #d8d8d8 }
		"thinkeramik"	{ set labelBg #f4f4f4;  set pressedBg #e2e2e2 }
	    }
	}

	"#0000ff #0000ff" {	;# color scheme "High Contrast Yellow on Blue"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #0000b4 }
		"light, 3rd revision"		      { set pressedBg #1919ff }
		"platinum"			      { set pressedBg #0000d4 }
		"*curve*"	{ set labelBg #0b0bff;  set pressedBg #1515b4 }
		"baghira"	{ set labelBg #4848ff;  set pressedBg #4646ff }
		"highcolor"	{ set labelBg #0e0ef5;  set pressedBg #0000ff }
		"keramik"	{ set labelBg #4949fb;  set pressedBg #2929e8 }
		"phase"		{ set labelBg #0909f7;  set pressedBg #0000d4 }
		"plastik"	{ set labelBg #0505f8;  set pressedBg #0000d8 }
		"thinkeramik"	{ set labelBg #5151f4;  set pressedBg #2222e2 }
	    }
	}

	"#000000 #000000" {	;# color scheme "High Contrast White Text"
	    switch -glob -- $style {  
		"dotnet"			      { set pressedBg #00005a }
		"light, 3rd revision"		      { set pressedBg #000000 }
		"platinum"			      { set pressedBg #000000 }
		"*curve*"	{ set labelBg #000000;  set pressedBg #000000 }
		"baghira"	{ set labelBg #818181;  set pressedBg #7f7f7f }
		"highcolor"	{ set labelBg #000000;  set pressedBg #000000 }
		"keramik"	{ set labelBg #494949;  set pressedBg #292929 }
		"phase"		{ set labelBg #000000;  set pressedBg #000000 }
		"plastik"	{ set labelBg #000000;  set pressedBg #000000 }
		"thinkeramik"	{ set labelBg #4d4d4d;  set pressedBg #222222 }
	    }
	}

	"#f6f6ff #e4eeff" {	;# color scheme "Ice (FreddyK)"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #9bd2ff }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #cdcdd4 }
		"*curve*"	{ set labelBg #fbfcff;  set pressedBg #8eb2e0 }
		"baghira"	{ set labelBg #f6f6f6;  set pressedBg #f2f4f6 }
		"highcolor"	{ set labelBg #e8edf5;  set pressedBg #e4eeff }
		"keramik"	{ set labelBg #edf3fb;  set pressedBg #d6dde8 }
		"phase"		{ set labelBg #f3f3f7;  set pressedBg #cdcdd4 }
		"plastik"	{ set labelBg #e3eaf8;  set pressedBg #c0c9d8 }
		"thinkeramik"	{ set labelBg #f1f1f4;  set pressedBg #dbdbe2 }
	    }
	}

	"#c0c0c0 #c0c0c0" {	;# color schemes "KDE 1" and "Storm"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #000080 }
		"light, 3rd revision"		      { set pressedBg #d3d3d3 }
		"platinum"			      { set pressedBg #a0a0a0 }
		"*curve*"	{ set labelBg #c9c9c9;  set pressedBg #979797 }
		"baghira"	{ set labelBg #e9e9e9;  set pressedBg #dedede }
		"highcolor"	{ set labelBg #c2c2c2;  set pressedBg #c0c0c0 }
		"keramik"	{ set labelBg #d3d3d3;  set pressedBg #bababa }
		"phase"		{ set labelBg #c1c1c1;  set pressedBg #a0a0a0 }
		"plastik"	{ set labelBg #bebebe;  set pressedBg #a2a2a2 }
		"thinkeramik"	{ set labelBg #cccccc;  set pressedBg #b2b2b2 }
	    }
	}

	"#dcdcdc #e4e4e4" {	;# color scheme "KDE 2"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #0a5f89 }
		"light, 3rd revision"		      { set pressedBg #d3d3d3 }
		"platinum"			      { set pressedBg #b7b7b7 }
		"*curve*"	{ set labelBg #efefef;  set pressedBg #b4b4b4 }
		"baghira"	{ set labelBg #f3f3f3;  set pressedBg #ededed }
		"highcolor"	{ set labelBg #e7e7e7;  set pressedBg #e4e4e4 }
		"keramik"	{ set labelBg #ededed;  set pressedBg #d6d6d6 }
		"phase"		{ set labelBg #dddddd;  set pressedBg #b7b7b7 }
		"plastik"	{ set labelBg #e2e2e2;  set pressedBg #c0c0c0 }
		"thinkeramik"	{ set labelBg #dfdfdf;  set pressedBg #c7c7c7 }
	    }
	}

	"#eae9e8 #e6f0f9" {	;# color scheme "Keramik"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #a9d1ff }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c3c2c1 }
		"*curve*"	{ set labelBg #f8fbfe;  set pressedBg #9ebedb }
		"baghira"	{ set labelBg #f4f4f4;  set pressedBg #f1f3f5 }
		"highcolor"	{ set labelBg #eaeef2;  set pressedBg #e6f0f9 }
		"keramik"	{ set labelBg #eef4f8;  set pressedBg #d7dfe5 }
		"phase"		{ set labelBg #ebeae9;  set pressedBg #c3c2c1 }
		"plastik"	{ set labelBg #e3ecf3;  set pressedBg #c0c9d2 }
		"thinkeramik"	{ set labelBg #e8e8e7;  set pressedBg #d2d1d0 }
	    }
	}

	"#eeeee6 #eeeade" {	;# color scheme "Keramik Emerald"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #86cc86 }
		"light, 3rd revision"		      { set pressedBg #fffffc }
		"platinum"			      { set pressedBg #c6c6bf }
		"*curve*"	{ set labelBg #f6f3ec;  set pressedBg #c7bea3 }
		"baghira"	{ set labelBg #f6f6f6;  set pressedBg #f3f2ee }
		"highcolor"	{ set labelBg #eeeae1;  set pressedBg #eeeade }
		"keramik"	{ set labelBg #f3f1e8;  set pressedBg #dddad1 }
		"phase"		{ set labelBg #efefef;  set pressedBg #c6c6bf }
		"plastik"	{ set labelBg #ebe7dc;  set pressedBg #c9c6bc }
		"thinkeramik"	{ set labelBg #ebebe5;  set pressedBg #d5d5cf }
	    }
	}

	"#e9e9e9 #f6f6f6" {	;# color scheme "Keramik White"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #ffddf6 }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c2c2c2 }
		"*curve*"	{ set labelBg #ffffff;  set pressedBg #c2c2c2 }
		"baghira"	{ set labelBg #f4f4f4;  set pressedBg #f1f1f1 }
		"highcolor"	{ set labelBg #f1f1f1;  set pressedBg #f6f6f6 }
		"keramik"	{ set labelBg #f7f7f7;  set pressedBg #e3e3e3 }
		"phase"		{ set labelBg #eaeaea;  set pressedBg #c2c2c2 }
		"plastik"	{ set labelBg #f1f1f1;  set pressedBg #cfcfcf }
		"thinkeramik"	{ set labelBg #e8e8e8;  set pressedBg #d1d1d1 }
	    }
	}

	"#ebe9e9 #f6f4f4" {	;# color scheme "Lipstik Noble"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #5186e1 }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c3c1c1 }
		"*curve*"	{ set labelBg #ffffff;  set pressedBg #c6bdbd }
		"baghira"	{ set labelBg #f4f4f4;  set pressedBg #f1f1f1 }
		"highcolor"	{ set labelBg #f1f0f0;  set pressedBg #f6f4f4 }
		"keramik"	{ set labelBg #f7f6f6;  set pressedBg #e3e1e1 }
		"phase"		{ set labelBg #f5f4f4;  set pressedBg #c3c1c1 }
		"plastik"	{ set labelBg #f2f2f2;  set pressedBg #d3d2d2 }
		"thinkeramik"	{ set labelBg #e9e8e8;  set pressedBg #d3d1d1 }
	    }
	}

	"#eeeee6 #eeeade" {	;# color scheme "Lipstik Standard"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #5c8ffd }
		"light, 3rd revision"		      { set pressedBg #fffffc }
		"platinum"			      { set pressedBg #c6c6bf }
		"*curve*"	{ set labelBg #f6f3ec;  set pressedBg #c7bea3 }
		"baghira"	{ set labelBg #f6f6f6;  set pressedBg #f3f2ee }
		"highcolor"	{ set labelBg #eeeae1;  set pressedBg #eeeade }
		"keramik"	{ set labelBg #f3f1e8;  set pressedBg #dddad1 }
		"phase"		{ set labelBg #eeeade;  set pressedBg #c6c6bf }
		"plastik"	{ set labelBg #ebe7dc;  set pressedBg #ccc9c0 }
		"thinkeramik"	{ set labelBg #ebebe5;  set pressedBg #d5d5cf }
	    }
	}

	"#eeeff2 #f7faff" {	;# color scheme "Lipstik White"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #649aff }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c6c7c9 }
		"*curve*"	{ set labelBg #ffffff;  set pressedBg #a1bdea }
		"baghira"	{ set labelBg #f5f5f5;  set pressedBg #f2f2f3 }
		"highcolor"	{ set labelBg #f1f2f5;  set pressedBg #f1faff }
		"keramik"	{ set labelBg #f8f9fb;  set pressedBg #e3e5e8 }
		"phase"		{ set labelBg #f4f5f7;  set pressedBg #c6c7c9 }
		"plastik"	{ set labelBg #f3f4f7;  set pressedBg #d0d3d8 }
		"thinkeramik"	{ set labelBg #ebecee;  set pressedBg #d5d6d8 }
	    }
	}

	"#f4ddb2 #f4ddb2" {	;# color scheme "Media Peach"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #800000 }
		"light, 3rd revision"		      { set pressedBg #ffebc7 }
		"platinum"			      { set pressedBg #cbb894 }
		"*curve*"	{ set labelBg #f8e5c3;  set pressedBg #dab672 }
		"baghira"	{ set labelBg #fcfced;  set pressedBg #faf6df }
		"highcolor"	{ set labelBg #f0dbb6;  set pressedBg #f4ddb2 }
		"keramik"	{ set labelBg #f6e8c9;  set pressedBg #e1d0b0 }
		"phase"		{ set labelBg #f4ddb2;  set pressedBg #cbb894 }
		"plastik"	{ set labelBg #ffdbaf;  set pressedBg #d5c19c }
		"thinkeramik"	{ set labelBg #efe0c3;  set pressedBg #d9c8a7 }
	    }
	}

	"#a8a8a8 #a8a8a8" {	;# color scheme "Next"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #000000 }
		"light, 3rd revision"		      { set pressedBg #b8b8b8 }
		"platinum"			      { set pressedBg #8c8c8c }
		"*curve*"	{ set labelBg #b0b0b0;  set pressedBg #848484 }
		"baghira"	{ set labelBg #dedede;  set pressedBg #d3d3d3 }
		"highcolor"	{ set labelBg #aaaaaa;  set pressedBg #a8a8a8 }
		"keramik"	{ set labelBg #c2c2c2;  set pressedBg #a8a8a8 }
		"phase"		{ set labelBg #a9a9a9;  set pressedBg #8c8c8c }
		"plastik"	{ set labelBg #a5a5a5;  set pressedBg #898989 }
		"thinkeramik"	{ set labelBg #bdbdbd;  set pressedBg #a0a0a0 }
	    }
	}

	"#d6d6d6 #d6d6d6" {	;# color scheme "Pale Gray"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #000000 }
		"light, 3rd revision"		      { set pressedBg #ebebeb }
		"platinum"			      { set pressedBg #b2b2b2 }
		"*curve*"	{ set labelBg #e0e0e0;  set pressedBg #a9a9a9 }
		"baghira"	{ set labelBg #f2f2f2;  set pressedBg #e8e8e8 }
		"highcolor"	{ set labelBg #d9d9d9;  set pressedBg #d6d6d6 }
		"keramik"	{ set labelBg #e3e3e3;  set pressedBg #cbcbcb }
		"phase"		{ set labelBg #d6d6d6;  set pressedBg #b2b2b2 }
		"plastik"	{ set labelBg #d3d3d3;  set pressedBg #bababa }
		"thinkeramik"	{ set labelBg #dbdbdb;  set pressedBg #c2c2c2 }
	    }
	}

	"#efefef #dddfe4" {	;# color scheme "Plastik"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #6784b2 }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c7c7c7 }
		"*curve*"	{ set labelBg #e9eaee;  set pressedBg #aaaeb8 }
		"baghira"	{ set labelBg #f5f5f5;  set pressedBg #ececee }
		"highcolor"	{ set labelBg #e0e1e7;  set pressedBg #dddfe4 }
		"keramik"	{ set labelBg #e8e9ed;  set pressedBg #d0d2d6 }
		"phase"		{ set labelBg #dee0e5;  set pressedBg #c7c7c7 }
		"plastik"	{ set labelBg #dbdde2;  set pressedBg #babcc0 }
		"thinkeramik"	{ set labelBg #ececec;  set pressedBg #d6d6d6 }
	    }
	}

	"#d3c5be #aba09a" {	;# color scheme "Point Reyes Green"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #417f4b }
		"light, 3rd revision"		      { set pressedBg #e8d9d1 }
		"platinum"			      { set pressedBg #afa49e }
		"*curve*"	{ set labelBg #b2a8a2;  set pressedBg #897d77 }
		"baghira"	{ set labelBg #f5efed;  set pressedBg #d7d0cd }
		"highcolor"	{ set labelBg #ada29d;  set pressedBg #aba09a }
		"keramik"	{ set labelBg #c4bcb8;  set pressedBg #aba29e }
		"phase"		{ set labelBg #d3c5be;  set pressedBg #afa49e }
		"plastik"	{ set labelBg #ab9f99;  set pressedBg #9b908a }
		"thinkeramik"	{ set labelBg #d9d0cc;  set pressedBg #c0b6b1 }
	    }
	}

	"#eed8ae #eed8ae" {	;# color scheme "Pumpkin"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #cd853f }
		"light, 3rd revision"		      { set pressedBg #ffe0c0 }
		"platinum"			      { set pressedBg #c6b390 }
		"*curve*"	{ set labelBg #f2e0bd;  set pressedBg #d1b173 }
		"baghira"	{ set labelBg #fcfbea;  set pressedBg #f9f4dd }
		"highcolor"	{ set labelBg #eed8b1;  set pressedBg #eed8ae }
		"keramik"	{ set labelBg #f3e4c6;  set pressedBg #ddcdad }
		"phase"		{ set labelBg #eed8ae;  set pressedBg #c6b390 }
		"plastik"	{ set labelBg #ebd5ac;  set pressedBg #cfbc96 }
		"thinkeramik"	{ set labelBg #ebdcc0;  set pressedBg #d5c4a4 }
	    }
	}

	"#d4d0c8 #d4d0c8" {	;# color scheme "Redmond 2000"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #002468 }
		"light, 3rd revision"		      { set pressedBg #e9e5dc }
		"platinum"			      { set pressedBg #b0ada6 }
		"*curve*"	{ set labelBg #ddd9d3;  set pressedBg #aba599 }
		"baghira"	{ set labelBg #f3f2ef;  set pressedBg #eae8e4 }
		"highcolor"	{ set labelBg #d7d3cb;  set pressedBg #d4d0c8 }
		"keramik"	{ set labelBg #e1ded9;  set pressedBg #cac7c1 }
		"phase"		{ set labelBg #d5d1c9;  set pressedBg #b0ada6 }
		"plastik"	{ set labelBg #d2cdc5;  set pressedBg #b2afa7 }
		"thinkeramik"	{ set labelBg #dad7d2;  set pressedBg #c1beb8 }
	    }
	}

	"#c3c3c3 #c3c3c3" {	;# color scheme "Redmond 95"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #000080 }
		"light, 3rd revision"		      { set pressedBg #d6d6d6 }
		"platinum"			      { set pressedBg #a2a2a2 }
		"*curve*"	{ set labelBg #cccccc;  set pressedBg #9a9a9a }
		"baghira"	{ set labelBg #eaeaea;  set pressedBg #dfdfdf }
		"highcolor"	{ set labelBg #c5c5c5;  set pressedBg #c3c3c3 }
		"keramik"	{ set labelBg #d5d5d5;  set pressedBg #bdbdbd }
		"phase"		{ set labelBg #c4c4c4;  set pressedBg #a2a2a2 }
		"plastik"	{ set labelBg #c1c1c1;  set pressedBg #a3a3a3 }
		"thinkeramik"	{ set labelBg #cecece;  set pressedBg #b5b5b5 }
	    }
	}

	"#eeeee6 #eeeade" {	;# color scheme "Redmond XP"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #4a79cd }
		"light, 3rd revision"		      { set pressedBg #fffffc }
		"platinum"			      { set pressedBg #c6c6bf }
		"*curve*"	{ set labelBg #f6f3ec;  set pressedBg #c7bea3 }
		"baghira"	{ set labelBg #f6f6f6;  set pressedBg #f3f2ee }
		"highcolor"	{ set labelBg #eeeae1;  set pressedBg #eeeade }
		"keramik"	{ set labelBg #f3f1e8;  set pressedBg #dddad1 }
		"phase"		{ set labelBg #efefe7;  set pressedBg #c6c6bf }
		"plastik"	{ set labelBg #ebe7dc;  set pressedBg #c9c6bc }
		"thinkeramik"	{ set labelBg #ebebe5;  set pressedBg #d5d5cf }
	    }
	}

	"#aeb2c3 #aeb2c3" {	;# color scheme "Solaris"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #718ba5 }
		"light, 3rd revision"		      { set pressedBg #bfc3d6 }
		"platinum"			      { set pressedBg #9194a2 }
		"*curve*"	{ set labelBg #b8bbcb;  set pressedBg #84899e }
		"baghira"	{ set labelBg #e4e7ef;  set pressedBg #d9dbe4 }
		"highcolor"	{ set labelBg #b0b4c5;  set pressedBg #aeb2c3 }
		"keramik"	{ set labelBg #c6c9d5;  set pressedBg #adb0bd }
		"phase"		{ set labelBg #aeb2c3;  set pressedBg #9194a2 }
		"plastik"	{ set labelBg #abafc0;  set pressedBg #969aa9 }
		"thinkeramik"	{ set labelBg #c0c3ce;  set pressedBg #a5a7b5 }
	    }
	}

	"#eeeaee #e6f0f9" {	;# color scheme "SuSE" old
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #447bcd }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c6c3c6 }
		"*curve*"	{ set labelBg #f8fbfe;  set pressedBg #9ebedb }
		"baghira"	{ set labelBg #f5f5f5;  set pressedBg #f1f3f5 }
		"highcolor"	{ set labelBg #eaeef2;  set pressedBg #e6f0f9 }
		"keramik"	{ set labelBg #eef4f8;  set pressedBg #d7dfe5 }
		"phase"		{ set labelBg #efecef;  set pressedBg #c6c3c6 }
		"plastik"	{ set labelBg #e3ecf3;  set pressedBg #c0c9d2 }
		"thinkeramik"	{ set labelBg #ebe8eb;  set pressedBg #d5d2d5 }
	    }
	}

	"#eeeeee #f4f4f4" {	;# color scheme "SuSE" new
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #44fbcd }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c6c6c6 }
		"*curve*"	{ set labelBg #ffffff;  set pressedBg #c0c0c0 }
		"baghira"	{ set labelBg #f5f5f5;  set pressedBg #f1f1f1 }
		"highcolor"	{ set labelBg #f0f0f0;  set pressedBg #f4f4f4 }
		"keramik"	{ set labelBg #f6f6f6;  set pressedBg #e1e1e1 }
		"phase"		{ set labelBg #efefef;  set pressedBg #c6c6c6 }
		"plastik"	{ set labelBg #f0f0f0;  set pressedBg #cdcdcd }
		"thinkeramik"	{ set labelBg #ebebeb;  set pressedBg #d5d5d5 }
	    }
	}

	"#eaeaea #eaeaea" {	;# color scheme "SUSE-kdm"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #2863bb }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #c3c3c3 }
		"*curve*"	{ set labelBg #f5f5f5;  set pressedBg #b8b8b8 }
		"baghira"	{ set labelBg #f4f4f4;  set pressedBg #efefef }
		"highcolor"	{ set labelBg #ececec;  set pressedBg #eaeaea }
		"keramik"	{ set labelBg #f1f1f1;  set pressedBg #dadada }
		"phase"		{ set labelBg #ebebeb;  set pressedBg #c3c3c3 }
		"plastik"	{ set labelBg #e7e7e7;  set pressedBg #c6c6c6 }
		"thinkeramik"	{ set labelBg #e8e8e8;  set pressedBg #d2d2d2 }
	    }
	}

	"#d3d3d3 #d3d3d3" {	;# color scheme "System"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #5a2400 }
		"light, 3rd revision"		      { set pressedBg #e8e8e8 }
		"platinum"			      { set pressedBg #afafaf }
		"*curve*"	{ set labelBg #dddddd;  set pressedBg #a6a6a6 }
		"baghira"	{ set labelBg #f0f0f0;  set pressedBg #e6e6e6 }
		"highcolor"	{ set labelBg #d6d6d6;  set pressedBg #d3d3d3 }
		"keramik"	{ set labelBg #e1e1e1;  set pressedBg #c9c9c9 }
		"phase"		{ set labelBg #d2d2d2;  set pressedBg #afafaf }
		"plastik"	{ set labelBg #d0d0d0;  set pressedBg #b9b9b9 }
		"thinkeramik"	{ set labelBg #d9d9d9;  set pressedBg #c0c0c0 }
	    }
	}

	"#e6e6de #f0f0ef" {	;# color scheme "Thin Keramik" old
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #88cb88 }
		"light, 3rd revision"		      { set pressedBg #fdfdf4 }
		"platinum"			      { set pressedBg #bfbfb8 }
		"*curve*"	{ set labelBg #fbfbfb;  set pressedBg #bebebb }
		"baghira"	{ set labelBg #f6f6f5;  set pressedBg #f0f0f0 }
		"highcolor"	{ set labelBg #eeeeee;  set pressedBg #f0f0ef }
		"keramik"	{ set labelBg #f4f4f4;  set pressedBg #dfdfde }
		"phase"		{ set labelBg #Re7e7df  set pressedBg #bfbfb8 }
		"plastik"	{ set labelBg #ededeb;  set pressedBg #cbcbc9 }
		"thinkeramik"	{ set labelBg #e6e6e1;  set pressedBg #cfcfc9 }
	    }
	}

	"#edede1 #f6f6e9" {	;# color scheme "Thin Keramik" new
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #76884c }
		"light, 3rd revision"		      { set pressedBg #fffff7 }
		"platinum"			      { set pressedBg #c5c5bb }
		"*curve*"	{ set labelBg #fdfdf9;  set pressedBg #d1d1a8 }
		"baghira"	{ set labelBg #f6f6f5;  set pressedBg #f3f3f1 }
		"highcolor"	{ set labelBg #f1f1ec;  set pressedBg #f6f6e9 }
		"keramik"	{ set labelBg #f7f7f0;  set pressedBg #e3e3da }
		"phase"		{ set labelBg #edede1;  set pressedBg #c5c5bb }
		"plastik"	{ set labelBg #f4f4e6;  set pressedBg #ddddd0 }
		"thinkeramik"	{ set labelBg #eaeae3;  set pressedBg #d4d4cb }
	    }
	}

	"#f6f5e8 #eeeade" {	;# color scheme "Thin Keramik II"
	    switch -glob -- $style {
		"dotnet"			      { set pressedBg #edc967 }
		"light, 3rd revision"		      { set pressedBg #ffffff }
		"platinum"			      { set pressedBg #cdccc1 }
		"*curve*"	{ set labelBg #f6f3ec;  set pressedBg #c7bea3 }
		"baghira"	{ set labelBg #f7f7f7;  set pressedBg #f3f2ee }
		"highcolor"	{ set labelBg #eeeae1;  set pressedBg #eeeade }
		"keramik"	{ set labelBg #f3f1e8;  set pressedBg #dddad1 }
		"phase"		{ set labelBg #f3f2e9;  set pressedBg #cdccc1 }
		"plastik"	{ set labelBg #ebe7dc;  set pressedBg #c9c6bc }
		"thinkeramik"	{ set labelBg #f1f1e8;  set pressedBg #dbdad0 }
	    }
	}
    }

    #
    # For some Qt styles the label colors are independent of the color scheme:
    #
    switch -- $style {
	"acqua" {
	    set labelBg #e7e7e7;  set labelFg #000000;  set pressedBg #8fbeec
	}

	"kde_xp" {
	    set labelBg #ebeadb;  set labelFg #000000;  set pressedBg #faf8f3
	}

	"lipstik" {
	    set labelBg $bg;                            set pressedBg $labelBg
	}

	"marble" {
	    set labelBg #cccccc;  set labelFg $fg;      set pressedBg $labelBg
	}

	"riscos" {
	    set labelBg #dddddd;  set labelFg #000000;  set pressedBg $labelBg
	}

	"system" -
	"systemalt" {
	    set labelBg #cbcbcb;  set labelFg #000000;  set pressedBg $labelBg
	}
    }

    #
    # The stripe background color is specified
    # by a global KDE configuration option:
    #
    if {[set val [getKdeConfigVal "General" "alternateBackground"]] eq ""} {
	set stripeBg ""
    } elseif {[string range $val 0 0] eq "#"} {
	set stripeBg $val
    } elseif {[scan $val "%d,%d,%d" r g b] == 3} {
	set stripeBg [format "#%02x%02x%02x" $r $g $b]
    } else {
	set stripeBg ""
    }

    #
    # The arrow color and style depend mainly on the current Qt style:
    #
    switch -glob -- $style {
	"*curve*"	{ set arrowColor $labelFg;  set arrowStyle flat7x5 }

	"dotnet" -
	"highcontrast" -
	"light, 2nd revision" -
	"light, 3rd revision" -
	"lipstik" -
	"phase" -
	"plastik"	{ set arrowColor $labelFg;  set arrowStyle flat7x4 }

	"baghira"	{ set arrowColor $labelFg;  set arrowStyle flat7x7 }

	"keramik" -
	"thinkeramik"	{ set arrowColor $labelFg;  set arrowStyle flat8x5 }

	default		{ set arrowColor "";	    set arrowStyle sunken12x11 }
    }

    variable themeDefaults
    array set themeDefaults [list \
	-background		$tableBg \
	-foreground		$tableFg \
	-disabledforeground	$tableDisFg \
	-stripebackground	$stripeBg \
	-selectbackground	$selectBg \
	-selectforeground	$selectFg \
	-selectborderwidth	0 \
	-font			TkTextFont \
        -labelbackground	$labelBg \
	-labeldisabledBg	$labelBg \
	-labelactiveBg		$labelBg \
	-labelpressedBg		$pressedBg \
	-labelforeground	$labelFg \
	-labeldisabledFg	$labelDisFg \
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
	-stripebackground	"" \
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
	-stripebackground	"" \
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
	-stripebackground	"" \
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

#------------------------------------------------------------------------------
# tablelist::getKdeConfigVal
#
# Returns the value of the global KDE configuration option identified by the
# given group (section) and key.
#------------------------------------------------------------------------------
proc tablelist::getKdeConfigVal {group key} {
    variable kdeDirList

    if {![info exists kdeDirList]} {
	makeKdeDirList 
    }

    #
    # Search for the entry corresponding to the given group and key in
    # the file "share/config/kdeglobals" within the KDE directories
    #
    foreach dir $kdeDirList {
	set fileName [file join $dir "share/config/kdeglobals"]
	if {[set val [readKdeConfigVal $fileName $group $key]] ne ""} {
	    return $val
	}
    }
    return ""
}

#------------------------------------------------------------------------------
# tablelist::makeKdeDirList
#
# Builds the list of the directories to be considered when searching for global
# KDE configuration options.
#------------------------------------------------------------------------------
proc tablelist::makeKdeDirList {} {
    variable kdeDirList {}

    if {[info exists ::env(USER)] && $::env(USER) eq "root"} {
	set name "KDEROOTHOME"
    } else {
	set name "KDEHOME"
    }
    if {[info exists ::env($name)] && $::env($name) ne ""} {
	set localKdeDir [file normalize $::env($name)]
    } elseif {[info exists ::env(HOME)] && $::env(HOME) ne ""} {
	set localKdeDir [file normalize [file join $::env(HOME) ".kde"]]
    }
    if {[info exists localKdeDir] && $localKdeDir ne "-"} {
	lappend kdeDirList $localKdeDir
    }

    if {[info exists ::env(KDEDIRS)] && $::env(KDEDIRS) ne ""} {
	foreach dir [split $::env(KDEDIRS) ":"] {
	    if {$dir ne ""} {
		lappend kdeDirList $dir
	    }
	}
    } elseif {[info exists ::env(KDEDIR)] && $::env(KDEDIR) ne ""} {
	lappend kdeDirList $::env(KDEDIR)
    }

    set prefix [exec kde-config --prefix]
    lappend kdeDirList $prefix

    set execPrefix [exec kde-config --expandvars --exec-prefix]
    if {$execPrefix ne $prefix} {
	lappend kdeDirList $execPrefix
    }
}

#------------------------------------------------------------------------------
# tablelist::readKdeConfigVal
#
# Reads the value of the global KDE configuration option identified by the
# given group (section) and key from the specified file.  Note that the
# procedure performs a case-sensitive search and only works as expected for
# "simple" group and key names.
#------------------------------------------------------------------------------
proc tablelist::readKdeConfigVal {fileName group key} {
    if {[catch {open $fileName r} chan] != 0} {
	return ""
    }

    #
    # Search for the specified group
    #
    set groupFound 0
    while {[gets $chan line] >= 0} {
	set line [string trim $line]
	if {$line eq "\[$group\]"} {
	    set groupFound 1
	    break
	}
    }
    if {!$groupFound} {
	close $chan
	return ""
    }

    #
    # Search for the specified key within the group
    #
    set pattern "^$key\\s*=\\s*(.+)$"
    set keyFound 0
    while {[gets $chan line] >= 0} {
	set line [string trim $line]
	if {[string range $line 0 0] eq "\["} {
	    break
	}

	if {[regexp $pattern $line dummy val]} {
	    set keyFound 1
	    break
	}
    }

    close $chan
    return [expr {$keyFound ? $val : ""}]
}
