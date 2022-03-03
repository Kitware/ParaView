## Remove TransferFunctionResetMode From General Settings

You can now set the automatic rescale mode for the color transfer function
only in the color editor. The other confusing option in general settings is
removed.

### API Changes

1. `vtkSMTransferFunctionProxy::ResetRescaleRangeModeToGlobalSetting` method
  is removed since a global setting
  no longer exists.

2. `vtkPVGeneralSettings` no longer has the `TransferFunctionResetMode` member
  variable.
