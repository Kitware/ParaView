
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

##############################################################################
# Patch version view

import gui.version_view
from gui.graphics_view import QInteractiveGraphicsScene, QInteractiveGraphicsView
from gui.qt import qt_super
from gui.utils import getBuilderWindow
from PyQt4 import QtCore, QtGui
from plugin.pgui.version_view import QPlaybackVersionTreeScene

def QVTV_new_init(self, parent=None):
    original_QVTV_init(self, parent)
    self.canSelectBackground = False
    self.canSelectRectangle = False
    self.setScene(QPlaybackVersionTreeScene(self))
    self.selectingIndicator = None
    self.descriptionWidget = getBuilderWindow().descriptionWidget
    self.isMultiSelecting = False
    self.multiSelectionLabel = ''
    self.multiSelectionMessages = None
    self.multiSelectionCount = 0
    self.currentSelectionStep = 0

def QVTV_mousePressEvent(self, e):
    QInteractiveGraphicsView.mousePressEvent(self, e)
    if (self.translateButton(e)==QtCore.Qt.LeftButton and
        self.selectingIndicator!=None):
        for item in self.items(e.pos()):
            if isinstance(item, gui.version_view.QGraphicsVersionItem):
                self.selectingIndicator.versionId = item.id
                self.scene().updateIndicator(self.selectingIndicator)
                self.selectingIndicator.show()
                self.emit(QtCore.SIGNAL('versionSelected()'))
                break

def QVTV_setSelectingIndicator(self, on, ind=0):
    """ Switch from interactive to just indicator selection
    mode. 'start' is used to specify if we want to select the start or
    end indicator """
    self.setInteractive(not on)
    if on:
        if ind == 0:
            self.selectingIndicator = self.scene().indicatorStart
        elif ind == 1:
            self.selectingIndicator = self.scene().indicatorEnd
        else:
            self.selectingIndicator = self.scene().indicatorExtra
    else:
        self.selectingIndicator = None

def QVTV_setIndicatorsVisible(self, start, end, extra):
    self.scene().indicatorStart.setVisible(start)
    self.scene().indicatorEnd.setVisible(end)
    self.scene().indicatorExtra.setVisible(extra)

def QVTV_setIndicatorVersions(self, start, end, extra=-1):    
    self.scene().indicatorStart.versionId = start
    self.scene().indicatorEnd.versionId = end
    self.scene().indicatorExtra.versionId = extra
    self.scene().updateIndicator(self.scene().indicatorStart)
    self.scene().updateIndicator(self.scene().indicatorEnd)
    self.scene().updateIndicator(self.scene().indicatorExtra)

def QVTV_getIndicatorVersions(self):
    return (self.scene().indicatorStart.versionId,
            self.scene().indicatorEnd.versionId,
            self.scene().indicatorExtra.versionId)

def QVTV_multiSelectionStep(self):
    self.setSelectingIndicator(True, self.currentSelectionStep)
    msg = self.multiSelectionMessages[self.currentSelectionStep]
    text = '<html><b>' + self.multiSelectionLabel + '</b>: ' \
        + msg + ' ' \
        '(<a href="cancelMultiSelection">Cancel</a>)' \
        '</html>'
    self.descriptionWidget.setText(text)
    if (self.currentSelectionStep < self.multiSelectionCount-1):
        if self.currentSelectionStep == 0:
            self.connect(self, QtCore.SIGNAL('versionSelected()'),
                         self.multiSelectionStep)
        self.currentSelectionStep += 1
    else:
        self.disconnect(self, QtCore.SIGNAL('versionSelected()'),
                        self.multiSelectionStep)
        self.connect(self, QtCore.SIGNAL('versionSelected()'),
                     self.multiSelectionDone)

def QVTV_multiSelectionDone(self):
    self.disconnect(self.descriptionWidget,
                    QtCore.SIGNAL('linkActivated(QString)'),
                    self.descriptionLinkActivated)
    self.setSelectingIndicator(False)
    self.descriptionWidget.hide()
    self.disconnect(self, QtCore.SIGNAL('versionSelected()'),
                    self.multiSelectionDone)
    versions = self.getIndicatorVersions()
    self.emit(QtCore.SIGNAL('doneMultiSelection'), True, versions)

