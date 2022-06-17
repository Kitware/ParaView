paraview_add_plugin(StreamingParticles
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES StreamingParticles::vtkStreamingParticles
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Particles/vtk.module")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
