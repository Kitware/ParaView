
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
""" The file describes the parameter exploration table for VisTrails

QParameterExplorationTable
"""


from PyQt4 import QtCore, QtGui
from gui.theme import CurrentTheme
from gui.common_widgets import QPromptWidget
from gui.paramexplore.param_view import QParameterTreeWidget
from gui.utils import show_warning
from core.modules.module_registry import registry
from core.modules.basic_modules import Constant
from core.vistrail.module_param import ModuleParam
from core.modules.paramexplore import QParameterEditor
import core.db.action

################################################################################
class QParameterExplorationWidget(QtGui.QScrollArea):
    """
    QParameterExplorationWidget is a place holder for
    QParameterExplorationTable

    is a grid layout widget having 4 comlumns corresponding to 4
    dimensions of exploration. It accept method/alias drops and can be
    fully configured onto any dimension. For each parameter, 3
    different approach can be chosen to assign the value of that
    parameter during the exploration: linear interpolation (for int,
    float), list (for int, float, string and boolean) and user-define
    function (for int, float, string and boolean)
    
    """
    def __init__(self, parent=None):
        """ QParameterExplorationWidget(parent: QWidget)
                                       -> QParameterExplorationWidget
        Put the QParameterExplorationTable as a main widget
        
        """
        QtGui.QScrollArea.__init__(self, parent)
        self.setAcceptDrops(True)
        self.setWidgetResizable(True)
        self.table = QParameterExplorationTable()
        self.setWidget(self.table)

    def dragEnterEvent(self, event):
        """ dragEnterEvent(event: QDragEnterEvent) -> None
        Set to accept drops from the parameter list
        
        """
        if type(event.source())==QParameterTreeWidget:            
            data = event.mimeData()
            if hasattr(data, 'items'):
                event.accept()
                return
        event.ignore()
        
    def dropEvent(self, event):
        """ dropEvent(event: QDragMoveEvent) -> None
        Accept drop event to add a new method
        
        """
        if type(event.source())==QParameterTreeWidget:
            data = event.mimeData()
            if hasattr(data, 'items'):
                event.accept()
                for item in data.items:
                    self.table.addParameter(item.parameter)
            vsb = self.verticalScrollBar()
            vsb.setValue(vsb.maximum())

    def updatePipeline(self, pipeline):
        """ updatePipeline(pipeline: Pipeline) -> None
        Assign a pipeline to the table
        
        """
        self.table.setPipeline(pipeline)
                    
