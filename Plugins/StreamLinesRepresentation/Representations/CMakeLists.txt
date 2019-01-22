set(classes
  vtkStreamLinesMapper
  vtkStreamLinesRepresentation)

set(sources)
set(private_headers)
set(shaders
  shaders/vtkStreamLines_fs.glsl
  shaders/vtkStreamLines_gs.glsl
  shaders/vtkStreamLines_vs.glsl
  shaders/vtkStreamLinesBlending_fs.glsl
  shaders/vtkStreamLinesCopy_fs.glsl)
foreach (shader IN LISTS shaders)
  get_filename_component(shader_name "${shader}" NAME_WE)
  vtk_encode_string(
    INPUT "${shader}"
    NAME  "${shader_name}"
    HEADER_OUTPUT shader_header
    SOURCE_OUTPUT shader_source)
  list(APPEND sources
    "${shader_source}")
  list(APPEND private_headers
    "${shader_header}")
endforeach ()

vtk_module_add_module(StreamLinesRepresentation::vtkStreamLines
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})

paraview_add_server_manager_xmls(
  XMLS  StreamLinesRepresentation.xml)
