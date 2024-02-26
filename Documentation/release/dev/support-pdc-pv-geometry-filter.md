## vtkPVGeometryFilter: Support creating vtkPartitionedDataSetCollection as output

This MR adds support for creating `vtkPartitionedDataSetCollection` as output
from `vtkPVGeometryFilter`. This is useful when the input is a `vtkPartitionedDataSet`
`vtkUniformGridAMR`, or `vtkPartitionedDataSetCollection`.