class QParameterExplorationTable(QPromptWidget):
    """
    QParameterExplorationTable is a grid layout widget having 4
    comlumns corresponding to 4 dimensions of exploration. It accept
    method/alias drops and can be fully configured onto any
    dimension. For each parameter, 3 different approach can be chosen
    to assign the value of that parameter during the exploration:
    linear interpolation (for int, float), list (for int, float,
    string and boolean) and user-define function (for int, float,
    string and boolean)
    
    """
    def __init__(self, parent=None):
        """ QParameterExplorationTable(parent: QWidget)
                                       -> QParameterExplorationTable
        Create an grid layout and accept drops
        
        """
        QPromptWidget.__init__(self, parent)
        self.pipeline = None
        self.setSizePolicy(QtGui.QSizePolicy.Expanding,
                           QtGui.QSizePolicy.Expanding)
        self.setPromptText('Drag aliases/parameters here for a parameter '
                           'exploration')
        self.showPrompt()
        
        vLayout = QtGui.QVBoxLayout(self)
        vLayout.setSpacing(0)
        vLayout.setMargin(0)
        vLayout.setAlignment(QtCore.Qt.AlignTop)
        self.setLayout(vLayout)

        self.label = QDimensionLabel()

        for labelIcon in self.label.labelIcons:
            self.connect(labelIcon.countWidget,
                         QtCore.SIGNAL('editingFinished()'),
                         self.updateWidgets)
        vLayout.addWidget(self.label)

        for i in xrange(2):
            hBar = QtGui.QFrame()
            hBar.setFrameStyle(QtGui.QFrame.HLine | QtGui.QFrame.Sunken)
            vLayout.addWidget(hBar)
        self._parameterCount = 0

    def addParameter(self, paramInfo):
        """ addParameter(paramInfo: (str, [ParameterInfo]) -> None
        Add a parameter to the table. The parameter info is specified
        in QParameterTreeWidgetItem
        
        """
        # Check to see paramInfo is not a subset of some other parameter set
        params = paramInfo[1]
        for i in xrange(self.layout().count()):
            pEditor = self.layout().itemAt(i).widget()
            if pEditor and type(pEditor)==QParameterSetEditor:
                subset = True
                for p in params:
                    if not (p in pEditor.info[1]):
                        subset = False
                        break                    
                if subset:
                    show_warning('Parameter Exists',
                                 'The parameter you are trying to add is '
                                 'already in the list.')
                    return
        self.showPrompt(False)
        newEditor = QParameterSetEditor(paramInfo, self)

        # Make sure to disable all duplicated parameter
        for p in xrange(len(params)):
            for i in xrange(self.layout().count()):
                pEditor = self.layout().itemAt(i).widget()
                if pEditor and type(pEditor)==QParameterSetEditor:
                    if params[p] in pEditor.info[1]:
                        widget = newEditor.paramWidgets[p]
                        widget.setDimension(4)
                        widget.setDuplicate(True)
                        widget.setEnabled(False)
                        break
        
        self.layout().addWidget(newEditor)
        newEditor.show()
        self.setMinimumHeight(self.layout().minimumSize().height())
        self.emit(QtCore.SIGNAL('exploreChange(bool)'), self.layout().count() > 3)

    def removeParameter(self, ps):
        """ removeParameterSet(ps: QParameterSetEditor) -> None
        Remove a parameter set from the table and validate the rest
        
        """
        self.layout().removeWidget(ps)
        # Restore disabled parameter
        for i in xrange(self.layout().count()):
            pEditor = self.layout().itemAt(i).widget()
            if pEditor and type(pEditor)==QParameterSetEditor:
                for p in xrange(len(pEditor.info[1])):
                    param = pEditor.info[1][p]
                    widget = pEditor.paramWidgets[p]                    
                    if (param in ps.info[1] and (not widget.isEnabled())):
                        widget.setDimension(0)
                        widget.setDuplicate(False)
                        widget.setEnabled(True)
                        break
        self.showPrompt(self.layout().count()<=3)
        self.emit(QtCore.SIGNAL('exploreChange(bool)'), self.layout().count() > 3)

    def updateWidgets(self):
        """ updateWidgets() -> None
        Update all widgets to reflect the step count
        
        """
        # Go through all possible parameter widgets
        counts = self.label.getCounts()
        for i in xrange(self.layout().count()):
            pEditor = self.layout().itemAt(i).widget()
            if pEditor and type(pEditor)==QParameterSetEditor:
                for paramWidget in pEditor.paramWidgets:
                    dim = paramWidget.getDimension()
                    if dim in [0, 1, 2, 3]:
                        se = paramWidget.editor.stackedEditors
                        # Notifies editor widgets of size update 
                        for i in xrange(se.count()):
                            wd = se.widget(i)
                            if hasattr(wd, 'size_was_updated'):
                                wd.size_was_updated(counts[dim])

    def clear(self):
        """ clear() -> None
        Clear all widgets
        
        """
        for i in reversed(range(self.layout().count())):
            pEditor = self.layout().itemAt(i).widget()
            if pEditor and type(pEditor)==QParameterSetEditor:
                pEditor.table = None
                self.layout().removeWidget(pEditor)
                pEditor.hide()
                pEditor.deleteLater()
        self.label.resetCounts()
        self.showPrompt()
        self.emit(QtCore.SIGNAL('exploreChange(bool)'), self.layout().count() > 3)

    def setPipeline(self, pipeline):
        """ setPipeline(pipeline: Pipeline) -> None
        Assign a pipeline to the current table
        
        """
        if pipeline:
            for i in xrange(self.layout().count()):
                pEditor = self.layout().itemAt(i).widget()
                if pEditor and type(pEditor)==QParameterSetEditor:
                    for param in pEditor.info[1]:
                        if not pipeline.db_has_object(param[3], param[2]):
                            pEditor.removeSelf()
        else:
            self.clear()
        self.pipeline = pipeline
        self.label.setEnabled(self.pipeline!=None)

    def collectParameterActions(self):
        """ collectParameterActions() -> list
        Return a list of action lists corresponding to each dimension
        
        """
        if not self.pipeline:
            return None
        parameterValues = [[], [], [], []]
        counts = self.label.getCounts()
        for i in xrange(self.layout().count()):
            pEditor = self.layout().itemAt(i).widget()
            if pEditor and type(pEditor)==QParameterSetEditor:
                for paramWidget in pEditor.paramWidgets:
                    editor = paramWidget.editor
                    interpolator = editor.stackedEditors.currentWidget()
                    paramInfo = paramWidget.param
                    dim = paramWidget.getDimension()
                    if dim in [0, 1, 2, 3]:
                        count = counts[dim]
                        values = interpolator.get_values(count)
                        if not values:
                            return None
                        pId = paramInfo.id
                        pType = paramInfo.dbtype
                        parentType = paramInfo.parent_dbtype
                        parentId = paramInfo.parent_id
                        function = self.pipeline.db_get_object(parentType,
                                                               parentId)  
                        fName = function.name
                        old_param = self.pipeline.db_get_object(pType,pId)
                        pName = old_param.name
                        pAlias = old_param.alias
                        actions = []
                        tmp_id = -1L
                        for v in values:
                            new_param = ModuleParam(id=tmp_id,
                                                    pos=old_param.pos,
                                                    name=pName,
                                                    alias=pAlias,
                                                    val=str(v),
                                                    type=paramInfo.type
                                                    )
                            action_spec = ('change', old_param, new_param,
                                           parentType, function.real_id)
                            action = core.db.action.create_action([action_spec])
                            actions.append(action)
                        parameterValues[dim].append(actions)
                        tmp_id -= 1
        return [zip(*p) for p in parameterValues]

