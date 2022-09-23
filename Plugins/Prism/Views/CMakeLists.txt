set(classes
  vtkPrismGeometryRepresentation
  vtkPrismGeometryRepresentationWithFaces
  vtkPrismSelectionRepresentation
  vtkPrismView
  vtkSMPrismViewProxy
  )

vtk_module_add_module(Prism::vtkPrismViews
  CLASSES ${classes}
  )

paraview_add_server_manager_xmls(
  XMLS PrismViews.xml
)
