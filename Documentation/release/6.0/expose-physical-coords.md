## Expose physical cooridnate system

This change exposes on render view proxy the `SetPhysicalToWorldMatrix` method
(via a new `PhysicalToWorldMatrix` property), which is recently available on the
`vtkRenderWindow` class in VTK.

Some common representations have been updated to be aware of the already-existing
coordinate system api on underlying actors, via a new representation proxy property
(called `CoordinateSystem`).  This property allows users to place objects in World
or Physical coords.

Also `vtkPVLODActor` and `vtkPVLODVolume` have been updated to be aware that they
can be in non-world coordinate systems, and to recompute bounds when that is the
case.
