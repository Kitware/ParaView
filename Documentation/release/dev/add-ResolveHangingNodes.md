## Resolve Hanging Nodes filter and weighted Cell Data to Point Data

A new **Resolve Hanging Nodes** filter resolves the hanging nodes of 2D or 3D multi-resolution
meshes made of quads or hexahedra, such as AMR datasets converted to unstructured grids or polydata:
the points of finer cells that hang on an edge or face of a coarser neighbor are inserted in the
coarse cell, so that point data interpolation is continuous across refinement levels. With
**Decompose** (the default), the modified cells are decomposed into triangles and quads (2D), or
tetrahedra and pyramids (3D); otherwise, they become single polygons or polyhedra. In parallel, the
points that hang on the boundary of a partition are exchanged with the neighboring partitions.
**Tolerance** sets how close to an edge or face a point must be to be inserted in it, and **Pass
Original Cell Ids** adds a cell array with the input cell of every output cell.

With **Compute Inverse Measure**, the filter also outputs an `InverseArea` (2D) or `InverseVolume`
(3D) cell array. The **Cell Data to Point Data** filter has a new **Enable Weighted Average**
option, with a **Weight Array** to choose such a cell array: the values of the cells using a point
are then averaged with these weights, so that the value of a point does not depend on how many
finer cells use it.
