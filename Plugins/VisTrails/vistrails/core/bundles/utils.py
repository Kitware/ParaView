
############################################################################
##
## This file is part of the Vistrails ParaView Plugin.
##
## This file may be used under the terms of the GNU General Public
## License version 2.0 as published by the Free Software Foundation
## and appearing in the file LICENSE.GPL included in the packaging of
## this file.  Please review the following to ensure GNU General Public
## Licensing requirements will be met:
## http://www.opensource.org/licenses/gpl-2.0.php
##
## This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
## WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
##
############################################################################

############################################################################
##
## Copyright (C) 2006, 2007, 2008 University of Utah. All rights reserved.
##
############################################################################

"""Utility functions for core.bundles"""

import core.system
import os

##############################################################################

def guess_graphical_sudo():
    """Tries to guess what to call to run a shell with elevated
privileges."""
    if core.system.executable_is_in_path('kdesu'):
        return 'kdesu -c'
    elif core.system.executable_is_in_path('gksu'):
        return 'gksu'
    elif (core.system.executable_is_in_path('sudo') and
          core.system.executable_is_in_path('zenity')):
        # This is a reasonably convoluted hack to only prompt for the password
        # if user has not recently entered it
        return ('((echo "" | sudo -v -S -p "") || ' +
                '(zenity --entry --title "sudo password prompt" --text "Please enter your password '
                'to give the system install authorization." --hide-text="" | sudo -v -S -p "")); sudo -S -p ""')
    else:
        print "Could not find a graphical su-like command."
        print "Will use regular su"
        return 'su -c'

##############################################################################

class System_guesser(object):

    def __init__(self):
        self._callable_dict = {}

    def add_test(self, test, system_name):
        if self._callable_dict.has_key(system_name):
            raise Exception("test for '%s' already present." % system_name)
        if system_name == 'UNKNOWN':
            raise Exception("Invalid system name")
        assert type(system_name) == str
        self._callable_dict[system_name] = test

    def guess_system(self):
        for (name, callable_) in self._callable_dict.iteritems():
            if callable_():
                return name
        else:
            return 'UNKNOWN'

_system_guesser = System_guesser()

##############################################################################
# System tests

def _guess_suse():
    try:
        tokens = file('/etc/SuSE-release').readline()[-1].split()
        return tokens[0] == 'SUSE'
    except:
        return False
_system_guesser.add_test(_guess_suse, 'linux-suse')

def _guess_ubuntu():
    return os.path.isfile('/etc/apt/apt.conf.d/01ubuntu')
_system_guesser.add_test(_guess_ubuntu, 'linux-ubuntu')

def _guess_fedora():
    return os.path.isfile('/etc/fedora-release')
_system_guesser.add_test(_guess_fedora, 'linux-fedora')

##############################################################################

def guess_system():
    """guess_system will try to identify which system you're running. Result
will be a string describing the system. This is more discriminating than
Linux/OSX/Windows: We'll try to figure out whether you're running SuSE, Debian,
Ubuntu, RedHat, fink, darwinports, etc.

Currently, we only support SuSE, Ubuntu and Fedora. However, we only have
actual bundle installing for Ubuntu and Fedora."""
    return _system_guesser.guess_system()
