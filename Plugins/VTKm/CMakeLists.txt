if(ParaView_SOURCE_DIR)
  include_directories(
    ${VTK_INCLUDE_DIRS}
  )
else()
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

# This plugin relies on classes in the vtkAcceleratorsVTKm VTK module, which
# is not built by default because the server manager XML points to classes
# in this module. Usually you could get this module to build by adding this
# module to a module.cmake file. However, we are not actually building our
# own source, so we can't really do it. For now, just instruct the user to
# turn it on manually.
if(NOT Module_vtkAcceleratorsVTKm)
  message(SEND_ERROR "VTKm plugin requires vtkAcceleratorsVTKm module. Please turn on Module_vtkAcceleratorsVTKm CMake option.")
endif()

add_paraview_plugin(VTKmFilters "0.1"
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  SERVER_MANAGER_XML VTKmSM.xml
  )
target_link_libraries(VTKmFilters LINK_PRIVATE vtkAcceleratorsVTKm)

# Add testing if necessary
if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
