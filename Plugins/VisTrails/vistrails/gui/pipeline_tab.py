
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
""" The file describes the pipeline tab widget to manage a single
pipeline

QPipelineTab
"""

from PyQt4 import QtCore, QtGui
from core.vistrail.module import Module
from core.vistrail.connection import Connection
from gui.common_widgets import QDockContainer, QToolWindowInterface
from gui.method_palette import QMethodPalette
from gui.module_methods import QModuleMethods
from gui.pipeline_view import QPipelineView
from weakref import proxy

################################################################################

class QPipelineTab(QDockContainer, QToolWindowInterface):
    """
    QPipelineTab is a tab widget setting QPipelineView in a center
    while having surrounding tool windows for manipulating a pipeline
    
    """
    def __init__(self, parent=None):
        """ QPipelineTab(parent: QWidget) -> QPipelineTab        
        Make it a main window with dockable area and a QPipelineView
        in the middle
        
        """
        QDockContainer.__init__(self, parent)
        self.setWindowTitle('Pipeline')
        self.pipelineView = QPipelineView()
        self.pipelineView.scene().pipeline_tab = proxy(self)
        self.setCentralWidget(self.pipelineView)
        self.toolWindow().setFeatures(QtGui.QDockWidget.NoDockWidgetFeatures)
        self.toolWindow().hide()        
        
        self.methodPalette = QMethodPalette(self)
        self.addDockWidget(QtCore.Qt.RightDockWidgetArea,
                           self.methodPalette.toolWindow())
        
        self.moduleMethods = QModuleMethods(self)
        self.addDockWidget(QtCore.Qt.RightDockWidgetArea,
                           self.moduleMethods.toolWindow())
        
        self.connect(self.toolWindow(),
                     QtCore.SIGNAL('topLevelChanged(bool)'),
                     self.updateWindowTitle)
        self.connect(self.pipelineView.scene(),
                     QtCore.SIGNAL('moduleSelected'),
                     self.moduleSelected)
        self.connect(self.pipelineView,
                     QtCore.SIGNAL('resetQuery()'),
                     self.resetQuery)

        self.controller = None

    def addViewActionsToMenu(self, menu):
        """addViewActionsToMenu(menu: QMenu) -> None
        Add toggle view actions to menu
        
        """
        menu.addAction(self.methodPalette.toolWindow().toggleViewAction())
        menu.addAction(self.moduleMethods.toolWindow().toggleViewAction())

    def removeViewActionsFromMenu(self, menu):
        """removeViewActionsFromMenu(menu: QMenu) -> None
        Remove toggle view actions from menu
        
        """
        menu.removeAction(self.methodPalette.toolWindow().toggleViewAction())
        menu.removeAction(self.moduleMethods.toolWindow().toggleViewAction())

    def updatePipeline(self, pipeline):
        """ updatePipeline(pipeline: Pipeline) -> None        
        Setup the pipeline to display and control a specific pipeline
        
        """
        self.pipelineView.scene().setupScene(pipeline)

    def updateWindowTitle(self, topLevel):
        """ updateWindowTitle(topLevel: bool) -> None
        Change the current widget title depends on the top level status
        
        """
        if topLevel:
            self.setWindowTitle('Pipeline <' +
                                self.toolWindow().parent().windowTitle()+'>')
        else:
            self.setWindowTitle('Pipeline')

    def moduleSelected(self, moduleId, selection = []):
        """ moduleChanged(moduleId: int, selection: [QGraphicsModuleItem])
                          -> None
        Set up the view when module selection has been changed
        
        """
        if self.pipelineView.scene().controller:
            pipeline = self.pipelineView.scene().controller.current_pipeline
        else:
            pipeline = None
        if pipeline and pipeline.modules.has_key(moduleId):
            module = pipeline.modules[moduleId]
            self.methodPalette.setEnabled(True)
            self.moduleMethods.setEnabled(True)
        else:
            module = None
            self.methodPalette.setEnabled(False)
            self.moduleMethods.setEnabled(False)
        self.methodPalette.setUpdatesEnabled(False)
        self.moduleMethods.setUpdatesEnabled(False)
        try:
            self.methodPalette.updateModule(module)
            self.moduleMethods.updateModule(module)
            self.emit(QtCore.SIGNAL('moduleSelectionChange'),
                      [m.id for m in selection])
        finally:
            self.methodPalette.setUpdatesEnabled(True)
            self.moduleMethods.setUpdatesEnabled(True)
             

    def setController(self, controller):
        """ setController(controller: VistrailController) -> None
        Assign a vistrail controller to this pipeline view
        
        """
        oldController = self.pipelineView.scene().controller
        if oldController!=controller:
            if oldController!=None:
                self.disconnect(oldController,
                                QtCore.SIGNAL('versionWasChanged'),
                                self.versionChanged)
                self.disconnect(oldController,
                                QtCore.SIGNAL('flushMoveActions()'),
                                self.flushMoveActions)
                oldController.current_pipeline_view = None
            self.controller = controller
            self.pipelineView.scene().controller = controller
            self.connect(controller,
                         QtCore.SIGNAL('versionWasChanged'),
                         self.versionChanged)
            self.connect(controller,
                         QtCore.SIGNAL('flushMoveActions()'),
                         self.flushMoveActions)
            self.methodPalette.controller = controller
            self.moduleMethods.controller = controller
            controller.current_pipeline_view = self.pipelineView.scene()

    def versionChanged(self, newVersion):
        """ versionChanged(newVersion: int) -> None        
        Update the pipeline when the new vistrail selected in the
        controller
        
        """
        self.updatePipeline(self.controller.current_pipeline)
            
    def flushMoveActions(self):
        """ flushMoveActions() -> None
        Update all move actions into vistrail
        
        """
        controller = self.pipelineView.scene().controller
        moves = []
        for (mId, item) in self.pipelineView.scene().modules.iteritems():
            module = controller.current_pipeline.modules[mId]
            (dx,dy) = (item.scenePos().x(), -item.scenePos().y())
            if (dx != module.center.x or dy != module.center.y):
                moves.append((mId, dx, dy))
        if len(moves)>0:
            controller.quiet = True
            controller.move_module_list(moves)
            controller.quiet = False

    def resetQuery(self):
        """ resetQuery() -> None
        pass along the signal

        """
        self.emit(QtCore.SIGNAL('resetQuery()'))
