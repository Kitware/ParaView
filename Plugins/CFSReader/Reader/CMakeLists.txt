# the plugin uses hdf5 c-api and hl-api via vtk

vtk_module_add_module(CFSReader::Reader
  CLASSES vtkCFSReader
  SOURCES hdf5Common.cxx hdf5Reader.cxx
  PRIVATE_HEADERS hdf5Common.h hdf5Reader.h)

paraview_add_server_manager_xmls(XMLS CFSReader.xml)
