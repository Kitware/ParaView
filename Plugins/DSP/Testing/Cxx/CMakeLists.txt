# Point to ParaView ExternalData when searching for test data (used in vtk_add_test_cxx)
set(_vtk_build_TEST_OUTPUT_DATA_DIRECTORY ${paraview_test_data_directory_output})

vtk_add_test_cxx(vtkDigitalSignalProcessingCxxTests tests
  NO_OUTPUT NO_VALID
  TestBandFiltering.cxx
  TestDSPTableFFT.cxx
  TestTemporalMultiplexing.cxx
)

set(_vtk_build_test "DigitalSignalProcessing::DSPFiltersPlugin")
vtk_test_cxx_executable(vtkDigitalSignalProcessingCxxTests tests)
