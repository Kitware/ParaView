#--------------------------------------------------------------------------------
vtk_module_test_executable(
  TestTransferFunctionManager
  TestTransferFunctionManager.cxx)
add_test(NAME TestTransferFunctionManager COMMAND TestTransferFunctionManager)
set_tests_properties(TestTransferFunctionManager PROPERTIES LABELS "PARAVIEW")

#--------------------------------------------------------------------------------
