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

vtk_module_add_module(CAVEInteraction::Styles
  CLASSES ${style_classes})

paraview_add_server_manager_xmls(
  XMLS  ${sm_xml_files})
