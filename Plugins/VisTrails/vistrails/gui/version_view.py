
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
""" This is a QGraphicsView for pipeline view, it also holds different
types of graphics items that are only available in the pipeline
view. It only handles GUI-related actions, the rest of the
functionalities are implemented at somewhere else,
e.g. core.vistrails

QGraphicsLinkItem
QGraphicsVersionTextItem
QGraphicsVersionItem
QVersionTreeScene
QVersionTreeView
"""

from PyQt4 import QtCore, QtGui
from core.dotty import DotLayout
from core.vistrails_tree_layout_lw import VistrailsTreeLayoutLW
from core.system import systemType
from gui.graphics_view import (QInteractiveGraphicsScene,
                               QInteractiveGraphicsView,
                               QGraphicsItemInterface,
                               QGraphicsRubberBandItem)
from gui.version_prop import QVersionPropOverlay
from gui.theme import CurrentTheme
from gui.vis_diff import QVisualDiff
from gui.qt import qt_super
import gui.utils
import math


################################################################################
# QGraphicsLinkItem

class QGraphicsLinkItem(QGraphicsItemInterface, QtGui.QGraphicsPolygonItem):
    """
    QGraphicsLinkItem is a connection shape connecting two versions
    
    """
    def __init__(self, parent=None, scene=None):
        """ QGraphicsLinkItem(parent: QGraphicsItem,
                              scene: QGraphicsScene) -> QGraphicsLinkItem
        Create the shape, initialize its pen and brush accordingly
        
        """
        QtGui.QGraphicsPolygonItem.__init__(self, parent, scene)
        self.setFlags(QtGui.QGraphicsItem.ItemIsSelectable)
        self.setZValue(0)
        self.linkPen = CurrentTheme.LINK_PEN
        self.ghosted = False
        
        # cache link endpoints to improve performance on scene updates
        self.c1 = None
        self.c2 = None
        self.expand = None
        self.collapse = None

    def mousePressEvent(self, event):
        """ mousePressEvent(event: QMouseEvent) -> None

        """
        qt_super(QGraphicsLinkItem, self).mousePressEvent(event)
        self.setSelected(True)

    def mouseReleaseEvent(self, event):
        """ mouseReleaseEvent(event: QMouseEvent) -> None
        
        """
        qt_super(QGraphicsLinkItem, self).mouseReleaseEvent(event)
        if self.expand:
            self.scene().controller.expand_versions(self.startVersion, self.endVersion)
        elif self.collapse:
            self.scene().controller.collapse_versions(self.endVersion)
        self.setSelected(False)

    def setGhosted(self, ghosted):
        """ setGhosted(ghosted: True) -> None
        Set this link to be ghosted or not
        
        """
        if self.ghosted <> ghosted:
            self.ghosted = ghosted
            if ghosted:
                self.linkPen = CurrentTheme.GHOSTED_LINK_PEN
            else:
                self.linkPen = CurrentTheme.LINK_PEN

    def setupLink(self, v1, v2, expand=False, collapse=False):
        """ setupLink(v1, v2: QGraphicsVersionItem, compact: bool) -> None
        Setup a line connecting v1 and v2 items
        
        """
        self.startVersion = min(v1.id, v2.id)
        self.endVersion = max(v1.id, v2.id)
        
        c1 = v1.sceneBoundingRect().center()
        c2 = v2.sceneBoundingRect().center()

        # check if it is the same geometry 
        # improves performance on updates
        if self.c1 != None and self.c2 != None and \
                self.expand != None and self.collapse !=None:
            isTheSame = self.c1 == c1 and \
                self.c2 == c2 and \
                self.expand == expand and \
                self.collapse == collapse
            if isTheSame:
                return
        
        # update current state
        self.c1 = c1
        self.c2 = c2
        self.collapse = collapse
        self.expand = expand

        # Compute the line of the link and its normal line throught
        # the midpoint
        mainLine = QtCore.QLineF(c1, c2)
        normalLine = mainLine.normalVector()        
        normalLine.setLength(CurrentTheme.LINK_SEGMENT_LENGTH)
        dis = (mainLine.pointAt(0.5)-mainLine.p1()+
               normalLine.p1()-normalLine.pointAt(0.5))
        normalLine.translate(dis.x(), dis.y())

        # Generate 2 segments along the main line and 3 segments along
        # the normal line
        if not self.collapse and not self.expand:
            self.lines = [mainLine]
            poly = QtGui.QPolygonF()
            poly.append(self.lines[0].p1())
            poly.append(self.lines[0].p2())
            poly.append(self.lines[0].p1())
            self.setPolygon(poly)
        else:
            self.lines = []
            
            normalLine = mainLine.normalVector()        
            normalLine.setLength(CurrentTheme.LINK_SEGMENT_SQUARE_LENGTH)
            dis = (mainLine.pointAt(0.5)-mainLine.p1()+
                   normalLine.p1()-normalLine.pointAt(0.5))
            normalLine.translate(dis.x(), dis.y())
            
            gapLine = QtCore.QLineF(mainLine)
            gapLine.setLength(CurrentTheme.LINK_SEGMENT_SQUARE_LENGTH/2)
            gapVector = gapLine.p2()-gapLine.p1()
            
            # First segment along the main line
            line = QtCore.QLineF(mainLine)
            line.setLength(line.length()/2-CurrentTheme.LINK_SEGMENT_SQUARE_LENGTH/2)
            self.lines.append(QtCore.QLineF(line))
            
            # Second segment along the main line
            line.translate(line.p2()-line.p1()+gapVector*2)
            self.lines.append(QtCore.QLineF(line))
            
            # First normal segment in front
            line_t = QtCore.QLineF(normalLine)
            line_t.translate(gapVector*(-1.0))
            self.lines.append(QtCore.QLineF(line_t))
        
            # Second normal segment in back
            line_b = QtCore.QLineF(normalLine)
            line_b.translate(gapVector)
            self.lines.append(QtCore.QLineF(line_b))
        
            # Left box
            line = QtCore.QLineF(line_t.p1(),line_b.p1())
            self.lines.append(QtCore.QLineF(line))
        
            # Right box
            line = QtCore.QLineF(line_t.p2(),line_b.p2())
            self.lines.append(QtCore.QLineF(line))

            # Horizontal plus
            line_h = QtCore.QLineF(normalLine.pointAt(0.2),normalLine.pointAt(0.8))
            self.lines.append(QtCore.QLineF(line_h))
            
            if self.expand:
                # Vertical plus
                line = QtCore.QLineF(mainLine)
                line.translate((line.p2()-line.p1())/2-gapVector)
                line.setLength(CurrentTheme.LINK_SEGMENT_SQUARE_LENGTH)
                line_v = QtCore.QLineF(line.pointAt(0.2), line.pointAt(0.8))
                self.lines.append(QtCore.QLineF(line_v))
        
            # Create the poly line for selection and redraw
            poly = QtGui.QPolygonF()
            poly.append(self.lines[0].p1())
            poly.append(self.lines[2].p1())
            poly.append(self.lines[3].p1())
            poly.append(self.lines[1].p2())
            poly.append(self.lines[3].p2())
            poly.append(self.lines[2].p2())
            poly.append(self.lines[0].p1())
            self.setPolygon(poly)

        self.setGhosted(v1.ghosted and v2.ghosted)

    def paint(self, painter, option, widget=None):
        """ paint(painter: QPainter, option: QStyleOptionGraphicsItem,
                  widget: QWidget) -> None
        Peform actual painting of the link
        
        """
        if self.isSelected():
            painter.setPen(CurrentTheme.LINK_SELECTED_PEN)
        else:
            painter.setPen(self.linkPen)
        for line in self.lines:
            painter.drawLine(line)

    def itemChange(self, change, value):
        """ itemChange(change: GraphicsItemChange, value: QVariant) -> QVariant
        Do not allow link to be selected with version shape
        
        """
        if change==QtGui.QGraphicsItem.ItemSelectedChange and value.toBool():
            selectedItems = self.scene().selectedItems()
            for item in selectedItems:
                if type(item)==QGraphicsVersionItem:
                    return QtCore.QVariant(False)
        return QtGui.QGraphicsPolygonItem.itemChange(self, change, value)


