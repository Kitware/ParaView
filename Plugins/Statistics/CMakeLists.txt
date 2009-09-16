# This plugin contains statistics tools for scientific datasets (i.e.,
# structured and unstructured meshes with point and/or cell data).

set( PLUGIN_NAME Statistics )
set( PLUGIN_VERSION "1.0" )

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${PARAVIEW_INCLUDE_DIRS}
  ${VTK_INCLUDE_DIR}
  ${PARAVIEW_SOURCE_DIR}/Utilities/VTKPythonWrapping/Executable
  ${pqComponents_BINARY_DIR}
  ${pqComponents_SOURCE_DIR}
  ${pqCore_BINARY_DIR}
  ${pqCore_SOURCE_DIR}
  ${QtWidgets_BINARY_DIR}
  ${QtWidgets_SOURCE_DIR}
)

set( SM_SRC
  vtkSciVizStatistics.cxx
  vtkPSciVizContingencyStats.cxx
  vtkPSciVizDescriptiveStats.cxx
  vtkPSciVizMultiCorrelativeStats.cxx
  vtkPSciVizPCAStats.cxx
  vtkPSciVizKMeans.cxx
)

set( SM_XML
  SciVizStatisticsSM.xml
)

set( UI_XML
  SciVizStatisticsUI.xml
)

set_source_files_properties(
  vtkSciVizStatistics.cxx
  PROPERTIES
    ABSTRACT TRUE
)

if ( PARAVIEW_BUILD_QT_GUI )
  include( ${QT_USE_FILE} )
  set( GUI_XML "GUI_RESOURCE_FILES;${UI_XML}" )
endif ( PARAVIEW_BUILD_QT_GUI )

add_paraview_plugin( ${PLUGIN_NAME} ${PLUGIN_VERSION}
  SERVER_MANAGER_XML ${SM_XML}
  SERVER_MANAGER_SOURCES ${SM_SRC}
  ${GUI_XML}
)

