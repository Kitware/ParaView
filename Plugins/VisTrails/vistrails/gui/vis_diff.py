
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
""" This modules builds a widget to interact with vistrail diff
operation """
from PyQt4 import QtCore, QtGui
from core.utils.color import ColorByName
from core.vistrail.abstraction_module import AbstractionModule
from core.vistrail.pipeline import Pipeline
from gui.pipeline_view import QPipelineView
from gui.theme import CurrentTheme
from gui.vistrail_controller import VistrailController
from core import system
import core.db.io
import copy

################################################################################

class QFunctionItemModel(QtGui.QStandardItemModel):
    """
    QFunctionItemModel is a item model that will allow item to be
    show as a disabled one on the table
    
    """
    def __init__(self, row, col, parent=None):
        """ QFunctionItemModel(row: int, col: int, parent: QWidget)
                              -> QFunctionItemModel                             
        Initialize with a number of rows and columns
        
        """
        QtGui.QStandardItemModel.__init__(self, row, col, parent)
        self.disabledRows = {}

    def flags(self, index):
        """ flags(index: QModelIndex) -> None
        Return the current flags of the item with the index 'index'
        
        """
        if index.isValid() and self.disabledRows.has_key(index.row()):
            return (QtCore.Qt.ItemIsDragEnabled | QtCore.Qt.ItemIsDropEnabled |
                    QtCore.Qt.ItemIsSelectable)
        return QtGui.QStandardItemModel.flags(self,index)

    def clearList(self):
        """ clearList() -> None
        Clear all items including the disabled ones
        
        """
        self.disabledRows = {}
        self.removeRows(0,self.rowCount())

    def disableRow(self,row):
        """ disableRow(row: int) -> None
        Disable a specific row on the table
        
        """
        self.disabledRows[row] = None

class QParamTable(QtGui.QTableView):
    """
    QParamTable is a widget represents a diff between two version
    as side-by-side comparisons
    
    """
    def __init__(self, v1Name=None, v2Name=None, parent=None):
        """ QParamTable(v1Name: str, v2Name: str, parent: QWidget)
                       -> QParamTable
        Initialize the table with two version names on the header view
        
        """
        QtGui.QTableView.__init__(self, parent)
        itemModel = QFunctionItemModel(0, 2, self)
        itemModel.setHeaderData(0, QtCore.Qt.Horizontal,
                                QtCore.QVariant(v1Name))
        itemModel.setHeaderData(1, QtCore.Qt.Horizontal,
                                QtCore.QVariant(v2Name))
        self.setModel(itemModel)
        self.setSelectionBehavior(QtGui.QAbstractItemView.SelectRows)        
        self.setSelectionMode(QtGui.QAbstractItemView.SingleSelection)        
        self.setEditTriggers(QtGui.QAbstractItemView.NoEditTriggers)        
        self.setFont(CurrentTheme.VISUAL_DIFF_PARAMETER_FONT)
        self.horizontalHeader().setResizeMode(QtGui.QHeaderView.Stretch)
        

class QParamInspector(QtGui.QWidget):
    """
    QParamInspector is a widget acting as an inspector vistrail modules
    in diff mode. It consists of a function inspector and annotation
    inspector
    
    """
    def __init__(self, v1Name='', v2Name='',
                 parent=None, f=QtCore.Qt.WindowFlags()):
        """ QParamInspector(v1Name: str, v2Name: str,
                            parent: QWidget, f: WindowFlags)
                            -> QParamInspector
        Construct a widget containing tabs: Functions and Annotations,
        each of them is in turn a table of two columns for two
        corresponding versions.
        
        """
        QtGui.QWidget.__init__(self, parent, f | QtCore.Qt.Tool)
        self.setWindowTitle('Parameter Inspector - None')
        self.firstTime = True        
        self.boxLayout = QtGui.QVBoxLayout()
        self.boxLayout.setMargin(0)
        self.boxLayout.setSpacing(0)
        self.tabWidget = QtGui.QTabWidget()
        self.tabWidget.setTabPosition(QtGui.QTabWidget.North)
        self.tabWidget.setTabShape(QtGui.QTabWidget.Triangular)
        self.functionsTab = QParamTable(v1Name, v2Name)
        self.tabWidget.addTab(self.functionsTab, 'Functions')        
