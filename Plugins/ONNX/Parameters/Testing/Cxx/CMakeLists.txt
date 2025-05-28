vtk_add_test_cxx(vtkONNXPluginParametersCxxTests tests
  NO_DATA NO_OUTPUT NO_VALID
  TestJsonParameters.cxx
  TestJsonVerify.cxx
)

vtk_test_cxx_executable(vtkONNXPluginParametersCxxTests tests)
