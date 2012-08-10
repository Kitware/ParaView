SET(TestNames
  ParaViewCoreClientServerCorePrintSelf 
  TestMPI
  )

vtk_module_test_executable(${vtk-module}PrintSelf
  ParaViewCoreClientServerCorePrintSelf.cxx)
add_test(NAME ${vtk-module}PrintSelf
         COMMAND ${vtk-module}PrintSelf)

vtk_module_test_executable(TestMPI TestMPI.cxx)
add_test(NAME TestMPI COMMAND TestMPI)
