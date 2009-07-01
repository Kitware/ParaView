
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
""" The file describes a container widget consisting of a pipeline
view and a version tree for each opened Vistrail """

import os.path
from PyQt4 import QtCore, QtGui
from core.debug import critical
from gui.common_widgets import QDockContainer, QToolWindowInterface
from gui.paramexplore.pe_tab import QParameterExplorationTab
from gui.pipeline_tab import QPipelineTab
from gui.query_tab import QQueryTab
from gui.version_tab import QVersionTab
from gui.vistrail_controller import VistrailController
from core.utils import any, profile
from core.system import vistrails_default_file_type
################################################################################

class QVistrailView(QDockContainer):
    """
    QVistrailView is a widget containing four stacked widgets: Pipeline View,
    Version Tree View, Query View and Parameter Exploration view
    for manipulating vistrails.
    """
    def __init__(self, parent=None):
        """ QVistrailItem(parent: QWidget) -> QVistrailItem
        Make it a main window with dockable area
        
        """
        QDockContainer.__init__(self, parent)
        
        # The window title is the name of the vistrail file
        self.setWindowTitle('untitled%s'%vistrails_default_file_type())

        # Create the views
        self.pipelineTab = QPipelineTab()
        self.versionTab = QVersionTab()
        self.connect(self.versionTab.versionProp,
                     QtCore.SIGNAL('textQueryChange(bool)'),
                     self.setQueryMode)

        self.pipelineTab.pipelineView.setPIPScene(
            self.versionTab.versionView.scene())
        self.versionTab.versionView.setPIPScene(            
            self.pipelineTab.pipelineView.scene())
        self.versionTab.versionView.scene()._pipeline_scene = self.pipelineTab.pipelineView.scene()
        self.queryTab = QQueryTab()

        self.peTab = QParameterExplorationTab()
        self.peTab.annotatedPipelineView.setScene(
            self.pipelineTab.pipelineView.scene())
        
        # Setup a central stacked widget for pipeline view and version
        # tree view
        self.stackedWidget = QtGui.QStackedWidget()
        self.setCentralWidget(self.stackedWidget)
        self.stackedWidget.addWidget(self.pipelineTab)
        self.stackedWidget.addWidget(self.versionTab)
        self.stackedWidget.addWidget(self.queryTab)
        self.stackedWidget.addWidget(self.peTab)
        self.stackedWidget.setCurrentIndex(1)

        # Initialize the vistrail controller
        self.controller = VistrailController()
        self.controller.vistrail_view = self
        self.connect(self.controller,
                     QtCore.SIGNAL('stateChanged'),
                     self.stateChanged)
        self.connect(self.controller,
                     QtCore.SIGNAL('new_action'),
                     self.new_action)

        self.connect(self.versionTab.versionView.scene(),
                     QtCore.SIGNAL('versionSelected(int,bool)'),
                     self.versionSelected)

        self.connect(self.versionTab,
                     QtCore.SIGNAL('twoVersionsSelected(int,int)'),
                     self.twoVersionsSelected)
        self.connect(self.queryTab,
                     QtCore.SIGNAL('queryPipelineChange'),
                     self.queryPipelineChange)
        self.connect(self.peTab,
                     QtCore.SIGNAL('exploreChange(bool)'),
                     self.exploreChange)

        # We also keep track where this vistrail comes from
        # So we can save in the right place
        self.locator = None
        
        self.closeEventHandler = None

        # the redo stack stores the undone action ids 
        # (undo is automatic with us, through the version tree)
        self.redo_stack = []

        # Keep the state of the execution button and menu items for the view
        self.execQueryEnabled = False
        self.execDiffEnabled = False
        self.execExploreEnabled = False
        self.execPipelineEnabled = False
        self.execDiffId1 = -1
        self.execDiffId2 = -1

    def updateCursorState(self, mode):
        """ updateCursorState(mode: Int) -> None 
        Change cursor state in all different modes.

        """
        self.pipelineTab.pipelineView.setDefaultCursorState(mode)
        self.versionTab.versionView.setDefaultCursorState(mode)
        self.queryTab.pipelineView.setDefaultCursorState(mode)
        if self.parent().parent().parent().pipViewAction.isChecked():
            self.pipelineTab.pipelineView.pipFrame.graphicsView.setDefaultCursorState(mode)
            self.versionTab.versionView.pipFrame.graphicsView.setDefaultCursorState(mode)


    def flush_changes(self):
        """Flush changes in the vistrail before closing or saving.
        """
        # Quick workaround for notes focus out bug (ticket #182)
        # There's probably a much better way to fix this.
        prop = self.versionTab.versionProp
        prop.versionNotes.commit_changes()

    def setup_view(self, version=None):
        """setup_view(version = None:int) -> None

        Sets up the correct view for a fresh vistrail.

        Previously, there was a method setInitialView and another
        setOpenView.

        They were supposed to do different things but the code was
        essentially identical.

        FIXME: this means that the different calls are being handled
        somewhere else in the code. Figure this out."""

        if version is None:
            self.controller.select_latest_version()
            version = self.controller.current_version
        self.versionSelected(version, False)
        self.setPIPMode(True)
        self.setQueryMode(False)
       
    def setPIPMode(self, on):
        """ setPIPMode(on: bool) -> None
        Set the PIP state for the view

        """
        self.pipelineTab.pipelineView.setPIPEnabled(on)
        self.versionTab.versionView.setPIPEnabled(on)

    def setQueryMode(self, on):
        """ setQueryMode(on: bool) -> None
        Set the Reset Query button mode for the view
        
        """
        self.pipelineTab.pipelineView.setQueryEnabled(on)
        self.versionTab.versionView.setQueryEnabled(on)
        self.queryTab.pipelineView.setQueryEnabled(on)

    def setMethodsMode(self, on):
        """ setMethodsMode(on: bool) -> None
        Set the methods panel state for the view

        """
        if on:
            self.pipelineTab.methodPalette.toolWindow().show()
        else:
            self.pipelineTab.methodPalette.toolWindow().hide()

    def setSetMethodsMode(self, on):
        """ setSetMethodsMode(on: bool) -> None
        Set the set methods panel state for the view

        """
        if on:
            self.pipelineTab.moduleMethods.toolWindow().show()
        else:
            self.pipelineTab.moduleMethods.toolWindow().hide()

    def setPropertiesMode(self, on):
        """ setPropertiesMode(on: bool) -> None
        Set the properties panel state for the view

        """
        if on:
            self.versionTab.versionProp.toolWindow().show()
        else:
            self.versionTab.versionProp.toolWindow().hide()

    def setPropertiesOverlayMode(self, on):
        """ setPropertiesMode(on: bool) -> None
        Set the properties overlay state for the view

        """
        if on:
            self.versionTab.versionView.versionProp.show()
        else:
            self.versionTab.versionView.versionProp.hide()

    def viewModeChanged(self, index):
        """ viewModeChanged(index: int) -> None        
        Slot for switching different views when the tab's current
        widget is changed
        
        """
        if self.stackedWidget.count()>index:
            self.stackedWidget.setCurrentIndex(index)

    def sizeHint(self):
        """ sizeHint(self) -> QSize
        Return recommended size of the widget
        
        """
        return QtCore.QSize(1024, 768)

    def set_vistrail(self, vistrail, locator=None):
        """ set_vistrail(vistrail: Vistrail, locator: BaseLocator) -> None
        Assign a vistrail to this view, and start interacting with it
        
        """
        self.vistrail = vistrail
        self.locator = locator
        self.controller.set_vistrail(vistrail, locator)
        self.versionTab.setController(self.controller)
        self.pipelineTab.setController(self.controller)
        self.peTab.setController(self.controller)

    def stateChanged(self):
        """ stateChanged() -> None

        Handles 'stateChanged' signal from VistrailController
        
        Update the window and tab title
        
        """
        title = self.controller.name
        if title=='':
            title = 'untitled%s'%vistrails_default_file_type()
        if self.controller.changed:
            title += '*'
        self.setWindowTitle(title)
        # propagate the state change to the version prop
        # maybe in the future we should propagate as a signal
        versionId = self.controller.current_version
        self.versionTab.versionProp.updateVersion(versionId)

    def emitDockBackSignal(self):
        """ emitDockBackSignal() -> None
        Emit a signal for the View Manager to take this widget back
        
        """
        self.emit(QtCore.SIGNAL('dockBack'), self)

    def closeEvent(self, event):
        """ closeEvent(event: QCloseEvent) -> None
        Only close if we save information
        
        """
        if self.closeEventHandler:
            if self.closeEventHandler(self):
                event.accept()
            else:
                event.ignore()
        else:
            #I think there's a problem with two pipeline views and the same
            #scene on Macs. After assigning a new scene just before deleting
            #seems to solve the problem
            self.peTab.annotatedPipelineView.setScene(QtGui.QGraphicsScene())
            return QDockContainer.closeEvent(self, event)
            # super(QVistrailView, self).closeEvent(event)

    def queryVistrail(self, on=True):
        """ queryVistrail(on: bool) -> None
        Inspecting the query tab to get a pipeline for querying
        
        """
        if on:
            queryPipeline = self.queryTab.controller.current_pipeline
            if queryPipeline:
                self.controller.query_by_example(queryPipeline)
                self.setQueryMode(True)
        else:
            self.controller.set_search(None)
            self.setQueryMode(False)

    def createPopupMenu(self):
        """ createPopupMenu() -> QMenu
        Create a pop up menu that has a list of all tool windows of
        the current tab of the view. Tool windows can be toggled using
        this menu
        
        """
        return self.stackedWidget.currentWidget().createPopupMenu()

    def executeParameterExploration(self):
        """ executeParameterExploration() -> None
        Execute the current parameter exploration in the exploration tab
        
        """
        self.peTab.performParameterExploration()

    def versionSelected(self, versionId, byClick):
        """ versionSelected(versionId: int, byClick: bool) -> None
        A version has been selected/unselected, update the controller
        and the pipeline view
        
        """
        if self.controller:
            self.controller.reset_pipeline_view = byClick
            self.controller.change_selected_version(versionId)
            self.controller.invalidate_version_tree(False)
            if byClick:
                self.controller.current_pipeline_view.fitToAllViews(True)
            self.versionTab.versionProp.updateVersion(versionId)
            self.versionTab.versionView.versionProp.updateVersion(versionId)
            self.redo_stack = []
            self.emit(QtCore.SIGNAL('versionSelectionChange'),versionId)
            self.execPipelineEnabled = versionId>-1
            self.execDiffEnabled = False
            self.execExploreChange = False
            self.emit(QtCore.SIGNAL('execStateChange()'))

    def twoVersionsSelected(self, id1, id2):
        """ twoVersionsSelected(id1: Int, id2: Int) -> None
        Just echo the signal from the view
        
        """
        self.execDiffEnabled = True
        self.execDiffId1 = id1
        self.execDiffId2 = id2
        self.emit(QtCore.SIGNAL('execStateChange()'))

    def queryPipelineChange(self, notEmpty):
        """ queryPipelineChange(notEmpty: bool) -> None
        Update the status of tool bar buttons if there are
        modules on the query canvas
        
        """
        self.execQueryEnabled = notEmpty
        self.emit(QtCore.SIGNAL('execStateChange()'))
                  
    def exploreChange(self, notEmpty):
        """ exploreChange(notEmpty: bool) -> None
        Update the status of tool bar buttons if there are
        parameters in the exploration canvas
        
        """
        self.execExploreEnabled = notEmpty
        self.emit(QtCore.SIGNAL('execStateChange()'))

    ##########################################################################
    # Undo/redo

    def set_pipeline_selection(self, old_action, new_action, optype):
        # Sets up the right selection based on the the old and new
        # actions coming from undo/redo.
        pScene = self.pipelineTab.pipelineView.scene()
        # select things, if appropriate
        def old_deletion_test():
            return (old_action and
                    any(old_action.operations,
                        lambda x: x.vtType == 'delete'))
        def old_deletion():
            # Previous action was a deletion, select what was just
            # deleted and undone
            for op in old_action.operations:
                if op.what == 'module' and op.vtType == 'delete':
                    module = pScene.modules[op.objectId]
                    module.setSelected(True)
        def old_move_test():
            return (old_action and
                    any(old_action.operations,
                        lambda x: x.what == 'location' and x.vtType == 'change'))
        def old_move():
            for op in old_action.operations:
                if op.what == 'location' and op.vtType == 'change':
                    module = pScene.modules[op.parentObjId]
                    module.setSelected(True)
        def new_addition_test():
            return (new_action and
                    any(new_action.operations,
                        lambda x: x.vtType == 'add'))
        def new_addition():
            # This action was an addition, select the thing that was
            # added in this version
            for op in new_action.operations:
                if op.what == 'module' and op.vtType == 'add':
                    module = pScene.modules[op.objectId]
                    module.setSelected(True)
        def new_move_test():
             return (new_action and
                     any(new_action.operations,
                         lambda x: x.what == 'location' and x.vtType == 'change'))
        def new_move():
            # some action was a move, select the things that were moved
            for op in new_action.operations:
                if op.what == 'location' and op.vtType == 'change':
                    module = pScene.modules[op.parentObjId]
                    module.setSelected(True)

        old_deletion_pair = (old_deletion_test, old_deletion)
        old_move_pair = (old_move_test, old_move)
        new_addition_pair = (new_addition_test, new_addition)
        new_move_pair = (new_move_test, new_move)

        dispatch = {'undo': [old_deletion_pair,
                             old_move_pair,
                             new_addition_pair,
                             new_move_pair],
                    'redo': [new_move_pair,
                             new_addition_pair,
                             old_move_pair,
                             old_deletion_pair]}
        assert optype in dispatch
        for (test, handler) in dispatch[optype]:
            if test():
                handler()
                return
        # If we get here, we couldn't recognize the action
        # FIXME: Add tests and handlers for change parameter
        

    def undo(self):
        """Performs one undo step, moving up the version tree."""
        action_map = self.controller.vistrail.actionMap
        old_action = action_map.get(self.controller.current_version, None)
        self.redo_stack.append(self.controller.current_version) 
        self.controller.show_parent_version()
        new_action = action_map.get(self.controller.current_version, None)
        self.set_pipeline_selection(old_action, new_action, 'undo')
        return self.controller.current_version
        
    def redo(self):
        """Performs one redo step if possible, moving down the version tree."""
        action_map = self.controller.vistrail.actionMap
        old_action = action_map.get(self.controller.current_version, None)
        if not self.can_redo():
            critical("Redo on an empty redo stack. Ignoring.")
            return
        next_version = self.redo_stack[-1]
        self.redo_stack = self.redo_stack[:-1]
        self.controller.show_child_version(next_version)
        new_action = action_map[self.controller.current_version]
        self.set_pipeline_selection(old_action, new_action, 'redo')
        return next_version

    def can_redo(self):
        return len(self.redo_stack) <> 0

    def new_action(self, action):
        """new_action

        Handler for VistrailController.new_action

        """
        self.redo_stack = []

################################################################################

# FIXME: There is a bug on VisTrails that shows up if you load terminator.vt,
# open the image slices HW, undo about 300 times and then try to redo.
# This should be a test here, as soon as we have an api for that.

if __name__=="__main__":
    # Initialize the Vistrails Application and Theme
    import sys
    from gui import qt, theme
    app = qt.createBogusQtGuiApp(sys.argv)
    theme.initializeCurrentTheme()

    # Now visually test QPipelineView
    vv = QVistrailView(None)
    vv.show()    
    sys.exit(app.exec_())
