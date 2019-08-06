vtk_add_test_cxx(vtkPVVTKExtensionsDefaultCxxTests tests
  NO_VALID NO_OUTPUT NO_DATA
  TestFileSequenceParser.cxx
  )
vtk_add_test_cxx(vtkPVVTKExtensionsDefaultCxxTests tests
  NO_VALID NO_OUTPUT
  TestPVDArraySelection.cxx
  )

if (PARAVIEW_USE_MPI)
  vtk_add_test_mpi(vtkPVVTKExtensionsDefaultCxxTests tests
    TESTING_DATA NO_VALID
    TestCSVWriter.cxx
    )
endif()
vtk_test_cxx_executable(vtkPVVTKExtensionsDefaultCxxTests tests)
