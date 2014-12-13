SET(PLUGIN_NAME NonOrthogonalSource)
SET(PLUGIN_VERSION "1.0")

include_directories(${VTK_INCLUDE_DIRS})

ADD_PARAVIEW_PLUGIN(NonOrthogonalSource "1.0"
     SERVER_MANAGER_XML vtkNonOrthogonalSource.xml
     SERVER_MANAGER_SOURCES
      vtkShearedWaveletSource.cxx
)

if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
