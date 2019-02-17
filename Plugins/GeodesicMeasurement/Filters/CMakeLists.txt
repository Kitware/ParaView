set(classes
  vtkFastMarchingGeodesicDistance
  vtkFastMarchingGeodesicPath
  vtkGeodesicsBetweenPoints
  vtkPolyDataGeodesicDistance)

vtk_module_add_module(GeodesicMeasurement::GeodesicMeasurementFilters
  CLASSES ${classes})

vtk_module_link(GeodesicMeasurement::GeodesicMeasurementFilters
  PRIVATE FmmMesh)

paraview_add_server_manager_xmls(
  XMLS GeodesicMeasurement.xml)

add_subdirectory(FmmMesh)
