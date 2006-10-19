#==============================================================================
# Main Tablelist package module.
#
# Copyright (c) 2000-2006  Csaba Nemethi (E-mail: csaba.nemethi@t-online.de)
#==============================================================================

package require Tcl 8
package require Tk  8
package require tablelist::common

package provide Tablelist $::tablelist::version
package provide tablelist $::tablelist::version

::tablelist::useTile 0
