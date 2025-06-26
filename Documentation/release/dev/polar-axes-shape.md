## Annotations Grid improvements

### PolarAxes data representation
The **Transform** property has better consistency:
* The **Transform** property now considers custom bounds, not only input data bounds.
* A new **PolePosition** property has been added to define the pole position, and the **Translation** property is no longer used to determine it.

When the min and max angles should be computed (**CustomAngles** off), we have different cases regarding the pole position:
* when the pole is in inside the bounds, this leads to a 0-360 degrees polar axes.
* when the pole is outside the bounds, angles are computed to exactly cover the bounds.
* when the pole is on a boundary, we now do the actual computation too.

Finally, different UI parts are now grouped with related properties.

### PolarGrid and LegendGrid View annotations
The **LegendGrid** and the **PolarGrid** annotations are only meaningful when the render view has **ParallelProjection** set to on.
Turning **ParallelProjection** off now hide those annotations. See the issue [#22934](https://gitlab.kitware.com/paraview/paraview/-/issues/22934).
