set(classes
  vtkDataMineBlockReader
  vtkDataMineDrillHoleReader
  vtkDataMineDummyReader
  vtkDataMinePerimeterReader
  vtkDataMinePointReader
  vtkDataMineReader
  vtkDataMineWireFrameReader)

set(private_classes
  ThirdParty/dmfile
  ThirdParty/PointMap
  ThirdParty/PropertyStorage)

set(private_headers
  ThirdParty/dm.h)

vtk_module_add_module(Datamine::DatamineReaders
  CLASSES ${classes}
  PRIVATE_CLASSES ${private_classes}
  PRIVATE_HEADERS ${private_headers})

paraview_add_server_manager_xmls(
  XMLS DatamineReaders.xml)
