
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
from gui.utils import getBuilderWindow
import CaptureAPI

class QPluginPreferencesDialog(QtGui.QDialog):
    """ Build the GUI for Preferences class """
    
    def __init__(self, parent=None):
        """ Construct a simple dialog layout """
        QtGui.QDialog.__init__(self, parent)
        self.setWindowTitle('Provenance Explorer Preferences')
        layout = QtGui.QVBoxLayout(self)
        self.setLayout(layout)

        # The number of visible versions edit box
        novLayout = QtGui.QHBoxLayout()
        layout.addLayout(novLayout)
        novLayout.setMargin(0)

        novLabel = QtGui.QLabel('Number of recent versions visible')
        novLayout.addWidget(novLabel)
        
        self.numberOfVisibleVersionsSB = QtGui.QSpinBox()
        novLayout.addWidget(self.numberOfVisibleVersionsSB)
        self.numberOfVisibleVersionsSB.setRange(0, 1000)
        self.numberOfVisibleVersionsSB.setValue(
            int(CaptureAPI.getPreference('VisTrailsNumberOfVisibleVersions')))
        novLayout.addStretch()

        layout.addSpacing(10)

        # Enabling snapshots
        self.snapShotCB = QtGui.QCheckBox('Take state snapshots')
        state = int(CaptureAPI.getPreference('VisTrailsSnapshotEnabled'))
        if state:
            self.snapShotCB.setCheckState(QtCore.Qt.Checked)
        else:
            self.snapShotCB.setCheckState(QtCore.Qt.Unchecked)
        layout.addWidget(self.snapShotCB)

        # The number of actions before a snapshot    
        nosLayout = QtGui.QHBoxLayout()
        layout.addLayout(nosLayout)
        nosLayout.setMargin(0)

        nosLabel = QtGui.QLabel('Number of actions between snapshots')
        nosLayout.addWidget(nosLabel)
        
        self.numSnapshotTB= QtGui.QSpinBox()
        nosLayout.addWidget(self.numSnapshotTB)
        self.numSnapshotTB.setRange(0,1000)
        self.numSnapshotTB.setValue(
            int(CaptureAPI.getPreference('VisTrailsSnapshotCount')))
        nosLayout.addStretch()

        layout.addSpacing(10)
        # Enabling snapshots
        self.fileStoreCB = QtGui.QCheckBox('Store opened and imported files in vistrail')
        state = int(CaptureAPI.getPreference('VisTrailsStoreFiles'))
        if state:
            self.fileStoreCB.setCheckState(QtCore.Qt.Checked)
        else:
            self.fileStoreCB.setCheckState(QtCore.Qt.Unchecked)
        layout.addWidget(self.fileStoreCB)

        # A space
        layout.addStretch()

        # Then the buttons
        bLayout = QtGui.QHBoxLayout()
        layout.addLayout(bLayout)
        bLayout.addStretch()

        self.saveButton = QtGui.QPushButton('Save')
        bLayout.addWidget(self.saveButton)
        
        self.cancelButton = QtGui.QPushButton('Cancel')
        bLayout.addWidget(self.cancelButton)

        # Connect buttons to dialog handlers
        self.connect(self.saveButton, QtCore.SIGNAL('clicked()'),
                     self.accept)
        self.connect(self.cancelButton, QtCore.SIGNAL('clicked()'),
                     self.reject)

        controller = getBuilderWindow().viewManager.currentWidget().controller
        controller.set_num_versions_always_shown(self.numberOfVisibleVersionsSB.value())

    def sizeHint(self):
        return QtCore.QSize(384, 128)

    def accept(self):
        """ Need to save the preferences """
        QtGui.QDialog.accept(self)
        self.savePreferences()

    def savePreferences(self):
        """ Map all widget values back to App Preferences """
        num_versions = self.numberOfVisibleVersionsSB.value()
        num_snapshot = self.numSnapshotTB.value()
        snap_enabled = 1
        if self.snapShotCB.checkState() == QtCore.Qt.Unchecked:
            snap_enabled = 0
        file_store = 1
        if self.fileStoreCB.checkState() == QtCore.Qt.Unchecked:
            file_store = 0
        controller = getBuilderWindow().viewManager.currentWidget().controller
        controller.set_num_versions_always_shown(num_versions)
        CaptureAPI.setPreference('VisTrailsNumberOfVisibleVersions',
                                 str(num_versions))
        CaptureAPI.setPreference('VisTrailsSnapshotCount',
                                 str(num_snapshot))
        CaptureAPI.setPreference('VisTrailsSnapshotEnabled',
                                 str(snap_enabled))
        CaptureAPI.setPreference('VisTrailsStoreFiles',
                                 str(file_store))
