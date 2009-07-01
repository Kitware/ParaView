
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
""" This holds a customized toolbar class for QVistrailView

QVistrailViewPaddedTabBar
QVistrailViewTabBar
QVistrailViewToolBar
QVistrailInteractionToolBar
"""

from PyQt4 import QtCore, QtGui
from gui.theme import CurrentTheme

################################################################################

class QVistrailViewToolBar(QtGui.QToolBar):
    """
    QVistrailViewToolBar is a special toolbar for vistrail view. It
    has a number of tool buttons, e.g. Execute, View Mode...., on the
    left and a tab bar on the right. The tab bar for now contains two
    tab to switch back and forth between pipeline view and version
    tree view in the tabbed mode
    
    """
    def __init__(self, view=None):
        """ QVistrailViewToolBar(view: QVistrailView) -> QVistrailViewToolBar
        Construct default buttons and tabbar for the toolbar
        """
        QtGui.QToolBar.__init__(self, view)
        self.setWindowTitle('Vistrail Controller')        
        self.setToolButtonStyle(QtCore.Qt.ToolButtonTextUnderIcon)

        self.addAction(self.executeAction())
        self.addSeparator()
        self.addAction(self.pipelineViewAction())
        self.addAction(self.historyViewAction())
        self.addAction(self.queryViewAction())
        self.addAction(self.exploreViewAction())
        
        self.connect(self.executeAction(),
                     QtCore.SIGNAL('triggered(bool)'),
                     self.execute)
        self.connect(self.parent(),
                     QtCore.SIGNAL('changeViewState(int)'),
                     self.changeViewState)
        self.connect(self.parent(),
                     QtCore.SIGNAL('executeEnabledChanged(bool)'),
                     self.executeEnabledChanged)

        self.currentViewIndex = 0

    def executeAction(self):
        """ executeAction() -> QAction        
        Returns the execute pipeline action that can be used in any
        menu or toolbar
        
        """
        if not hasattr(self, '_executeAction'):
            self._executeAction = QtGui.QAction(
                CurrentTheme.EXECUTE_PIPELINE_ICON,
                '&Execute',
                self)
            self._executeAction.setToolTip('Execute the current '
                                                   'pipeline')
            self._executeAction.setStatusTip(
                self._executeAction.toolTip())
            self._executeAction.setEnabled(False)
        return self._executeAction

    def viewActionGroup(self):
        """ viewActionGroup() -> None        
        A group for all view actions in order to have only one
        selected at a time
        
        """
        if not hasattr(self, '_viewActionGroup'):
            self._viewActionGroup = QtGui.QActionGroup(self)
        return self._viewActionGroup

    def pipelineViewAction(self):
        """ pipelineViewAction() -> QAction
         Returns the action for using the pipeline view
        
        """
        if not hasattr(self, '_pipelineViewAction'):
            self._pipelineViewAction = QtGui.QAction(
                CurrentTheme.PIPELINE_ICON,
                'Pipeline',
                self.viewActionGroup())
            self._pipelineViewAction.setCheckable(True)
            self._pipelineViewAction.setToolTip('Edit the pipeline')
            self._pipelineViewAction.setStatusTip(
                self._pipelineViewAction.toolTip())
            self.connect(self._pipelineViewAction,
                         QtCore.SIGNAL('triggered(bool)'),
                         self.viewChanged)
        return self._pipelineViewAction

    def historyViewAction(self):
        """ historyViewAction() -> QAction
         Returns the action for using the history view
        
        """
        if not hasattr(self, '_historyViewAction'):
            self._historyViewAction = QtGui.QAction(
                CurrentTheme.HISTORY_ICON,
                'History',
                self.viewActionGroup())
            self._historyViewAction.setCheckable(True)
            self._historyViewAction.setToolTip('View the history')
            self._historyViewAction.setStatusTip(
                self._historyViewAction.toolTip())
            self.connect(self._historyViewAction,
                         QtCore.SIGNAL('triggered(bool)'),
                         self.viewChanged)
        return self._historyViewAction

    def queryViewAction(self):
        """ queryViewAction() -> QAction
         Returns the action for using the query view
        
        """
        if not hasattr(self, '_queryViewAction'):
            self._queryViewAction = QtGui.QAction(
                CurrentTheme.QUERY_ICON,
                'Query',
                self.viewActionGroup())
            self._queryViewAction.setCheckable(True)
            self._queryViewAction.setToolTip('Edit a visual query')
            self._queryViewAction.setStatusTip(
                self._queryViewAction.toolTip())
            self.connect(self._queryViewAction,
                         QtCore.SIGNAL('triggered(bool)'),
                         self.viewChanged)
        return self._queryViewAction

    def exploreViewAction(self):
        """ exploreViewAction() -> QAction
         Returns the action for using the explore view
        
        """
        if not hasattr(self, '_exploreViewAction'):
            self._exploreViewAction = QtGui.QAction(
                CurrentTheme.EXPLORE_ICON,
                'Exploration',
                self.viewActionGroup())
            self._exploreViewAction.setCheckable(True)
            self._exploreViewAction.setToolTip('Edit a parameter exploration')
            self._exploreViewAction.setStatusTip(
                self._exploreViewAction.toolTip())
            self.connect(self._exploreViewAction,
                         QtCore.SIGNAL('triggered(bool)'),
                         self.viewChanged)
        return self._exploreViewAction

    def changeViewState(self, viewIndex):
        """ changeViewState(viewIndex: int) -> None
        Change the active view action
        
        """
        if viewIndex == 0:
            self.pipelineViewAction().setChecked(True)
        elif viewIndex == 1:
            self.historyViewAction().setChecked(True)
        elif viewIndex == 2:
            self.queryViewAction().setChecked(True)
        else:
            self.exploreViewAction().setChecked(True)
        self.currentViewIndex = viewIndex
        self.changeExecuteButtonState(self.currentViewIndex)

    def viewChanged(self, checked=True):
        """ viewChanged(checked: bool) -> None
        The view has changed, emit a signal

        """
        if self.pipelineViewAction().isChecked():
            self.currentViewIndex = 0
        elif self.historyViewAction().isChecked():
            self.currentViewIndex = 1
        elif self.queryViewAction().isChecked():
            self.currentViewIndex = 2
        else:
            self.currentViewIndex = 3

        self.changeExecuteButtonState(self.currentViewIndex)
        self.parent().viewModeChanged(self.currentViewIndex)

    def changeExecuteButtonState(self, index):
        """ changeExecuteButtonState(index: int) -> None
        Change icon, tooltip, and enabled state for different view modes

        """
        if index == 3: # parameter explore
            self.executeAction().setIcon(
                CurrentTheme.EXECUTE_EXPLORE_ICON)
            self.executeAction().setToolTip('Execute the parameter'
                                            'exploration')
        elif index == 2: # query
            self.executeAction().setIcon(
                CurrentTheme.VISUAL_QUERY_ICON)
            self.executeAction().setToolTip('Execute a visual query')
        else:
            self.executeAction().setIcon(
                CurrentTheme.EXECUTE_PIPELINE_ICON)
            self.executeAction().setToolTip('Execute the current '
                                            'pipeline')

    def execute(self):
        """ execute() -> None
        Execute action was triggered

        """
        self.parent().execute(self.currentViewIndex)

    def executeEnabledChanged(self, on):
        """ executeEnabledChanged(on: bool) -> None

        """
        self.executeAction().setEnabled(on)


