
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
""" This containing a subclassed QGraphicsView that allows
zoom/pan/select inside the graphics view and supports PIP as well

QInteractiveGraphicsScene
QInteractiveGraphicsView
QPIPGraphicsView
"""

from PyQt4 import QtCore, QtGui
from gui.theme import CurrentTheme
import core.system
import math
from gui.qt import qt_super
################################################################################

class QGraphicsItemInterface(object):
    """
    QGraphicsItem will override the default QGraphicsItem mouseRelease
    event to let it fires only 1 ItemSelectionChanged event. This is
    due to the clearSelection() in
    qgraphicsitem.cpp::QGrahpicsItem::mouseReleaseEvent
    
    """
    def mouseReleaseEvent(self, event):
        """ mouseReleaseEvent(event: QMouseEvent) -> None        
        Make sure the current item is not selected before
        clearSelection and perform the selected again
        
        """
        if ((self.flags() & QtGui.QGraphicsItem.ItemIsSelectable) and
            event.scenePos()==event.buttonDownScenePos(QtCore.Qt.LeftButton)):
            multiSelect = event.modifiers() & QtCore.Qt.ControlModifier
            if multiSelect:
                self.setSelected(not self.isSelected())
            else:
                if self.scene():
                    for item in self.scene().selectedItems():
                        if item!=self:
                            item.setSelected(False)
                self.setSelected(True)
        else:
            qt_super(QGraphicsItemInterface, self).mouseReleaseEvent(event)

class QGraphicsRubberBandItem(QtGui.QGraphicsRectItem):
    """
    QGraphicsRubberBandItem try to replace QRubberBand to have a
    unified look and feel on all platform. In the end, it mimics the
    Windows look. It is just a rectangle with a 50% transparency
    
    """
    def paint(self, painter, option, widget=None):
        """ paint(painter: QPainter, option: QStyleOptionGraphicsItem,
                  widget: QWidget) -> None
        Peform actual painting of the rubber band
        
        """
        painter.setOpacity(0.5)
        painter.fillRect(self.rect(),
                         CurrentTheme.SELECTION_BOX_BRUSH)
        painter.setOpacity(1.0)
        painter.setPen(CurrentTheme.SELECTION_BOX_PEN)
        painter.drawRect(self.rect())

