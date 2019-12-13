set(interfaces)
set(sources)
if (PARAVIEW_USE_QT)
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
  MODULE_FILES  "${CMAKE_CURRENT_SOURCE_DIR}/Representations/vtk.module"
  XML_DOCUMENTATION OFF)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
