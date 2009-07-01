
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

from PyQt4 import QtGui, QtCore
from gui.theme import CurrentTheme
from gui.utils import getBuilderWindow
from plugin.pgui.addon_toolbar import QAddonToolBar
import CaptureAPI
import time

class QPlaybackToolBar(QAddonToolBar):
    """
    QPlaybackToolBar contains Play, Pause, Stop, Version Selection and
    a speed adjustment for playing back between versions of the captured app.

    """
    def __init__(self, parent=None):
        """ QPlaybackToolBar(parent: QWidget) -> QPlaybackToolBar
        
        """
        QAddonToolBar.__init__(self, parent)

        self.setWindowTitle('Playback')
        self.setIconSize(QtCore.QSize(16, 16))

        # Play/Pause/Stop Buttons
        self.playPauseAction = QtGui.QAction(CurrentTheme.PLAY_ICON, 'Play', self)
        self.playPauseAction.setStatusTip('Play/Pause the animation')
        self.addAction(self.playPauseAction)
        self.connect(self.playPauseAction, QtCore.SIGNAL('triggered(bool)'),
                     self.playPauseTriggered)
        
        self.stopAction = QtGui.QAction(CurrentTheme.STOP_ICON, 'Stop', self)
        self.stopAction.setStatusTip('Stop the animation')
        self.addAction(self.stopAction)
        self.connect(self.stopAction, QtCore.SIGNAL('triggered(bool)'),
                     self.stop)
        
        self.playerTimer = QtCore.QTimer()
        self.playerTimer.setSingleShot(True)
        self.connect(self.playerTimer, QtCore.SIGNAL('timeout()'),
                     self.playNextVersion)

        self.addSeparator()

        # Pick Start/Stop Buttons
        
        self.pickAction = QtGui.QAction(CurrentTheme.PICK_VERSIONS_ICON,
                                        'Select versions', self)
        self.pickAction.setStatusTip('Select start and end versions')
        self.pickAction.setCheckable(False)
        self.addAction(self.pickAction)        
        self.connect(self.pickAction, QtCore.SIGNAL('triggered(bool)'),
                     self.pickingTriggered)

        self.addSeparator()
        
        # Frame Slider
        self.frameSlider = QtGui.QSlider(self)
        self.frameSlider.setOrientation(QtCore.Qt.Horizontal)
        self.frameSlider.setMinimum(0)
        self.frameSlider.setMaximum(0)
        self.frameSlider.setSingleStep(1)
        self.addWidget(self.frameSlider)
        self.addSeparator()
        self.frameLabel = QtGui.QLabel(self)
        self.frameLabel.setFixedWidth(40)
        self.addWidget(self.frameLabel)
        self.connect(self.frameSlider, QtCore.SIGNAL('valueChanged(int)'),
                     self.updateFrameLabel)
        self.connect(self.frameSlider, QtCore.SIGNAL('sliderMoved(int)'),
                     self.updateFrameSlider)
        self.connect(self.frameSlider, QtCore.SIGNAL('sliderReleased()'),
                     self.updateFrameContent)

        # Delay combo box
        self.delayComboBox = QtGui.QComboBox()
        self.delayComboBox.addItem('Slow', QtCore.QVariant(2.0))
        self.delayComboBox.addItem('Medium', QtCore.QVariant(0.5))
        self.delayComboBox.addItem('Fast', QtCore.QVariant(0.0))
        self.delayComboBox.setCurrentIndex(1)
        self.addWidget(self.delayComboBox)

        # Set the view toggle icon
        tva = self.toggleViewAction()
        tva.setIcon(CurrentTheme.PLAY_ICON)
        tva.setToolTip('Playback')
        tva.setStatusTip('Playback the changes from one version to another')
        tva.setShortcut('Ctrl+P')

        # Default version
        self.frames = []
        self.currentFrame = 0
        self.opsMap = {}
        self.lastVersion = -1

        self.setCurrentFrame(0)

        self.previousVersions = (-1, -1)

    def updateFrameLabel(self, value):
        """ Update the frame label """
        maxValue = self.frameSlider.maximum()
        if value>maxValue:
            value = maxValue
        self.frameLabel.setText('%d/%d' % (value+1, maxValue+1))

    def updateFrameSlider(self, value):
        """ Highlight the frame without contents when the user moved """
        if self.controller and len(self.frames)>0:
            if self.lastVersion==-1:
                self.lastVersion = self.controller.current_version
            builderWindow = getBuilderWindow()
            builderWindow.changeVersionWithoutUpdatingApp(self.frames[value])

    def updateFrameContent(self):
        """ Update the frame content when the user stops moving """
        if self.controller and len(self.frames)>0:
            value = self.frameSlider.value()
            self.setCurrentFrame(value)
            self.controller.update_app_with_current_version(self.lastVersion)
            self.lastVersion = -1

    def updateControllerInfo(self):
        """ Update the start and end version """
        if self.controller:
            self.opsMap = self.controller.diff_ops_linear(self.controller.playback_start,
                                                          self.controller.playback_end)
            self.frames = self.opsMap.keys()
            self.frames.sort()
        else:
            self.frames = []
            self.opsMap = {}
        self.frameSlider.setMaximum(max(len(self.frames)-1, 0))
        self.setCurrentFrame(0)

    def setController(self, controller):
        """ Make sure to update the start and end version """
        if self.controller:
            self.disconnect(self.controller,
                            QtCore.SIGNAL('scene_updated'),
                            self.vistrailModified)
            if self.isVisible() and controller!=self.controller:
                self.hide()
                self.setController(None)
        QAddonToolBar.setController(self, controller)
        if self.controller:
            self.connect(self.controller,
                         QtCore.SIGNAL('scene_updated'),
                         self.vistrailModified)
        self.updateControllerInfo()
        versionView = self.getVersionView()
        if versionView:
            versions = self.getCurrentVersions()
            versionView.setIndicatorVersions(versions[0], versions[1], -1)

    def setCurrentFrame(self, frameNumber):
        """ Set the frame number and its slider bar position """
        self.currentFrame = frameNumber
        self.updateFrameLabel(frameNumber)
        self.frameSlider.setValue(frameNumber)

    def stop(self, checked=False):
        """ Stop and reset the playback"""
        if self.playPauseAction.text()=='Pause':
            self.playPauseAction.setText('Play')
            self.playPauseAction.setIcon(CurrentTheme.PLAY_ICON)
        self.setCurrentFrame(0)
        if self.controller!=None and len(self.frames)>0:
            self.controller.change_selected_version(self.frames[self.currentFrame])
            
    def playPauseTriggered(self, checked=False):
        """ Set play and pause status """
        if self.playPauseAction.text()=='Play':
            if self.currentFrame+1>=len(self.frames):
                self.setCurrentFrame(0)
            self.playPauseAction.setText('Pause')
            self.playPauseAction.setIcon(CurrentTheme.PAUSE_ICON)
            self.playNextVersion()
        else:
            self.playerTimer.stop()
            self.playPauseAction.setText('Play')
            self.playPauseAction.setIcon(CurrentTheme.PLAY_ICON)

    def getVersionView(self):
        """ Return the attached version view """
        if self.controller:
            return self.controller.vistrail_view.versionTab.versionView
        return None

    def getCurrentVersions(self):
        """ Selecting the currently playback versions """
        if self.controller:
            return (self.controller.playback_start,
                    self.controller.playback_end)
        return (-1, -1)

    def setCurrentVersions(self, start, end):
        """ Update the current version of the controller """
        if self.controller:
            self.controller.playback_start = start
            self.controller.playback_end = end
            self.updateControllerInfo()

    def pickingTriggered(self, checked):
        """ Start picking a start version """
        versionView = self.getVersionView()
        if versionView:
            self.previousVersions = self.getCurrentVersions()
            self.setEnabled(False)
            versionView.multiSelectionStart(2,'Playback')
            self.connect(versionView,
                         QtCore.SIGNAL('doneMultiSelection'),
                         self.doneMultiSelection)

    def showEvent(self, e):
        """ Show the indicators in the version view """
        QAddonToolBar.showEvent(self, e)
        versionView = self.getVersionView()
        if versionView:
            if self.controller:
                if self.controller.playback_start==-1 or self.controller.playback_end==-1:
                    self.setCurrentVersions(0, self.controller.current_version)
                versions = self.getCurrentVersions()
                versionView.setIndicatorVersions(versions[0],versions[1],-1)
            versionView.acquireMultiSelection('Playback')
            versionView.setIndicatorsVisible(True, True, False)
