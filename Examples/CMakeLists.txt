cmake_minimum_required(VERSION 3.3)
project(ParaViewExamples)

option(ENABLE_CATALYST "Enable Catalyst Examples" OFF)

set(paraview_components)
if (ENABLE_CATALYST)
  list(APPEND paraview_components vtkPVPythonCatalyst)
endif ()

if (paraview_components)
  list(INSERT paraview_components 0 COMPONENTS)
endif ()
find_package(ParaView REQUIRED ${paraview_components})
include(${PARAVIEW_USE_FILE})

add_subdirectory(Plugins/Filter)
add_subdirectory(Plugins/ParametricSource)
add_subdirectory(Plugins/SMProxy)
add_subdirectory(Plugins/Representation)
add_subdirectory(Plugins/HiddenLinesRemoval)

if (ENABLE_CATALYST)
  add_subdirectory(Catalyst)
endif ()

if (PARAVIEW_BUILD_QT_GUI)
  add_subdirectory(Plugins/Autostart)
  add_subdirectory(Plugins/DockWidget)
  add_subdirectory(Plugins/GUIToolBar)
  add_subdirectory(Plugins/LagrangianIntegrationModel)
  add_subdirectory(Plugins/PropertyWidgets)
  add_subdirectory(Plugins/Reader)
  add_subdirectory(Plugins/ReaderXMLOnly)
  add_subdirectory(Plugins/RenderPassViews)
  # TODO: update this plugin to use the pipeline controller instead.
  #add_subdirectory(Plugins/RepresentationBehavior)
  add_subdirectory(Plugins/SourceToolbar)
  add_subdirectory(Plugins/Writer)

  add_subdirectory(CustomApplications/Clone1)
  add_subdirectory(CustomApplications/Clone2)
  add_subdirectory(CustomApplications/Demo0)
  add_subdirectory(CustomApplications/Demo1)
  add_subdirectory(CustomApplications/MultiServerClient)
# TODO Update this custom application to load only specific data types.
#  add_subdirectory(CustomApplications/Spreadsheet) 
endif()

if (TARGET vtkIOVisItBridge)
  # add_subdirectory(Plugins/VisItReader)
endif ()
