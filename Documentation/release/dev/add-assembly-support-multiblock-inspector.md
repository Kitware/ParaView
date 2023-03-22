## Add Assembly Support to Multiblock Inspector

The `Multiblock Inspector` now has the option to choose the assembly type. If the dataset
is `vtkPartitionedDataSetCollection` then the available assembly options are `Assembly` and `Hierarchy`. If
the dataset is `vtkUniformGridAMR` or `vtkMultiBlockDataSet` then the only available assembly option is `Hierarchy`.
The default assembly is `Assembly` if present, otherwise it's `Hierarchy`. Finally, the default has also changed for
the `Find Data Panel`, and `ExtractBlock` in the same way.
