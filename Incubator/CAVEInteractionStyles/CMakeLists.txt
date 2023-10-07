set(classes
  vtkSMVRGrabTransformStyleProxy
  vtkSMVRControlSlicePositionStyleProxy
  vtkSMVRResetTransformStyleProxy
  vtkSMVRMovePointStyleProxy
  vtkSMVRSkeletonStyleProxy
  vtkSMVRControlSliceOrientationStyleProxy
  vtkSMVRGrabPointStyleProxy
  vtkSMVRSpaceNavigatorGrabWorldStyleProxy
  vtkSMVRStylusStyleProxy
  vtkSMVRVirtualHandStyleProxy
  vtkSMVRGrabWorldStyleProxy
  vtkVRQueue
  vtkSMVRInteractorStyleProxy
  vtkSMVRTrackStyleProxy)

if (TARGET VTK::WrappingPythonCore)
  list(APPEND classes
    vtkSMVRPythonInteractorStyleProxy)
endif ()

vtk_module_add_module(ParaViewIncubator::CAVEInteractionStyles
  CLASSES ${classes})
