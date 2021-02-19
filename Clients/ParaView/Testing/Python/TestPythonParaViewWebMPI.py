#/usr/bin/env python

# Global python import
import exceptions, traceback, logging, random, sys, threading, time, os

# Update python path to have ParaView libs
build_path='/Volumes/SebKitSSD/Kitware/code/ParaView/build-ninja'
sys.path.append('%s/lib'%build_path)
sys.path.append('%s/lib/site-packages'%build_path)

# ParaView import
from vtk.web import server
from paraview.vtk import *
from paraview.web import wamp as pv_wamp
from paraview.web import ipython as pv_ipython

from vtk.vtkCommonCore import *
from vtk.vtkCommonDataModel import *
from vtk.vtkCommonExecutionModel import *
from vtk.vtkFiltersSources import *
from vtk.vtkParallelCore import *
from vtk.vtkPVClientWeb import *
from vtk.vtkRemotingCore import *
from vtk.vtkRemotingApplication import *
from vtk.vtkRemotingServerManager import *

#------------------------------------------------------------------------------
# InLine protocol
#------------------------------------------------------------------------------

class TestProtocol(pv_wamp.PVServerProtocol):
    dataDir        = None
    authKey        = "vtkweb-secret"
    fileToLoad     = None
    groupRegex     = "[0-9]+\\."
    excludeRegex   = "^\\.|~$|^\\$"

    @staticmethod
    def updateArguments(options):
        TestProtocol.dataDir      = options.dataDir
        TestProtocol.authKey      = options.authKey
        TestProtocol.fileToLoad   = options.fileToLoad
        TestProtocol.groupRegex   = options.groupRegex
        TestProtocol.excludeRegex = options.excludeRegex

    def initialize(self):
        from paraview import simple
        from paraview.web import protocols as pv_protocols

        # Bring used components
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebFileListing(TestProtocol.dataDir, "Home", TestProtocol.excludeRegex, TestProtocol.groupRegex))
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebMouseHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPort())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortImageDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebViewPortGeometryDelivery())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebTimeHandler())
        self.registerVtkWebProtocol(pv_protocols.ParaViewWebRemoteConnection())

        # Update authentication key to use
        self.updateSecret(TestProtocol.authKey)


#------------------------------------------------------------------------------
# Start server
#------------------------------------------------------------------------------

paraviewHelper = pv_ipython.ParaViewIPython()
webArguments   = pv_ipython.WebArguments('%s/www' % build_path)

def start():
    paraviewHelper.Initialize(os.path.join(os.getcwd(), 'Testing', 'Temporary', 'mpi-python'))
    paraviewHelper.SetWebProtocol(TestProtocol, webArguments)
    return paraviewHelper.Start()

def start_thread():
    thread = threading.Thread(target=start)
    print ("Starting thread")
    thread.start()
    for i in range(20):
        print ("Working... %ds" % (i*5))
        time.sleep(5)
    thread.join()
    print ("Done")

#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------
if __name__ == "__main__":
    start_thread()
