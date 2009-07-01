
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
""" This containing a subclassed QGraphicsView that allows View the
pipeline in a specific way in the parameter exploration window
"""
from PyQt4 import QtCore, QtGui
from core.inspector import PipelineInspector
from gui.common_widgets import QToolWindowInterface
from gui.pipeline_view import QPipelineView, QGraphicsModuleItem
from gui.theme import CurrentTheme

################################################################################
class QAnnotatedPipelineView(QPipelineView, QToolWindowInterface):
    """
    QAnnotatedPipelineView subclass QPipelineView to perform some overlay
    marking on a pipeline view
    
    """
    def __init__(self, parent=None):
        """ QPipelineView(parent: QWidget) -> QPipelineView
        Initialize the graphics view and its properties
        
        """
        QPipelineView.__init__(self, parent)
        self.setWindowTitle('Annotated Pipeline')
        self.inspector = PipelineInspector()

    def sizeHint(self):
        """ sizeHint() -> QSize
        Prefer the view not so large
        
        """
        return QtCore.QSize(256, 256)

    def paintEvent(self, event):
        """ paintEvent(event: QPaintEvent) -> None
        Paint an overlay annotation on spreadsheet cell modules
        
        """
        QPipelineView.paintEvent(self, event)
        # super(QAnnotatedPipelineView, self).paintEvent(event)
        if self.scene():
            painter = QtGui.QPainter(self.viewport())
            for mId, annotatedId in \
                    self.inspector.annotated_modules.iteritems():
                item = self.scene().modules[mId]
                br = item.sceneBoundingRect()
                rect = QtCore.QRect(self.mapFromScene(br.topLeft()),
                                    self.mapFromScene(br.bottomRight()))
                QAnnotatedPipelineView.drawId(painter, rect, annotatedId)
            painter.end()

    def updateAnnotatedIds(self, pipeline):
        """ updateAnnotatedIds(pipeline: Pipeline) -> None
        Re-inspect the pipeline to get annotated ids
        
        """
        self.inspector.inspect_ambiguous_modules(pipeline)
        self.scene().fitToView(self)

    @staticmethod
    def drawId(painter, rect, id, align=QtCore.Qt.AlignCenter):
        """ drawId(painter: QPainter, rect: QRect, id: int,
                   align: QtCore.Qt.Align) -> None
        Draw the rounded id number on a rectangular area
        
        """
        painter.save()
        painter.setRenderHints(QtGui.QPainter.Antialiasing)
        painter.setPen(CurrentTheme.ANNOTATED_ID_BRUSH.color())
        painter.setBrush(CurrentTheme.ANNOTATED_ID_BRUSH)
        font = QtGui.QFont()
        font.setStyleStrategy(QtGui.QFont.ForceOutline)
        font.setBold(True)
        painter.setFont(font)
        fm = QtGui.QFontMetrics(font)
        size = fm.size(QtCore.Qt.TextSingleLine, str(id))
        size = max(size.width(), size.height())
        
        x = rect.left()
        if align & QtCore.Qt.AlignHCenter:
            x = rect.left() + rect.width()/2-size/2
        if align & QtCore.Qt.AlignRight:
            x = rect.left() + rect.width()-size
        y = rect.top()
        if align & QtCore.Qt.AlignVCenter:
            y = rect.top() + rect.height()/2-size/2
        if align & QtCore.Qt.AlignBottom:
            y = rect.top() + rect.height()-size
            
        newRect = QtCore.QRect(x, y, size, size)
        painter.drawEllipse(newRect)
        painter.setPen(CurrentTheme.ANNOTATED_ID_PEN)
        painter.drawText(newRect, QtCore.Qt.AlignCenter, str(id))
        painter.restore()
