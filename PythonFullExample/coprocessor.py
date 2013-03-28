import numpy
import paraview
import vtkPVCatalystPython as catalyst
import vtkPVPythonCatalystPython as pythoncatalyst
import paraview.simple
import paraview.vtk as vtk
from paraview import numpy_support

# set up ParaView to properly use MPI
globalController = None
coProcessor = None

usecp = True

def initialize():
    global globalController, coProcessor, usecp
    if usecp:
        coProcessor = pythoncatalyst.vtkCPPythonProcessor()
        coProcessor.Initialize()


    pm = paraview.servermanager.vtkProcessModule.GetProcessModule()
    globalController = pm.GetGlobalController()
    from mpi4py import MPI
    # we only need to use the vtkMPIController if we have more than a single process
    if MPI.COMM_WORLD.Get_size() > 1 and (globalController == None or globalController.IsA("vtkDummyController") == True):
        import vtkParallelMPIPython
        globalController = vtkParallelMPIPython.vtkMPIController()
        globalController.Initialize()
        globalController.SetGlobalController(globalController)

def finalize():
    global globalController, coProcessor, usecp
    if usecp:
        coProcessor.Finalize()
    if globalController:
        globalController.SetGlobalController(None)
        globalController.Finalize()
        globalController = None

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


