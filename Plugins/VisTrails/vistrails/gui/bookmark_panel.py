
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

""" This file contains widgets related to the bookmark palette
displaying a list of all user's bookmarks in Vistrails

QBookmarkPanel
QBookmarkPalette
QBookmarkTreeWidget
QBookmarkTreeWidgetItem
"""

from PyQt4 import QtCore, QtGui
from gui.theme import CurrentTheme
from gui.common_widgets import (QSearchTreeWindow,
                                QSearchTreeWidget,
                                QToolWindowInterface)

################################################################################
class QBookmarkPanel(QtGui.QFrame, QToolWindowInterface):
    """
    QBookmarkPanel contains a Bookmark palette and a toolbar for interacting
    with the bookmarks.
    
    """
    def __init__(self, parent=None, manager=None):
        """ QBookmarkPanel(parent: QWidget) -> QBookmarkPanel
        Creates a panel with a palette of bookmarks and a toolbar.
        Set bookmark manager
        
        """
        QtGui.QFrame.__init__(self, parent)
        self.setFrameStyle(QtGui.QFrame.Panel|QtGui.QFrame.Sunken)
        self.manager = manager
        layout = QtGui.QVBoxLayout()
        self.setLayout(layout)
        self.layout().setMargin(0)
        self.layout().setSpacing(0)
        self.bookmarkPalette = QBookmarkPalette(None, self.manager)
        self.createActions()
        self.createToolBar()
        layout.addWidget(self.bookmarkPalette, 1)
        self.setWindowTitle('Bookmarks')
        
        self.connectSignals()
        
    def createActions(self):
        """ createActions() -> None
        Construct all menu/toolbar actions for bookmarks  window
        
        """
        self.executeAction = QtGui.QAction(CurrentTheme.EXECUTE_PIPELINE_ICON,
                                       'Execute', self)
        self.executeAction.setToolTip('Execute checked bookmarks')
        self.executeAction.setStatusTip(self.executeAction.toolTip())
        self.executeAction.setEnabled(False)

        self.removeAction = QtGui.QAction(CurrentTheme.BOOKMARKS_REMOVE_ICON,
                                          'Remove', self)
        self.removeAction.setToolTip('Remove selected bookmark')
        self.removeAction.setStatusTip(self.removeAction.toolTip())
        self.reloadAction = QtGui.QAction(CurrentTheme.BOOKMARKS_RELOAD_ICON,
                                          'Reload', self)
        self.reloadAction.setToolTip('Reload selected bookmark with ' 
                                     'original values')
        self.reloadAction.setStatusTip(self.reloadAction.toolTip())
        
    def createToolBar(self):
        """ createToolBar() -> None
        Create a default toolbar for bookmarks panel
        
        """
        self.toolBar = QtGui.QToolBar()
        self.toolBar.setIconSize(QtCore.QSize(32,32))
        self.toolBar.setWindowTitle('Bookmarks controls')
        self.layout().addWidget(self.toolBar)
        self.toolBar.addAction(self.executeAction)
        wdgt = self.toolBar.widgetForAction(self.executeAction)
        wdgt.setFocusPolicy(QtCore.Qt.ClickFocus)
        self.toolBar.addAction(self.removeAction)
        wdgt = self.toolBar.widgetForAction(self.removeAction)
        wdgt.setFocusPolicy(QtCore.Qt.ClickFocus)
        self.toolBar.addAction(self.reloadAction)
        wdgt = self.toolBar.widgetForAction(self.reloadAction)
        wdgt.setFocusPolicy(QtCore.Qt.ClickFocus)
        
    def connectSignals(self):
        """ connectSignals() -> None
        Map signals between  GUI components        
        
        """
        self.connect(self.executeAction,
                     QtCore.SIGNAL('triggered(bool)'),
                     self.bookmarkPalette.executeCheckedWorkflows)
        self.connect(self.removeAction,
                     QtCore.SIGNAL('triggered(bool)'),
                     self.bookmarkPalette.treeWidget.removeCurrentItem)
        self.connect(self.reloadAction,
                     QtCore.SIGNAL('triggered(bool)'),
                     self.bookmarkPalette.treeWidget.reloadSelectedBookmark)
        self.connect(self.bookmarkPalette,
                     QtCore.SIGNAL('checkedListChanged'),
                     self.checkedListChanged)

    def checkedListChanged(self, num):
        """checkedListChanged(num: int) -> None
        Updates the buttons state according to the number of checked items

        """
        self.executeAction.setEnabled(num>0)

    def updateBookmarkPalette(self):
        """updateBookmarkPalette() -> None
        Reload bookmarks from the collection
        
        """
        self.bookmarkPalette.treeWidget.updateFromBookmarkCollection()

