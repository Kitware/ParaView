## Use vtkGeometryFilter for Rendering/ExtractSurface

`vtkPVGeometryFilter` and `Extract Surface` now use `vtkGeometryFilter` instead of `vtkDataSetSurfaceFilter`.
Due to this change, `UseGeometryFilter` flag has been removed. Finally, `RemoveGhostInterfaces` flag has been
added which allows the removal of ghost interfaces when surface is extracted.
