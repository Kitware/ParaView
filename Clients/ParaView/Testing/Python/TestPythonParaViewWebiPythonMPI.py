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
from vtk.vtkParaViewWebCore import *
from vtk.vtkPVClientWeb import *
from vtk.vtkRemotingCore import *
from vtk.vtkRemotingApplication import *
from vtk.vtkRemotingServerManager import *
from vtk.vtkRemotingViews import *
from vtk import *

#------------------------------------------------------------------------------
# Start server
#------------------------------------------------------------------------------

paraviewHelper = pv_ipython.ParaViewIPython()
webArguments   = pv_ipython.WebArguments('%s/www' % build_path)
sphere = None

def start():
    paraviewHelper.Initialize(os.path.join(os.getcwd(), 'Testing', 'Temporary', 'mpi-python'))
    pv_ipython.IPythonProtocol.updateArguments(webArguments)
    paraviewHelper.SetWebProtocol(pv_ipython.IPythonProtocol, webArguments)
    return paraviewHelper.Start()

def start_thread():
    # Register some data at startup
    global sphere
    position = [random.random() * 2, random.random() * 2, random.random() * 2]
    sphere = vtkSphereSource()
    sphere.SetCenter(position)
    sphere.Update()
    pv_ipython.IPythonProtocol.RegisterDataSet('iPython-demo', sphere.GetOutput())

    # Start root+satelites
    thread = threading.Thread(target=start)
    print ("Starting thread")
    thread.start()
    for i in range(20):
        print ("Working... %ds" % (i*5))
        position = [random.random() * 2, random.random() * 2, random.random() * 2]
        print (position)
        sphere.SetCenter(position)
        sphere.Update()
        pv_ipython.IPythonProtocol.RegisterDataSet('iPython-demo', sphere.GetOutput())
        time.sleep(5)
        pv_ipython.IPythonProtocol.ActivateDataSet('iPython-demo')
    thread.join()
    print ("Done")

#------------------------------------------------------------------------------
# Main
#------------------------------------------------------------------------------
if __name__ == "__main__":
    start_thread()
