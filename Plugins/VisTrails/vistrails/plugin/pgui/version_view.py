
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

from gui.version_view import QVersionTreeScene
from PyQt4 import QtGui
from core.utils.color import ColorByName

class QPlaybackVersionIndicator(QtGui.QGraphicsEllipseItem):
    """
    An elliptical shape showing around a selected version. This can be
    snapped to a QVersionGrahpicsItem.
    """

    def __init__(self, parent=None):
        """ Create a hidden indicator by default """
        QtGui.QGraphicsEllipseItem.__init__(self, parent)
        self.versionId = -1
        self.setVisible(False)
    
    def snap(self, versionItem):
        """ Adjust this indicator to wrap around the versionItem """
        if versionItem!=None and versionItem>0:
            pad = 5
            rect = versionItem.boundingRect().adjusted(-pad, -pad, pad, pad)
            self.setRect(rect)
            self.versionId = versionItem.id
            self.show()
        else:
            self.hide()

class QPlaybackVersionTreeScene(QVersionTreeScene):
    """
    QVersionTreeScene allow users to exit interactive mode and select
    a start and end version for playing back
    
    """
    def __init__(self, parent=None):
        QVersionTreeScene.__init__(self, parent)
        self.indicatorStart = QPlaybackVersionIndicator()
        self.indicatorStart.setPen(QtGui.QPen(QtGui.QColor(0, 128, 0), 2))
        self.indicatorEnd = QPlaybackVersionIndicator()
        self.indicatorEnd.setPen(QtGui.QPen(QtGui.QColor(128, 0, 0), 2))
        self.indicatorExtra = QPlaybackVersionIndicator()
        self.indicatorExtra.setPen(QtGui.QPen(QtGui.QColor(*(ColorByName.get_int('goldenrod_medium'))), 2))

    def updateIndicator(self, indicator):
        """ Setup the version item for an indicator """
        versionItem = None
        if self.versions.has_key(indicator.versionId):
            versionItem = self.versions[indicator.versionId]
        indicator.snap(versionItem)
        if indicator.scene()!=self:
            self.addItem(indicator)

    def setupScene(self, controller):
        """ Update the indicator whenever the scene is updated """
        QVersionTreeScene.setupScene(self, controller)
        self.updateIndicator(self.indicatorStart)
        self.updateIndicator(self.indicatorEnd)
        self.updateIndicator(self.indicatorExtra)
