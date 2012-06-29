vtk_module_test_executable(${vtk-module}PrintSelf
  ParaViewCoreCommonPrintSelf.cxx)

add_test(NAME ${vtk-module}PrintSelf
  COMMAND ${vtk-module}PrintSelf)
