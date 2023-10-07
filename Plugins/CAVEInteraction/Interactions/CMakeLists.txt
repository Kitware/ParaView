set(style_classes
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

if (PARAVIEW_USE_PYTHON)
  list(APPEND style_classes vtkSMVRPythonInteractorStyleProxy)
endif ()

vtk_module_add_module(CAVEInteraction::Styles
  CLASSES ${style_classes})
