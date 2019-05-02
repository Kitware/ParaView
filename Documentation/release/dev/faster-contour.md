# Faster **Contour** filter datasets with linear cells

For certain inputs, this filter delegates operation to vtkContour3DLinearGrid.
The input dataset must meet the following conditions for this filter to be used:

* all cells in the input are linear (i.e., not higher order)
* the contour array is one of the types supported by the vtkContour3DLinearGrid
* the *Compute Scalars* option is off
