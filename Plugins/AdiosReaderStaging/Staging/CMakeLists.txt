vtk_module_find_package(PACKAGE ADIOS1)

set(classes
  vtkCartisoReader)

vtk_module_add_module(AdiosReaderStaging::Staging
  CLASSES ${classes})

vtk_module_link(AdiosReaderStaging::Staging
  PRIVATE
    adios1::adios1)

paraview_add_server_manager_xmls(
  XMLS  AdiosReaderPixie.xml)
