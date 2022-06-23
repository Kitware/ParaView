set(classes
  vtkLANLX3DReader)

set(sources
  X3D_reader.cxx
  X3D_tokens.cxx)

set(private_headers
  X3D.hxx
  X3D_reader.hxx
  X3D_tokens.hxx)

vtk_module_add_module(
  LANLX3DReader::vtkLANLX3DReader
  CLASSES ${classes}
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})

paraview_add_server_manager_xmls(
  XMLS LANLX3DReader.xml)
