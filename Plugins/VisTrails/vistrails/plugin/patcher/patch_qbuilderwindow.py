
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

##############################################################################
# Patch QBuilderWindow

import gui.builder_window
from gui.view_manager import QViewManager
from gui.module_palette import QModulePalette

from PyQt4 import QtGui, QtCore

import plugin.pgui.builder_window

import CaptureAPI
from core.vistrail.vistrail import Vistrail

def VC_open_vistrail(self, locator_class):
    """ open_vistrail(locator_class) -> None
    Prompt user for information to get to a vistrail in different ways,
    depending on the locator class given.
    """
    locator=locator_class.load_from_gui(self, Vistrail.vtType)
    if locator:
        if locator.has_temporaries():
            # Don't prompt for autosave if the user is reloading the same file from
            # last save
            current_locator = self.viewManager.currentWidget().controller.locator
            if (current_locator and current_locator.name == locator.name) \
                    or not locator_class.prompt_autosave(self):
                locator.clean_temporaries()
        try:
            name=locator._get_name()
        except:
            name=''
        CaptureAPI.beforeOpenVistrail(name)
        self.open_vistrail_without_prompt(locator)
        CaptureAPI.afterOpenVistrail()

gui.builder_window.QBuilderWindow.open_vistrail=VC_open_vistrail
