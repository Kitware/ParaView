paraview_add_plugin(SurfaceLIC
  VERSION "2.0"
  DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc"
  MODULES SurfaceLIC::vtkSurfaceLICRepresentations
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Representations/vtk.module")
