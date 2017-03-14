cmake_minimum_required(VERSION 3.3)

IF (NOT ParaView_BINARY_DIR)
  FIND_PACKAGE(ParaView REQUIRED)
  INCLUDE(${PARAVIEW_USE_FILE})
ENDIF ()

add_paraview_plugin(LagrangianIntegrationModelExample "1.0"
  SERVER_MANAGER_XML LagrangianIntegrationModelExample.xml
  SERVER_MANAGER_SOURCES vtkLagrangianIntegrationModelExample.cxx
  )

target_link_libraries(LagrangianIntegrationModelExample LINK_PRIVATE vtkFiltersFlowPaths)
