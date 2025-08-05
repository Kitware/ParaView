set(classes
  vtkSMVRControlLocationEgoStyleProxy
  vtkSMVRControlOrientationStyleProxy
  vtkSMVRControlLocationStyleProxy
  vtkSMVRTravelGrabEgoStyleProxy
  vtkSMVRTravelGrabExoSplitStyleProxy
  vtkSMVRInteractorStyleProxy
  vtkSMVRControlLocationRelativeStyleProxy
  vtkSMVRPythonInteractorStyleProxy
  vtkSMVRDebugStyleProxy
  vtkSMVRTravelTrackballExoSplitStyleProxy
  vtkSMVRTrackStyleProxy
  vtkVRQueue)

vtk_module_add_module(ParaViewIncubator::CAVEInteractionStyles
  CLASSES ${classes})
