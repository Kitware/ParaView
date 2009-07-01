
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

from PyQt4 import QtGui, QtCore
from gui.theme import CurrentTheme

class QAddonToolBar(QtGui.QToolBar):
    """
    QAddonToolBar is a toolbar that is always at the bottom dock area
    with a closing button. We know that the parents is the builder
    window so we can update its controller when it is visible.
    
    """
    def __init__(self, parent=None):
        """ QSearchToolBar(parent: QWidget) -> QSearchToolBar
        
        """
        QtGui.QToolBar.__init__(self, parent)
        self._closeButton = QtGui.QToolButton(self)
        self._closeButton.setIcon(CurrentTheme.CLOSE_ICON)
        self.connect(self._closeButton, QtCore.SIGNAL('clicked(bool)'), self.hide)
        self.builderWindow = parent
        self.controller = None
        self.hide()

    def keyPressEvent(self, e):
        """ Hide on hitting Esc """
        if e.key()==QtCore.Qt.Key_Escape:
            self.hide()
        return QtGui.QToolBar.keyPressEvent(self, e)

    def showEvent(self, e):
        """ Make sure we adjust the close button geometry on showing the toolbar """
        QtGui.QToolBar.showEvent(self, e)
        self.updateCloseButtonGeometry()
        if self.builderWindow:
            curWidget = self.builderWindow.viewManager.currentWidget()
            if curWidget:
                self.setController(curWidget.controller)

    def resizeEvent(self, e):
        """ Make sure we adjust the close button geometry on resizing the toolbar """
        QtGui.QToolBar.resizeEvent(self, e)
        if self.isVisible():
            self.updateCloseButtonGeometry()

    def updateCloseButtonGeometry(self):
        """ Adjust the close button to the far right """
        buttonWidth = 24
        cRect = self.contentsRect()
        self.setContentsMargins(0, 0, buttonWidth+4, 0)
        self.layout().invalidate()
        rect = QtCore.QRect(self.width()-buttonWidth, 0,
                            buttonWidth, cRect.height())
        self._closeButton.setGeometry(rect)
        
    def setController(self, controller):
        """ setController(controller: VistrailController) -> None

        """
        self.controller = controller
