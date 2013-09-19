include_directories(
  ${VTK_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

add_paraview_plugin(
  SurfaceLIC "2.0"
  DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc"
  SERVER_MANAGER_XML vtkSurfaceLICRepresentation.xml
  SERVER_MANAGER_SOURCES vtkSurfaceLICRepresentation.cxx
  )

set(SurfaceLIC_DEPENDENCIES vtkRenderingLIC)
if (PARAVIEW_USE_MPI)
  list(APPEND SurfaceLIC_DEPENDENCIES vtkRenderingParallelLIC)
endif()
target_link_libraries(SurfaceLIC ${SurfaceLIC_DEPENDENCIES})

if (VTK_RENDERINGPARALLELHYBRIDOPENGL_SURFACELICPAINTER_TIMER)
  add_definitions("-DvtkSurfaceLICPainterTIME")
endif()
if (VTK_RENDERINGPARALLELHYBRIDOPENGL_LINEINTEGRALCONVLOLUTION2D_TIMER)
  add_definitions("-DvtkLineIntegralConvolution2DTIME")
endif()

if (PARAVIEW_ENABLE_PYTHON AND BUILD_TESTING)
  add_subdirectory(Testing/Python)
endif()
