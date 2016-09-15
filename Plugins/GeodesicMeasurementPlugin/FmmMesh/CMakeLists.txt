project( FmmMesh )

set( vtkFmmMesh_THIRD_PARTY 1)
set(vtkFmmMesh_LIBRARIES FmmMesh)

set( VTKFMMMESH_BUILD_SHARED_LIBS OFF )
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
)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/gw_core
                     ${CMAKE_CURRENT_SOURCE_DIR}/gw_geodesic
                     ${CMAKE_CURRENT_BINARY_DIR} )

add_library( ${PROJECT_NAME} STATIC
  ${${PROJECT_NAME}_SRCS}
  ${${PROJECT_NAME}_HDRS} )

if (NOT WIN32)
  set_property(TARGET ${PROJECT_NAME} APPEND
    PROPERTY COMPILE_FLAGS "-fPIC")
endif()