class QDimensionLabel(QtGui.QWidget):
    """
    QDimensionLabel represents a horizontal header item of the
    parameter window. It has 4 small icons represents the dimensions
    and a Skip label. It represents a group box.
    
    """
    def __init__(self, parent=None):
        """ QDimensionLabel(parent: QWidget) -> None
        Initialize icons and labels
        
        """
        QtGui.QWidget.__init__(self, parent)
        self.setAutoFillBackground(True)
        self.setSizePolicy(QtGui.QSizePolicy.Expanding,
                           QtGui.QSizePolicy.Maximum)

        hLayout = QtGui.QHBoxLayout(self)
        hLayout.setMargin(0)
        hLayout.setSpacing(0)
        self.setLayout(hLayout)        

        self.params = QDimensionLabelText('Parameters')
        hLayout.addWidget(self.params)
        self.params.setSizePolicy(QtGui.QSizePolicy.Expanding,
                                  QtGui.QSizePolicy.Expanding)
        hLayout.addWidget(QDimensionLabelSeparator())
        
        pixes = [CurrentTheme.EXPLORE_COLUMN_PIXMAP,
                 CurrentTheme.EXPLORE_ROW_PIXMAP,
                 CurrentTheme.EXPLORE_SHEET_PIXMAP,
                 CurrentTheme.EXPLORE_TIME_PIXMAP]
        self.labelIcons = []
        for pix in pixes:
            labelIcon = QDimensionLabelIcon(pix)
            hLayout.addWidget(labelIcon)
            self.labelIcons.append(labelIcon)            
            hLayout.addWidget(QDimensionLabelSeparator())

        hLayout.addWidget(QDimensionLabelIcon(CurrentTheme.EXPLORE_SKIP_PIXMAP,
                                              False))

    def getCounts(self):
        """ getCounts() -> [int]        
        Return a list of 4 ints denoting the step count desired for
        each dimension
        
        """
        return [l.countWidget.value() for l in self.labelIcons]

    def resetCounts(self):
        """ resetCounts() -> None
        Reset all counts to 1
        
        """
        for l in self.labelIcons:
            l.countWidget.setValue(1)
    
