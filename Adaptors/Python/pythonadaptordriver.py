# This python script is an example of using the coprocessing library directly
# from a python code.  The user must properly set PYTHONPATH and LD_LIBRARY_PATH
# based on the installation on their machine.  Examples below are for a
# Linux machine.
# export PYTHONPATH=${PYTHONPATH}:<ParaView build directory>/Utilities/VTKPythonWrapping:<ParaView build directory>/VTK/Wrapping/Python:<ParaView build directory>/debuginstall/bin
#export LD_LIBRARY_PATH=<ParaView build directory>/bin:<Qt library directory>

import mpi  # available through pyMPI
print('process ', mpi.rank, ' is running!')

# set up for running paraview server in parallel
import paraview
paraview.options.batch = True
paraview.options.symmetric = True

# used to create the vtkUnstructuredGrid and add point data to it
import vtk

# arbitrary simulation parameters
numtimesteps =  10
starttime = 0.
endtime = 1.
timestep = (endtime-starttime)/numtimesteps

# importing the coprocessing libraries needed for the adaptor
import vtkCoProcessorPython

# this is the actual coprocessing script created from the ParaView plugin
import cp_pythonadaptorscript

for step in range(numtimesteps):
    currenttime = step*timestep
    datadescription = vtkCoProcessorPython.vtkCPDataDescription()
    datadescription.SetTimeData(currenttime, step)
    datadescription.AddInput("input")

    # determine if we want to perform coprocessing this time step
    cp_pythonadaptorscript.RequestDataDescription(datadescription)
    inputdescription = datadescription.GetInputDescriptionByName("input")

    if inputdescription.GetIfGridIsNecessary() == True:
        # since we do want to perform coprocessing, create grid and field data
        points = vtk.vtkPoints()
        for i in range(2):
            points.InsertNextPoint(float(i+mpi.rank), 0, 0)
            points.InsertNextPoint(float(i+mpi.rank), 0, 1)
            points.InsertNextPoint(float(i+mpi.rank), 1, 1)
            points.InsertNextPoint(float(i+mpi.rank), 1, 0)

        grid = vtk.vtkUnstructuredGrid()
        grid.SetPoints(points)
        pointIds = vtk.vtkIdList()
        for i in range(8):
            pointIds.InsertNextId(i)

        grid.InsertNextCell(vtk.VTK_HEXAHEDRON, pointIds)

        pointdata = vtk.vtkDoubleArray()
        for point in range(grid.GetNumberOfPoints()):
            loc = grid.GetPoint(point)
            pointdata.InsertNextValue(currenttime*loc[0])

        pointdata.SetName("varying array")
        grid.GetPointData().AddArray(pointdata)

        inputdescription.SetGrid(grid)
        cp_pythonadaptorscript.DoCoProcessing(datadescription)
