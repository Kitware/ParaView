
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

from PyQt4 import QtGui, QtCore
from gui.builder_window import QBuilderWindow
from gui.theme import CurrentTheme
from gui.vistrail_toolbar import QVistrailInteractionToolBar
from plugin.pgui.search_toolbar import QSearchToolBar
from plugin.pgui.playback_toolbar import QPlaybackToolBar
from plugin.pgui.preferences import QPluginPreferencesDialog
from plugin.pgui.common_widgets import QWarningWidget
from plugin.pgui.common_widgets import QMessageDialog
from plugin.pgui.common_widgets import MessageDialogContainer
from core.db.locator import FileLocator, untitled_locator
from plugin.pcore.patch import PluginPatch
import core.system
import CaptureAPI

class QPluginOperationEvent(QtCore.QEvent):
    """
    QPluginOperationEvent is used to send action changes from the plugin to the
    builder window in a thread-safe manner

    """
    eventType = QtCore.QEvent.Type(QtCore.QEvent.User)
    def __init__(self, name, snapshot, operations, fromVersion):
        """ QPluginOperationEvent()
        """
        QtCore.QEvent.__init__(self, self.eventType)
        self.name = name
        self.snapshot = snapshot
        self.operations = operations
        self.fromVersion = fromVersion

class QPluginBuilderWindow(QBuilderWindow):
    def __init__(self, parent=None):
        QBuilderWindow.__init__(self, parent)
        self.descriptionWidget = QtGui.QLabel()
        self.progressWidget = QtGui.QProgressBar()
        self.progressWidget.setRange(0,100)
        self.statusBar().addWidget(self.descriptionWidget,1)
        self.statusBar().addWidget(self.progressWidget,1)
        self.descriptionWidget.hide()
        self.progressWidget.hide()

        self.title = 'Provenance Explorer'
        self.setWindowTitle(self.title)
        self.setWindowIcon(CurrentTheme.APPLICATION_ICON)
        self.modulePalette.toolWindow().hide()
        self.updateApp = False
        self.timeStatsWindow = None

        if core.system.systemType in ['Darwin']:
            self.menuBar().setAttribute(QtCore.Qt.WA_MacSmallSize)

        self.searchToolBar = QSearchToolBar(self)
        self.addToolBar(QtCore.Qt.BottomToolBarArea, self.searchToolBar)

        self.addToolBarBreak(QtCore.Qt.BottomToolBarArea)

        self.playbackToolBar = QPlaybackToolBar(self)
        self.addToolBar(QtCore.Qt.BottomToolBarArea, self.playbackToolBar)

        self.addonToolBar.addAction(self.searchToolBar.toggleViewAction())
        self.addonToolBar.addAction(self.playbackToolBar.toggleViewAction())

        self.toolsMenu.addSeparator()
        self.toolsMenu.addAction(self.searchToolBar.toggleViewAction())
        self.toolsMenu.addAction(self.playbackToolBar.toggleViewAction())

        self.interactionToolBar._selectCursorAction.setToolTip('Select')
        self.interactionToolBar._selectCursorAction.setStatusTip('Select versions and edit the tree view')
        self.interactionToolBar._panCursorAction.setToolTip('Pan')
        self.interactionToolBar._panCursorAction.setStatusTip('Pan the tree view (Shift + Click)')
        self.interactionToolBar._zoomCursorAction.setToolTip('Zoom')
        self.interactionToolBar._zoomCursorAction.setStatusTip('Zoom the tree view (Ctrl + Click)')

        self.connect(self.viewManager,
                     QtCore.SIGNAL('currentVistrailChanged'),
                     self.updateAddonToolBar)

        self.warningWidget = QWarningWidget()
        self.connect(self.warningWidget,
                     QtCore.SIGNAL('linkActivated(QString)'),
                     self.warningLinkActivated)
        geometry = CaptureAPI.getPreference('VisTrailsBuilderWindowGeometry')
        #if geometry!=None:
        if geometry!='':
            self.restoreGeometry(QtCore.QByteArray.fromBase64(QtCore.QByteArray(geometry)))
            desktop = QtGui.QDesktopWidget()
            if desktop.screenNumber(self)==-1:
                self.move(desktop.screenGeometry().topLeft())

    def warningLinkActivated(self, link):
        """ warningLinkActivated(link: str) -> None
        Check to see which hot link has been clicked on the
        warning message
        """
        if link=='resetUndoStack':
            CaptureAPI.resetUndoStack()
        self.hideUndoStackWarning()

    def hideUndoStackWarning(self):
        self.warningWidget.attachTo(None)

    def showUndoStackWarning(self):
        """ Inform users that the undo stack has been turned off """
        if not self.warningWidget.isVisible():
            text = "<html>" \
                   "Application's Undo Stack is currently disabled. " \
                   "VisTrails is still recording changes but no provenance " \
                   "traces will be issued until the Undo Stack is turned back on." \
                   "<br />" \
                   "<a href=\"resetUndoStack\">Reset the Undo Stack</a>" \
                   "</html>"
            self.warningWidget.setText(text)
            self.warningWidget.attachTo(self.centralWidget())

    def create_first_vistrail(self):
        """ create_first_vistrail() -> None

        """
        # FIXME: when interactive and non-interactive modes are separated,
        # this autosave code can move to the viewManager
        if not self.dbDefault and untitled_locator().has_temporaries():
            if not FileLocator().prompt_autosave(self):
                untitled_locator().clean_temporaries()
        if not self.dbDefault:
            core.system.clean_temporary_save_directory()
        if self.viewManager.newVistrail(True):
            self.viewModeChanged(1)
        self.viewManager.set_first_view(self.viewManager.currentView())

        # Pre-compute default values for existing nodes in a new scene
        controller = self.viewManager.currentWidget().controller
        controller.store_preset_attributes()
        controller.change_selected_version(0)
        self.connect(controller,
                     QtCore.SIGNAL('versionWasChanged'),
                     self.versionSelectionChange)

        self.updateAddonToolBar(self.viewManager.currentWidget())



    def sizeHint(self):
        """ sizeHint() -> QRect
        Return the recommended size of the builder window

        """
        return QtCore.QSize(360, 500)

    def newVistrail(self):
        """ newVistrail() -> None
        Start a new vistrail

        """

        # don't break maya!
        if hasattr(CaptureAPI, 'beforeNewVistrail'):
            CaptureAPI.beforeNewVistrail()

        QBuilderWindow.newVistrail(self)
        self.viewModeChanged(1)

        # Pre-compute default values for existing nodes in a new scene
        controller = self.viewManager.currentWidget().controller
        controller.store_preset_attributes()
        self.connect(controller,
                     QtCore.SIGNAL('versionWasChanged'),
                     self.versionSelectionChange)
        self.updateAddonToolBar(self.viewManager.currentWidget())
        CaptureAPI.afterNewVistrail()

    def open_vistrail(self, locator_class):
        """ open_vistrail(locator_class: locator) -> None

        """
        QBuilderWindow.open_vistrail(self, locator_class)
        self.updateAddonToolBar(self.viewManager.currentWidget())

    def setUpdateAppEnabled(self, b):
        """ setUpdateAppEnabled(b: bool)
        Enable/Disable updating app when the version change in the
        version tree

        """
        self.updateApp = b

    def changeVersionWithoutUpdatingApp(self, version):
        """ changeVersionWithoutUpdate(version: int)
        Change the current version of the version tree without updating the app

        """
        if version<0:
            version = 0
        b = self.updateApp
        self.setUpdateAppEnabled(False)
        controller = self.viewManager.currentWidget().controller
        if controller:
            controller.change_selected_version(version)
        self.setUpdateAppEnabled(b)

    def addNewVersion(self, name, snapshot, ops, fromVersion):
        """ addNewVersion(name: str, snapshot: int, ops: str, fromVersion: int)
        Post the operation event to self for adding it to the version
        tree. 'fromVersion' indicates where the new version should derive
        from (the value of -1 to indicate the current version).

        """
        QtCore.QCoreApplication.sendEvent(self, QPluginOperationEvent(name, snapshot, ops, fromVersion))

    def event(self, e):
        """ event(e: QEvent)
        Parse QPluginOperationEvent and store it to Visrails
        """
        if (e.type()==QPluginOperationEvent.eventType) \
        and (self.viewManager.currentWidget()!=None):
            controller = self.viewManager.currentWidget().controller
            if e.fromVersion>=0:
                self.changeVersionWithoutUpdatingApp(e.fromVersion)
            controller.update_scene_script(e.name, e.snapshot, e.operations)
            return False
        return QBuilderWindow.event(self, e)

    def createToolBar(self):
        """ createToolBar() -> None
        Create a plugin specific toolbar

        """
        iconSize = 20
        self.toolBar = QtGui.QToolBar(self)
        self.toolBar.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
        self.toolBar.setIconSize(QtCore.QSize(iconSize,iconSize))
        self.toolBar.layout().setSpacing(1)

        self.addToolBar(self.toolBar)
        self.toolBar.addAction(self.newVistrailAction)
        self.toolBar.addAction(self.openFileAction)
        self.toolBar.addAction(self.saveFileAction)
        self.toolBar.addSeparator()
        self.toolBar.addAction(self.undoAction)
        self.toolBar.addAction(self.redoAction)

        self.addonToolBar = QtGui.QToolBar(self)
        self.addonToolBar.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
        self.addonToolBar.setIconSize(QtCore.QSize(iconSize,iconSize))
        self.addonToolBar.layout().setSpacing(1)
        self.addToolBar(self.addonToolBar)

        self.interactionToolBar = QVistrailInteractionToolBar(self)
        self.interactionToolBar.setToolButtonStyle(QtCore.Qt.ToolButtonIconOnly)
        self.interactionToolBar.setIconSize(QtCore.QSize(iconSize,iconSize))
        self.interactionToolBar.layout().setSpacing(1)
        self.addToolBar(self.interactionToolBar)

    def createActions(self):
        """ createActions() -> None
        Construct Plug-in specific actions

        """
        QBuilderWindow.createActions(self)

        # Modify Core Actions
        self.redoAction.setShortcuts(['Shift+Z','Ctrl+Y'])
        self.quitVistrailsAction = QtGui.QAction('Quit Plug-In', self)
        self.quitVistrailsAction.setShortcut('Ctrl+Q')
        self.quitVistrailsAction.setStatusTip('Exit Plug-In')
        self.editPreferencesAction.setStatusTip('Edit plug-in preferences')
        self.helpAction = QtGui.QAction(self.tr('About Provenance Explorer...'), self)

        # Create plugin specific actions
        self.useRecordedViewsAction = QtGui.QAction('Use Recorded Views', self)
        self.useRecordedViewsAction.setEnabled(True)
        self.useRecordedViewsAction.setCheckable(True)
        self.useRecordedViewsAction.setChecked(
            int(CaptureAPI.getPreference('VisTrailsUseRecordedViews')))
        self.useRecordedViewsAction.setStatusTip('Use the recorded views when switching versions')

        self.expandBranchAction = QtGui.QAction('Expand Branch', self)
        self.expandBranchAction.setEnabled(True)
        self.expandBranchAction.setStatusTip('Expand all versions in the tree below the current version')

        self.collapseBranchAction = QtGui.QAction('Collapse Branch', self)
        self.collapseBranchAction.setEnabled(True)
        self.collapseBranchAction.setStatusTip('Collapse all expanded versions in the tree below the current version')

        self.collapseAllAction = QtGui.QAction('Collapse All', self)
        self.collapseAllAction.setEnabled(True)
        self.collapseAllAction.setStatusTip('Collapse all expanded branches of the tree')

        self.hideBranchAction = QtGui.QAction('Hide Branch', self)
        self.hideBranchAction.setEnabled(True)
        self.hideBranchAction.setStatusTip('Hide all versions in the tree including and below the current version')

        self.showAllAction = QtGui.QAction('Show All', self)
        self.showAllAction.setEnabled(True)
        self.showAllAction.setStatusTip('Show all hidden versions')

        self.resetViewAction = QtGui.QAction('Reset View', self)
        self.resetViewAction.setShortcut('Ctrl+R')
        self.resetViewAction.setStatusTip('Reset tree view to default view')

        self.timeStatsAction = QtGui.QAction('Time Statistics', self)
        self.timeStatsAction.setEnabled(True)
        self.timeStatsAction.setStatusTip('Show time statistics')

        self.snapshotAction = QtGui.QAction('Take Snapshot', self)
        self.snapshotAction.setEnabled(False)
        self.snapshotAction.setStatusTip('Create a new version with the contents of the current scene')

        self.visDiffAction = QtGui.QAction('Visual Difference', self)
        self.visDiffAction.setEnabled(True)
        self.visDiffAction.setStatusTip('Visually show differences between two versions')

        self.patchAction = QtGui.QAction('Patch', self)
        self.patchAction.setEnabled(True)
        self.patchAction.setStatusTip('Apply a sequence of operations to a version')

    def connectSignals(self):
        """ connectSignals() -> None
        Connect Plug-in specific signals

        """
        QBuilderWindow.connectSignals(self)
        trigger_actions = [
            (self.expandBranchAction, self.expandBranch),
            (self.collapseBranchAction, self.collapseBranch),
            (self.collapseAllAction, self.collapseAll),
            (self.hideBranchAction, self.hideBranch),
            (self.showAllAction, self.showAll),
            (self.resetViewAction, self.resetView),
            (self.timeStatsAction, self.timeStats),
            (self.snapshotAction, self.createSnapshot),
            (self.visDiffAction, self.visDiffSelection),
            (self.patchAction, self.patchSelection),
            ]
        for (emitter, receiver) in trigger_actions:
            self.connect(emitter, QtCore.SIGNAL('triggered()'), receiver)

        self.connect(self.useRecordedViewsAction,
                     QtCore.SIGNAL('toggled(bool)'),
                     self.useRecordedViewsChanged)

    def createMenu(self):
        """ create Menu() -> None
        Create a Plug-in specific menu

        """
        self.fileMenu = self.menuBar().addMenu('&File')
        self.fileMenu.addAction(self.newVistrailAction)
        self.fileMenu.addAction(self.openFileAction)
        self.fileMenu.addAction(self.saveFileAction)
        self.fileMenu.addAction(self.saveFileAsAction)
        self.fileMenu.addAction(self.quitVistrailsAction)

        self.editMenu = self.menuBar().addMenu('&Edit')
        self.editMenu.addAction(self.undoAction)
        self.editMenu.addAction(self.redoAction)
        self.editMenu.addSeparator()
        self.editMenu.addAction(self.editPreferencesAction)

        self.viewMenu = self.menuBar().addMenu('&View')
        self.viewMenu.addAction(self.expandBranchAction)
        self.viewMenu.addAction(self.collapseBranchAction)
        self.viewMenu.addAction(self.collapseAllAction)
        self.viewMenu.addSeparator()
        self.viewMenu.addAction(self.hideBranchAction)
        self.viewMenu.addAction(self.showAllAction)
        self.viewMenu.addSeparator()
        self.viewMenu.addAction(self.resetViewAction)
        self.viewMenu.addAction(self.useRecordedViewsAction)

        self.toolsMenu = self.menuBar().addMenu('Tools')
        self.toolsMenu.addAction(self.timeStatsAction)
        self.toolsMenu.addSeparator()
        self.toolsMenu.addAction(self.snapshotAction)
        self.toolsMenu.addAction(self.visDiffAction)
        self.toolsMenu.addAction(self.patchAction)

        self.helpMenu = self.menuBar().addMenu('Help')
        self.helpMenu.addAction(self.helpAction)

        # Add this dummy menu to play nice with the core VisTrails code
        self.vistrailMenu = QtGui.QMenu()

    def updateAddonToolBar(self, vistrailView):
        """ Update the controller for those add-on toolbars """
        if vistrailView:
            controller = vistrailView.controller
        else:
            controller = None
        self.searchToolBar.setController(controller)
        self.playbackToolBar.setController(controller)

    def saveWindowPreferences(self):
        """ Save the current window settings """
        CaptureAPI.setPreference('VisTrailsBuilderWindowGeometry',
                                 self.saveGeometry().toBase64().data())

    def moveEvent(self, event):
        """ Save current window settings right away"""
        self.saveWindowPreferences()
        return QBuilderWindow.moveEvent(self, event)

    def resizeEvent(self, event):
        """ Save current window settings right away"""
        self.saveWindowPreferences()
        return QBuilderWindow.resizeEvent(self, event)

    def quitVistrails(self):
        """ quitVistrails() -> bool
        Quit Vistrail, return False if not succeeded

        """
        if self.viewManager.closeAllVistrails():
            CaptureAPI.unloadPlugin()
        return False

    def expandBranch(self):
        """ expandBranch() -> None
        Expand branch of tree

        """
        controller = self.viewManager.currentWidget().controller
        controller.expand_or_collapse_all_versions_below(controller.current_version, True)

    def collapseBranch(self):
        """ collapseBranch() -> None
        Collapse branch of tree

        """
        controller = self.viewManager.currentWidget().controller
        controller.expand_or_collapse_all_versions_below(controller.current_version, False)

    def collapseAll(self):
        """ collapseAll() -> None
        Collapse all branches of tree

        """
        controller = self.viewManager.currentWidget().controller
        controller.collapse_all_versions()

    def hideBranch(self):
        """ hideBranch() -> None
        Hide node and all children

        """
        controller = self.viewManager.currentWidget().controller
        controller.hide_versions_below(controller.current_version)

    def showAll(self):
        """ showAll() -> None
        Show all hidden nodes

        """
        controller = self.viewManager.currentWidget().controller
        controller.show_all_versions()

    def resetView(self):
        """ resetView() -> None
        Reset view to default

        """
        self.viewManager.fitToView(True)

    def timeStats(self):
        """ timeStats() -> None
        Show a time histogram

        """
        from plugin.pgui.histogram_window import QTimeStatisticsWindow
        if self.timeStatsWindow ==None:
            self.timeStatsWindow=QTimeStatisticsWindow(self)

        self.timeStatsWindow.update()
        self.timeStatsWindow.show()

    def getVersionView(self):
        """ getVersionView() -> QTreeVersionVew
        Return the current version view

        """
        currentView = self.viewManager.currentWidget()
        if currentView:
            return currentView.versionTab.versionView
        return None

    def createSnapshot(self):
        """ createSnapshot() -> None
        Store the contents of the scene in an new version
        
        """
        controller = self.viewManager.currentWidget().controller
        if controller.current_version > 0:
            CaptureAPI.createSnapshot(True)

    def visDiffSelection(self):
        """ visDiffSelection() -> None
        Select 2 versions and show the visual diff

        """
        versionView = self.getVersionView()
        if versionView!=None:
            versionView.multiSelectionStart(2, 'Visual Difference',
                                            ('Select the start version',
                                            'Select the end version'))
            self.connect(versionView,
                         QtCore.SIGNAL('doneMultiSelection'),
                         self.diffMultiSelection)

    def diffMultiSelection(self, successful, versions):
        """ diffMultiSelection(successful: bool) -> None
        Handled the selection is done. successful specifies if the
        selection was completed or aborted by users.

        """
        versionView = self.getVersionView()
        if versionView!=None:
            self.disconnect(versionView, QtCore.SIGNAL('doneMultiSelection'),
                            self.diffMultiSelection)
        if successful:
            self.showVisualDiff(versions)

    def showAboutMessage(self):
        """showAboutMessage() -> None
        Displays Application about message

        """
        QtGui.QMessageBox.about(self, 'About Provenance Explorer',
                                '<b>Provenance Explorer</b><br/>'\
                                'Beta Release<br/>'\
                                'Copyright (C) 2008,2009 VisTrails, Inc. All rights reserved.')

    def useRecordedViewsChanged(self, checked=True):
        """ Update app settings accordingly """
        CaptureAPI.setPreference('VisTrailsUseRecordedViews', str(int(checked)))

    def showPreferences(self):
        """showPreferences() -> None
        Display Preferences dialog

        """
        dialog = QPluginPreferencesDialog(self)
        dialog.exec_()

    def patchSelection(self):
        """ patchSelection() -> None

        Select 2 versions and start patching process
        """
        versionView = self.getVersionView()
        if versionView!=None:
            versionView.multiSelectionStart(3, 'Patch',
                                            ('Select the first source version',
                                             'Select the last source version',
                                             'Select the destination version'))
            self.connect(versionView,
                         QtCore.SIGNAL('doneMultiSelection'),
                         self.patchTripleSelection)

    

    def patchTripleSelection(self, successful, versions):
        """ patchTripleSelection(successful: bool) -> None

        Handled the selection is done. successful specifies if the
        selection was completed or aborted by users.
        """
        versionView = self.getVersionView()
        if versionView!=None:
            self.disconnect(versionView, QtCore.SIGNAL('doneMultiSelection'),
                            self.patchTripleSelection)
        if successful:
            controller = self.viewManager.currentWidget().controller
            if not PluginPatch.isValidPatch(controller, *versions):
                QtGui.QMessageBox.critical(self,
                                           'Invalid versions',
                                           'The selected versions cannot be used for a patch. ' \
                                           'The last source version must be a descendant of the ' \
                                           'first source version. The selection has been canceled.')
                self.messageDialog = None
                self.hidePatch(False)
            else:
                PluginPatch.start(controller, *versions)
                def finishPatch():
                    self.messageDialog = QMessageDialog('Accept', 'Discard',
                                                        'Applied ' + PluginPatch.getReportText() +
                                                        ' operation(s).\nAccept the patch?', self)
                    self.connect(self.messageDialog,
                                 QtCore.SIGNAL('accepted'),
                                 self.hidePatch)
                    self.messageDialog.show()
                    MessageDialogContainer.instance().registerDialog(self.messageDialog)
                CaptureAPI.executeDeferred(finishPatch)

    def hidePatch(self, accepted):
        """ hidePatch(accepted: bool) -> None

        """
        versionView = self.getVersionView()
        versionView.multiSelectionAbort('Patch')
        PluginPatch.stop(accepted)
        if self.messageDialog:
            self.messageDialog.destroy()
            MessageDialogContainer.instance().unregisterDialog(self.messageDialog)

    def showVisualDiff(self, versions):
        """ Show the visual diff window """
        startVersion=versions[0]
        endVersion=versions[1]
        controller=self.viewManager.currentWidget().controller
        commonVersion=controller.vistrail.getFirstCommonVersion(startVersion,endVersion)
        CaptureAPI.startVisualDiff(commonVersion,startVersion,endVersion)
        def finishVisualDiff():
            self.messageDialog = QMessageDialog('Close', None,
                                                'Close the Visual Difference?',
                                                self)
            self.connect(self.messageDialog,
                         QtCore.SIGNAL('accepted'),
                         self.hideVisualDiff)
            self.messageDialog.show()
            MessageDialogContainer.instance().registerDialog(self.messageDialog)
            #self.hideVisualDiff(self.messageDialog.execute()=='accepted')
        CaptureAPI.executeDeferred(finishVisualDiff)

    def hideVisualDiff(self,accepted=True):
        """ hideVisualDiff(accepted: bool) -> None
        Clean up the visual diff interface.
        """
        self.getVersionView().multiSelectionAbort('Visual Difference')
        CaptureAPI.stopVisualDiff()

        #self.messageDialog.destroy()
        self.messageDialog.reject()
        MessageDialogContainer.instance().unregisterDialog(self.messageDialog)

        controller = self.viewManager.currentWidget().controller
        controller.update_app_with_current_version(0, False)

    def startProgress(self):
        """ startProgress() -> None
        Show the progress bar

        """
        self.progressWidget.reset()
        self.progressWidget.show()

    def endProgress(self):
        """ endProgress() -> None
        Hide the progress bar

        """
        self.progressWidget.hide()

    def updateProgress(self, val):
        """ updateProgress(val: float) -> None
        Set the progress bar status.  val is in (0,1].

        """
        self.progressWidget.setValue(val*100)

    def versionSelectionChange(self, versionId):
        """ versionSelectionChange(versionId: int) -> None
        Setup state of actions
        
        """
        self.undoAction.setEnabled(versionId>0)
        currentView = self.viewManager.currentWidget()
        if currentView:
            self.redoAction.setEnabled(currentView.can_redo())
        else:
            self.redoAction.setEnabled(False)
        self.snapshotAction.setEnabled(versionId>0)
        
        
