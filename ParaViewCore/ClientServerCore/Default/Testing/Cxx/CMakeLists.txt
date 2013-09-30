SET(TestNames
  ParaViewCoreClientServerCorePrintSelf
  TestMPI
  TestSystemCaps
  )

vtk_module_test_executable(${vtk-module}PrintSelf
  ParaViewCoreClientServerCorePrintSelf.cxx)
add_test(NAME ${vtk-module}PrintSelf
  COMMAND ${vtk-module}PrintSelf)

vtk_module_test_executable(TestMPI TestMPI.cxx)
add_test(NAME TestMPI COMMAND TestMPI)

vtk_module_test_executable(TestSystemCaps TestSystemCaps.cxx)
add_test(NAME TestSystemCaps COMMAND TestSystemCaps)

if (PARAVIEW_USE_MPI)
  set_source_files_properties(TestSystemCaps
    PROPERTIES COMPILE_DEFINITIONS TEST_MPI_CAPS)
  vtk_mpi_link(TestSystemCaps)
endif()
if (PARAVIEW_ENABLE_PYTHON)
  set_source_files_properties(TestSystemCaps
    PROPERTIES COMPILE_DEFINITIONS TEST_PY_CAPS)
endif()

set_tests_properties(
  ${vtk-module}PrintSelf
  TestMPI
  TestSystemCaps
  PROPERTIES LABELS "PARAVIEW"
  )
