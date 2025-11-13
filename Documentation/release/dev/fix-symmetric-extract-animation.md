## vtkSMSaveAnimationExtractsProxy: Ensure all timesteps are extracted in symmetric pvbatch mode

In symmetric mode, `vtkSMProperty` did not allow using domain's default values because they are not guaranteed to be the
same across all processes. This caused `vtkSMSaveAnimationExtractsProxy` to not extract all timesteps when using
`pvbatch` in symmetric mode. To fix this, `vtkSMProperty::SetDefaultValues` now checks if the domain is one of
`vtkSMFileListDomain`, `vtkSMFrameStrideQueryDomain` or `vtkSMAnimationFrameWindowDomain`, which are domains used
by `vtkSMSaveAnimationExtractsProxy`, and allows using default values from those domains in symmetric mode. This is
acceptable because those domains are guaranteed to return the same default values across all processes, because the
information provided by those domains is guaranteed to be the same across all processes at the reader level. Finally,
thanks to this fix, the SaveAnimation test can be enabled for symmetric mode.
