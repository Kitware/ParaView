
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
""" This file contains widgets that can be used for dropping
methods. It will construct an input form for that method

QMethodDropBox
QVerticalWidget
QMethodInputForm
QHoverAliasLabel
"""

from PyQt4 import QtCore, QtGui
from core.utils import expression
from core.vistrail.module_function import ModuleFunction
from core.modules import module_registry
from core.modules.constant_configuration import FileChooserToolButton
from gui.common_widgets import QPromptWidget
from gui.method_palette import QMethodTreeWidget
from gui.theme import CurrentTheme

################################################################################

class QMethodDropBox(QtGui.QScrollArea):
    """
    QMethodDropBox is just a widget such that method items from
    QMethodPalette can be dropped into its client rect. It then
    construct an input form based on the type of handling widget
    
    """
    def __init__(self, parent=None):
        """ QMethodPalette(parent: QWidget) -> QMethodPalette
        Initialize widget constraints
        
        """
        QtGui.QScrollArea.__init__(self, parent)
        self.setAcceptDrops(True)
        self.setWidgetResizable(True)
        self.vWidget = QVerticalWidget()
        self.setWidget(self.vWidget)
        self.updateLocked = False
        self.controller = None
        self.module = None

    def dragEnterEvent(self, event):
        """ dragEnterEvent(event: QDragEnterEvent) -> None
        Set to accept drops from the method palette
        
        """
        if type(event.source())==QMethodTreeWidget:
            data = event.mimeData()
            if hasattr(data, 'items'):
                event.accept()
        else:
            event.ignore()
        
    def dragMoveEvent(self, event):
        """ dragMoveEvent(event: QDragMoveEvent) -> None
        Set to accept drag move event from the method palette
        
        """
        if type(event.source())==QMethodTreeWidget:
            data = event.mimeData()
            if hasattr(data, 'items'):
                event.accept()

    def dropEvent(self, event):
        """ dropEvent(event: QDragMoveEvent) -> None
        Accept drop event to add a new method
        
        """
        if type(event.source())==QMethodTreeWidget:
            data = event.mimeData()
            if hasattr(data, 'items'):
                event.accept()
                for item in data.items:
                    if item.port:
                        function = item.spec.create_module_function(item.port)
                        # FIXME need to get the position,
                        # but not sure if this is correct
                        function.id = item.module.getNumFunctions()
                        self.vWidget.addFunction(item.module,
                                                 item.module.getNumFunctions(),
                                                 function)
                        self.scrollContentsBy(0, self.viewport().height())
                        self.lockUpdate()
                        if self.controller:
                            self.controller.add_method(self.module.id, function)
                        self.unlockUpdate()
                self.emit(QtCore.SIGNAL("paramsAreaChanged"))

    def updateModule(self, module):
        """ updateModule(module: Module) -> None        
        Construct input forms with the list of functions of a module
        
        """
        self.module = module
        if self.updateLocked: return
        self.vWidget.clear()
        if module:
            fId = 0
            for f in module.functions:
                self.vWidget.addFunction(module.id, fId, f)
                fId += 1
            self.vWidget.showPromptByChildren()
        else:
            self.vWidget.showPrompt(False)

    def lockUpdate(self):
        """ lockUpdate() -> None
        Do not allow updateModule()
        
        """
        self.updateLocked = True
        
    def unlockUpdate(self):
        """ unlockUpdate() -> None
        Allow updateModule()
        
        """
        self.updateLocked = False

