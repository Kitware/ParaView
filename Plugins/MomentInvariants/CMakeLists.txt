include_directories(${VTK_INCLUDE_DIRS})

if (Module_vtkFiltersMomentInvariants)
  add_paraview_plugin(
    MomentInvariants
    "1.0"
    SERVER_MANAGER_XML MomentInvariants.xml
  )

  # Add testing if necessary
  # if(BUILD_TESTING)
  #   add_subdirectory(Testing)
  # endif()
endif()