# FIXME add annotation support back in
#         self.annotationsTab = QParamTable(v1Name, v2Name)
#         self.annotationsTab.horizontalHeader().setStretchLastSection(True)
#         self.tabWidget.addTab(self.annotationsTab, 'Annotations')        
        self.boxLayout.addWidget(self.tabWidget)
        self.boxLayout.addWidget(QtGui.QSizeGrip(self))
        self.setLayout(self.boxLayout)

    def closeEvent(self, e):
        """ closeEvent(e: QCloseEvent) -> None        
        Doesn't allow the QParamInspector widget to close, but just hide
        instead
        
        """
        e.ignore()
        self.parent().showInspectorAction.setChecked(False)
        

class QLegendBox(QtGui.QFrame):
    """
    QLegendBox is just a rectangular box with a solid color
    
    """
    def __init__(self, brush, size, parent=None, f=QtCore.Qt.WindowFlags()):
        """ QLegendBox(color: QBrush, size: (int,int), parent: QWidget,
                      f: WindowFlags) -> QLegendBox
        Initialize the widget with a color and fixed size
        
        """
        QtGui.QFrame.__init__(self, parent, f)
        self.setFrameStyle(QtGui.QFrame.Box | QtGui.QFrame.Plain)
        self.setAttribute(QtCore.Qt.WA_PaintOnScreen)
        self.setAutoFillBackground(True)
        self.palette().setBrush(QtGui.QPalette.Window, brush)
        self.setFixedSize(*size)
        if system.systemType in ['Darwin']:
            #the mac's nice looking mess up with the colors
            self.setAttribute(QtCore.Qt.WA_MacMetalStyle, False)
        
class QLegendWindow(QtGui.QWidget):
    """
    QLegendWindow contains a list of QLegendBox and its description
    
    """
    def __init__(self, v1Name='', v2Name= None, parent='',
                 f=QtCore.Qt.WindowFlags()):
        """ QLegendWindow(v1Name: str, v2Name: str,
                          parent: QWidget, f: WindowFlags)
                          -> QLegendWindow
        Construct a window by default with 4 QLegendBox and 4 QLabels
        
        """
        QtGui.QWidget.__init__(self, parent,
                               f | QtCore.Qt.Tool )
        self.setWindowTitle('Visual Diff Legend')
        self.firstTime = True
        self.gridLayout = QtGui.QGridLayout(self)
        self.gridLayout.setMargin(10)
        self.gridLayout.setSpacing(10)
        self.setFont(CurrentTheme.VISUAL_DIFF_LEGEND_FONT)
        
        parent = self.parent()
        
        self.legendV1Box = QLegendBox(
            CurrentTheme.VISUAL_DIFF_FROM_VERSION_BRUSH,
            CurrentTheme.VISUAL_DIFF_LEGEND_SIZE,
            self)        
        self.gridLayout.addWidget(self.legendV1Box, 0, 0)
        self.legendV1 = QtGui.QLabel("Version '" + v1Name + "'", self)
        self.gridLayout.addWidget(self.legendV1, 0, 1)
        
        self.legendV2Box = QLegendBox(
            CurrentTheme.VISUAL_DIFF_TO_VERSION_BRUSH,            
            CurrentTheme.VISUAL_DIFF_LEGEND_SIZE,
            self)        
        self.gridLayout.addWidget(self.legendV2Box, 1, 0)
        self.legendV2 = QtGui.QLabel("Version '" + v2Name + "'", self)
        self.gridLayout.addWidget(self.legendV2, 1, 1)
        
        self.legendV12Box = QLegendBox(CurrentTheme.VISUAL_DIFF_SHARED_BRUSH,
                                       CurrentTheme.VISUAL_DIFF_LEGEND_SIZE,
                                       self)
        self.gridLayout.addWidget(self.legendV12Box, 2, 0)
        self.legendV12 = QtGui.QLabel("Shared", self)
        self.gridLayout.addWidget(self.legendV12, 2, 1)
        
        self.legendParamBox = QLegendBox(
            CurrentTheme.VISUAL_DIFF_PARAMETER_CHANGED_BRUSH,
            CurrentTheme.VISUAL_DIFF_LEGEND_SIZE,
            self)
        self.gridLayout.addWidget(self.legendParamBox,3,0)
        self.legendParam = QtGui.QLabel("Parameter Changes", self)
        self.gridLayout.addWidget(self.legendParam,3,1)
        self.adjustSize()
        
    def closeEvent(self,e):
        """ closeEvent(e: QCloseEvent) -> None
        Doesn't allow the Legend widget to close, but just hide
        instead
        
        """
        e.ignore()
        self.parent().showLegendsAction.setChecked(False)
        

