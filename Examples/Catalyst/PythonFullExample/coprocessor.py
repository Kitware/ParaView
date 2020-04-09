coProcessor = None

def initialize():
    global coProcessor

    import paraview
    import paraview.vtk as vtk
    import paraview.simple
    from paraview import servermanager
    from paraview.modules import vtkPVCatalyst as catalyst

    from mpi4py import MPI
    import os, sys

    paraview.options.batch = True
    paraview.options.symmetric = True

    if not servermanager.vtkProcessModule.GetProcessModule():
        pvoptions = None
        if paraview.options.batch:
            pvoptions = servermanager.vtkPVOptions();
            pvoptions.SetProcessType(servermanager.vtkPVOptions.PVBATCH)
            if paraview.options.symmetric:
                pvoptions.SetSymmetricMPIMode(True)
        servermanager.vtkInitializationHelper.Initialize(sys.executable, servermanager.vtkProcessModule.PROCESS_BATCH, pvoptions)

    coProcessor = catalyst.vtkCPProcessor()
    pm = paraview.servermanager.vtkProcessModule.GetProcessModule()

def finalize():
    global coProcessor
    coProcessor.Finalize()
    # if we are running through Python we need to finalize extra stuff
    # to avoid memory leak messages.
    import sys, ntpath
    if ntpath.basename(sys.executable) == 'python':
        servermanager.vtkInitializationHelper.Finalize()

def addscript(name):
    global coProcessor
    from paraview.modules import vtkPVPythonCatalyst as pythoncatalyst
    pipeline = pythoncatalyst.vtkCPPythonScriptPipeline()
    pipeline.Initialize(name)
    coProcessor.AddPipeline(pipeline)

def coprocess(time, timeStep, grid, attributes):
    global coProcessor
    import vtk
    from paraview.modules import vtkPVCatalyst as catalyst
    import paraview
    from paraview.vtk.util import numpy_support
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
