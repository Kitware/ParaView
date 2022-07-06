## Expose SnapToCellWithClosestPoint ResampleWithDataSet

ResampleWithDataSet has a new flag named SnapToCellWithClosestPoint which allows you to snap to the cell with the
closest point if no cell has been found using FindCell. This flag is only useful when the source is a vtkPointSet.
