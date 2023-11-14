## deprecation-removals

* The `SurfaceLIC` plugin has been removed (deprecated in 5.11).
* In `pqAnimationManager`, the `beginPlay()` and `endPlay()` argument-less
  signals have been removed (deprecated in 5.11). Use the variants with
  `vtkCallback` APIs instead.
* In `pqAnimationManager`, the `onBeginPlay()` and `onEndPlay()` argument-less
  slots have been removed (deprecated in 5.11). the variants with `vtkCallback`
  APIs instead.
* In `pqPropertiesPanel`, the `setAutoApply()` and `autoApply()` methods have
  been removed (deprecated in 5.11). Use the methods on `vtkPVGeneralSettings`
  instead.
