
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

class QAttachingLabel(QtGui.QLabel):
    """ QAttachingLabel is a QLabel attaching to another
    widget. Inherited classes should implement the
    adjustGeometryToHost() to setup how the widget is attached """

    def __init__(self, text='', parent=None):
        """ Set a transparent background for the center align label """
        QtGui.QLabel.__init__(self, text, parent)
        self._widget = None
        self._disableHost = True
        self.setAutoFillBackground(True)
        self.hide()

    def adjustGeometryToHost(self, widget):
        """ The host widget geometry has been changed, this attaching
        widget also need updates """
        self.setGeometry(widget.geometry())

    def eventFilter(self, widget, e):
        """ Adjust the warning to the attached widget """
        if widget==self._widget:
            if e.type()==QtCore.QEvent.Resize:
                self.adjustGeometryToHost(widget)
            if e.type()==QtCore.QEvent.ParentChange:
                self.setParent(widget.parent())
            if e.type()==QtCore.QEvent.Show:
                self.show()
                self.raise_()
            if e.type()==QtCore.QEvent.Hide:
                self.hide()
        return False

    def attachTo(self, widget):
        """ Attach and show the warning over the widget """
        if widget!=None:
            if self._disableHost:
                widget.setEnabled(False)
            widget.installEventFilter(self)
            self.setParent(widget.parent())
            self.adjustGeometryToHost(widget)
            if widget.isVisible():
                self.show()
                self.raise_()
            self._widget = widget
        elif self._widget!=None:
            self.hide()
            self.setParent(None)
            if self._disableHost:
                self._widget.setEnabled(True)
            self._widget.removeEventFilter(self)
            self._widget = None

class QWarningWidget(QAttachingLabel):
    """ QWarningWidget is a widget overlaying the whole area of a
    widget to show a warning with a single button
    """
    def __init__(self, text='', parent=None):
        """ Set a transparent background for the center align label """
        QAttachingLabel.__init__(self, text, parent)
        self.setAlignment(QtCore.Qt.AlignCenter)
        self.setWordWrap(True)

        palette = QtGui.QPalette(self.palette())
        palette.setColor(QtGui.QPalette.Window,
                         QtGui.QColor(255, 255, 255, 212))
        palette.setColor(QtGui.QPalette.WindowText,
                         QtGui.QColor(255, 0, 0, 255))
        self.setPalette(palette)

        font = QtGui.QFont(self.font())
        font.setPointSizeF(font.pointSizeF()*1.5)
        self.setFont(font)

class QDescriptionWidget(QAttachingLabel):
    """ QDescriptionWidget is a widget attached to the bottom of
    another widget to display some instruction messages """
    def __init__(self, text='', parent=None):
        """ Set a transparent background for the center align label """
        QAttachingLabel.__init__(self, text, parent)
        self._disableHost = False
        self.setAlignment(QtCore.Qt.AlignLeft)
        self.setWordWrap(False)
        self.setIndent(4)

        palette = QtGui.QPalette(self.palette())
        palette.setColor(QtGui.QPalette.Window,
                         QtGui.QColor(160, 160, 160, 128))
        palette.setColor(QtGui.QPalette.WindowText,
                         QtGui.QColor(0, 0, 0, 255))
        self.setPalette(palette)

        fontMetrics = QtGui.QFontMetrics(self.font())
        self.wHeight = fontMetrics.height()

    def adjustGeometryToHost(self, widget):
        """ The widget will stay and span the bottom of the host """
        rect = QtCore.QRect(widget.geometry())
        rect.setTop(rect.bottom() - self.wHeight)
        self.setGeometry(rect)

class QMessageDialog(QtGui.QDialog):
    """
    QMessageDialog is a QMessageBox like dialog that is modeless so that it
    works on all platforms.
    """
    def __init__(self, firstButtonText = '', secondButtonText = None,
                 messageText='', parent=None):
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
        self.label = QtGui.QLabel(messageText, self)
        self.textLayout.addWidget(self.label)
        self.buttonLayout = QtGui.QHBoxLayout()
        self.buttonLayout.addStretch()
        self.firstButton = QtGui.QPushButton(firstButtonText, self)
        self.firstButton.setDefault(True)
        self.buttonLayout.addWidget(self.firstButton)
        self.connect(self.firstButton,
                     QtCore.SIGNAL('clicked()'),
                     self.firstButtonPressed)
        if secondButtonText:
            self.secondButton = QtGui.QPushButton(secondButtonText, self)
            self.buttonLayout.addWidget(self.secondButton)
            self.connect(self.secondButton,
                         QtCore.SIGNAL('clicked()'),
                         self.secondButtonPressed)
        self.dialogLayout = QtGui.QVBoxLayout()
        self.dialogLayout.addLayout(self.textLayout)
        self.dialogLayout.addLayout(self.buttonLayout)
        self.setLayout(self.dialogLayout)
        self.result=None

    def closeEvent(self, e):
        """ closeEvent(e:QCloseEvent) -> None
        """
        self.emit(QtCore.SIGNAL('accepted'), False)
        self.result='rejected'
        e.accept()

    def firstButtonPressed(self):
        """ firstButtonPressed() -> None

        """
        self.emit(QtCore.SIGNAL('accepted'), True)
        self.result='accepted'

    def secondButtonPressed(self):
        """ secondButtonPressed() -> None

        """
        self.emit(QtCore.SIGNAL('accepted'), False)
        self.result='rejected'

    def execute(self):
        """ execute()

        """
        self.show()
        while self.result==None:
            QtCore.QCoreApplication.processEvents()
        self.destroy()
        return self.result


messageDialogContainerInstance=None

class MessageDialogContainer:

    def __init__(self):
        self.dialogs=set()

    def registerDialog(self,dialog):
        self.dialogs.add(dialog)

    def unregisterDialog(self,dialog):
        if dialog in self.dialogs:
            if dialog.isVisible():
                dialog.reject()
            self.dialogs.remove(dialog)

    def rejectAllOpenDialogs(self):
        remove=[]
        result=len(self.dialogs)>0
        for dialog in self.dialogs:
            if dialog.isVisible():
                remove.append(dialog)
        self.dialogs=set()
        for dialog in remove:
            dialog.closeEvent(QtGui.QCloseEvent())
        return result

    @staticmethod
    def instance():
        global messageDialogContainerInstance
        if messageDialogContainerInstance==None:
            messageDialogContainerInstance=MessageDialogContainer()
        return messageDialogContainerInstance
