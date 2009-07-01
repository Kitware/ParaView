
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
""" The file describes the query tab widget to apply a query/filter to
the current pipeline/version view

QQueryTab
"""

from PyQt4 import QtCore, QtGui
# FIXME broke this as Actions have been changed
#
# from core.vistrail.action import ChangeParameterAction, \
#      ChangeAnnotationAction, AddModulePortAction
from core.vistrail.action import Action
from core.vistrail.location import Location
from core.vistrail.pipeline import Pipeline
from core.vistrail.vistrail import Vistrail
import db.services.io
from core.modules import module_registry
from gui.method_dropbox import QMethodInputForm
from gui.pipeline_tab import QPipelineTab
from gui.theme import CurrentTheme
from gui.vistrail_controller import VistrailController
import copy

################################################################################

# FIXME: We need new infrastructure for QBE on user-defined types.

class QQueryTab(QPipelineTab):
    """
    QQuery is the similar to the pipeline tab where we can interact
    with pipeline. However, no modules properties is accessibled. Just
    connections. Then we can apply this pipeline to be a query on both
    version and pipeline view
    
    """
    def __init__(self, parent=None):
        """ QQueryTab(parent: QWidget) -> QQueryTab
        Create an empty vistrail controller for this query tab
        
        """
        QPipelineTab.__init__(self, parent)
        self.pipelineView.setBackgroundBrush(
            CurrentTheme.QUERY_BACKGROUND_BRUSH)

        self.moduleMethods.vWidget.formType = QFunctionQueryForm
        
        controller = QueryVistrailController(auto_save=False)
        controller.set_vistrail(Vistrail(), None)
        self.setController(controller)
        self.connect(controller,
                     QtCore.SIGNAL('vistrailChanged()'),
                     self.vistrailChanged)

    def vistrailChanged(self):
        """ vistrailChanged() -> None
        Update the pipeline when the vistrail version has changed
        
        """
        self.controller.current_pipeline.ensure_connection_specs()
        self.emit(QtCore.SIGNAL("queryPipelineChange"),
                  len(self.controller.current_pipeline.modules)>0)
        
################################################################################

class QFunctionQueryForm(QMethodInputForm):
    def __init__(self, parent=None):
        """ QFunctionQueryForm(parent: QWidget) -> QFunctionQueryForm
        Intialize the widget
        
        """
        QMethodInputForm.__init__(self, parent)
        self.function = None
        self.fields = []

    def updateFunction(self, function):
        """ updateFunction(function: ModuleFunction) -> None
        Auto create widgets to describes the function 'function'
        
        """
        self.function = function
        self.setTitle(function.name)
        gLayout = self.layout()
        for pIdx in xrange(len(function.params)):
            p = function.params[pIdx]
            field = QParameterQuery(p)
            self.fields.append(field)
            gLayout.addWidget(field, pIdx, 0)
        self.update()

    def updateMethod(self):
        """ updateMethod() -> None        
        Update the method values to vistrail. We only keep a monotonic
        version tree of the query pipeline, we can skip the actions
        here.
        
        """
        methodBox = self.parent().parent().parent()
        if methodBox.controller:
            paramList = []
            pipeline = methodBox.controller.current_pipeline
            f = pipeline.modules[methodBox.module.id].functions[self.fId]
            p = f.params
            for i in xrange(len(self.fields)):
                p[i].strValue = str(self.fields[i].editor.contents())
                p[i].queryMethod = self.fields[i].selector.getCurrentMethod()

        # Go upstream to update the pipeline
        qtab = self
        while type(qtab)!=QQueryTab and qtab!=None:
            qtab = qtab.parent()
        if qtab:
            qtab.vistrailChanged()
            
################################################################################

