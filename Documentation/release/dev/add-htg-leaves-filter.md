## Expose Hyper Tree Grid Compute Visible Leaves Size filter

A new Hyper Tree Grid utility filter is exposed : HyperTreeGridVisibleLeavesSize.
This filter creates 2 new cell fields:
- `ValidCell` has a (double) value of 1.0 for visible (non ghost, non masked) leaf cells, and 0.0 for the others.
- `CellSize`'s value corresponds to the volume of the cell in 3D, or area in 2D.
