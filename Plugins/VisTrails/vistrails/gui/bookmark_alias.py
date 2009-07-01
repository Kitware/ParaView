
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
""" This file contains widgets related to the panel 
displaying a list of common aliases in user's bookmarks

QBookmarkAliasPanel
QAliasTable
QAliasTableItem

"""
from PyQt4 import QtCore, QtGui
from gui.theme import CurrentTheme
from gui.common_widgets import (QSearchTreeWindow,
                                QSearchTreeWidget,
                                QToolWindowInterface)

################################################################################
class QBookmarkAliasPanel(QtGui.QFrame, QToolWindowInterface):
    """
    QBookmarkAliasPanel shows common aliases to be manipulated.
    
    """
    def __init__(self, parent=None, manager=None):
        """ QBookmarkAliasPanel(parent: QWidget, manaager: BookmarksManager) 
                             -> QBookmarkAliasPanel
        Initializes the panel and sets bookmark manager
        
        """
        QtGui.QFrame.__init__(self, parent)
        self.setFrameStyle(QtGui.QFrame.Panel|QtGui.QFrame.Sunken)
        self.manager = manager
        layout = QtGui.QVBoxLayout()
        self.setLayout(layout)
        self.layout().setMargin(0)
        self.layout().setSpacing(0)
        self.parameters = QAliasTable(None, self.manager)
        layout.addWidget(self.parameters, 1)
        self.setWindowTitle('Parameters')
        
    def updateAliasTable(self, aliases):
        """updateAliasTable(aliases:dict) -> None
        Reload aliases from the ensemble
        
        """
        self.parameters.updateFromEnsemble(aliases)

class QAliasTable(QtGui.QTableWidget):
    """
    QAliasTable just inherits from QTableWidget to have a customized
    table and items

    """
    def __init__(self, parent=None, manager=None):
        """ QAliasTable(parent: QWidget, manager:BookmarkManager,
                        rows: int, cols: int) -> QAliasTable
        Set bookmark manager
        """
        QtGui.QTableWidget.__init__(self, parent)
        self.manager = manager
        self.aliases = {}
        self.setColumnCount(1)

        self.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)
        self.setSelectionMode(QtGui.QAbstractItemView.SingleSelection)
        self.setDragEnabled(True)
        self.horizontalHeader().setStretchLastSection(True)
        self.horizontalHeader().hide()
        self.connect(self,
                     QtCore.SIGNAL("cellChanged(int,int)"),
                     self.processCellChanges)
        self.connect(self.verticalHeader(),
                     QtCore.SIGNAL("sectionClicked(int)"),
                     self.updateFocus)
        
    def processCellChanges(self, row, col):
        """ processCellChanges(row: int, col: int) -> None
        Event handler for capturing when the contents in a cell changes

        """
        if col == 0:
            value = str(self.item(row,col).text())
            alias = str(self.verticalHeaderItem(row).text())
            self.manager.update_alias(alias, value)
        
    def updateFromEnsemble(self, aliases):
        """ updateFromEnsemble(aliases: dict) -> None        
        Setup this table to show common aliases
        
        """
        def createAliasRow(alias, type, value):
            """ createAliasRow( alias: str, type: str, value: str) -> None
            Creates a row in the table
            
            """
            row = self.rowCount()
            self.insertRow(row)
            item = QAliasTableWidgetItem(alias, type, alias)
            item.setFlags(QtCore.Qt.ItemIsEnabled)
            self.setVerticalHeaderItem(row,item)

            item = QAliasTableWidgetItem(alias,type, value)
            item.setFlags(QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsEditable \
                          | QtCore.Qt.ItemIsSelectable | QtCore.Qt.ItemIsDragEnabled)
            self.setItem(row, 0, item)
            
        if self.manager:
            #check if an alias was removed:
            for alias in self.aliases.keys():
                if not aliases.has_key(alias):
                    row = self.getItemRow(alias)
                    self.removeRow(row)
                    self.emit(QtCore.SIGNAL("rowRemoved"),alias)
            newAliases = {}
            for alias, info in aliases.iteritems():
                if not self.aliases.has_key(alias):
                    type = info[0]
                    value = info[1][0]
                    createAliasRow(alias,type,value)
                else:
                    row = self.getItemRow(alias)
                    self.processCellChanges(row, 0)
                newAliases[alias] = info
            self.aliases = newAliases

    def updateFocus(self, row):
        """ updateFocus(row: int) -> None
        Set focus to the alias cell when clicking on the header cell 

        """
        self.setCurrentCell(row,0)

    def mimeData(self, itemList):
        """ mimeData(itemList) -> None        
        Setup the mime data to contain itemList 
        
        """
        data = QtGui.QTableWidget.mimeData(self, itemList)
        data.aliases = itemList
        return data
        
    def getItemRow(self, alias):
        for i in xrange(self.rowCount()):
            item = self.item(i,0)
            if item:
                if item.alias == alias:
                    return i
        return -1

class QAliasTableWidgetItem(QtGui.QTableWidgetItem):
    """
    QAliasTableWidgetItem represents alias on QAliasTableWidget
    
    """
    def __init__(self, alias, type, text):
        """ QAliasTableWidgetItem(alias: str, type: str, text: str): 
        Create a new table widget item with alias and text

        """
        QtGui.QTableWidgetItem.__init__(self, text)
        self.alias = alias
        self.type = type
    
