from paraview.catalyst import bridge

def initialize():
    # initialize ParaView Catalyst.
    bridge.initialize()

def finalize():
    # finalize ParaView Catalyst.
    bridge.finalize()

def addscript(filename):
    bridge.add_pipeline(filename)

def coprocess(time, timeStep, grid, attributes):
    from paraview import vtk
    from paraview.modules import vtkPVCatalyst as catalyst
    from paraview.vtk.util import numpy_support

    # access coprocessor from the bridge
    coProcessor = bridge.coprocessor

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
