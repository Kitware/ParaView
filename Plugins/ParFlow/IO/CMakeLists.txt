set(classes
  vtkParFlowReader
  vtkParFlowMetaReader
)

set(private_headers
  vtkVectorJSON.h
)

vtk_module_add_module(ParFlow::ParFlowIO
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers}
)

paraview_add_server_manager_xmls(
  XMLS  ParFlowIO.xml
)
