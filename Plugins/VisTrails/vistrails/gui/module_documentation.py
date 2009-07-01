
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
""" This file contains a dialog and widgets related to the module documentation
dialog, which displays the available documentation for a given VisTrails module.

QModuleDocumentation
"""

from PyQt4 import QtCore, QtGui
from core.modules.module_registry import registry

################################################################################

class QModuleDocumentation(QtGui.QDialog):
    """
    QModuleDocumentation is a dialog for showing module documentation. duh.

    """
    def __init__(self, descriptor, parent=None):
        """ 
        QModuleAnnotation(descriptor: ModuleDescriptor, parent)
        -> None

        """
        QtGui.QDialog.__init__(self, parent)
        self.descriptor = descriptor
        self.setModal(True)
        self.setWindowTitle('Documentation for "%s"' % descriptor.name)
        self.setLayout(QtGui.QVBoxLayout())
        self.layout().addStrut(600)
        self.layout().addWidget(QtGui.QLabel("Module name: %s" % descriptor.name))
        package = descriptor.module_package()
        self.layout().addWidget(QtGui.QLabel("Module package: %s" % package))
        self.closeButton = QtGui.QPushButton('Ok', self)
        self.textEdit = QtGui.QTextEdit(self)
        self.layout().addWidget(self.textEdit, 1)
        if descriptor.module.__doc__:
            self.textEdit.insertPlainText(descriptor.module.__doc__)
        else:
            self.textEdit.insertPlainText("Documentation not available.")
        self.textEdit.setReadOnly(True)
        self.textEdit.setTextCursor(QtGui.QTextCursor(self.textEdit.document()))
        self.layout().addWidget(self.closeButton)
        self.connect(self.closeButton, QtCore.SIGNAL('clicked(bool)'), self.close)
        self.closeButton.setShortcut('Enter')
