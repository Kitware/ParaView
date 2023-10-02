add_subdirectory(Cxx)

ExternalData_Expand_Arguments("ParaViewData" _
  "DATA{${paraview_test_data_directory_input}/Data/transient_sphere.hdf}")

include(FindPythonModules)
find_python_module(numpy numpy_found)
find_python_module(mpi4py mpi4py_found)
if (NOT PARAVIEW_USE_PYTHON OR NOT numpy_found)
  message(STATUS "Cannot test plugin DigitalSignalProcessing, missing Python + numpy support")
  return()
endif()

# PODT based and spectrogram tests do not support distributed computing
set (tests_with_baselines_no_cs
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPBandFiltering.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPMeanPowerSpectralDensity.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPMergeReduceTables.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPProjectSpectrumMagnitude.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPSoundQuantities.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPSpectrogram.xml")

paraview_add_client_tests(
  LOAD_PLUGIN "DigitalSignalProcessing"
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS "${tests_with_baselines_no_cs}")
set (DSPSoundQuantities_FORCE_SERIAL TRUE)

# DimensionBrowser does not support distributed data
# TemporalMultiplexing is checking actual values in the spreadsheet, which is not resilient with distributed
set (tests_without_baselines_no_cs
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPBandFilteringOnMultiplexer.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPDimensionBrowser.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPTemporalMultiplexing.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPTableFFT.xml")

paraview_add_client_tests(
  LOAD_PLUGIN "DigitalSignalProcessing"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS "${tests_without_baselines_no_cs}")

set (tests_with_baselines
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPMeanPowerSpectralDensityMultiplex.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPMergeReduceTablesMultiplex.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPProjectSpectrumMagnitudeMultiplex.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPSoundQuantitiesMultiplex.xml")

paraview_add_client_tests(
  LOAD_PLUGIN "DigitalSignalProcessing"
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS "${tests_with_baselines}")
paraview_add_client_server_tests(
  LOAD_PLUGIN "DigitalSignalProcessing"
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS "${tests_with_baselines}")
paraview_add_client_server_render_tests(
  LOAD_PLUGIN "DigitalSignalProcessing"
  BASELINE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Data/Baseline"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS "${tests_with_baselines}")

set (tests_without_baselines
  "${CMAKE_CURRENT_SOURCE_DIR}/DSPDistributedDimensionBrowser.xml"
  "${CMAKE_CURRENT_SOURCE_DIR}/DistributedDSPTableFFT.xml")

paraview_add_client_server_tests(
  LOAD_PLUGIN "DigitalSignalProcessing"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS "${tests_without_baselines}")

paraview_add_client_server_render_tests(
  LOAD_PLUGIN "DigitalSignalProcessing"
  TEST_DATA_TARGET ParaViewData
  TEST_SCRIPTS "${tests_without_baselines}")

# add test dependant to Qt
if (PARAVIEW_USE_QT AND PARAVIEW_PLUGIN_dsp_enable_audio_player AND numpy_found AND mpi4py_found)
  paraview_add_client_tests(
    LOAD_PLUGIN "DigitalSignalProcessing"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/DSPAudioPlayer.xml")
  paraview_add_client_server_tests(
    LOAD_PLUGIN "DigitalSignalProcessing"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/DSPAudioPlayer.xml")
  paraview_add_client_server_render_tests(
    LOAD_PLUGIN "DigitalSignalProcessing"
    TEST_DATA_TARGET ParaViewData
    TEST_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/DSPAudioPlayer.xml")
endif()
