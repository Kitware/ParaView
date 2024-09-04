set(classes
  vtkSMVRControlSliceOrientationStyleProxy
  vtkSMVRControlSlicePositionStyleProxy
  vtkSMVRGrabPointStyleProxy
  vtkSMVRGrabTransformStyleProxy
  vtkSMVRGrabWorldStyleProxy
  vtkSMVRInteractorStyleProxy
  vtkSMVRMovePointStyleProxy
  vtkSMVRPythonInteractorStyleProxy
  vtkSMVRResetTransformStyleProxy
  vtkSMVRSkeletonStyleProxy
  vtkSMVRStylusStyleProxy
  vtkSMVRTrackStyleProxy
  vtkSMVRVirtualHandStyleProxy
  vtkVRQueue)

vtk_module_add_module(ParaViewIncubator::CAVEInteractionStyles
  CLASSES ${classes})