##############################################################################
# QGraphicsVerstionTextItem

class QGraphicsVersionTextItem(QGraphicsItemInterface, QtGui.QGraphicsTextItem):
    """
    QGraphicsVersionTextItem is an editable text item that appears on top of
    a QGraphicsVersionItem to allow the tag to be changed

    """
    def __init__(self, parent=None, scene=None):
        """ QGraphicsVersionTextItem(parent: QGraphicsVersionItem, 
        scene: QGraphicsScene) -> QGraphicsVersionTextItem

        Create the shape, intialize its drawing style

        """
        QtGui.QGraphicsTextItem.__init__(self, parent, scene)
        self.parent = parent
        self.setTextInteractionFlags(QtCore.Qt.TextEditorInteraction)
        self.setFont(CurrentTheme.VERSION_FONT)
        self.setTextWidth(CurrentTheme.VERSION_LABEL_MARGIN[0])
        self.centerX = 0.0
        self.centerY = 0.0
        self.label = ''
        self.isTag = True
        self.updatingTag = False
        self.previousWidth = 0.0

    def changed(self, x, y, label, tag=True):
        """ changed(x: float, y: float, label: str) -> None
        Change the position and text label from outside the editor

        """
        if self.centerX <> x or self.centerY <> y or self.label <> label:
            self.centerX = x
            self.centerY = y
            self.label = label
            self.isTag = tag
            if self.isTag:
                self.setFont(CurrentTheme.VERSION_FONT)
            else:
                self.setFont(CurrentTheme.VERSION_DESCRIPTION_FONT)
            self.reset()

    def reset(self):
        """ reset() -> None
        Resets the text label, width, and positions to the stored values

        """
        previousWidth = self.boundingRect().width()
        self.setPlainText(self.label)
        if (len(str(self.label)) > 0):
            self.setTextWidth(-1)
        else:
            self.setTextWidth(CurrentTheme.VERSION_LABEL_MARGIN[0])
            
        if self.isTag:
            self.setFont(CurrentTheme.VERSION_FONT)
        else:
            self.setFont(CurrentTheme.VERSION_DESCRIPTION_FONT)  
        self.updatePos()
        self.parent.updateWidthFromLabel(self.boundingRect().width()-previousWidth)

    def updatePos(self):
        """ updatePos() -> None
        Center the text, by default it uses the upper left corner
        
        """
        self.setPos(self.centerX-self.boundingRect().width()/2.0,
                    self.centerY-self.boundingRect().height()/2.0)

    def keyPressEvent(self, event):
        """ keyPressEvent(event: QEvent) -> None
        Enter and Return keys signal a change in the label.  Other keys
        update the position and size of the parent ellipse during key entry.

        """
        if event.key() in [QtCore.Qt.Key_Enter, QtCore.Qt.Key_Return, QtCore.Qt.Key_Escape]:
            self.updatingTag = True
            if (self.label == str(self.toPlainText()) or
                not self.scene().controller.update_current_tag(str(self.toPlainText()))):
                self.reset()
            self.updatingTag = False
            event.ignore()
            self.clearFocus()
            self.hide()
            return
        prevWidth = self.boundingRect().width()
        qt_super(QGraphicsVersionTextItem, self).keyPressEvent(event)
        if (len(str(self.toPlainText())) > 0):
            self.setTextWidth(-1)
        if (len(str(self.toPlainText())) > 2):
            self.parent.updateWidthFromLabel(self.boundingRect().width()-prevWidth)
        self.updatePos()
        

    def focusOutEvent(self, event):
        """ focusOutEvent(event: QEvent) -> None
        Update the tag if the text has changed

        """
        qt_super(QGraphicsVersionTextItem, self).focusOutEvent(event)
        if not self.updatingTag and QtCore.QString.compare(self.label, self.toPlainText()) != 0:
            self.updatingTag = True
            if (self.label == str(self.toPlainText()) or 
                not self.scene().controller.update_current_tag(str(self.toPlainText()))):
                self.reset()
            self.updatingTag = False


