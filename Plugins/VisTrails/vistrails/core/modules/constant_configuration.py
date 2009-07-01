
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
""" This file specifies the configuration widget for Constant
modules. Please notice that this is different from the module configuration
widget described in module_configure.py. We present a Color constant to be
used as a template for creating a configuration widget for other custom
constants.

"""
from PyQt4 import QtCore, QtGui
from core.modules.module_registry import registry
from core.utils import any, expression
from core import system
############################################################################

class ConstantWidgetMixin(object):

    def __init__(self):
        self._last_contents = None

    def update_parent(self):
        if self.parent():
            newContents = self.contents()
            if newContents != self._last_contents:
                self.parent().updateMethod()
                self._last_contents = newContents

class StandardConstantWidget(QtGui.QLineEdit, ConstantWidgetMixin):
    """
    StandardConstantWidget is a basic widget to be used
    to edit int/float/string values in VisTrails.

    When creating your own widget, you can subclass from this widget if you
    need only a QLineEdit or use your own QT widget. There are two things you
    need to pay attention to:

    1) Re-implement the contents() method so we can get the current value
       stored in the widget.

    2) When the user is done with configuration, make sure to call
       update_parent() so VisTrails can pass that information to the Provenance
       System. In this example we do that on focusOutEvent and when the user
       presses the return key.

    """
    def __init__(self, param, parent=None):
        """__init__(param: core.vistrail.module_param.ModuleParam,
                    parent: QWidget)

        Initialize the line edit with its contents. Content type is limited
        to 'int', 'float', and 'string'

        """
        QtGui.QLineEdit.__init__(self, parent)
        ConstantWidgetMixin.__init__(self)
        # assert param.namespace == None
        # assert param.identifier == 'edu.utah.sci.vistrails.basic'
        contents = param.strValue
        contentType = param.type
        self.setText(contents)
        self._contentType = contentType
        self.connect(self,
                     QtCore.SIGNAL('returnPressed()'),
                     self.update_parent)

    def contents(self):
        """contents() -> str
        Re-implement this method to make sure that it will return a string
        representation of the value that it will be passed to the module
        As this is a QLineEdit, we just call text()

        """
        self.update_text()
        return str(self.text())

    def update_text(self):
        """ update_text() -> None
        Update the text to the result of the evaluation

        """
        # FIXME: eval should pretty much never be used
        base = expression.evaluate_expressions(self.text())
        if self._contentType == 'String':
            self.setText(base)
        else:
            try:
                self.setText(str(eval(str(base), None, None)))
            except:
                self.setText(base)

    ###########################################################################
    # event handlers

    def focusInEvent(self, event):
        """ focusInEvent(event: QEvent) -> None
        Pass the event to the parent

        """
        self._contents = str(self.text())
        if self.parent():
            self.parent().focusInEvent(event)
        QtGui.QLineEdit.focusInEvent(self, event)

    def focusOutEvent(self, event):
        self.update_parent()
        QtGui.QLineEdit.focusOutEvent(self, event)
        if self.parent():
            self.parent().focusOutEvent(event)

###############################################################################
# File Constant Widgets

class FileChooserToolButton(QtGui.QToolButton):
    """
    MethodFileChooser is a toolbar button that opens a browser for
    files.  The lineEdit is updated with the filename that is
    selected.

    """
    def __init__(self, parent=None, lineEdit=None):
        """
        FileChooserButton(parent: QWidget, lineEdit: StandardConstantWidget) ->
                 FileChooserToolButton

        """
        QtGui.QToolButton.__init__(self, parent)
        self.setIcon(QtGui.QIcon(
                self.style().standardPixmap(QtGui.QStyle.SP_DirOpenIcon)))
        self.setIconSize(QtCore.QSize(12,12))
        self.setToolTip('Open a file chooser')
        self.setAutoRaise(True)
        self.lineEdit = lineEdit
        self.connect(self,
                     QtCore.SIGNAL('clicked()'),
                     self.openChooser)


    def openChooser(self):
        """
        openChooser() -> None

        """
        fileName = QtGui.QFileDialog.getOpenFileName(self,
                                                     'Use Filename '
                                                     'as Value...',
                                                     self.text(),
                                                     'All files '
                                                     '(*.*)')
        if self.lineEdit and not fileName.isEmpty():
            self.lineEdit.setText(fileName)
            self.lineEdit.update_parent()

