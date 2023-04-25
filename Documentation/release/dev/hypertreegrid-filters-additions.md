## HyperTreeGrid filters additions

### HyperTreeGrid Evaluate Coarse

You can now use the `vtkHyperTreeGridEvaluateCoarse` filter in ParaView.
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
children which is worth f^d, where f is the refinement factor and d the number of spatial
dimensions. In the calculation of the mean for splatting, the division involves f^(d-1).

### HyperTreeGrid Geometry Filter

You can now use the geometry filter to convert an HTG to a PolyData.
The filter was already defined in ParaView but not exposed in any list
so it was still inaccessible.

### HyperTreeGrid Gradient

#### Unlimited mode

Extend the Hyper tree grid version of the gradient filter with a new
**UNLIMITED** mode. In this version, the gradient is computed using unlimited
cursors, refining neighboring nodes to have a local computation similar to
a regular grid.

In this mode, it is possible to handle extensive attributes so the
virtual subdivision reduces their influence.

#### New parameters

The Divergence, Vorticity and QCriterion are now exposed when computing the
Gradient of an Hyper Tree Grid. Vector fields are now also supported.

### Minor tweaks

`vtkHyperTreeGridCellCenters` now creates vertex cells by default as
it is the most common use case.

`Slice` meta filter now hides properly the "value range" property
when applied to HyperTreeGrid.

HyperTreeGrid filters are now named consistently, beginning with
"HyperTreeGrid <some_filter_name>".
