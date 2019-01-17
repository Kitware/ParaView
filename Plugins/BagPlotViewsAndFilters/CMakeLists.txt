cmake_minimum_required(VERSION 3.3)

if (NOT ParaView_BINARY_DIR)
  find_package(ParaView REQUIRED)
  include(${PARAVIEW_USE_FILE})
endif()

include(ParaViewPlugins)

include_directories(
  ${VTK_INCLUDE_DIRS}
)

add_paraview_plugin(BagPlotViewsAndFilters "1.0"
  GUI_RESOURCES BagPlotViewsAndFilters.qrc
  SERVER_MANAGER_XML BagPlotViewsAndFilters.xml
  SERVER_MANAGER_SOURCES 
  vtkPVExtractBagPlots.cxx
  vtkSMFunctionalBagChartSeriesSelectionDomain.cxx
  vtkSMBagChartSeriesListDomain.cxx
  vtkPVBagChartRepresentation.cxx
  vtkPVBagPlotMatrixView.cxx
  vtkPVBagPlotMatrixRepresentation.cxx)

if (BUILD_TESTING)
  add_subdirectory(Testing)
endif()