class QDimensionSpinBox(QtGui.QSpinBox):
    """
    QDimensionSpinBox is just an overrided spin box that will also emit
    'editingFinished()' signal when the user interact with mouse
    
    """    
    def mouseReleaseEvent(self, event):
        """ mouseReleaseEvent(event: QMouseEvent) -> None
        Emit 'editingFinished()' signal when the user release a mouse button
        
        """
        QtGui.QSpinBox.mouseReleaseEvent(self, event)
        # super(QDimensionSpinBox, self).mouseReleaseEvent(event)
        self.emit(QtCore.SIGNAL("editingFinished()"))

class QDimensionLabelIcon(QtGui.QWidget):
    """
    QDimensionLabelIcon describes those icons staying on the header
    view of the table
    
    """
    def __init__(self, pix, hasCount = True, parent=None):
        """ QDimensionalLabelIcon(pix: QPixmap, hasCount: bool, parent: QWidget)
                                  -> QDimensionalLabelIcon
        Using size 32x32 and a margin of 2
        
        """
        QtGui.QWidget.__init__(self, parent)
        layout = QtGui.QVBoxLayout()
        layout.setMargin(0)
        layout.setSpacing(0)
        self.setLayout(layout)

        label = QtGui.QLabel()
        label.setAlignment(QtCore.Qt.AlignCenter)
        label.setPixmap(pix.scaled(32, 32, QtCore.Qt.KeepAspectRatio,
                                  QtCore.Qt.SmoothTransformation))
        layout.addWidget(label)

        if hasCount:
            self.countWidget = QDimensionSpinBox()
            self.countWidget.setFixedWidth(32)
            self.countWidget.setRange(1, 10000000)
            self.countWidget.setAlignment(QtCore.Qt.AlignRight)
            self.countWidget.setFrame(False)
            pal = QtGui.QPalette(self.countWidget.lineEdit().palette())
            pal.setBrush(QtGui.QPalette.Base,
                         QtGui.QBrush(QtCore.Qt.NoBrush))
            self.countWidget.lineEdit().setPalette(pal)
            layout.addWidget(self.countWidget)
            
        self.setSizePolicy(QtGui.QSizePolicy.Maximum,
                            QtGui.QSizePolicy.Maximum)        
                
class QDimensionLabelText(QtGui.QWidget):
    """
    QDimensionLabelText describes those texts staying on the header
    view of the table. It also has a button to perform exploration
    
    """
    def __init__(self, text, parent=None):
        """ QDimensionalLabelText(text: str, parent: QWidget)
                                   -> QDimensionalLabelText
        Putting the text bold in the center
        
        """
        QtGui.QWidget.__init__(self, parent)
        hLayout = QtGui.QHBoxLayout()
        self.setLayout(hLayout)

        hLayout.addStretch()
        
        hLayout.addWidget(QtGui.QLabel('<b>Parameters</b>'))

        hLayout.addStretch()
        
        
class QDimensionLabelSeparator(QtGui.QFrame):
    """
    QDimensionLabelSeparator is acting as a vertical separator which
    has an appropriate style to go with the QDimensionalLabel
    
    """
    def __init__(self, parent=None):
        """ QDimensionalLabelSeparator(parent: QWidget)
                                       -> QDimensionalLabelSeparator
        Make sure the frame only has a width of 2
        
        """
        QtGui.QFrame.__init__(self, parent)
        self.setFrameStyle(QtGui.QFrame.Panel | QtGui.QFrame.Sunken)
        self.setFixedWidth(2)

