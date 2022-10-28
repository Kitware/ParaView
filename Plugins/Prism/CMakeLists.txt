set(ui_files)
set(qrc_files)
set(interfaces)
set(sources)
if (PARAVIEW_USE_QT)
  list(APPEND qrc_files Resources/Prism.qrc)
  list(APPEND ui_files pqSESAMEConversionsPanelWidget.ui)
  list(APPEND sources pqSESAMEConversionsPanelWidget.h pqSESAMEConversionsPanelWidget.cxx)
  paraview_plugin_add_property_widget(
    KIND GROUP_WIDGET
    TYPE "SESAMEConversions"
    CLASS_NAME pqSESAMEConversionsPanelWidget
    INTERFACES property_interfaces
    SOURCES property_sources)
  list(APPEND interfaces ${property_interfaces})
  list(APPEND sources ${property_sources})
endif ()

set(modules
  Prism::vtkPrismFilters
  Prism::vtkPrismReaders
  Prism::vtkPrismServerManager
  Prism::vtkPrismViews
  )

set(module_files
  "${CMAKE_CURRENT_SOURCE_DIR}/Filters/vtk.module"
  "${CMAKE_CURRENT_SOURCE_DIR}/Readers/vtk.module"
  "${CMAKE_CURRENT_SOURCE_DIR}/ServerManager/vtk.module"
  "${CMAKE_CURRENT_SOURCE_DIR}/Views/vtk.module"
  )

paraview_add_plugin(Prism
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "2.0"
  UI_FILES ${ui_files}
  UI_INTERFACES ${interfaces}
  UI_RESOURCES ${qrc_files}
  SOURCES ${sources}
  MODULES ${modules}
  MODULE_FILES ${module_files}
  XML_DOCUMENTATION OFF
  )

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif ()
