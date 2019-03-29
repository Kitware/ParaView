paraview_add_plugin(MooseXfemClip
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES MooseXfemClip::vtkMooseXfemClip
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module"
  DOCUMENTATION_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
