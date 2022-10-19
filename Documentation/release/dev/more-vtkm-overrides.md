## Added VTK-m accelerated overrides for Clip, Slice and Threshold filters

VTK-m accelerated overrides for the `Clip`, `Slice` and `Threshold` filters are
available when ParaView is built with the cmake option `VTK_ENABLE_VTKM_OVERRIDES` turned on.
The overrides are used when the setting `Settings > General > Use Accelerated Filters` is on.