def QVTV_multiSelectionAbort(self, label=''):
    if self.multiSelectionLabel!='' and label!=self.multiSelectionLabel:
        return
    self.disconnect(self.descriptionWidget,
                    QtCore.SIGNAL('linkActivated(QString)'),
                    self.descriptionLinkActivated)
    self.setSelectingIndicator(False)
    self.descriptionWidget.hide()
    self.setIndicatorVersions(-1, -1)
    self.disconnect(self, QtCore.SIGNAL('versionSelected()'), self.multiSelectionStep)
    self.disconnect(self, QtCore.SIGNAL('versionSelected()'), self.multiSelectionDone)
    if self.isMultiSelecting:
        self.emit(QtCore.SIGNAL('doneMultiSelection'), False, None)
        self.isMultiSelecting = False
    self.multiSelectionLabel = ''
    self.multiSelectionMessages = None
    
def QVTV_descriptionLinkActivated(self, link):
    if link=='cancelMultiSelection':
        self.multiSelectionAbort(self.multiSelectionLabel)

def QVTV_multiSelectionStart(self, count, label='', msg = None):
    self.multiSelectionCount = count
    self.acquireMultiSelection(label)
    self.setIndicatorVersions(-1, -1)
    self.descriptionWidget.setText('')
    self.descriptionWidget.show()
    self.connect(self.descriptionWidget,
                 QtCore.SIGNAL('linkActivated(QString)'),
                 self.descriptionLinkActivated)
    if msg == None:
        msg = ('Select first version',
               'Select second version',
               'Select third version')
    self.multiSelectionMessages = msg
    self.currentSelectionStep = 0
    self.multiSelectionStep()

def QVTV_acquireMultiSelection(self, label=''):
    if label!=self.multiSelectionLabel:
        if self.isMultiSelecting:
            self.multiSelectionAbort(self.multiSelectionLabel)
        self.multiSelectionLabel = label
        self.isMultiSelecting = True

def QGVI_mouseMoveEvent(self, event):
    QtGui.QGraphicsEllipseItem.mouseMoveEvent(self, event)

def QGVI_dragEnterEvent(self, event):
    event.ignore()

def QGVI_dropEvent(self, event):
    event.ignore()

def QVTS_keyPressEvent(self, event):
    selectedItems = self.selectedItems()
    versions = [item.id for item in selectedItems
                if type(item)== gui.version_view.QGraphicsVersionItem
                and not item.text.hasFocus()]
    if (self.controller and len(versions)>0 and
        event.key() in [QtCore.Qt.Key_Backspace, QtCore.Qt.Key_Delete]):
        self.controller.hide_versions_below(self.controller.current_version)
    qt_super(gui.version_view.QVersionTreeScene, self).keyPressEvent(event)


original_QVTV_init = gui.version_view.QVersionTreeView.__init__
gui.version_view.QVersionTreeView.__init__ = QVTV_new_init
gui.version_view.QVersionTreeView.mousePressEvent = QVTV_mousePressEvent
gui.version_view.QVersionTreeView.setSelectingIndicator = QVTV_setSelectingIndicator
gui.version_view.QVersionTreeView.setIndicatorsVisible = QVTV_setIndicatorsVisible
gui.version_view.QVersionTreeView.setIndicatorVersions = QVTV_setIndicatorVersions
gui.version_view.QVersionTreeView.getIndicatorVersions = QVTV_getIndicatorVersions
gui.version_view.QVersionTreeView.multiSelectionStart = QVTV_multiSelectionStart
gui.version_view.QVersionTreeView.multiSelectionStep = QVTV_multiSelectionStep
gui.version_view.QVersionTreeView.multiSelectionDone = QVTV_multiSelectionDone
gui.version_view.QVersionTreeView.multiSelectionAbort = QVTV_multiSelectionAbort
gui.version_view.QVersionTreeView.descriptionLinkActivated = QVTV_descriptionLinkActivated
gui.version_view.QVersionTreeView.acquireMultiSelection = QVTV_acquireMultiSelection

gui.version_view.QGraphicsVersionItem.dropEvent = QGVI_dropEvent
gui.version_view.QGraphicsVersionItem.dragEnterEvent = QGVI_dragEnterEvent
gui.version_view.QGraphicsVersionItem.mouseMoveEvent = QGVI_mouseMoveEvent

gui.version_view.QVersionTreeScene.keyPressEvent = QVTS_keyPressEvent
