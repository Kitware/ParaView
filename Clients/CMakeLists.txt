add_subdirectory(CommandLineExecutables)
if (PARAVIEW_USE_QT)
  # NOTE: Sync with the `paraview_will_be_built` calculation in `CMakeLists.txt`.
  if (WIN32 OR VTK_USE_X OR VTK_USE_COCOA)
    add_subdirectory(ParaView)
  else ()
    message(WARNING
      "Enabling Qt while not building with onscreen rendering support has "
      "limited support, The `paraview` Qt client will not be built.")
  endif ()
endif ()
