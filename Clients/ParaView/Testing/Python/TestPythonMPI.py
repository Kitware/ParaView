#/usr/bin/env python

# Global python import
import exceptions, logging, random, sys, threading, time, os

# Update python path to have ParaView libs
build_path='/Volumes/SebKitSSD/Kitware/code/ParaView/build-ninja'
sys.path.append('%s/lib'%build_path)
sys.path.append('%s/lib/site-packages'%build_path)

# iPython import
#from IPython.display import HTML
#from IPython.parallel import Client
import paraview
from paraview.web import ipython as pv_ipython
from vtk import *

iPythonClient = None
paraviewHelper = pv_ipython.ParaViewIPython()
webArguments = pv_ipython.WebArguments('/.../path-to-web-directory')

def _start_paraview():
    paraviewHelper.Initialize()

    paraviewHelper.SetWebProtocol(IPythonProtocol, webArguments)
    return paraviewHelper.Start()


def _stop_paraview():
    paraviewHelper.Finalize()


def _pv_activate_dataset():
    IPythonProtocol.ActivateDataSet('iPython-demo')


def _push_new_timestep():
    # processing code generating new vtkDataSet
    # newDataset = ...
    IPythonProtocol.RegisterDataSet('iPython-demo', newDataset)


def StartParaView(height=600, path='/apps/WebVisualizer/'):
    global iPythonClient, paraviewHelper
    if not iPythonClient:
        iPythonClient = Client()
    urls = iPythonClient[:].apply_sync(lambda:_start_paraview())
    url = ""
    for i in urls:
        if len(i) > 0:
            url = i
    return  HTML("<iframe src='%s/%s' width='100%%' height='%i'></iframe>"%(url, path, height))


def StopParaView():
    global iPythonClient, paraviewHelper
    iPythonClient[:].apply_sync(lambda:_stop_paraview())


def ActivateDataSet():
    iPythonClient[:].apply_sync(lambda:_pv_activate_dataset())


def ComputeNextTimeStep(ds):
    iPythonClient[:].apply_sync(lambda:_push_new_timestep())


print ("Start waiting")
time.sleep(10)
print ("Done")
