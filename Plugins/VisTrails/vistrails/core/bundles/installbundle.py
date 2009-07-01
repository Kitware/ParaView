
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

"""Module with utilities to try and install a bundle if possible."""

from core.bundles.utils import guess_system, guess_graphical_sudo
import core.bundles.installbundle # this is on purpose
import os

##############################################################################

def has_qt():
    try:
        import PyQt4.QtGui
        # Must import this on Ubuntu linux, because PyQt4 doesn't come with
        # PyQt4.QtOpenGL by default
        import PyQt4.QtOpenGL
        return True
    except ImportError:
        return False

def hide_splash_if_necessary():
    qt = has_qt()
    # HACK, otherwise splashscreen stays in front of windows
    if qt:
        try:
            import PyQt4.QtCore
            PyQt4.QtCore.QCoreApplication.instance().splashScreen.hide()
        except:
            pass

def linux_ubuntu_install(package_name):
    qt = has_qt()
    hide_splash_if_necessary()
        
    if qt:
        cmd = core.system.vistrails_root_directory()
        cmd += '/core/bundles/linux_ubuntu_install.py'
    else:
        cmd = 'apt-get install -y'

    if type(package_name) == str:
        cmd += ' ' + package_name
    elif type(package_name) == list:
        for package in package_name:
            if type(package) != str:
                raise TypeError("Expected string or list of strings")
            cmd += ' ' + package

    if qt:
        sucmd = guess_graphical_sudo() + " '" + cmd + "'"
    else:
        print "VisTrails wants to install package(s) '%s'" % package_name
        sucmd = "sudo " + cmd

    result = os.system(sucmd)

    return (result == 0) # 0 indicates success

def linux_fedora_install(package_name):
    qt = has_qt()
    hide_splash_if_necessary()
    if qt:
        cmd = core.system.vistrails_root_directory()
        cmd += '/core/bundles/linux_fedora_install.py'
    else:
        cmd = 'yum -y install'

    if type(package_name) == str:
        cmd += ' ' + package_name
    elif type(package_name) == list:
        for package in package_name:
            if type(package) != str:
                raise TypeError("Expected string or list of strings")
            cmd += ' ' + package

    if qt:
        sucmd = guess_graphical_sudo() + " " + cmd
    else:
        print ("VisTrails wants to install package(s) '%s' through _sudo_. Make sure" +
               " you are a sudoer.") % package_name
        sucmd = "sudo " + cmd

    print sucmd
    result = os.system(sucmd)
    print "RETURN VALUE", result
    return (result == 0)

def show_question(which_files):
    qt = has_qt()
    if qt:
        import gui.utils
        if type(which_files) == str:
            which_files = [which_files]
        v = gui.utils.show_question("Required packages missing",
                                    "One or more required packages are missing: " +
                                    " ".join(which_files) +
                                    ". VisTrails can " +
                                    "automaticallly install them. " +
                                    "If you click OK, VisTrails will need "+
                                    "administrator privileges, and you " +
                                    "might be asked for the administrator password.",
                                    buttons=[gui.utils.OK_BUTTON,
                                             gui.utils.CANCEL_BUTTON],
                                    default=gui.utils.OK_BUTTON)
        return v == gui.utils.OK_BUTTON
    else:
        print "Required package missing"
        print ("A required package is missing, but VisTrails can " +
               "automaticallly install it. " +
               "If you say Yes, VisTrails will need "+
               "administrator privileges, and you" +
               "might be asked for the administrator password.")
        print "Give VisTrails permission to try to install package? (y/N)"
        v = raw_input().upper()
        return v == 'Y' or v == 'YES'


def install(dependency_dictionary):
    """Tries to import a python module. If unsuccessful, tries to install
the appropriate bundle and then reimport. py_import tries to be smart
about which system it runs on."""

    # Ugly fix to avoid circular import
    distro = guess_system()
    if not dependency_dictionary.has_key(distro):
        return False
    else:
        files = dependency_dictionary[distro]
        if show_question(files):
            callable_ = getattr(core.bundles.installbundle,
                                distro.replace('-', '_') + '_install')
            return callable_(files)
        else:
            return False
