#### Verify that Merge Blocks produces polydata output when inputs are polydata
from paraview.simple import *
from vtkmodules.util import vtkConstants

# create a new 'Sphere'
sphere1 = Sphere()
sphere2 = Sphere()
sphere2.Center = [1.0, 0.0, 0.0]

# create a new 'Group Datasets'
groupDatasets = GroupDatasets(Input=[sphere1, sphere2])

# create a new 'Merge Blocks'
mergeBlocks = MergeBlocks(Input=groupDatasets)
mergeBlocks.UpdatePipeline()

# By default the output data type of appendDatasets is vtkUnstructuredGrid
assert(mergeBlocks.GetDataInformation().GetDataSetTypeAsString() == 'vtkUnstructuredGrid')

# Ensure that the output data type of mergeBlocks is vtkPolyData
mergeBlocks.OutputDataSetType = vtkConstants.VTK_POLY_DATA
mergeBlocks.UpdatePipeline()
assert(mergeBlocks.GetDataInformation().GetDataSetTypeAsString() == 'vtkPolyData')

# Now add in a non-polydata source
unstructured = UnstructuredCellTypes()
groupDatasets.Input = [sphere1, sphere2, unstructured]
groupDatasets.UpdatePipeline()
mergeBlocks.UpdatePipeline()

# Ensure that the output data type of appendDatasets is still vtkPolyData
assert(mergeBlocks.GetDataInformation().GetDataSetTypeAsString() == 'vtkPolyData')

# Ensure output number of cells is the sum of the number of cells in the vtkPolyData input.
# The unstructured grid will be skipped when merging blocks.
sphere1NumCells = sphere1.GetDataInformation().GetNumberOfCells()
sphere2NumCells = sphere2.GetDataInformation().GetNumberOfCells()
outputNumCells = mergeBlocks.GetDataInformation().GetNumberOfCells()

assert(sphere1NumCells + sphere2NumCells == outputNumCells)
