set(classes
  vtkVDFReader)

find_package(Vapor QUIET REQUIRED)

vtk_module_add_module(VDFReaderPlugin::vtkVaporReaders
  CLASSES ${classes})
vtk_module_link(VDFReaderPlugin::vtkVaporReaders
  PUBLIC
    Vapor::vdf)

paraview_add_server_manager_xmls(
  XMLS  VDFReader.xml)