class QVerticalWidget(QPromptWidget):
    """
    QVerticalWidget is a widget holding other function widgets
    vertically
    
    """
    def __init__(self, parent=None):
        """ QVerticalWidget(parent: QWidget) -> QVerticalWidget
        Initialize with a vertical layout
        
        """
        QPromptWidget.__init__(self, parent)
        self.setPromptText("Drag methods here to set parameters")
        self.setLayout(QtGui.QVBoxLayout())
        self.layout().setMargin(0)
        self.layout().setSpacing(5)
        self.layout().setAlignment(QtCore.Qt.AlignTop)
        self.setSizePolicy(QtGui.QSizePolicy.Expanding,
                           QtGui.QSizePolicy.Expanding)
        self.formType = QMethodInputForm
        self.setMinimumHeight(50)
        self._functions = []
        
    def addFunction(self, moduleId, fId, function):
        """ addFunction(moduleId: int, fId: int,
                        function: ModuleFunction) -> None
        Add an input form for the function
        
        """
        inputForm = self.formType(self)
        inputForm.moduleId = moduleId
        inputForm.fId = fId
        inputForm.updateFunction(function)
        self.layout().addWidget(inputForm)
        inputForm.show()
        self.setMinimumHeight(self.layout().minimumSize().height())
        self.showPrompt(False)
        self._functions.append(inputForm)

    def clear(self):
        """ clear() -> None
        Clear and delete all widgets in the layout
        
        """
        self.setEnabled(False)
        for v in self._functions:
            self.layout().removeWidget(v)
            v.deleteLater()
        self._functions = []
        self.setEnabled(True)

class QMethodInputForm(QtGui.QGroupBox):
    """
    QMethodInputForm is a widget with multiple input lines depends on
    the method signature
    
    """
    def __init__(self, parent=None):
        """ QMethodInputForm(parent: QWidget) -> QMethodInputForm
        Initialize with a vertical layout
        
        """
        QtGui.QGroupBox.__init__(self, parent)
        self.setLayout(QtGui.QGridLayout())
        self.layout().setMargin(5)
        self.layout().setSpacing(5)
        self.setFocusPolicy(QtCore.Qt.ClickFocus)
        self.palette().setColor(QtGui.QPalette.Window,
                                CurrentTheme.METHOD_SELECT_COLOR)
        self.fId = -1
        self.function = None

    def focusInEvent(self, event):
        """ gotFocus() -> None
        Make sure the form painted as selected
        
        """
        self.setAutoFillBackground(True)

    def focusOutEvent(self, event):
        """ lostFocus() -> None
        Make sure the form painted as non-selected and then
        perform a parameter changes
        
        """
        self.setAutoFillBackground(False)

    def updateMethod(self):
        """ updateMethod() -> None
        Update the method values to vistrail
        
        """
        methodBox = self.parent().parent().parent()
        if methodBox.controller:
            paramList = []
            for i in xrange(len(self.widgets)):
                paramList.append((str(self.widgets[i].contents()),
                                  self.function.params[i].type,
                                  self.function.params[i].namespace,
                                  self.function.params[i].identifier,
                                  str(self.labels[i].alias)))
            methodBox.lockUpdate()
            methodBox.controller.replace_method(methodBox.module,
                                                self.fId,
                                                paramList)
            methodBox.unlockUpdate()

    def check_alias(self, name):
        """ check_alias(name: str) -> Boolean
        Returns True if the current pipeline already has the alias name

        """
        methodBox = self.parent().parent().parent()
        if methodBox.controller:
            return methodBox.controller.check_alias(name)
        return False

    def updateFunction(self, function):
        """ updateFunction(function: ModuleFunction) -> None
        Auto create widgets to describes the function 'function'
        
        """
        reg = module_registry.registry
        self.setTitle(function.name)
        self.function = function
        self.widgets = []
        self.labels = []
        for pIndex in xrange(len(function.params)):
            p = function.params[pIndex]
            # FIXME: Find the source of this problem instead
            # of working around it here.
            if p.identifier == '':
                idn = 'edu.utah.sci.vistrails.basic'
            else:
                idn = p.identifier
            p_module = reg.get_module_by_name(idn,
                                              p.type,
                                              p.namespace)
            widget_type = p_module.get_widget_class()
            label = QHoverAliasLabel(p.alias, p.type)
            constant_widget = widget_type(p, self)            
            self.widgets.append(constant_widget)
            self.labels.append(label)
            self.layout().addWidget(label, pIndex, 0)
            self.layout().addWidget(constant_widget, pIndex, 1)
            # Ugly hack to add browse button to methods that look like
            # they have to do with files
            if('file' in function.name.lower() and p.type == 'String'):
                browseButton = FileChooserToolButton(self, constant_widget)
                self.layout().addWidget(browseButton, pIndex, 2)

    def keyPressEvent(self, e):
        """ keyPressEvent(e: QKeyEvent) -> None
        Handle Del/Backspace to delete the input form
        
        """
        if e.key() in [QtCore.Qt.Key_Delete, QtCore.Qt.Key_Backspace]:
            methodBox = self.parent().parent().parent()
            self.parent().layout().removeWidget(self)
            self.parent()._functions.remove(self)
            self.deleteLater()
            self.parent().showPromptByChildren()
            for i in xrange(self.parent().layout().count()):
                self.parent().layout().itemAt(i).widget().fId = i
            methodBox.lockUpdate()
            if methodBox.controller:
                methodBox.controller.delete_method(self.fId,
                                                   methodBox.module.id)            
            methodBox.unlockUpdate()
            methodBox.emit(QtCore.SIGNAL("paramsAreaChanged"))
        else:
            QtGui.QGroupBox.keyPressEvent(self, e)
            # super(QMethodInputForm, self).keyPressEvent(e)

