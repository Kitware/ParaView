/*
 * tclMacAETE.r --
 *
 *	This file creates the Apple Event Terminology resources 
 *	for use Tcl and Tk.  It is not used in the Simple Tcl shell
 *      since SIOUX does not support AppleEvents.  An example of its
 *      use in Tcl is the TclBGOnly project.  And it is used in all the
 *      Tk Shells.
 *
 * Copyright (c) 1997 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * RCS: @(#) Id
 */

#define SystemSevenOrLater 1

#include <Types.r>
#include <SysTypes.r>
#include <AEUserTermTypes.r>

/*
 * The following resources defines the Apple Events that Tk can be
 * sent from Apple Script.
 */

resource 'aete' (0, "Wish Suite") {
    0x01, 0x00, english, roman,
    {
	"Required Suite", 
	"Events that every application should support", 
	'reqd', 1, 1,
	{},
	{},
	{},
	{},

	"Wish Suite", "Events for the Wish application", 'WIsH', 1, 1,
	{
	    "do script", "Execute a Tcl script", 'misc', 'dosc',
	    'TEXT', "Result", replyOptional, singleItem,
	    notEnumerated, reserved, reserved, reserved, reserved,
	    reserved, reserved, reserved, reserved, reserved,
	    reserved, reserved, reserved, reserved, 
	    'TEXT', "Script to execute", directParamRequired,
	    singleItem, notEnumerated, changesState, reserved,
	    reserved, reserved, reserved, reserved, reserved,
	    reserved, reserved, reserved, reserved, reserved,
	    reserved, 
	    {},
	},
	{},
	{},
	{},
    }
};
