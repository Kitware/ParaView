
vtk_module_test_executable(
  ${vtk-module}CxxTests
  coverClientServer.cxx)
  
add_test(
  NAME vtkClientServerCoverage
  COMMAND ${vtk-module}CxxTests)
