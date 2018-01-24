cmake_minimum_required(VERSION 3.3)

if (NOT ParaView_BINARY_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

include(ParaViewPlugins)

include_directories(${VTK_INCLUDE_DIRS})

add_paraview_plugin(DigitalRockPhysics "1.0"
  SERVER_MANAGER_XML DigitalRockPhysics.xml
  SERVER_MANAGER_SOURCES vtkPMaterialClusterAnalysisFilter.cxx
                         vtkPMaterialClusterExplodeFilter.cxx)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
