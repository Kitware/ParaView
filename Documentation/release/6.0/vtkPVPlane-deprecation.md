## vtkPVPlane deprecation

vtkPlane now supports AxisAligned and Offset options.
Therefore, vtkPVPlane is now deprecated. Please consider using vtkPlane instead.

Please note that the protected members of vtkPVPlane have been modified without deprecation, this may have an impact if you were inheriting it, please consider inheriting vtkPlane instead.
