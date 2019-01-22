vtk_add_test_cxx(vtkPVClientServerCorePythonCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  ParaViewCoreClientServerPythonPrintSelf.cxx
  )

vtk_test_cxx_executable(vtkPVClientServerCorePythonCxxTests tests)
