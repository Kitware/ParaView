import paraview
import vtkParallelCorePython
import vtk
from mpi4py import MPI
import os, sys

paraview.options.batch = True
paraview.options.symmetric = True
from vtkPVClientServerCoreCorePython import *
try:
    from vtkPVServerManagerApplicationPython import *
except:
    paraview.print_error("Error: Cannot import vtkPVServerManagerApplicationPython")

if not vtkProcessModule.GetProcessModule():
    pvoptions = None
    if paraview.options.batch:
        pvoptions = vtkPVOptions();
        pvoptions.SetProcessType(vtkPVOptions.PVBATCH)
        if paraview.options.symmetric:
            pvoptions.SetSymmetricMPIMode(True)
    vtkInitializationHelper.Initialize(sys.executable, vtkProcessModule.PROCESS_BATCH, pvoptions)

import paraview.servermanager as pvsm
# we need ParaView 4.2 since ParaView 4.1 doesn't properly wrap
# vtkPVPythonCatalystPython
if pvsm.vtkSMProxyManager.GetVersionMajor() != 4 or \
        pvsm.vtkSMProxyManager.GetVersionMinor() !=2:
    print 'Must use ParaView v4.2'
    sys.exit(0)

import numpy
import vtkPVCatalystPython as catalyst
import vtkPVPythonCatalystPython as pythoncatalyst
import paraview.simple
import paraview.vtk as vtk
from paraview import numpy_support
paraview.options.batch = True
paraview.options.symmetric = True
from vtkPVClientServerCoreCorePython import *
try:
    from vtkPVServerManagerApplicationPython import *
except:
    paraview.print_error("Error: Cannot import vtkPVServerManagerApplicationPython")

# set up ParaView to properly use MPI
coProcessor = None

usecp = True

def initialize():
    global coProcessor, usecp
    if usecp:
        coProcessor = catalyst.vtkCPProcessor()

    pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
    from mpi4py import MPI

def finalize():
    global coProcessor, usecp
    if usecp:
        coProcessor.Finalize()

def addscript(name):
    global coProcessor
    pipeline = pythoncatalyst.vtkCPPythonScriptPipeline()
    pipeline.Initialize(name)
    coProcessor.AddPipeline(pipeline)

def coprocess(time, timeStep, grid, attributes):
    global coProcessor
    dataDescription = catalyst.vtkCPDataDescription()
    dataDescription.SetTimeData(time, timeStep)
    dataDescription.AddInput("input")

    if coProcessor.RequestDataDescription(dataDescription):
        import fedatastructures
        imageData = vtk.vtkImageData()
        imageData.SetExtent(grid.XStartPoint, grid.XEndPoint, 0, grid.NumberOfYPoints-1, 0, grid.NumberOfZPoints-1)
        imageData.SetSpacing(grid.Spacing)

        velocity = numpy_support.numpy_to_vtk(attributes.Velocity)
        velocity.SetName("velocity")
        imageData.GetPointData().AddArray(velocity)

        pressure = numpy_support.numpy_to_vtk(attributes.Pressure)
        pressure.SetName("pressure")
        imageData.GetCellData().AddArray(pressure)
        dataDescription.GetInputDescriptionByName("input").SetGrid(imageData)
        dataDescription.GetInputDescriptionByName("input").SetWholeExtent(0, grid.NumberOfGlobalXPoints-1, 0, grid.NumberOfYPoints-1, 0, grid.NumberOfZPoints-1)
        coProcessor.CoProcess(dataDescription)


