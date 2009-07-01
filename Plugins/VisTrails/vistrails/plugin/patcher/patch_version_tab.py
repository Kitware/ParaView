
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
# Patch version view

import gui.version_tab
from PyQt4 import QtCore

def QVT_new_vistrailChanged(self):
    self.versionView.scene().setupScene(self.controller)    
    if self.controller:
        if self.controller.need_view_adjust and self.controller.current_version!=-1:
            self.versionView.currentScale = 800
            self.versionView.updateMatrix()
            self.versionView.verticalScrollBar().setValue(-90)
            self.controller.need_view_adjust = False
        self.versionProp.updateVersion(self.controller.current_version)
        self.versionView.versionProp.updateVersion(self.controller.current_version)
    self.emit(QtCore.SIGNAL("vistrailChanged()"))

    
def QVT_new_single_node_changed(self, old_version, new_version):
    self.versionView.scene().update_scene_single_node_change(self.controller,
                                                             old_version,
                                                             new_version)
    if self.controller:
        self.versionProp.updateVersion(self.controller.current_version)
        self.versionView.versionProp.updateVersion(self.controller.current_version)
        self.emit(QtCore.SIGNAL("vistrailChanged()"))
        
gui.version_tab.QVersionTab.vistrailChanged = QVT_new_vistrailChanged
gui.version_tab.QVersionTab.single_node_changed = QVT_new_single_node_changed