class QVisualDiff(QtGui.QMainWindow):
    """
    QVisualDiff is a main widget for Visual Diff containing a GL
    Widget to draw the pipeline
    
    """
    def __init__(self, vistrail, v1, v2,
                 controller,
                 parent=None, f=QtCore.Qt.WindowFlags()):
        """ QVisualDiff(vistrail: Vistrail, v1: str, v2: str,
                        parent: QWidget, f: WindowFlags) -> QVisualDiff
        Initialize with all
        
        """
        # Set up the version name correctly
        v1Name = vistrail.getVersionName(v1)
        if not v1Name: v1Name = 'Version %d' % v1
        v2Name = vistrail.getVersionName(v2)        
        if not v2Name: v2Name = 'Version %d' % v2
        
        # Actually perform the diff and store its result
        self.diff = vistrail.getPipelineDiff(v1, v2)

        self.v1_name = v1Name
        self.v2_name = v2Name
        self.v1 = v1
        self.v2 = v2
        self.controller = controller

        # Create the top-level Visual Diff window
        windowDecors = f | QtCore.Qt.Dialog |QtCore.Qt.WindowMaximizeButtonHint
        QtGui.QMainWindow.__init__(self, parent)
        self.setWindowTitle('Visual Diff - from %s to %s' % (v1Name, v2Name))
        self.setMouseTracking(True)
        self.setFocusPolicy(QtCore.Qt.StrongFocus)
        self.setSizePolicy(QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding,
                                             QtGui.QSizePolicy.Expanding))
        self.createPipelineView()
        self.createToolBar()
        self.createToolWindows(v1Name, v2Name)

    def createPipelineView(self):
        """ createPipelineView() -> None        
        Create a center pipeline view for Visual Diff and setup the
        view based on the diff result self.diff
        
        """
        # Initialize the shape engine
        self.pipelineView = QPipelineView()
        self.setCentralWidget(self.pipelineView)

        # Add all the shapes into the view
        self.createDiffPipeline()

        # Hook shape selecting functions
        self.connect(self.pipelineView.scene(), QtCore.SIGNAL("moduleSelected"),
                     self.moduleSelected)

    def createToolBar(self):
        """ createToolBar() -> None        
        Create the default toolbar of Visual Diff window with two
        buttons to toggle the Parameter Inspector and Legend window
        
        """
        # Initialize the Visual Diff toolbar
        self.toolBar = self.addToolBar('Visual Diff Toolbar')
        self.toolBar.setMovable(False)

        # Add the Show Parameter Inspector action
        self.showInspectorAction = self.toolBar.addAction(
            CurrentTheme.VISUAL_DIFF_SHOW_PARAM_ICON,
            'Show Parameter Inspector window')
        self.showInspectorAction.setCheckable(True)
        self.connect(self.showInspectorAction, QtCore.SIGNAL("toggled(bool)"),
                     self.toggleShowInspector)
        
        # Add the Show Legend window action
        self.showLegendsAction = self.toolBar.addAction(
            CurrentTheme.VISUAL_DIFF_SHOW_LEGEND_ICON,
            'Show Legends')
        self.showLegendsAction.setCheckable(True)
        self.connect(self.showLegendsAction, QtCore.SIGNAL("toggled(bool)"),
                     self.toggleShowLegend)

        # Add the create analogy action
        self.createAnalogyAction = self.toolBar.addAction(
            CurrentTheme.VISUAL_DIFF_CREATE_ANALOGY_ICON,
            'Create analogy')
        self.connect(self.createAnalogyAction, QtCore.SIGNAL("triggered()"),
                     self.createAnalogy)

    def createAnalogy(self):
        default = 'from %s to %s' % (self.v1_name, self.v2_name)
        (result, ok) = QtGui.QInputDialog.getText(None, "Enter Analogy Name",
                                                  "Name of analogy:",
                                                  QtGui.QLineEdit.Normal,
                                                  default)
        if not ok:
            return
        result = str(result)
        try:
            self.controller.add_analogy(result, self.v1, self.v2)
        except:
            QtGui.QMessageBox.warning(self,
                                      QtCore.QString("Error"),
                                      QtCore.QString("Analogy name already exists"),
                                      QtGui.QMessageBox.Ok,
                                      QtGui.QMessageBox.NoButton,
                                      QtGui.QMessageBox.NoButton)
        
    def createToolWindows(self, v1Name, v2Name):
        """ createToolWindows(v1Name: str, v2Name: str) -> None
        Create Inspector and Legend window

        """
        self.inspector = QParamInspector(v1Name, v2Name, self)
        self.inspector.resize(QtCore.QSize(
            *CurrentTheme.VISUAL_DIFF_PARAMETER_WINDOW_SIZE))
        self.legendWindow = QLegendWindow(v1Name, v2Name,self)

    def moduleSelected(self, id, selectedItems):
        """ moduleSelected(id: int, selectedItems: [QGraphicsItem]) -> None
        When the user click on a module, display its parameter changes
        in the Inspector
        
        """
        if len(selectedItems)!=1 or id==-1:
            self.moduleUnselected()
            return
        
        # Interprete the diff result and setup item models
        (p1, p2, v1Andv2, v1Only, v2Only, paramChanged) = self.diff

        # Set the window title
        if id>self.maxId1:
            self.inspector.setWindowTitle('Parameter Changes - %s' %
                                          p2.modules[id-self.maxId1-1].name)
        else:
            self.inspector.setWindowTitle('Parameter Changes - %s' %
                                          p1.modules[id].name)
            
        # Clear the old inspector
        functions = self.inspector.functionsTab.model()
