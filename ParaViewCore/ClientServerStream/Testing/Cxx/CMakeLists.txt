vtk_add_test_cxx(vtkClientServerCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  coverClientServer.cxx
  )
vtk_test_cxx_executable(vtkClientServerCxxTests tests)
