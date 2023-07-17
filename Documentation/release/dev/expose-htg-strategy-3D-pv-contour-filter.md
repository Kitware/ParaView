## Expose the HTG Strategy 3D property in vtkPVContourFilter

In case of a HTG input, the vtkPVContourFilter now exposes the `HTG Strategy 3D` property.
It forwards the property to the internal `vtkHyperTreeGridContour` filter.
This property allows better results in the 3D case but is significantly more time-consuming.