class QParameterQuery(QtGui.QWidget):
    """
    QParameterQuery is a widget containing a line edit and a drop down
    menu allowing users to choose how they want to query a parameter
    
    """
    def __init__(self, param, parent=None):
        """ QParameterQuery(param: ModuleParam) -> QParameterQuery
        Construct the widget layout
        
        """
        QtGui.QWidget.__init__(self, parent)
        self.value = param.strValue
        self.type = param.type
        
        layout = QtGui.QHBoxLayout()
        layout.setSpacing(0)
        layout.setMargin(0)
        self.setLayout(layout)
        
        self.label = QtGui.QLabel('')
        layout.addWidget(self.label)
        
        self.selector = QParameterQuerySelector(self.type)
        layout.addWidget(self.selector)        

        reg = module_registry.registry
        constant_class = reg.get_module_by_name(param.identifier,
                                                param.type,
                                                param.namespace)
        widget_type = constant_class.get_widget_class() 
        self.editor = widget_type(param)
        layout.addWidget(self.editor)

        self.connect(self.selector.operationActionGroup,
                     QtCore.SIGNAL('triggered(QAction*)'),
                     self.operationChanged)
        if self.type =='String':            
            self.connect(self.selector.caseActionGroup,
                         QtCore.SIGNAL('triggered(QAction*)'),
                         self.caseChanged)
        self.selector.initAction(param.queryMethod)

    def operationChanged(self, action):
        """ operationChanged(action: QAction) -> None
        Update the text to reflect the operation being used
        
        """
        self.label.setText(action.text())
        self.updateMethod()
        
    def caseChanged(self, action):
        """ caseChanged(action: QAction) -> None
        Update the text to reflect the case sensitivity being used
        
        """
        self.updateMethod()
        
    def updateMethod(self):
        """ updateMethod() -> None        
        Update the method values to vistrail. We only keep a monotonic
        version tree of the query pipeline, we can skip the actions
        here.
        
        """
        if self.parent():
            self.parent().updateMethod()
        
################################################################################

class QParameterQuerySelector(QtGui.QToolButton):
    """
    QParameterEditorSelector is a button with a down arrow allowing
    users to select which type of interpolator he/she wants to use
    
    """
    def __init__(self, pType, parent=None):
        """ QParameterEditorSelector(pType: str, parent: QWidget)
                                     -> QParameterEditorSelector
        Put a stacked widget and a popup button
        
        """
        QtGui.QToolButton.__init__(self, parent)
        self.type = pType
        self.setAutoRaise(True)
        self.setToolButtonStyle(QtCore.Qt.ToolButtonTextOnly)
        menu = QtGui.QMenu(self)

        self.setPopupMode(QtGui.QToolButton.InstantPopup)        
        
        if pType=='String':
            self.setText(QtCore.QString(QtCore.QChar(0x25bc))) # Down triangle
            self.operationActionGroup = QtGui.QActionGroup(self)                    
            self.containAction = self.operationActionGroup.addAction('Contain')
            self.containAction.setCheckable(True)

            self.exactAction = self.operationActionGroup.addAction('Exact')
            self.exactAction.setCheckable(True)
            
            self.regAction = self.operationActionGroup.addAction('Reg Exp')
            self.regAction.setCheckable(True)
            menu.addActions(self.operationActionGroup.actions())
            menu.addSeparator()
            
            self.caseActionGroup = QtGui.QActionGroup(self)
            self.sensitiveAction = self.caseActionGroup.addAction(
                'Case Sensitive')
            self.sensitiveAction.setCheckable(True)
            
            self.insensitiveAction = self.caseActionGroup.addAction(
                'Case Insensitive')
            self.insensitiveAction.setCheckable(True)
            menu.addActions(self.caseActionGroup.actions())
            
        else:
            self.setText('') # Down triangle
            self.operationActionGroup = QtGui.QActionGroup(self)                    
            self.expAction = self.operationActionGroup.addAction('Expression')
            self.expAction.setCheckable(True)
            self.setEnabled(False)
            
        self.setMenu(menu)

    def initAction(self, pMethod):
        """ initAction(pMethod: int) -> None
        Select the first choice of selector based on self.type
        
        """
        if self.type=='String':
            opMap = {0: self.containAction,
                     1: self.exactAction,
                     2: self.regAction}
            caseMap = {0: self.insensitiveAction,
                       1: self.sensitiveAction}
            if pMethod>5:
                pMethod = 0
            opMap[pMethod/2].trigger()
            caseMap[pMethod%2].trigger()
        else:
            self.expAction.trigger()

    def getCurrentMethod(self):
        """ getCurrentMethod() -> int
        Get the current method based on the popup selection

        """
        if self.type=='String':
            opMap = {self.containAction: 0,
                     self.exactAction: 1,
                     self.regAction: 2}
            caseMap = {self.insensitiveAction: 0,
                       self.sensitiveAction: 1}
            method = (opMap[self.operationActionGroup.checkedAction()]*2 +
                      caseMap[self.caseActionGroup.checkedAction()])
            return method
        else:
            return 0

################################################################################

class QueryVistrailController(VistrailController):
    """ QueryVistrailController derives from VistrailController to
    include the copy and paste of query methods
    
    """
    
