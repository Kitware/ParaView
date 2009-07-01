
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
"""
HistogramMetrics
QHistogramItem
QHistogramXAxis
QHistogramLabeling
QTimeHistogramLabeling
QHistogramView
QHistogramWidget
QHistogramInteraction (obsolete)
QTimeHistogramView
"""


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
from PyQt4 import QtGui, QtCore
from gui.utils import getBuilderWindow
from plugin.pgui.statistics.datasource import *
import math,time,datetime


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class HistogramMetrics(object):
    """ HistogramMetrics is the interface between the data source and the
    HistogramView; e.g., all paremeters like bounds and zoom-params are stored.
    Information about the histogram item dimensions are also provides.
    
    This class is compatible to DataSource and ScrollableDataSource objects.
    """

    def __init__(self,dataSource):
        """ __init__(dataSource:DataSource) -> None
        """
        self.viewWidth=0
        self.setDataSource(dataSource)
        
    def setDataSource(self,dataSource):
        """ setDataSource(dataSource:DataSource) -> None
        Set the current data source. Read bounds and page size, if it is
        a 'scrollable source'.
        """
        self.dataSource=dataSource
        if issubclass(self.dataSource.__class__,ScrollableDataSource) \
        and not self.dataSource.isEmpty():
            self.dataLeftBound=self.dataSource.defaultLeftBound()
            self.dataRightBound=self.dataSource.defaultRightBound()
            self.dataPageSize=self.dataSource.defaultPageSize()
        else:
            self.dataLeftBound=None
            self.dataRightBound=None
            self.dataPageSize=None
        self.viewPageResolution=20
        self.viewResolution=None
        self.update()
        
    def setViewWidth(self,width):
        """ setViewWidth(width:float) -> None
        Set the variable and update.
        """
        self.viewWidth=width
        self.update()
        
    def setViewPageResolution(self,resolution):
        """ setViewWidth(width:float) -> None
        Set the variable and update.
        """
        self.viewPageResolution=resolution
        self.update()
    
    def setViewResolution(self,resolution):
        """ setViewWidth(width:float) -> None
        Set the viewPageResolution variable and update; note that viewResolution
        is set during update.
        """
        self.viewPageResolution=resolution*self.dataPageSize/self.dataRange
        self.update()
        
    def setDataPageSize(self,dataPageSize):
        """ setDataPageSize(dataPageSize:float) -> None
        Set the variable and update.
        """
        self.dataPageSize=dataPageSize
        self.update()

    def update(self):
        """ update() -> None
        Depending on the data source and the variables
            dataLeftBound,dataLeftBound,dataPageSize,viewPageResolution,
        update the following ones:
            dataRange,viewResoltion,dataBinWidth,viewBinWidth.
        """
        if not self.dataSource.isEmpty():
            self.dataRange=self.dataRightBound-self.dataLeftBound
            self.viewResolution=int(round(self.viewPageResolution*self.dataRange/self.dataPageSize))
            self.dataBinWidth=self.dataRange/self.viewResolution
        else:
            self.dataBinWidth=0
            self.dataRange=0
            self.viewResolution=self.viewPageResolution
        self.viewBinWidth=self.viewWidth/self.viewResolution
        
    def binOffsetX(self,index):
        """ binOffsetX(index:int) -> float
        Return the x offset for the bin with index 'index'.
        """
        return index*self.viewBinWidth


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QHistogramItem(QtGui.QAbstractGraphicsShapeItem):
    """ QHistogramItem is a rectangle that is drawn for each histogram bin.
    """

    def __init__(self,x,y,width,height,parent=None):
        """ __init__(x:float,y:float,width:float,height:float,parent:QWidget) -> None
        Define the rectangle through (x,y,width,height), brush, and pen.
        """
        QtGui.QAbstractGraphicsShapeItem.__init__(self,parent)
        self.rect=QtCore.QRectF(x,y,width,height)
        self.setPen(QtGui.QPen(QtCore.Qt.black))
        brush=QtGui.QBrush()
        brush.setColor(QtGui.QColor(131,155,199))
        brush.setStyle(QtCore.Qt.SolidPattern)
        self.setBrush(brush)
        self.setZValue(0)

    def paint(self,painter,option,widget=None):
        """ paint(painter:QPainter,options:QStyleOptionGraphicsItem,widget:QWidget) -> None
        Self-explanatory.
        """
        painter.setPen(self.pen())
        painter.setBrush(self.brush())
        painter.drawRect(self.rect)

    def boundingRect(self):
        """ boundingRect() -> QRectF
        Self-explanatory.
        """
        return self.rect


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QHistogramXAxis(QtGui.QGraphicsRectItem):
    """ QHistogramXAxis implements a simple x-axis for a histogram :D (without labels).
    """

    def __init__(self,metrics,parent=None):
        """ __init__(metrics:HistogramMetrics,parent:QWidget) -> None
        Do initial stuff and set the z-value so that the axis as drawn on top of the items.
        """
        QtGui.QGraphicsRectItem.__init__(self,parent)
        self.metrics=metrics
        self.setZValue(1)
        self.update(0,0)
        
    def update(self,x,y):
        """ update(x:float,y:float) -> None
        Update the axis dimensions depending on the histogram metrics.
        """
        self.baseLineOffset=self.metrics.viewBinWidth/7
        self.setRect(x,y,self.metrics.viewWidth,self.baseLineOffset)
        
    def baseLineY(self):
        """ baseLineY() -> float
        Return the y coordinate of the base line.
        """
        return self.rect().top()+self.baseLineOffset
        
    def paint(self,painter,option,widget):
        """ paint(painter:QPainter,options:QStyleOptionGraphicsItem,widget:QWidget) -> None
        Paint the axis.
        """
        
        """ base line """
        p1=QtCore.QPointF(self.rect().left(),self.baseLineY())
        p2=QtCore.QPointF(self.rect().right(),self.baseLineY())
        painter.drawLine(p1,p2)
        
        """ tick marks """
        p1=QtCore.QPointF(self.rect().left(),self.rect().top())
        p2=QtCore.QPointF(self.rect().left(),self.baseLineY())
        for i in xrange(self.metrics.viewResolution+1):
            painter.drawLine(p1,p2)
            p1.setX(self.rect().left()+(i+1)*self.metrics.viewBinWidth)
            p2.setX(p1.x())
        p1.setX(self.rect().right())
        p2.setX(p1.x())
        painter.drawLine(p1,p2)


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QHistogramLabeling(QtGui.QGraphicsRectItem):
    """ QHistogramLabeling implements the labeling for the x-axis.
    """

    def __init__(self,metrics,labels,parent=None):
        """
        """
        QtGui.QGraphicsRectItem.__init__(self,parent)
        self.metrics=metrics
        self.labels=labels
        self.cacheViewBinWidth=0
        
    def update(self,x,y,force=False):
        """ update(x:float,y:float,force:bool) -> None
        Update font metrics and misc variables that are needed for drawing;
        recompute only, if histogram width hast changed;
        the recomputation is forced, if force flag is set.
        """
        viewBinWidth=self.metrics.viewBinWidth
        if viewBinWidth!=self.cacheViewBinWidth or force:
            self.cacheViewBinWidth=viewBinWidth
            self.textSize=viewBinWidth/3
            self.labelSpacing=viewBinWidth/3
            self.font=QtGui.QFont('Helvetica',self.textSize)
            fontMetrics=QtGui.QFontMetricsF(self.font)        
            self.textHeight=max(map(fontMetrics.width,self.labels))
        self.setRect(x,y,self.metrics.viewWidth,self.textHeight+self.labelSpacing)

    def paint(self,painter,option,widget):
        """ paint(painter:QPainter,options:QStyleOptionGraphicsItem,widget:QWidget) -> None
        Paint the label texts.
        """
        painter.setFont(self.font)
        painter.translate(self.rect().left(),self.rect().bottom())
        painter.rotate(270)
        for i in range(len(self.labels)):
            pos=QtCore.QRectF(0,self.metrics.binOffsetX(i),self.textHeight,self.metrics.viewBinWidth)
            painter.drawText(pos,QtCore.Qt.AlignVCenter|QtCore.Qt.AlignRight,self.labels[i])
            

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QTimeHistogramLabeling(QHistogramLabeling):
    """ QTimeHistogramLabeling implements x-axis labeling for a time histogram.
    """

    def __init__(self,metrics):
        """ __init__(metrics:HistogramMetrics) -> None
        Construct the labels.
        """
        QHistogramLabeling.__init__(self,metrics,[])
        self.construct()
        
    def construct(self):
        """ contruct() -> None
        Construct the labels.
        """
        self.labels=[]
        for i in range(self.metrics.viewResolution):
            right=int(round((i+1)*self.metrics.dataBinWidth))
            #last=0
            #next=0
            #if i>0:
            #    last=int(round(int(round(histogramBounds[i]-leftBound))/60))
            #    if i<len(histogramBounds)-2:
            #        next=int(round(int(round(histogramBounds[i+2]-leftBound))/60))
            minutes=int(round(right/60))
            #if right>60 and last!=minutes and next!=minutes:
            self.labels.append(str(minutes)+'m')
            #else:
            #    self.labels.append(str(right)+'s')

        self.update(self.rect().x(),self.rect().y(),True)


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QHistogramView(QtGui.QGraphicsView):
    """ QHistogramView implements a graphics view that manages a scene
    with histogram elements (items,axis,labels).
    """

    def __init__(self,dataSource,parent=None):
        """ __init__(dataSource:DataSource,parent:QWidget) -> None
        Initialize the variables.
        """
        QtGui.QGraphicsView.__init__(self,parent)
        self.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAsNeeded)
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setInteractive(False)
        self.setCacheMode(QtGui.QGraphicsView.CacheBackground)
        self.setResizeAnchor(QtGui.QGraphicsView.AnchorViewCenter)
        self.setAlignment(QtCore.Qt.AlignLeft|QtCore.Qt.AlignTop)
        self.setScene(QtGui.QGraphicsScene(self))
        self.dataSource=dataSource
        self.metrics=HistogramMetrics(self.dataSource)
        self.counts=[]
        
    def updateData(self):
        """ updateData() -> None
        This function is called to update histogram content, which is mainly
        the heights of the data bins.
        """
        self.updateView()
        
    def updateView(self):
        """ updateView() -> None
        This function is called to reconstruct the scene, e.g., when a resize event occured.
        Right now, the scene is completely reconstructed (no rescaling previous content).
        """
    
        """ delete all items """
        for item in self.scene().items():
            self.scene().removeItem(item)
            
        """ define the scene by an invisible frame """
        frame=QtCore.QRectF(0,0,self.viewport().width()-1,self.viewport().height()-1)
        if issubclass(self.dataSource.__class__,ScrollableDataSource) \
        and not self.dataSource.isEmpty():
            frame.setWidth(frame.width()*self.metrics.dataRange/self.metrics.dataPageSize)
        self.setSceneRect(frame)
        self.scene().addRect(frame,QtGui.QPen(self.backgroundBrush(),1))
        
        """ drawArea defines the actual area to draw """
        self.drawArea=QtCore.QRectF(4,4,frame.width()-8,frame.height()-8)
        self.metrics.setViewWidth(self.drawArea.width())
                
    def resizeEvent(self,event):
        """ resizeEvent(event:QResizeEvent) -> None
        Resize the canvas and call the updateView function.
        """
        QtGui.QGraphicsView.resizeEvent(self,event)
        self.updateView()


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QHistogramWidget(QtGui.QWidget):
    """ QHistogramWidget holds one QHistogramView and controls that allow to
    control histogram paramters, e.g., zoom level and the resolution.
    """

    def __init__(self,histogramView,parent=None):
        """ __init__(histogramView:QHistogramView,parent:QWidget) -> None
        Create GUI content.
        """
        QtGui.QWidget.__init__(self,parent)
        self.histogramView=histogramView
    
        """ global layout """
        layout=QtGui.QVBoxLayout(self)
        layout.setMargin(0)
        layout.setSpacing(3)
        self.setLayout(layout)
    
        """ embed the histogram view """
        histogramView.setParent(self)
        layout.addWidget(histogramView)
        
        if issubclass(self.histogramView.dataSource.__class__,ScrollableDataSource):
            """ layout with controls """
            controlLayout=QtGui.QHBoxLayout(self)
            controlLayout.setMargin(0)
            controlLayout.setSpacing(3)
            layout.addLayout(controlLayout)
        
            """ resolution slider """
            controlLayout.addWidget(QtGui.QLabel('Resolution'))
            resSlider=QtGui.QSlider(QtCore.Qt.Horizontal,self)
            resSlider.setRange(10,3*self.histogramView.metrics.viewPageResolution)
            resSlider.setValue(self.histogramView.metrics.viewResolution)
            self.connect(resSlider,QtCore.SIGNAL('valueChanged(int)'),self.resolutionChanged)
            controlLayout.addWidget(resSlider)
            controlLayout.addSpacing(10)
        
            """ view range slider """
            controlLayout.addWidget(QtGui.QLabel('View Range'))
            fovSlider=QtGui.QSlider(QtCore.Qt.Horizontal,self)
            fovSlider.setRange(1,100)
            if not self.histogramView.dataSource.isEmpty():
                fovSlider.setValue(int(round(100*self.histogramView.metrics.dataPageSize/self.histogramView.metrics.dataRange)))
            self.connect(fovSlider,QtCore.SIGNAL('valueChanged(int)'),self.fieldOfViewChanged)
            controlLayout.addWidget(fovSlider)
            controlLayout.addSpacing(10)
        
            """ reset button """
            button=QtGui.QPushButton('Reset',self)
            self.connect(button,QtCore.SIGNAL('clicked'),self.resetParameters)
            controlLayout.addWidget(button)
        
    def updateData(self):
        """ updateData() -> None
        Call the updateData function of the histogram view.
        """
        self.histogramView.updateData()
    
    def resolutionChanged(self,value):
        """ resolutionChanged(value:int) -> None
        Update the histogram metrics and call updateData.
        """
        self.histogramView.metrics.setViewPageResolution(value)
        self.histogramView.updateData()
    
    def fieldOfViewChanged(self,value):
        """ fieldOfViewChanged(value:int) -> None
        Update the histogram metrics and call updateData.
        """        
        self.histogramView.metrics.setDataPageSize(value*self.histogramView.metrics.dataRange/100)
        self.histogramView.updateData()
        
    def resetParameters(self):
        """ resetParameters() -> None
        TBD
        """
        pass


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QHistogramInteraction(object):
    """ obsolete """

    def __init__(self):
        self.dragMode=0
        self.interactionEnabled=True
        self.dragPos=QtCore.QPoint()

    def mousePressEvent(self,event):
        if self.interactionEnabled:
            if event.buttons() & QtCore.Qt.LeftButton:
                self.dragMode=1
            elif event.buttons() & QtCore.Qt.RightButton:
                self.dragMode=2
            else:
                self.dragMode=0
            self.dragPos=QtCore.QPoint(event.pos())
            
    def mouseReleaseEvent(self,event):
        if self.interactionEnabled:
            self.dragMode=0
            
    def mouseMoveEvent(self,event):
        pass
            

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QTimeHistogramView(QHistogramView):
    """ QTimeHistogramView implements the time histogram
    """

    def __init__(self,dataSource,parent=None):
        """ __init__(dataSource:DataSource,parent:QWidget) -> None
        Do nothing.
        """
        QHistogramView.__init__(self,dataSource,parent)
        
    def updateData(self):
        """ updateData() -> None
        Construct the histogram; see QHistogramView.updateData()
        """
    
        """ compute bounds and counts """
        self.counts=[]
        if not self.dataSource.isEmpty():
            for i in range(self.metrics.viewResolution):
                self.counts.append(0)
            if self.metrics.dataBinWidth>0:
                for value in self.dataSource.data():
                    bin=int(math.floor((value-self.metrics.dataLeftBound)/self.metrics.dataBinWidth))
                    if bin>=0 and bin<self.metrics.viewResolution:
                        self.counts[bin]+=1

        """ update view """
        self.updateView()
        
    def updateView(self):
        """ updateView() -> None
        Construct the histogram scene; see QHistogramView.updateView().
        """
        
        QHistogramView.updateView(self)
        
        if not self.dataSource.isEmpty():
            """ x-axis labeling """
            labeling=QTimeHistogramLabeling(self.metrics)
            labeling.update(self.drawArea.left(),self.drawArea.bottom()-labeling.rect().height())
            self.scene().addItem(labeling)
        
            """ x-axis """
            axis=QHistogramXAxis(self.metrics)
            axis.update(self.drawArea.left(),labeling.rect().top()-axis.rect().height())
            self.scene().addItem(axis)
        
            """ data """
            if len(self.counts)>0:
                maxValue=max(self.counts)
                if maxValue>0:
                    maxHeight=axis.baseLineY()-self.drawArea.top()
                    for i in range(len(self.counts)):
                        height=self.counts[i]*maxHeight/maxValue
                        self.scene().addItem(QHistogramItem(self.drawArea.left()+i*self.metrics.viewBinWidth, \
                                             axis.baseLineY()-height,self.metrics.viewBinWidth,height))


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QActionHistogramView(QHistogramView):
    """ QTimeHistogramView implements the action histogram
    """

    def __init__(self,dataSource,parent=None):
        """ __init__(dataSource:DataSource,parent:QWidget) -> None
        Do nothing.
        """
        QHistogramView.__init__(self,dataSource,parent)
        
    def updateData(self):
        """ updateData() -> None
        Construct the histogram; see QHistogramView.updateData()
        """
    
        """ compute bounds and counts """

        """ update view """
        self.updateView()
        
    def updateView(self):
        """ updateView() -> None
        Construct the histogram scene; see QHistogramView.updateView().
        """
        
        QHistogramView.updateView(self)
        
        if not self.dataSource.isEmpty():
            """ x-axis labeling """
            labeling=QTimeHistogramLabeling(self.metrics)
            labeling.update(self.drawArea.left(),self.drawArea.bottom()-labeling.rect().height())
            self.scene().addItem(labeling)
        
            """ x-axis """
            axis=QHistogramXAxis(self.metrics)
            axis.update(self.drawArea.left(),labeling.rect().top()-axis.rect().height())
            self.scene().addItem(axis)
        
            """ data """
            if len(self.counts)>0:
                maxValue=max(self.counts)
                if maxValue>0:
                    maxHeight=axis.baseLineY()-self.drawArea.top()
                    for i in range(len(self.counts)):
                        height=self.counts[i]*maxHeight/maxValue
                        self.scene().addItem(QHistogramItem(self.drawArea.left()+i*self.metrics.viewBinWidth, \
                                             axis.baseLineY()-height,self.metrics.viewBinWidth,height))
