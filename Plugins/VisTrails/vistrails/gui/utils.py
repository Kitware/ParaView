
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

""" Utilities for creating simple dialogs, notifications in Vistrails
without exposing Qt codes """

from PyQt4 import QtGui, QtCore
from gui.theme import CurrentTheme
import gui.theme
from core.system import systemType
################################################################################

OK_BUTTON              = QtGui.QMessageBox.Ok
OPEN_BUTTON            = QtGui.QMessageBox.Open
SAVE_BUTTON            = QtGui.QMessageBox.Save
CANCEL_BUTTON          = QtGui.QMessageBox.Cancel
CLOSE_BUTTON           = QtGui.QMessageBox.Close
DISCARD_BUTTON         = QtGui.QMessageBox.Discard
APPLY_BUTTON           = QtGui.QMessageBox.Apply
RESET_BUTTON           = QtGui.QMessageBox.Reset
RESTOREDEFAULTS_BUTTON = QtGui.QMessageBox.RestoreDefaults
HELP_BUTTON            = QtGui.QMessageBox.Help
SAVEALL_BUTTON         = QtGui.QMessageBox.SaveAll
YES_BUTTON             = QtGui.QMessageBox.Yes
YESTOALL_BUTTON        = QtGui.QMessageBox.YesToAll
NO_BUTTON              = QtGui.QMessageBox.No
NOTOALL_BUTTON         = QtGui.QMessageBox.NoToAll
ABORT_BUTTON           = QtGui.QMessageBox.Abort
RETRY_BUTTON           = QtGui.QMessageBox.Retry
IGNORE_BUTTON          = QtGui.QMessageBox.Ignore
NOBUTTON_BUTTON        = QtGui.QMessageBox.NoButton

_buttons_captions_dict = { OK_BUTTON   : "Ok",
                           OPEN_BUTTON : "Open",
                           SAVE_BUTTON : "Save",
                           CANCEL_BUTTON : "Cancel",
                           CLOSE_BUTTON : "Close",
                           DISCARD_BUTTON : "Discard",
                           APPLY_BUTTON : "Apply",
                           RESET_BUTTON : "Reset",
                           RESTOREDEFAULTS_BUTTON : "Restore Defaults",
                           HELP_BUTTON : "Help",
                           SAVEALL_BUTTON : "Save All",
                           YES_BUTTON : "Yes",
                           NO_BUTTON : "No",
                           NOTOALL_BUTTON : "No to All",
                           ABORT_BUTTON : "Abort",
                           RETRY_BUTTON : "Retry",
                           IGNORE_BUTTON : "Ignore",
                           NOBUTTON_BUTTON : ""}


def show_warning(title, message):
    """ show_warning(title: str, message: str) -> None
    Show a warning  message box with a specific title and contents

    """
    if systemType not in ['Darwin']:
        QtGui.QMessageBox.warning(None, title, message)
    else:
        show_custom(title,message)

def show_info(title, message):
    """ show_info(title: str, message: str) -> None
    Show an information message box with a specific title and contents

    """
    if systemType not in ['Darwin']:
        QtGui.QMessageBox.information(None, title, message)
    else:
        show_custom(title,message)

def show_question(title,
                  message,
                  buttons = [OK_BUTTON],
                  default = NOBUTTON_BUTTON):
    """ show_question(title: str,
                      message: str,
                      buttons: list of buttons (defined above),
                      default: button (defined above)) -> button
    Show a question message with a specific title, message and a set
    of buttons defined by the list buttons. Default button is the
    button that will take role when the user press 'Enter' without
    selecting a button. The function returns the button that ends the
    dialog.

    """
    qButtons = QtGui.QMessageBox.StandardButtons()
    for button in buttons:
        qButtons |= button
    if systemType not in ['Darwin']:
        return QtGui.QMessageBox.question(None, title, message,
                                          qButtons, default)
    else:
        return show_custom(title,message,None,buttons)

def build_custom_window(title, message, icon=None,
                buttons = [OK_BUTTON], default=OK_BUTTON, escape=-1,
                modal=True, parent=None):
    """ show_custom(title: str,
                    message: str,
                    icon: QPixmap,
                    buttons: list of buttons (defined above),
                    default: str,
                    escape: str,
                    modal: bool,
                    parent: QWidget) -> QMessageBox
    Build a custom dialog box.
    Default is the button in buttons that will be clicked if
    the user presses Enter. escape is the button in buttons
    that will be clicked if Esc is pressed.
    The function returns the index of the button that was pressed.

    """
    msgBox = QtGui.QMessageBox(parent)
    abstractButtons = {}
    for b in buttons:
        msgBox.addButton(b)
        abstractButtons[b] = msgBox.button(b)
    if abstractButtons.has_key(default):
        msgBox.setDefaultButton(abstractButtons[default])
    if escape != -1:
        msgBox.setEscapeButton(abstractButtons[escape])
    msgBox.setWindowFlags(QtCore.Qt.SplashScreen | QtCore.Qt.WindowStaysOnTopHint)
    if modal:
        msgBox.setWindowModality(QtCore.Qt.ApplicationModal)
    else:
        msgBox.setWindowModality(QtCore.Qt.NonModal)
    if icon:
        msgBox.setIconPixmap(icon)
    else:
        pixmap = CurrentTheme.APPLICATION_PIXMAP.scaledToHeight(48)
        msgBox.setIconPixmap(pixmap)

    #mac doesn't show a window title
    #we need to include the title as part of the message
    if systemType not in ['Darwin']:
        msgBox.setWindowTitle(title)
        msgBox.setText(message)
    else:
        msgBox.setText("%s\n%s"%(title,
                                 message))

    qButtons = QtGui.QMessageBox.StandardButtons()
    return msgBox

def show_custom(title, message, icon=None,
                buttons = [OK_BUTTON], default=OK_BUTTON, escape=-1):
    """ show_custom(title: str,
                    message: str,
                    icon: QPixmap,
                    buttons: list of buttons (defined above),
                    default: str,
                    escape: str) -> int
    Show a custom dialog box.
    Default is the button in buttons that will be clicked if
    the user presses Enter. escape is the button in buttons
    that will be clicked if Esc is pressed.
    The function returns the index of the button that was pressed.

    """
    msgBox = build_custom_window(title,message,icon,
                          buttons, default,escape)
    return msgBox.exec_()

def getBuilderWindow():
    """ getBuilderWindow() -> QMainWindow
    Return the current builder window of VisTrails if exists    
    """
    try:
        return QtCore.QCoreApplication.instance().builderWindow
    except:
        return None

def getCurrentVersion():
    """ getCurrentVersion() -> int
    Return the current version on VisTrails, return -1 if the GUI is not available
    
    """
    builderWindow = getBuilderWindow()
    if builderWindow!=None:
        current_view = builderWindow.viewManager.currentWidget()
        if current_view!=None:
            return current_view.controller.current_version
    return -1

def initTheme():
    return gui.theme.initializeCurrentTheme()

################################################################################
# VisTrails GUI unit test class - setUp and teardown ensure no
# vistrails are open

import unittest

class TestVisTrailsGUI(unittest.TestCase):

    def _close_all(self):
        import api
        # Close all open vistrails
        try:
            while 1:
                v = api.get_current_vistrail_view()
                api.close_vistrail(v)
        except api.NoVistrail:
            pass

    def setUp(self):
        self._close_all()

    def tearDown(self):
        self._close_all()