#         annotations = self.inspector.annotationsTab.model()
        functions.clearList()
#         annotations.clearList()

        # Find the parameter changed module
        matching = None
        for ((m1id, m2id), paramMatching) in paramChanged:
            if m1id==id:
                matching = paramMatching
                break

        # If the module has no parameter changed, just display nothing
        if not matching:          
            return
        
        # Else just layout the diff on a table
        functions.insertRows(0,len(matching))
        currentRow = 0
        for (f1, f2) in matching:
            if f1[0]!=None:
                functions.setData(
                    functions.index(currentRow, 0),
                    QtCore.QVariant('%s(%s)' % (f1[0],
                                                ','.join(v[1] for v in f1[1]))))
            if f2[0]!=None:
                functions.setData(
                    functions.index(currentRow, 1),
                    QtCore.QVariant('%s(%s)' % (f2[0],
                                                ','.join(v[1] for v in f2[1]))))
            if f1==f2:                
                functions.disableRow(currentRow)
            currentRow += 1

        self.inspector.functionsTab.resizeRowsToContents()
#         self.inspector.annotationsTab.resizeRowsToContents()

    def moduleUnselected(self):
        """ moduleUnselected() -> None
        When a user selects nothing, make sure to display nothing as well
        
        """
#         self.inspector.annotationsTab.model().clearList()
        self.inspector.functionsTab.model().clearList()
        self.inspector.setWindowTitle('Parameter Changes - None')

    def toggleShowInspector(self):
        """ toggleShowInspector() -> None
        Show/Hide the inspector when toggle this button
        
        """
        if self.inspector.firstTime:
            max_geom = QtGui.QApplication.desktop().screenGeometry(self)
            if (self.frameSize().width() <
                max_geom.width() - self.inspector.width()):
                self.inspector.move(self.pos().x()+self.frameSize().width(),
                                    self.pos().y())
            else:
                self.inspector.move(self.pos().x()+self.frameSize().width()-
                                   self.inspector.frameSize().width(),
                                   self.pos().y() +
                                    self.legendWindow.frameSize().height())
            self.inspector.firstTime = False
        self.inspector.setVisible(self.showInspectorAction.isChecked())
            
    def toggleShowLegend(self):
        """ toggleShowLegend() -> None
        Show/Hide the legend window when toggle this button
        
        """
        if self.legendWindow.firstTime:
            self.legendWindow.move(self.pos().x()+self.frameSize().width()-
                                   self.legendWindow.frameSize().width(),
                                   self.pos().y())
        self.legendWindow.setVisible(self.showLegendsAction.isChecked())
        if self.legendWindow.firstTime:
            self.legendWindow.firstTime = False
            self.legendWindow.setFixedSize(self.legendWindow.size())            
                
    def createDiffPipeline(self):
        """ createDiffPipeline() -> None        
        Actually walk through the self.diff result and add all modules
        into the pipeline view
        
        """

        # Interprete the diff result
        (p1, p2, v1Andv2, v1Only, v2Only, paramChanged) = self.diff
        p1.ensure_connection_specs()
        p2.ensure_connection_specs()
        p_both = Pipeline()
        # the abstraction map is the same for both p1 and p2
        p_both.set_abstraction_map(p1.abstraction_map)
        
        scene = self.pipelineView.scene()
        scene.clearItems()

        # FIXME HACK: We will create a dummy object that looks like a
        # controller so that the qgraphicsmoduleitems and the scene
        # are happy. It will simply store the pipeline will all
        # modules and connections of the diff, and know how to copy stuff
        class DummyController(object):
            def __init__(self, pip):
                self.current_pipeline = pip
                self.search = None
            def copy_modules_and_connections(self, module_ids, connection_ids):
                """copy_modules_and_connections(module_ids: [long],
                                             connection_ids: [long]) -> str
                Serializes a list of modules and connections
                """

                pipeline = Pipeline()
                pipeline.set_abstraction_map( \
                    self.current_pipeline.abstraction_map)
                for module_id in module_ids:
                    module = self.current_pipeline.modules[module_id]
                    if module.vtType == AbstractionModule.vtType:
                        abstraction = \
                            pipeline.abstraction_map[module.abstraction_id]
                        pipeline.add_abstraction(abstraction)
                    pipeline.add_module(module)
                for connection_id in connection_ids:
                    connection = self.current_pipeline.connections[connection_id]
                    pipeline.add_connection(connection)
                return core.db.io.serialize(pipeline)
                
        controller = DummyController(p_both)
        scene.controller = controller

        # Find the max version id from v1 and start the adding process
        self.maxId1 = 0
        for m1id in p1.modules.keys():
            if m1id>self.maxId1:
                self.maxId1 = m1id
        shiftId = self.maxId1 + 1

        # First add all shared modules, just use v1 module id
        for (m1id, m2id) in v1Andv2:
            item = scene.addModule(p1.modules[m1id],
                                   CurrentTheme.VISUAL_DIFF_SHARED_BRUSH)
            item.controller = controller
            p_both.add_module(copy.copy(p1.modules[m1id]))
            
        # Then add parameter changed version
        for ((m1id, m2id), matching) in paramChanged:
            m1 = p1.modules[m1id]
            m2 = p2.modules[m2id]
            
            #this is a hack for modules with a dynamic local registry.
            #The problem arises when modules have the same name but different
            #input/output ports. We just make sure that the module we add to
            # the canvas has the ports from both modules, so we don't have
            # addconnection errors.
            if m1.registry:
                m1ports = m1.port_specs.itervalues()
                ports = {}
                for p in m1.port_specs.itervalues():
                    ports[p.name] = p
                for port in m2.port_specs.itervalues():
                    if not ports.has_key(port.name):
                        m1.add_port_to_registry(port)
                    else:
                        if ports[port.name].spec != port.spec:
                            #if we add this port, we will get port overloading.
                            #To avoid this, just cast the current port to be of
                            # Module or Variant type.
                            new_port = ports[port.name]
                            m1.delete_port_from_registry(new_port.id)
                            if new_port.type == 'input':
                                new_port.spec = "(Module)"
                            else:
                                new_port.spec = "(Variant)"
                            m1.add_port_to_registry(new_port)
                            
            item = scene.addModule(p1.modules[m1id],
                                   CurrentTheme.VISUAL_DIFF_PARAMETER_CHANGED_BRUSH)
            item.controller = controller
            p_both.add_module(copy.copy(p1.modules[m1id]))

        # Now add the ones only applicable for v1, still using v1 ids
        for m1id in v1Only:
            item = scene.addModule(p1.modules[m1id],
                                   CurrentTheme.VISUAL_DIFF_FROM_VERSION_BRUSH)
            item.controller = controller
            p_both.add_module(copy.copy(p1.modules[m1id]))

        # Now add the ones for v2 only but must shift the ids away from v1
        for m2id in v2Only:
            p2.modules[m2id].id = m2id + shiftId
            item = scene.addModule(p2.modules[m2id],
                                   CurrentTheme.VISUAL_DIFF_TO_VERSION_BRUSH)
            item.controller = controller
            p_both.add_module(copy.copy(p2.modules[m2id]))

        # Create a mapping between share modules between v1 and v2
        v1Tov2 = {}
        v2Tov1 = {}
        for (m1id, m2id) in v1Andv2:
            v1Tov2[m1id] = m2id
            v2Tov1[m2id] = m1id
        for ((m1id, m2id), matching) in paramChanged:
            v1Tov2[m1id] = m2id
            v2Tov1[m2id] = m1id

        # Next we're going to add connections, only connections of
        # v2Only need to shift their ids
        if p1.connections.keys():
            connectionShift = max(p1.connections.keys())+1
        else:
            connectionShift = 0
        allConnections = copy.copy(p1.connections)
        sharedConnections = []
        v2OnlyConnections = []        
        for (cid2, connection2) in copy.copy(p2.connections.items()):
            if connection2.sourceId in v2Only:
                connection2.sourceId += shiftId
            else:
                connection2.sourceId = v2Tov1[connection2.sourceId]
                
            if connection2.destinationId in v2Only:
                connection2.destinationId += shiftId
            else:
                connection2.destinationId = v2Tov1[connection2.destinationId]

            # Is this connection also shared by p1?
            shared = False
            for (cid1, connection1) in p1.connections.items():
                if (connection1.sourceId==connection2.sourceId and
                    connection1.destinationId==connection2.destinationId and
                    connection1.source.name==connection2.source.name and
                    connection1.destination.name==connection2.destination.name):
                    sharedConnections.append(cid1)
                    shared = True
                    break
            if not shared:
                allConnections[cid2+connectionShift] = connection2
                connection2.id = cid2+connectionShift
                v2OnlyConnections.append(cid2+connectionShift)

        connectionItems = []
        for c in allConnections.values():
            p_both.add_connection(copy.copy(c))
            connectionItems.append(scene.addConnection(c))

        # Color Code connections
        for c in connectionItems:
            if c.id in sharedConnections:
                pass
            elif c.id in v2OnlyConnections:
                pen = QtGui.QPen(CurrentTheme.CONNECTION_PEN)
                pen.setBrush(CurrentTheme.VISUAL_DIFF_TO_VERSION_BRUSH)
                c.connectionPen = pen
            else:
                pen = QtGui.QPen(CurrentTheme.CONNECTION_PEN)
                pen.setBrush(CurrentTheme.VISUAL_DIFF_FROM_VERSION_BRUSH)
                c.connectionPen = pen

       

        scene.updateSceneBoundingRect()
        scene.fitToView(self.pipelineView, True)
