## Expose vtkHyperTreeGridEvaluateCoarse in Paraview

Expose the missing filter `vtkHyperTreeGridEvaluateCoarse` in ParaView.
You can set the calculation method and the default value for different "Average" methods.

The operators available for the different calculation methods are:
* Don't Change Fast: does not change coarse value (default), just shallow copy
* Don't Change: does not change coarse value but iterate over all cells, just shallow copy
* Min: the smallest value of the unmasked child cells
* Max: the biggest value of the unmasked child cells
* Sum: the sum of the values of the unmasked child cells
* Average: the average of the values of the child cells
* Unmasked Average: the average of the values of the unmasked child cells
* Splatting Average: The splatting average of the values of the child cells
* Elder Child: puts the value of the first child (elder child)

The calculation of the average should normally be done by dividing by the number of
children which is worth f^d where f, refinement factor and d, number of spatial
dimension. In the calculation of the mean for splatting, the division involves f^(d-1).