#        CaptureAPI.setReportingEnabled(False)
#        CaptureAPI.setAppTracking(False)

    def hideEvent(self, e):
        """ Hide the indicators in the version view """
        QAddonToolBar.hideEvent(self, e)
        versionView = self.getVersionView()
        if versionView:
            versionView.multiSelectionAbort('Playback')
#        CaptureAPI.setAppTracking(True)
#        CaptureAPI.setReportingEnabled(True)
            
    def doneMultiSelection(self, successful, versions):
        """ doneMultiSelection(successful: bool) -> None

        Handled the selection is done. successful specifies if the
        selection was completed or aborted by users.
        """
        versionView = self.getVersionView()
        self.disconnect(versionView, QtCore.SIGNAL('doneMultiSelection'),
                        self.doneMultiSelection)
        if successful:
            currentVersions = versions
            valid = True
            try:
                valid = self.controller.vistrail.actionChain(currentVersions[1],
                                                             currentVersions[0])!=[]
            except:
                valid = False
            if valid:
                self.setCurrentVersions(currentVersions[0],currentVersions[1])
            else:
                successful = False
                QtGui.QMessageBox.critical(getBuilderWindow(),
                                           'Invalid versions',
                                           'The selected versions cannot be used for playback. ' \
                                           'The end version must be a descendant of the start' \
                                           'version. The selection has been canceled.')
        if not successful:
            self.setCurrentVersions(*self.previousVersions)
            versionView.setIndicatorVersions(self.previousVersions[0],self.previousVersions[1],-1)
            versionView.acquireMultiSelection('Playback')
            versionView.setIndicatorsVisible(True,True,False)
        self.setEnabled(True)

        # Mac Qt bug: For some reason these widgets need to be enabled individually
        self.frameSlider.setEnabled(True)
        self.frameLabel.setEnabled(True)
        self.delayComboBox.setEnabled(True)

        self.stop()

    def vistrailModified(self):
        """ Exit playback mode when users make changes to the vistrail """
        self.hide()


    def playNextVersion(self):
        """ Walk to the next version and wait a delay """
        if self.showNextVersion():
            index = self.delayComboBox.currentIndex()
            def startTimer():
                delayTime = int(self.delayComboBox.itemData(index).toDouble()[0]*1000)
                time.sleep(delayTime/1000.0)
                self.playerTimer.start()
            CaptureAPI.executeDeferred(startTimer)
        elif self.playPauseAction.text()=='Pause':
            self.playPauseTriggered()

    def showNextVersion(self):
        """ Show the next version and return True if there are more
        to show """
        if (not self.isVisible() or self.playPauseAction.text()=='Play'):
            return False
        
        if self.currentFrame+1>=len(self.frames):
            return False
        
        if self.frames[self.currentFrame]!=self.controller.current_version:
            self.controller.change_selected_version(self.frames[self.currentFrame])
        else:
            self.setCurrentFrame(self.currentFrame+1)
            self.controller.change_selected_version(self.frames[self.currentFrame])
        CaptureAPI.refreshApp()
        
        return self.currentFrame+1<len(self.frames)
