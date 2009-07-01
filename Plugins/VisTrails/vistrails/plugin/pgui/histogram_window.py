
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
QHistogramItem
QHistogramXAxis
QHistogramView
QTimeHistogramView
QHistogramWindow
QStatisticsWindow
"""


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
import urllib
import time
import datetime
import math
import array
from PyQt4 import QtGui, QtCore
from core.inspector import PipelineInspector
from gui.utils import getBuilderWindow


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QHistogramItem(QtGui.QAbstractGraphicsShapeItem):
    """

    """

    def __init__(self,x,y,width,height,parent=None):
        """

        """
        QtGui.QAbstractGraphicsShapeItem.__init__(self,parent)
        self.rect=QtCore.QRectF(x,y,width,height)
        pen=QtGui.QPen()
        pen.setColor(QtCore.Qt.black)
        #pen.setColor(QtGui.QColor(131,155,199))
        self.setPen(pen)
        brush=QtGui.QBrush()
        #brush.setColor(QtGui.QColor(0,4,96))
        brush.setColor(QtGui.QColor(131,155,199))
        brush.setStyle(QtCore.Qt.SolidPattern)
        self.setBrush(brush)
        self.setZValue(0)

    def boundingRect(self):
        return self.rect

    def paint(self,painter,option,widget=None):
        painter.setPen(self.pen())
        painter.setBrush(self.brush())
        painter.drawRect(self.rect)


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QTimeHistogramXAxis(QtGui.QGraphicsRectItem):
    """

    """

    def __init__(self,histogramBounds,leftBound,parent=None):
        QtGui.QGraphicsRectItem.__init__(self,parent)
        self.labels=[]
        for i in range(len(histogramBounds)-1):
            right=int(round(histogramBounds[i+1]-leftBound))
            last=0
            next=0
            if i>0:
                last=int(round(int(round(histogramBounds[i]-leftBound))/60))
                if i<len(histogramBounds)-2:
                    next=int(round(int(round(histogramBounds[i+2]-leftBound))/60))
            minutes=int(round(right/60))
            if right>60 and last!=minutes and next!=minutes:
                self.labels.append(str(minutes)+'m')
            else:
                self.labels.append(str(right)+'s')
        self.setZValue(1)
        
    def update(self,x,y,width):
        self.barWidth=float(width)/len(self.labels)
        self.font=QtGui.QFont('Helvetica',self.barWidth/3)
        fontMetrics=QtGui.QFontMetricsF(self.font)
        self.maxTextHeight=max(map(fontMetrics.width,self.labels))
        height=self.barWidth/2.5+self.maxTextHeight
        self.setRect(x,y,self.barWidth*len(self.labels),height)
        
    def baseLineY(self):
        return self.rect().top()+self.barWidth/7
        
    def paint(self,painter,option,widget):
        p1=QtCore.QPointF(self.rect().left(),self.baseLineY())
        p2=QtCore.QPointF(self.rect().right(),self.baseLineY())
        painter.drawLine(p1,p2)
        
        p1=QtCore.QPointF(self.rect().left(),self.rect().top())
        p2=QtCore.QPointF(self.rect().left(),self.baseLineY())
        for i in xrange(len(self.labels)+1):
            painter.drawLine(p1,p2)
            p1.setX(self.rect().left()+(i+1)*self.barWidth)
            p2.setX(p1.x())
        p1.setX(self.rect().right())
        p2.setX(p1.x())
        painter.drawLine(p1,p2)

        painter.setFont(self.font)
        painter.translate(self.rect().left(),self.rect().bottom())
        painter.rotate(270)
        for i in range(len(self.labels)):
            painter.drawText(QtCore.QRectF(0,i*self.barWidth,self.maxTextHeight,self.barWidth),
                             QtCore.Qt.AlignVCenter|QtCore.Qt.AlignRight,self.labels[i])


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QHistogramView(QtGui.QGraphicsView):
    """

    """
    
    dragMode=0
    dragPos=None

    def __init__(self,parent=None):
        """ __init__(parent:QObject) -> None

        """
        QtGui.QGraphicsView.__init__(self,parent)
        self.setHorizontalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setVerticalScrollBarPolicy(QtCore.Qt.ScrollBarAlwaysOff)
        self.setInteractive(False)
        self.setCacheMode(QtGui.QGraphicsView.CacheBackground)
        self.setResizeAnchor(QtGui.QGraphicsView.AnchorViewCenter)
        self.setAlignment(QtCore.Qt.AlignCenter)

    def updateView(self):
        pass

    def resizeEvent(self,event):
        result=QtGui.QGraphicsView.resizeEvent(self,event)
        self.updateView()
        return result
        
    def mousePressEvent(self,event):
        if event.buttons() & QtCore.Qt.LeftButton:
            self.dragMode=1
        elif event.buttons() & QtCore.Qt.RightButton:
            self.dragMode=2
        else:
            self.dragMode=0
        self.dragPos=QtCore.QPoint(event.pos())
            
    def mouseReleaseEvent(self,event):
        self.dragMode=0
      

#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QTimeHistogramView(QHistogramView):
    """

    """

    histogramBounds=None
    histogramCounts=None
    
    leftBound=None
    rightBound=None
    times=None

    def __init__(self,parent=None):
        """ __init__(parent:QObject) -> None

        """
        QHistogramView.__init__(self,parent)

    def updateView(self):
        for item in self.scene().items():
            self.scene().removeItem(item)
        self.setSceneRect(QtCore.QRectF(0,0,self.rect().width(),self.rect().height()))
        if self.histogramCounts!=None and len(self.histogramCounts)>0:
            offsetX=3
            offsetY=3
            width=self.width()-12
            height=self.height()-9
            barWidth=float(width)/len(self.histogramCounts)
            self.barWidth=barWidth
            axis=QTimeHistogramXAxis(self.histogramBounds,self.leftBound)
            axis.update(offsetX,offsetY,width)
            axis.moveBy(0,height-axis.rect().height())
            self.scene().addItem(axis)
            maxValue=max(self.histogramCounts)
            if maxValue>0:
                baseLineY=axis.y()+axis.baseLineY()
                for i in range(len(self.histogramCounts)):
                    height=self.histogramCounts[i]*(baseLineY-offsetY)/maxValue
                    if i==len(self.histogramCounts)-1:
                        x=offsetX+i*barWidth
                        self.scene().addItem(QHistogramItem(x,baseLineY-height,offsetX+width-x,height))
                    else:
                        self.scene().addItem(QHistogramItem(offsetX+i*barWidth,baseLineY-height,barWidth,height))

    def initData(self,times,leftBound,rightBound,currentLeft,currentRight):
        self.times=times
        self.leftBound=leftBound
        self.rightBound=rightBound
        self.binNumber=20
        #self.currentLeftBound=leftBound
        #self.currentRightBound=rightBound
        #self.updateData(False)
        #self.maxValue=max(self.histogramCounts)
        self.currentLeftBound=currentLeft
        self.currentRightBound=currentRight
        self.updateData()
        
    def updateData(self,updateView=True):
        self.histogramBounds=[]
        self.histogramCounts=[]
        self.histogramBounds.append(self.currentLeftBound)
        self.binWidth=(self.currentRightBound-self.currentLeftBound)/self.binNumber
        for i in range(self.binNumber):
            self.histogramBounds.append(self.currentLeftBound+(i+1)*self.binWidth)
            self.histogramCounts.append(0)
        for time in self.times:
            if self.binWidth>0:
                bin=int(math.floor((time-self.currentLeftBound)/self.binWidth))
                if bin>=0 and bin<self.binNumber:
                    self.histogramCounts[bin]+=1
        if updateView:
            self.updateView()
        
    def mouseMoveEvent(self,event):
        if self.dragMode==1:
            pos=event.pos()
            if abs(pos.x()-self.dragPos.x())>=self.barWidth:
                delta=self.binWidth*(pos.x()-self.dragPos.x())/self.barWidth
                delta=-delta
                spread=self.currentRightBound-self.currentLeftBound
                self.currentLeftBound+=delta
                self.currentRightBound+=delta
                if self.currentLeftBound<self.leftBound:
                    self.currentLeftBound=self.leftBound
                    self.currentRightBound=self.currentLeftBound+spread
                if self.currentRightBound>self.rightBound:
                    self.currentRightBound=self.rightBound
                    self.currentLeftBound=self.currentRightBound-spread
                self.dragPos=QtCore.QPoint(pos)
                self.updateData()
        elif self.dragMode==2:
            pos=event.pos()
            if abs(pos.y()-self.dragPos.y())>=5:
                diff=pos.y()-self.dragPos.y()
                sign=diff/abs(diff)
                scale=0.9
                spread=self.currentRightBound-self.currentLeftBound
                if sign>0:
                    spread*=0.9
                else:
                    spread/=scale
                if spread>self.rightBound-self.currentLeftBound:
                    spread=self.rightBound-self.currentLeftBound
                if spread<self.binNumber:
                    spread=self.binNumber
                self.currentRightBound=self.currentLeftBound+spread
                self.dragPos=QtCore.QPoint(pos)
                self.updateData()


#-------------------------------------------------------------------------------
#
#-------------------------------------------------------------------------------
class QHistogramWindow(QtGui.QDialog):
    """ QHistogramWindow

    """

    """ maximum time difference in seconds """
    timeDiffThreshold=60*10

    histogramType=None

    """ QHistogramView object """
    histogramView=None

    def __init__(self,histogramType,parent=None):
        """ __init__(histogramType:string,parent:QObject) -> None
        Create the dialog content.
        """
        self.histogramType=histogramType

        QtGui.QWidget.__init__(self,parent)
        self.setWindowTitle('Histogram')

        """ global layout """
        layout=QtGui.QVBoxLayout(self)
        layout.setMargin(5)
        layout.setSpacing(5)
        self.setLayout(layout)

        """ combo box and layout """
        self.updateBlocked=True
        if self.histogramType=='time' or self.histogramType=='timediff':
            self.comboBox=QtGui.QComboBox()
            self.comboBox.addItem('Time')
            self.comboBox.addItem('Time Difference')
            self.connect(self.comboBox,QtCore.SIGNAL('currentIndexChanged(int)'),self.switchToTimeHistogram)
            hLayout=QtGui.QHBoxLayout()
            hLayout.setSpacing(0)
            hLayout.setMargin(0)
            hLayout.addWidget(self.comboBox)
            hLayout.addStretch()
            layout.addLayout(hLayout)
        self.updateBlocked=False

        """ graphics scene """
        scene=QtGui.QGraphicsScene(self)

        self.histogramView=QTimeHistogramView(self)
        self.histogramView.setScene(scene)
        layout.addWidget(self.histogramView)

#        """ update and close button """
#        updateButton=QtGui.QPushButton('Update')
#        self.connect(updateButton,QtCore.SIGNAL('clicked()'),self.update)
#        closeButton=QtGui.QPushButton('Close')
#        self.connect(closeButton,QtCore.SIGNAL('clicked()'),self.close)
#        bLayout=QtGui.QHBoxLayout()
#        bLayout.setSpacing(5)
#        bLayout.addWidget(updateButton)
#        bLayout.addWidget(closeButton)
#        layout.addLayout(bLayout)
        
    def switchToTimeHistogram(self,index):
        if index==1:
            self.histogramType='timediff'
        else:
            self.histogramType='time'
        self.update()

    def sizeHint(self):
        """ sizeHint() -> QRect
        Return the recommended size of the window.
        """
        return QtCore.QSize(600,250)

    def update(self):
        """ update() -> None
        """
        if not self.updateBlocked:
            if self.histogramType=='timediff':
                self.updateTimeDiffHistogram()
            else:
                self.updateTimeHistogram()

    def updateTimeDiffHistogram(self):
        """ updateTimeDiffHistogram() -> None
        
        FIXME: Does not use session ids yet like the text statistics do
        """

        """ extract the vistrail dates """
        dates=[]
        for action in getBuilderWindow().viewManager.currentWidget().controller.vistrail.actions:
            dt=datetime.datetime.strptime(action.date,'%d %b %Y %H:%M:%S')
            dates.append(time.mktime(dt.timetuple())+dt.microsecond/1000000.0)

        """ compute the time differences """
        if len(dates)>0:
            dates=sorted(dates)
            diffs=[]
            for i in range(len(dates)-1):
                diff=dates[i+1]-dates[i]
                if diff<=self.timeDiffThreshold:
                    diffs.append(diff)

            """ compute bounds and counts """
            if len(diffs)>0:
                leftBound=0
                rightBound=max(diffs)

                mean=0
                for diff in diffs:
                    mean+=diff
                mean/=len(diffs)
                stddev=0
                for diff in diffs:
                    stddev+=(diff-mean)*(diff-mean)
                stddev=math.sqrt(stddev/len(diffs))

                self.histogramView.initData(diffs,leftBound,rightBound,leftBound,2*stddev)

    def updateTimeHistogram(self):
        """ updateTimeHistogram() -> None

        FIXME: Does not use session ids yet like the text statistics do
        """

        """ extract the vistrail dates """
        dates=[]
        for action in getBuilderWindow().viewManager.currentWidget().controller.vistrail.actions:
            dt=datetime.datetime.strptime(action.date,'%d %b %Y %H:%M:%S')
            dates.append(time.mktime(dt.timetuple())+dt.microsecond/1000000.0)

        """ compute bounds and counts """
        histogramBounds=[]
        histogramCounts=[]
        if len(dates)>0:
            leftBound=min(dates)
            rightBound=max(dates)
            self.histogramView.initData(dates,leftBound,rightBound,leftBound,rightBound)
                    
    def getInfoFromPipeline(self):
        """ getInfoFromPipeline() -> (string array,string array)
        get operations and descriptions from pipeline """
        controller=getBuilderWindow().viewManager.currentWidget().controller
        descriptions = [controller.vistrail.get_description(v) 
                        for v in controller.vistrail.actionMap.keys()]
        ops = controller.extract_ops_per_version(0,controller.current_version)        
        return (descriptions,ops)


class QTimeStatisticsWindow(QtGui.QDialog):
    """ QTimeStatisticsWindow
    
    Create a simple window showing users and the total time between
    actions in a session.
    """
    def __init__(self,parent=None):
        """ __init__(parent: QObject) -> None
        
        """
        QtGui.QDialog.__init__(self, parent)
        self.setWindowTitle('Time Statistics')
        self.gridLayout = QtGui.QGridLayout(self)
        self.gridLayout.setMargin(10)
        self.gridLayout.setSpacing(10)
        self.userLabel = QtGui.QLabel('<b>User</b>')
        self.timeLabel = QtGui.QLabel('<b>Total Time</b>')
        self.gridLayout.addWidget(self.userLabel, 0, 0,
                                  QtCore.Qt.AlignLeft)
        self.gridLayout.addWidget(self.timeLabel, 0, 1,
                                  QtCore.Qt.AlignHCenter)
        self.line = QtGui.QFrame()
        self.line.setLineWidth(1.0)
        self.line.setFrameShadow(QtGui.QFrame.Plain)
        self.line.setFrameShape(QtGui.QFrame.HLine)
        self.gridLayout.addWidget(self.line, 1, 0, 1, 2)
        self.gridLayout.setColumnMinimumWidth(1, 150)
        self.setLayout(self.gridLayout)
        self.tempWidgets = []

    def update(self):
        """ updateTimeTotals() -> None
        Calculate the total time for the user across sessions
        
        """
        stats={}
        previous_time = None
        previous_session = None
        controller = getBuilderWindow().viewManager.currentWidget().controller
        for action in controller.vistrail.actions:
            dt = datetime.datetime.strptime(action.date, '%d %b %Y %H:%M:%S')
            current_time = time.mktime(dt.timetuple())+dt.microsecond/1000000.0
            if previous_time and action.session == previous_session:
                if stats.has_key(action.user):
                    stats[action.user] += current_time-previous_time
                else:
                    stats[action.user] = current_time-previous_time
            previous_session = action.session
            previous_time = current_time
        
        for w in self.tempWidgets:
            self.gridLayout.removeWidget(w)
            w.destroy()
        self.tempWidgets[:] = []

        row = 2
        for (u,t) in stats.iteritems():
            uLabel = QtGui.QLabel(u)
            secs = int(t)%60
            mins = int(t)%3600/60
            hours = int(t)/3600
            tt = str(hours) + ' hr(s) ' + str(mins) + ' min(s) ' + str(secs) + ' sec(s)'
            tLabel = QtGui.QLabel(tt)
            self.gridLayout.addWidget(uLabel, row, 0, 
                                      QtCore.Qt.AlignLeft)
            self.gridLayout.addWidget(tLabel, row, 1,
                                      QtCore.Qt.AlignHCenter)
            self.tempWidgets.append(uLabel)
            self.tempWidgets.append(tLabel)
            row+=1
