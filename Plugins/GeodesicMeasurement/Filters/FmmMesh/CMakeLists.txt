project( FmmMesh )

set(CMAKE_CXX_STANDARD 11)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/vtkFmmMeshConfig.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/vtkFmmMeshConfig.h)

set( ${PROJECT_NAME}_SRCS
     gw_core/GW_Config.cpp
     gw_core/GW_Face.cpp
     gw_core/GW_FaceIterator.cpp
     gw_core/GW_Mesh.cpp
     gw_core/GW_SmartCounter.cpp
     gw_core/GW_Vertex.cpp
     gw_core/GW_VertexIterator.cpp
     gw_geodesic/GW_GeodesicFace.cpp
     gw_geodesic/GW_GeodesicMesh.cpp
     gw_geodesic/GW_GeodesicPath.cpp
     gw_geodesic/GW_GeodesicPoint.cpp
     gw_geodesic/GW_GeodesicVertex.cpp
     gw_geodesic/GW_TriangularInterpolation_Linear.cpp
     gw_geodesic/GW_TriangularInterpolation_Quadratic.cpp
     gw_geodesic/GW_TriangularInterpolation_Cubic.cpp
)

set( ${PROJECT_NAME}_HDRS
     gw_core/GW_Config.h
     gw_core/GW_Face.h
     gw_core/GW_Mesh.h
     gw_core/GW_FaceIterator.h
     gw_core/GW_Vertex.h
     gw_core/GW_VertexIterator.h
     gw_core/GW_SmartCounter.h
     gw_geodesic/GW_GeodesicFace.h
     gw_geodesic/GW_GeodesicMesh.h
     gw_geodesic/GW_GeodesicPath.h
     gw_geodesic/GW_GeodesicPoint.h
     gw_geodesic/GW_GeodesicVertex.h
     gw_geodesic/GW_TriangularInterpolation_Linear.h
     gw_geodesic/GW_TriangularInterpolation_Quadratic.h
     gw_geodesic/GW_TriangularInterpolation_Cubic.h
     ${CMAKE_CURRENT_BINARY_DIR}/vtkFmmMeshConfig.h
)

add_library( ${PROJECT_NAME} STATIC
  ${${PROJECT_NAME}_SRCS}
  ${${PROJECT_NAME}_HDRS} )
_vtk_module_apply_properties(${PROJECT_NAME})
_vtk_module_install(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gw_core>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gw_geodesic>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/gw_maths>")
set_property(TARGET "${PROJECT_NAME}"
  PROPERTY
    POSITION_INDEPENDENT_CODE ON)
