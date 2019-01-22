vtk_add_test_cxx(vtkPVCommonCxxTests tests
  NO_DATA NO_VALID NO_OUTPUT
  ParaViewCoreCommonPrintSelf.cxx
  )
vtk_test_cxx_executable(vtkPVCommonCxxTests tests)