class QHoverAliasLabel(QtGui.QLabel):
    """
    QHoverAliasLabel is a QLabel that supports hover actions similar
    to a hot link
    """
    def __init__(self, alias='', text='', parent=None):
        """ QHoverAliasLabel(alias:str , text: str, parent: QWidget)
                             -> QHoverAliasLabel
        Initialize the label with a text
        
        """
        QtGui.QLabel.__init__(self, parent)
        self.alias = alias
        self.caption = text
        self.updateText()
        self.setAttribute(QtCore.Qt.WA_Hover)
        self.setCursor(QtCore.Qt.PointingHandCursor)
        self.setToolTip(alias)
        self.palette().setColor(QtGui.QPalette.WindowText,
                                CurrentTheme.HOVER_DEFAULT_COLOR)

    def updateText(self):
        """ updateText() -> None
        Update the label text to contain the alias name when appropriate
        
        """
        if self.alias!='':
            self.setText(self.alias+': '+self.caption)
        else:
            self.setText(self.caption)

    def event(self, event):
        """ event(event: QEvent) -> Event Result
        Override to handle hover enter and leave events for hot links
        
        """
        if event.type()==QtCore.QEvent.HoverEnter:
            self.palette().setColor(QtGui.QPalette.WindowText,
                                    CurrentTheme.HOVER_SELECT_COLOR)
        if event.type()==QtCore.QEvent.HoverLeave:
            self.palette().setColor(QtGui.QPalette.WindowText,
                                    CurrentTheme.HOVER_DEFAULT_COLOR)
        return QtGui.QLabel.event(self, event)
        # return super(QHoverAliasLabel, self).event(event)

    def mousePressEvent(self, event):
        """ mousePressEvent(event: QMouseEvent) -> None        
        If mouse click on the label, show up a dialog to change/add
        the alias name
        
        """
        if event.button()==QtCore.Qt.LeftButton:
            (text, ok) = QtGui.QInputDialog.getText(self,
                                                    'Set Parameter Alias',
                                                    'Enter the parameter alias',
                                                    QtGui.QLineEdit.Normal,
                                                    self.alias)
            while ok and self.parent().check_alias(str(text)):
                msg =" This alias is already being used.\
 Please enter a different parameter alias "
                (text, ok) = QtGui.QInputDialog.getText(self,
                                                        'Set Parameter Alias',
                                                        msg,
                                                        QtGui.QLineEdit.Normal,
                                                        text)
            if ok and str(text)!=self.alias:
                if not self.parent().check_alias(str(text)):
                    self.alias = str(text).strip()
                    self.updateText()
                    self.parent().updateMethod()

