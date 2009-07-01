
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

QMethodDocumentation
"""

from PyQt4 import QtCore, QtGui
from core.modules.module_registry import registry
from core.vistrail.port import PortEndPoint
from core.utils import VistrailsInternalError

class QPortDocumentation(QtGui.QDialog):
    """
    QPortDocumentation is a dialog for showing port documentation. duh.

    """
    def __init__(self, descriptor, endpoint, port_name, parent=None):
        QtGui.QDialog.__init__(self, parent)
        self.descriptor = descriptor
        self.setModal(True)
        if endpoint == PortEndPoint.Source:
            port_type = 'output'
            call_ = descriptor.module.provide_output_port_documentation
        elif endpoint == PortEndPoint.Destination:
            port_type = 'input'
            call_ = descriptor.module.provide_input_port_documentation
        else:
            raise VistrailsInternalError("Invalid port type")
        self.setWindowTitle('Documentation for %s port %s in "%s"' %
                            (port_type, port_name, descriptor.name))
        self.setLayout(QtGui.QVBoxLayout())
        self.layout().addStrut(600)
        self.layout().addWidget(QtGui.QLabel("Port name: %s" % port_name))
        self.layout().addWidget(QtGui.QLabel("Module name: %s" % descriptor.name))
        package = descriptor.module_package()
        self.layout().addWidget(QtGui.QLabel("Module package: %s" % package))
        self.closeButton = QtGui.QPushButton('Ok', self)
        self.textEdit = QtGui.QTextEdit(self)
        self.layout().addWidget(self.textEdit, 1)
        doc = call_(port_name)
        if doc:
            self.textEdit.insertPlainText(doc)
        else:
            self.textEdit.insertPlainText("Documentation not available.")
        self.textEdit.setReadOnly(True)
        self.textEdit.setTextCursor(QtGui.QTextCursor(self.textEdit.document()))
        self.layout().addWidget(self.closeButton)
        self.connect(self.closeButton, QtCore.SIGNAL('clicked(bool)'), self.close)
        self.closeButton.setShortcut('Enter')
