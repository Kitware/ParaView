set(classes
  vtkParFlowReader
  vtkParFlowMetaReader
)

set(private_headers
  vtkVectorJSON.h
)

vtk_module_find_package(
  PACKAGE nlohmann_json)

vtk_module_add_module(ParFlow::IO
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers}
)
vtk_module_link(ParFlow::IO
  PUBLIC nlohmann_json::nlohmann_json
)

paraview_add_server_manager_xmls(
  XMLS  ParFlowIO.xml
)
