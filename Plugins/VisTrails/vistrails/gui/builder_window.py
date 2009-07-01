
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
""" File for the builder window, the workspace of Vistrails

QBuilderWindow
"""
from PyQt4 import QtCore, QtGui
from core import system
import core.db.locator
from core.db.locator import DBLocator, FileLocator, XMLFileLocator, untitled_locator
from core.packagemanager import get_package_manager
from core.vistrail.vistrail import Vistrail
from gui.application import VistrailsApplication
from gui.graphics_view import QInteractiveGraphicsView
from gui.module_palette import QModulePalette
from gui.open_db_window import QOpenDBWindow
from gui.preferences import QPreferencesDialog
from gui.shell import QShellDialog
from gui.theme import CurrentTheme
from gui.view_manager import QViewManager
from gui.vistrail_toolbar import QVistrailViewToolBar, QVistrailInteractionToolBar
from gui.preferences import QPreferencesDialog
from gui.vis_diff import QVisualDiff
from gui.utils import build_custom_window
import copy
import core.interpreter.cached
import os
import sys

################################################################################

class QBuilderWindow(QtGui.QMainWindow):
    """
    QBuilderWindow is a main widget containing an editin area for
    VisTrails and several tool windows. Also remarks that almost all
    of QBuilderWindow components are floating dockwidget. This mimics
    a setup of an IDE

    """
    def __init__(self, parent=None):
        """ QBuilderWindow(parent: QWidget) -> QBuilderWindow
        Construct the main window with menus, toolbar, and floating toolwindow

        """
        QtGui.QMainWindow.__init__(self, parent)
        self.title = 'VisTrails Builder'
        self.setWindowTitle(self.title)
        self.setStatusBar(QtGui.QStatusBar(self))
        self.setDockNestingEnabled(True)

        self.viewManager = QViewManager()
        self.setCentralWidget(self.viewManager)

        self.modulePalette = QModulePalette(self)
        self.addDockWidget(QtCore.Qt.LeftDockWidgetArea,
                           self.modulePalette.toolWindow())

        self.viewIndex = 0
        self.dbDefault = False

        self.createActions()
        self.createMenu()
        self.createToolBar()

        self.connectSignals()

        self.shell = None

        # If this is true, we're currently executing a pipeline, so
        # We can't allow other executions.
        self._executing = False

        # This keeps track of the menu items for each package
        self._package_menu_items = {}

    def create_first_vistrail(self):
        """ create_first_vistrail() -> None
        Create untitled vistrail in interactive mode
        """
        # FIXME: when interactive and non-interactive modes are separated,
        # this autosave code can move to the viewManager
        if not self.dbDefault and untitled_locator().has_temporaries():
            if not FileLocator().prompt_autosave(self):
                untitled_locator().clean_temporaries()
        if self.viewManager.newVistrail(True):
            self.emit(QtCore.SIGNAL("changeViewState(int)"), 0)
            self.viewModeChanged(0)
        self.viewManager.set_first_view(self.viewManager.currentView())

    def sizeHint(self):
        """ sizeHint() -> QRect
        Return the recommended size of the builder window

        """
        return QtCore.QSize(1280, 768)

    def closeEvent(self, e):
        """ closeEvent(e: QCloseEvent) -> None
        Close the whole application when the builder is closed

        """
        if not self.quitVistrails():
            e.ignore()

    def keyPressEvent(self, event):
        """ keyPressEvent(event: QKeyEvent) -> None
        Capture modifiers (Ctrl, Alt, Shift) and send them to one of
        the widget under the mouse cursor. It first starts at the
        widget directly under the mouse and check if the widget has
        property named captureModifiers. If yes, it calls
        'modifiersPressed' function

        """
        if event.key() in [QtCore.Qt.Key_Control,
                           QtCore.Qt.Key_Alt,
                           QtCore.Qt.Key_Shift,
                           QtCore.Qt.Key_Meta]:
            widget = QtGui.QApplication.widgetAt(QtGui.QCursor.pos())
            if widget:
                while widget:
                    if widget.property('captureModifiers').isValid():
                        if hasattr(widget, 'modifiersPressed'):
                            widget.modifiersPressed(event.modifiers())
                        break
                    widget = widget.parent()
        QtGui.QMainWindow.keyPressEvent(self, event)
        # super(QBuilderWindow, self).keyPressEvent(event)

    def keyReleaseEvent(self, event):
        """ keyReleaseEvent(event: QKeyEvent) -> None
        Capture modifiers (Ctrl, Alt, Shift) and send them to one of
        the widget under the mouse cursor. It first starts at the
        widget directly under the mouse and check if the widget has
        property named captureModifiers. If yes, it calls
        'modifiersReleased' function

        """
        if event.key() in [QtCore.Qt.Key_Control,
                           QtCore.Qt.Key_Alt,
                           QtCore.Qt.Key_Shift,
                           QtCore.Qt.Key_Meta]:
            widget = QtGui.QApplication.widgetAt(QtGui.QCursor.pos())
            if widget:
                while widget:
                    if widget.property('captureModifiers').isValid():
                        if hasattr(widget, 'modifiersReleased'):
                            widget.modifiersReleased()
                        break
                    widget = widget.parent()
        QtGui.QMainWindow.keyReleaseEvent(self, event)

    def createActions(self):
        """ createActions() -> None
        Construct all menu/toolbar actions for builder window

        """
        self.newVistrailAction = QtGui.QAction(CurrentTheme.NEW_VISTRAIL_ICON,
                                               '&New', self)
        self.newVistrailAction.setShortcut('Ctrl+N')
        self.newVistrailAction.setStatusTip('Create a new vistrail')

        self.openFileAction = QtGui.QAction(CurrentTheme.OPEN_VISTRAIL_ICON,
                                            '&Open', self)
        self.openFileAction.setShortcut('Ctrl+O')
        self.openFileAction.setStatusTip('Open an existing vistrail from '
                                         'a file')

        self.importFileAction = QtGui.QAction(CurrentTheme.OPEN_VISTRAIL_DB_ICON,
                                              'Import', self)
        self.importFileAction.setStatusTip('Import an existing vistrail from '
                                           'a database')

        self.saveFileAction = QtGui.QAction(CurrentTheme.SAVE_VISTRAIL_ICON,
                                                '&Save', self)
        self.saveFileAction.setShortcut('Ctrl+S')
        self.saveFileAction.setStatusTip('Save the current vistrail '
                                         'to a file')
        self.saveFileAction.setEnabled(False)

        self.saveFileAsAction = QtGui.QAction('Save as...', self)
        self.saveFileAsAction.setShortcut('Ctrl+Shift+S')
        self.saveFileAsAction.setStatusTip('Save the current vistrail '
                                           'to a different file location')
        self.saveFileAsAction.setEnabled(False)

        self.exportFileAction = QtGui.QAction('Export', self)
        self.exportFileAction.setStatusTip('Export the current vistrail to '
                                           'a database')
        self.exportFileAction.setEnabled(False)

        self.closeVistrailAction = QtGui.QAction('Close', self)
        self.closeVistrailAction.setShortcut('Ctrl+W')
        self.closeVistrailAction.setStatusTip('Close the current vistrail')
        self.closeVistrailAction.setEnabled(False)

        self.saveLogAction = QtGui.QAction('Save Log...', self)
        self.saveLogAction.setStatusTip('Save the execution log to '
                                        'a file')
        self.saveLogAction.setEnabled(True)

        self.exportLogAction = QtGui.QAction('Export Log...', self)
        self.exportLogAction.setStatusTip('Save the execution log to '
                                          'a database')
        self.exportLogAction.setEnabled(True)

        self.saveWorkflowAction = QtGui.QAction('Save Workflow...', self)
        self.saveWorkflowAction.setStatusTip('Save the current workflow to '
                                             'a file')
        self.saveWorkflowAction.setEnabled(True)

        self.exportWorkflowAction = QtGui.QAction('Export Workflow...', self)
        self.exportWorkflowAction.setStatusTip('Save the current workflow to '
                                               'a database')
        self.exportWorkflowAction.setEnabled(True)

        self.saveVersionTreeToPDFAction = \
            QtGui.QAction('Save Version Tree as PDF...', self)
        self.saveVersionTreeToPDFAction.setStatusTip('Save the current version'
                                                     'tree to a PDF file')
        self.saveVersionTreeToPDFAction.setEnabled(True)

        self.saveWorkflowToPDFAction = \
            QtGui.QAction('Save Workflow as PDF...', self)
        self.saveWorkflowToPDFAction.setStatusTip('Save the current workflow'
                                                     'to a PDF file')
        self.saveWorkflowToPDFAction.setEnabled(True)

        self.quitVistrailsAction = QtGui.QAction('Quit', self)
        self.quitVistrailsAction.setShortcut('Ctrl+Q')
        self.quitVistrailsAction.setStatusTip('Exit Vistrails')

        self.undoAction = QtGui.QAction(CurrentTheme.UNDO_ICON,
                                        'Undo', self)
        self.undoAction.setEnabled(False)
        self.undoAction.setStatusTip('Undo the previous action')
        self.undoAction.setShortcut('Ctrl+Z')

        self.redoAction = QtGui.QAction(CurrentTheme.REDO_ICON,
                                        'Redo', self)
        self.redoAction.setEnabled(False)
        self.redoAction.setStatusTip('Redo an undone action')
        self.redoAction.setShortcut('Ctrl+Y')

        self.copyAction = QtGui.QAction('Copy\tCtrl+C', self)
        self.copyAction.setEnabled(False)
        self.copyAction.setStatusTip('Copy selected modules in '
                                     'the current pipeline view')

        self.pasteAction = QtGui.QAction('Paste\tCtrl+V', self)
        self.pasteAction.setEnabled(False)
        self.pasteAction.setStatusTip('Paste copied modules in the clipboard '
                                      'into the current pipeline view')

        self.groupAction = QtGui.QAction('Group', self)
        self.groupAction.setShortcut('Ctrl+G')
        self.groupAction.setEnabled(False)
        self.groupAction.setStatusTip('Group the '
                                      'selected modules in '
                                      'the current pipeline view')
        self.ungroupAction = QtGui.QAction('Ungroup', self)
        self.ungroupAction.setShortcut('Ctrl+Shift+G')
        self.ungroupAction.setEnabled(False)
        self.ungroupAction.setStatusTip('Ungroup the '
                                      'selected groups in '
                                      'the current pipeline view')

        self.selectAllAction = QtGui.QAction('Select All\tCtrl+A', self)
        self.selectAllAction.setEnabled(False)
        self.selectAllAction.setStatusTip('Select all modules in '
                                          'the current pipeline view')

        self.editPreferencesAction = QtGui.QAction('Preferences...', self)
        self.editPreferencesAction.setEnabled(True)
        self.editPreferencesAction.setStatusTip('Edit system preferences')

        self.shellAction = QtGui.QAction(CurrentTheme.CONSOLE_MODE_ICON,
                                         'VisTrails Console', self)
        self.shellAction.setCheckable(True)
        self.shellAction.setShortcut('Ctrl+H')

        self.pipViewAction = QtGui.QAction('Picture-in-Picture', self)
        self.pipViewAction.setCheckable(True)
        self.pipViewAction.setChecked(True)

        self.methodsViewAction = QtGui.QAction('Methods Panel', self)
        self.methodsViewAction.setCheckable(True)
        self.methodsViewAction.setChecked(True)

        self.setMethodsViewAction = QtGui.QAction('Set Methods Panel', self)
        self.setMethodsViewAction.setCheckable(True)
        self.setMethodsViewAction.setChecked(True)

        self.propertiesViewAction = QtGui.QAction('Properties Panel', self)
        self.propertiesViewAction.setCheckable(True)
        self.propertiesViewAction.setChecked(True)

        self.propertiesOverlayAction = QtGui.QAction('Properties Overlay', self)
        self.propertiesOverlayAction.setCheckable(True)
        self.propertiesOverlayAction.setChecked(False)

        self.helpAction = QtGui.QAction(self.tr('About VisTrails...'), self)

        a = QtGui.QAction(self.tr('Execute Current Workflow\tCtrl+Enter'),
                          self)
        self.executeCurrentWorkflowAction = a
        self.executeCurrentWorkflowAction.setEnabled(False)

        self.executeDiffAction = QtGui.QAction('Execute Version Difference', self)
        self.executeDiffAction.setEnabled(False)
        self.flushCacheAction = QtGui.QAction(self.tr('Erase Cache Contents'),
                                              self)

        self.executeQueryAction = QtGui.QAction('Execute Visual Query', self)
        self.executeQueryAction.setEnabled(False)

        self.executeExplorationAction = QtGui.QAction(
            'Execute Parameter Exploration', self)
        self.executeExplorationAction.setEnabled(False)

        self.executeShortcuts = [
            QtGui.QShortcut(QtGui.QKeySequence(QtCore.Qt.ControlModifier +
                                               QtCore.Qt.Key_Return), self),
            QtGui.QShortcut(QtGui.QKeySequence(QtCore.Qt.ControlModifier +
                                               QtCore.Qt.Key_Enter), self)
            ]
        
        self.vistrailActionGroup = QtGui.QActionGroup(self)



    def createMenu(self):
        """ createMenu() -> None
        Initialize menu bar of builder window

        """
        self.fileMenu = self.menuBar().addMenu('&File')
        self.fileMenu.addAction(self.newVistrailAction)
        self.fileMenu.addAction(self.openFileAction)
        self.fileMenu.addAction(self.saveFileAction)
        self.fileMenu.addAction(self.saveFileAsAction)
        self.fileMenu.addAction(self.closeVistrailAction)
        self.fileMenu.addSeparator()
        self.fileMenu.addAction(self.importFileAction)
        self.fileMenu.addAction(self.exportFileAction)
        self.fileMenu.addSeparator()
        self.fileMenu.addAction(self.saveLogAction)
        self.fileMenu.addAction(self.exportLogAction)
        self.fileMenu.addAction(self.saveWorkflowAction)
        self.fileMenu.addAction(self.exportWorkflowAction)
        self.fileMenu.addSeparator()
        self.fileMenu.addAction(self.saveVersionTreeToPDFAction)
        self.fileMenu.addAction(self.saveWorkflowToPDFAction)
        self.fileMenu.addSeparator()
        self.fileMenu.addAction(self.quitVistrailsAction)

        self.editMenu = self.menuBar().addMenu('&Edit')
        self.editMenu.addAction(self.undoAction)
        self.editMenu.addAction(self.redoAction)
        self.editMenu.addSeparator()
        self.editMenu.addAction(self.copyAction)
        self.editMenu.addAction(self.pasteAction)
        self.editMenu.addAction(self.selectAllAction)
        self.editMenu.addSeparator()
        self.editMenu.addAction(self.groupAction)
        self.editMenu.addAction(self.ungroupAction)
        self.editMenu.addSeparator()        
        self.editMenu.addAction(self.editPreferencesAction)

        self.viewMenu = self.menuBar().addMenu('&View')
        self.viewMenu.addAction(self.shellAction)
        self.viewMenu.addSeparator()
        self.viewMenu.addAction(self.pipViewAction)
        self.viewMenu.addAction(
            self.modulePalette.toolWindow().toggleViewAction())
        self.viewMenu.addAction(self.methodsViewAction)
        self.viewMenu.addAction(self.setMethodsViewAction)
        self.viewMenu.addAction(self.propertiesViewAction)
        self.viewMenu.addAction(self.propertiesOverlayAction)

        self.runMenu = self.menuBar().addMenu('&Run')
        self.runMenu.addAction(self.executeCurrentWorkflowAction)
        self.runMenu.addAction(self.executeDiffAction)
        self.runMenu.addAction(self.executeQueryAction)
        self.runMenu.addAction(self.executeExplorationAction)
        self.runMenu.addSeparator()
        self.runMenu.addAction(self.flushCacheAction)

        self.vistrailMenu = self.menuBar().addMenu('Vis&trail')
        self.vistrailMenu.menuAction().setEnabled(False)

        self.packagesMenu = self.menuBar().addMenu('Packages')
        self.packagesMenu.menuAction().setEnabled(False)

        self.helpMenu = self.menuBar().addMenu('Help')
        self.helpMenu.addAction(self.helpAction)

    def createToolBar(self):
        """ createToolBar() -> None
        Create a default toolbar for this builder window

        """
        self.toolBar = QtGui.QToolBar(self)
        self.toolBar.setWindowTitle('Vistrail File')
        self.toolBar.setToolButtonStyle(QtCore.Qt.ToolButtonTextUnderIcon)
        self.addToolBar(self.toolBar)
        self.toolBar.addAction(self.newVistrailAction)
        self.toolBar.addAction(self.openFileAction)
        self.toolBar.addAction(self.saveFileAction)
        self.toolBar.addSeparator()
        self.toolBar.addAction(self.undoAction)
        self.toolBar.addAction(self.redoAction)

        self.viewToolBar = QVistrailViewToolBar(self)
        self.addToolBar(self.viewToolBar)

        self.interactionToolBar = QVistrailInteractionToolBar(self)
        self.addToolBar(self.interactionToolBar)

    def connectSignals(self):
        """ connectSignals() -> None
        Map signals between various GUI components

        """
        self.connect(self.viewManager,
                     QtCore.SIGNAL('moduleSelectionChange'),
                     self.moduleSelectionChange)
        self.connect(self.viewManager,
                     QtCore.SIGNAL('versionSelectionChange'),
                     self.versionSelectionChange)
        self.connect(self.viewManager,
                     QtCore.SIGNAL('execStateChange()'),
                     self.execStateChange)
        self.connect(self.viewManager,
                     QtCore.SIGNAL('currentVistrailChanged'),
                     self.currentVistrailChanged)
        self.connect(self.viewManager,
                     QtCore.SIGNAL('vistrailChanged()'),
                     self.vistrailChanged)
        self.connect(self.viewManager,
                     QtCore.SIGNAL('vistrailViewAdded'),
                     self.vistrailViewAdded)
        self.connect(self.viewManager,
                     QtCore.SIGNAL('vistrailViewRemoved'),
                     self.vistrailViewRemoved)

        self.connect(QtGui.QApplication.clipboard(),
                     QtCore.SIGNAL('dataChanged()'),
                     self.clipboardChanged)

        trigger_actions = [
            (self.redoAction, self.viewManager.redo),
            (self.undoAction, self.viewManager.undo),
            (self.copyAction, self.viewManager.copySelection),
            (self.pasteAction, self.viewManager.pasteToCurrentPipeline),
            (self.selectAllAction, self.viewManager.selectAllModules),
            (self.groupAction, self.viewManager.group),
            (self.ungroupAction, self.viewManager.ungroup),
            (self.newVistrailAction, self.newVistrail),
            (self.openFileAction, self.open_vistrail_default),
            (self.importFileAction, self.import_vistrail_default),
            (self.saveFileAction, self.save_vistrail_default),
            (self.saveFileAsAction, self.save_vistrail_default_as),
            (self.exportFileAction, self.export_vistrail_default),
            (self.closeVistrailAction, self.viewManager.closeVistrail),
            (self.saveLogAction, self.save_log_default),
            (self.exportLogAction, self.export_log_default),
            (self.saveWorkflowAction, self.save_workflow_default),
            (self.exportWorkflowAction, self.export_workflow_default),
            (self.saveVersionTreeToPDFAction, self.save_tree_to_pdf),
            (self.saveWorkflowToPDFAction, self.save_workflow_to_pdf),
            (self.helpAction, self.showAboutMessage),
            (self.editPreferencesAction, self.showPreferences),
            (self.executeCurrentWorkflowAction,
             self.execute_current_pipeline),
            (self.executeDiffAction, self.showDiff),
            (self.executeQueryAction, self.queryVistrail),
            (self.executeExplorationAction,
             self.execute_current_exploration),
            (self.flushCacheAction, self.flush_cache),
            (self.quitVistrailsAction, self.quitVistrails),
            ]

        for (emitter, receiver) in trigger_actions:
            self.connect(emitter, QtCore.SIGNAL('triggered()'), receiver)

        self.connect(self.pipViewAction,
                     QtCore.SIGNAL('triggered(bool)'),
                     self.viewManager.setPIPMode)

        self.connect(self.methodsViewAction,
                     QtCore.SIGNAL('triggered(bool)'),
                     self.viewManager.setMethodsMode)

        self.connect(self.setMethodsViewAction,
                     QtCore.SIGNAL('triggered(bool)'),
                     self.viewManager.setSetMethodsMode)

        self.connect(self.propertiesViewAction,
                     QtCore.SIGNAL('triggered(bool)'),
                     self.viewManager.setPropertiesMode)

        self.connect(self.propertiesOverlayAction,
                     QtCore.SIGNAL('triggered(bool)'),
                     self.viewManager.setPropertiesOverlayMode)

        self.connect(self.vistrailActionGroup,
                     QtCore.SIGNAL('triggered(QAction *)'),
                     self.vistrailSelectFromMenu)

        self.connect(self.shellAction,
                     QtCore.SIGNAL('triggered(bool)'),
                     self.showShell)

        for shortcut in self.executeShortcuts:
            self.connect(shortcut,
                         QtCore.SIGNAL('activated()'),
                         self.execute_current_pipeline)

        self.connect_package_manager_signals()

    def connect_package_manager_signals(self):
        """ connect_package_manager_signals()->None
        Connect specific signals related to the package manager """
        pm = get_package_manager()
        self.connect(pm,
                     pm.add_package_menu_signal,
                     self.add_package_menu_items)
        self.connect(pm,
                     pm.remove_package_menu_signal,
                     self.remove_package_menu_items)
        self.connect(pm,
                     pm.package_error_message_signal,
                     self.show_package_error_message)

    def add_package_menu_items(self, pkg_id, pkg_name, items):
        """add_package_menu_items(pkg_id: str,pkg_name: str,items: list)->None
        Add a pacckage menu entry with submenus defined by 'items' to
        Packages menu.

        """
        if len(self._package_menu_items) == 0:
            self.packagesMenu.menuAction().setEnabled(True)

        # we don't support a menu hierarchy yet, only a flat list
        # this can be added later
        if not self._package_menu_items.has_key(pkg_id):
            pkg_menu = self.packagesMenu.addMenu(str(pkg_name))
            self._package_menu_items[pkg_id] = pkg_menu
        else:
            pkg_menu = self._package_menu_items[pkg_id]
            pkg_menu.clear()
        for item in items:
            (name, callback) = item
            action = QtGui.QAction(name,self)
            self.connect(action, QtCore.SIGNAL('triggered()'),
                         callback)
            pkg_menu.addAction(action)

    def remove_package_menu_items(self, pkg_id):
        """remove_package_menu_items(pkg_id: str)-> None
        removes all menu entries from the Packages Menu created by pkg_id """
        if self._package_menu_items.has_key(pkg_id):
            pkg_menu = self._package_menu_items[pkg_id]
            del self._package_menu_items[pkg_id]
            pkg_menu.clear()
            pkg_menu.deleteLater()
        if len(self._package_menu_items) == 0:
            self.packagesMenu.menuAction().setEnabled(False)

    def show_package_error_message(self, pkg_id, pkg_name, msg):
        """show_package_error_message(pkg_id: str, pkg_name: str, msg:str)->None
        shows a message box with the message msg.
        Because the way initialization is being set up, the messages will be
        shown after the builder window is shown.

        """
        msgbox = build_custom_window("Package %s (%s) says:"%(pkg_name,pkg_id),
                                    msg,
                                    modal=True,
                                    parent=self)
        #we cannot call self.msgbox.exec_() or the initialization will hang
        # creating a modal window and calling show() does not cause it to hang
        # and forces the messages to be shown on top of the builder window after
        # initialization
        msgbox.show()

    def setDBDefault(self, on):
        """ setDBDefault(on: bool) -> None
        The preferences are set to turn on/off read/write from db instead of
        file. Update the state accordingly.

        """
        self.dbDefault = on
        if self.dbDefault:
            self.openFileAction.setIcon(CurrentTheme.OPEN_VISTRAIL_DB_ICON)
            self.openFileAction.setStatusTip('Open an existing vistrail from '
                                             'a database')
            self.importFileAction.setIcon(CurrentTheme.OPEN_VISTRAIL_ICON)
            self.importFileAction.setStatusTip('Import an existing vistrail '
                                               ' from a file')
            self.saveFileAction.setStatusTip('Save the current vistrail '
                                             'to a database')
            self.saveFileAsAction.setStatusTip('Save the current vistrail to a '
                                               'different database location')
            self.exportFileAction.setStatusTip('Save the current vistrail to '
                                               ' a file')
            self.exportLogAction.setStatusTip('Save the execution log to '
                                              'a file')
            self.saveLogAction.setStatusTip('Save the execution log to '
                                            'a database')
            self.exportWorkflowAction.setStatusTip('Save the current workflow '
                                                   'to a file')
            self.saveWorkflowAction.setStatusTip('Save the current workflow '
                                                 'to a database')


        else:
            self.openFileAction.setIcon(CurrentTheme.OPEN_VISTRAIL_ICON)
            self.openFileAction.setStatusTip('Open an existing vistrail from '
                                             'a file')
            self.importFileAction.setIcon(CurrentTheme.OPEN_VISTRAIL_DB_ICON)
            self.importFileAction.setStatusTip('Import an existing vistrail '
                                               ' from a database')
            self.saveFileAction.setStatusTip('Save the current vistrail '
                                             'to a file')
            self.saveFileAsAction.setStatusTip('Save the current vistrail to a '
                                               'different file location')
            self.exportFileAction.setStatusTip('Save the current vistrail to '
                                               ' a database')
            self.saveLogAction.setStatusTip('Save the execution log to '
                                            'a file')
            self.exportLogAction.setStatusTip('Export the execution log to '
                                              'a database')
            self.saveWorkflowAction.setStatusTip('Save the current workflow '
                                                 'to a file')
            self.exportWorkflowAction.setStatusTip('Save the current workflow '
                                                   'to a database')

    def moduleSelectionChange(self, selection):
        """ moduleSelectionChange(selection: list[id]) -> None
        Update the status of tool bar buttons if there is module selected

        """
        self.copyAction.setEnabled(len(selection)>0)
        self.groupAction.setEnabled(len(selection)>0)
        self.ungroupAction.setEnabled(len(selection)>0)

    def versionSelectionChange(self, versionId):
        """ versionSelectionChange(versionId: int) -> None
        Update the status of tool bar buttons if there is a version selected

        """
        self.undoAction.setEnabled(versionId>0)
        self.selectAllAction.setEnabled(self.viewManager.canSelectAll())
        currentView = self.viewManager.currentWidget()
        if currentView:
            self.redoAction.setEnabled(currentView.can_redo())
        else:
            self.redoAction.setEnabled(False)

    def execStateChange(self):
        """ execStateChange() -> None
        Something changed on the canvas that effects the execution state,
        update interface accordingly.

        """
        currentView = self.viewManager.currentWidget()
        if currentView:
            # Update toolbars
            if self.viewIndex == 2:
                self.emit(QtCore.SIGNAL("executeEnabledChanged(bool)"),
                          currentView.execQueryEnabled) 
            elif self.viewIndex == 3:
                self.emit(QtCore.SIGNAL("executeEnabledChanged(bool)"),
                          currentView.execExploreEnabled)
            else: 
                self.emit(QtCore.SIGNAL("executeEnabledChanged(bool)"),
                          currentView.execPipelineEnabled)

            # Update menu
            self.executeCurrentWorkflowAction.setEnabled(
                currentView.execPipelineEnabled)
            self.executeDiffAction.setEnabled(currentView.execDiffEnabled)
            self.executeQueryAction.setEnabled(currentView.execQueryEnabled)
            self.executeExplorationAction.setEnabled(currentView.execExploreEnabled)
        else:
            self.emit(QtCore.SIGNAL("executeEnabledChanged(bool)"),
                      False)
            self.executeCurrentWorkflowAction.setEnabled(False)
            self.executeDiffAction.setEnabled(False)
            self.executeQueryAction.setEnabled(False)
            self.executeExplorationAction.setEnabled(False)

    def viewModeChanged(self, index):
        """ viewModeChanged(index: int) -> None
        Update the state of the view buttons

        """
        self.viewIndex = index
        self.execStateChange()
        self.viewManager.viewModeChanged(index)

    def clipboardChanged(self, mode=QtGui.QClipboard.Clipboard):
        """ clipboardChanged(mode: QClipboard) -> None
        Update the status of tool bar buttons when the clipboard
        contents has been changed

        """
        clipboard = QtGui.QApplication.clipboard()
        self.pasteAction.setEnabled(not clipboard.text().isEmpty())

    def currentVistrailChanged(self, vistrailView):
        """ currentVistrailChanged(vistrailView: QVistrailView) -> None
        Redisplay the new title of vistrail

        """
        self.execStateChange()
        if vistrailView:
            self.setWindowTitle(self.title + ' - ' +
                                vistrailView.windowTitle())
            self.saveFileAction.setEnabled(True)
            self.closeVistrailAction.setEnabled(True)
            self.saveFileAsAction.setEnabled(True)
            self.exportFileAction.setEnabled(True)
            self.vistrailMenu.menuAction().setEnabled(True)
        else:
            self.setWindowTitle(self.title)
            self.saveFileAction.setEnabled(False)
            self.closeVistrailAction.setEnabled(False)
            self.saveFileAsAction.setEnabled(False)
            self.exportFileAction.setEnabled(False)
            self.vistrailMenu.menuAction().setEnabled(False)

        if vistrailView and vistrailView.viewAction:
            vistrailView.viewAction.setText(vistrailView.windowTitle())
            if not vistrailView.viewAction.isChecked():
                vistrailView.viewAction.setChecked(True)


    def vistrailChanged(self):
        """ vistrailChanged() -> None
        An action was performed on the current vistrail

        """
        self.saveFileAction.setEnabled(True)
        self.saveFileAsAction.setEnabled(True)
        self.exportFileAction.setEnabled(True)

    def newVistrail(self):
        """ newVistrail() -> None
        Start a new vistrail, unless user cancels during interaction.

        FIXME: There should be a separation between the interactive
        and non-interactive parts.

        """
        if self.viewManager.newVistrail(False):
            self.emit(QtCore.SIGNAL("changeViewState(int)"), 0)
            self.viewModeChanged(0)

    def open_vistrail(self, locator_class):
        """ open_vistrail(locator_class) -> None
        Prompt user for information to get to a vistrail in different ways,
        depending on the locator class given.
        """
        locator = locator_class.load_from_gui(self, Vistrail.vtType)
        if locator:
            if locator.has_temporaries():
                if not locator_class.prompt_autosave(self):
                    locator.clean_temporaries()
            self.open_vistrail_without_prompt(locator)

    def open_vistrail_without_prompt(self, locator, version=None,
                                     execute_workflow=False):
        """open_vistrail_without_prompt(locator_class, version: int or str,
                                        execute_workflow: bool) -> None
        Open vistrail depending on the locator class given.
        If a version is given, the workflow is shown on the Pipeline View.
        I execute_workflow is True the workflow will be executed.
        """
        if not locator.is_valid():
                ok = locator.update_from_gui()
        else:
            ok = True
        if ok:
            self.viewManager.open_vistrail(locator, version, True)
            self.closeVistrailAction.setEnabled(True)
            self.saveFileAsAction.setEnabled(True)
            self.exportFileAction.setEnabled(True)
            self.vistrailMenu.menuAction().setEnabled(True)
            if version:
                self.emit(QtCore.SIGNAL("changeViewState(int)"), 0)
                self.viewModeChanged(0)
            else:
                self.emit(QtCore.SIGNAL("changeViewState(int)"), 1)
                self.viewModeChanged(1)
            if execute_workflow:
                self.execute_current_pipeline()
                
        
    def open_vistrail_default(self):
        """ open_vistrail_default() -> None
        Opens a vistrail from the file/db

        """
        if self.dbDefault:
            self.open_vistrail(DBLocator)
        else:
            self.open_vistrail(FileLocator())

    def import_vistrail_default(self):
        """ import_vistrail_default() -> None
        Imports a vistrail from the file/db

        """
        if self.dbDefault:
            self.open_vistrail(FileLocator)
        else:
            self.open_vistrail(DBLocator)

    def save_vistrail(self):
        """ save_vistrail() -> None
        Save the current vistrail to file

        """
        current_view = self.viewManager.currentWidget()
        locator = current_view.controller.locator
        if locator is None:
            class_ = FileLocator()
        else:
            class_ = type(locator)
        self.viewManager.save_vistrail(class_)

    def save_vistrail_default(self):
        """ save_vistrail_default() -> None
        Save the current vistrail to the file/db

        """
        if self.dbDefault:
            self.viewManager.save_vistrail(DBLocator)
        else:
            self.viewManager.save_vistrail(FileLocator())

    def save_vistrail_default_as(self):
        """ save_vistrail_file_as() -> None
        Save the current vistrail to the file/db

        """
        if self.dbDefault:
            self.viewManager.save_vistrail(DBLocator,
                                           force_choose_locator=True)
        else:
            self.viewManager.save_vistrail(FileLocator(),
                                           force_choose_locator=True)

    def export_vistrail_default(self):
        """ export_vistrail_default() -> None
        Export the current vistrail to the file/db

        """
        if self.dbDefault:
            self.viewManager.save_vistrail(FileLocator(),
                                           force_choose_locator=True)
        else:
            self.viewManager.save_vistrail(DBLocator,
                                           force_choose_locator=True)

    def save_log(self, invert=False, choose=True):
        # want xor of invert and dbDefault
        if (invert and not self.dbDefault) or (not invert and self.dbDefault):
            self.viewManager.save_log(DBLocator,
                                      force_choose_locator=choose)
        else:
            self.viewManager.save_log(XMLFileLocator,
                                      force_choose_locator=choose)
    def save_log_default(self):
        self.save_log(False)
    def export_log_default(self):
        self.save_log(True)

    def save_workflow(self, invert=False, choose=True):
        # want xor of invert and dbDefault
        if (invert and not self.dbDefault) or (not invert and self.dbDefault):
            self.viewManager.save_workflow(DBLocator,
                                           force_choose_locator=choose)
        else:
            self.viewManager.save_workflow(XMLFileLocator,
                                           force_choose_locator=choose)
    def save_workflow_default(self):
        self.save_workflow(False)
    def export_workflow_default(self):
        self.save_workflow(True)

    def save_tree_to_pdf(self):
        self.viewManager.save_tree_to_pdf()
    def save_workflow_to_pdf(self):
        self.viewManager.save_workflow_to_pdf()

    def quitVistrails(self):
        """ quitVistrails() -> bool
        Quit Vistrail, return False if not succeeded

        """
        if self.viewManager.closeAllVistrails():            
            QtCore.QCoreApplication.quit()
            # In case the quit() failed (when Qt doesn't have the main
            # event loop), we have to return True still
            return True
        return False

    def vistrailViewAdded(self, view):
        """ vistrailViewAdded(view: QVistrailView) -> None
        Add this vistrail to the Vistrail menu

        """
        view.viewAction = QtGui.QAction(view.windowTitle(), self)
        view.viewAction.view = view
        view.viewAction.setCheckable(True)
        self.vistrailActionGroup.addAction(view.viewAction)
        self.vistrailMenu.addAction(view.viewAction)
        view.versionTab.versionView.scene().fitToView(
            view.versionTab.versionView, True)

    def vistrailViewRemoved(self, view):
        """ vistrailViewRemoved(view: QVistrailView) -> None
        Remove this vistrail from the Vistrail menu

        """
        self.vistrailActionGroup.removeAction(view.viewAction)
        self.vistrailMenu.removeAction(view.viewAction)
        view.viewAction.view = None

    def vistrailSelectFromMenu(self, menuAction):
        """ vistrailSelectFromMenu(menuAction: QAction) -> None
        Handle clicked from the Vistrail menu

        """
        self.viewManager.setCurrentWidget(menuAction.view)

    def showShell(self, checked=True):
        """ showShell() -> None
        Display the shell console

        """
        if checked:
            self.savePythonPrompt()
            if not self.shell:
                self.shell = QShellDialog(self)
                self.connect(self.shell,QtCore.SIGNAL("shellHidden()"),
                             self.shellAction.toggle)
            self.shell.show()
        else:
            if self.shell:
                self.shell.hide()
            self.recoverPythonPrompt()

    def savePythonPrompt(self):
        """savePythonPrompt() -> None
        Keep system standard input and output internally

        """
        self.stdout = sys.stdout
        self.stdin = sys.stdin
        self.stderr = sys.stderr

    def recoverPythonPrompt(self):
        """recoverPythonPrompt() -> None
        Reassign system standard input and output to previous saved state.

        """
        sys.stdout = self.stdout
        sys.stdin = self.stdin
        sys.stderr = self.stderr

