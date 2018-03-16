# Specify a Cell Locator to be used by the ResampleWithDataSet filter

`vtkProbeFilter`, the underlying filter used by `ResampleWithDataSet`, now uses
a `vtkAbstractCellLocator` during the resampling process. With this change, the
type of cell locator can be specified in paraview.
