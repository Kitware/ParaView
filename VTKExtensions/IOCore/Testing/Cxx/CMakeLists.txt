vtk_add_test_cxx(vtkPVVTKExtensionsIOCoreCxxTests tests
  NO_VALID NO_OUTPUT
  TestPVDArraySelection.cxx
  )

if (PARAVIEW_USE_MPI AND TARGET VTK::IOInfovis AND TARGET VTK::TestingRendering)
  vtk_add_test_mpi(vtkPVVTKExtensionsIOCoreCxxTests tests
    TESTING_DATA NO_VALID
    TestCSVWriter.cxx
    )
endif()
vtk_test_cxx_executable(vtkPVVTKExtensionsIOCoreCxxTests tests)
