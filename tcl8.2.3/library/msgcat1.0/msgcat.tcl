# msgcat.tcl --
#
#	This file defines various procedures which implement a
#	message catalog facility for Tcl programs.  It should be
#	loaded with the command "package require msgcat".
#
# Copyright (c) 1998 by Scriptics Corporation.
# Copyright (c) 1998 by Mark Harrison.
#
# See the file "license.terms" for information on usage and redistribution
# of this file, and for a DISCLAIMER OF ALL WARRANTIES.
# 
# RCS: @(#) Id

package provide msgcat 1.0

namespace eval msgcat {
    namespace export mc mcset mclocale mcpreferences mcunknown

    # Records the current locale as passed to mclocale
    variable locale ""

    # Records the list of locales to search
    variable loclist {}

    # Records the mapping between source strings and translated strings.  The
    # array key is of the form "<locale>,<namespace>,<src>" and the value is
    # the translated string.
    array set msgs {}
}

# msgcat::mc --
#
#	Find the translation for the given string based on the current
#	locale setting.
#
# Arguments:
#	src	The string to translate.
#
# Results:
#	Returns the translatd string.

proc msgcat::mc {src} {
    set ns [uplevel {namespace current}]
    foreach loc $::msgcat::loclist {
	if {[info exists ::msgcat::msgs($loc,$ns,$src)]} {
	    return $::msgcat::msgs($loc,$ns,$src)
	}
    }
    # we have not found the translation
    return [uplevel 1 [list [namespace origin mcunknown] \
	    $::msgcat::locale $src]]
}

# msgcat::mclocale --
#
#	Query or set the current locale.
#
# Arguments:
#	newLocale	(Optional) The new locale string. Locale strings
#			should be composed of one or more sublocale parts
#			separated by underscores (e.g. en_US).
#
# Results:
#	Returns the current locale.

proc msgcat::mclocale {args} {
    set len [llength $args]

    if {$len > 1} {
	error {wrong # args: should be "mclocale ?newLocale?"}
    }

    set args [string tolower $args]
    if {$len == 1} {
	set ::msgcat::locale $args
	set ::msgcat::loclist {}
	set word ""
	foreach part [split $args _] {
	    set word [string trimleft "${word}_${part}" _]
	    set ::msgcat::loclist \
                    [linsert $::msgcat::loclist 0 $word]
	}
    }
    return $::msgcat::locale
}

# msgcat::mcpreferences --
#
#	Fetch the list of locales used to look up strings, ordered from
#	most preferred to least preferred.
#
# Arguments:
#	None.
#
# Results:
#	Returns an ordered list of the locales preferred by the user.

proc msgcat::mcpreferences {} {
    return $::msgcat::loclist
}

# msgcat::mcload --
#
#	Attempt to load message catalogs for each locale in the
#	preference list from the specified directory.
#
# Arguments:
#	langdir		The directory to search.
#
# Results:
#	Returns the number of message catalogs that were loaded.

proc msgcat::mcload {langdir} {
    set x 0
    foreach p [::msgcat::mcpreferences] {
	set langfile [file join $langdir $p.msg]
	if {[file exists $langfile]} {
	    incr x
	    uplevel [list source $langfile]
	}
    }
    return $x
}

# msgcat::mcset --
#
#	Set the translation for a given string in a specified locale.
#
# Arguments:
#	locale		The locale to use.
#	src		The source string.
#	dest		(Optional) The translated string.  If omitted,
#			the source string is used.
#
# Results:
#	Returns the new locale.

proc msgcat::mcset {locale src {dest ""}} {
    if {$dest == ""} {
	set dest $src
    }

    set ns [uplevel {namespace current}]

    set ::msgcat::msgs([string tolower $locale],$ns,$src) $dest
    return $dest
}

# msgcat::mcunknown --
#
#	This routine is called by msgcat::mc if a translation cannot
#	be found for a string.  This routine is intended to be replaced
#	by an application specific routine for error reporting
#	purposes.  The default behavior is to return the source string.  
#
# Arguments:
#	locale		The current locale.
#	src		The string to be translated.
#
# Results:
#	Returns the translated value.

proc msgcat::mcunknown {locale src} {
    return $src
}

# Initialize the default locale

namespace eval msgcat {
    # set default locale, try to get from environment
    if {[info exists ::env(LANG)]} {
        mclocale $::env(LANG)
    } else {
        mclocale "C"
    }
}