#     def showBookmarks(self, checked=True):
#         """ showBookmarks() -> None
#         Display Bookmarks Interactor Window

#         """
#         if checked:
#             if self.bookmarksWindow:
#                 self.bookmarksWindow.show()
#         else:
#             if self.bookmarksWindow:
#                 self.bookmarksWindow.hide()

    def showAboutMessage(self):
        """showAboutMessage() -> None
        Displays Application about message

        """
        class About(QtGui.QLabel):
            def mousePressEvent(self, e):
                self.emit(QtCore.SIGNAL("clicked()"))

        dlg = QtGui.QDialog(self, QtCore.Qt.FramelessWindowHint)
        layout = QtGui.QVBoxLayout()
        layout.setMargin(0)
        layout.setSpacing(0)
        bgimage = About(dlg)
        bgimage.setPixmap(CurrentTheme.DISCLAIMER_IMAGE)
        layout.addWidget(bgimage)
        dlg.setLayout(layout)
        text = "<font color=\"white\"><b>%s</b></font>" % \
               system.short_about_string()
        version = About(text, dlg)
        version.setGeometry(11,20,400,30)
        self.connect(bgimage,
                     QtCore.SIGNAL('clicked()'),
                     dlg,
                     QtCore.SLOT('accept()'))
        self.connect(version,
                     QtCore.SIGNAL('clicked()'),
                     dlg,
                     QtCore.SLOT('accept()'))
        dlg.setSizeGripEnabled(False)
        dlg.exec_()

        #QtGui.QMessageBox.about(self,self.tr("About VisTrails..."),
        #                        self.tr(system.about_string()))

    def showPreferences(self):
        """showPreferences() -> None
        Display Preferences dialog

        """
        dialog = QPreferencesDialog(self)
        dialog.exec_()

        # Update the state of the icons if changing between db and file
        # support
        dbState = getattr(VistrailsApplication.configuration, 'dbDefault')
        if self.dbDefault != dbState:
            self.setDBDefault(dbState)

    def showDiff(self):
        """showDiff() -> None
        Show the visual difference interface

        """
        currentView = self.viewManager.currentWidget()
        if (currentView and currentView.execDiffId1 > 0 and
            currentView.execDiffId2 > 0):
            visDiff = QVisualDiff(currentView.controller.vistrail,
                                  currentView.execDiffId1,
                                  currentView.execDiffId2,
                                  currentView.controller,
                                  self)
            visDiff.show()

    def execute(self, index):
        """ execute(index: int) -> None
        Execute something depending on the view

        """
        if index == 2:
            self.queryVistrail()
        elif index == 3:
            self.execute_current_exploration()
        else:
            self.execute_current_pipeline()

    def queryVistrail(self):
        """ queryVistrail() -> None
        Execute a query and switch to history view if in query or explore mode

        """
        if self.viewIndex > 1:
            self.emit(QtCore.SIGNAL("changeViewState(int)"), 1)
            self.viewModeChanged(1)
        self.viewManager.queryVistrail()

    def flush_cache(self):
        core.interpreter.cached.CachedInterpreter.flush()

    def execute_current_exploration(self):
        """execute_current_exploration() -> None
        Executes the current parameter exploration, if possible.

        """
        if self._executing:
            return
        self._executing = True
        try:
            self.emit(QtCore.SIGNAL("executeEnabledChanged(bool)"),
                      False)
            self.viewManager.executeCurrentExploration()
        finally:
            self._executing = False
            self.emit(QtCore.SIGNAL("executeEnabledChanged(bool)"),
                      True)

    def execute_current_pipeline(self):
        """execute_current_pipeline() -> None
        Executes the current pipeline, if possible.

        """
        if self._executing:
            return
        self._executing = True
        try:
            self.emit(QtCore.SIGNAL("executeEnabledChanged(bool)"),
                      False)
            self.viewManager.executeCurrentPipeline()
        finally:
            self._executing = False
            self.emit(QtCore.SIGNAL("executeEnabledChanged(bool)"),
                      True)

################################################################################


# import unittest
# import api

# class TestBuilderWindow(unittest.TestCase):

#     def test_close_actions_enabled(self):
        
