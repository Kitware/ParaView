vtk_module_find_package(PACKAGE ADIOS1)

set(classes
  vtkAdiosPixieReader)

set(private_headers
  vtkAdiosInternals.h)

vtk_module_add_module(AdiosReaderPixie::Pixie
  CLASSES ${classes}
  PRIVATE_HEADERS ${private_headers})

vtk_module_link(AdiosReaderStaging::Pixie
  PRIVATE
    adios1::adios1)

paraview_add_server_manager_xmls(
  XMLS  AdiosReaderPixie.xml)
