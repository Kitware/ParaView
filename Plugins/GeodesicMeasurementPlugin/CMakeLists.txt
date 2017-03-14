project( GeodesicMeasurementPlugin )
cmake_minimum_required( VERSION 3.3 )

if (NOT ParaView_BINARY_DIR)
  find_package( ParaView REQUIRED )
  include( ${PARAVIEW_USE_FILE} )
endif()

add_subdirectory( FmmMesh )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/FmmMesh
  ${CMAKE_CURRENT_BINARY_DIR}/FmmMesh
  ${VTK_INCLUDE_DIRS}
  )

add_paraview_plugin( GeodesicMeasurement "1.0"
  SERVER_MANAGER_XML GeodesicMeasurement.xml
  SERVER_MANAGER_SOURCES
    vtkPolyDataGeodesicDistance.cxx
    vtkFastMarchingGeodesicDistance.cxx
    vtkFastMarchingGeodesicPath.cxx
    vtkGeodesicsBetweenPoints.cxx
)

target_link_libraries( GeodesicMeasurement LINK_PRIVATE FmmMesh )
