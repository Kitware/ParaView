vtk_add_test_cxx(vtkPVCoreCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  ParaViewCoreCorePrintSelf.cxx
  )
vtk_test_cxx_executable(vtkPVCoreCxxTests tests)
