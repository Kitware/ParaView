vtk_add_test_cxx(vtkPVVTKExtensionsCoreCxxTests tests
  NO_VALID NO_OUTPUT
  TestDataUtilities.cxx
  TestDistributedTrivialProducer.cxx
  TestFileSequenceParser.cxx
  TestTrivialProducer.cxx)

vtk_test_cxx_executable(vtkPVVTKExtensionsCoreCxxTests tests)