class QInteractiveGraphicsScene(QtGui.QGraphicsScene):
    """
    QInteractiveGraphicsScene expands QGraphicsScene to allow panning
    freely over the view are
    
    """
    def __init__(self, parent=None):
        """ QInteractiveGraphicsScene(parent: QWidget)
                                      -> QInteractiveGraphicsScene
        Initialize the actual scene bounding rect
        """
        QtGui.QGraphicsScene.__init__(self, parent)
        self.sceneBoundingRect = QtCore.QRectF()
        self.multiSelecting = False
        
    def updateSceneBoundingRect(self):
        """ updateSceneBoundingRect() -> None        
        Compute the actual bounding rect of all shapes, then update
        the scene rect to be much wider for panning
        
        """
        self.sceneBoundingRect = QtCore.QRectF()
        for item in self.items():
            rect = item.sceneBoundingRect()
            self.sceneBoundingRect = self.sceneBoundingRect.united(rect)

        # Keep a minimum size
        minWDiff = 0
        minHDiff = 0
        min = CurrentTheme.BOUNDING_RECT_MINIMUM
        if self.sceneBoundingRect.width() < min:
            minWDiff = min-self.sceneBoundingRect.width()
        if self.sceneBoundingRect.height() < min:
            minHDiff = min-self.sceneBoundingRect.height()
        self.sceneBoundingRect.adjust(-minWDiff/2, minHDiff/2, 
                                       minWDiff/2, -minHDiff/2)

        diff = abs(self.sceneBoundingRect.width()-
                   self.sceneBoundingRect.height())
        if self.sceneBoundingRect.width()<self.sceneBoundingRect.height():
            self.sceneBoundingRect.adjust(-diff/2, 0, diff/2, 0)
        else:
            self.sceneBoundingRect.adjust(0, -diff/2, 0, diff/2)
        panRect = self.sceneBoundingRect.adjusted(
            -self.sceneBoundingRect.width()*100,
            -self.sceneBoundingRect.height()*100,
            self.sceneBoundingRect.width()*100,
            self.sceneBoundingRect.height()*100)
        if panRect.width()<1e-6 and panRect.height()<1e-6:
            panRect = QtCore.QRectF(-1000,-1000,2000,2000)
        self.setSceneRect(panRect)

        # Reset cache
        for view in self.views():
            view.resetCachedContent()


    def fitToView(self, view, recompute_bounding_rect=False):
        """ fitToView(view: QGraphicsView,
                      recompute_bounding_rect=False) -> None
        Adjust view to fit and center the whole scene. If recompute_bounding_rect is
        False, does not recompute bounds, and instead uses previous one.
        
        """
        if recompute_bounding_rect:
            self.updateSceneBoundingRect()
        view.centerOn(self.sceneBoundingRect.center())
        view.fitInView(self.sceneBoundingRect, QtCore.Qt.KeepAspectRatio)
            
    def fitToAllViews(self, recompute_bounding_rect=False):
        """ fitToAllViews(recompute_bounding_rect=False) -> None
        Adjust all views using this scene to fit and center the whole scene

        if recompute_bounding_rect is False, uses previous bounding rect.
        
        """
        if recompute_bounding_rect:
            self.updateSceneBoundingRect()
        for view in self.views():
            self.fitToView(view, False)

    def clearItems(self):
        """ clearShapes() -> None
        Remove and delete all items belonging to this scene
        
        """
        self.removeItems(self.items())

    def removeItems(self, itemList):
        """ removeItems(itemList: sequence of [QGraphicsItem]) -> None
        Remove all items in itemList
        
        """
        for item in itemList:
            if item.scene():
                self.removeItem(item)

    def saveToPDF(self, filename):
        self.updateSceneBoundingRect()
        printer = QtGui.QPrinter()
        printer.setOutputFormat(QtGui.QPrinter.PdfFormat)
        printer.setOutputFileName(filename)
        painter = QtGui.QPainter(printer)
        brush = self.backgroundBrush()
        self.setBackgroundBrush(QtGui.QBrush(QtGui.QColor(255,255,255)))
        b_rect = self.sceneBoundingRect
        self.render(painter, QtCore.QRectF(), b_rect)
        painter.end()
        self.setBackgroundBrush(brush)

class QInteractiveGraphicsView(QtGui.QGraphicsView):
    """
    QInteractiveGraphicsView is QGraphicsView with abilities to
    zoom/span with right/mid click
    
    """
    def __init__(self, parent=None):
        """ QInteractiveGraphicsView(parent: QWidget)
                                     -> QInteractiveGraphicsView
        Initialize the graphics view with interactive options
        
        """
        QtGui.QGraphicsView.__init__(self, parent)
        # FIXME: 
        # Workaround for Qt/PyQt weirdness on Ubuntu 7.04 as of 2008/05/21
        # If scroll bar is not visible, panning does not work.
        self.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setInteractive(True)
