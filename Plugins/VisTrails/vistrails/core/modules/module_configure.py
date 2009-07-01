
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
from PyQt4 import QtCore, QtGui
from core.utils import any
from core.modules.module_registry import registry, ModuleRegistry
# from core.vistrail.action import ChangeParameterAction
from core.vistrail.module_function import ModuleFunction
from core.vistrail.module_param import ModuleParam
from core.vistrail.port import PortEndPoint
import urllib

class StandardModuleConfigurationWidget(QtGui.QDialog):

    def __init__(self, module, controller, parent=None):
        QtGui.QDialog.__init__(self, parent)
        self.setModal(True)
        self.module = module
        reg = module.registry or registry
        self.module_descriptor = reg.get_descriptor_by_name(
            self.module.package,
            self.module.name,
            self.module.namespace)
        self.controller = controller


class DefaultModuleConfigurationWidget(StandardModuleConfigurationWidget):

    def __init__(self, module, controller, parent=None):
        StandardModuleConfigurationWidget.__init__(self, module, controller, parent)
        self.setWindowTitle('Module Configuration')
        self.setLayout(QtGui.QVBoxLayout())
        self.layout().setMargin(0)
        self.layout().setSpacing(0)
        self.scrollArea = QtGui.QScrollArea(self)
        self.layout().addWidget(self.scrollArea)
        self.scrollArea.setFrameStyle(QtGui.QFrame.NoFrame)
        self.listContainer = QtGui.QWidget(self.scrollArea)
        self.listContainer.setLayout(QtGui.QGridLayout(self.listContainer))
        self.inputPorts = self.module.destinationPorts()
        self.inputDict = {}
        self.outputPorts = self.module.sourcePorts()
        self.outputDict = {}
        self.constructList()
        self.buttonLayout = QtGui.QHBoxLayout()
        self.buttonLayout.setMargin(5)
        self.okButton = QtGui.QPushButton('&OK', self)
        self.okButton.setFixedWidth(100)
        self.buttonLayout.addWidget(self.okButton)
        self.cancelButton = QtGui.QPushButton('&Cancel', self)
        self.cancelButton.setShortcut('Esc')
        self.cancelButton.setFixedWidth(100)
        self.buttonLayout.addWidget(self.cancelButton)
        self.layout().addLayout(self.buttonLayout)
        self.connect(self.okButton, QtCore.SIGNAL('clicked(bool)'), self.okTriggered)
        self.connect(self.cancelButton, QtCore.SIGNAL('clicked(bool)'), self.close)

    def checkBoxFromPort(self, port, input_=False):
        checkBox = QtGui.QCheckBox(port.name)
        if input_:
            pep = PortEndPoint.Destination
        else:
            pep = PortEndPoint.Source
        if not port.optional or (pep, port.name) in self.module.portVisible:
            checkBox.setCheckState(QtCore.Qt.Checked)
        else:
            checkBox.setCheckState(QtCore.Qt.Unchecked)
        if not port.optional or (input_ and port.spec.create_sigstring()==['()']):
            checkBox.setEnabled(False)
        return checkBox

    def constructList(self):
        label = QtGui.QLabel('Input Ports')
        label.setAlignment(QtCore.Qt.AlignHCenter)
        label.font().setBold(True)
        label.font().setPointSize(12)
        self.listContainer.layout().addWidget(label, 0, 0)
        label = QtGui.QLabel('Output Ports')
        label.setAlignment(QtCore.Qt.AlignHCenter)
        label.font().setBold(True)
        label.font().setPointSize(12)
        self.listContainer.layout().addWidget(label, 0, 1)

        for i in xrange(len(self.inputPorts)):
            port = self.inputPorts[i]
            checkBox = self.checkBoxFromPort(port, True)
            self.inputDict[port.name] = checkBox
            self.listContainer.layout().addWidget(checkBox, i+1, 0)
        
        for i in xrange(len(self.outputPorts)):
            port = self.outputPorts[i]
            checkBox = self.checkBoxFromPort(port)
            self.outputDict[port.name] = checkBox
            self.listContainer.layout().addWidget(checkBox, i+1, 1)
        
        self.listContainer.adjustSize()
        self.listContainer.setFixedHeight(self.listContainer.height())
        self.scrollArea.setWidget(self.listContainer)
        self.scrollArea.setWidgetResizable(True)

    def sizeHint(self):
        return QtCore.QSize(384, 512)

    def okTriggered(self, checked = False):
        for port in self.inputPorts:
            entry = (PortEndPoint.Destination, port.name)
            if (port.optional and
                self.inputDict[port.name].checkState()==QtCore.Qt.Checked):
                self.module.portVisible.add(entry)
            else:
                self.module.portVisible.discard(entry)
            
        for port in self.outputPorts:
            entry = (PortEndPoint.Source, port.name)
            if (port.optional and
                self.outputDict[port.name].checkState()==QtCore.Qt.Checked):
                self.module.portVisible.add(entry)
            else:
                self.module.portVisible.discard(entry)
        self.close()
        

