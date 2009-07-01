
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
"""
QGeneralStatisticsPanel
QStatisticsWindow
"""


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
from PyQt4 import QtGui,QtCore
from plugin.pgui.statistics.datasource import TimeDataSource,TimeDiffDataSource
from plugin.pgui.statistics.histogram import QTimeHistogramView,QHistogramWidget


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QGeneralStatisticsPanel(QtGui.QFrame):
    """ QGeneralStatisticsPanel implements the container to hold the
    statistics view and controls to specify the source type and 
    the view type.
    """

    def __init__(self,parent=None):
        """ __init__(parent:QWidget) -> None
        Create the GUI content.
        """
        QtGui.QFrame.__init__(self,parent)
        self.setFrameStyle(QtGui.QFrame.Panel|QtGui.QFrame.Sunken)
        self.setLineWidth(2)
    
        """ global layout """
        layout=QtGui.QVBoxLayout(self)
        layout.setMargin(3)
        layout.setSpacing(3)
        self.setLayout(layout)
        
        """ spin box and layout """
        comboLayout=QtGui.QHBoxLayout()
        layout.addLayout(comboLayout)
        comboLayout.setSpacing(3)
        comboLayout.setMargin(0)

        """ source combo box """
        self.sourceComboBox=QtGui.QComboBox(self)
        self.sourceComboBox.addItem('Time')
        self.sourceComboBox.addItem('Time Difference')
        self.connect(self.sourceComboBox,QtCore.SIGNAL('currentIndexChanged(int)'),self.updateWidget)
        comboLayout.addWidget(QtGui.QLabel('Source'))
        comboLayout.addWidget(self.sourceComboBox)
        comboLayout.addStretch()
        
        """ view type combo box """
        self.typeComboBox=QtGui.QComboBox(self)
        self.typeComboBox.addItem('Histogram')
        self.connect(self.typeComboBox,QtCore.SIGNAL('currentIndexChanged(int)'),self.updateWidget)
        comboLayout.addWidget(QtGui.QLabel('View'))
        comboLayout.addWidget(self.typeComboBox)
        
        """ statistics view """
        self.counter=0
        self.statisticsWidget=None
        self.updateWidget()

    def updateData(self):
        """ updateData() -> None
        Call updateData for the statistics widget.
        """
        if self.statisticsWidget!=None:
            self.statisticsWidget.updateData()
    
    def updateWidget(self):
        """ updateWidget() -> None
        Remove the current statisticsWidget (if existing) and create a new
        one depending on the user specified source and view type.
        """

        """ clean up """
        if self.statisticsWidget!=None:
            self.layout().removeWidget(self.statisticsWidget)
            self.statisticsWidget.setParent(None)
            del self.statisticsWidget
        
        """ construct widget """
        item=self.sourceComboBox.currentText()
        if item=='Time':
            self.statisticsWidget=QHistogramWidget(QTimeHistogramView(TimeDataSource()),self)
        elif item=='Time Difference':
            self.statisticsWidget=QHistogramWidget(QTimeHistogramView(TimeDiffDataSource()),self)
        self.statisticsWidget.updateData()
        self.layout().addWidget(self.statisticsWidget)
        

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QStatisticsWindow(QtGui.QDialog):
    """ QStatisticsWindow is the main statistics dialog.
    """
    
    def __init__(self,parent=None):
        """ __init__(parent:QWidget) -> None
        Create the GUI content.
        """
        
        QtGui.QDialog.__init__(self,parent)
        self.setWindowTitle('Statistics')
        self.setModal(False)
        
        """ global layout """
        layout=QtGui.QVBoxLayout(self)
        layout.setMargin(5)
        layout.setSpacing(5)
        self.setLayout(layout)
        
        """ spin box and layout """
        spinLayout=QtGui.QHBoxLayout()
        spinLayout.setSpacing(5)
        spinLayout.setMargin(0)

        self.rowsSpinBox=QtGui.QSpinBox(self)
        self.rowsSpinBox.setRange(1,3)
        self.connect(self.rowsSpinBox,QtCore.SIGNAL('valueChanged(int)'),self.updateLayout)
        spinLayout.addWidget(QtGui.QLabel('Rows'))
        spinLayout.addWidget(self.rowsSpinBox)
        
        self.colsSpinBox=QtGui.QSpinBox(self)
        self.colsSpinBox.setRange(1,3)
        self.connect(self.colsSpinBox,QtCore.SIGNAL('valueChanged(int)'),self.updateLayout)
        spinLayout.addWidget(QtGui.QLabel('Columns'))
        spinLayout.addWidget(self.colsSpinBox)
        spinLayout.addStretch()
        layout.addLayout(spinLayout)
        
        """ content layout """
        self.contentLayout=QtGui.QGridLayout(self)
        layout.addLayout(self.contentLayout)
        self.statisticsPanels=[]
        self.updateLayout()
        
        """ update and close button """
        updateButton=QtGui.QPushButton('Update')
        self.connect(updateButton,QtCore.SIGNAL('clicked()'),self.updateData)
        closeButton=QtGui.QPushButton('Close')
        self.connect(closeButton,QtCore.SIGNAL('clicked()'),self.close)
        buttonLayout=QtGui.QHBoxLayout()
        buttonLayout.setSpacing(5)
        buttonLayout.addWidget(updateButton)
        buttonLayout.addWidget(closeButton)
        layout.addLayout(buttonLayout)
        
    def updateData(self):
        """ updateData() -> None
        Call the updateData function for each panel.
        """
        for panel in self.statisticsPanels:
            panel.updateData()

    def updateLayout(self):
        """ updateLayout() -> None
        Construct the individual statistics panels depending on the
        user specified number of rows and columns in the spin boxes.
        """
        
        """ clean """
        for panel in self.statisticsPanels:
            self.contentLayout.removeWidget(panel)
            panel.setParent(None)
            del panel
            
        """ create """
        self.statisticsPanels=[]
        for row in range(self.rowsSpinBox.value()):
            for col in range(self.colsSpinBox.value()):
                panel=QGeneralStatisticsPanel(self)
                self.statisticsPanels.append(panel)
                self.contentLayout.addWidget(panel,row,col)

        """ adjust gridlayout stretch """
        for row in range(self.rowsSpinBox.value()):
            self.contentLayout.setRowStretch(row,1)
        for col in range(self.colsSpinBox.value()):
            self.contentLayout.setColumnStretch(col,1)

        """ update """
        self.updateData()