class QParameterSetEditor(QtGui.QWidget):
    """
    QParameterSetEditor is a widget controlling a set of
    parameters. The set can contain a single parameter (aliases) or
    multiple of them (module methods).
    
    """
    def __init__(self, info, table=None, parent=None):
        """ QParameterSetEditor(info: paraminfo,
                                table: QParameterExplorationTable,
                                parent: QWidget)
                                -> QParameterSetEditor
        Construct a parameter editing widget based on the paraminfo
        (described in QParameterTreeWidgetItem)
        
        """
        QtGui.QWidget.__init__(self, parent)
        self.info = info
        self.table = table
        (name, paramList) = info
        if table:
            size = table.label.getCounts()[0]
        else:
            size = 1
        
        vLayout = QtGui.QVBoxLayout(self)
        vLayout.setMargin(0)
        vLayout.setSpacing(0)
        self.setLayout(vLayout)

        label = QParameterSetLabel(name)
        self.connect(label.removeButton, QtCore.SIGNAL('clicked()'),
                     self.removeSelf)
        vLayout.addWidget(label)
        
        self.paramWidgets = []
        for param in paramList:
            paramWidget = QParameterWidget(param, size)
            vLayout.addWidget(paramWidget)
            self.paramWidgets.append(paramWidget)

        vLayout.addSpacing(10)

        hBar = QtGui.QFrame()
        hBar.setFrameStyle(QtGui.QFrame.HLine | QtGui.QFrame.Sunken)
        vLayout.addWidget(hBar)

    def removeSelf(self):
        """ removeSelf() -> None
        Remove itself out of the parent layout()
        
        """
        if self.table:
            self.table.removeParameter(self)            
            self.table = None
            self.close()
            self.deleteLater()

class QParameterSetLabel(QtGui.QWidget):
    """
    QParameterSetLabel is the label bar showing at the top of the
    parameter set editor. It also has a Remove button to remove the
    parameter
    
    """
    def __init__(self, text, parent=None):
        """ QParameterSetLabel(text: str, parent: QWidget) -> QParameterSetLabel
        Init a label and a button
        
        """
        QtGui.QWidget.__init__(self, parent)        
        hLayout = QtGui.QHBoxLayout(self)
        hLayout.setMargin(0)
        hLayout.setSpacing(0)
        self.setLayout(hLayout)

        hLayout.addSpacing(5)

        label = QtGui.QLabel(text)
        font = QtGui.QFont(label.font())
        font.setBold(True)
        label.setFont(font)
        hLayout.addWidget(label)

        hLayout.addSpacing(5)
        
        self.removeButton = QtGui.QToolButton()
        self.removeButton.setAutoRaise(True)
        self.removeButton.setIcon(QtGui.QIcon(
            self.style().standardPixmap(QtGui.QStyle.SP_DialogCloseButton)))
        self.removeButton.setIconSize(QtCore.QSize(12, 12))
        self.removeButton.setFixedWidth(16)
        hLayout.addWidget(self.removeButton)

        hLayout.addStretch()
        
