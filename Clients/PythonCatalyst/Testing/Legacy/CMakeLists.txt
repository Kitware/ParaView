#---------------------------------------------------------------------
# Tests for legacy Catalyst analysis pipeline scripts.
set(legacy_catalyst_pipelines
  # test to see if the input is changing, i.e. that the initial
  # pipeline is having it's trivial producer updated with a new grid
  TestInput.py

  # test if a given Catalyst analsyis script can import another Python script
  cpimport.py

  # test if we can use a Python programmable filter in a Catalyst Python script
  cpprogrammablefilter.py

  PythonScriptTest.py
  )

if (PARAVIEW_PLUGIN_ENABLE_VTKmFilters AND PARAVIEW_USE_VTKM)
  list(APPEND legacy_catalyst_pipelines
    CatalystLoadVTKmFilterPlugin.py)
endif()

paraview_add_test(
  PREFIX "Catalyst::Legacy::WaveletMiniApp"
  ARGS "$<TARGET_FILE:ParaView::pvbatch>" -sym -m paraview.demos.wavelet_miniapp -t 4 --size 21 -s
  TEST_SCRIPTS ${legacy_catalyst_pipelines})

if (PARAVIEW_USE_MPI)
  paraview_add_test_mpi(
    PREFIX "Catalyst::Legacy::MPI::WaveletMiniApp"
    ARGS "$<TARGET_FILE:ParaView::pvbatch>" -sym -m paraview.demos.wavelet_miniapp -t 4 --size 21 -s
    TEST_SCRIPTS ${legacy_catalyst_pipelines})
endif()

# FIXME: call the following for all tests.
# set_tests_properties(CoProcessingProgrammableFilter PROPERTIES PASS_REGULAR_EXPRESSION "in DoCoProcessing")
