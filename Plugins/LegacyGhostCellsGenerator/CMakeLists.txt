set(modules
  LegacyGhostCellsGeneratorFilters)

set(module_files
  "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module")

if (TARGET VTK::ParallelMPI)
  list(APPEND modules
    LegacyGhostCellsGeneratorParallel)
  list(APPEND module_files
    "${CMAKE_CURRENT_SOURCE_DIR}/Parallel/vtk.module")

endif()

paraview_add_plugin(LegacyGhostCellsGenerator
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES ${modules}
  MODULE_FILES ${module_files})

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
