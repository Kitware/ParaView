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
  vtkSMVRSpaceNavigatorGrabWorldStyleProxy
  vtkSMVRStylusStyleProxy
  vtkSMVRTrackStyleProxy
  vtkSMVRVirtualHandStyleProxy
  vtkVRQueue)

vtk_module_add_module(ParaViewIncubator::CAVEInteractionStyles
  CLASSES ${classes})