class QBookmarkPalette(QSearchTreeWindow):
    """
    QBookmarkPalette just inherits from QSearchTreeWindow to have its
    own type of tree widget

    """
    def __init__(self, parent=None, manager=None):
        """ QBookmarkPalette(parent: QWidget) -> QBookmarkPalette
        Set bookmark manager
        """
        QSearchTreeWindow.__init__(self, parent)
        self.manager = manager
        self.checkedList = []
        self.connect(self.treeWidget,
                     QtCore.SIGNAL("itemChanged(QTreeWidgetItem*,int)"),
                     self.processItemChanges)
        self.connect(self.treeWidget,
                     QtCore.SIGNAL("itemRemoved(int)"),
                     self.remove_bookmark)
        self.connect(self.treeWidget,
                     QtCore.SIGNAL("reloadBookmark"),
                     self.reloadBookmark)
        
    def createTreeWidget(self):
        """ createTreeWidget() -> QBookmarkTreeWidget
        Return the search tree widget for this window
        
        """
        return QBookmarkTreeWidget(self)

    def executeCheckedWorkflows(self):
        """ executeCheckedWorkflows() -> None
        get the checked bookmark ids and send them to the manager

        """
        if len(self.checkedList) > 0:
            self.manager.execute_workflows(self.checkedList)

    def processItemChanges(self, item, col):
        """processItemChanges(item: QBookmarkTreeWidgetItem, col:int)
        Event handler for propagating bookmarking changes to the collection. 
        
        """
        if col == 0 and item != self.treeWidget.headerItem():
            if item.bookmark.type == 'item':
                id = item.bookmark.id
                if item.checkState(0) ==QtCore.Qt.Unchecked:
                    if id in self.checkedList:
                        self.checkedList.remove(id)
                        self.manager.set_active_pipelines(self.checkedList)
                elif item.checkState(0) ==QtCore.Qt.Checked:
                    if id not in self.checkedList:
                        self.checkedList.append(id)
                        self.manager.set_active_pipelines(self.checkedList)
                self.emit(QtCore.SIGNAL("checkedListChanged"),
                          len(self.checkedList))
            if str(item.text(0)) != item.bookmark.name:
                item.bookmark.name = str(item.text(0))
                self.manager.write_bookmarks()
    
    def remove_bookmark(self, id):
        """remove_bookmark(id: int) -> None 
        Tell manager to remove bookmark with id 

        """
        if id in self.checkedList:
            self.checkedList.remove(id)
            self.manager.set_active_pipelines(self.checkedList)
        self.manager.remove_bookmark(id)
    
    def reloadBookmark(self, id):
        """reloadBookmark(id : int) -> None 
        Resets the pipeline of the selected bookmark

        """
        if id in self.checkedList:
            self.checkedList.remove(id)
            self.manager.set_active_pipelines(self.checkedList)
        self.manager.reload_pipeline(id)

