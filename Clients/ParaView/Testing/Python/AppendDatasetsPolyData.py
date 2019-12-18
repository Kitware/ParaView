#### Verify that Append Datasets produces polydata output when inputs are polydata
from paraview.simple import *
from vtkmodules.util import vtkConstants

# create a new 'Sphere'
sphere1 = Sphere()
sphere2 = Sphere()
sphere2.Center = [1.0, 0.0, 0.0]

# create a new 'Append Datasets'
appendDatasets = AppendDatasets(Input=[sphere1, sphere2])
appendDatasets.UpdatePipeline()

# By default the output data type of appendDatasets is vtkUnstructuredGrid
assert(appendDatasets.GetDataInformation().GetDataSetTypeAsString() == 'vtkUnstructuredGrid')

# Ensure that the output data type of appendDatasets is vtkPolyData
appendDatasets.OutputDataSetType = vtkConstants.VTK_POLY_DATA
appendDatasets.UpdatePipeline()
assert(appendDatasets.GetDataInformation().GetDataSetTypeAsString() == 'vtkPolyData')

# Now add in a non-polydata source. It will be skipped in the append
unstructured = UnstructuredCellTypes()
appendDatasets.Input = [sphere1, sphere2, unstructured]
appendDatasets.UpdatePipeline()

# Ensure that the output data type of appendDatasets is still vtkPolyData
assert(appendDatasets.GetDataInformation().GetDataSetTypeAsString() == 'vtkPolyData')

# Ensure output number of cells is the sum of the number of cells in the vtkPolyData input.
# The unstructured grid will be skipped when appending blocks.
sphere1NumCells = sphere1.GetDataInformation().GetNumberOfCells()
sphere2NumCells = sphere2.GetDataInformation().GetNumberOfCells()
outputNumCells = appendDatasets.GetDataInformation().GetNumberOfCells()

assert(sphere1NumCells + sphere2NumCells == outputNumCells)
