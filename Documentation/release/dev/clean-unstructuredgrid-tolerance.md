# Add tolerance to vtkCleanUnstructuredGrid

This patch adds tolerance to the vtkCleanUnstructuredGrid class like it already exists in vtkCleanPolyData.

So far, the vtkCleanUnstructuredGrid rounds all coordinates first to float precision, then directly compares for equality (like x1 == x2) - which is never a good idea for float or double numbers. The reason is that points are fed into a vtkPoints object that has float precision by default.

The latter can be changed by writing a vtkObjectFactory derived factory class for vtkPoints with double precision and registering it. But then the effect is only that the vtkCleanUnstructuredGrid class will not have much effect any more - because then the direct comparison will be done at double precision level, so points will hardly ever coincide any more and no merging will happen.

Here these patches will come into the game because now a tolerance can be specified, in absolute or relative terms. With that, the merging can be exactly adapted to the geometry of the problem.