class PortTable(QtGui.QTableWidget):
    def __init__(self, parent=None):
        QtGui.QTableWidget.__init__(self,1,2,parent)
        self.horizontalHeader().setResizeMode(QtGui.QHeaderView.Interactive)
        self.horizontalHeader().setMovable(False)
        self.horizontalHeader().setStretchLastSection(True)
        self.setSelectionMode(QtGui.QAbstractItemView.NoSelection)
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.delegate = PortTableItemDelegate(self)
        self.setItemDelegate(self.delegate)
        self.setFrameStyle(QtGui.QFrame.NoFrame)
        self.connect(self.model(),
                     QtCore.SIGNAL('dataChanged(QModelIndex,QModelIndex)'),
                     self.handleDataChanged)

    def sizeHint(self):
        return QtCore.QSize()

    def fixGeometry(self):
        rect = self.visualRect(self.model().index(self.rowCount()-1,
                                                  self.columnCount()-1))
        self.setFixedHeight(self.horizontalHeader().height()+
                            rect.y()+rect.height()+1)

    def handleDataChanged(self, topLeft, bottomRight):
        if topLeft.column()==0:
            text = str(self.model().data(topLeft, QtCore.Qt.DisplayRole).toString())
            changedGeometry = False
            if text!='' and topLeft.row()==self.rowCount()-1:
                self.setRowCount(self.rowCount()+1)
                changedGeometry = True
            if text=='' and topLeft.row()<self.rowCount()-1:
                self.removeRow(topLeft.row())
                changedGeometry = True
            if changedGeometry:
                self.fixGeometry()

    def initializePorts(self, ports):
        self.disconnect(self.model(),
                        QtCore.SIGNAL('dataChanged(QModelIndex,QModelIndex)'),
                        self.handleDataChanged)
        for p in ports:
            model = self.model()
            portType = registry.get_descriptor(p.spec.signature[0][0]).name
            model.setData(model.index(self.rowCount()-1, 1),
                          QtCore.QVariant(portType),
                          QtCore.Qt.DisplayRole)
            model.setData(model.index(self.rowCount()-1, 0),
                          QtCore.QVariant(p.name),
                          QtCore.Qt.DisplayRole)
            self.setRowCount(self.rowCount()+1)
        self.fixGeometry()
        self.connect(self.model(),
                     QtCore.SIGNAL('dataChanged(QModelIndex,QModelIndex)'),
                     self.handleDataChanged)
            

