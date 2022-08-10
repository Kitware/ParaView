## Remove the globalID generation in the parallel versions of the Ensight Reader

The generation of GlobalNodeId in the parallel versions of the Ensight reader (`vtkPEnSightGoldReader` and `vtkPEnSightGoldBinaryReader`)
can lead to duplicate ids. This makes some filters crash afterward (notably `vtkGhostCellsGenerator` and `vtkRedistributeDatasetFilter`).

As creating globalIds in the readers is not needed anymore, we remove it to resolve the problem.
