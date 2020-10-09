set(classes
  vtkGeoMapConvertFilter
  vtkGeoMapFetcher)

vtk_module_find_package(
  PACKAGE CURL)

vtk_module_add_module(GeographicalMap::vtkGeographicalMap
  CLASSES ${classes})

vtk_module_link(GeographicalMap::vtkGeographicalMap
  PRIVATE CURL::libcurl)

paraview_add_server_manager_xmls(
  XMLS GeographicalMapServerManager.xml)
