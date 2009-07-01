
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
from core.query.version import SearchCompiler, SearchParseError
from gui.common_widgets import QSearchBox
from gui.theme import CurrentTheme
from plugin.pgui.addon_toolbar import QAddonToolBar

class QSearchToolBar(QAddonToolBar):
    """
    QSearchToolBar contains a search box for querying the versions.  By
    selecting the reset button, the toolbar is hidden.

    """
    def __init__(self, parent=None):
        """ QSearchToolBar(parent: QWidget) -> QSearchToolBar
        
        """
        QAddonToolBar.__init__(self, parent)
        self.setWindowTitle('Search')
        self.setIconSize(QtCore.QSize(16, 16))

        self.searchBox = QSearchBox(self)
        self.searchBox.resetButton.hide()
        self.searchBox.searchButton.hide()

        self.viewAction = QtGui.QToolButton(self)
        self.viewAction.setIcon(CurrentTheme.QUERY_ARROW_ICON)
        self.viewAction.setToolTip('View')
        self.viewAction.setStatusTip('Search or refine view')
        self.viewAction.setPopupMode(QtGui.QToolButton.InstantPopup)
        self.viewAction.setMenu(self.searchBox.searchMenu)
        self.addWidget(self.viewAction)

        self.addWidget(self.searchBox)

        self.connect(self.searchBox, QtCore.SIGNAL('executeSearch(QString)'),
                     self.executeSearch)
        self.connect(self.searchBox, QtCore.SIGNAL('refineMode(bool)'),
                     self.refineMode)
        self.connect(self.searchBox, QtCore.SIGNAL('resetSearch()'),
                     self.resetSearch)

        tva = self.toggleViewAction()
        tva.setIcon(CurrentTheme.QUERY_VIEW_ICON)
        tva.setToolTip('Search')
        tva.setStatusTip('Search and refine the version tree')
        tva.setShortcut('Ctrl+F')

    def resetSearch(self):
        """ Reset the search of the current controller """
        if self.controller!=None:
            self.controller.set_search(None)        

    def showEvent(self, e):
        """ showEvent(e) -> None
        Make sure that we get the search focus
        
        """
        QAddonToolBar.showEvent(self, e)
        self.searchBox.setFocus(QtCore.Qt.ActiveWindowFocusReason)

    def hideEvent(self, e):
        """ hideEvent() -> None
        Make sure that the search is reset
        
        """
        QAddonToolBar.hideEvent(self, e)
        self.resetSearch()

    def executeSearch(self, text):
        """ executeSearch(text: QString) -> None
        Change the version view with query
        
        """
        s = str(text)
        if self.controller:
            try:
                search = SearchCompiler(s).searchStmt
            except SearchParseError, e:
                QtGui.QMessageBox.warning(self,
                                          QtCore.QString("Search Parse Error"),
                                          QtCore.QString(str(e)),
                                          QtGui.QMessageBox.Ok,
                                          QtGui.QMessageBox.NoButton,
                                          QtGui.QMessageBox.NoButton)
                search = None
            self.controller.set_search(search, s)

    def refineMode(self, on):
        """ refineMode(on: bool) -> None
        
        """
        if self.controller:
            self.controller.set_refine(on)