class PortTableItemDelegate(QtGui.QItemDelegate):

    def createEditor(self, parent, option, index):
        if index.column()==1: #Port type
            combo = QtGui.QComboBox(parent)
            combo.setEditable(False)
            for m in sorted(registry._module_key_map.itervalues()):
                combo.addItem(m[1])
            return combo
        else:
            return QtGui.QItemDelegate.createEditor(self, parent, option, index)

    def setEditorData(self, editor, index):
        if index.column()==1:
            text = index.model().data(index, QtCore.Qt.DisplayRole).toString()
            editor.setCurrentIndex(editor.findText(text))
        else:
            QtGui.QItemDelegate.setEditorData(self, editor, index)

    def setModelData(self, editor, model, index):
        if index.column()==1:
            model.setData(index, QtCore.QVariant(editor.currentText()), QtCore.Qt.DisplayRole)
        else:
            QtGui.QItemDelegate.setModelData(self, editor, model, index)

class PythonHighlighter(QtGui.QSyntaxHighlighter):
    def __init__( self, document ):
        QtGui.QSyntaxHighlighter.__init__( self, document )
        self.rules = []
        literalFormat = QtGui.QTextCharFormat()
        literalFormat.setForeground(QtGui.QColor(65, 105, 225)) #royalblue
        self.rules += [(r"^[^']*'", 0, literalFormat, 1, -1)]
        self.rules += [(r"'[^']*'", 0, literalFormat, -1, -1)]
        self.rules += [(r"'[^']*$", 0, literalFormat, -1, 1)]
        self.rules += [(r"^[^']+$", 0, literalFormat, 1, 1)]        
        self.rules += [(r'^[^"]*"', 0, literalFormat, 2, -1)]
        self.rules += [(r'"[^"]*"', 0, literalFormat, -1, -1)]
        self.rules += [(r'"[^"]*$', 0, literalFormat, -1, 2)]
        self.rules += [(r'^[^"]+$', 0, literalFormat, 2, 2)]
        
        keywordFormat = QtGui.QTextCharFormat()
        keywordFormat.setForeground(QtCore.Qt.blue)
        self.rules += [(r"\b%s\b"%w, 0, keywordFormat, -1, -1)
                       for w in ["def","class","from", 
                                 "import","for","in", 
                                 "while","True","None",
                                 "False","pass","return",
                                 "self","tuple","list",
                                 "print","if","else",
                                 "elif","in","len",
                                 "assert","try","except",
                                 "exec", "break", "continue"
                                 "not", "and", "or", "as",
                                 "type", "int", "float",
                                 "string"
                                 ]]
        
        defclassFormat = QtGui.QTextCharFormat()
        defclassFormat.setForeground(QtCore.Qt.blue)
        self.rules += [(r"\bdef\b\s*(\w+)", 1, defclassFormat, -1, -1)]
        self.rules += [(r"\bclass\b\s*(\w+)", 1, defclassFormat, -1, -1)]
        
        commentFormat = QtGui.QTextCharFormat()
        commentFormat.setFontItalic(True)
        commentFormat.setForeground(QtCore.Qt.darkGreen)
        self.rules += [(r"#[^\n]*", 0, commentFormat, -1, -1)]
        
    def highlightBlock(self, text):
        baseFormat = self.format(0)
        prevState = self.previousBlockState()
        self.setCurrentBlockState(prevState)
        for rule in self.rules:
            if prevState==rule[3] or rule[3]==-1:
                expression = QtCore.QRegExp(rule[0])
                pos = expression.indexIn(text, 0)
                while pos != -1:
                    pos = expression.pos(rule[1])
                    length = expression.cap(rule[1]).length()
                    if self.format(pos)==baseFormat:
                        self.setFormat(pos, length, rule[2])
                        self.setCurrentBlockState(rule[4])
                        pos = expression.indexIn(text, pos+expression.matchedLength())
                    else:
                        pos = expression.indexIn(text, pos+1)
                

