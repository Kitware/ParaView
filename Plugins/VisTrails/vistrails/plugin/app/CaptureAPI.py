
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
## Copyright (C) 2008, 2009 VisTrails, Inc. All rights reserved.
##
############################################################################




from PyQt4 import QtCore, QtNetwork
import core
import thread
import socket
import struct

################################################################################

# These message id's need to match the ParaView plugin side!!

# Commands we can send to ParaView.
pvSHUTDOWN = 0
pvRESET = 1
pvMODIFYSTACK = 2

# Commands that ParaView sends to us.
vtSHUTDOWN = 0
vtUNDO = 1
vtREDO = 2
vtNEW_VERSION = 3
vtREQUEST_VERSION_DATA = 4


# Where do we expect to find ParaView listening for us.
pvHost = QtNetwork.QHostAddress(QtNetwork.QHostAddress.LocalHost)
pvPort = 50013

# The Port that we listen on.
vtPort = 50007



# This helper class wraps a QTcpSocket so you can easily read/write
# strings and ints to/from it.  It keeps a buffer of received raw
# data that has not been asked for yet.
class SocketHelper(object):

    def __init__(self, sock):
        self.sock = sock
        self.buff = QtCore.QByteArray()

    def writeInt(self, i):
        bytes = struct.pack('!i', i)
        self.sock.write(bytes)
#            raise self.SocketError("error writing int %i" % i)

    def writeString(self, s):
        self.writeInt(len(s))
        if self.sock.write(s) != len(s):
            raise self.SocketError("error writing string %s" % s)

    def readData(self, size):
        while len(self.buff) < size:
            if self.sock.waitForReadyRead(-1):
                self.buff.append(self.sock.readAll())
            else:
                raise self.SocketError("error reading data")


        ret = self.buff.left(size)
        self.buff.remove(0, size)
        return ret

    def readInt(self):
        (i,) = struct.unpack('!i', self.readData(4)) # unpack returns a tuple
        return i

    def readString(self):
        size = self.readInt()
        ret = str(self.readData(size))
        return ret


    # Just pass a couple functions through to the underlying socket
    def waitForBytesWritten(self, timeout):
        self.sock.waitForBytesWritten(timeout)

    def close(self):
        self.sock.close();


    class SocketError(Exception):
        pass



# taken from patch_vistrailcontroller.py
def getPipeline(actions):
    from db.domain import DBWorkflow
    from db.services.vistrail import performActions
    from core.vistrail.pipeline import Pipeline

    workflow = DBWorkflow()
    performActions(actions, workflow)
    Pipeline.convert(workflow)
    return workflow

