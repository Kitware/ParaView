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
* In `pqPropertiesPanel`, the `setAutoApplyDelay()` and `autoApplyDelay()`
  methods have been removed (deprecated in 5.11). Use the methods on
  `pqApplyBehavior` instead.
* `pqVCRController::playing(bool)` signal has been removed (deprecated in
  5.11). Use the `::playing(bool, bool)` signal instead.
* In `pqAnimationScene`, the `beginPlay()` and `endPlay()` signals have been
  removed (deprecated in 5.11). Use the variants with `vtkCallback` APIs
  instead.
* In `pqRenderView`, the `selectOnSurface()` method has been removed in favor
  of `selectCellsOnSurface` and `selectFrustum` in favor of
  `selectFrustumCells` (deprecated in 5.11).
* `pqServerResource::sessionServer()` and `::setSessionServer()` methods have
  been removed (deprecated in 5.11). Any calls should just be removed and the
  resource used directly instead.
* Remove `pqTimelineScrollbar` (deprecated in 5.11). Use
  `pqTimeInspectorWidget` instead.
* Remove `vtkPVPluginsInformation::PluginRequirementsSatisfied()` method
  (deprecated in 5.11). Use `vtkSMProxyManager::SatisfyPluginRequirements()`
  instead.
* Remove `vtkPVRenderView` `VolumeAnisotropy` methods (deprecated in 5.11). Use
  `vtkVolumeProperty`'s `ScatteringAnisotropy` methods instead.
* Remove `vtkPVGeometryFilter` `UseStrips` and `ForceUseStripsOn` methods
  (deprecated in 5.11).
* In `paraview.simple`, remove the `Show3DWidgets` and `Hide3DWidgets`
  functions (deprecated in 5.11). Use `ShowInteractiveWidgets` and
  `HideInteractiveWidgets` instead.
