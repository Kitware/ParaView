
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
""" This file contains widgets related to the method palette
displaying a list of all functions available in a module

QMethodPalette
QMethodTreeWidget
QMethodTreeWidgetItem
"""

from PyQt4 import QtCore, QtGui
from gui.common_widgets import (QSearchTreeWindow,
                                QSearchTreeWidget,
                                QToolWindowInterface)
from core.modules.module_registry import registry
from core.vistrail.port import PortEndPoint
from gui.port_documentation import QPortDocumentation
################################################################################

class QMethodPalette(QSearchTreeWindow, QToolWindowInterface):
    """
    QModulePalette just inherits from QSearchTreeWindow to have its
    own type of tree widget

    """
    def __init__(self, parent=None):
        QSearchTreeWindow.__init__(self, parent)
        
        tagLayout = QtGui.QHBoxLayout()
        tagLayout.setMargin(0)
        tagLayout.setSpacing(2)
        tagLabel = QtGui.QLabel('Tag:', self)
        tagLayout.addWidget(tagLabel)
        self.tagEdit = QtGui.QLineEdit()
        tagLabel.setBuddy(self.tagEdit)
        tagLayout.addWidget(self.tagEdit)
        self.layout().insertLayout(0, tagLayout)

        self.connect(self.tagEdit, QtCore.SIGNAL('editingFinished()'),
                     self.tagFinished)

    def createTreeWidget(self):
        """ createTreeWidget() -> QMethodTreeWidget
        Return the search tree widget for this window
        
        """
        self.setWindowTitle('Methods')
        return QMethodTreeWidget(self)

    def updateModule(self, module):
        self.module = module
        if module is not None and module.tag is not None:
            self.tagEdit.setText(module.tag)
        else:
            self.tagEdit.setText('')
        self.treeWidget.updateModule(module)

    def tagFinished(self):
        text = str(self.tagEdit.text())
        self.module.tag = text
        if self.controller:
            self.controller.update_module_tag(self.module, text)
#         if text.strip() != '':
#             self.module.tag = text

class QMethodTreeWidget(QSearchTreeWidget):
    """
    QMethodTreeWidget is a subclass of QSearchTreeWidget to display all
    methods available of a module
    
    """
    def __init__(self, parent=None):
        """ QModuleMethods(parent: QWidget) -> QModuleMethods
        Set up size policy and header

        """
        QSearchTreeWidget.__init__(self, parent)
        self.header().setStretchLastSection(True)
        self.setHeaderLabels(QtCore.QStringList() << 'Method' << 'Signature')

    def updateModule(self, module):
        """ updateModule(module: Module) -> None        
        Setup this tree widget to show functions of module
        
        """
        self.clear()

        if module:
            try:
                descriptor = registry.get_descriptor_by_name(module.package,
                                                             module.name,
                                                             module.namespace)
            except registry.MissingModulePackage, e:
                # FIXME handle this the same way as
                # vistrail_controller:change_selected_version
                raise
            moduleHierarchy = registry.get_module_hierarchy(descriptor)
            for baseModule in moduleHierarchy:
                baseName = registry.get_descriptor(baseModule).name
                base_package = registry.get_descriptor(baseModule).identifier
                # Create the base widget item
                baseItem = QMethodTreeWidgetItem(None,
                                                 None,
                                                 None,
                                                 self,
                                                 (QtCore.QStringList()
                                                  <<  baseName
                                                  << ''))
                methods = registry.method_ports(baseModule)
                # Also check the local registry
                if module.registry and module.registry.has_module(base_package,
                                                                  baseName):
                    methods += module.registry.method_ports(baseModule)
                for method in methods:
                    sig = method.spec.create_sigstring(short=True)
                    QMethodTreeWidgetItem(module,
                                          method,
                                          method.spec,
                                          baseItem,
                                          (QtCore.QStringList()
                                           << method.name
                                           << sig))
            self.expandAll()
            self.resizeColumnToContents(0)
                                          
    def contextMenuEvent(self, event):
        # Just dispatches the menu event to the widget item
        item = self.itemAt(event.pos())
        if item:
            item.contextMenuEvent(event, self)

class QMethodTreeWidgetItem(QtGui.QTreeWidgetItem):
    """
    QMethodTreeWidgetItem represents method on QMethodTreeWidget
    
    """
    def __init__(self, module, port, spec, parent, labelList):
        """ QMethodTreeWidgetItem(module: Module
                                  port: Port,
                                  spec: tuple,
                                  parent: QTreeWidgetItem
                                  labelList: QStringList)
                                  -> QMethodTreeWidgetItem                               
        Create a new tree widget item with a specific parent and
        labels

        """
        self.module = module
        self.port = port
        self.spec = spec
        QtGui.QTreeWidgetItem.__init__(self, parent, labelList)

    def contextMenuEvent(self, event, widget):
        if self.module is None:
            return
        act = QtGui.QAction("View Documentation", widget)
        act.setStatusTip("View method documentation")
        QtCore.QObject.connect(act,
                               QtCore.SIGNAL("triggered()"),
                               self.view_documentation)
        menu = QtGui.QMenu(widget)
        menu.addAction(act)
        menu.exec_(event.globalPos())
        
    def view_documentation(self):
        descriptor = registry.get_descriptor_by_name(self.module.package,
                                                     self.module.name,
                                                     self.module.namespace)
        widget = QPortDocumentation(descriptor,
                                    PortEndPoint.Destination,
                                    self.port.name)
        widget.setAttribute(QtCore.Qt.WA_DeleteOnClose)
        widget.exec_()