class ParaViewPlugin(QtCore.QObject):
    """
    ParaViewPlugin is the main class for the vistrails component of the
    ParaView plugin.  It sets up the builder window so it looks like we 
    want it to, starts a thread to read messages sent from paraview, and
    handles updating the vistrail.

    """

    # we can only have one instance of the plugin
    instance = None

    def __init__(self, builderWindow):
        """
        ParaViewPlugin(builderWindow: QBuilderWindow) -> ParaViewPlugin
        
        """

        # we only allow one instance of this class since the capture api
        # is just a set of functions (no class to hold state)
        if ParaViewPlugin.instance:
            print "ParaViewPlugin already instantiated!"
            raise Exception()
        ParaViewPlugin.instance = self

        self.pvVersionStack = None
        self.pvCurrentStackIndex = 0
        self.ignoreVersionChange = False
        self.builderWindow = builderWindow

        # generic initialization of the builder window
        self.builderWindow.title = 'ParaView Provenance Recorder'
        self.builderWindow.setWindowTitle(self.builderWindow.title)

        # we can't perform these actions yet
        self.builderWindow.visDiffAction.setVisible(False)
        self.builderWindow.patchAction.setVisible(False)
        self.builderWindow.useRecordedViewsAction.setVisible(False)

        # start our thread to listen to what paraview has to say
        self.pvSender = None
        thread.start_new_thread(self.listenerMain, ())

        # init the current vistrail
        self.vistrailChanged()

    def vistrailChanged(self):

        newView = self.builderWindow.viewManager.currentView()

        if newView:
            # we always get this signal twice in a row for some reason
            print 'vistrailChanged'
        
            # set paraview stack to the base empty version
            self.pvVersionStack = [0]
            self.pvCurrentStackIndex = 0

            # let paraview know we're starting fresh
            self.sendResetSignalToParaView()

            self.versionChanged()


    def versionChanged(self):
        print "versionChangedSlot"

        controller = self.builderWindow.viewManager.currentView().controller

        oldVersion = self.pvVersionStack[self.pvCurrentStackIndex]
        newVersion = controller.current_version

        if newVersion < 0:
            print "selected non-version!"
            return

        # get the two chains to the different versions
        oldChain = self.getChainToRoot(oldVersion)
        oldChain.append(oldVersion)

        newChain = self.getChainToRoot(newVersion)
        newChain.append(newVersion)


        # build a sequence of versions that we need to go through to get
        # to the version we want
        versionSequence = [oldChain.pop()]
        numUp = 0

        hasUndoLoadState = False

        # walk up the version tree
        while 1:

            # check if we're undo'ing a state load
            if versionSequence[-1] != 0:

                actionChain = [controller.vistrail.actionMap[versionSequence[-1]]]
                delta = controller.getOpsFromPipeline(getPipeline(actionChain))

                if delta[0].startswith('s'):
                    hasUndoLoadState = True


            # see if we've gone far enough
            if versionSequence[-1] in newChain:
                break

            # go up one version
            versionSequence.append(oldChain.pop())
            self.pvCurrentStackIndex -= 1
            numUp += 1


        # if we're trying to undo a load state, we have to reset all the way back to
        # the beginning and replay down to the new version
        if hasUndoLoadState:
            self.sendResetSignalToParaView()
            versionSequence = [0]
            self.pvCurrentStackIndex = 0
            numUp = 0


        # walk back down to the new version
        downChain = newChain[newChain.index(versionSequence[-1])+1:]
        for downVersion in downChain:
            versionSequence.append(downVersion)

            self.pvCurrentStackIndex += 1

            # check if this is a "redo"
            if len(self.pvVersionStack) > self.pvCurrentStackIndex and \
                    self.pvVersionStack[self.pvCurrentStackIndex] == downVersion:
                pass

            # otherwise, truncate the version stack because we're doing new stuff
            else:
                self.pvVersionStack[self.pvCurrentStackIndex:] = [downVersion]

        if len(versionSequence) == 1:
            print "no change in version"

        else:
            # send the change to paraview
            print "sending MODIFYSTACK to ParaView"
            self.pvSender.writeInt(pvMODIFYSTACK)
            self.pvSender.writeInt(len(versionSequence))
            self.pvSender.writeInt(numUp)
            for v in versionSequence:
                self.pvSender.writeInt(v)
            self.pvSender.waitForBytesWritten(-1)
    
            # we expect an ack
            ack = self.pvSender.readInt()
            if ack != 0:
                print "ParaView error"


    def aboutToQuit(self):
        # Send a message to ParaView's listener thread so it can shut itself down too
        print "about to quit..."

        if self.pvSender:
            self.pvSender.writeInt(pvSHUTDOWN)
            self.pvSender.waitForBytesWritten(-1)


    def getChainToRoot(self, version, start=0):
        if version == 0:
            return []

        vistrail = self.builderWindow.viewManager.currentView().controller.vistrail
        
        result = []
        t = vistrail.actionMap[version].parent
        while  t != start:
            result.append(int(t))
            t = vistrail.actionMap[t].parent
        result.append(0)
        result.reverse()
        return result


    def listenerMain(self):

        print "Listener starting..."

        # Open the port to listen for a ParaView connection on.
        server = QtNetwork.QTcpServer()
        server.listen(QtNetwork.QHostAddress(QtNetwork.QHostAddress.LocalHost), vtPort)

        # Loop trying to open sender and receiver sockets.
        receiver = None
        self.pvSender = None
        while not receiver or not self.pvSender:

            if not self.pvSender:
                print "Opening connection to ParaView server...",
                tmpSock = QtNetwork.QTcpSocket()
                tmpSock.connectToHost(pvHost, pvPort)
                if tmpSock.waitForConnected(1000):
                    self.pvSender = SocketHelper(tmpSock)
                    print "connection open."
                else:
                    print "timed out."

                del tmpSock

            if not receiver:
                print "Waiting for incoming ParaView connection...",
                if server.waitForNewConnection(1000)[0]:
                    receiver = SocketHelper(server.nextPendingConnection())
                    print "found connection."
                else:
                    print "timed out."


        # Enter the main server loop that waits for messages on the receiver socket.
        print "Entering listener loop."

        try:
            while 1:

                # get the next message id
                msg = receiver.readInt()

                if msg == vtSHUTDOWN:
                    print "received SHUTDOWN"

                    self.builderWindow.quitVistrailsAction.emit(QtCore.SIGNAL("triggered()"))
                    # FIXME: check if we've actually shutdown?

                    receiver.writeInt(0)
                    receiver.waitForBytesWritten(-1)
                    receiver.close()

                    return

                elif msg == vtUNDO:
                    print "received UNDO"

                    if self.pvCurrentStackIndex <= 0:
                        receiver.writeInt(1)

                    else:
                        self.pvCurrentStackIndex -= 1
                        controller = self.builderWindow.viewManager.currentView().controller
                        controller.change_selected_version(self.pvVersionStack[self.pvCurrentStackIndex])
                        controller.invalidate_version_tree(False)

                        receiver.writeInt(0)

                    receiver.waitForBytesWritten(-1)

                elif msg == vtREDO:
                    print "received REDO"

                    if self.pvCurrentStackIndex >= len(self.pvVersionStack)-1:
                        receiver.writeInt(1)

                    else:
                        self.pvCurrentStackIndex += 1
                        controller = self.builderWindow.viewManager.currentView().controller
                        controller.change_selected_version(self.pvVersionStack[self.pvCurrentStackIndex])
                        controller.invalidate_version_tree(False)

                        receiver.writeInt(0)

                    receiver.waitForBytesWritten(-1)

                elif msg == vtNEW_VERSION:
                    print "received NEW_VERSION"

                    label = receiver.readString()
                    delta = receiver.readString()
                    maxId = receiver.readInt()

                    # insert the new version and send back its id
                    newVersion = self._addNewVersion(label, delta, maxId)

                    # update the state we think paraview is in
                    self.pvVersionStack[self.pvCurrentStackIndex+1:] = [newVersion]
                    self.pvCurrentStackIndex += 1

                    receiver.writeInt(newVersion)
                    receiver.waitForBytesWritten(-1)

                    print "new version: ", newVersion

                elif msg == vtREQUEST_VERSION_DATA:
                    print "received REQUEST_VERSION_DATA"

                    num = receiver.readInt()
                    versions = []

                    for i in xrange(num):
                        v = receiver.readInt()
                        versions.append(v)

                    for v in versions:
                        controller = self.builderWindow.viewManager.currentView().controller
                        actionChain = [controller.vistrail.actionMap[v]]
                        delta = controller.getOpsFromPipeline(getPipeline(actionChain))

                        receiver.writeString(controller.vistrail.get_description(v))
                        receiver.writeString(delta)


                    receiver.waitForBytesWritten(-1)

                else:
                    print "received unknown message", msg
                    receiver.writeInt(1)
                    receiver.waitForBytesWritten(-1)

        except SocketHelper.SocketError:
            print "socket error in listener thread!"


    def sendResetSignalToParaView(self):
        #return # these are getting sent when they shouldn't!
        if self.pvSender:
            print 'Sending RESET command to ParaView.'

            controller = self.builderWindow.viewManager.currentView().controller
            maxIdAnnotation = controller.vistrail.get_annotation("ParaView_max_id")

            if maxIdAnnotation:
                maxId = int(maxIdAnnotation.value)
            else:
                maxId = 0

            self.pvSender.writeInt(pvRESET)
            self.pvSender.writeInt(maxId)
            self.pvSender.waitForBytesWritten(-1)

            # we expect an ack
            ack = self.pvSender.readInt()
            if ack != 0:
                print "ParaView error"


    def _addNewVersion(self, label, delta, maxId):

        controller = self.builderWindow.viewManager.currentView().controller
        controller.update_scene_script(label, False, delta)

        # Keep track of the max id that ParaView has allocated
        controller.vistrail.set_annotation("ParaView_max_id", maxId)

        return controller.current_version








