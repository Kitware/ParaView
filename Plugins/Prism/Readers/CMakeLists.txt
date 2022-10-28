set(classes
  vtkPrismSESAMEFileSeriesReader
  vtkPrismSESAMEReader
  )

vtk_module_add_module(Prism::vtkPrismReaders
  CLASSES ${classes}
  )

paraview_add_server_manager_xmls(
  XMLS PrismReaders.xml
)
