vtk_module_scan(
  MODULE_FILES      "${CMAKE_CURRENT_SOURCE_DIR}/LagrangianExample/vtk.module"
  REQUEST_MODULES   LagrangianExample
  PROVIDES_MODULES  provided_modules)

vtk_module_build(MODULES ${provided_modules})

paraview_add_plugin(LagrangianIntegrationModel
  VERSION "1.0"
  MODULES LagrangianExample)
