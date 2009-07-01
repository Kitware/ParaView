
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
from PyQt4 import QtCore, QtGui
from gui.theme import CurrentTheme
import core.system

class QFileReferencesDialog(QtGui.QDialog):
    """
    """
    def __init__(self, filePath, dialogFlags, defaultDir=''):
        from gui.utils import getBuilderWindow
        parent=getBuilderWindow()

        flags = QtCore.Qt.WindowStaysOnTopHint
        modal = True
        if core.system.systemType in ['Darwin']:
            flags = flags | QtCore.Qt.Sheet
            modal = False
        else:
            flags = flags | QtCore.Qt.MSWindowsFixedSizeDialogHint

        QtGui.QDialog.__init__(self, parent, flags)
        self.setModal(modal)
        self.setSizeGripEnabled(False)
        self.textLayout = QtGui.QHBoxLayout()
        self.appIcon = QtGui.QLabel(self)
        self.appIcon.setPixmap(CurrentTheme.APPLICATION_PIXMAP)
        self.textLayout.addWidget(self.appIcon)

        self.textContent=QtGui.QVBoxLayout()
        self.textLayout.addLayout(self.textContent)

        self.textContent.addWidget(QtGui.QLabel('The file',self))

        label=QtGui.QLabel(filePath,self)
        self.textContent.addWidget(label)
        if len(filePath)>80:
            label.setWordWrap(True)
        font=QtGui.QFont(label.font())
        font.setFamily('Courier')
        label.setFont(font)

        #label=QtGui.QPlainTextEdit(filePath,self)
        #self.textContent.addWidget(label)
        #label.setReadOnly(True)
        #if len(filePath)>80:
        #    label.setLineWrapMode(QPlainTextEdit.WidgetWidth)
        #else:
        #    label.setLineWrapMode

        self.textContent.addWidget(\
          QtGui.QLabel('is referenced but could not be found.'))

        self.buttonLayout=QtGui.QHBoxLayout()
        self.buttonLayout.addStretch()

        browseButton=QtGui.QPushButton('Browse File',self)
        browseButton.setDefault(True)
        self.buttonLayout.addWidget(browseButton)
        self.connect(browseButton,QtCore.SIGNAL('clicked()'),
                     self.browseButtonPressed)

        if dialogFlags&2:
            browseDirButton=QtGui.QPushButton('Browse Directory', self)
            self.buttonLayout.addWidget(browseDirButton)
            self.connect(browseDirButton,QtCore.SIGNAL('clicked()'),
                         self.browseDirButtonPressed)

        ignoreButton=QtGui.QPushButton('Ignore Missing File',self)
        self.buttonLayout.addWidget(ignoreButton)
        self.connect(ignoreButton,QtCore.SIGNAL('clicked()'),
                     self.ignoreButtonPressed)

        if dialogFlags&1:
            ignoreAllButton=QtGui.QPushButton('Ignore All', self)
            self.buttonLayout.addWidget(ignoreAllButton)
            self.connect(ignoreAllButton,QtCore.SIGNAL('clicked()'),
                         self.ignoreAllButtonPressed)

        #cancelButton=QtGui.QPushButton('Cancel',self)
        #self.buttonLayout.addWidget(cancelButton)
        #self.connect(cancelButton,QtCore.SIGNAL('clicked()'),
        #             self.cancelButtonPressed)

        self.dialogLayout = QtGui.QVBoxLayout()
        self.dialogLayout.addLayout(self.textLayout)
        self.dialogLayout.addLayout(self.buttonLayout)
        self.setLayout(self.dialogLayout)

        self.filePath=filePath
        self.defaultDir=defaultDir
        self.result=None
        self.resultPath=None

    def browseButtonPressed(self):
        """ browseButtonPressed() -> None

        """
        self.result='browse'

    def browseDirButtonPressed(self):
        """ browseButtonPressed() -> None

        """
        self.result='browseDir'

    def ignoreButtonPressed(self):
        """ ignoreButtonPressed() -> None

        """
        self.result='ignore'

    def ignoreAllButtonPressed(self):
        """ ignoreAllButtonPressed() -> None

        """
        self.result='ignoreAll'

    #def cancelButtonPressed(self):
    #    """ thirdButtonPressed() -> None
    #
    #    """
    #    self.result='cancel'

    def execute(self):
        """ execute() -> int

        """
        self.show()
        while self.result==None:
            while self.result==None:
                QtCore.QCoreApplication.processEvents()

            if self.result[:6]=='browse':
                import os.path
                (head,tail)=os.path.split(self.filePath)
                if (head==None) or (head==''):
                    head=self.defaultDir
                if self.result=='browseDir':
                    self.resultPath=QtGui.QFileDialog.getExistingDirectory(self,'Select Directory',self.defaultDir)
                else:
                    self.resultPath=QtGui.QFileDialog.getOpenFileName(self,'Select File',head);
                if self.resultPath=='':
                    self.result=None
                else:
                    self.result='ok'

        self.destroy()
        if self.result=='ok':
            return (self.result,self.resultPath)
        else:
            return self.result
