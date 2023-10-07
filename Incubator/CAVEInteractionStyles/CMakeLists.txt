set(classes
  vtkSMVRControlSliceOrientationStyleProxy
  vtkSMVRControlSlicePositionStyleProxy
  vtkSMVRGrabPointStyleProxy
  vtkSMVRGrabTransformStyleProxy
  vtkSMVRGrabWorldStyleProxy
  vtkSMVRInteractorStyleProxy
  vtkSMVRMovePointStyleProxy
  vtkSMVRResetTransformStyleProxy
  vtkSMVRSkeletonStyleProxy
  vtkSMVRSpaceNavigatorGrabWorldStyleProxy
  vtkSMVRStylusStyleProxy
  vtkSMVRTrackStyleProxy
  vtkSMVRVirtualHandStyleProxy
  vtkVRQueue)

if (TARGET VTK::WrappingPythonCore)
  list(APPEND classes
    vtkSMVRPythonInteractorStyleProxy)
endif ()

vtk_module_add_module(ParaViewIncubator::CAVEInteractionStyles
  CLASSES ${classes})
