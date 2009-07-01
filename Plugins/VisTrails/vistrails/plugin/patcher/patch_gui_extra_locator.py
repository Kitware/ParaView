
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
## Copyright (C) 2008, 2009 VisTrails, Inc. All rights reserved.
##
############################################################################
import core.system
import os
from PyQt4 import QtGui, QtCore
from core.db.locator import DBLocator, FileLocator
#from plugin.pcore.preferences import Preferences
import CaptureAPI

suffix_map = {'vistrail': ['vt'],
              'workflow': ['xml'],
              'log': ['xml'],
              }

def new_get_load_file_locator_from_gui(parent, obj_type):
    from gui.application import VistrailsApplication
    suffixes = "*." + " *.".join(suffix_map[obj_type])
    fileName = QtGui.QFileDialog.getOpenFileName(
        parent,
        "Open %s..." % obj_type.capitalize(),
        core.system.vistrails_file_directory(),
        "VisTrails files (%s)\nOther files (*)" % suffixes)
    if fileName.isEmpty():
        return None
    filename = os.path.abspath(str(fileName))
    dirName = os.path.dirname(filename)
    setattr(VistrailsApplication.configuration, 'fileDirectory', dirName)
    #Preferences.set('VisTrailsFileDirectory', dirName)
    CaptureAPI.setPreference('VisTrailsFileDirectory', dirName)
    core.system.set_vistrails_file_directory(dirName)
    return FileLocator(filename)

def new_get_save_file_locator_from_gui(parent, obj_type, locator=None):
    # Ignore current locator for now
    # In the future, use locator to guide GUI for better starting directory
    from gui.application import VistrailsApplication
    suffixes = "*." + " *.".join(suffix_map[obj_type])
    fileName = QtGui.QFileDialog.getSaveFileName(
        parent,
        "Save Vistrail...",
        core.system.vistrails_file_directory(),
        "VisTrails files (%s)" % suffixes, # filetypes.strip()
        None,
        QtGui.QFileDialog.DontConfirmOverwrite)
    if fileName.isEmpty():
        return None
    f = str(fileName)

    # check for proper suffix
    found_suffix = False
    for suffix in suffix_map[obj_type]:
        if f.endswith(suffix):
            found_suffix = True
            break
    if not found_suffix:
        if obj_type == 'vistrail':
            f += VistrailsApplication.configuration.defaultFileType
        else:
            f += '.' + suffix_map[obj_type][0]

    if os.path.isfile(f):
        msg = QtGui.QMessageBox(QtGui.QMessageBox.Question,
                                "VisTrails",
                                "File exists. Overwrite?",
                                (QtGui.QMessageBox.Yes |
                                 QtGui.QMessageBox.No),
                                parent)
        if msg.exec_() == QtGui.QMessageBox.No:
            return None
    dirName = os.path.dirname(str(f))
    setattr(VistrailsApplication.configuration, 'fileDirectory', dirName)
    #Preferences.set('VisTrailsFileDirectory', dirName)
    CaptureAPI.setPreference('VisTrailsFileDirectory', dirName)
    core.system.set_vistrails_file_directory(dirName)
    return FileLocator(f)

import gui.extras.core.db.locator
gui.extras.core.db.locator.get_load_file_locator_from_gui = new_get_load_file_locator_from_gui
gui.extras.core.db.locator.get_save_file_locator_from_gui = new_get_save_file_locator_from_gui
