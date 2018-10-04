#--------------------------------------------------
# Find and Use ParaView
#--------------------------------------------------
if(NOT ParaView_SOURCE_DIR)
  find_package(ParaView)
  include("${PARAVIEW_USE_FILE}")
  include (ParaViewPlugins)
else()
  # we're building from within ParaView Source.
endif()

include_directories(
  ${VTK_INCLUDE_DIRS}
  ${PARAVIEW_INCLUDE_DIRS})

if (PARAVIEW_ENABLE_PYTHON)
  include(FindPythonModules)
  find_python_module(netcdftime netcdftime_found)

  if(NOT netcdftime_found)
    message(STATUS "netcdftime Python module not found! NetCDFTimeAnnotationPlugin not be available until it is installed.")
  else()
    add_paraview_plugin(NetCDFTimeAnnotationPlugin "1.0"
      SERVER_MANAGER_XML NetCDFTimeAnnotation.xml
      REQUIRED_ON_SERVER)

    if(BUILD_TESTING)
      add_subdirectory(Testing)
    endif()
  endif()
endif()
