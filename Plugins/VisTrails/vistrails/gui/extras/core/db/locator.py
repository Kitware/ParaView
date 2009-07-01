
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

from gui.application import VistrailsApplication
from gui.open_db_window import QOpenDBWindow, QConnectionDBSetupWindow
from core.db.locator import DBLocator, FileLocator, untitled_locator
from db import VistrailsDBException
import db
from PyQt4 import QtGui, QtCore
import core.system
import os
import core.configuration

##############################################################################
# DB dialogs

def get_load_db_locator_from_gui(parent, obj_type):
    config, obj_id = QOpenDBWindow.getOpenDBObject(obj_type)
    if config == {} or obj_id == -1:
        return None
    return DBLocator(config['host'],
                     config['port'],
                     config['db'],
                     config['user'],
                     config['passwd'],
                     None,
                     obj_id,
                     obj_type,
                     config.get('id', None))

def get_save_db_locator_from_gui(parent, obj_type, locator=None):
    config, name = QOpenDBWindow.getSaveDBObject(obj_type)
    if config == {} or name == '':
        return None
    return DBLocator(config['host'],
                     config['port'],
                     config['db'],
                     config['user'],
                     config['passwd'],
                     name,
                     None,
                     obj_type,
                     config.get('id', None))

##############################################################################

def get_db_connection_from_gui(parent, id, name, host, port, user, passwd,
                               database):
    def show_dialog(parent, id, name, host, port, user,
                    passwd, databaseb, create):
        dialog = QConnectionDBSetupWindow(parent, id, name, host, port, user,
                                          passwd, database, create)
        config = None
        if dialog.exec_() == QtGui.QDialog.Accepted:
            config = {'host': str(dialog.hostEdt.text()),
                      'port': int(dialog.portEdt.value()),
                      'user': str(dialog.userEdt.text()),
                      'passwd': str(dialog.passwdEdt.text()),
                      'db': str(dialog.databaseEdt.text())
                      }
            try:
                db.services.io.test_db_connection(config)
                config['succeeded'] = True
                config['name'] = str(dialog.nameEdt.text())
                config['id'] = dialog.id
            except VistrailsDBException, e:
                QtGui.QMessageBox.critical(None,
                                           'Vistrails',
                                           str(e))
                config['succeeded'] = False
        return config
    #check if the information is already there
    dbwindow = QOpenDBWindow.getInstance()
        
    config = dbwindow.connectionList.findConnectionInfo(host,port,database)

    if config:
        testconfig = dict(config)
        del testconfig['id']
        del testconfig['name']
        try:
            db.services.io.test_db_connection(testconfig)
            config['succeeded'] = True
        except VistrailsDBException, e:
            config = show_dialog(parent, config['id'],
                                 config['name'], host, port, config['user'],
                                 passwd, database, create = False)
            
    elif config is None:
        config = show_dialog(parent, -1,"",
                             host, port, user, passwd,
                             database, create = True)
        if config['succeeded'] == True:
            #add to connection list
            dbwindow.connectionList.setConnectionInfo(**config)
    return config

##############################################################################
# File dialogs

suffix_map = {'vistrail': ['vt', 'xml'],
              'workflow': ['xml'],
              'log': ['xml'],
              }

def get_load_file_locator_from_gui(parent, obj_type):
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
    core.system.set_vistrails_file_directory(dirName)
    return FileLocator(filename)

def get_save_file_locator_from_gui(parent, obj_type, locator=None):
    # Ignore current locator for now
    # In the future, use locator to guide GUI for better starting directory

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
    core.system.set_vistrails_file_directory(dirName)
    return FileLocator(f)
   
def get_autosave_prompt(parent):
    """ get_autosave_prompt(parent: QWidget) -> bool
    
    """
    result = QtGui.QMessageBox.question(parent, 
                                        QtCore.QString("AutoSave"),
                                        QtCore.QString("Autosave data has been found.\nDo you want to open autosave data?"),
                                        QtGui.QMessageBox.Open,
                                        QtGui.QMessageBox.Ignore)
    return result == QtGui.QMessageBox.Open
