# Improvements to pqApplyBehavior

- pqApplyBehavior can now be used without registering any pqPropertiesPanel
- pqApplyBehavior is now responsible for the auto apply mechanism
- AutoApply is now solely controlled using vtkPVGeneralSettings
- AutoApply can now be used without registering any pqPropertiesPanel
- A new method pqPVApplicationCore::applyPipeline let developers apply a pipeline without need to use a pqPropertiesPanel
- QuickLaunch dialog now support quick apply by pressing the shift key when selecting a filter or a source to apply
