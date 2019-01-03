paraview_add_plugin(NetCDFTimeAnnotationPlugin
  REQUIRED_ON_SERVER
  VERSION             "1.0"
  SERVER_MANAGER_XML  NetCDFTimeAnnotation.xml)

set_property(GLOBAL APPEND
  PROPERTY
    vtk_required_python_modules "netcdftime")

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