############################################################################
## These are the api functions that vistrails calls directly
############################################################################

def start(builderWindow):
    builderWindow.setUpdateAppEnabled(True)
    ParaViewPlugin(builderWindow)

def printApp():
    print "printApp"

def storePresetAttributes():
    print "storePresetAttributes"

def updateAppWithCurrentVersion(partialUpdates, commonVersion, oldVersion, currentVersion, useCamera):
    print "updateAppWithCurrentVersion"

def flushCurrentContext(version):
    # we get these when the user clicks on undo/redo - when else??
    print "flushcurrentContext"
    if ParaViewPlugin.instance and not ParaViewPlugin.instance.ignoreVersionChange:
        ParaViewPlugin.instance.versionChanged()
   
def refreshApp():
    print "refresh"
   

preferences = {'VisTrailsFileDirectory' : '.',
               'VisTrailsUseRecordedViews' : '0',
               'VisTrailsBuilderWindowGeometry' : '',
               'VisTrailsNumberOfVisibleVersions' : '10',
               'VisTrailsSnapshotEnabled': '0',
               'VisTrailsSnapshotCount': '50'}


def getPreference(pref):
    return preferences[pref]


def setPreference(pref, val):
    preferences[pref] = val
   
def startVisualDiff():
    print "startVisualDiff"