class PythonEditor(QtGui.QTextEdit):

    def __init__(self, parent=None):
        QtGui.QTextEdit.__init__(self, parent)
        self.setLineWrapMode(QtGui.QTextEdit.NoWrap)
        self.setFontFamily('Courier')
        self.setFontPointSize(10.0)
        self.setCursorWidth(8)
        self.highlighter = PythonHighlighter(self.document())
        self.connect(self,
                     QtCore.SIGNAL('currentCharFormatChanged(QTextCharFormat)'),
                     self.formatChanged)

    def formatChanged(self, f):
        self.setFontFamily('Courier')
        self.setFontPointSize(10.0)

    def keyPressEvent(self, event):
        """ keyPressEvent(event: QKeyEvent) -> Nont
        Handle tab with 4 spaces
        
        """
        if event.key()==QtCore.Qt.Key_Tab:
            self.insertPlainText('    ')
        else:
            # super(PythonEditor, self).keyPressEvent(event)
            QtGui.QTextEdit.keyPressEvent(self, event)
                 
class PythonSourceConfigurationWidget(StandardModuleConfigurationWidget):

    def __init__(self, module, controller, parent=None):
        StandardModuleConfigurationWidget.__init__(self, module, controller, parent)
        self.setWindowTitle('PythonSource Configuration')
        self.setLayout(QtGui.QVBoxLayout())
        self.layout().setMargin(0)
        self.layout().setSpacing(0)
        self.createPortTable()
        self.createEditor()
        self.createButtonLayout()        

    def createPortTable(self):
        self.inputPortTable = PortTable(self)
        labels = QtCore.QStringList() << "Input Port Name" << "Type"
        self.inputPortTable.setHorizontalHeaderLabels(labels)
        self.outputPortTable = PortTable(self)
        labels = QtCore.QStringList() << "Output Port Name" << "Type"
        self.outputPortTable.setHorizontalHeaderLabels(labels)
        if self.module.registry:
            iPorts = self.module.registry.destination_ports_from_descriptor(self.module_descriptor)
            self.inputPortTable.initializePorts(iPorts)
            oPorts = self.module.registry.source_ports_from_descriptor(self.module_descriptor)
            self.outputPortTable.initializePorts(oPorts)
        self.layout().addWidget(self.inputPortTable)
        self.layout().addWidget(self.outputPortTable)
        self.performPortConnection(self.connect)
        self.inputPortTable.fixGeometry()
        self.outputPortTable.fixGeometry()

    def findSourceFunction(self):
        fid = -1
        for i in xrange(self.module.getNumFunctions()):
            if self.module.functions[i].name=='source':
                fid = i
                break
        return fid

    def createEditor(self):
        self.codeEditor = PythonEditor(self)
        fid = self.findSourceFunction()
        if fid!=-1:
            f = self.module.functions[fid]
            self.codeEditor.setPlainText(urllib.unquote(f.params[0].strValue))
        self.codeEditor.document().setModified(False)
        self.layout().addWidget(self.codeEditor, 1)

    def createButtonLayout(self):
        self.buttonLayout = QtGui.QHBoxLayout()
        self.buttonLayout.setMargin(5)
        self.okButton = QtGui.QPushButton('&OK', self)
        self.okButton.setAutoDefault(False)
        self.okButton.setFixedWidth(100)
        self.buttonLayout.addWidget(self.okButton)
        self.cancelButton = QtGui.QPushButton('&Cancel', self)
        self.cancelButton.setAutoDefault(False)
        self.cancelButton.setShortcut('Esc')
        self.cancelButton.setFixedWidth(100)
        self.buttonLayout.addWidget(self.cancelButton)
        self.layout().addLayout(self.buttonLayout)
        self.connect(self.okButton, QtCore.SIGNAL('clicked(bool)'), self.okTriggered)
        self.connect(self.cancelButton, QtCore.SIGNAL('clicked(bool)'), self.close)

    def sizeHint(self):
        return QtCore.QSize(512, 512)

    def performPortConnection(self, operation):
        operation(self.inputPortTable.horizontalHeader(),
                  QtCore.SIGNAL('sectionResized(int,int,int)'),
                  self.portTableResize)
        operation(self.outputPortTable.horizontalHeader(),
                  QtCore.SIGNAL('sectionResized(int,int,int)'),
                  self.portTableResize)

    def portTableResize(self, logicalIndex, oldSize, newSize):
        self.performPortConnection(self.disconnect)
        if self.inputPortTable.horizontalHeader().sectionSize(logicalIndex)!=newSize:
            self.inputPortTable.horizontalHeader().resizeSection(logicalIndex,newSize)
        if self.outputPortTable.horizontalHeader().sectionSize(logicalIndex)!=newSize:
            self.outputPortTable.horizontalHeader().resizeSection(logicalIndex,newSize)
        self.performPortConnection(self.connect)

    def newInputOutputPorts(self):
        ports = []
        for i in xrange(self.inputPortTable.rowCount()):
            model = self.inputPortTable.model()
            name = str(model.data(model.index(i, 0), QtCore.Qt.DisplayRole).toString())
            typeName = str(model.data(model.index(i, 1), QtCore.Qt.DisplayRole).toString())
            if name!='' and typeName!='':
                ports.append(('input', name, '('+typeName+')'))
        for i in xrange(self.outputPortTable.rowCount()):
            model = self.outputPortTable.model()
            name = str(model.data(model.index(i, 0), QtCore.Qt.DisplayRole).toString())
            typeName = str(model.data(model.index(i, 1), QtCore.Qt.DisplayRole).toString())
            if name!='' and typeName!='':
                ports.append(('output', name, '('+typeName+')'))
        return ports

    def specsFromPorts(self, portType, ports):
        return [(portType,
                 p.name,
                 '('+registry.get_descriptor(p.spec.signature[0][0]).name+')')
                for p in ports[0][1]]

    def registryChanges(self, oldRegistry, newPorts):
        if oldRegistry:
            oldIn = self.specsFromPorts('input',
                                        oldRegistry.all_destination_ports(self.module_descriptor))
            oldOut = self.specsFromPorts('output',
                                         oldRegistry.all_source_ports(self.module_descriptor))
        else:
            oldIn = []
            oldOut = []
        deletePorts = [p for p in oldIn if not p in newPorts]
        deletePorts += [p for p in oldOut if not p in newPorts]
        addPorts = [p for p in newPorts if ((not p in oldIn) and (not p in oldOut))]
        return (deletePorts, addPorts)

    def updateActionsHandler(self, controller):
        oldRegistry = self.module.registry
        newPorts = self.newInputOutputPorts()
        (deletePorts, addPorts) = self.registryChanges(oldRegistry, newPorts)
        for (cid, c) in controller.current_pipeline.connections.items():
            if ((c.sourceId==self.module.id and
                 any([c.source.name==p[1] for p in deletePorts])) or
                (c.destinationId==self.module.id and
                 any([c.destination.name==p[1] for p in deletePorts]))):
                controller.delete_connection(cid)
        for p in deletePorts:
            controller.delete_module_port(self.module.id, p)
        for p in addPorts:
            controller.add_module_port(self.module.id, p)
        if self.codeEditor.document().isModified():
            code = urllib.quote(str(self.codeEditor.toPlainText()))
            fid = self.findSourceFunction()
            
            # FIXME make sure that this makes sense
            if fid==-1:
                # do add function
                fid = self.module.getNumFunctions()
                f = ModuleFunction(pos=fid,
                                   name='source')
                param = ModuleParam(type='String',
                                    val=code,
                                    alias='',
                                    name='<no description>',
                                    pos=0)
                f.addParameter(param)
                controller.add_method(self.module.id, f)
            else:
                # do change parameter
                paramList = [(code, 'String', None, 
                              'edu.utah.sci.vistrails.basic', '')]
                controller.replace_method(self.module, fid, paramList)
#             action = ChangeParameterAction()
#             action.addParameter(self.module.id, fid, 0, 'source',
#                                 '<no description>',code,'String', '')
#             controller.performAction(action)
                
    def okTriggered(self, checked = False):
        self.updateActionsHandler(self.controller)
        self.emit(QtCore.SIGNAL('doneConfigure()'))
        self.close()