class QBookmarkTreeWidget(QSearchTreeWidget):
    """
    QBookmarkTreeWidget is a subclass of QSearchTreeWidget to display all
    user's Vistrails Bookmarks 
    
    """
    def __init__(self, parent=None):
        """ __init__(parent: QWidget) -> QBookmarkTreeWidget
        Set up size policy and header

        """
        QSearchTreeWidget.__init__(self, parent)
        self.setColumnCount(1)
        self.header().setResizeMode(QtGui.QHeaderView.ResizeToContents)
        self.header().hide()
        self.connect(self, 
                     QtCore.SIGNAL("itemSelectionChanged()"),
                     self.checkButtons)
    
    def checkButtons(self):
        if len(self.selectedItems()) == 0:
            self.parent().parent().removeAction.setEnabled(False)
        else:
            self.parent().parent().removeAction.setEnabled(True)

    def updateFromBookmarkCollection(self):
        """ updateFromBookmarkCollection() -> None        
        Setup this tree widget to show bookmarks currently inside
        BookmarksManager.bookmarks
        
        """
        def createBookmarkItem(parentFolder, bookmark):
            """ createBookmarkItem(parentFolder: QBookmarkTreeWidgetItem,
                                   bookmark: BookmarkTree) 
                                                    -> QBookmarkTreeWidgetItem
            Traverse a bookmark to create items recursively. Then return
            its bookmark item
            
            """
            labels = QtCore.QStringList()
            bObj = bookmark.bookmark
            labels << bObj.name
            bookmarkItem = QBookmarkTreeWidgetItem(bookmark.bookmark,
                                                   parentFolder,
                                                   labels)
            if bObj.type == "item":
                if bObj.error == 0:
                    bookmarkItem.setToolTip(0,bObj.filename)
                    bookmarkItem.setCheckState(0,QtCore.Qt.Unchecked)
                else:
                    if bObj.error == 1:
                        tooltip = "File not found: %s" % bObj.filename
                    elif bObj.error == 2:
                        msg = "Couldn't find version %s in %s"
                        tooltip =  msg % (bObj.pipeline, bObj.filename)
                        
                    bookmarkItem.setToolTip(0,tooltip)
                    bookmarkItem.setIcon(0,
                                         QtGui.QIcon(CurrentTheme.EXPLORE_SKIP_PIXMAP))
            
            for child in bookmark.children:
                createBookmarkItem(bookmarkItem, child)
            
        if self.parent().manager:
            self.clear()
            bookmark = self.parent().manager.collection.bookmarks
            createBookmarkItem(self, bookmark)
            self.sortItems(0,QtCore.Qt.AscendingOrder)
            self.expandAll()

    def keyPressEvent(self, event):
        if (event.key() == QtCore.Qt.Key_Delete or 
            event.key() == QtCore.Qt.Key_Backspace):
            if self.isVisible():      
                self.removeCurrentItem()
                event.accept()
            else:
                event.ignore()
        else:
            event.ignore()

    def removeCurrentItem(self):
        """removeCurrentItem() -> None Removes from the GUI and Collection """
        item = self.currentItem()
        if item and item.parent():
            parent = item.parent()
            parent.takeChild(parent.indexOfChild(item))
            id = item.bookmark.id
            del item
            self.emit(QtCore.SIGNAL("itemRemoved(int)"),id)
    
    def reloadSelectedBookmark(self):
        """ reloadSelectedBookmark() -> None
        uncheck selected item and propagates the signal with the item id

        """
        item = self.currentItem()
        if item and item != self.headerItem():
            id = item.bookmark.id
            item.setCheckState(0,QtCore.Qt.Unchecked)
            self.emit(QtCore.SIGNAL("reloadBookmark"), id)
               

class QBookmarkTreeWidgetItem(QtGui.QTreeWidgetItem):
    """
    QBookmarkTreeWidgetItem represents bookmark on QBookmarkTreeWidget
    
    """
    def __init__(self, bookmark, parent, labelList):
        """ QBookmarkTreeWidgetItem(bookmark: Bookmark,
                                  parent: QTreeWidgetItem
                                  labelList: QStringList)
                                  -> QBookmarkTreeWidget                                  
        Create a new tree widget item with a specific parent and
        labels

        """
        QtGui.QTreeWidgetItem.__init__(self, parent, labelList)
        self.bookmark = bookmark
        self.setFlags((QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsSelectable
                       | QtCore.Qt.ItemIsEnabled | 
                       QtCore.Qt.ItemIsUserCheckable))
