paraview_add_plugin(LagrangianIntegrationModel
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "1.0"
  MODULES LagrangianExample
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/LagrangianExample/vtk.module")
