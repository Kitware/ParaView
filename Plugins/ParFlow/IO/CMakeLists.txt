set(classes
  vtkParFlowReader
  vtkParFlowMetaReader
)

find_package(nlohmann_json REQUIRED)

vtk_module_add_module(ParFlow::IO
  CLASSES ${classes}
)
target_link_libraries(IO
  LINK_PUBLIC nlohmann_json::nlohmann_json
)

paraview_add_server_manager_xmls(
  XMLS  ParFlowIO.xml
)
