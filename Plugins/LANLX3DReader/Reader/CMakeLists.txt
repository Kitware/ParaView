

set (sources
    X3D_reader.cxx
    X3D_tokens.cxx
    )

set (private_headers
    X3D.hxx
    X3D_reader.hxx
    X3D_tokens.hxx
    )

VTK_MODULE_ADD_MODULE(
  LANLX3DReader::vtkLANLX3DReader
  CLASSES vtkLANLX3DReader
  SOURCES ${sources}
  PRIVATE_HEADERS ${private_headers})

PARAVIEW_ADD_SERVER_MANAGER_XMLS(
  XMLS LANLX3DReader.xml)
