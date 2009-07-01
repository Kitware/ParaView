
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
# Patch graphics_view
from PyQt4 import QtCore, QtGui
import gui.graphics_view

def IGV_wheelEvent(self, e):
    self.computeScale()
    newScale = self.currentScale + e.delta()/5.0
    if newScale < 0: newScale = 0
    if newScale > self.scaleMax: newScale = self.scaleMax
    self.currentScale = newScale
    self.updateMatrix()

gui.graphics_view.QInteractiveGraphicsView.wheelEvent = IGV_wheelEvent
