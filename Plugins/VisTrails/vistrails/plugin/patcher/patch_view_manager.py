
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
# Patch QViewManager

from PyQt4 import QtGui, QtCore
from gui.vistrail_view import QVistrailView
import gui.view_manager

def QVM_new_init(self, parent=None):
    """ QViewManager(parent: QWidget) -> QViewManager
    Create a tab widget without the tabs

    """
    QtGui.QTabWidget.__init__(self, parent)
    self.set_single_document_mode(True)
    self.splittedViews = {}
    self.activeIndex = -1
    self._views = {}

def QVM_fitToView(self, recompute_bounding_rect):
    vistrailView = self.currentWidget()
    if vistrailView:
        versionView = vistrailView.versionTab.versionView
        versionView.scene().fitToView(versionView, recompute_bounding_rect)

gui.view_manager.QViewManager.__init__ = QVM_new_init
gui.view_manager.QViewManager.fitToView = QVM_fitToView

