if(ParaView_SOURCE_DIR)
  include_directories(
    ${VTK_INCLUDE_DIRS}
  )
else()
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

add_paraview_plugin(VTKmFilters "0.1"
  REQUIRED_ON_SERVER
  SERVER_MANAGER_XML VTKmSM.xml
  )
target_link_libraries(VTKmFilters LINK_PRIVATE vtkAcceleratorsVTKm)

# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