##############################################################################
# QGraphicsVersionItem


class QGraphicsVersionItem(QGraphicsItemInterface, QtGui.QGraphicsEllipseItem):
    """
    QGraphicsVersionItem is the version shape holding version id and
    label
    
    """
    def __init__(self, parent=None, scene=None):
        """ QGraphicsVersionItem(parent: QGraphicsItem, scene: QGraphicsScene)
                                -> QGraphicsVersionItem
        Create the shape, initialize its pen and brush accordingly
        
        """
        QtGui.QGraphicsEllipseItem.__init__(self, parent, scene)
        self.setZValue(1)
        self.setAcceptDrops(True)
        self.versionPen = CurrentTheme.VERSION_PEN
        self.versionLabelPen = CurrentTheme.VERSION_LABEL_PEN
        self.versionBrush = CurrentTheme.VERSION_USER_BRUSH
        self.setFlags(QtGui.QGraphicsItem.ItemIsSelectable)
        self.id = -1
        self.label = ''
        self.descriptionLabel = ''
        self.dragging = False
        self.ghosted = False
        self.createActions()

        # self.rank is a positive number that determines the
        # saturation of the node. Two version nodes might have the
        # same rank if they were created by different users
        # self.max_rank is the maximum rank for that version class
        self.rank = -1
        self.max_rank = -1
        
        # Editable text item that remains hidden unless the version is selected
        self.text = QGraphicsVersionTextItem(self)
        self.text.hide()

        # Need a timer to start a drag to avoid stalls on QGraphicsView
        self.dragTimer = QtCore.QTimer()
        self.dragTimer.setSingleShot(True)
        self.dragTimer.connect(self.dragTimer,
                               QtCore.SIGNAL('timeout()'),
                               self.startDrag)

        self.dragPos = QtCore.QPoint()

    def setGhosted(self, ghosted=True):
        """ setGhosted(ghosted: True) -> None
        Set this version to be ghosted or not
        
        """
        if self.ghosted <> ghosted:
            self.ghosted = ghosted
            if ghosted:
                self.versionPen = CurrentTheme.GHOSTED_VERSION_PEN
                self.versionLabelPen = CurrentTheme.GHOSTED_VERSION_LABEL_PEN
                self.versionBrush = CurrentTheme.GHOSTED_VERSION_USER_BRUSH
            else:
                self.versionPen = CurrentTheme.VERSION_PEN
                self.versionLabelPen = CurrentTheme.VERSION_LABEL_PEN
                self.versionBrush = CurrentTheme.VERSION_USER_BRUSH

    def update_color(self, isThisUs, new_rank, new_max_rank, new_ghosted):
        """ update_color(isThisUs: bool,
                         new_rank, new_max_rank: int) -> None

        If necessary, update the colors of this version node based on
        who owns the node and new ranks

        NOTE: if username changes during execution, this might break.
        """
        if (new_rank == self.rank and new_max_rank == self.max_rank and
            new_ghosted == self.ghosted):
            # nothing changed
            return
        self.setGhosted(new_ghosted)
        self.rank = new_rank
        self.max_rank = new_max_rank
        if not self.ghosted:
            if isThisUs:
                brush = CurrentTheme.VERSION_USER_BRUSH
            else:
                brush = CurrentTheme.VERSION_OTHER_BRUSH
            sat = float(new_rank+1) / new_max_rank
            (h, s, v, a) = brush.color().getHsvF()
            newHsv = (h, s*sat, v+(1.0-v)*(1-sat), a)
            self.versionBrush = QtGui.QBrush(QtGui.QColor.fromHsvF(*newHsv))
                
    def setSaturation(self, isThisUser, sat):
        """ setSaturation(isThisUser: bool, sat: float) -> None        
        Set the color of this version depending on whose is the user
        and its saturation
        
        """
        if not self.ghosted:
            if isThisUser:
                brush = CurrentTheme.VERSION_USER_BRUSH
            else:
                brush = CurrentTheme.VERSION_OTHER_BRUSH

            (h, s, v, a) = brush.color().getHsvF()
            newHsv = (h, s*sat, v+(1.0-v)*(1-sat), a)
            self.versionBrush = QtGui.QBrush(QtGui.QColor.fromHsvF(*newHsv))
    
    def updateWidthFromLabel(self, dx):
        """ updateWidthFromLabel(dx: int) -> None
        Change the width of the ellipse based on a temporary change in the label

        """
        prevWidth = self.rect().width()
        r = self.rect()
        r.setX(r.x()-dx/2.0)
        r.setWidth(prevWidth+dx)
        self.setRect(r)
        self.update()

    def setupVersion(self, node, action, tag, description):
        """ setupPort(node: DotNode,
                      action: DBAction,
                      tag: DBTag) -> None
        Update the version dimensions and id
        
        """
        # Lauro:
        # what was this hacking??? the coordinates inside
        # the input "node" should come to this point ready. This is
        # not the point to do layout calculations (e.g. -node.p.y/2)

        # Carlos:
        # This is not layout as much as dealing with the way Qt
        # specifies rectangles. Besides, moving this back here reduces
        # code duplication, and allows customized behavior for
        # subclasses.

        rect = QtCore.QRectF(node.p.x-node.width/2.0,
                             node.p.y-node.height/2.0,
                             node.width,
                             node.height)
        validLabel = True
        if tag is None:
            label = ''
            validLabel=False
        else:
            label = tag.name

        self.id = node.id
        self.label = label
        if description is None:
            self.descriptionLabel = ''
        else:
            self.descriptionLabel = description
        if validLabel:
            textToDraw=self.label
        else:
            textToDraw=self.descriptionLabel
        self.text.changed(node.p.x, node.p.y, textToDraw, validLabel)
        self.setRect(rect)

    def boundingRect(self):
        """ boundingRect() -> QRectF
        Add a padded space to avoid un-updated area
        """
        return self.rect().adjusted(-2, -2, 2, 2)

    def paint(self, painter, option, widget=None):
        """ paint(painter: QPainter, option: QStyleOptionGraphicsItem,
                  widget: QWidget) -> None
        Peform actual painting of the version shape
        
        """
        if self.isSelected():
            painter.setPen(CurrentTheme.VERSION_SELECTED_PEN)
        else:
            painter.setPen(self.versionPen)
        painter.setBrush(self.versionBrush)
        painter.drawEllipse(self.rect())
        
        # Only draw text if editable text item is not present
        if not self.text.isVisible():
            if self.isSelected() and not self.ghosted:
                painter.setPen(CurrentTheme.VERSION_LABEL_SELECTED_PEN)
            else:
                painter.setPen(self.versionLabelPen)

            if self.label == '' and self.descriptionLabel != '':
                # Draw description text if available
                painter.setFont(CurrentTheme.VERSION_DESCRIPTION_FONT)
                painter.drawText(self.rect(), QtCore.Qt.AlignCenter, self.descriptionLabel)
            else:
                painter.setFont(CurrentTheme.VERSION_FONT)
                painter.drawText(self.rect(), QtCore.Qt.AlignCenter, self.label)

    def itemChange(self, change, value):
        """ itemChange(change: GraphicsItemChange, value: QVariant) -> QVariant
        # Do not allow links to be selected with version
        
        """
        if (change==QtGui.QGraphicsItem.ItemSelectedChange and not value.toBool()):
            self.text.hide()
        if ((change==QtGui.QGraphicsItem.ItemSelectedChange and value.toBool()) or
            (change==QtGui.QGraphicsItem.ItemSelectedChange and
             ((not value.toBool()) and
              len(self.scene().selectedItems()) == 1))):
            selectedItems = self.scene().selectedItems()
            selectedId = -1
            selectByClick = not self.scene().multiSelecting
            if value.toBool():
                for item in selectedItems:
                    if type(item)==QGraphicsLinkItem:
                        item.setSelected(False)
                        item.update()
                selectedItems = self.scene().selectedItems()
                if len(selectedItems)==0:
                    selectedId = self.id
            elif len(selectedItems)==2:
                if selectedItems[0]==self:
                    selectedId = selectedItems[1].id
                else:
                    selectedId = selectedItems[0].id
            selectByClick = self.scene().mouseGrabberItem() == self
            if not selectByClick:
                for item in self.scene().items():
                    if type(item)==QGraphicsRubberBandItem:
                        selectByClick = True
                        break
            # Update the selected items list to include only versions and 
            # check if two versions selected
            selectedVersions = [item for item in 
                                self.scene().selectedItems() 
                                if type(item) == QGraphicsVersionItem]
            # If adding a version, the ids are self and other selected version
            if (len(selectedVersions) == 1 and value.toBool()): 
                self.scene().emit(QtCore.SIGNAL('twoVersionsSelected(int,int)'),
                                  selectedVersions[0].id, self.id)
            # If deleting a version, the ids are the two selected versions that
            # are not self
            if (len(selectedVersions) == 3 and not value.toBool()):
                if selectedVersions[0] == self:
                    self.scene().emit(QtCore.SIGNAL(
                            'twoVersionsSelected(int,int)'),
                                      selectedVersions[1].id, 
                                      selectedVersions[2].id)
                elif selectedVersions[1] == self:
                    self.scene().emit(QtCore.SIGNAL(
                            'twoVersionsSelected(int,int)'),
                                      selectedVersions[0].id, 
                                      selectedVersions[2].id)
                else:
                    self.scene().emit(QtCore.SIGNAL(
                            'twoVersionsSelected(int,int)'),
                                      selectedVersions[0].id, 
                                      selectedVersions[1].id)

        return QtGui.QGraphicsItem.itemChange(self, change, value)    

    def mousePressEvent(self, event):
        """ mousePressEvent(event: QMouseEvent) -> None
        Start dragging a version to someplaces...
        
        """
        if event.button()==QtCore.Qt.LeftButton:
            self.dragging = True
            self.dragPos = QtCore.QPoint(event.screenPos())
            self.scene().emit(QtCore.SIGNAL('versionSelected(int, bool)'),
                              self.id, True)
        return QtGui.QGraphicsEllipseItem.mousePressEvent(self, event)
        
    def mouseMoveEvent(self, event):
        """ mouseMoveEvent(event: QMouseEvent) -> None        
        Now set the timer preparing for dragging. Must use a timer in
        junction with QDrag in order to avoid problem updates stall of
        QGraphicsView, especially on Linux
        
        """
        if (self.dragging and
            (event.screenPos()-self.dragPos).manhattanLength()>2):
            self.dragging = False
            #the timer has undesirable effects on Windows
            if systemType not in ['Windows', 'Microsoft']:
                self.dragTimer.start(1)
            else:
                self.startDrag()
        QtGui.QGraphicsEllipseItem.mouseMoveEvent(self, event)
        # super(QGraphicsVersionItem, self).mouseMoveEvent(event)

    def startDrag(self):
        """ startDrag() -> None
        Start the drag of QDrag
        
        """
        data = QtCore.QMimeData()
        data.versionId = self.id
        data.controller = self.scene().controller
        drag = QtGui.QDrag(self.scene().views()[0])
        drag.setMimeData(data)
        drag.setPixmap(CurrentTheme.VERSION_DRAG_PIXMAP)
        drag.start()

    def mouseReleaseEvent(self, event):
        """ mouseReleaseEvent(event: QMouseEvent) -> None
        Cancel the drag
        
        """
        self.dragging = False
        qt_super(QGraphicsVersionItem, self).mouseReleaseEvent(event)
        if self.id != 0:
            self.text.show()

    def dragEnterEvent(self, event):
        """ dragEnterEvent(event: QDragEnterEvent) -> None
        Capture version-to-version drag-and-drop
        
        """
        data = event.mimeData()
        if (hasattr(data, 'versionId') and
            hasattr(data, 'controller') and
            data.versionId!=self.id):
            event.accept()
        else:
            event.ignore()

    def dropEvent(self, event):
        data = event.mimeData()
        if (hasattr(data, 'versionId') and hasattr(data, 'controller') and
            data.controller==self.scene().controller):
            event.accept()
            visDiff = QVisualDiff(self.scene().controller.vistrail,
                                  data.versionId,
                                  self.id,
                                  self.scene().controller,
                                  self.scene().views()[0])
            visDiff.show()
        else:
            event.ignore()  

    def perform_analogy(self):
        sender = self.scene().sender()
        analogy_name = str(sender.text())
        selectedItems = self.scene().selectedItems()
        controller = self.scene().controller
        for item in selectedItems:
            controller.perform_analogy(analogy_name, item.id)

    def contextMenuEvent(self, event):
        """contextMenuEvent(event: QGraphicsSceneContextMenuEvent) -> None
        Captures context menu event.

        """
        #menu.addAction(self.addToBookmarksAct)
        controller = self.scene().controller
        if len(controller.analogy) > 0:
            menu = QtGui.QMenu()
            analogies = QtGui.QMenu("Perform analogy...")
            for title in sorted(controller.analogy.keys()):
                act = QtGui.QAction(title, self.scene())
                analogies.addAction(act)
                QtCore.QObject.connect(act,
                                       QtCore.SIGNAL("triggered()"),
                                       self.perform_analogy)
            menu.addMenu(analogies)
            menu.exec_(event.screenPos())

    def createActions(self):
        """ createActions() -> None
        Create actions related to context menu 

        """
        self.addToBookmarksAct = QtGui.QAction("Add To Bookmarks", self.scene())
        self.addToBookmarksAct.setStatusTip("Add this pipeline to bookmarks")

