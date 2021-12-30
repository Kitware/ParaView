## Coordinate Frame Widget

ParaView now has a widget for manipulating coordinate frames.
A coordinate frame is all the information needed to uniquely specify
a linear transformation:

+ a base point in world coordinates
+ three ortho-normal vectors defining new coordinate axes in terms of the existing world coordinates.

If you have a group of properties containing 4 double-vectors, you can
add `panel_widget="InteractiveFrame"` to the group's XML attributes and
it will appear in the properties panel as a `pqCoordinateFramePropertyWidget`.

See [this VTK discourse topic](https://discourse.vtk.org/t/vtkcoordinateframewidget/) for
more information about how to use the widget.
