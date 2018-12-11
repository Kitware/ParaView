vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/Representations/vtk.module"
  REQUEST_MODULES   EmbossingRepresentations::vtkEmbossingRepresentations
  PROVIDES_MODULES  embossing_modules
  REQUIRES_MODULES  required_modules
  HIDE_MODULES_FROM_CACHE ON)

if (required_modules)
  foreach (required_module IN LISTS required_modules)
    if (NOT TARGET "${required_module}")
      message(FATAL_ERROR
        "Failed to find the required module ${required_module}.")
    endif ()
  endforeach ()
endif ()

vtk_module_build(
  MODULES             ${embossing_modules}
  PACKAGE             EmbossingRepresentations
  INSTALL_HEADERS     OFF
  LIBRARY_NAME_SUFFIX "pv${PARAVIEW_VERSION_MAJOR}.${PARAVIEW_VERSION_MINOR}"
  VERSION             "${PARAVIEW_VERSION}"
  SOVERSION           "1")

set(interfaces)
set(sources)
if (PARAVIEW_BUILD_QT_GUI)
  paraview_plugin_add_property_widget(
    KIND        WIDGET_DECORATOR
    TYPE        ExtrusionPropertyWidgetDecorator
    CLASS_NAME  pqExtrusionPropertyWidgetDecorator
    INTERFACES  pwi_interfaces
    SOURCES     pwi_sources)
  list(APPEND interfaces
    ${pwi_interfaces})
  list(APPEND sources
    ${pwi_sources}
    pqExtrusionPropertyWidgetDecorator.cxx
    pqExtrusionPropertyWidgetDecorator.h)
endif ()

paraview_add_plugin(EmbossingRepresentations
  VERSION       "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES       ${sources}
  MODULES       EmbossingRepresentations::vtkEmbossingRepresentations
  XML_DOCUMENTATION OFF)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
