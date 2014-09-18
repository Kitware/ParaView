include_directories(${VTK_INCLUDE_DIRS})

add_paraview_plugin(
  InSituExodus
  "1.0"
  SERVER_MANAGER_XML InSituExodus.xml
)

# Add testing if necessary
if(BUILD_TESTING)
  add_subdirectory(Testing)
endif()
