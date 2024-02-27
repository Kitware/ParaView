paraview_add_plugin(NetCDFTimeAnnotationPlugin
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION             "1.0"
  SERVER_MANAGER_XML  NetCDFTimeAnnotation.xml)

include(ParaViewFindPythonModules)

# First look for the module cftime
find_python_module(cftime module_found)
if (module_found)
  set(module_name "cftime")
else ()
  # If cftime is not found, search for netcdftime
  find_python_module(netcdftime module_found)
  if (module_found)
    set(module_name "netcdftime")
  endif ()
endif ()

if (module_found)
  set_property(GLOBAL APPEND
    PROPERTY
      vtk_required_python_modules "${module_name}")
  if (BUILD_TESTING)
    add_subdirectory(Testing)
  endif ()
else ()
  message(FATAL_ERROR
    "The Python module cftime (preferred) or netcdftime is required to build this plugin.")
endif ()