##         self.setCacheMode(QtGui.QGraphicsView.CacheBackground)
        self.setResizeAnchor(QtGui.QGraphicsView.AnchorViewCenter)
        self.setRenderHints(QtGui.QPainter.Antialiasing |
                            QtGui.QPainter.SmoothPixmapTransform)
        self.scaleMax = 2000
        self.scaleRatio = self.scaleMax/10
        self.currentScale = self.scaleMax/2
        self.startScroll = (0,0)
        self.lastPos = QtCore.QPoint(0,0)
        self.pipScene = None
        self.pipFrame = None
        self.resetButton = None
        self.selectionBox = QGraphicsRubberBandItem(None)
        self.startSelectingPos = None
        self.setProperty('captureModifiers',
                         QtCore.QVariant(1))
        self.defaultCursorState = 0
        self.setCursorState(self.defaultCursorState)
        self.canSelectBackground = True
        self.canSelectRectangle = True

    def modifiersPressed(self, modifiers):
        """ modifiersPressed(modifiers: QtCore.Qt.KeyboardModifiers) -> None
        Notification when one of the modifier keys has been pressed
        
        """
        self.validateCursorState(modifiers)
        
    def modifiersReleased(self):
        """ modifiersReleased() -> None
        Notification when one of the modifier keys has been released
        
        """
        self.validateCursorState()

    def findCursorState(self, modifiers=None):
        """ findCursorState(modifiers: QtCore.Qt.KeyboardModifiers) -> None
        Check the keyboard modifiers and return the cursor state

        """
        if not self.isActiveWindow():
            return self.defaultCursorState
        if modifiers==None:
            modifiers = QtGui.QApplication.keyboardModifiers()
        shift = modifiers & QtCore.Qt.ShiftModifier
        alt = modifiers & QtCore.Qt.AltModifier
        meta = modifiers & QtCore.Qt.MetaModifier or (alt and shift)
        ctrl = modifiers & QtCore.Qt.ControlModifier
        if shift and (not alt) and (not ctrl) and (not meta):
            return 1
        elif meta and (not ctrl):
            return 2
        else:
            return self.defaultCursorState

    def validateCursorState(self, modifiers=None):
        """ validateCursorState(modifiers: QtCore.Qt.KeyboardModifiers) -> None
        Check the keyboard modifiers to change the cursor shape correspondingly
        
        """        
        self.setCursorState(self.findCursorState(modifiers))

    def enterEvent(self, event):
        """ enterEvent(event: QEnterEvent) -> None        
        Check the modifiers state when the mouse enter the
        canvas. Then update the mouse functionality appropriately
        
        """
        self.validateCursorState()
        return QtGui.QGraphicsView.enterEvent(self, event)
        # super(QInteractiveGraphicsView, self).enterEvent(event)

    def setCursorState(self, state):
        """ setCursorState(state: int) -> None        
        Update the cursor shape

        Keyword arguments:
        state - 0: selecting (default)
                1: pan
                2: zoom
                3: panning
        
        """
        if state==0:
            self.viewport().setCursor(CurrentTheme.SELECT_CURSOR)
        elif state==1:
            self.viewport().setCursor(CurrentTheme.OPEN_HAND_CURSOR)
        elif state==2:
            self.viewport().setCursor(CurrentTheme.ZOOM_CURSOR)
        elif state==3:
            self.viewport().setCursor(CurrentTheme.CLOSE_HAND_CURSOR)
        
    def setDefaultCursorState(self, state):
        """ setDefaultCursorState(state: int) -> None
        Set the default cursor state when no modifier key is pressed
        
        """
        self.defaultCursorState = state
        self.validateCursorState()

    def translateButton(self, event):
        """ translateButton(event: QInputEvent) -> None
        Translate mouse button and modifiers into a virtual mouse button
        
        """
        if event.buttons() & QtCore.Qt.LeftButton:
            state = self.findCursorState(event.modifiers())
            state2Button = {0: QtCore.Qt.LeftButton,
                            1: QtCore.Qt.MidButton,
                            2: QtCore.Qt.RightButton}
            if state2Button.has_key(state):
                return state2Button[state]
        return event.buttons()

    def mousePressEvent(self, e):
        """ mousePressEvent(e: QMouseEvent) -> None        
        Handle mouse click event, use Qt rubber band for left-click
        selection and prepare for zoom/pan on right/mid click
        
        """
        scenePos = self.mapToScene(e.pos())
        item = self.scene().itemAt(scenePos)
        buttons = self.translateButton(e)
        if buttons == QtCore.Qt.LeftButton:
            if item==None:
                if self.scene():
                    self.scene().multiSelecting = True
                    self.scene().addItem(self.selectionBox)
                    self.selectionBox.setZValue(1000)
                    
                    self.startSelectingPos = self.mapToScene(e.pos())
                    rect = QtCore.QRectF(self.startSelectingPos,
                                         QtCore.QSizeF(0,0))
                    self.selectionBox.setRect(rect)
                    self.selectionBox.setVisible(self.canSelectRectangle)
            else:
                QtGui.QGraphicsView.mousePressEvent(self, e)
                # super(QInteractiveGraphicsView, self).mousePressEvent(e)
        else:
            if buttons & QtCore.Qt.RightButton:
                self.setCursorState(2)
                self.computeScale()
            elif buttons & QtCore.Qt.MidButton:
                self.setCursorState(3)
                self.startScroll = (self.horizontalScrollBar().value(),
                                    self.verticalScrollBar().value())
            self.lastPos = QtCore.QPoint(QtGui.QCursor.pos())
            self.setDragMode(QtGui.QGraphicsView.NoDrag)

    def mouseMoveEvent(self, e):
        """ mouseMoveEvent(e: QMouseEvent) -> None        
        Handle right click (zoom) and mid click (pan). This function
        uses QCursor globalPos instead of e.globalX() and e.globalY()
        because of their flaky values during transformation
        
        """
        self.setUpdatesEnabled(False)
        buttons = self.translateButton(e)
        if buttons == QtCore.Qt.LeftButton:
            if self.startSelectingPos:
                dis = self.mapToScene(e.pos())-self.startSelectingPos
                rect = QtCore.QRectF(self.startSelectingPos,
                                 QtCore.QSizeF(dis.x(), dis.y()))
                self.selectionBox.prepareGeometryChange()
                self.selectionBox.setRect(rect)
                self.selectModules()
            else:
                QtGui.QGraphicsView.mouseMoveEvent(self, e)
                # super(QInteractiveGraphicsView, self).mousePressEvent(e)
        elif self.lastPos:
            if buttons == QtCore.Qt.RightButton:
                globalPos = QtGui.QCursor.pos()
            
                # Set up new scale based on pixels moved
                newScale = self.currentScale + (globalPos.y() - self.lastPos.y()) + \
                    (globalPos.x() - self.lastPos.x()) 

                # Clamp the scale
                if newScale<0: newScale = 0
                if newScale>self.scaleMax: newScale = self.scaleMax

                # Update the scale and transformation matrix
                self.currentScale = newScale
                self.updateMatrix()

                # Need to update last position
                self.lastPos = QtCore.QPoint(globalPos)
                
            elif buttons == QtCore.Qt.MidButton:
                globalPos = QtGui.QCursor.pos()
                
               # Just need to pan the scroll bar
                self.horizontalScrollBar().setValue(self.startScroll[0] -
                                                    globalPos.x() +
                                                    self.lastPos.x())
                self.verticalScrollBar().setValue(self.startScroll[1] -
                                                  globalPos.y() +
                                                  self.lastPos.y())
        else:
            self.validateCursorState(e.modifiers())
        self.setUpdatesEnabled(True)

    def mouseReleaseEvent(self, e):
        """ mouseReleaseEvent(self, e: QMouseEvent) -> None
        Mark box selecting as off
        """
        if self.startSelectingPos:
            self.selectModules()
            self.startSelectingPos = None
            self.selectionBox.setVisible(False)
            self.scene().removeItem(self.selectionBox)
            self.scene().multiSelecting = False
        self.lastPos = None
        self.validateCursorState(e.modifiers())
        self.setUpdatesEnabled(True)
        QtGui.QGraphicsView.mouseReleaseEvent(self, e)
        # super(QInteractiveGraphicsView, self).mouseReleaseEvent(e)

    def mouseDoubleClickEvent(self, e):
        """ mouseDoubleClickEvent(self, e: QMouseEvent) -> None
        Try to avoid unselect if double-click on the background        
        """
        if not self.canSelectBackground:
            return
        else:
            qt_super(QInteractiveGraphicsView, self).mouseDoubleClickEvent(e)

    def selectModules(self):
        """ selectModules() -> None
        Select all modules inside the self.selectionBox
        
        """
        if self.canSelectRectangle:
            br = self.selectionBox.sceneBoundingRect()
        else:
            br = QtCore.QRectF(self.startSelectingPos,
                              self.startSelectingPos)
        if not self.canSelectBackground:
            items = self.scene().items(br) 
            if len(items)==0 or items==[self.selectionBox]:
                return
        path = QtGui.QPainterPath()
        path.addRect(br)
        self.scene().setSelectionArea(path)
        
    def updateMatrix(self):
        """ updateMatrix() -> None
        Update the view matrix with the current scale
        
        """        
        matrix = QtGui.QMatrix()
        power = float(self.currentScale-self.scaleMax/2)/self.scaleRatio
        scale = pow(2.0, power)
        matrix.scale(scale, scale)
        self.setMatrix(matrix)

    def computeScale(self):
        """ computeScale() -> None
        Compute the current scale based on the view matrix
        
        """
        self.currentScale = (math.log(self.matrix().m11(), 2.0)*
                             self.scaleRatio + self.scaleMax/2)

    def setPIPScene(self, scene):
        """ setPIPScene(scene: QGraphicsScene) -> None        
        Set the Picture-In-Picture scene fo the current GraphicsView
        to 'scene'
        
        """
        self.pipScene = scene

    def setPIPEnabled(self, enabled=True):
        """ setPIPEnabled(enabled: boolean) -> None        
        Enable/Disable PIP view
        
        """
        if self.pipScene:
            if enabled:
                if self.pipFrame==None:
                    self.pipFrame = QPIPGraphicsView(self)
                    self.pipFrame.graphicsView.setScene(self.pipScene)
                    self.pipFrame.move(self.width(), 0)
                self.pipFrame.show()
                self.pipFrame.updateGeometry()
            elif self.pipFrame!=None:
                self.pipFrame.hide()

    def setQueryEnabled(self, enabled=True):
        """ setQueryEnabled(enabled: bool) -> None
        Enable/Disable the query reset button

        """
        if enabled:
            if not self.resetButton:
                self.resetButton = QResetQueryButton(self)
                self.connect(self.resetButton,
                             QtCore.SIGNAL('resetQuery()'),
                             self.resetQuery)
            self.resetButton.show()
            self.resetButton.updateGeometry()
        else:
            if self.resetButton:
                self.resetButton.hide()

    def resetQuery(self):
        """ resetQuery() -> None
        pass the signal along
        
        """
        self.emit(QtCore.SIGNAL('resetQuery()'))

    def resizeEvent(self, event):
        """ resizeEvent(event: QResizeEvent) -> None
        Make sure the pip frame is inside the graphics view
        """
        if self.pipFrame:
            self.pipFrame.updateGeometry()
        if self.resetButton:
            self.resetButton.updateGeometry()
        return QtGui.QGraphicsView.resizeEvent(self, event)
        # super(QInteractiveGraphicsView, self).resizeEvent(event)

    def keyPressEvent(self, event):
        """ keyPressEvent(event: QKeyEvent) -> None
        Handle general key-bindings, e.g. 'R' for Reset
        """
        # Reset the view when 'R' is pressed
        if event.key()==QtCore.Qt.Key_R \
                and event.modifiers() in [QtCore.Qt.ControlModifier]:
            self.scene().fitToView(self, True)
        else:
            QtGui.QGraphicsView.keyPressEvent(self, event)
            # super(QInteractiveGraphicsView, self).keyPressEvent(event)

    def sizeHint(self):
        """ sizeHint(self) -> QSize
        Return recommended size of the widget
        
        """
        return QtCore.QSize(512, 512)

    # Workaround for border aliasing on OSX
    # However, it breaks things on Linux, because it
    # makes zooming _extremely_ slow, so we check it
    # before we run.
    if core.system.systemType == 'Darwin':
        def setScene(self, scene):
            """ setScene(scene: QGraphicsScene) -> None
            Make sure the viewport background brush the same as the scene
            one. This is only neccessary on the Mac to work around Qt/Mac
            bug. We can remove this if any future release of Qt fixes
            this.

            """
            QtGui.QGraphicsView.setScene(self, scene)
            if self.scene():
                palette = QtGui.QPalette(self.viewport().palette())
                palette.setBrush(QtGui.QPalette.Base, 
                                 self.scene().backgroundBrush())
                self.viewport().setPalette(palette)

