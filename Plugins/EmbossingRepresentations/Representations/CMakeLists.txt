set(classes
  vtkBumpMapMapper
  vtkBumpMapRepresentation
  vtkExtrusionMapper
  vtkExtrusionRepresentation)

set(shaders
  shaders/vtkExtrudeCell_gs.glsl)
set(sources)
set(private_headers)

foreach (shader IN LISTS shaders)
  vtk_encode_string(
    INPUT "${shader}"
    HEADER_OUTPUT shader_header
    SOURCE_OUTPUT shader_source)
  list(APPEND private_headers
    "${shader_header}")
  list(APPEND sources
    "${shader_source}")
endforeach ()

vtk_module_add_module(EmbossingRepresentations::vtkEmbossingRepresentations
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})

paraview_add_server_manager_xmls(
  XMLS  EmbossingRepresentations.xml)