class QParameterWidget(QtGui.QWidget):
    """
    QParameterWidget is a row widget containing a label, a parameter
    editor and a radio group.
    
    """
    def __init__(self, param, size, parent=None):
        """ QParameterWidget(param: ParameterInfo, size: int, parent: QWidget)
                             -> QParameterWidget
        """
        QtGui.QWidget.__init__(self, parent)
        self.param = param
        self.prevWidget = 0
        
        hLayout = QtGui.QHBoxLayout(self)
        hLayout.setMargin(0)
        hLayout.setSpacing(0)        
        self.setLayout(hLayout)

        hLayout.addSpacing(5+16+5)

        self.label = QtGui.QLabel(param.type)
        self.label.setFixedWidth(50)
        hLayout.addWidget(self.label)

        module = registry.get_module_by_name(param.identifier,
                                             param.type,
                                             param.namespace)
        assert issubclass(module, Constant)

        self.editor = QParameterEditor(param, size)
        hLayout.addWidget(self.editor)

        self.selector = QDimensionSelector()
        self.connect(self.selector.radioButtons[4],
                     QtCore.SIGNAL('toggled(bool)'),
                     self.disableParameter)
        hLayout.addWidget(self.selector)

    def getDimension(self):
        """ getDimension() -> int        
        Return a number 0-4 indicating which radio button is
        selected. If none is selected (should not be in this case),
        return -1
        
        """
        for i in xrange(5):
            if self.selector.radioButtons[i].isChecked():
                return i
        return -1

    def disableParameter(self, disabled=True):
        """ disableParameter(disabled: bool) -> None
        Disable/Enable this parameter when disabled is True/False
        
        """
        self.label.setEnabled(not disabled)
        self.editor.setEnabled(not disabled)

    def setDimension(self, dim):
        """ setDimension(dim: int) -> None
        Select a dimension for this parameter
        
        """
        if dim in xrange(5):
            self.selector.radioButtons[dim].setChecked(True)

    def setDuplicate(self, duplicate):
        """ setDuplicate(duplicate: True) -> None
        Set if this parameter is a duplicate parameter
        
        """
        if duplicate:
            self.prevWidget = self.editor.stackedEditors.currentIndex()
            self.editor.stackedEditors.setCurrentIndex(3)
        else:
            self.editor.stackedEditors.setCurrentIndex(self.prevWidget)

class QDimensionSelector(QtGui.QWidget):
    """
    QDimensionSelector provides 5 radio buttons to select dimension of
    exploration or just skipping it.
    
    """
    def __init__(self, parent=None):
        """ QDimensionSelector(parent: QWidget) -> QDimensionSelector
        Initialize the horizontal layout and set the width to be fixed
        equal to the QDimensionLabel
        
        """
        QtGui.QWidget.__init__(self, parent)
        self.setSizePolicy(QtGui.QSizePolicy.Maximum,
                           QtGui.QSizePolicy.Maximum)
        
        hLayout = QtGui.QHBoxLayout(self)
        hLayout.setMargin(0)
        hLayout.setSpacing(0)        
        self.setLayout(hLayout)

        self.radioButtons = []
        for i in xrange(5):
            hLayout.addSpacing(2)
            button = QDimensionRadioButton()
            self.radioButtons.append(button)
            button.setFixedWidth(32)
            hLayout.addWidget(button)
        self.radioButtons[0].setChecked(True)

class QDimensionRadioButton(QtGui.QRadioButton):
    """
    QDimensionRadioButton is a replacement of QRadioButton with
    simpler appearance. We just need to override the paint event
    
    """
    def paintEvent(self, event):
        """ paintEvent(event: QPaintEvent) -> None
        Draw an outer circle and another solid one in side
        
        """
        painter = QtGui.QPainter(self)
        painter.setRenderHint(QtGui.QPainter.Antialiasing)
        painter.setPen(self.palette().color(QtGui.QPalette.Dark))
        painter.setBrush(QtCore.Qt.NoBrush)
        l = min(self.width()-2, self.height()-2, 12)
        r = QtCore.QRect(0, 0, l, l)
        r.moveCenter(self.rect().center())
        painter.drawEllipse(r)

        if self.isChecked():
            r.adjust(3, 3, -3, -3)
            painter.setPen(QtCore.Qt.NoPen)
            painter.setBrush(self.palette().color(QtGui.QPalette.WindowText))
            painter.drawEllipse(r)
        
        painter.end()

    def mousePressEvent(self, event):
        """ mousePressEvent(event: QMouseEvent) -> None
        Force toggling the radio button
        
        """
        self.click()

################################################################################

if __name__=="__main__":        
    import sys
    import gui.theme
    app = QtGui.QApplication(sys.argv)
    gui.theme.initializeCurrentTheme()
    vc = QDimensionLabel(CurrentTheme.EXPLORE_SHEET_PIXMAP, 'Hello World')
    vc.show()
    sys.exit(app.exec_())
