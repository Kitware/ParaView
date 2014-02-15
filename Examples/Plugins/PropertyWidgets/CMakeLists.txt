# This plugin demonstrates creating new types pqPropertyWidget and
# pqPropertyWidgetDecorator instances for customizing the Properties panel.
cmake_minimum_required(VERSION 2.8.8)

if (NOT ParaView_BINARY_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()


if (PARAVIEW_BUILD_QT_GUI)
  if (PARAVIEW_QT_VERSION VERSION_GREATER "4")
    qt5_wrap_cpp(moc_srcs
      pqMyPropertyWidgetForProperty.h
      pqMyPropertyWidgetForGroup.h
      pqMyPropertyWidgetDecorator.h)
  else ()
    qt4_wrap_cpp(moc_srcs
      pqMyPropertyWidgetForProperty.h
      pqMyPropertyWidgetForGroup.h
      pqMyPropertyWidgetDecorator.h)
  endif ()

  set (outifaces0)
  set (outsrcs0)
  add_paraview_property_widget(outifaces0 outsrcs0
    TYPE "my_property_widget_type"
    CLASS_NAME pqMyPropertyWidgetForProperty)

  set (outifaces1)
  set (outsrcs1)
  add_paraview_property_group_widget(outifaces1 outsrcs1
    TYPE "my_property_group_type"
    CLASS_NAME pqMyPropertyWidgetForGroup)

  set (outifaces2)
  set (outsrcs2)
  add_paraview_property_widget_decorator(outifaces2 outsrcs2
    TYPE "my_decorator"
    CLASS_NAME pqMyPropertyWidgetDecorator)

  # Now, create the plugin.
  add_paraview_plugin(ExamplesPropertyWidgets "1.0"
                      SERVER_MANAGER_XML PropertyWidgetsFilter.xml
                      GUI_INTERFACES ${outifaces0}
                                     ${outifaces1}
                                     ${outifaces2}
                      SOURCES ${outsrcs0}
                              ${outsrcs1}
                              ${outsrcs2}
                              ${moc_srcs}
                              pqMyPropertyWidgetForProperty.cxx
                              pqMyPropertyWidgetForGroup.cxx
                              pqMyPropertyWidgetDecorator.cxx)
endif()
