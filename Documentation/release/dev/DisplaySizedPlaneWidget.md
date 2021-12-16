## Display Sized Plane Widget

ParaView has a new plane widget which replaces the old plane widget.

The new plane widget is based on the `vtkDisplaySizedImplicitPlaneWidget` (details can be found in [display-sized-implicit-plane-widget](https://gitlab.kitware.com/vtk/vtk/-/blob/master/Documentation/release/dev/add-display-sized-implicit-plane-widget.md)).

`vtkPVRayCastPickingHelper` is now responsible for calculating both the intersection point and the normal.

`vtkSMRenderViewProxy` now actually returns if picking a point on the surface was successful or not.

`pqPointPickingHelper` can now pick a point or a normal and can enable or not (default is disabled) the picking of the camera focal point/normal.
