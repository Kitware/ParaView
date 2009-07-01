
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

""" File for Interacting with Bookmarks, a more user-friendly way of 
using Vistrails

QBookmarksWindow
BookmarksManager
PipelineSceneInterface

"""

from PyQt4 import QtCore, QtGui
from gui.bookmark_panel import QBookmarkPanel
from gui.bookmark_alias import QBookmarkAliasPanel
from gui.bookmark_explore import QAliasExplorationPanel
from gui.vistrail_controller import VistrailController
from core.param_explore import InterpolateDiscreteParam, ParameterExploration
from core.bookmark import (Bookmark, 
                           BookmarkCollection, 
                           BookmarkController)
    
################################################################################

class QBookmarksWindow(QtGui.QMainWindow):
    """
    QBookmarksWindow is a main widget containing several tool windows
    for interacting with bookmarked vistrails pipelines without having to use 
    the builder.
    It is based on QBuilderWindow visual style.
        
    """
    def __init__(self, parent=None):
        """ __init__(parent: QWidget) -> QBookmarksWindow
        Construct a window with menus, toolbar, and floating toolwindow
        
        """
        QtGui.QMainWindow.__init__(self, parent)
        
        self.setWindowTitle('VisTrails Bookmarks')
        self.setStatusBar(QtGui.QStatusBar(self))
        self.setDockNestingEnabled(True)

        self.bookmarkPanel = QBookmarkPanel(self,BookmarksManager)
        self.addDockWidget(QtCore.Qt.LeftDockWidgetArea,
                           self.bookmarkPanel.toolWindow())

        self.bookmarkAliasPanel = QBookmarkAliasPanel(self,BookmarksManager)
        self.addDockWidget(QtCore.Qt.RightDockWidgetArea,
                           self.bookmarkAliasPanel.toolWindow())

        self.aliasExplorePanel = QAliasExplorationPanel(self,BookmarksManager)
        self.addDockWidget(QtCore.Qt.RightDockWidgetArea,
                           self.aliasExplorePanel.toolWindow())
        
        self.createActions()
        self.createMenu()
        self.createToolBar()

        self.connectSignals()
        self.setVisible(False)

    def sizeHint(self):
        """ sizeHint() -> QRect
        Return the recommended size of the bookmarks window
        
        """
        return QtCore.QSize(800, 600)

    def keyPressEvent(self, event):
        if (event.key() == QtCore.Qt.Key_Enter or 
            event.key() == QtCore.Qt.Key_Return) and \
            (event.modifiers() & QtCore.Qt.ControlModifier):
            if self.bookmarkPanel.isVisible():          
                self.bookmarkPanel.executeAction.trigger()

    def createActions(self):
        """ createActions() -> None
        Construct all menu/toolbar actions for bookmarks panel
        
        """
        pass
                
    def createMenu(self):
        """ createMenu() -> None
        Initialize menu bar of bookmarks window
        
        """
        self.viewMenu = self.menuBar().addMenu('&View')
        self.viewMenu.addAction(
            self.bookmarkPanel.toolWindow().toggleViewAction())
        self.viewMenu.addAction(
            self.bookmarkAliasPanel.toolWindow().toggleViewAction())
        self.viewMenu.addAction(
            self.aliasExplorePanel.toolWindow().toggleViewAction())
        
    def createToolBar(self):
        """ createToolBar() -> None
        Create a default toolbar for bookmarks window
        
        """
        pass

    def closeEvent(self, e):
        """closeEvent(e) -> None
        Event handler called when the window is about to close."""
        self.hide()
        self.emit(QtCore.SIGNAL("bookmarksHidden()"))

    def showEvent(self, e):
        """showEvent(e: QShowEvent) -> None
        Event handler called immediately before the window is shown.
        Checking if the bookmarks list is sync with the Collection """
        if BookmarksManager.collection.updateGUI:
            self.bookmarkPanel.updateBookmarkPalette()
            BookmarksManager.collection.updateGUI = False

    def connectSignals(self):
        """ connectSignals() -> None
        Map signals between various GUI components        
        
        """
        self.connect(BookmarksManager, 
                     QtCore.SIGNAL("updateAliasGUI"),
                     self.bookmarkAliasPanel.updateAliasTable)
        self.connect(BookmarksManager, 
                     QtCore.SIGNAL("updateBookmarksGUI"),
                     self.bookmarkPanel.updateBookmarkPalette)
        self.connect(self.bookmarkAliasPanel.parameters,
                     QtCore.SIGNAL("rowRemoved"),
                     self.aliasExplorePanel.removeAlias)
        
################################################################################

class BookmarksManagerSingleton(QtCore.QObject, BookmarkController):
    def __init__(self):
        """__init__() -> BookmarksManagerSingleton
        Creates Bookmarks manager 
        """
        QtCore.QObject.__init__(self)
        BookmarkController.__init__(self)

    def __call__(self):
        """ __call__() -> BookmarksManagerSingleton
        Return self for calling method
        
        """
        return self

    def add_bookmark(self, parent, vistrailsFile, pipeline, name=''):
        """add_bookmark(parent: int, vistrailsFile: str, pipeline: int,
                       name: str) -> None
        creates a bookmark with the given information and adds it to the 
        collection

        """
        BookmarkController.add_bookmark(self, parent, vistrailsFile, 
                                       pipeline, name)
        self.emit(QtCore.SIGNAL("updateBookmarksGUI"))
        self.collection.updateGUI = False
        
    def load_pipeline(self, id):
        """load_pipeline(id: int) -> None
        Given a bookmark id, loads its correspondent pipeline and include it in
        the ensemble 

        """
        BookmarkController.load_pipeline(self,id)
        self.emit(QtCore.SIGNAL("updateAliasGUI"), self.ensemble.aliases)

    def load_all_pipelines(self):
        """load_all_pipelines() -> None
        Load all bookmarks' pipelines and sets an ensemble 

        """
        BookmarkController.load_all_pipelines(self)
        self.emit(QtCore.SIGNAL("updateAliasGUI"), self.ensemble.aliases)

    def set_active_pipelines(self, ids):
        """ set_active_pipelines(ids: list) -> None
        updates the list of active pipelines 
        
        """
        BookmarkController.set_active_pipelines(self, ids)
        self.emit(QtCore.SIGNAL("updateAliasGUI"), self.ensemble.aliases)
        
###############################################################################

def initBookmarks(filename):
    """initBookmarks(filename: str) -> None 
    Sets BookmarksManager's filename and tells it to load bookmarks from this
    file

    """
    BookmarksManager.filename = filename
    BookmarksManager.load_bookmarks()


#singleton technique
BookmarksManager = BookmarksManagerSingleton()
del BookmarksManagerSingleton
