set(interfaces)
set(sources
  pqMyPropertyWidgetDecorator.cxx
  pqMyPropertyWidgetDecorator.h
  pqMyPropertyWidgetForGroup.cxx
  pqMyPropertyWidgetForGroup.h
  pqMyPropertyWidgetForProperty.cxx
  pqMyPropertyWidgetForProperty.h)

paraview_plugin_add_property_widget(
  KIND WIDGET
  TYPE "my_property_widget_type"
  CLASS_NAME pqMyPropertyWidgetForProperty
  INTERFACES property_interfaces
  SOURCES property_sources)
list(APPEND interfaces
  ${property_interfaces})
list(APPEND sources
  ${property_sources})

paraview_plugin_add_property_widget(
  KIND GROUP_WIDGET
  TYPE "my_property_group_type"
  CLASS_NAME pqMyPropertyWidgetForGroup
  INTERFACES property_interfaces
  SOURCES property_sources)
list(APPEND interfaces
  ${property_interfaces})
list(APPEND sources
  ${property_sources})

paraview_plugin_add_property_widget(
  KIND WIDGET_DECORATOR
  TYPE "my_decorator"
  CLASS_NAME pqMyPropertyWidgetDecorator
  INTERFACES property_interfaces
  SOURCES property_sources)
list(APPEND interfaces
  ${property_interfaces})
list(APPEND sources
  ${property_sources})

paraview_add_plugin(PropertyWidgets
  VERSION "1.0"
  UI_INTERFACES ${interfaces}
  SOURCES ${sources}
  SERVER_MANAGER_XML PropertyWidgetsFilter.xml)
