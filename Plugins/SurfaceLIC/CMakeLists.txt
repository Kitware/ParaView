include_directories(
  ${VTK_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  )

if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  add_definitions(-DVTKGL2)
endif()

add_paraview_plugin(
  SurfaceLIC "2.0"
  DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc"
  SERVER_MANAGER_XML vtkSurfaceLICRepresentation.xml
  SERVER_MANAGER_SOURCES vtkSurfaceLICRepresentation.cxx
  )
if("${VTK_RENDERING_BACKEND}" STREQUAL "OpenGL2")
  target_link_libraries(SurfaceLIC
    LINK_PRIVATE vtkRenderingLICOpenGL2)
endif()

if (VTK_RENDERINGPARALLELLIC_SURFACELICPAINTER_TIMER)
  add_definitions("-DvtkSurfaceLICPainterTIME")
endif()
if (VTK_RENDERINGPARALLELLIC_LINEINTEGRALCONVLOLUTION2D_TIMER)
  add_definitions("-DvtkLineIntegralConvolution2DTIME")
endif()

# because these tests can be a little flakey provide the
# option to disable them on problematic systems.
SET(SURFACELIC_PLUGIN_TESTING
  ON CACHE BOOL
  "Explicitly disable SurfaceLIC plugin ctests"
  )
mark_as_advanced(SURFACELIC_PLUGIN_TESTING)

if (BUILD_TESTING AND SURFACELIC_PLUGIN_TESTING AND PARAVIEW_ENABLE_PYTHON)
  add_subdirectory(Testing/Python)
endif()