class QVersionTreeScene(QInteractiveGraphicsScene):
    """
    QVersionTree inherits from QInteractiveGraphicsScene to keep track
    of the version scenes, i.e. versions, connections, etc.
    
    """

    def __init__(self, parent=None):
        """ QVersionTree(parent: QWidget) -> QVersionTree
        Initialize the graphics scene with no shapes
        
        """
        QInteractiveGraphicsScene.__init__(self, parent)
        self.setBackgroundBrush(CurrentTheme.VERSION_TREE_BACKGROUND_BRUSH)
        self.setSceneRect(QtCore.QRectF(-5000, -5000, 10000, 10000))
        self.versions = {}  # id -> version gui object
        self.edges = {}     # (sourceVersion, targetVersion) -> edge gui object
        self.controller = None
        self.fullGraph = None
        self.timer = QtCore.QBasicTimer()
        self.animation_step = 1
        self.num_animation_steps = 10
   
    def addVersion(self, node, action, tag, description):
        """ addModule(node, action: DBAction, tag: DBTag) -> None
        Add a module to the scene.
        
        """
        versionShape = QGraphicsVersionItem(None)
        versionShape.setupVersion(node, action, tag, description)
        self.addItem(versionShape)
        self.versions[node.id] = versionShape

    def removeVersion(self, v):
        """ addLink(v: integer) -> None
        Remove version from scene and mapping
        
        """
        versionShape = self.versions[v]
        self.removeItem(versionShape)
        self.versions.pop(v)

    def addLink(self, guiSource, guiTarget, expand, collapse):
        """ addLink(v1, v2: QGraphicsVersionItem) -> None
        Add a link to the scene
        
        """
        linkShape = QGraphicsLinkItem()
        linkShape.setupLink(guiSource, guiTarget, expand, collapse)
        self.addItem(linkShape)
        self.edges[(guiSource.id, guiTarget.id)] = linkShape
        
    def removeLink(self, source, target):
        """ removeLink(v1, v2: integers) -> None
        Remove link from scene and mapping
        
        """
        linkShape = self.edges[(source,target)]
        self.removeItem(linkShape)
        self.edges.pop((source, target))

    def clear(self):
        """ clear() -> None
        Clear the whole scene
        
        """
        self.versions = {}
        self.clearItems()

    def adjust_version_colors(self, controller):
        """ adjust_version_colors(controller: VistrailController) -> None
        Based on the controller to set version colors
        
        """
        currentUser = controller.vistrail.getUser()
        ranks = {}
        ourMaxRank = 0
        otherMaxRank = 0
        am = controller.vistrail.actionMap
        for nodeId in sorted(self.versions.keys()):
            if nodeId!=0:
                nodeUser = am[nodeId].user
                if nodeUser==currentUser:
                    ranks[nodeId] = ourMaxRank
                    ourMaxRank += 1
                else:
                    ranks[nodeId] = otherMaxRank
                    otherMaxRank += 1
        for (nodeId, item) in self.versions.iteritems():
            if nodeId == 0:
                item.setGhosted(True)
                continue
            nodeUser = am[nodeId].user
            if controller.search and nodeId!=0:
                ghosted = not controller.search.match(controller.vistrail, 
                                                      am[nodeId])
            else:
                ghosted = False
                
            #item.setGhosted(ghosted) # we won't set it now so we can check if
                                      # the state changed in update_color
            
            max_rank = ourMaxRank if nodeUser==currentUser else otherMaxRank
            item.update_color(nodeUser==currentUser,
                              ranks[nodeId],
                              max_rank, ghosted)

    def update_scene_single_node_change(self, controller, old_version, new_version):
        """ update_scene_single_node_change(controller: VistrailController,
        old_version, new_version: int) -> None

        Faster alternative to setup_scene when a single version is
        changed. When this is called, we know that both old_version
        and new_version don't have tags associated, so no layout
        changes happen
    
        """
        # self.setupScene(controller)

        # we need to call this every time because version ranks might
        # change
        self.adjust_version_colors(controller)

        # update version item
        v = self.versions[old_version]
        self.versions[new_version] = v
        del self.versions[old_version]
        v.id = new_version

        tm = controller.vistrail.tagMap
 
        # update link items
        dst = controller._current_terse_graph.edges_from(new_version)
        for eto, _ in dst:
            edge = self.edges[(old_version, eto)]
            edge.setupLink(self.versions[new_version],
                           self.versions[eto],
                           self.fullGraph.parent(eto) != new_version,
                           False) # We shouldn't ever need a collapse here
            self.edges[(new_version, eto)] = edge
            del self.edges[(old_version, eto)]

        src = controller._current_terse_graph.edges_to(new_version)
        for efrom, _ in src:
            edge = self.edges[(efrom, old_version)]
            edge.setupLink(self.versions[efrom],
                           self.versions[new_version],
                           self.fullGraph.parent(new_version) != efrom,
                           False) # We shouldn't ever need a collapse here
            self.edges[(efrom, new_version)] = edge
            del self.edges[(efrom, old_version)]

    def setupScene(self, controller):
        """ setupScene(controller: VistrailController) -> None
        Construct the scene to view a version tree
        
        """
        import time
        t = time.clock()

        tClearRefine = time.clock()

        # Clean the previous scene
        # self.clear()
        
        self.controller = controller

        # perform graph layout
        (tree, self.fullGraph, layout) = \
            controller.refine_graph(float(self.animation_step)/
                                    float(self.num_animation_steps))

        tClearRefine = time.clock() - tClearRefine

        # compute nodes that should be removed
        # O(n  * (hashmap query key time)) on 
        # where n is the number of current 
        # nodes in the scene
        removeNodeSet = set(i for i in self.versions
                            if not i in tree.vertices)

        # compute edges to be removed
        # O(n * (hashmap query key time)) 
        # where n is the number of current 
        # edges in the scene
        removeEdgeSet = set((s, t) for (s, t) in self.edges
                            if (s in removeNodeSet or
                                t in removeNodeSet or
                                not tree.has_edge(s, t)))

        # remove gui edges from scene
        for (v1, v2) in removeEdgeSet:
            self.removeLink(v1,v2)

        # remove gui nodes from scene
        for v in removeNodeSet:
            self.removeVersion(v)

        tCreate = time.clock()

        # loop on the nodes of the tree
        tm = controller.vistrail.tagMap
        am = controller.vistrail.actionMap
        last_n = controller.vistrail.getLastActions(controller.num_versions_always_shown)

        for node in layout.nodes.itervalues():

            # version id
            v = node.id

            # version tag
            tag = tm.get(v, None)
            action = am.get(v, None)
            description = controller.vistrail.get_description(v)

            # if the version gui object already exists...
            if self.versions.has_key(v):
                versionShape = self.versions[v]
                versionShape.setupVersion(node, action, tag, description)
            else:
                self.addVersion(node, action, tag, description)

            # set as selected
            self.versions[v].setSelected(v == controller.current_version)

        # adjust the colors
        self.adjust_version_colors(controller)

        # Add or update links
        for source in tree.vertices.iterkeys():
            if source not in self.versions:
                continue
            eFrom = tree.edges_from(source)
            for (target, aux) in eFrom:
                if target not in self.versions:
                    continue
                guiSource = self.versions[source]
                guiTarget = self.versions[target]
                sourceChildren = [to for (to, _) in 
                                  self.fullGraph.adjacency_list[source]
                                  if (to in am) and not am[to].prune]
                targetChildren = [to for (to, _) in
                                  self.fullGraph.adjacency_list[target]
                                  if (to in am) and not am[to].prune]
                expand = self.fullGraph.parent(target)!=source
                collapse = (self.fullGraph.parent(target)==source and # No in betweens
                            len(targetChildren) == 1 and # target is not a leaf or branch
                            target != controller.current_version and # target is not selected
                            target not in tm and # target has no tag
                            target not in last_n and # not one of the last n modules
                            (source in tm or # source has a tag
                             source == 0 or # source is root node
                             len(sourceChildren) > 1 or # source is branching node 
                             source == controller.current_version)) # source is selected
                if self.edges.has_key((source,target)):
                    linkShape = self.edges[(source,target)]
                    linkShape.setupLink(guiSource, guiTarget,
                                        expand, collapse)
                else:
                    #print "add link %d %d" % (source, target)
                    self.addLink(guiSource, guiTarget, 
                                 expand, collapse)

        tCreate = time.clock() - tCreate

        # Update bounding rects and fit to all view
        tUpdate = time.clock()
        if not self.controller.animate_layout:
            self.updateSceneBoundingRect()
        elif not self.timer.isActive():
            self.timer.start(0, self)
        tUpdate = time.clock() - tUpdate

        t = time.clock() - t
        # print "time in msec to setupScene total: %f  refine %f  layout %f  create %f" % (t, tClearRefine, tCreate)

    def timerEvent(self, event):
        """ timerEvent(event: QTimerEvent) -> None
        
        Start up a timer for animating tree drawing events
        """
        if event.timerId() == self.timer.timerId():
            self.animation_step += 1
            if self.animation_step >= self.num_animation_steps:
                self.animation_step = 1
                self.timer.stop()
                self.controller.animate_layout = False
            self.setupScene(self.controller)
            self.update()
        else:
            qt_super(QVersionTreeScene, self).timerEvent(event)

    def keyPressEvent(self, event):
         """ keyPressEvent(event: QKeyEvent) -> None
         Capture 'Del', 'Backspace' for pruning versions when not editing a tag
       
         """        
         selectedItems = self.selectedItems()
         versions = [item.id for item in selectedItems 
                     if type(item)==QGraphicsVersionItem
                     and not item.text.hasFocus()] 
         if (self.controller and len(versions)>0 and
             event.key() in [QtCore.Qt.Key_Backspace, QtCore.Qt.Key_Delete]):
             versions = [item.id for item in selectedItems]
             res = gui.utils.show_question("VisTrails",
                                           "Are you sure that you want to "
                                           "prune the selected version(s)?",
                                           [gui.utils.YES_BUTTON,
                                            gui.utils.NO_BUTTON],
                                           gui.utils.NO_BUTTON)
             if res == gui.utils.YES_BUTTON:
                 self.controller.prune_versions(versions)
         qt_super(QVersionTreeScene, self).keyPressEvent(event)

    def mouseReleaseEvent(self, event):
        """ mouseReleaseEvent(event: QMouseEvent) -> None
        
        """
        if len(self.selectedItems()) != 1:
            self._pipeline_scene.clear()
            self.emit(QtCore.SIGNAL('versionSelected(int, bool)'),
                      -1, True)
        qt_super(QVersionTreeScene, self).mouseReleaseEvent(event)
        
class QVersionTreeView(QInteractiveGraphicsView):
    """
    QVersionTreeView inherits from QInteractiveGraphicsView that will
    handle drawing of versions layout output from Dotty
    
    """

    def __init__(self, parent=None):
        """ QVersionTreeView(parent: QWidget) -> QVersionTreeView
        Initialize the graphics view and its properties
        
        """
        QInteractiveGraphicsView.__init__(self, parent)
        self.setWindowTitle('Version Tree')
        self.setScene(QVersionTreeScene(self))
        self.versionProp = QVersionPropOverlay(self, self.viewport())
        self.versionProp.hide()

    def selectModules(self):
        """ selectModules() -> None
        Overrides parent class to disable text items if you click on background

        """
        if self.canSelectRectangle:
            br = self.selectionBox.sceneBoundingRect()
        else:
            br = QtCore.QRectF(self.startSelectingPos,
                              self.startSelectingPos)
        items = self.scene().items(br)
        if len(items)==0 or items==[self.selectionBox]:
            for item in self.scene().selectedItems():
                if type(item) == gui.version_view.QGraphicsVersionItem:
                    item.text.clearFocus()
                    item.text.hide()
        qt_super(QVersionTreeView, self).selectModules()
        
################################################################################
