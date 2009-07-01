
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

"""module that allows online inspection of environment to test presence of
runtime components such as binaries, libraries, other python modules, etc."""

import sys
import core.system
import core.bundles.installbundle

##############################################################################

def python_module_exists(module_name):
    """python_module_exists(module_name): Boolean.
Returns if python module of given name can be safely imported."""
    
    try:
        sys.modules[module_name]
        return True
    except KeyError:
        pass
    try:
        __import__(module_name)
        return True
    except ImportError:
        return False
    

def executable_file_exists(filename):
    """executable_file_exists(filename): Boolean.
Returns if certain file is in current path and is executable."""
    result = core.system.executable_is_in_path(filename)
    if result == "":
        result = core.system.executable_is_in_pythonpath(filename)
    return result != ""

# FIXME: Add documentation.

def require_python_module(module_name):
    if not python_module_exists(module_name):
        raise MissingRequirement(module_name)

def require_executable(filename):
    if not executable_file_exists(filename):
        raise MissingRequirement(filename)

def check_pyqt4():
    # checks for the presence of pyqt4, which is more important than the rest,
    # since using pyqt requires a qapplication.
    try:
        require_python_module('PyQt4.QtGui')
        require_python_module('PyQt4.QtOpenGL')
    except MissingRequirement:
        r = core.bundles.installbundle.install({'linux-ubuntu': ['python-qt4',
                                                                 'python-qt4-gl',
                                                                 'python-qt4-sql']})
        if not r:
            raise


def check_all_vistrails_requirements():
    pass

    # check scipy
#     try:
#         require_python_module('scipy')
#     except MissingRequirement:
#         r = core.bundles.installbundle.install({'linux-ubuntu': 'python-scipy'})
#         if not r:
#             raise
        

##############################################################################

class MissingRequirement(Exception):
    """Raise this exception in packages when necessary items are missing."""
    def __init__(self, req):
        self.requirement = req
    def __str__(self):
        return "Missing Requirement: %s" % self.requirement

