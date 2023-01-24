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

set(sm_xml_files CAVEInteraction_SM.xml)

if (PARAVIEW_USE_PYTHON)
  list(APPEND style_classes vtkSMVRPythonInteractorStyleProxy)
  list(APPEND sm_xml_files CAVEInteraction_SM_Python.xml)
endif ()

vtk_module_add_module(CAVEInteraction::Styles
  CLASSES ${style_classes})

paraview_add_server_manager_xmls(
  XMLS  ${sm_xml_files})