################################################################################

class QVistrailInteractionToolBar(QtGui.QToolBar):
    """
    QVistrailInteractionToolBar is a special toolbar for interaction. It
    contains buttons for select, pan, and zoom.
    
    """
    def __init__(self, view=None):
        """ QVistrailInteractionToolBar(view: QVistrailView) -> QVistrailInteractionToolBar
        Construct default buttons and tabbar for the toolbar
        """
        QtGui.QToolBar.__init__(self, view)
        self.setToolButtonStyle(QtCore.Qt.ToolButtonTextUnderIcon)

        self.addAction(self.selectCursorAction())
        self.addAction(self.panCursorAction())
        self.addAction(self.zoomCursorAction())

    def cursorMenu(self):
        """ cursorMenu() -> None        
        A menu of different cursor shapes
        
        """
        if not hasattr(self, '_cursorMenu'):
            menu = QtGui.QMenu("Select an action for the cursor", self)
            menu.addAction(self.selectCursorAction())
            menu.addAction(self.panCursorAction())
            menu.addAction(self.zoomCursorAction())
            checkedAction = self.cursorActionGroup().checkedAction()
            self._cursorMenu = menu
            self.assignCursorMenuAction(checkedAction)
        return self._cursorMenu

    def assignCursorMenuAction(self, action):
        """ assignCursorMenuAction(action: QAction) -> None
        Assign one of the three cursor actions to be the default one
        
        """
        menu = self.cursorMenu()
        menu.setIcon(action.icon())
        menu.setTitle(action.toolTip())
        menu.menuAction().setStatusTip(action.statusTip())                

    def cursorActionGroup(self):
        """ cursorActionGroup() -> None        
        A group for all cursor actions in order to have only one
        selected at a time
        
        """
        if not hasattr(self, '_cursorActionGroup'):
            self._cursorActionGroup = QtGui.QActionGroup(self)
        return self._cursorActionGroup

    def selectCursorAction(self):
        """ selectCursorAction() -> QAction        
        Returns the action for using the cursor as a selection tool by
        default (an arrow shape)
        
        """
        if not hasattr(self, '_selectCursorAction'):
            self._selectCursorAction = QtGui.QAction(
                CurrentTheme.SELECT_ICON,
                'Select',
                self.cursorActionGroup())
            self._selectCursorAction.setCheckable(True)
            self._selectCursorAction.setToolTip('Selecting components '
                                                'inside the view ')
            self._selectCursorAction.setStatusTip(
                self._selectCursorAction.toolTip())
            self._selectCursorAction.setChecked(True)
            self.connect(self._selectCursorAction,
                         QtCore.SIGNAL('triggered(bool)'),
                         self.cursorToggled)
        return self._selectCursorAction

    def panCursorAction(self):
        """ panCursorAction() -> QAction        
        Returns the action for using the cursor as a panning tool
        
        """
        if not hasattr(self, '_panCursorAction'):
            self._panCursorAction = QtGui.QAction(
                CurrentTheme.PAN_ICON,
                'Pan',
                self.cursorActionGroup())
            self._panCursorAction.setCheckable(True)
            self._panCursorAction.setToolTip('Pan the view (Shift+Click)')
            self._panCursorAction.setStatusTip(
                self._panCursorAction.toolTip())
            self.connect(self._panCursorAction,
                         QtCore.SIGNAL('triggered(bool)'),
                         self.cursorToggled)
        return self._panCursorAction

    def zoomCursorAction(self):
        """ zoomCursorAction() -> QAction        
        Returns the action for using the cursor as a zooming tool
        
        """
        if not hasattr(self, '_zoomCursorAction'):
            self._zoomCursorAction = QtGui.QAction(
                CurrentTheme.ZOOM_ICON,
                'Zoom',
                self.cursorActionGroup())
            self._zoomCursorAction.setCheckable(True)
            self._zoomCursorAction.setToolTip('Zoom in/out the view '
                                              '(Meta[Shift+Alt]+Click)')
            self._zoomCursorAction.setStatusTip(
                self._zoomCursorAction.toolTip())
            self.connect(self._zoomCursorAction,
                         QtCore.SIGNAL('triggered(bool)'),
                         self.cursorToggled)
        return self._zoomCursorAction

    def cursorToggled(self, checked=True):
        """ cursorToggled(checked: bool) -> None        
        Slot to handle cursor actions toggled. This will in turn emit
        another signal to specify cursor has been selected
        
        """
        cursorMode = -1
        action = None
        if self.selectCursorAction().isChecked():
            cursorMode = 0
            action = self.selectCursorAction()
        elif self.panCursorAction().isChecked():
            cursorMode = 1
            action = self.panCursorAction()
        elif self.zoomCursorAction().isChecked():
            cursorMode = 2
            action = self.zoomCursorAction()
        if action:
            self.assignCursorMenuAction(action)
        self.parent().viewManager.changeCursor(cursorMode)