#     def pasteModulesAndConnections(self, str):
#         """ pasteModulesAndConnections(modules: [Module],
#                                        connections: [Connection]) -> version id
#         Paste a list of modules and connections into the current
#         pipeline. Also paste the queryMethod attribute

#         """
#         pipeline = db.services.io.getWorkflowFromXML(str)
#         Pipeline.convert(pipeline)

#         ops = []
#         module_remap = {}
#         for module in pipeline.modules.itervalues():
#             module = copy.copy(module)
#             old_id = module.id
#             if module.location is not None:
#                 loc_id = self.vistrail.idScope.getNewId(Location.vtType)
#                 module.location = Location(id=loc_id,
#                                            x=module.location.x + 10.0,
#                                            y=module.location.y + 10.0,
#                                            )
#             mops = db.services.action.create_copy_op_chain(object=module,
#                                                id_scope=self.vistrail.idScope)
#             module_remap[old_id] = mops[0].db_objectId
#             ops.extend(mops)
#         for connection in pipeline.connections.itervalues():
#             connection = copy.copy(connection)
#             for port in connection.ports:
#                 port.moduleId = module_remap[port.moduleId]
#             ops.extend( \
#                 db.services.action.create_copy_op_chain(object=connection,
#                                                id_scope=self.vistrail.idScope))
#         action = db.services.action.create_action_from_ops(ops)
#         Action.convert(action)

#         self.perform_action(action)

#         self.quiet = True
#         modulesMap ={}
#         modulesToSelect = []
#         actions = []
#         self.previousModuleIds = []
#         for module in modules:
#             name = module.name
#             x = module.center.x + 10.0
#             y = module.center.y + 10.0
#             t = self.addModule(name,x,y)
#             newId = self.vistrail.actionMap[self.currentVersion].module.id
#             self.previousModuleIds.append(newId)
#             modulesMap[module.id]=newId
#             modulesToSelect.append(newId)
#             for fi in range(len(module.functions)):
#                 f = module.functions[fi]
#                 action = ChangeParameterAction()
#                 if f.getNumParams() == 0:
#                     action.addParameter(newId, fi, -1, f.name, "",
#                                         "","", "")
#                 queryMethods = []
#                 for i in range(f.getNumParams()):
#                     p = f.params[i]
#                     if self.currentPipeline.hasAlias(p.alias):
#                         p.alias = ""
#                     action.addParameter(newId, fi, i, f.name, p.name,
#                                         p.strValue, p.type, p.alias)
#                     queryMethods.append(p.queryMethod)
#                 self.performAction(action)
#                 newF = self.currentPipeline.modules[newId].functions[fi]
#                 for i in range(f.getNumParams()):
#                     newF.params[i].queryMethod = queryMethods[i]
#             for (key,value) in module.annotations.items():
#                 action = ChangeAnnotationAction()
#                 action.addAnnotation(newId,key,value)
#                 actions.append(action)
#             if module.registry:
#                 desc = module.registry.getDescriptorByName(module.name)
#                 for (name, spec) in desc.inputPorts.iteritems():
#                     names = [module_registry.registry.getDescriptor(p[0]).name
#                              for p in spec[0]]                    
#                     action = AddModulePortAction()
#                     action.addModulePort(newId, 'input', name, '('+','.join(names)+')')
#                     actions.append(action)
#                 for (name, spec) in desc.outputPorts.iteritems():
#                     names = [module_registry.registry.getDescriptor(p[0]).name
#                              for p in spec[0]]
#                     action = AddModulePortAction()
#                     action.addModulePort(newId, 'output', name, '('+','.join(names)+')')
#                     actions.append(action)

#         currentAction = self.performBulkActions(actions)
        
#         for c in connections:
#             conn = copy.copy(c)
#             conn.id = self.currentPipeline.fresh_connection_id()
#             conn.sourceId = modulesMap[conn.sourceId]
#             conn.destinationId = modulesMap[conn.destinationId]
#             currentAction = self.addConnection(conn)            
#         self.quiet = False

#         self.currentVersion = currentAction
#         self.invalidate_version_tree()

#     def setPipeline(self, pipeline):
#         """ setPipeline(pipeline) -> None
#         Replace the current pipeline with the given pipeline
        
#         """
#         # First remove all modules
#         self.deleteModuleList(copy.copy(self.currentPipeline.modules.keys()))

#         # Then paste this new modules and connections
#         self.pasteModulesAndConnections(pipeline.modules.values(),
#                                         pipeline.connections.values())