def stopVisualDiff():
    print "stopVisualDiff"

def setAppTracking(enable):
    print "setTracking", enable
   
def resetUndoStack():
    print "resetUndoStack"

   
def unpickleAndPerformOperation():
    print "unpickle"
   
def unloadPlugin():
    print "unloadPlugin"

    if ParaViewPlugin.instance:
        ParaViewPlugin.instance.aboutToQuit()
    import sys
    import gui.application
    gui.application.stop_application()
    # QtCore.QCoreApplication.quit()
    # stop_application() should already take care of quitting the application
    # we will just tell python to quit. sys.exit(0) does some cleaning up.
    sys.exit(0)

def executeDeferred(func):
    print "executeDeferred"
    func()
  

##
def beforeNewVistrail():
    print "beforeNewVistrail"
    if ParaViewPlugin.instance:
        ParaViewPlugin.instance.ignoreVersionChange = True

def afterNewVistrail():
    print "afterNewVistrail"
    if ParaViewPlugin.instance:
        ParaViewPlugin.instance.ignoreVersionChange = False
        ParaViewPlugin.instance.vistrailChanged()


##
def beforeOpenVistrail(file):
    print "beforeOpenVistrail"
    if ParaViewPlugin.instance:
        ParaViewPlugin.instance.ignoreVersionChange = True

def afterOpenVistrail():
    print "afterOpenVistrail"
    if ParaViewPlugin.instance:
        ParaViewPlugin.instance.ignoreVersionChange = False
        ParaViewPlugin.instance.vistrailChanged()


##
def afterVersionSelectedByUser():
    print "afterVersionSelected"
    if ParaViewPlugin.instance and not ParaViewPlugin.instance.ignoreVersionChange:
        ParaViewPlugin.instance.versionChanged()

def createSnapshot():
    print "createSnapshot"