class FileChooserWidget(QtGui.QWidget, ConstantWidgetMixin):
    """
    FileChooserWidget is a widget containing a line edit and a button that
    opens a browser for files. The lineEdit is updated with the filename that is
    selected.

    """
    def __init__(self, param, parent=None):
        """__init__(param: core.vistrail.module_param.ModuleParam,
        parent: QWidget)
        Initializes the line edit with contents

        """
        QtGui.QWidget.__init__(self, parent)
        ConstantWidgetMixin.__init__(self)
        layout = QtGui.QHBoxLayout()
        self.line_edit = StandardConstantWidget(param, self)
        self.browse_button = FileChooserToolButton(self, self.line_edit)
        layout.setMargin(5)
        layout.setSpacing(5)
        layout.addWidget(self.line_edit)
        layout.addWidget(self.browse_button)
        self.setLayout(layout)

    def updateMethod(self):
        if self.parent():
            self.parent().updateMethod()

    def contents(self):
        """contents() -> str
        Return the contents of the line_edit

        """
        return self.line_edit.contents()

class BooleanWidget(QtGui.QCheckBox, ConstantWidgetMixin):

    _values = ['True', 'False']
    _states = [QtCore.Qt.Checked, QtCore.Qt.Unchecked]

    def __init__(self, param, parent=None):
        """__init__(param: core.vistrail.module_param.ModuleParam,
                    parent: QWidget)
        Initializes the line edit with contents
        """
        QtGui.QCheckBox.__init__(self, parent)
        ConstantWidgetMixin.__init__(self)
        assert param.type == 'Boolean'
        assert param.identifier == 'edu.utah.sci.vistrails.basic'
        assert param.namespace is None
        if param.strValue:
            value = param.strValue
        else:
            value = "False"
        assert value in self._values

        self.connect(self, QtCore.SIGNAL('stateChanged(int)'),
                     self.change_state)
        self.setCheckState(self._states[self._values.index(value)])

    def contents(self):
        return self._values[self._states.index(self.checkState())]

    def change_state(self, state):
        self.update_parent()

###############################################################################
# Constant Color widgets

class ColorChooserButton(QtGui.QFrame):
    def __init__(self, parent=None):
        QtGui.QFrame.__init__(self, parent)
        self.setFrameStyle(QtGui.QFrame.Box | QtGui.QFrame.Plain)
        self.setAttribute(QtCore.Qt.WA_PaintOnScreen)
        self.setAutoFillBackground(True)
        self.setColor(QtCore.Qt.white)
        self.setFixedSize(30,22)
        if system.systemType == 'Darwin':
            #the mac's nice look messes up with the colors
            self.setAttribute(QtCore.Qt.WA_MacMetalStyle, False)

    def setColor(self, qcolor):
        self.qcolor = qcolor
        self.palette().setBrush(QtGui.QPalette.Window, self.qcolor)
        self.repaint()

    def sizeHint(self):
        return QtCore.QSize(24,24)

    def mousePressEvent(self, event):
        if event.button() == QtCore.Qt.LeftButton:
            self.openChooser()

    def openChooser(self):
        """
        openChooser() -> None

        """
        color = QtGui.QColorDialog.getColor(self.qcolor, self.parent())
        if color.isValid():
            self.setColor(color)
            self.emit(QtCore.SIGNAL("color_selected"))
        else:
            self.setColor(self.qcolor)


class ColorWidget(QtGui.QWidget, ConstantWidgetMixin):
    def __init__(self, param, parent=None):
        """__init__(param: core.vistrail.module_param.ModuleParam,
                    parent: QWidget)
        """
        contents = param.strValue
        contentsType = param.type
        QtGui.QWidget.__init__(self, parent)
        ConstantWidgetMixin.__init__(self)
        layout = QtGui.QHBoxLayout()
        self.color_indicator = ColorChooserButton(self)
        self.connect(self.color_indicator,
                     QtCore.SIGNAL("color_selected"),
                     self.update_parent)
        self._last_contents = contents
        layout.setMargin(5)
        layout.setSpacing(5)
        layout.addWidget(self.color_indicator)
        layout.addStretch(1)
        self.setLayout(layout)
        if contents != "":
            color = contents.split(',')
            qcolor = QtGui.QColor(float(color[0])*255,
                                  float(color[1])*255,
                                  float(color[2])*255)
            self.color_indicator.setColor(qcolor)

    def contents(self):
        """contents() -> str
        Return the string representation of color_indicator

        """
        return "%s,%s,%s" % (self.color_indicator.qcolor.redF(),
                             self.color_indicator.qcolor.greenF(),
                             self.color_indicator.qcolor.blueF())
