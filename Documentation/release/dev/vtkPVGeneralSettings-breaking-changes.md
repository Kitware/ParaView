### Developper notes: vtkPVGeneralSettings breaking changes

The `vtkPVGeneralSettings` class was updated to limits its dependency to other
modules at the strict minimum.
During this work, its members were moved from `protected` to `private`.
Please use the getters and setters instead.