class QPIPGraphicsView(QtGui.QWidget):
    """
    QPIPGraphicsView is a tool window contain a
    QInteractiveGraphicsView for PIP display

    """
    def __init__(self, parent=None):
        """ QPIPGraphicsView(parent: QWidget) -> QPIPGraphicsView
        Initialize a layout with some margin and a central widget
        """
        QtGui.QWidget.__init__(self, parent, QtCore.Qt.FramelessWindowHint)
        self.setMouseTracking(True)
        self.setAutoFillBackground(True)
        self.palette().setColor(QtGui.QPalette.Base,
                                CurrentTheme.PIP_FRAME_COLOR)
        self.setLayout(QtGui.QHBoxLayout(self))
        self.layout().setMargin(CurrentTheme.PIP_OUT_FRAME_WIDTH)
        self.graphicsView = QInteractiveGraphicsView()
        self.layout().addWidget(self.graphicsView)
        self.firstShow = True
        self.anchorCorner = [QtCore.Qt.AlignRight, QtCore.Qt.AlignTop]

    def sizeHint(self):
        """ sizeHint(self) -> QSize
        Return recommended size of the widget
        
        """        
        return QtCore.QSize(*CurrentTheme.PIP_DEFAULT_SIZE)
        
    def mousePressEvent(self, e):
        """ mousePressEvent(e: QMouseEvent) -> None
        Capture mouse press event on the frame to move the widget
        
        """
        if e.buttons() & QtCore.Qt.LeftButton:
            self.resizing = False
            self.lastPos = QtCore.QPoint(e.globalX(), e.globalY())
            gvRect = self.graphicsView.geometry()
            if ((e.x()<gvRect.left() or e.x()>gvRect.right()) and
                (e.y()<gvRect.top() or e.y()>gvRect.bottom())):
                self.direction = [1,1]
                if e.x()<gvRect.left():
                    self.direction[0] = -1
                if e.y()<gvRect.top():
                    self.direction[1] = -1
                self.resizing = True

    def mouseMoveEvent(self, e):
        """ mouseMoveEvent(e: QMouseEvent) -> None
        Move widget as the mouse moving
        
        """
        if e.buttons() & QtCore.Qt.LeftButton:
            (dx, dy) = (e.globalX()-self.lastPos.x(),
                        e.globalY()-self.lastPos.y())
            parentGeometry = self.parent().geometry()
            newGeometry = QtCore.QRect(self.geometry())
            if self.resizing:
                if self.direction[0]==1:
                    if newGeometry.right()+dx>parentGeometry.width()-1:
                        dx = parentGeometry.width()-1-newGeometry.right()
                    newGeometry.adjust(0, 0, dx, 0)
                else:
                    if newGeometry.left()+dx<0:
                        dx = -newGeometry.left()
                    newGeometry.adjust(dx, 0, 0, 0)
                    
                if self.direction[1]==1:
                    if newGeometry.bottom()+dy>parentGeometry.height()-1:
                        dy = parentGeometry.height()-1-newGeometry.bottom()
                    newGeometry.adjust(0, 0, 0, dy)
                else:
                    if newGeometry.top()+dy<0:
                        dy = -newGeometry.top()
                    newGeometry.adjust(0, dy, 0, 0)
            else:
                newGeometry.translate(dx, dy)
            if (newGeometry.left()>0 and
                newGeometry.right()<parentGeometry.width()-1):
                self.anchorCorner[0] = None
            if (newGeometry.top()>0 and
                newGeometry.bottom()<parentGeometry.height()-1):
                self.anchorCorner[1] = None
            self.updateGeometry(newGeometry)
            self.lastPos = QtCore.QPoint(e.globalX(), e.globalY())

    def updateGeometry(self, newGeometry=None):
        """ updateGeometry() -> None
        Make sure the widget is inside the parent graphics view
        
        """
        parentGeometry = self.parent().geometry()
        if newGeometry==None:
            newGeometry = QtCore.QRect(self.geometry())
            
        if self.anchorCorner[0]==None:
            if newGeometry.left()<0:
                newGeometry.moveLeft(0)
            if newGeometry.right()>=parentGeometry.width():
                newGeometry.moveRight(parentGeometry.width()-1)
            if newGeometry.left()==0:
                self.anchorCorner[0] = QtCore.Qt.AlignLeft
            if newGeometry.right()==parentGeometry.width()-1:
                self.anchorCorner[0] = QtCore.Qt.AlignRight
        else:
            if self.anchorCorner[0]==QtCore.Qt.AlignLeft:
                newGeometry.moveLeft(0)
            if self.anchorCorner[0]==QtCore.Qt.AlignRight:
                newGeometry.moveRight(parentGeometry.width()-1)
                
        if self.anchorCorner[1]==None:
            if newGeometry.top()<0:
                newGeometry.moveTop(0)
            if newGeometry.bottom()>=parentGeometry.height():
                newGeometry.moveBottom(parentGeometry.height()-1)
            if newGeometry.top()==0:
                self.anchorCorner[1] = QtCore.Qt.AlignTop
            if newGeometry.bottom()==parentGeometry.height()-1:
                self.anchorCorner[1] = QtCore.Qt.AlignBottom
        else:
            if self.anchorCorner[1]==QtCore.Qt.AlignTop:
                newGeometry.moveTop(0)
            if self.anchorCorner[1]==QtCore.Qt.AlignBottom:
                newGeometry.moveBottom(parentGeometry.height()-1)
                
        self.setGeometry(newGeometry)

    def showEvent(self, event):
        """ showEvent(event: QShowEvent) -> None
        Fit the scene to view for the show event only
        
        """
        if self.firstShow:
            self.firstShow = False
            self.graphicsView.scene().fitToView(self.graphicsView, True)
        return QtGui.QWidget.showEvent(self, event)        
        # super(QPIPGraphicsView, self).showEvent(event)

    def enterEvent(self, event):
        """ enterEvent(event: QEnterEvent) -> None        
        Show a larger frame when the mouse enter to facilitate
        resizing
        
        """
        self.layout().setMargin(CurrentTheme.PIP_IN_FRAME_WIDTH)

    def leaveEvent(self, event):
        """ leaveEvent(event: QLeaveEvent) -> None        
        Show a smaller frame when the mouse exit the widget to get
        more view
        
        """
        self.layout().setMargin(CurrentTheme.PIP_OUT_FRAME_WIDTH)


class QResetQueryButton(QtGui.QLabel):
    """
    
    """
    def __init__(self, parent=None):
        QtGui.QLabel.__init__(self, parent)

        self.setText('Reset Query')
        self.setFrameStyle(QtGui.QFrame.StyledPanel)
        self.setFrameShadow(QtGui.QFrame.Raised)
        self.marginPad = 10

    def mousePressEvent(self, e):
        """ mousePressEvent(e: QMouseEvent) -> None
        Capture mouse press event on the frame to move the widget
        
        """
        if e.buttons() & QtCore.Qt.LeftButton:
            self.setFrameShadow(QtGui.QFrame.Sunken)
    
    def mouseReleaseEvent(self, e):
        self.setFrameShadow(QtGui.QFrame.Raised)
        self.emit(QtCore.SIGNAL('resetQuery()'))

    def updateGeometry(self):
        parentGeometry = self.parent().geometry()
        self.move(self.marginPad, 
                  parentGeometry.height()-self.height()-self.marginPad)


